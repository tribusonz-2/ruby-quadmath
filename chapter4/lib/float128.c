/*******************************************************************************
    float128.c -- Float128 Class
    
    Author: Hironobu Inatsuka
*******************************************************************************/
#include <ruby.h>
#include <quadmath.h>
#include "rb_quadmath.h"
#include "internal/rb_quadmath.h"

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
	VALUE obj;
	ptr->value = x;
	obj = TypedData_Wrap_Struct(rb_cFloat128, &float128_data_type, ptr);
	RB_OBJ_FREEZE(obj);
	return obj;
}

__float128
GetF128(VALUE self)
{
	struct F128 *f128;
	
	TypedData_Get_Struct(self, struct F128, &float128_data_type, f128);
	
	return f128->value;
}


/*
 *  call-seq:
 *    hash -> Integer
 *  
 *  +self+のHash値を返す．
 *  
 */
static VALUE
float128_hash(VALUE self)
{
	struct F128 *f128;
	st_index_t hash;
	
	TypedData_Get_Struct(self, struct F128, &float128_data_type, f128);
	
	hash = rb_memhash(f128, sizeof(struct F128));
	
	return ST2FIX(hash);
}

/*
 *  call-seq:
 *    eql?(other) -> bool
 *  
 *  +self+と+other+が等しければ真を返す．
 */
static VALUE
float128_eql_p(VALUE self, VALUE other)
{
	__float128 left_f128, right_f128;
	
	if (CLASS_OF(other) != rb_cFloat128)
		return Qfalse;
	
	left_f128 = GetF128(self);
	right_f128 = GetF128(other);
	
	return left_f128 == right_f128 ? Qtrue : Qfalse;
}

#if 0
/*
 *  call-seq:
 *    -self -> Float128
 *  
 *  単項マイナス．+self+をマイナスにして返す．
 */
static VALUE
float128_uminus(VALUE self)
{
	struct F128 *f128 = F128_Get_Struct(self);
	
	return rb_float128_cf128(-f128->value);
}
#endif

/*
 *  call-seq:
 *    finite? -> bool
 *  
 *  +self+が有限ならtrueを，そうでないならfalseを返す．
 *  
 *    Float128::INFINITY.finite? # => false
 */
static VALUE
float128_finite_p(VALUE self)
{
	__float128 f128 = GetF128(self);
	
	return (finite(f128)) ? Qtrue : Qfalse;
}



/*
 *  call-seq:
 *    infinite? -> 1 | -1 | nil
 *  
 *  +self+が無限大の場合，負であれば-1を，正であれば1を，そうでないならnilを返す．
 *  
 *    Float128::INFINITY.infinite? # => 1
 *    (-Float128::INFINITY).infinite? # => -1
 */
static VALUE
float128_infinite_p(VALUE self)
{
	__float128 f128 = GetF128(self);
	int sign = isinfq(f128);
	
	if (sign)
		return INT2NUM(sign);
	else
		return Qnil;
}


/*
 *  call-seq:
 *    nan? -> bool
 *  
 *  +self+がNaN(Not a Number)ならtrueを，そうでないならfalseを返す．
 *  
 *    Float128::NAN.nan? # => true
 */
