/*******************************************************************************
    numerable.c -- Numerable for quadmath arithmetic
    
    Author: Hironobu Inatsuka
*******************************************************************************/
#include <ruby.h>
#include <quadmath.h>
#include "rb_quadmath.h"
#include "internal/rb_quadmath.h"


#define OPE_ADD  1
#define OPE_SUB  2
#define OPE_MUL  3
#define OPE_DIV  4
#define OPE_MOD  5
#define OPE_POW  6
#define OPE_CMP  7
#define OPE_COERCE  8

#define FMT_EMPTY  0
#define FMT_DRCTV  1
#define FMT_WIDTH  2
#define FMT_WIDTH_SCALAR  3
#define FMT_POINT  4
#define FMT_PREC   5
#define FMT_PREC_SCALAR   6
#define FMT_SET_FT 7

#define FLAG_SHARP  0x1
#define FLAG_ZERO   0x2
#define FLAG_SPACE  0x4
#define FLAG_PLUS   0x8
#define FLAG_MINUS  0x10

#define FT_FLT  0
#define FT_DBL  1
#define FT_LDBL 2
#define FT_QUAD 3

#define BUF_SIZ  128

static inline VALUE
numeric_to_f128_inline(VALUE self, int exception)
{
	
	if (!rb_respond_to(self, rb_intern("to_f128")))
	{
		if (!exception)
			return Qnil;
		else
			rb_raise(rb_eNoMethodError, 
			"can't convert %"PRIsVALUE" into %s", 
			rb_String(self), rb_class2name(rb_cFloat128));
	}
	else
		return rb_funcall(self, rb_intern("to_f128"), 0);
}

/*
 *  call-seq:
 *    to_f128 -> Float128
 *  
 *  システムがFloat128型への変換を要求する，サブクラスのためのフックである．
 *  内部的にはメソッドコールする．実際には未定義である．
 */
static VALUE
numeric_to_f128(VALUE self)
{
	return numeric_to_f128_inline(self, true);
}

static inline VALUE
numeric_to_c128_inline(VALUE self, int exception)
{
	if (!rb_respond_to(self, rb_intern("to_c128")))
	{
		if (!exception)
			return Qnil;
		else
			rb_raise(rb_eNoMethodError, 
			"can't convert %"PRIsVALUE" into %s", 
			rb_String(self), rb_class2name(rb_cComplex128));
	}
	else
		return rb_funcall(self, rb_intern("to_c128"), 0);
}

/*
 *  call-seq:
 *    to_c128 -> Complex128
 *  
 *  システムがComplex128型への変換を要求する，サブクラスのためのフックである．
 *  内部的にはメソッドコールする．実際には未定義である．
 */
static VALUE
numeric_to_c128(VALUE self)
{
	return numeric_to_c128_inline(self, true);
}

/*
 *  call-seq:
 *    to_f128 -> Float128
 *  
 *  StringからFloat128型を生成する．実装はライブラリ関数である．
 */
static VALUE
string_to_f128(VALUE self)
{
	__float128 x = strtoflt128(StringValuePtr(self), NULL);
	return rb_float128_cf128(x);
}


static inline __float128
integer_to_cf128(VALUE self)
{
	__float128 x = 0.0Q;
	switch (TYPE(self)) {
	case T_FIXNUM:
		x = (__float128)FIX2LONG(self);
		break;
	case T_BIGNUM:
		VALUE inum = rb_big2str(self, 10);
		x = strtoflt128(StringValuePtr(inum), NULL);
	default:
		break;
	}
	return x;
}

/*
 *  call-seq:
 *    to_f128 -> Float128
 *  
 *  IntegerからFloat128型へ変換する．
 *  内部的には，なるべく精度落ちしないよう工夫して変換する．
 *  
 */
static VALUE
integer_to_f128(VALUE self)
{
	__float128 x = integer_to_cf128(self);
	
	return rb_float128_cf128(x);
}

/*
 *  call-seq:
 *    to_f128 -> Float128
 *  
 *  IntegerからComplex128型へ変換する．
 *  to_f128と同じ実装であるが，いったんFloat128型へ変換したあとCのレベルで型キャストしている．
 *  
 */
static VALUE
integer_to_c128(VALUE self)
{
	__complex128 z = (__complex128)integer_to_cf128(self);
	return rb_complex128_cc128(z);
}



static inline __float128
rational_to_cf128(VALUE self)
{
	VALUE numer = rb_rational_num(self),
	      denom = rb_rational_den(self);
	
	return integer_to_cf128(numer) / integer_to_cf128(denom);
}

/*
 *  call-seq:
 *    to_f128 -> Float128
 *  
 *  RationalからFloat128型へ変換する．
 *  変換は分母・分子を各々取り出してから除算するため，正確な型変換が期待できる．
 *  二倍精度から生成したRationalの場合，値によっては情報落ちしていることがある．
 *    
 *    (-1/3r).to_f128 # => -0.333333333333333333333333333333333
 *    Rational(2.1).to_f128 # => 2.1000000000000000888178419700125
 */
static VALUE
rational_to_f128(VALUE self)
{
	__float128 x = rational_to_cf128(self);
	
	return rb_float128_cf128(x);
}

/*
 *  call-seq:
 *    to_c128 -> Complex128
 *  
 *  RationalからComplex128型へ変換する．
 *  to_f128と同じ実装であるが，いったんFloat128に変換したあとCのレベルで型キャストしている．
 *    
 *    Rational(-1, 2).to_c128 # => (-0.5+0.0i)
 */
static VALUE
rational_to_c128(VALUE self)
{
	__complex128 z = (__complex128)rational_to_cf128(self);
	return rb_complex128_cc128(z);
}


/*
 *  call-seq:
 *    to_f128 -> Float128
 *  
 *  FloatからFloat128型へ変換する．C言語のレベルで型キャストしている．
 *  二倍精度から四倍精度への変換であるため，情報落ちする場合がある．
 *  
 *    Float::INFINITY.to_f128 == Float::INFINITY # => true
 *    -Float('1').to_f128 == Float128('-1.0') # => true
 *    -Float('1.1').to_f128 == Float128('-1.1') # => false
 *    
 */
static VALUE
float_to_f128(VALUE self)
{
	double x = NUM2DBL(self);
	
	return rb_float128_cf128((__float128)x);
}

static VALUE
float_to_c128(VALUE self)
{
	double x = NUM2DBL(self);
	return rb_float128_cf128((__complex128)x);
}



static inline __float128
elem_to_cf128(VALUE self)
{
	__float128 x;
	switch (TYPE(self)) {
	case T_FIXNUM:
	case T_BIGNUM:
		x = integer_to_cf128(self);
		break;
	case T_RATIONAL:
		x = rational_to_cf128(self);
		break;
	case T_FLOAT:
		x = (__float128)NUM2DBL(self);
		break;
	default:
		if (CLASS_OF(self) != rb_cFloat128)
			self = rb_funcall(self, rb_intern("to_f128"), 0);
		
		x = rb_float128_value(self);
		
		break;
	}
	return x;
}


static inline VALUE
nucomp_to_f128_inline(VALUE self, int exception)
{
	VALUE real = rb_complex_real(self), 
	      imag = rb_complex_imag(self);
	
	if (elem_to_cf128(imag) != 0)
	{
		if (!exception)
			return Qnil;
		else
			rb_raise(rb_eRangeError, 
			  "can't convert %"PRIsVALUE" into %s", 
			  rb_String(self), rb_class2name(rb_cFloat128));
	}
	return rb_float128_cf128(elem_to_cf128(real));
}

static VALUE
nucomp_to_f128(VALUE self)
{
	return nucomp_to_f128_inline(self, true);
}

static VALUE
nucomp_to_c128(VALUE self)
{
	VALUE real = rb_complex_real(self), 
	      imag = rb_complex_imag(self);
	__complex128 z;
	__real__ z = elem_to_cf128(real);
	__imag__ z = elem_to_cf128(imag);
	
	return rb_complex128_cc128(z);
}

static inline VALUE
complex128_to_f128_inline(VALUE self, int exception)
{
	__complex128 z = rb_complex128_value(self);
	
	if (cimagq(z) != 0)
	{
		if (!exception)
			return Qnil;
		else
			rb_raise(rb_eRangeError, 
			  "can't convert %"PRIsVALUE" into %s", 
			  rb_String(self), rb_class2name(rb_cFloat128));
	}
	return rb_float128_cf128(crealq(z));
}

/*
 *  call-seq:
 *    Float128(val) -> Float128
 *    Float128(val, exception: false) -> Float128 | nil
 *  
 *  +val+よりFloat128を生成する．+val+は+self+ほか数値クラスやStringなどより生成できる．
 *  変換できない場合，RangeErrorが発生する．
 *  リテラルはなるべく四倍精度への変換を試みる．ただし，二倍精度からの変換では，精度保持される可能性が低いこともあるので，注意が必要である．
 *  キーワード引数exceptionをfalseに指定すると，例外のときnilが返されるようになる．
 *  
 *    Float128(1)       # => 1.0
 *    Float128(2.0)     # => 2.0
 *    Float128(2.1)     # => 2.1000000000000000888178419700125
 *    Float128('2.1')   # => 2.1
 *    Float128(1.0/3.0) # => 0.333333333333333314829616256247391
 *    Float128(1/3r)    # => 0.333333333333333333333333333333333
 *    Float128(1+0i)    # => 1.0
 *    Float128(1i)      # => RangeError
 *    Float128(1i, expeption: false) # => nil
 */
