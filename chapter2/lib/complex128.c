/*******************************************************************************
	complex128.c -- Complex128 Class

	Author: Hironobu Inatsuka
*******************************************************************************/
#include <ruby.h>
#include <quadmath.h>
#include "abi.h"

char ool_quad2str(__float128 x, char format, int *exp, int *sign, char **buf);

struct C128 { __complex128 value; } ;

static void
free_complex128(void *v)
{
	if (v != NULL)
	{
		xfree(v);
	}
}

static size_t
memsize_complex128(const void *_)
{
	return sizeof(struct C128);
}

static const rb_data_type_t complex128_data_type = {
	"complex128",
	{0, free_complex128, memsize_complex128,},
	0, 0,
	RUBY_TYPED_FREE_IMMEDIATELY,
};

static VALUE
complex128_allocate(__complex128 x)
{
	struct C128 *ptr = ruby_xcalloc(1, sizeof(struct C128));
	ptr->value = x;
	return TypedData_Wrap_Struct(rb_cComplex128, &complex128_data_type, ptr);
}


VALUE
rb_complex128_cc128(__complex128 x)
{
	VALUE obj = complex128_allocate(x);
	return obj;
}

static VALUE
rb_complex128_inspect(VALUE self)
{
	char* str;
	struct C128 *c128;
	int exp, sign;
	VALUE retval = rb_str_new_cstr("(");
	
	TypedData_Get_Struct(self, struct C128, &complex128_data_type, c128);
	
	switch (ool_quad2str(crealq(c128->value), 'g', &exp, &sign, &str)) {
	case '0':
		rb_raise(rb_eRuntimeError, "error occured in ool_quad2str()");
		break;
	case '1':
		if (sign == -1)
			rb_str_concat(retval, rb_sprintf("-%s", str));
		else
			rb_str_concat(retval, rb_sprintf("%s", str));
		break;
	case 'e':
		if (sign == -1)
			rb_str_concat(retval, rb_sprintf("-%se%+d", str, exp));
		else
			rb_str_concat(retval, rb_sprintf("%se%+d", str, exp));
		break;
	case 'f':
		if (sign == -1)
			rb_str_concat(retval, rb_sprintf("-%s", str));
		else
			rb_str_concat(retval, rb_sprintf("%s", str));
		break;
	default:
		rb_raise(rb_eRuntimeError, "format error");
		break;
	}

	switch (ool_quad2str(cimagq(c128->value), 'g', &exp, &sign, &str)) {
	case '0':
		rb_raise(rb_eRuntimeError, "error occured in ool_quad2str()");
		break;
	case '1':
		if (sign == -1)
			rb_str_concat(retval, rb_sprintf("-%s*", str));
		else
			rb_str_concat(retval, rb_sprintf("+%s*", str));
		break;
	case 'e':
		if (sign == -1)
			rb_str_concat(retval, rb_sprintf("-%se%+d", str, exp));
		else
			rb_str_concat(retval, rb_sprintf("+%se%+d", str, exp));
		break;
	case 'f':
		if (sign == -1)
			rb_str_concat(retval, rb_sprintf("-%s", str));
		else
			rb_str_concat(retval, rb_sprintf("+%s", str));
		break;
	default:
		rb_raise(rb_eRuntimeError, "format error");
		break;
	}

	rb_str_cat(retval, "i)", 2);
	return retval;
}

#if 0
static VALUE
complex128_initialize_copy(VALUE self, VALUE other)
{
	struct C128 *pv = rb_check_typeddata(self, &complex128_data_type);
	struct C128 *x = rb_check_typeddata(other, &complex128_data_type);

	if (self != other) 
	{
		pv->value = x->value;
	}
	return self;
}
#endif

void
InitVM_Complex128(void)
{
    /* Global function */
//    rb_define_global_function("Complex128", f_Complex128, -1);
    
    /* Class methods */
    rb_undef_alloc_func(rb_cComplex128);
    rb_undef_method(CLASS_OF(rb_cComplex128), "new");
    
    rb_define_method(rb_cComplex128, "inspect", rb_complex128_inspect, 0);
    
    /* Constants */
    rb_define_const(rb_cComplex128, "I", rb_complex128_cc128(0+1i));
}
