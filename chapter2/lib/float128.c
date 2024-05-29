/*******************************************************************************
    float128.c -- Float128 Class
    
    Author: Hironobu Inatsuka
*******************************************************************************/
#include <ruby.h>
#include <quadmath.h>
#include "abi.h"

char ool_quad2str(__float128 x, char format, int *exp, int *sign, char **buf);

struct F128 { __float128 value; } ;

static void
free_float128(void *v)
{
	if (v != NULL)
	{
		xfree(v);
	}
}

static size_t
memsize_float128(const void *_)
{
	return sizeof(struct F128);
}

static const rb_data_type_t float128_data_type = {
	"float128",
	{0, free_float128, memsize_float128,},
	0, 0,
	RUBY_TYPED_FREE_IMMEDIATELY,
};

static VALUE
float128_allocate(const __float128 x)
{
	struct F128 *ptr = ruby_xcalloc(1, sizeof(struct F128));
	ptr->value = x;
	return TypedData_Wrap_Struct(rb_cFloat128, &float128_data_type, ptr);
}


VALUE
rb_float128_cf128(const __float128 x)
{
	VALUE obj = float128_allocate(x);
	return obj;
}

static VALUE
rb_float128_inspect(VALUE self)
{
	char* str;
	struct F128 *f128;
	int exp, sign;
	
	TypedData_Get_Struct(self, struct F128, &float128_data_type, f128);
	
	switch (ool_quad2str(f128->value, 'g', &exp, &sign, &str)) {
	case '0':
		rb_raise(rb_eRuntimeError, "error occured in ool_quad2str()");
		break;
	case '1':
		if (sign == -1)
			return rb_sprintf("-%s", str);
		else
			return rb_sprintf("%s", str);
		break;
	case 'e':
		if (sign == -1)
			return rb_sprintf("-%se%+d", str, exp);
		else
			return rb_sprintf("%se%+d", str, exp);
		break;
	case 'f':
		if (sign == -1)
			return rb_sprintf("-%s", str);
		else
			return rb_sprintf("%s", str);
		break;
	default:
		rb_raise(rb_eRuntimeError, "format error");
		break;
	}
	return rb_str_new(0,0);
}



#if 0
static VALUE
float128_initialize_copy(VALUE self, VALUE other)
{
	struct F128 *pv = rb_check_typeddata(self, &float128_data_type);
	struct F128 *x = rb_check_typeddata(other, &float128_data_type);

	if (self != other) 
	{
		pv->value = x->value;
	}
	return self;
}
#endif

void
InitVM_Float128(void)
{
    /* Global function */
//    rb_define_global_function("Float128", f_Float128, -1);
    
    /* Class methods */
    rb_undef_alloc_func(rb_cFloat128);
    rb_undef_method(CLASS_OF(rb_cFloat128), "new");
    
    rb_define_method(rb_cFloat128, "inspect", rb_float128_inspect, 0);
    
    /* Constants */
    rb_define_const(rb_cFloat128, "NAN", rb_float128_cf128(nanq(NULL)));
    rb_define_const(rb_cFloat128, "INFINITY", rb_float128_cf128(strtoflt128("inf", NULL)));
    
    rb_define_const(rb_cFloat128, "MAX", rb_float128_cf128(FLT128_MAX));
    rb_define_const(rb_cFloat128, "MIN", rb_float128_cf128(FLT128_MIN));
    rb_define_const(rb_cFloat128, "EPSILON", rb_float128_cf128(FLT128_EPSILON));
    rb_define_const(rb_cFloat128, "DENORM_MIN", rb_float128_cf128(FLT128_DENORM_MIN));
    rb_define_const(rb_cFloat128, "MANT_DIG", INT2NUM(FLT128_MANT_DIG));
    rb_define_const(rb_cFloat128, "MIN_EXP", INT2NUM(FLT128_MIN_EXP));
    rb_define_const(rb_cFloat128, "MAX_EXP", INT2NUM(FLT128_MAX_EXP));
    rb_define_const(rb_cFloat128, "DIG", INT2NUM(FLT128_DIG));
    rb_define_const(rb_cFloat128, "MIN_10_EXP", INT2NUM(FLT128_MIN_10_EXP));
    rb_define_const(rb_cFloat128, "MAX_10_EXP", INT2NUM(FLT128_MAX_10_EXP));
    
}