static VALUE
f_Float128(int argc, VALUE *argv, VALUE self)
{
	VALUE val, opts = Qnil;
	argc = rb_scan_args(argc, argv, "11", &val, &opts);
	int exception = opts_exception_p(opts);
	
	switch (TYPE(val)) {
	case T_FIXNUM:
	case T_BIGNUM:
		val = integer_to_f128(val);
		break;
	case T_RATIONAL:
		val = rational_to_f128(val);
		break;
	case T_FLOAT:
		val = float_to_f128(val);
		break;
	case T_COMPLEX:
		val = nucomp_to_f128_inline(val, exception);
		break;
	case T_STRING:
		val = string_to_f128(val);
		break;
	default:
		if (CLASS_OF(val) == rb_cFloat128);
		else if (CLASS_OF(val) == rb_cComplex128)
			val = complex128_to_f128_inline(val, exception);
		else
			val = numeric_to_f128_inline(val, exception);
		break;
	}
	return val;
}

/*
 *  call-seq:
 *    polar -> [Float128, Float128]
 *  
 *  +self+の絶対値と偏角を配列にして返す．振る舞いはFloatと同じだが精度が異なる．
 *  
 *  1.0.polar             # => [1.0, 0]
 *  Float128('2.0').polar # => [1.0, 0]
 *  -1.0.polar # => [1.0, 3.141592653589793]
 *  Float128('-1.0').polar # => [1.0, 3.1415926535897932384626433832795]
 *  
 */
static VALUE
float128_polar(VALUE self)
{
	__float128 f128 = GetF128(self);
	
	if (signbitq(f128))
		return rb_assoc_new(
			rb_float128_cf128(fabsq(f128)),
			rb_float128_cf128(M_PIq));
	else
		return rb_assoc_new(
			rb_float128_cf128(fabsq(f128)),
			INT2NUM(0));
}


/*
 *  call-seq:
 *    abs -> Float128
 *    magnitude -> Float128
 *  
 *  +self+の絶対値を返す．
 */
static VALUE
float128_abs(VALUE self)
{
	__float128 f128 = GetF128(self);
	
	return rb_float128_cf128(fabsq(f128));
}

/*
 *  call-seq:
 *    abs2 -> Float128
 *  
 *  +self+の絶対値の二乗を返す．
 */
static VALUE
float128_abs2(VALUE self)
{
	__float128 f128 = GetF128(self);
	
	return rb_float128_cf128(f128 * f128);
}


/*
 *  call-seq:
 *    arg -> 0 | QuadMath::PI
 *    angle -> 0 | QuadMath::PI
 *    phase -> 0 | QuadMath::PI
 *  
 *  +self+の偏角(正なら0，負ならPI)を返す．
 *  Float型と振る舞いは同じだが，成分が異なる．
 *  
 *    Float128('1').arg # => 0
 *    Float128('-1').arg # => 3.1415926535897932384626433832795
 *    Float('-1').arg # => 3.141592653589793
 */
static VALUE
float128_arg(VALUE self)
{
	__float128 f128 = GetF128(self);
	
	return signbitq(f128) ?
		rb_float128_cf128(M_PIq) :
		INT2NUM(0);
}


static inline __float128
f128_modulo(__float128 x, __float128 y)
{
	switch ((signbitq(x) << 1) | signbitq(y)) {
	case 0b00: case 0b11:
		return fmodq(x, y);
	default:
		return x - y * floorq(x / y);
	}
}

static inline __float128
get_real(VALUE num)
{
	__float128 real;
	VALUE nucomp;
	
	switch (convertion_num_types(num)) {
	case NUM_FIXNUM:
	case NUM_BIGNUM:
		real = integer_to_cf128(num);
		break;
	case NUM_RATIONAL:
		real = rational_to_cf128(num);
		break;
	case NUM_FLOAT:
		real = (__float128)NUM2DBL(num);
		break;
	case NUM_COMPLEX:
		nucomp = nucomp_to_f128_inline(num, false);
		if (NIL_P(nucomp))  goto not_a_real;
		real = rb_float128_value(nucomp);
		break;
	case NUM_FLOAT128:
		real = rb_float128_value(num);
		break;
	case NUM_COMPLEX128:
		nucomp = complex128_to_f128_inline(num, false);
		if (NIL_P(nucomp))  goto not_a_real;
		real = rb_float128_value(nucomp);
		break;
	case NUM_OTHERTYPE:
	default:
		if (RTEST(rb_obj_is_kind_of(num, rb_cNumeric)))
		{
			VALUE val = numeric_to_f128_inline(num, false);
			if (NIL_P(val))  goto not_a_real;
			real = rb_float128_value(val);
		}
		else
			goto not_a_real;
		break;
	}
	return real;
	
not_a_real:
	rb_raise(rb_eTypeError, "not a real");

}

static inline void
unknown_opecode(void)
{
	rb_warn("unknown operator code (in %s)", __func__);
}

static VALUE
float128_ope_integer(VALUE self, VALUE other, int ope)
{
	__float128 x = GetF128(self);
	__float128 y = integer_to_cf128(other);
	
	switch (ope) {
	case OPE_ADD:
		return rb_float128_cf128(x + y);
		break;
	case OPE_SUB:
		return rb_float128_cf128(x - y);
		break;
	case OPE_MUL:
		return rb_float128_cf128(x * y);
		break;
	case OPE_DIV:
		return rb_float128_cf128(x / y);
		break;
	case OPE_MOD:
		return rb_float128_cf128(f128_modulo(x, y));
		break;
	case OPE_POW:
		return rb_float128_cf128(powq(x, y));
		break;
	case OPE_CMP:
		if (isnanq(x))
			return Qnil;
		else if (x <  y)
			return INT2NUM(-1);
		else if (x >  y)
			return INT2NUM(1);
		else /* (x == y) */
			return INT2NUM(0);
		break;
	case OPE_COERCE:
		return rb_assoc_new(
			rb_float128_cf128(y),
			rb_float128_cf128(x));
		break;
	default:
		unknown_opecode();
		return rb_float128_cf128(0.q);
		break;
	}
}

static VALUE
float128_ope_rational(VALUE self, VALUE other, int ope)
{
	__float128 x = GetF128(self);
	__float128 y = rational_to_cf128(other);
	
	switch (ope) {
	case OPE_ADD:
		return rb_float128_cf128(x + y);
		break;
	case OPE_SUB:
		return rb_float128_cf128(x - y);
		break;
	case OPE_MUL:
		return rb_float128_cf128(x * y);
		break;
	case OPE_DIV:
		return rb_float128_cf128(x / y);
		break;
	case OPE_MOD:
		return rb_float128_cf128(f128_modulo(x, y));
		break;
	case OPE_POW:
		return rb_float128_cf128(powq(x, y));
		break;
	case OPE_CMP:
		if (isnanq(x))
			return Qnil;
		else if (x < y)
			return INT2NUM(-1);
		else if (x > y)
			return INT2NUM(1);
		else /* (x == y) */
			return INT2NUM(0);
		break;
	case OPE_COERCE:
		return rb_assoc_new(
			rb_float128_cf128(y),
			rb_float128_cf128(x));
		break;
	default:
		unknown_opecode();
		return rb_float128_cf128(0.q);
		break;
	}
}

static VALUE
float128_ope_float(VALUE self, VALUE other, int ope)
{
	__float128 x = GetF128(self);
	__float128 y = (__float128)NUM2DBL(other);
	
	switch (ope) {
	case OPE_ADD:
		return rb_float128_cf128(x + y);
		break;
	case OPE_SUB:
		return rb_float128_cf128(x - y);
		break;
	case OPE_MUL:
		return rb_float128_cf128(x * y);
		break;
	case OPE_DIV:
		return rb_float128_cf128(x / y);
		break;
	case OPE_MOD:
		return rb_float128_cf128(f128_modulo(x, y));
		break;
	case OPE_POW:
		return rb_float128_cf128(powq(x, y));
		break;
	case OPE_CMP:
		if (isnanq(x) || isnanq(y))
			return Qnil;
		else if (x < y)
			return INT2NUM(-1);
		else if (x > y)
			return INT2NUM( 1);
		else /* (x == y) */
			return INT2NUM( 0);
		break;
	case OPE_COERCE:
		return rb_assoc_new(
			rb_float128_cf128(y),
			rb_float128_cf128(x));
		break;
	default:
		unknown_opecode();
		return rb_float128_cf128(0.q);
		break;
	}
}

static VALUE
float128_ope_float128(VALUE self, VALUE other, int ope)
{
	__float128 x = GetF128(self);
	__float128 y = GetF128(other);
	
	switch (ope) {
	case OPE_ADD:
		return rb_float128_cf128(x + y);
		break;
	case OPE_SUB:
		return rb_float128_cf128(x - y);
		break;
	case OPE_MUL:
		return rb_float128_cf128(x * y);
		break;
	case OPE_DIV:
		return rb_float128_cf128(x / y);
		break;
	case OPE_MOD:
		return rb_float128_cf128(f128_modulo(x, y));
		break;
	case OPE_POW:
		return rb_float128_cf128(powq(x, y));
		break;
	case OPE_CMP:
		if (isnanq(x) || isnanq(y))
			return Qnil;
		else if (x < y)
			return INT2NUM(-1);
		else if (x > y)
			return INT2NUM( 1);
		else /* (x == y) */
			return INT2NUM( 0);
		break;
	case OPE_COERCE:
		return rb_assoc_new(
			rb_float128_cf128(y),
			rb_float128_cf128(x));
		break;
	default:
		unknown_opecode();
		return rb_float128_cf128(0.q);
		break;
	}
}

