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
complex128_cc128(__complex128 x)
{
	VALUE obj = complex128_allocate(x);
	return obj;
}
/*
 *  call-seq:
 *    -self -> Complex128
 *  
 *  単項マイナス．自身をマイナスにして返す．
 */
static VALUE
complex128_uminus(VALUE self)
{
	struct C128 *c128;
	
	TypedData_Get_Struct(self, struct C128, &complex128_data_type, c128);
	
	return complex128_cc128(-c128->value);
}

/*
 *  call-seq:
 *    inspect -> String
 *    to_s -> String
 *  
 *  自身を(Re+Im)の形で見やすくする．
 */
static VALUE
complex128_inspect(VALUE self)
{
	char* str;
	int exp, sign;
	VALUE retval = rb_str_new_cstr("(");
	struct C128 *c128;
	
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

/*
 *  call-seq:
 *    to_i -> Integer
 *  
 *  +self+を整数にして返す．
 *  虚部がゼロでない(すなわち虚数が現れている)ならRangeErrorが，
 *  実部が非数か無限ならFloatDomainErrorが発生する．
 */
static VALUE
complex128_to_i(VALUE self)
{
	struct C128 *c128;
	
	TypedData_Get_Struct(self, struct C128, &complex128_data_type, c128);
	
	if (cimagq(c128->value) == 0)
	{
		__float128 real = crealq(c128->value);
		if (FIXABLE(real))
			return LONG2FIX((long)real);
		else
		{
			VALUE f128 = rb_float128_cf128(real);
			return rb_funcall(f128, rb_intern("to_i"), 0);
		}
	}
	else
		rb_raise(rb_eRangeError, 
		  "can't convert %"PRIsVALUE" into %s", 
		  rb_inspect(self), rb_class2name(rb_cInteger));
}

/*
 *  call-seq:
 *    to_f -> Float
 *  
 *  +self+をFloat型にして返す．
 *  内部的にはいったん__float128型を取り出してdouble型にキャストしている．
 *  虚部がゼロでない(すなわち虚数が現れている)ならRangeErrorが発生する．
 */
static VALUE
complex128_to_f(VALUE self)
{
	struct C128 *c128;
	
	TypedData_Get_Struct(self, struct C128, &complex128_data_type, c128);
	
	if (cimagq(c128->value) == 0)
		return DBL2NUM((double)crealq(c128->value));
	else
		rb_raise(rb_eRangeError, 
		  "can't convert %"PRIsVALUE" into %s", 
		  rb_inspect(self), rb_class2name(rb_cFloat));
}

/*
 *  call-seq:
 *    to_f128 -> Float128
 *  
 *  +self+をFloat128型にして返す．
 *  虚部がゼロでない(すなわち虚数が現れている)ならRangeErrorが発生する．
 */
static VALUE
complex128_to_f128(VALUE self)
{
	struct C128 *c128;
	
	TypedData_Get_Struct(self, struct C128, &complex128_data_type, c128);
	
	if (cimagq(c128->value) == 0)
		return rb_float128_cf128(crealq(c128->value));
	else
		rb_raise(rb_eRangeError, 
		  "can't convert %"PRIsVALUE" into %s", 
		  rb_inspect(self), rb_class2name(rb_cFloat));
}

/*
 *  call-seq:
 *    to_c -> Complex
 *  
 *  +self+をComplex型にして返す．
 *  実部・虚部は取り出され，Complex型のメンバ変数としてフックされる．
 */
static VALUE
complex128_to_c(VALUE self)
{
	struct C128 *c128;
	
	TypedData_Get_Struct(self, struct C128, &complex128_data_type, c128);
	
	return rb_Complex(
		rb_float128_cf128(crealq(c128->value)),
		rb_float128_cf128(cimagq(c128->value)));
}

/*
 *  call-seq:
 *    to_c128 -> Complex128
 *  
 *  常に+self+を返す．
 */
static VALUE
complex128_to_c128(VALUE self)
{
	return self;
}

/*
 *  call-seq:
 *    real -> Float128
 *  
 *  +self+の実部を返す．型はFloat128である．
 */
static VALUE
complex128_real(VALUE self)
{
	struct C128 *c128;
	
	TypedData_Get_Struct(self, struct C128, &complex128_data_type, c128);
	
	return rb_float128_cf128(crealq(c128->value));
}

/*
 *  call-seq:
 *    imag -> Float128
 *    imaginary -> Float128
 *  
 *  +self+の虚部を返す．型はFloat128である．
 */
static VALUE
complex128_imag(VALUE self)
{
	struct C128 *c128;
	
	TypedData_Get_Struct(self, struct C128, &complex128_data_type, c128);
	
	return rb_float128_cf128(cimagq(c128->value));
}

/*
 *  call-seq:
 *    real? -> false
 *  
 *  常にfalseを返す．
 */
static VALUE
complex128_real_p(VALUE self)
{
	return Qfalse;
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
//	rb_define_global_function("Complex128", f_Complex128, -1);
	
	/* Class methods */
	rb_undef_alloc_func(rb_cComplex128);
	rb_undef_method(CLASS_OF(rb_cComplex128), "new");
	
	/* The unique methods */
	rb_define_method(rb_cComplex128, "real?", complex128_real_p, 0);
	rb_define_method(rb_cComplex128, "real", complex128_real, 0);
	rb_define_method(rb_cComplex128, "imag", complex128_imag, 0);
	rb_define_alias(rb_cComplex128, "imaginary", "imag");
	
	/* Operators */
	rb_define_method(rb_cComplex128, "-@", complex128_uminus, 0);
	
	/* Type convertion methods */
	rb_define_method(rb_cComplex128, "inspect", complex128_inspect, 0);
	rb_define_method(rb_cComplex128, "to_s", complex128_inspect, 0);
	rb_define_method(rb_cComplex128, "to_str", complex128_inspect, 0);
	
	rb_define_method(rb_cComplex128, "to_f", complex128_to_f, 0);
	rb_define_method(rb_cComplex128, "to_f128", complex128_to_f128, 0);
	
	rb_define_method(rb_cComplex128, "to_i", complex128_to_i, 0);
	rb_define_method(rb_cComplex128, "to_int", complex128_to_i, 0);
	
	rb_define_method(rb_cComplex128, "to_c", complex128_to_c, 0);
	rb_define_method(rb_cComplex128, "to_c128", complex128_to_c128, 0);
	
	/* Constants */
	rb_define_const(rb_cComplex128, "I", complex128_cc128(0+1i));
}