static VALUE
float128_nan_p(VALUE self)
{
	__float128 f128 = GetF128(self);
	
	return isnanq(f128) ? Qtrue : Qfalse;
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
	__float128 f128 = GetF128(self);
	
	switch (ool_quad2str(f128, 'g', &exp, &sign, &str)) {
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
	__float128 f128;
	int exp, sign;
	
	rb_scan_args(argc, argv, "01", &base);
	
	if (NIL_P(base))
		return float128_inspect(self);
	
to_s_retry:
	switch(TYPE(base)) {
	case T_FIXNUM:
		long base_number = FIX2LONG(base);
		f128 = GetF128(self);
		switch (base_number) {
		case 2:
			if (isinfq(f128) || isnanq(f128))
				goto to_s_none_finite;
			if ('0' == ool_quad2str(f128, 'b', &exp, &sign, &str))
				goto to_s_invalid_format;
			if (sign == -1)
				return rb_sprintf("-%se%+d", str, exp);
			else
				return rb_sprintf("%se%+d", str, exp);
			break;
		case 10:
			if (isinfq(f128) || isnanq(f128))
				goto to_s_none_finite;
			if ('0' == ool_quad2str(f128, 'e', &exp, &sign, &str))
				goto to_s_invalid_format;
			if (sign == -1)
				return rb_sprintf("-%se%+d", str, exp);
			else
				return rb_sprintf("%se%+d", str, exp);
			break;
		case 16:
			if (isinfq(f128) || isnanq(f128))
				goto to_s_none_finite;
			if ('0' == ool_quad2str(f128, 'a', &exp, &sign, &str))
				goto to_s_invalid_format;
			if (sign == -1)
				return rb_sprintf("-%sp%+d", str, exp);
			else
				return rb_sprintf("%sp%+d", str, exp);
			break;
		default:
			break;
		}
		// break; /* going through to switch to BIGNUM */
	case T_BIGNUM:
		rb_raise(rb_eRangeError, 
			"unavailable radix: %"PRIsVALUE" (operational: 2,10,16)", 
			rb_String(base));
		break;
	default:
		base = rb_Integer(base);
		goto to_s_retry;
		break;
	}
to_s_none_finite:
	if ('0' == ool_quad2str(f128, 'f', &exp, &sign, &str))
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
	__float128 f128 = GetF128(self);
	
	if (isinfq(f128) || isnanq(f128))
	{
		rb_raise(rb_eFloatDomainError, "%"PRIsVALUE"", rb_String(self));
	}
	else if (FIXABLE(f128))
	{
		return LONG2FIX((long)f128);
	}
	else
	{
		char* str;
		int exp, sign;
		
		ool_quad2str(f128, 'f', &exp, &sign, &str);
		
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
	__float128 f128 = GetF128(self);
	
	return DBL2NUM((double)f128);
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
	__float128 f128 = GetF128(self);
	
	return rb_complex128_cc128((__complex128)f128);
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

/*
 *  call-seq:
 *    modf -> [Float128, Float128]
 *  
 *  +self+を整数部と小数部に分解し，ペアの配列で返す．
 *  これはC言語のmodf()を直接使えるようにしたものである．
 *  
 *    '12.345'.to_f128.modf # => [12.0, 0.345]

 */
static VALUE
float128_modf(VALUE self)
{
	__float128 f128 = GetF128(self), intpart, frapart;
	frapart = modfq(f128, &intpart);
	return rb_assoc_new(
		rb_float128_cf128(intpart), 
		rb_float128_cf128(frapart));
}

/*
 *  call-seq:
 *    frexp -> [Float128, Integer]
 *  
 *  +self+を2を基底とした指数と仮数に分解し，[仮数, 指数]のペア配列で返す．
 *  有限でない場合，どのような値が返るかは機種依存である．
 *  
 *    8.to_f128.frexp # => [0.5, 4]
 */
static VALUE
float128_frexp(VALUE self)
{
	__float128 f128 = GetF128(self), frapart;
	int exppart;
	frapart = frexpq(f128, &exppart);
	return rb_assoc_new(
		rb_float128_cf128(frapart),
		INT2FIX(exppart));
}

/*
 *  call-seq:
 *    scalb(n) -> Float128
 *    scalbn(n) -> Float128
 *  
 *  +self+にFloat::RADIXのn乗をかけた値にする．
 *  特異メソッドにも同等のメソッドがあるが，こちらはレシーバをオペランドの代わりにしたものである．
 *  
 *    Float::RADIX # => 2
 *    x = 3.to_f128
 *    x.scalb(4) # => 48.0
 */
static VALUE
float128_scalb(VALUE self, VALUE n)
{
	__float128 f128 = GetF128(self);
	long v_exp = NUM2LONG(n);
	
	return rb_float128_cf128(scalbln(f128, v_exp));
}

void
InitVM_Float128(void)
{
	/* Class methods */
	rb_undef_alloc_func(rb_cFloat128);
	rb_undef_method(CLASS_OF(rb_cFloat128), "new");
	
	/* Object methods */
	rb_define_method(rb_cFloat128, "hash", float128_hash, 0);
	rb_define_method(rb_cFloat128, "eql?", float128_eql_p, 1);
	
	/* The unique Methods */
	rb_define_method(rb_cFloat128, "infinite?", float128_infinite_p, 0);
	rb_define_method(rb_cFloat128, "finite?", float128_finite_p, 0);
	rb_define_method(rb_cFloat128, "nan?", float128_nan_p, 0);
	
	/* Operators & Evals */
#if 0
	rb_define_method(rb_cFloat128, "-@", float128_uminus, 0);
#endif
	
	/* Type convertion methods */
	rb_define_method(rb_cFloat128, "inspect", float128_inspect, 0);
	rb_define_method(rb_cFloat128, "to_s", float128_to_s, -1);

	rb_define_method(rb_cFloat128, "to_f", float128_to_f, 0);
	rb_define_method(rb_cFloat128, "to_f128", float128_to_f128, 0);

	rb_define_method(rb_cFloat128, "to_i", float128_to_i, 0);
	
	rb_define_method(rb_cFloat128, "to_c128", float128_to_c128, 0);
	
	/* Utilities */
	rb_define_method(rb_cFloat128, "modf", float128_modf, 0);
	rb_define_method(rb_cFloat128, "frexp", float128_frexp, 0);
	rb_define_method(rb_cFloat128, "scalb", float128_scalb, 1);
	rb_define_alias(rb_cFloat128, "scalbn", "scalb");
	
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

VALUE
rb_float128_cf128(const __float128 x)
{
	VALUE obj = float128_allocate(x);
	return obj;
}

__float128
rb_float128_value(VALUE x)
{
	struct F128 *f128 = rb_check_typeddata(x, &float128_data_type);
	
	return f128->value;
}