VALUE
float128_nucomp_pow(VALUE x, VALUE y)
{
		if (RTEST(rb_num_coerce_cmp(INT2FIX(0), rb_complex_imag(y), rb_intern("=="))))
		{
			__float128 x_real = GetF128(x);
			__float128 y_real = get_real(rb_complex_real(y));
			__float128 z_real = powq(x_real, y_real);
			return rb_Complex(rb_float128_cf128(z_real), rb_complex_imag(y));
		}
		else
		{
			__float128 x_real = GetF128(x);
#if 0
			__float128 y_real = get_real(rb_complex_real(y));
			__float128 y_imag = get_real(rb_complex_imag(y));
			__float128 z_real = powq(x_real, y_real) * cosq(y_imag * logq(x_real));
			__float128 z_imag = powq(x_real, y_real) * sinq(y_imag * logq(x_real));
			return rb_Complex(rb_float128_cf128(z_real), rb_float128_cf128(z_imag));
#else
			__complex128 z;
			__real__ z = get_real(rb_complex_real(y));
			__imag__ z = get_real(rb_complex_imag(y));
			z = cpowq(x_real, z);
			return rb_Complex(
				rb_float128_cf128(crealq(z)), 
				rb_float128_cf128(cimagq(z)));
#endif
		}
}

static VALUE
float128_ope_nucomp(VALUE self, VALUE other, int ope)
{
	VALUE x = rb_Complex1(self);
	VALUE y = other;
	switch (ope) {
	case OPE_ADD:
		return rb_complex_plus(x, y);
		break;
	case OPE_SUB:
		return rb_complex_minus(x, y);
		break;
	case OPE_MUL:
		return rb_complex_mul(x, y);
		break;
	case OPE_DIV:
		return rb_complex_div(x, y);
		break;
	case OPE_MOD:
		// undefined
		return rb_num_coerce_bin(x, y, '%');
		break;
	case OPE_POW:
		return float128_nucomp_pow(self, other);
		break;
	case OPE_CMP:
		return rb_num_coerce_cmp(x, y, rb_intern("<=>"));
		break;
	case OPE_COERCE:
		return rb_assoc_new(y, x);
		break;
	default:
		unknown_opecode();
		return rb_float128_cf128(0.q);
		break;
	}
}

static VALUE
float128_ope_complex128(VALUE self, VALUE other, int ope)
{
	__float128 x = GetF128(self);
	__complex128 y = rb_complex128_value(other);
	
	switch (ope) {
	case OPE_ADD:
		return rb_complex128_cc128(x + y);
		break;
	case OPE_SUB:
		return rb_complex128_cc128(x - y);
		break;
	case OPE_MUL:
		return rb_complex128_cc128(x * y);
		break;
	case OPE_DIV:
		return rb_complex128_cc128(x / y);
		break;
	case OPE_MOD:
		return rb_complex128_cc128(cmodq(x, y));
		break;
	case OPE_POW:
		return rb_complex128_cc128(cpowq(x, y));
		break;
	case OPE_CMP:
		if (isnanq(x) || isnanq(crealq(y)) || (cimagq(y) != 0))
			return Qnil;
		else
		{
			__float128 y_real = crealq(y);
			if (x < y_real)
				return INT2NUM(-1);
			else if (x > y_real)
				return INT2NUM( 1);
			else /* (x == y_real) */
				return INT2NUM( 0);
		}
		break;
	case OPE_COERCE:
		return rb_assoc_new(
			rb_complex128_cc128(y), 
			rb_complex128_cc128((__complex128)x));
		break;
	default:
		unknown_opecode();
		return rb_float128_cf128(0.q);
		break;
	}
}

/*
 *  call-seq:
 *    Float128 + Numeric -> Float128 | Complex128 | Complex
 *  
 *  右オペランドを加算する．
 *  右オペランドが実数であればFloat128を，複素数であれば各自Complexクラスに変換し演算する．
 */
static VALUE
float128_add(VALUE self, VALUE other)
{
	switch (convertion_num_types(other)) {
	case NUM_FIXNUM:
	case NUM_BIGNUM:
		return float128_ope_integer(self, other, OPE_ADD);
		break;
	case NUM_RATIONAL:
		return float128_ope_rational(self, other, OPE_ADD);
		break;
	case NUM_FLOAT:
		return float128_ope_float(self, other, OPE_ADD);
		break;
	case NUM_COMPLEX:
		return float128_ope_nucomp(self, other, OPE_ADD);
		break;
	case NUM_FLOAT128:
		return float128_ope_float128(self, other, OPE_ADD);
		break;
	case NUM_COMPLEX128:
	case NUM_OTHERTYPE:
	default:
		return rb_num_coerce_bin(self, other, '+');
		break;
	}
}

/*
 *  call-seq:
 *    Float128 - Numeric -> Float128 | Complex128 | Complex
 *  
 *  右オペランドを減算する．
 *  右オペランドが実数であればFloat128を，複素数であれば各自Complexクラスに変換し演算する．
 */
static VALUE
float128_sub(VALUE self, VALUE other)
{
	switch (convertion_num_types(other)) {
	case NUM_FIXNUM:
	case NUM_BIGNUM:
		return float128_ope_integer(self, other, OPE_SUB);
		break;
	case NUM_RATIONAL:
		return float128_ope_rational(self, other, OPE_SUB);
		break;
	case NUM_FLOAT:
		return float128_ope_float(self, other, OPE_SUB);
		break;
	case NUM_COMPLEX:
		return float128_ope_nucomp(self, other, OPE_SUB);
		break;
	case NUM_FLOAT128:
		return float128_ope_float128(self, other, OPE_SUB);
		break;
	case NUM_COMPLEX128:
	case NUM_OTHERTYPE:
	default:
		return rb_num_coerce_bin(self, other, '-');
		break;
	}
}

/*
 *  call-seq:
 *    Float128 * Numeric -> Float128 | Complex128 | Complex
 *  
 *  右オペランドとの積を取る．
 *  右オペランドが実数であればFloat128を，複素数であれば各自Complexクラスに変換し演算する．
 */
static VALUE
float128_mul(VALUE self, VALUE other)
{
	switch (convertion_num_types(other)) {
	case NUM_FIXNUM:
	case NUM_BIGNUM:
		return float128_ope_integer(self, other, OPE_MUL);
		break;
	case NUM_RATIONAL:
		return float128_ope_rational(self, other, OPE_MUL);
		break;
	case NUM_FLOAT:
		return float128_ope_float(self, other, OPE_MUL);
		break;
	case NUM_COMPLEX:
		return float128_ope_nucomp(self, other, OPE_MUL);
		break;
	case NUM_FLOAT128:
		return float128_ope_float128(self, other, OPE_MUL);
		break;
	case NUM_COMPLEX128:
	case NUM_OTHERTYPE:
	default:
		return rb_num_coerce_bin(self, other, '*');
		break;
	}
}

/*
 *  call-seq:
 *    Float128 / Numeric -> Float128 | Complex128 | Complex
 *  
 *  右オペランドとの商を取る．
 *  右オペランドが実数であればFloat128を，複素数であれば各自Complexクラスに変換し演算する．
 */
static VALUE
float128_div(VALUE self, VALUE other)
{
	switch (convertion_num_types(other)) {
	case NUM_FIXNUM:
	case NUM_BIGNUM:
		return float128_ope_integer(self, other, OPE_DIV);
		break;
	case NUM_RATIONAL:
		return float128_ope_rational(self, other, OPE_DIV);
		break;
	case NUM_FLOAT:
		return float128_ope_float(self, other, OPE_DIV);
		break;
	case NUM_COMPLEX:
		return float128_ope_nucomp(self, other, OPE_DIV);
		break;
	case NUM_FLOAT128:
		return float128_ope_float128(self, other, OPE_DIV);
		break;
	case NUM_COMPLEX128:
	case NUM_OTHERTYPE:
	default:
		return rb_num_coerce_bin(self, other, '/');
		break;
	}
}

/*
 *  call-seq:
 *    Float128 % Numeric -> Float128
 *  
 *  右オペランドと剰余計算を行う．
 *  オペランド同士が同じ符号ならゼロの方向で除算し，同じでないなら負の方向で除算する．
 */
static VALUE
float128_mod(VALUE self, VALUE other)
{
	switch (convertion_num_types(other)) {
	case NUM_FIXNUM:
	case NUM_BIGNUM:
		return float128_ope_integer(self, other, OPE_MOD);
		break;
	case NUM_RATIONAL:
		return float128_ope_rational(self, other, OPE_MOD);
		break;
	case NUM_FLOAT:
		return float128_ope_float(self, other, OPE_MOD);
		break;
	case NUM_COMPLEX:
		return float128_ope_nucomp(self, other, OPE_MOD);
		break;
	case NUM_FLOAT128:
		return float128_ope_float128(self, other, OPE_MOD);
		break;
	case NUM_COMPLEX128:
	case NUM_OTHERTYPE:
	default:
		return rb_num_coerce_bin(self, other, '%');
		break;
	}
}

/*
 *  call-seq:
 *    Float128 ** Numeric -> Float128 | Complex128 | Complex
 *  
 *  右オペランドと累乗計算を行う．
 *  右オペランドが実数であればFloat128を，複素数であれば各自Complexクラスに変換し演算する．
 */
