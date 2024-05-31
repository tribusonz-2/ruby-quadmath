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

/*
 *  call-seq:
 *    -self -> Float128
 *  
 *  単項マイナス．+self+をマイナスにして返す．
 */
static VALUE
rb_float128_uminus(VALUE self)
{
	struct F128 *f128;
	
	TypedData_Get_Struct(self, struct F128, &float128_data_type, f128);
	
	return rb_float128_cf128(-f128->value);
}

/*
 *  call-seq:
 *    inspect -> String
 *  
 *  +self+を見やすくする．#to_sと働きは同じだが，引数を持たずジェネリック表記するのが異なる．
 *  
 *    Float128('10.0') #=> "10.0"
 *    Float128('1e34') #=> "1.0e+34"
 *    Float128('0.1') #=> "0.1"
 *    Float128('0.0000001') #=> "1.0e-7"
 */
static VALUE
float128_inspect(VALUE self)
{
	char* str;
	int exp, sign;
	struct F128 *f128;
	
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

/*
 *  call-seq:
 *    to_s(base = nil) -> String
 *  
 *  +self+を文字列へ変換する．基数+base+ (2, 10, 16) を設定すると進数が変わる．
 *  
 *    Float128('10.0').to_s #=> "10.0"
 *    Float128('1e3').to_s(10) #=> "1.0e+3"
 *    Float128('0.1').to_s(2) #=> "0.1e+0"
 *    Float128('0.001').to_s(10) #=> "1.0e-3"
 *    Float128::MAX.to_s(16) #=> "0x1.ffffffffffffffffffffffffffffp+16383"
 *    Float128::INFINITY.to_s(2) #=> "Infinity" # 非数・無限は基数に関係なく文字列変換する
 *  
 *  +base+が範囲外であるとRangeErrorが発生する．
 */
static VALUE
float128_to_s(int argc, VALUE *argv, VALUE self)
{
	char *str;
	VALUE base;
	struct F128 *f128;
	int exp, sign;
	
	rb_scan_args(argc, argv, "01", &base);
	
	if (NIL_P(base))
		return float128_inspect(self);
	
to_s_retry:
	switch(TYPE(base)) {
	case T_FIXNUM:
		long base_number = FIX2INT(base);
		
		TypedData_Get_Struct(self, struct F128, &float128_data_type, f128);
		
		switch (base_number) {
		case 2:
			if (isinfq(f128->value) || isnanq(f128->value))
				goto to_s_none_finite;
			if ('0' == ool_quad2str(f128->value, 'b', &exp, &sign, &str))
				goto to_s_invalid_format;
			if (sign == -1)
				return rb_sprintf("-%se%+d", str, exp);
			else
				return rb_sprintf("%se%+d", str, exp);
			break;
		case 10:
			if (isinfq(f128->value) || isnanq(f128->value))
				goto to_s_none_finite;
			if ('0' == ool_quad2str(f128->value, 'e', &exp, &sign, &str))
				goto to_s_invalid_format;
			if (sign == -1)
				return rb_sprintf("-%se%+d", str, exp);
			else
				return rb_sprintf("%se%+d", str, exp);
			break;
		case 16:
			if (isinfq(f128->value) || isnanq(f128->value))
				goto to_s_none_finite;
			if ('0' == ool_quad2str(f128->value, 'a', &exp, &sign, &str))
				goto to_s_invalid_format;
			if (sign == -1)
				return rb_sprintf("-%sp%+d", str, exp);
			else
				return rb_sprintf("%sp%+d", str, exp);
			break;
		default:
			break;
		}
		// break; /* go through to switch to BigNum */
	case T_BIGNUM:
		rb_raise(rb_eRangeError, "unavailable base: %"PRIsVALUE" (operational: 2,10,16)", rb_inspect(base));
		break;
	default:
		base = rb_Integer(base);
		goto to_s_retry;
		break;
	}
to_s_none_finite:
	if ('0' == ool_quad2str(f128->value, 'f', &exp, &sign, &str))
		goto to_s_invalid_format;
	if (sign == -1)
		return rb_sprintf("-%s", str);
	else
		return rb_sprintf("%s", str);
to_s_invalid_format:
	rb_raise(rb_eRuntimeError, "invalid format in ool_quad2str()");
}

/*
 *  call-seq:
 *    to_i -> Integer
 *  
 *  +self+を整数にして返す．
 *  非数や無限を変換しようとするとFloatDomainErrorが発生する．
 */
static VALUE
float128_to_i(VALUE self)
{
	struct F128 *f128;
	
	TypedData_Get_Struct(self, struct F128, &float128_data_type, f128);
	
	if (isinfq(f128->value) || isnanq(f128->value))
	{
		rb_raise(rb_eFloatDomainError, "%"PRIsVALUE"", rb_inspect(self));
	}
	else if (FIXABLE(f128->value))
	{
		return LONG2FIX((long)f128->value);
	}
	else
	{
		char* str;
		int exp, sign;
		
		ool_quad2str(f128->value, 'f', &exp, &sign, &str);
		
		for (volatile int i = 0; i < (int)strlen(str); i++)
			if (str[i] == '.')  { str[i] = 0; break; }
		
		if (sign == -1)
			return rb_str_to_inum(rb_sprintf("-%s", str), 10, 1);
		else /* if sign == 1 */
			return rb_str_to_inum(rb_sprintf("%s", str), 10, 1);
	}
}

/*
 *  call-seq:
 *    to_f -> Float
 *  
 *  +self+をFloat型にして返す．
 *  内部的には__float128型をdouble型へキャストしている．
 */
static VALUE
float128_to_f(VALUE self)
{
	struct F128 *f128;
	
	TypedData_Get_Struct(self, struct F128, &float128_data_type, f128);
	
	return DBL2NUM((double)f128->value);
}

/*
 *  call-seq:
 *    to_f128 -> Float128
 *  
 *  常に+self+を返す．
 */
static VALUE
float128_to_f128(VALUE self)
{
	return self;
}

/*
 *  call-seq:
 *    to_c128 -> Complex128
 *  
 *  +self+をComplex128型にして返す．
 *  内部的には__float128型を__complex128型へキャストしている．
 */
static VALUE
float128_to_c128(VALUE self)
{
	struct F128 *f128;
	
	TypedData_Get_Struct(self, struct F128, &float128_data_type, f128);
	
	return rb_complex128_cc128((__complex128)f128->value);
}


#if 0
static VALUE
numeric_to_f128(VALUE self)
{
	if (rb_respond_to(self, rb_intern("to_f128")))
		rb_raise(rb_eNoMethodError, 
		"can't convert %s into %"PRIsVALUE"", 
		rb_class2name(rb_cFloat128), CLASS_OF(self));
	else
		return rb_funcall(self, rb_intern("to_f128"), 0);
}

static VALUE
float_to_f128(VALUE self)
{
	double x = NUM2DBL(self);
	return rb_float128_cf128((__float128)x);
}
#endif


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
//	rb_define_method(rb_cNumeric, "to_f128", numeric_to_f128, 0);
	
//	rb_define_method(rb_cFloat, "to_f128", float_to_f128, 0);
	
	/* Global function */
//	rb_define_global_function("Float128", f_Float128, -1);
	
	/* Class methods */
	rb_undef_alloc_func(rb_cFloat128);
	rb_undef_method(CLASS_OF(rb_cFloat128), "new");
	
	/* Operators */
	rb_define_method(rb_cFloat128, "-@", rb_float128_uminus, 0);
	
	/* Type convertion methods */
	rb_define_method(rb_cFloat128, "inspect", float128_inspect, 0);
	rb_define_method(rb_cFloat128, "to_s", float128_to_s, -1);
	rb_define_method(rb_cFloat128, "to_str", float128_to_s, -1);

	rb_define_method(rb_cFloat128, "to_f", float128_to_f, 0);
	rb_define_method(rb_cFloat128, "to_f128", float128_to_f128, 0);

	rb_define_method(rb_cFloat128, "to_i", float128_to_i, 0);
	
	rb_define_method(rb_cFloat128, "to_c128", float128_to_c128, 0);
	
	/* Constants */
	rb_define_const(rb_cFloat128, "NAN", rb_float128_cf128(nanq(NULL)));
	rb_define_const(rb_cFloat128, "INFINITY", rb_float128_cf128(HUGE_VALQ));
	
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