static VALUE
float128_pow(VALUE self, VALUE other)
{
	switch (convertion_num_types(other)) {
	case NUM_FIXNUM:
	case NUM_BIGNUM:
		return float128_ope_integer(self, other, OPE_POW);
		break;
	case NUM_RATIONAL:
		return float128_ope_rational(self, other, OPE_POW);
		break;
	case NUM_FLOAT:
		return float128_ope_float(self, other, OPE_POW);
		break;
	case NUM_COMPLEX:
		return float128_ope_nucomp(self, other, OPE_POW);
		break;
	case NUM_FLOAT128:
		return float128_ope_float128(self, other, OPE_POW);
		break;
	case NUM_COMPLEX128:
	case NUM_OTHERTYPE:
	default:
		return rb_num_coerce_bin(self, other, rb_intern("**"));
		break;
	}
}

/*
 *  call-seq:
 *    Float128 <=> Numeric -> -1 | 0 | 1 | nil
 *  
 *  オペランド同士の比較を行う．
 *  +othe+rが+self+より大きい場合は1，同じ場合は0，小さい場合は1を返す．比較できない場合はnilを返す．
 */
static VALUE
float128_cmp(VALUE self, VALUE other)
{
	switch (convertion_num_types(other)) {
	case NUM_FIXNUM:
	case NUM_BIGNUM:
		return float128_ope_integer(self, other, OPE_CMP);
		break;
	case NUM_RATIONAL:
		return float128_ope_rational(self, other, OPE_CMP);
		break;
	case NUM_FLOAT:
		return float128_ope_float(self, other, OPE_CMP);
		break;
	case NUM_COMPLEX:
		return float128_ope_nucomp(self, other, OPE_CMP);
		break;
	case NUM_FLOAT128:
		return float128_ope_float128(self, other, OPE_CMP);
		break;
	case NUM_COMPLEX128:
		return float128_ope_complex128(self, other, OPE_CMP);
		break;
	case NUM_OTHERTYPE:
	default:
		return rb_num_coerce_cmp(self, other, rb_intern("<=>"));
		break;
	}
}

/*
 *  call-seq:
 *    coerce(other) -> [other, self]
 *  
 *  +other+を+self+と等しくしたうえで[other, self]のペア配列にしてこれを返す．
 *  
 *    1.0.coerce(Float128::MAX).all?(Float)         # => true
 *    1.0.to_f128.coerce(Float::MAX).all?(Float128) # => true
 */
static VALUE
float128_coerce(int argc, VALUE *argv, VALUE self)
{
	VALUE other;
	rb_scan_args(argc, argv, "10", &other);
	switch (convertion_num_types(other)) {
	case NUM_FIXNUM:
	case NUM_BIGNUM:
		return float128_ope_integer(self, other, OPE_COERCE);
		break;
	case NUM_RATIONAL:
		return float128_ope_rational(self, other, OPE_COERCE);
		break;
	case NUM_FLOAT:
		return float128_ope_float(self, other, OPE_COERCE);
		break;
	case NUM_COMPLEX:
		return float128_ope_nucomp(self, other, OPE_COERCE);
		break;
	case NUM_FLOAT128:
		return float128_ope_float128(self, other, OPE_COERCE);
		break;
	case NUM_COMPLEX128:
		return float128_ope_complex128(self, other, OPE_COERCE);
		break;
	case NUM_OTHERTYPE:
	default:
		return rb_call_super(argc, argv);
		break;
	}
}


static VALUE
float128_fused_multiply_add(VALUE xhs, VALUE yhs, VALUE zhs)
{
	__float128 x = get_real(xhs),
	           y = get_real(yhs),
	           z = get_real(zhs);
	
	return rb_float128_cf128(fmaq(x, y, z));
}

/*
 *  call-seq:
 *    Float128.fma(x, y, z) -> Float128
 *  
 *  融合積和演算法を使用し，x*y+zを効率よく計算する．
 *  多くの場合，数式通りに計算するよりも正確な結果を期待できる．
 *  引数は四倍精度の浮動小数点を考慮する．
 *  考慮できない場合，暗黙の型変換を試みるが，できない場合はTypeErrorを引き起こす．
 *  
 *    Float128.fma(1/3r, 2/3r, 3/3r) # => 1.2222222222222222222222222222222
 *    # 精度の違い
 *    Float128.fma(1.1, 1.2, 1.3) # => 2.6200000000000001021405182655144
 *    Float128.fma('1.1'.to_f128, '1.2'.to_f128, '1.3'.to_f128) # => 2.62
 */
static VALUE
float128_s_fma(int argc, VALUE *argv, VALUE self)
{
	VALUE xhs, yhs, zhs;
	rb_scan_args(argc, argv, "30", &xhs, &yhs, &zhs);
	
	return float128_fused_multiply_add(xhs, yhs, zhs);
}

static VALUE
float128_sincos(VALUE xhs)
{
	__float128 x = get_real(xhs), sin, cos;
	sincosq(x, &sin, &cos);
	return rb_assoc_new( rb_float128_cf128(sin), rb_float128_cf128(cos) );
}

/*
 *  call-seq:
 *    Float128.sincos(x) -> [Float128, Float128]
 *  
 *  xの正弦と余弦を配列で返す．
 *  引数は四倍精度の浮動小数点を考慮する．
 *  考慮できない場合，暗黙の型変換を試みるが，できない場合はTypeErrorを引き起こす．
 *  
 *    Float128.sincos(2) # => [0.909297426825681695396019865911745, -0.416146836547142386997568229500762]
 */
static VALUE
float128_s_sincos(int argc, VALUE *argv, VALUE self)
{
	VALUE xhs;
	rb_scan_args(argc, argv, "10", &xhs);
	
	return float128_sincos(xhs);
}

static VALUE
float128_fmin(VALUE lhs, VALUE rhs)
{
	__float128 l = get_real(lhs), r = get_real(rhs);
	return rb_float128_cf128(fminq(l, r));
}

/*
 *  call-seq:
 *    Float128.fmin(l, r) -> Float128
 *  
 *  引数lとrを比べて小さいほうを返す．
 *  引数は四倍精度の浮動小数点を考慮する．
 *  考慮できない場合，暗黙の型変換を試みるが，できない場合はTypeErrorを引き起こす．
 *  
 *    Float128.fmin(1.23r, 4.56r) # => 1.23
 *    # 二倍精度と四倍精度の同値を比較
 *    Float128.fmin(1.1r, 1.1r) # => 1.1
 *    Float128.fmin(-1.1r, -1.1) # => -1.1000000000000000888178419700125
 */
static VALUE
float128_s_fmin(int argc, VALUE *argv, VALUE self)
{
	VALUE lhs, rhs;
	rb_scan_args(argc, argv, "20", &lhs, &rhs);
	
	return float128_fmin(lhs, rhs);
}

static VALUE
float128_fmax(VALUE lhs, VALUE rhs)
{
	__float128 l = get_real(lhs), r = get_real(rhs);
	return rb_float128_cf128(fmaxq(l, r));
}

/*
 *  call-seq:
 *    Float128.fmax(l, r) -> Float128
 *  
 *  引数lとrを比べて大きいほうを返す．
 *  引数は四倍精度の浮動小数点を考慮する．
 *  考慮できない場合，暗黙の型変換を試みるが，できない場合はTypeErrorを引き起こす．
 *  
 *    Float128.fmax(1.23r, 4.56r) # => 4.56
 *    # 二倍精度と四倍精度の同値を比較
 *    Float128.fmax(1.1r, 1.1r) # => 1.1000000000000000888178419700125
 *    Float128.fmax(-1.1r, -1.1) # => -1.1
 */
static VALUE
float128_s_fmax(int argc, VALUE *argv, VALUE self)
{
	VALUE lhs, rhs;
	rb_scan_args(argc, argv, "20", &lhs, &rhs);
	
	return float128_fmax(lhs, rhs);
}

/*
 *  call-seq:
 *    Float128.ldexp(x, exp) -> Float128
 *  
 *  xに2のexp乗をかけた値を返す(Load Exponent)．Mathモジュールにも同名関数があるが，返却値は四倍精度である．
 *  frexpで分解した指数と仮数をもとの数値に戻すのに使う．
 *  
 *    fra, exp = '2.2'.to_f128.frexp # => [0.55, 2]
 *    Float128.ldexp(fra, exp) # => 2.2
 */
static VALUE
float128_s_ldexp(VALUE obj, VALUE x, VALUE exp)
{
	__float128 v_x = get_real(x);
	int v_exp = NUM2INT(exp);
	
	return rb_float128_cf128(ldexpq(v_x, v_exp));
}

/*
 *  call-seq:
 *    Float128.scalb(x, n) -> Float128
 *    Float128.scalbn(x, n) -> Float128
 *  
 *  xにFloat::RADIXのn乗をかけた値を返す．返却値は四倍精度である．
 *  メソッド名はC/C++の策定のときのオリジナルだが，(もちろん)newの意味を持つnが付け加えられたaliasもある．
 *  内部的にはnがlong型パージョンなscalblnq()を使用する．
 *  
 *    # 3.0 * (Float::RADIX * 4)
 *    Float128.scalb(3, 4) # => 48.0
 */
static VALUE
float128_s_scalb(VALUE obj, VALUE x, VALUE exp)
{
	__float128 v_x = get_real(x);
	long v_exp = NUM2LONG(exp);
	
	return rb_float128_cf128(scalblnq(v_x, v_exp));
}


/*
 *  call-seq:
 *    polar -> [Float128, Float128]
 *  
 *  +self+の絶対値と偏角を配列にして返す．成分はいずれもFloat128である．
 *  
 *    Complex128.polar(1, 2).polar # => [1, 2]
 */
static VALUE
complex128_polar(VALUE self)
{
	__complex128 c128 = GetC128(self);
	
	return rb_assoc_new(
		rb_float128_cf128(cabsq(c128)), 
		rb_float128_cf128(cargq(c128)));
}

/*
 *  call-seq:
 *    abs -> Float128
 *    magnitude -> Float128
 *  
 *    +self+の絶対値を返す．
 */
static VALUE
complex128_abs(VALUE self)
{
	__complex128 c128 = GetC128(self);
	
	return rb_float128_cf128(cabsq(c128));
}

/*
 *  call-seq:
 *    abs2 -> Float128
 *  
 *  +self+の絶対値の二乗を返す．
 */
static VALUE
complex128_abs2(VALUE self)
{
	__complex128 c128 = GetC128(self);
	__float128 real, imag;
	
	real = crealq(c128); imag = cimagq(c128);
	
	return rb_float128_cf128(real * real + imag * imag);
}

/*
 *  call-seq:
 *    arg -> Float128
 *    angle -> Float128
 *    phase -> Float128
 *  
 *  +self+の偏角を[-π,π]の範囲で返す．
 */
static VALUE
complex128_arg(VALUE self)
{
	__complex128 c128 = GetC128(self);
	
	return rb_float128_cf128(cargq(c128));
}

/*
 *  call-seq:
 *    conj -> Complex128
 *  
 *  +self+の共役複素数を返す．
 */
static VALUE
complex128_conj(VALUE self)
{
	__complex128 c128 = GetC128(self);
	
	return rb_complex128_cc128(conjq(c128));
}


__complex128
cmodq(__complex128 z, __complex128 w)
{
	return 0+0i; // 未定義
}


static VALUE
complex128_ope_integer(VALUE self, VALUE other, int ope)
{
	__complex128 z = GetC128(self);
	__float128 w = integer_to_cf128(other);
	
	switch (ope) {
	case OPE_ADD:
		return rb_complex128_cc128(z + w);
		break;
	case OPE_SUB:
		return rb_complex128_cc128(z - w);
		break;
	case OPE_MUL:
		return rb_complex128_cc128(z * w);
		break;
	case OPE_DIV:
		return rb_complex128_cc128(z / w);
		break;
	case OPE_MOD:
		return rb_complex128_cc128(cmodq(z, w));
		break;
	case OPE_POW:
		return rb_complex128_cc128(cpowq(z, w));
		break;
	case OPE_CMP:
		__float128 z_real = crealq(z);
		if (isnanq(z_real) || cimagq(z) != 0)
			return Qnil;
		else if (z_real <  w)
			return INT2NUM(-1);
		else if (z_real >  w)
			return INT2NUM(1);
		else /* (z_real == w) */
			return INT2NUM(0);
		break;
	case OPE_COERCE:
		return rb_assoc_new(
			rb_complex128_cc128(w),
			rb_complex128_cc128(z));
		break;
	default:
		unknown_opecode();
		return rb_complex128_cc128(0+0i);
		break;
	}
}

static VALUE
complex128_ope_rational(VALUE self, VALUE other, int ope)
{
	__complex128 z = GetC128(self);
	__float128 w = rational_to_cf128(other);
	
	switch (ope) {
	case OPE_ADD:
		return rb_complex128_cc128(z + w);
		break;
	case OPE_SUB:
		return rb_complex128_cc128(z - w);
		break;
	case OPE_MUL:
		return rb_complex128_cc128(z * w);
		break;
	case OPE_DIV:
		return rb_complex128_cc128(z / w);
		break;
	case OPE_MOD:
		return rb_complex128_cc128(cmodq(z, w));
		break;
	case OPE_POW:
		return rb_complex128_cc128(cpowq(z, w));
		break;
	case OPE_CMP:
		__float128 z_real = crealq(z);
		if (isnanq(z_real) || cimagq(z) != 0)
			return Qnil;
		else if (z_real <  w)
			return INT2NUM(-1);
		else if (z_real >  w)
			return INT2NUM(1);
		else /* (z_real == w) */
			return INT2NUM(0);
		break;
	case OPE_COERCE:
		return rb_assoc_new(
			rb_complex128_cc128(w),
			rb_complex128_cc128(z));
		break;
	default:
		unknown_opecode();
		return rb_complex128_cc128(0+0i);
		break;
	}
}

static VALUE
complex128_ope_float(VALUE self, VALUE other, int ope)
{
	__complex128 z = GetC128(self);
	__float128 w = (__float128)NUM2DBL(other);
	
	switch (ope) {
	case OPE_ADD:
		return rb_complex128_cc128(z + w);
		break;
	case OPE_SUB:
		return rb_complex128_cc128(z - w);
		break;
	case OPE_MUL:
		return rb_complex128_cc128(z * w);
		break;
	case OPE_DIV:
		return rb_complex128_cc128(z / w);
		break;
	case OPE_MOD:
		return rb_complex128_cc128(cmodq(z, w));
		break;
	case OPE_POW:
		return rb_complex128_cc128(cpowq(z, w));
		break;
	case OPE_CMP:
		__float128 z_real = crealq(z);
		if (isnanq(z_real) || isnanq(w) || cimagq(z) != 0)
			return Qnil;
		else if (z_real <  w)
			return INT2NUM(-1);
		else if (z_real >  w)
			return INT2NUM(1);
		else /* (z_real == w) */
			return INT2NUM(0);
		break;
	case OPE_COERCE:
		return rb_assoc_new(
			rb_complex128_cc128(w),
			rb_complex128_cc128(z));
		break;
	default:
		unknown_opecode();
		return rb_complex128_cc128(0+0i);
		break;
	}
}

static VALUE
complex128_ope_float128(VALUE self, VALUE other, int ope)
{
	__complex128 z = GetC128(self);
	__float128 w = GetF128(other);
	
	switch (ope) {
	case OPE_ADD:
		return rb_complex128_cc128(z + w);
		break;
	case OPE_SUB:
		return rb_complex128_cc128(z - w);
		break;
	case OPE_MUL:
		return rb_complex128_cc128(z * w);
		break;
	case OPE_DIV:
		return rb_complex128_cc128(z / w);
		break;
	case OPE_MOD:
		return rb_complex128_cc128(cmodq(z, w));
		break;
	case OPE_POW:
		return rb_complex128_cc128(cpowq(z, w));
		break;
	case OPE_CMP:
		__float128 z_real = crealq(z);
		if (isnanq(z_real) || isnanq(w) || cimagq(z) != 0)
			return Qnil;
		else if (z_real <  w)
			return INT2NUM(-1);
		else if (z_real >  w)
			return INT2NUM(1);
		else /* (z_real == w) */
			return INT2NUM(0);
		break;
	case OPE_COERCE:
		return rb_assoc_new(
			rb_complex128_cc128(w),
			rb_complex128_cc128(z));
		break;
	default:
		unknown_opecode();
		return rb_complex128_cc128(0+0i);
		break;
	}
}

VALUE
complex128_nucomp_mod(VALUE z, VALUE w)
{
	return rb_Complex1(INT2FIX(0)); // 未定義
}

VALUE
complex128_nucomp_pow(VALUE z, VALUE w)
{
	__complex128 z_value = GetC128(z);
	__complex128 w_value;
	__real__ w_value = get_real(rb_complex_real(w));
	__imag__ w_value = get_real(rb_complex_imag(w));
	__complex128 c = cpowq(z_value, w_value);
	return rb_Complex(
		rb_float128_cf128(crealq(c)), 
		rb_float128_cf128(cimagq(c)));
}

static VALUE
complex128_ope_nucomp(VALUE self, VALUE other, int ope)
{
	__complex128 c128 = GetC128(self);
	VALUE x = rb_Complex(
		rb_float128_cf128(crealq(c128)), 
		rb_float128_cf128(cimagq(c128)));
	VALUE y = other;
	switch (ope) {
	case OPE_ADD:
		return rb_complex_plus(x, y);
		break;
	case OPE_SUB:
		return rb_complex_minus(x, y);
		break;
	case OPE_MUL:
		return rb_complex_mul(x, y);
		break;
	case OPE_DIV:
		return rb_complex_div(x, y);
		break;
	case OPE_MOD:
		return complex128_nucomp_mod(self, other);
		break;
	case OPE_POW:
		return complex128_nucomp_pow(self, other);
		break;
	case OPE_CMP:
		return rb_num_coerce_cmp(x, y, rb_intern("<=>"));
		break;
	case OPE_COERCE:
		return rb_assoc_new(y, x);
		break;
	default:
		unknown_opecode();
		return rb_complex128_cc128(0+0i);
		break;
	}
}

static VALUE
complex128_ope_complex128(VALUE self, VALUE other, int ope)
{
	__complex128 z = GetC128(self);
	__complex128 w = GetC128(other);
	
	switch (ope) {
	case OPE_ADD:
		return rb_complex128_cc128(z + w);
		break;
	case OPE_SUB:
		return rb_complex128_cc128(z - w);
		break;
	case OPE_MUL:
		return rb_complex128_cc128(z * w);
		break;
	case OPE_DIV:
		return rb_complex128_cc128(z / w);
		break;
	case OPE_MOD:
		return rb_complex128_cc128(cmodq(z, w));
		break;
	case OPE_POW:
		return rb_complex128_cc128(cpowq(z, w));
		break;
	case OPE_CMP:
		__float128 z_real = crealq(z), z_imag = cimagq(z);
		__float128 w_real = crealq(z), w_imag = cimagq(z);
		if (isnanq(crealq(z)) || isnanq(cimagq(z)) ||
		    isnanq(crealq(w)) || isnanq(cimagq(w)))
			return Qnil;
		else
		{
			if (z < w)
				return INT2NUM(-1);
			else if (z > w)
				return INT2NUM( 1);
			else /* (z == w) */
				return INT2NUM( 0);
		}
		break;
	case OPE_COERCE:
		return rb_assoc_new(other, self);
		break;
	default:
		unknown_opecode();
		return rb_complex128_cc128(0+0i);
		break;
	}
}



/*
 *  call-seq:
 *    Complex128 + Numeric -> Complex128 | Complex
 *  
 *  右オペランドを加算する．
 *  右オペランドがComplexクラスであればComplexクラスを，そのほかはComplex128クラスを返却値にそれぞれ演算する．
 */
static VALUE
complex128_add(VALUE self, VALUE other)
{
	switch (convertion_num_types(other)) {
	case NUM_FIXNUM:
	case NUM_BIGNUM:
		return complex128_ope_integer(self, other, OPE_ADD);
		break;
	case NUM_RATIONAL:
		return complex128_ope_rational(self, other, OPE_ADD);
		break;
	case NUM_FLOAT:
		return complex128_ope_float(self, other, OPE_ADD);
		break;
	case NUM_COMPLEX:
		return complex128_ope_nucomp(self, other, OPE_ADD);
		break;
	case NUM_FLOAT128:
		return complex128_ope_float128(self, other, OPE_ADD);
		break;
	case NUM_COMPLEX128:
		return complex128_ope_complex128(self, other, OPE_ADD);
		break;
	case NUM_OTHERTYPE:
	default:
		return rb_num_coerce_bin(self, other, '+');
		break;
	}
}

/*
 *  call-seq:
 *    Complex128 - Numeric -> Complex128 | Complex
 *  
 *  右オペランドを減算する．
 *  右オペランドがComplexクラスであればComplexクラスを，そのほかはComplex128クラスを返却値にそれぞれ演算する．
 */
static VALUE
complex128_sub(VALUE self, VALUE other)
{
	switch (convertion_num_types(other)) {
	case NUM_FIXNUM:
	case NUM_BIGNUM:
		return complex128_ope_integer(self, other, OPE_SUB);
		break;
	case NUM_RATIONAL:
		return complex128_ope_rational(self, other, OPE_SUB);
		break;
	case NUM_FLOAT:
		return complex128_ope_float(self, other, OPE_SUB);
		break;
	case NUM_COMPLEX:
		return complex128_ope_nucomp(self, other, OPE_SUB);
		break;
	case NUM_FLOAT128:
		return complex128_ope_float128(self, other, OPE_SUB);
		break;
	case NUM_COMPLEX128:
		return complex128_ope_complex128(self, other, OPE_SUB);
		break;
	case NUM_OTHERTYPE:
	default:
		return rb_num_coerce_bin(self, other, '-');
		break;
	}
}

/*
 *  call-seq:
 *    Complex128 * Numeric -> Complex128 | Complex
 *  
 *  右オペランドとの積を取る．
 *  右オペランドがComplexクラスであればComplexクラスを，そのほかはComplex128クラスを返却値にそれぞれ演算する．
 */
static VALUE
complex128_mul(VALUE self, VALUE other)
{
	switch (convertion_num_types(other)) {
	case NUM_FIXNUM:
	case NUM_BIGNUM:
		return complex128_ope_integer(self, other, OPE_MUL);
		break;
	case NUM_RATIONAL:
		return complex128_ope_rational(self, other, OPE_MUL);
		break;
	case NUM_FLOAT:
		return complex128_ope_float(self, other, OPE_MUL);
		break;
	case NUM_COMPLEX:
		return complex128_ope_nucomp(self, other, OPE_MUL);
		break;
	case NUM_FLOAT128:
		return complex128_ope_float128(self, other, OPE_MUL);
		break;
	case NUM_COMPLEX128:
		return complex128_ope_complex128(self, other, OPE_MUL);
		break;
	case NUM_OTHERTYPE:
	default:
		return rb_num_coerce_bin(self, other, '*');
		break;
	}
}

/*
 *  call-seq:
 *    Complex128 / Numeric -> Complex128 | Complex
 *  
 *  右オペランドとの商を取る．
 *  右オペランドがComplexクラスであればComplexクラスを，そのほかはComplex128クラスを返却値にそれぞれ演算する．
 */
static VALUE
complex128_div(VALUE self, VALUE other)
{
	switch (convertion_num_types(other)) {
	case NUM_FIXNUM:
	case NUM_BIGNUM:
		return complex128_ope_integer(self, other, OPE_DIV);
		break;
	case NUM_RATIONAL:
		return complex128_ope_rational(self, other, OPE_DIV);
		break;
	case NUM_FLOAT:
		return complex128_ope_float(self, other, OPE_DIV);
		break;
	case NUM_COMPLEX:
		return complex128_ope_nucomp(self, other, OPE_DIV);
		break;
	case NUM_FLOAT128:
		return complex128_ope_float128(self, other, OPE_DIV);
		break;
	case NUM_COMPLEX128:
		return complex128_ope_complex128(self, other, OPE_DIV);
		break;
	case NUM_OTHERTYPE:
	default:
		return rb_num_coerce_bin(self, other, '/');
		break;
	}
}

/*
 *  call-seq:
 *    Complex128 % Numeric -> Complex128 | Complex
 *  
 *  右オペランドと剰余計算を行う．
 *  このメソッドは実装定義である.
 */
static VALUE
complex128_mod(VALUE self, VALUE other)
{
	switch (convertion_num_types(other)) {
	case NUM_FIXNUM:
	case NUM_BIGNUM:
		return complex128_ope_integer(self, other, OPE_MOD);
		break;
	case NUM_RATIONAL:
		return complex128_ope_rational(self, other, OPE_MOD);
		break;
	case NUM_FLOAT:
		return complex128_ope_float(self, other, OPE_MOD);
		break;
	case NUM_COMPLEX:
		return complex128_ope_nucomp(self, other, OPE_MOD);
		break;
	case NUM_FLOAT128:
		return complex128_ope_float128(self, other, OPE_MOD);
		break;
	case NUM_COMPLEX128:
		return complex128_ope_complex128(self, other, OPE_MOD);
		break;
	case NUM_OTHERTYPE:
	default:
		return rb_num_coerce_bin(self, other, '%');
		break;
	}
}

/*
 *  call-seq:
 *    Complex128 ** Numeric -> Complex128 | Complex
 *  
 *  右オペランドと累乗計算を行う．
 *  右オペランドがComplexクラスであればComplexクラスを，そのほかはComplex128クラスを返却値にそれぞれ演算する．
 */
static VALUE
complex128_pow(VALUE self, VALUE other)
{
	switch (convertion_num_types(other)) {
	case NUM_FIXNUM:
	case NUM_BIGNUM:
		return complex128_ope_integer(self, other, OPE_POW);
		break;
	case NUM_RATIONAL:
		return complex128_ope_rational(self, other, OPE_POW);
		break;
	case NUM_FLOAT:
		return complex128_ope_float(self, other, OPE_POW);
		break;
	case NUM_COMPLEX:
		return complex128_ope_nucomp(self, other, OPE_POW);
		break;
	case NUM_FLOAT128:
		return complex128_ope_float128(self, other, OPE_POW);
		break;
	case NUM_COMPLEX128:
		return complex128_ope_complex128(self, other, OPE_POW);
		break;
	case NUM_OTHERTYPE:
	default:
		return rb_num_coerce_bin(self, other, rb_intern("**"));
		break;
	}
}

/*
 *  call-seq:
 *    Complex128 <=> Numeric -> -1 | 0 | 1 | nil
 *  
 *  オペランド同士の比較を行う．
 */
static VALUE
complex128_cmp(VALUE self, VALUE other)
{
	switch (convertion_num_types(other)) {
	case NUM_FIXNUM:
	case NUM_BIGNUM:
		return complex128_ope_integer(self, other, OPE_CMP);
		break;
	case NUM_RATIONAL:
		return complex128_ope_rational(self, other, OPE_CMP);
		break;
	case NUM_FLOAT:
		return complex128_ope_float(self, other, OPE_CMP);
		break;
	case NUM_COMPLEX:
		return complex128_ope_nucomp(self, other, OPE_CMP);
		break;
	case NUM_FLOAT128:
		return complex128_ope_float128(self, other, OPE_CMP);
		break;
	case NUM_COMPLEX128:
		return complex128_ope_complex128(self, other, OPE_CMP);
		break;
	case NUM_OTHERTYPE:
	default:
		return rb_num_coerce_cmp(self, other, rb_intern("<=>"));
		break;
	}
}

/*
 *  call-seq:
 *    coerce(other) -> [other, self]
 *  
 *  +other+を+self+と等しくしたうえで[other, self]のペア配列にしてこれを返す．
 */
static VALUE
complex128_coerce(int argc, VALUE *argv, VALUE self)
{
	VALUE other;
	rb_scan_args(argc, argv, "10", &other);
	switch (convertion_num_types(other)) {
	case NUM_FIXNUM:
	case NUM_BIGNUM:
		return complex128_ope_integer(self, other, OPE_COERCE);
		break;
	case NUM_RATIONAL:
		return complex128_ope_rational(self, other, OPE_COERCE);
		break;
	case NUM_FLOAT:
		return complex128_ope_float(self, other, OPE_COERCE);
		break;
	case NUM_COMPLEX:
		return complex128_ope_nucomp(self, other, OPE_COERCE);
		break;
	case NUM_FLOAT128:
		return complex128_ope_float128(self, other, OPE_COERCE);
		break;
	case NUM_COMPLEX128:
		return complex128_ope_complex128(self, other, OPE_COERCE);
		break;
	case NUM_OTHERTYPE:
	default:
		return rb_call_super(argc, argv);
		break;
	}
}

static VALUE
complex128_polarize(VALUE rho, VALUE theta)
{
	__float128 abs = get_real(rho),
	           arg = get_real(theta);
	__complex128 expi = cexpiq(arg);
	return rb_complex128_cc128(abs * expi);
}

/*
 *  call-seq:
 *    Complex128.polar(rho, theta = 0) -> Complex128
 *  
 *  極座標(絶対値と偏角)からComplex128を生成する．偏角はラジアンを与える．
 *  絶対値のみ与えた場合，偏角の値は0である．
 *  引数は四倍精度の浮動小数点を考慮する．
 *  考慮できない場合，暗黙の型変換を試みるが，できない場合はTypeErrorを引き起こす．
 *  リテラルは浮動小数点のとき内部的には二倍->四倍と型変換する．そのため精度が落ちる場合がある．
 *  
 *    # 整数リテラルでの生成
 *    Complex128.polar(3) # => (3.0+0.0i)
 *    # 浮動小数点リテラルでの生成．ある程度は精度落ちしない
 *    Complex.polar(3, 2.0) # => (-1.2484405096414273+2.727892280477045i)
 *    Complex128.polar(3, 2.0) # => (-1.2484405096414271609927046885022+2.7278922804770450861880595977352i)
 *    # 精度比較．ある環境の場合
 *    Complex128.polar(3, Math::PI) # => (-2.9999999999999999999999999999999+3.67394039744205953167819779682499e-16i)
 *    Complex128.polar(3, QuadMath::PI) # => (-3.0+2.60154303903713430743911320781301e-34i)
 */
static VALUE
complex128_s_polar(int argc, VALUE *argv, VALUE self)
{
	VALUE rho, theta;
	if (argc == 1)
	{
		theta = INT2FIX(0);
		rb_scan_args(argc, argv, "10", &rho);
	}
	else
	{
		rb_scan_args(argc, argv, "11", &rho, &theta);
	}
	return complex128_polarize(rho, theta);
}

static VALUE
complex128_rectangularize(VALUE r, VALUE i)
{
	__float128 real = get_real(r),
	           imag = get_real(i);
	__complex128 z;
	__real__ z = real;
	__imag__ z = imag;
	return rb_complex128_cc128(z);
}

/*
 *  call-seq:
 *    Complex128.rect(real, imag = 0) -> Complex128
 *    Complex128.rectangular(real, imag = 0) -> Complex128
 *  
 *  直交座標からComplex128を生成する．
 *  引数は四倍精度の浮動小数点を考慮する．
 *  考慮できない場合，暗黙の型変換を試みるが，できない場合はTypeErrorを引き起こす．
 *  
 *    Complex128.rect(3)  # => (3.0+0.0i)
 *    
 *    Complex128.rect(3, QuadMath::PI) # => (3.0+3.1415926535897932384626433832795i)
 *    # 二倍精度のπで生成．情報が足りないため途中から精度落ちする
 *    Complex128.rect(3, Math::PI) # => (3.0+3.1415926535897931159979634685441i)
 */
static VALUE
complex128_s_rect(int argc, VALUE *argv, VALUE self)
{
	VALUE r, i;
	if (argc == 1)
	{
		i = INT2FIX(0);
		rb_scan_args(argc, argv, "10", &r);
	}
	else
	{
		rb_scan_args(argc, argv, "11", &r, &i);
	}
	return complex128_rectangularize(r, i);
}


/*
 *  call-seq:
 *    strtoflt128(str) -> Float128
 *    strtoflt128(str, sp: "") -> Float128
 *  
 *  ライブラリ関数strtoflt128のフロントエンドである．第一引数strにStringを与え，Float128型へ変換する．
 *  キーワード引数spを与えると，アウトバッファとしてstrtoflt128の第二引数spを受け取る．
 *  
 *    strtoflt128('inf') # => Infinity
 *    strtoflt128('-1') # => -1.0
 *    strtoflt128('0xdeadbeef') # => 3735928559.0
 *    sp = '' # => ""
 *    strtoflt128('0+1i', sp: sp) # => 0.0
 *    sp # => "+1i"
 */
static VALUE
f_strtoflt128(int argc, VALUE *argv, VALUE self)
{
	static ID kwds[1];
	VALUE str, opts, sp;
	__float128 x;
	
	if (!kwds[0])  kwds[0] = rb_intern_const("sp");
	
	rb_scan_args(argc, argv, "10:", &str, &opts);
	
	if (argc != 1)
	{
		char *next_pointer;
		
		rb_get_kwargs(opts, kwds, 0, 1, &sp);
		
		StringValue(sp);
		rb_str_resize(sp, 0);
		
		x = strtoflt128(StringValuePtr(str), &next_pointer);
		
		rb_str_cat_cstr(sp, next_pointer);
	}
	else
		x = strtoflt128(StringValuePtr(str), NULL);
	
	return rb_float128_cf128(x);
}

static int
xsnprintf(char *buf, size_t sz, char *format, int width, int prec, __float128 x)
{
	int n;
	if (width == 0 && prec == 0)
		n = quadmath_snprintf(buf, sz, format, x);
	else if (width != 0 && prec == 0)
		n = quadmath_snprintf(buf, sz, format, width, x);
	else if (width == 0 && prec != 0)
		n = quadmath_snprintf(buf, sz, format, prec, x);
	else /* if (width != 0 && prec != 0) */
		n = quadmath_snprintf(buf, sz, format, width, prec, x);
	return n;
}

/*
 *  call-seq:
 *    quadmath_sprintf(format, *arg) -> String
 *  
 *  quadmath_snprintf()ライブラリ関数をアクセスできるようにしたフロントエンドである．
 *  メソッド名と引数の数がライブラリのものとは少し異なるところに気をつけよう．
 *  
 *  フォーマットはライブラリ関数をふまえる．
 *  ライブラリ関数の引数は可変長であり，コンパイラが判断する．そのため，このフロントエンドではパーサで対応している．
 *  パーサは変換可能と判断次第，一つのフォーマット・エラーも許さない中でStringへの変換を試みる．
 *  
 *  文字列は内部バッファが保持し，ユーザレベル側へ提供する．
 *  もし内部バッファが足りず関数がエラーを出すと，自動的にメモリを確保しこれを渡す．
 *  
 *    quadmath_sprintf("%Qf", 2) # => "2.000000"
 *    quadmath_sprintf("%Qf", 7/10r) # => "0.700000"
 *    quadmath_sprintf("%.*Qf", Float128::DIG, 1/3r) # => "0.333333333333333333333333333333333"
 *    quadmath_sprintf("%.*Qf", Float128::DIG, 1.0/3.0) # => "0.333333333333333314829616256247391"
 *    quadmath_sprintf("%.*Qf", Float128::DIG, 1.to_f128 / 3) # => "0.333333333333333333333333333333333"
 *    width = 46; prec = 20;
 *    quadmath_sprintf("%+-#*.*Qe", width, prec, QuadMath.sqrt(2)) # => "+1.41421356237309504880e+00                   "
 */
static VALUE
f_quadmath_sprintf(int argc, VALUE *argv, VALUE self)
{
	VALUE vformat, arg, apptd, retval;
	char *format;
	int fmt_stat = FMT_EMPTY, flags = 0, width = 0, prec = 0, float_type = FT_FLT;
	long arg_offset = 0;
	char notation = 'f';
	
	rb_scan_args(argc, argv, "1*", &vformat, &arg);
	
	format = StringValuePtr(vformat);
	retval = rb_str_new(0,0);
	
	for (long i = 0; i < RSTRING_LEN(vformat); i++)
	{
		const char c = format[i];
		
		if (fmt_stat == FMT_EMPTY && c != '%')
			continue;
		
		switch (c) {
		case '%':
			if (fmt_stat == FMT_EMPTY)
				fmt_stat = FMT_DRCTV;
			else
				goto fmt_error;
			break;
		case '#':
			if (fmt_stat == FMT_DRCTV)
				flags |= FLAG_SHARP;
			else
				goto fmt_error;
			break;
		case ' ':
			if (fmt_stat == FMT_DRCTV)
				flags |= FLAG_SPACE;
			else
				goto fmt_error;
			break;
		case '+':
			if (fmt_stat == FMT_DRCTV)
				flags |= FLAG_PLUS;
			else
				goto fmt_error;
			break;
		case '-':
			if (fmt_stat == FMT_DRCTV)
				flags |= FLAG_MINUS;
			else
				goto fmt_error;
			break;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			if (fmt_stat == FMT_DRCTV)
			{
				if (c == '0')
					flags |= FLAG_ZERO;
				else
					fmt_stat = FMT_WIDTH;
			}
			else if (fmt_stat == FMT_POINT)
			{
				fmt_stat = FMT_PREC;
			}
			
			if (fmt_stat == FMT_WIDTH)
			{
				width = width * 10 + (c - '0');
				if (width < 0)
					goto biggest_width_error;
			}
			else if (fmt_stat == FMT_PREC)
			{
				prec = prec * 10 + (c - '0');
				if (prec < 0)
					goto biggest_prec_error;
			}
			else
				goto fmt_error;
			
			break;
		case '*':
			if (fmt_stat == FMT_DRCTV)
				fmt_stat = FMT_WIDTH_SCALAR;
			else if (fmt_stat == FMT_POINT)
				fmt_stat = FMT_PREC_SCALAR;
			else
				goto fmt_error;
			
			if (RARRAY_LEN(arg) <= arg_offset)
				goto too_few_arguments;
			else
			{
				VALUE item = rb_ary_entry(arg, arg_offset);
				long n;
				
				if (!RB_TYPE_P(item, T_FIXNUM) ||
				    !RB_TYPE_P(item, T_BIGNUM))
					item = rb_Integer(item);
				
				n = NUM2LONG(item);
				
				if (fmt_stat == FMT_WIDTH_SCALAR)
				{
					if (n < 0)  goto biggest_width_error;
					width = (int)n;
				}
				else /* if (fmt_stat == FMT_PREC_SCALAR) */
				{
					if (n < 0)  goto biggest_prec_error;
					prec = (int)n;
				}
				
				arg_offset++;
			}
			break;
		case '.':
			switch (fmt_stat) {
			case FMT_DRCTV:
			case FMT_WIDTH:
			case FMT_WIDTH_SCALAR:
				fmt_stat = FMT_POINT;
				break;
			default:
				goto fmt_error;
			}
			break;
		case 'l':
			if (fmt_stat == FMT_POINT || fmt_stat == FMT_SET_FT)
				goto fmt_error;
			
			float_type = FT_DBL;
			fmt_stat = FMT_SET_FT;
			
			break;
		case 'L':
			if (fmt_stat == FMT_POINT || fmt_stat == FMT_SET_FT)
				goto fmt_error;
			
			float_type = FT_LDBL;
			fmt_stat = FMT_SET_FT;
			break;
		case 'Q':
			if (fmt_stat == FMT_POINT || fmt_stat == FMT_SET_FT)
				goto fmt_error;
			
			float_type = FT_QUAD;
			fmt_stat = FMT_SET_FT;
			break;
		case 'a': case 'A':
		case 'e': case 'E':
		case 'f': case 'F':
		case 'g': case 'G':
			if (fmt_stat == FMT_POINT)  goto fmt_error;
			
			if (float_type == FT_QUAD)
			{
				if (RARRAY_LEN(arg) <= arg_offset)
					goto too_few_arguments;
				else
				{
					__float128 x = get_real(rb_ary_entry(arg, arg_offset));
					int n;
					char buf[BUF_SIZ];
					apptd = rb_usascii_str_new_cstr("%");
					notation = isupper(c) ? c + 0x20 : c;
					if (flags & FLAG_PLUS)   rb_str_concat(apptd, CHR2FIX('+'));
					if (flags & FLAG_MINUS)  rb_str_concat(apptd, CHR2FIX('-'));
					if (flags & FLAG_SHARP)  rb_str_concat(apptd, CHR2FIX('#'));
					if (flags & FLAG_ZERO)   rb_str_concat(apptd, CHR2FIX('0'));
					if (flags & FLAG_SPACE)  rb_str_concat(apptd, CHR2FIX(' '));
					if (width != 0)          rb_str_concat(apptd, CHR2FIX('*'));
					if (prec != 0)         { rb_str_concat(apptd, CHR2FIX('.'));
					                         rb_str_concat(apptd, CHR2FIX('*')); }
					                         rb_str_concat(apptd, CHR2FIX('Q'));
					                         rb_str_concat(apptd, CHR2FIX(notation));
					
					n = xsnprintf(buf, BUF_SIZ, StringValuePtr(apptd), width, prec, x);
					
					if ((size_t)n < sizeof(buf))
						rb_str_cat_cstr(retval, buf);
					else
					{
						n = xsnprintf(NULL, 0, StringValuePtr(apptd), width, prec, x);
						if (n > -1)
						{
							char *str = ruby_xmalloc(n + 1);
							if (str)
							{
								xsnprintf(str, n + 1, StringValuePtr(apptd), width, prec, x);
								rb_str_cat_cstr(retval, str);
							}
							ruby_xfree(str);
						}
					}
				}
			}
			goto return_value;
			break;
		default:
			goto fmt_error;
			break;
		}
	}
return_value:
	return retval;
fmt_error:
	rb_raise(rb_eArgError, "format error");
biggest_width_error:
	rb_raise(rb_eArgError, "biggest (or negative) width size");
biggest_prec_error:
	rb_raise(rb_eArgError, "biggest (or negative) precision size");
too_few_arguments:
	rb_raise(rb_eArgError, "too few arguments");
}


void
InitVM_Numerable(void)
{
	/* Numerical Type Conversions */
	rb_define_method(rb_cString, "to_f128", string_to_f128, 0);
	
	rb_define_method(rb_cNumeric, "to_f128", numeric_to_f128, 0);
	rb_define_method(rb_cNumeric, "to_c128", numeric_to_c128, 0);
	
	rb_undef_method(rb_cNumeric, "to_f128");
	rb_undef_method(rb_cNumeric, "to_c128");
	
	rb_define_method(rb_cInteger, "to_f128", integer_to_f128, 0);
	rb_define_method(rb_cInteger, "to_c128", integer_to_c128, 0);
	
	rb_define_method(rb_cRational, "to_f128", rational_to_f128, 0);
	rb_define_method(rb_cRational, "to_c128", rational_to_c128, 0);
	
	rb_define_method(rb_cFloat, "to_f128", float_to_f128, 0);
	rb_define_method(rb_cFloat, "to_c128", float_to_c128, 0);
	
	rb_define_method(rb_cComplex, "to_f128", nucomp_to_f128, 0);
	rb_define_method(rb_cComplex, "to_c128", nucomp_to_c128, 0);
	
	/* Global function */
	rb_define_global_function("Float128", f_Float128, -1);
//	rb_define_global_function("Complex128", f_Complex128, -1);
	rb_define_global_function("strtoflt128", f_strtoflt128, -1);
	rb_define_global_function("quadmath_sprintf", f_quadmath_sprintf, -1);

	/* Singleton Methods */
	rb_define_singleton_method(rb_cFloat128, "fma", float128_s_fma, -1);
	rb_define_singleton_method(rb_cFloat128, "sincos", float128_s_sincos, -1);
	rb_define_singleton_method(rb_cFloat128, "fmin", float128_s_fmin, -1);
	rb_define_singleton_method(rb_cFloat128, "fmax", float128_s_fmax, -1);
	rb_define_singleton_method(rb_cFloat128, "ldexp", float128_s_ldexp, 2);
	rb_define_singleton_method(rb_cFloat128, "scalb", float128_s_scalb, 2);
	rb_define_singleton_method(rb_cFloat128, "scalbn", float128_s_scalb, 2);
	rb_define_singleton_method(rb_cComplex128, "polar", complex128_s_polar, -1);
	rb_define_singleton_method(rb_cComplex128, "rect", complex128_s_rect, -1);
	rb_define_singleton_method(rb_cComplex128, "rectangular", complex128_s_rect, -1);
	
	/* Operators & Evals */
	rb_define_method(rb_cFloat128, "polar", float128_polar, 0);
	rb_define_method(rb_cFloat128, "abs", float128_abs, 0);
	rb_define_alias(rb_cFloat128, "magnitude", "abs");
	rb_define_method(rb_cFloat128, "abs2", float128_abs2, 0);
	rb_define_method(rb_cFloat128, "arg", float128_arg, 0);
	rb_define_alias(rb_cFloat128, "angle", "arg");
	rb_define_alias(rb_cFloat128, "phase", "arg");
	
	rb_define_method(rb_cFloat128, "+", float128_add, 1);
	rb_define_method(rb_cFloat128, "-", float128_sub, 1);
	rb_define_method(rb_cFloat128, "*", float128_mul, 1);
	rb_define_method(rb_cFloat128, "/", float128_div, 1);
	rb_define_method(rb_cFloat128, "%", float128_mod, 1);
	rb_define_method(rb_cFloat128, "modulo", float128_mod, 1);
	rb_define_method(rb_cFloat128, "**", float128_pow, 1);
	rb_define_method(rb_cFloat128, "<=>", float128_cmp, 1);
	rb_define_method(rb_cFloat128, "coerce", float128_coerce, -1);
	
	rb_define_method(rb_cComplex128, "polar", complex128_polar, 0);
	rb_define_method(rb_cComplex128, "abs", complex128_abs, 0);
	rb_define_alias(rb_cComplex128, "magnitude", "abs");
	rb_define_method(rb_cComplex128, "abs2", complex128_abs2, 0);
	rb_define_method(rb_cComplex128, "arg", complex128_arg, 0);
	rb_define_alias(rb_cComplex128, "angle", "arg");
	rb_define_alias(rb_cComplex128, "phase", "arg");
	rb_define_method(rb_cComplex128, "conj", complex128_conj, 0);
	
	rb_define_method(rb_cComplex128, "+", complex128_add, 1);
	rb_define_method(rb_cComplex128, "-", complex128_sub, 1);
	rb_define_method(rb_cComplex128, "*", complex128_mul, 1);
	rb_define_method(rb_cComplex128, "/", complex128_div, 1);
	rb_define_method(rb_cComplex128, "%", complex128_mod, 1);
	rb_define_method(rb_cComplex128, "modulo", complex128_mod, 1);
	rb_define_method(rb_cComplex128, "**", complex128_pow, 1);
	rb_define_method(rb_cComplex128, "<=>", complex128_cmp, 1);
	rb_define_method(rb_cComplex128, "coerce", complex128_coerce, -1);
	
	rb_undef_method(rb_cComplex128, "%");
}
