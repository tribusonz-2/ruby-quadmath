/*******************************************************************************
    quadmath.c -- module QuadMath
    
    Author: Hironobu Inatsuka
*******************************************************************************/
#include <ruby.h>
#include <quadmath.h>
#include "rb_quadmath.h"
#include "internal/rb_quadmath.h"

static inline bool
num_real_p(VALUE self)
{
	if (!rb_respond_to(self, rb_intern("real?")))
		rb_fatal("method `real?' is undefined");
	else
		return RTEST(rb_funcall(self, rb_intern("real?"), 0));
}

static inline __float128
fixnum_to_cf128(VALUE self)
{
	long x = FIX2LONG(self);
	
	return (__float128)x;
}

static inline __float128
bignum_to_cf128(VALUE self)
{
	VALUE inum = rb_big2str(self, 10);
	__float128 x = strtoflt128(StringValuePtr(inum), NULL);
	
	return x;
}

static inline __float128
integer_to_cf128(VALUE self)
{
	__float128 x = 0.0Q;
	switch (TYPE(self)) {
	case T_FIXNUM:
		x = fixnum_to_cf128(self);
		break;
	case T_BIGNUM:
		x = bignum_to_cf128(self);
		break;
	default:
		break;
	}
	return x;
}

static inline __float128
rational_to_cf128(VALUE self)
{
	VALUE numer = rb_rational_num(self),
	      denom = rb_rational_den(self);
	__float128 x = integer_to_cf128(numer) / integer_to_cf128(denom);
	
	return x;
}

static inline __float128
float_to_cf128(VALUE self)
{
	double x = NUM2DBL(self);
	
	return (__float128)x;
}



static inline __float128
primitive_real_to_cf128(VALUE self)
{
	__float128 x;
	switch (TYPE(self)) {
	case T_FIXNUM:
		x = fixnum_to_cf128(self);
		break;
	case T_BIGNUM:
		x = bignum_to_cf128(self);
		break;
	case T_RATIONAL:
		x = rational_to_cf128(self);
		break;
	case T_FLOAT:
		x = float_to_cf128(self);
		break;
	default:
		self = rb_funcall(self, rb_intern("to_f128"), 0);
		x = GetF128(self);
		break;
	}
	return x;
}

static inline __complex128
nucomp_to_cc128(VALUE self)
{
	VALUE real = rb_complex_real(self);
	VALUE imag = rb_complex_imag(self);
	__complex128 z = 0+0i;
	
	__real__ z = primitive_real_to_cf128(real);
	__imag__ z = primitive_real_to_cf128(imag);
	
	return z;
}

static inline VALUE
complex128_to_nucomp(VALUE self)
{
	__complex128 z = GetC128(self);
	return rb_Complex(
		rb_float128_cf128(crealq(z)),
		rb_float128_cf128(cimagq(z)));
}

static VALUE
quadmath_exp_realsolve(__float128 x)
{
	return rb_float128_cf128(expq(x));
}

static VALUE
quadmath_exp_nucompsolve(__complex128 z)
{
	return rb_complex128_cc128(cexpq(z));
}

/*
 *  call-seq:
 *    QuadMath.exp(x) -> Float128 | Complex | Complex128
 *  
 *  xの指数関数を返す．xが実数なら実数解，複素数なら複素数解として各々返却する．
 *  
 *  QuadMath.exp(1) # => 2.7182818284590452353602874713526
 *  QuadMath.exp(-2.1r) # => 0.122456428252981910218647376072626
 *  QuadMath.exp(1+0i) # => (2.7182818284590452353602874713526+0.0i)
 *  QuadMath.exp('-10.4'.to_f128) # => 0.0000304324830084036365062222042316828
 *  QuadMath.exp(Complex128.rect(1.1r, 1))
 *  # => (1.6231578299489788500278329108526+2.5279185426966278702444317537622i)
 */
static VALUE
quadmath_exp(VALUE unused_obj, VALUE x)
{
	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
		return quadmath_exp_realsolve(fixnum_to_cf128(x));
		break;
	case NUM_BIGNUM:
		return quadmath_exp_realsolve(bignum_to_cf128(x));
		break;
	case NUM_RATIONAL:
		return quadmath_exp_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_exp_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		VALUE nucomp = quadmath_exp_nucompsolve(nucomp_to_cc128(x));
		return complex128_to_nucomp(nucomp);
		break;
	case NUM_FLOAT128:
		return quadmath_exp_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_exp_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_exp_realsolve(rb_float128_value(x));
		else
			return quadmath_exp_nucompsolve(rb_complex128_value(x));
		break;
	}
}

static inline VALUE
quadmath_exp2_realsolve(__float128 x)
{
	return rb_float128_cf128(exp2q(x));
}

static inline VALUE
quadmath_exp2_nucompsolve(__complex128 z)
{
	return rb_complex128_cc128(cpowq(2, z));
}

/*
 *  call-seq:
 *    QuadMath.exp2(x) -> Integer | Rational | Float128 | Complex | Complex128
 *  
 *  2のx乗を返す．xが整数なら正では整数解，負では有理数解，実数なら実数解，複素数なら複素数解として各々返却する．
 *  これは一般に`2 ** x`と計算するのとさして変わらない．唯一の違いは実数の精度が二倍ではなく四倍であることである．
 *  
 *  QuadMath.exp2(-1) # => (1/2)
 *  QuadMath.exp2(5) # => 32
 *  QuadMath.exp2(11/7r) # => 2.9719885782738968495997065734291
 *  QuadMath.exp2(Complex128::I) # => (0.76923890136397212657832999366127+0.638961276313634801150032911464701i)
 */
static VALUE
quadmath_exp2(VALUE unused_obj, VALUE x)
{
	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
	case NUM_BIGNUM:
		return rb_num_coerce_bin(INT2NUM(2), x, rb_intern("**"));
		break;
	case NUM_RATIONAL:
		return quadmath_exp2_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_exp2_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		return float128_nucomp_pow(rb_float128_cf128(2), x);
		break;
	case NUM_FLOAT128:
		return quadmath_exp2_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_exp2_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_exp2_realsolve(GetF128(x));
		else
			return quadmath_exp2_nucompsolve(GetC128(x));
		break;
	}
}

static inline VALUE
quadmath_expm1_realsolve(__float128 x)
{
	return rb_float128_cf128(expm1q(x));
}

static inline VALUE
quadmath_expm1_nucompsolve(__complex128 z)
{
	if (cimagq(z) == 0)
	{
		__float128 real = crealq(z);
		return rb_complex128_cc128((__complex128)expm1q(real));
	}
	else
	{
		__complex128 w = 2 * cexpq(z / 2) * csinhq(z / 2);
		return rb_complex128_cc128(w);
	}
}

/*
 *  call-seq:
 *    QuadMath.expm1(x) -> Float128 | Complex128
 *  
 *  xの指数関数より1を引いた値を返す．
 *  この関数はゼロに近傍な値を数式通りにexp(x)-1と計算するよりも高精度化する．
 *  xが実数なら実数解，複素数なら複素数解として各々返却する．
 *  
 *    QuadMath.expm1('-1e-33'.to_f128) # => -1.0e-33
 *    QuadMath.exp('-1e-33'.to_f128)-1 # => -9.62964972193617926527988971292464e-34
 *    QuadMath.expm1(Complex128::I) # => (-0.459697694131860282599063392557023+0.841470984807896506652502321630299i)
 */
static VALUE
quadmath_expm1(VALUE unused_obj, VALUE x)
{
	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
		return quadmath_expm1_realsolve(fixnum_to_cf128(x));
		break;
	case NUM_BIGNUM:
		return quadmath_expm1_realsolve(bignum_to_cf128(x));
		break;
	case NUM_RATIONAL:
		return quadmath_expm1_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_expm1_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		VALUE nucomp = quadmath_expm1_nucompsolve(nucomp_to_cc128(x));
		return complex128_to_nucomp(nucomp);
		break;
	case NUM_FLOAT128:
		return quadmath_expm1_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_expm1_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_expm1_realsolve(GetF128(x));
		else
			return quadmath_expm1_nucompsolve(GetC128(x));
		break;
	}
}

static inline VALUE
quadmath_log_realsolve(__float128 x)
{
	if (isnanq(x))
		return rb_float128_cf128(x);
	else if (!signbitq(x))
		return rb_float128_cf128(logq(x));
	else
	{
		__complex128 z;
		__real__ z = logq(fabsq(x));
		__imag__ z = M_PIq;
		return rb_complex128_cc128(z);
	}
}

static inline VALUE
quadmath_log_nucompsolve(__complex128 z)
{
	return rb_complex128_cc128(clogq(z));
}

/*
 *  call-seq:
 *    QuadMath.log(x) -> Float128 | Complex128
 *  
 *  xの自然対数を返す．
 *  xが実数なら正ならば実数解，負なら複素数解，複素数なら複素数解として値を各々返す．
 *  
 *  QuadMath.log(2) # => 0.693147180559945309417232121458176
 *  QuadMath.log(-2) # => (0.693147180559945309417232121458176+3.1415926535897932384626433832795i)
 *  QuadMath.log(Complex128::I) # => (0.0+1.5707963267948966192313216916397i)
 */
static VALUE
quadmath_log(VALUE unused_obj, VALUE x)
{
	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
		return quadmath_log_realsolve(fixnum_to_cf128(x));
		break;
	case NUM_BIGNUM:
		return quadmath_log_realsolve(bignum_to_cf128(x));
		break;
	case NUM_RATIONAL:
		return quadmath_log_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_log_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		VALUE nucomp = quadmath_log_nucompsolve(nucomp_to_cc128(x));
		return complex128_to_nucomp(nucomp);
		break;
	case NUM_FLOAT128:
		return quadmath_log_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_log_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_log_realsolve(GetF128(x));
		else
			return quadmath_log_nucompsolve(GetC128(x));
		break;
	}
}

static inline VALUE
quadmath_log2_realsolve(__float128 x)
{
	if (isnanq(x))
		return rb_float128_cf128(x);
	else if (!signbitq(x))
		return rb_float128_cf128(log2q(x));
	else
	{
		__complex128 z;
		__real__ z = log2q(fabsq(x));
		__imag__ z = M_PIq / M_LN2q;
		return rb_complex128_cc128(z);
	}
}

static inline VALUE
quadmath_log2_nucompsolve(__complex128 z)
{
	return rb_complex128_cc128(clogq(z) / M_LN2q);
}

/*
 *  call-seq:
 *    QuadMath.log2(x) -> Float128 | Complex128
 *  
 *  xの2を底とする対数を返す．(Binary Logarithm)
 *  xが実数なら正ならば実数解，負なら複素数解，複素数なら複素数解として値を各々返す．
 *  
 *    Array.new(5){|i|i} # => [0, 1, 2, 3, 4]
 *    ary = Array.new(5){|i| 2**i} # => [1, 2, 4, 8, 16]
 *    ary.map {|x| QuadMath.log2(x)} # => [0.0, 1.0, 2.0, 3.0, 4.0]
 *    
 *    -1.step(-5, -1).to_a # => [-1, -2, -3, -4, -5]
 *    ary = -1.step(-5, -1).map{|i| 2**i} # => [(1/2), (1/4), (1/8), (1/16), (1/32)]
 *    ary.map{|x| QuadMath.log2(x)} # => [-1.0, -2.0, -3.0, -4.0, -5.0]
 *    
 *    QuadMath.log2(1+1i) # => (0.5+1.1330900354567984524069207364291i)
 */
static VALUE
quadmath_log2(VALUE unused_obj, VALUE x)
{
	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
		return quadmath_log2_realsolve(fixnum_to_cf128(x));
		break;
	case NUM_BIGNUM:
		return quadmath_log2_realsolve(bignum_to_cf128(x));
		break;
	case NUM_RATIONAL:
		return quadmath_log2_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_log2_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		VALUE nucomp = quadmath_log2_nucompsolve(nucomp_to_cc128(x));
		return complex128_to_nucomp(nucomp);
		break;
	case NUM_FLOAT128:
		return quadmath_log2_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_log2_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_log2_realsolve(GetF128(x));
		else
			return quadmath_log2_nucompsolve(GetC128(x));
		break;
	}
}

static inline VALUE
quadmath_log10_realsolve(__float128 x)
{
	if (isnanq(x))
		return rb_float128_cf128(x);
	else if (!signbitq(x))
		return rb_float128_cf128(log10q(x));
	else
	{
		__complex128 z;
		__real__ z = log10q(fabsq(x));
		__imag__ z = M_PIq / M_LN10q;
		return rb_complex128_cc128(z);
	}
}

static inline VALUE
quadmath_log10_nucompsolve(__complex128 z)
{
	return rb_complex128_cc128(clog10q(z));
}

/*
 *  call-seq:
 *    QuadMath.log10(x) -> Float128 | Complex128
 *  
 *  xの常用対数を返す．
 *  xが実数なら正ならば実数解，負なら複素数解，複素数なら複素数解として値を各々返す．
 *
 *    Array.new(5){|i|i} # => [0, 1, 2, 3, 4]
 *    ary = Array.new(5){|i| 10**i} # => [1, 10, 100, 1000, 10000]
 *    ary.map {|x| QuadMath.log10(x)} # => [0.0, 1.0, 2.0, 3.0, 4.0]
 *    
 *    -1.step(-5, -1).to_a # => [-1, -2, -3, -4, -5]
 *    ary = -1.step(-5, -1).map{|i| 10**i} # => [(1/10), (1/100), (1/1000), (1/10000), (1/100000)]
 *    ary.map{|x| QuadMath.log10(x)} # => [-1.0, -2.0, -3.0, -4.0, -5.0]
 *    
 *    QuadMath.log10(1+1i) # => (0.150514997831990597606869447362246+0.341094088460460336871445906357839i)

 */
static VALUE
quadmath_log10(VALUE unused_obj, VALUE x)
{
	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
		return quadmath_log10_realsolve(fixnum_to_cf128(x));
		break;
	case NUM_BIGNUM:
		return quadmath_log10_realsolve(bignum_to_cf128(x));
		break;
	case NUM_RATIONAL:
		return quadmath_log10_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_log10_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		VALUE nucomp = quadmath_log10_nucompsolve(nucomp_to_cc128(x));
		return complex128_to_nucomp(nucomp);
		break;
	case NUM_FLOAT128:
		return quadmath_log10_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_log10_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_log10_realsolve(GetF128(x));
		else
			return quadmath_log10_nucompsolve(GetC128(x));
		break;
	}
}

static inline VALUE
log1p_realsolve_neg(__float128 x)
{
	__complex128 z = 2 * catanhq(x / (2 + x));
	if (cimagq(z) != M_PIq)
		__imag__ z = M_PIq;
	return rb_complex128_cc128(z);
}

static inline VALUE
quadmath_log1p_realsolve(__float128 x)
{
	if (isnanq(x))
		return nanq("");
	if (x >= -1)
		return rb_float128_cf128(log1pq(x));
	else
		return log1p_realsolve_neg(x);
}

static inline VALUE
quadmath_log1p_nucompsolve(__complex128 z)
{
	if (cimagq(z) == 0)
	{
		__float128 real = crealq(z);
		if (real >= -1)
			return rb_complex128_cc128((__complex128)log1pq(real));
		else
			return log1p_realsolve_neg(real);
	}
	else
		return rb_complex128_cc128(clogq(1+z));
}

/*
 *  call-seq:
 *    QuadMath.log1p(x) -> Float128 | Complex128
 *  
 *  xに1を加算した自然対数を返す．
 *  この関数はxがゼロに近傍な値のとき数式通りにlog(1+x)と計算するよりも高精度化する．
 *  xが実数なら実数解，複素数なら複素数解として各々返却する．
 *  
 *    x = '-1e-16'.to_f128 # => -1.0e-16
 *    QuadMath.log1p(x) # => -1.00000000000000005e-16
 *    QuadMath.log(1+x) # => -1.0000000000000000502830161122942e-16

 */
static VALUE
quadmath_log1p(VALUE unused_obj, VALUE x)
{
	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
		return quadmath_log1p_realsolve(fixnum_to_cf128(x));
		break;
	case NUM_BIGNUM:
		return quadmath_log1p_realsolve(bignum_to_cf128(x));
		break;
	case NUM_RATIONAL:
		return quadmath_log1p_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_log1p_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		VALUE nucomp = quadmath_log1p_nucompsolve(nucomp_to_cc128(x));
		return complex128_to_nucomp(nucomp);
		break;
	case NUM_FLOAT128:
		return quadmath_log1p_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_log1p_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_log1p_realsolve(GetF128(x));
		else
			return quadmath_log1p_nucompsolve(GetC128(x));
		break;
	}

}

static inline VALUE
quadmath_sqrt_realsolve(__float128 x)
{
	if (!signbitq(x))
		return rb_float128_cf128(sqrtq(x));
	else
	{
		__complex128 z;
		__real__ z = 0.q;
		__imag__ z = sqrtq(fabsq(x));
		return rb_complex128_cc128(z);
	}
}

static inline VALUE
quadmath_sqrt_nucompsolve(__complex128 z)
{
	return rb_complex128_cc128(csqrtq(z));
}

/*
 *  call-seq:
 *    QuadMath.sqrt(x) -> Float128 | Complex128
 *  
 *  xの平方根を返す．
 *  xが実数であり正の場合は実数解，負の場合は虚数解，複素数の場合は複素数解として各々返却する．
 *  
 *    QuadMath.sqrt(4) # => 2.0
 *    QuadMath.sqrt(-1) == Complex128::I # => true
 *    QuadMath.sqrt(1+1i) # => (1.0986841134678099660398011952406+0.455089860562227341304357757822468i)
 */
static VALUE
quadmath_sqrt(VALUE unused_obj, VALUE x)
{

	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
		return quadmath_sqrt_realsolve(fixnum_to_cf128(x));
		break;
	case NUM_BIGNUM:
		return quadmath_sqrt_realsolve(bignum_to_cf128(x));
		break;
	case NUM_RATIONAL:
		return quadmath_sqrt_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_sqrt_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		VALUE nucomp = quadmath_sqrt_nucompsolve(nucomp_to_cc128(x));
		return complex128_to_nucomp(nucomp);
		break;
	case NUM_FLOAT128:
		return quadmath_sqrt_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_sqrt_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_sqrt_realsolve(GetF128(x));
		else
			return quadmath_sqrt_nucompsolve(GetC128(x));
		break;
	}
}

static inline VALUE
quadmath_sqrt3_realsolve(__float128 x)
{
	if (!signbitq(x))
		return rb_float128_cf128(cbrtq(x));
	else
		return rb_complex128_cc128(cpowq((__complex128)x, 1.q/3.q));
}

static inline VALUE
quadmath_sqrt3_nucompsolve(__complex128 z)
{
	return rb_complex128_cc128(cpowq(z, (__complex128)(1.q/3.q)));
}

/*
 *  call-seq:
 *    QuadMath.sqrt3(x) -> Float128 | Complex128
 *  
 *  xの立方根を返す．
 *  xが実数であり正の場合は実数解，負の場合は複素数解，複素数の場合は複素数解として各々返却する．
 *  
 *    QuadMath.sqrt3(3*3*3) # => 3.0
 *    QuadMath.sqrt3(-1) # => (0.5+0.866025403784438646763723170752936i)
 *    QuadMath.sqrt3(1+1i) # => (1.0842150814913511818796660082610+0.290514555507251444503813188624929i)
 */
static VALUE
quadmath_sqrt3(VALUE unused_obj, VALUE x)
{
	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
		return quadmath_sqrt3_realsolve(fixnum_to_cf128(x));
		break;
	case NUM_BIGNUM:
		return quadmath_sqrt3_realsolve(bignum_to_cf128(x));
		break;
	case NUM_RATIONAL:
		return quadmath_sqrt3_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_sqrt3_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		VALUE nucomp = quadmath_sqrt3_nucompsolve(nucomp_to_cc128(x));
		return complex128_to_nucomp(nucomp);
		break;
	case NUM_FLOAT128:
		return quadmath_sqrt3_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_sqrt3_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_sqrt3_realsolve(GetF128(x));
		else
			return quadmath_sqrt3_nucompsolve(GetC128(x));
		break;
	}
}

static inline VALUE
quadmath_cbrt_realsolve(__float128 x)
{
	return rb_float128_cf128(cbrtq(x));
}

static inline VALUE
quadmath_cbrt_nucompsolve(__complex128 z)
{
	if (cimagq(z) != 0)
		return Qnil;
	
	return quadmath_cbrt_realsolve(crealq(z));
}

/*
 *  call-seq:
 *    QuadMath.cbrt(x) -> Float128 | nil
 *  
 *  xの実数としての三乗根を返す．
 *  実計算の'x ** 1/3'における複素根はcbrt関数では扱われない．そのため虚数が現れるならnilとして扱われる．
 * 
 *    QuadMath.cbrt(3*3*3) # => 3.0
 *    QuadMath.cbrt(-1) # => -1.0
 *    QuadMath.cbrt(1+1i) # => nil
 */
static VALUE
quadmath_cbrt(VALUE unused_obj, VALUE x)
{
	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
		return quadmath_cbrt_realsolve(fixnum_to_cf128(x));
		break;
	case NUM_BIGNUM:
		return quadmath_cbrt_realsolve(bignum_to_cf128(x));
		break;
	case NUM_RATIONAL:
		return quadmath_cbrt_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_cbrt_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		return quadmath_cbrt_nucompsolve(nucomp_to_cc128(x));
		break;
	case NUM_FLOAT128:
		return quadmath_cbrt_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_cbrt_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_cbrt_realsolve(GetF128(x));
		else
			return quadmath_cbrt_nucompsolve(GetC128(x));
		break;
	}
}

static inline VALUE
quadmath_sin_realsolve(__float128 x)
{
	return rb_float128_cf128(sinq(x));
}

static inline VALUE
quadmath_sin_nucompsolve(__complex128 z)
{
	return rb_complex128_cc128(csinq(z));
}

/*
 *  call-seq:
 *    QuadMath.sin(x) -> Float128 | Complex128
 *  
 *  xの正弦を返す．
 *  xが実数なら実数解，複素数なら複素数解として各々返却する．
 *  
 *    QuadMath.sin(0.5r * QuadMath::PI) / 0.5r # => 2.0
 *    QuadMath.sin(1+1i) # => (1.2984575814159772948260423658078+0.634963914784736108255082202991509i)
 */
static VALUE
quadmath_sin(VALUE unused_obj, VALUE x)
{
	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
		return quadmath_sin_realsolve(fixnum_to_cf128(x));
		break;
	case NUM_BIGNUM:
		return quadmath_sin_realsolve(bignum_to_cf128(x));
		break;
	case NUM_RATIONAL:
		return quadmath_sin_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_sin_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		VALUE nucomp = quadmath_sin_nucompsolve(nucomp_to_cc128(x));
		return complex128_to_nucomp(nucomp);
		break;
	case NUM_FLOAT128:
		return quadmath_sin_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_sin_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_sin_realsolve(GetF128(x));
		else
			return quadmath_sin_nucompsolve(GetC128(x));
		break;
	}
}

static inline VALUE
quadmath_cos_realsolve(__float128 x)
{
	return rb_float128_cf128(cosq(x));
}

static inline VALUE
quadmath_cos_nucompsolve(__complex128 z)
{
	return rb_complex128_cc128(ccosq(z));
}

/*
 *  call-seq:
 *    QuadMath.cos(x) -> Float128 | Complex128
 *  
 *  xの余弦を返す．
 *  xが実数なら実数解，複素数なら複素数解として各々返却する．
 *  
 *    QuadMath.cos(0.5r * QuadMath::PI) / 0.5r # => 8.67181013012378102479704402604335e-35
 *    QuadMath.cos(1+1i) # => (0.833730025131149048883885394335094-0.988897705762865096382129540892686i)
 */
static VALUE
quadmath_cos(VALUE unused_obj, VALUE x)
{
	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
		return quadmath_cos_realsolve(fixnum_to_cf128(x));
		break;
	case NUM_BIGNUM:
		return quadmath_cos_realsolve(bignum_to_cf128(x));
		break;
	case NUM_RATIONAL:
		return quadmath_cos_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_cos_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		VALUE nucomp = quadmath_cos_nucompsolve(nucomp_to_cc128(x));
		return complex128_to_nucomp(nucomp);
		break;
	case NUM_FLOAT128:
		return quadmath_cos_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_cos_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_cos_realsolve(GetF128(x));
		else
			return quadmath_cos_nucompsolve(GetC128(x));
		break;
	}
}

static inline __float128
tan_realsolve(__float128 x)
{
	__float128 y;
	switch (fpclassify(x)) {
	case FP_NAN:
	case FP_INFINITE:
		y = nanq("");
		break;
	case FP_ZERO:
		y = 0.q;
		break;
	default:
		y = fmodq(fabsq(x), M_PIq * 2);
		if (y == M_PI_2q)
			y = copysignq(HUGE_VALQ, x);
		else if (y == M_PIq)
			y = 0.q;
		else
			y = tanq(x);
		break;
	}
	
	return y;
}

static inline VALUE
quadmath_tan_realsolve(__float128 x)
{
	return rb_float128_cf128(tan_realsolve(x));
}

static inline VALUE
quadmath_tan_nucompsolve(__complex128 z)
{
	if (cimagq(z) == 0)
		__real__ z = tan_realsolve(crealq(z));
	else
		z = ctanq(z);
	return rb_complex128_cc128(z);
}

/*
 *  call-seq:
 *    QuadMath.tan(x) -> Float128 | Complex128
 *  
 *  xの正接を返す．
 *  xが実数なら実数解，複素数なら複素数解として各々返却する．
 *  この関数はxは周期性$2\pi$として虚数の現れていない数なら極と解釈し，特殊値を返す．
 *  特殊値の送出は$-2\pi \le x \le 2\pi$の範囲内では正確に戻すよう試みるが，範囲外で正確かどうかは機種依存である．
 *  
 *    QuadMath.tan(QuadMath.atan(Float128::INFINITY)) # => Infinity
 *    QuadMath.tan(3/4r * QuadMath::PI) # => -1.0
 *    QuadMath.tan(1+1i) # => (0.271752585319511716528843722498589+1.0839233273386945434757520612119i)

 */
static VALUE
quadmath_tan(VALUE unused_obj, VALUE x)
{
	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
		return quadmath_tan_realsolve(fixnum_to_cf128(x));
		break;
	case NUM_BIGNUM:
		return quadmath_tan_realsolve(bignum_to_cf128(x));
		break;
	case NUM_RATIONAL:
		return quadmath_tan_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_tan_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		VALUE nucomp = quadmath_tan_nucompsolve(nucomp_to_cc128(x));
		return complex128_to_nucomp(nucomp);
		break;
	case NUM_FLOAT128:
		return quadmath_tan_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_tan_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_tan_realsolve(GetF128(x));
		else
			return quadmath_tan_nucompsolve(GetC128(x));
		break;
	}
}

static inline VALUE
quadmath_asin_realsolve(__float128 x)
{
	if (x >= -1 && x <= 1)
		return rb_float128_cf128(asinq(x));
	else
		return rb_complex128_cc128(casinq((__complex128)x));
}

static inline VALUE
quadmath_asin_nucompsolve(__complex128 z)
{
	return rb_complex128_cc128(casinq(z));
}

/*
 *  call-seq:
 *    QuadMath.asin(x) -> Float128 | Complex128
 *  
 *  xの逆正弦を返す．
 *  xが(-1<=x<=1)の範囲なら実数解，ほかは複素数解として各々返却する．
 *  
 *    QuadMath.asin(-1/4r) # => -0.252680255142078653485657436993711
 *    QuadMath.asin(5/4r) # => (1.5707963267948966192313216916397+0.693147180559945309417232121458176i)
 *    QuadMath.asin(1+1i) # => (0.666239432492515255104004895977792+1.0612750619050356520330189162135i)

 */
static VALUE
quadmath_asin(VALUE unused_obj, VALUE x)
{
	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
		return quadmath_asin_realsolve(fixnum_to_cf128(x));
		break;
	case NUM_BIGNUM:
		return quadmath_asin_realsolve(bignum_to_cf128(x));
		break;
	case NUM_RATIONAL:
		return quadmath_asin_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_asin_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		VALUE nucomp = quadmath_asin_nucompsolve(nucomp_to_cc128(x));
		return complex128_to_nucomp(nucomp);
		break;
	case NUM_FLOAT128:
		return quadmath_asin_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_asin_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_asin_realsolve(GetF128(x));
		else
			return quadmath_asin_nucompsolve(GetC128(x));
		break;
	}
}

static inline VALUE
quadmath_acos_realsolve(__float128 x)
{
	if (x >= -1 && x <= 1)
		return rb_float128_cf128(acosq(x));
	else
		return rb_complex128_cc128(cacosq((__complex128)x));
}

static inline VALUE
quadmath_acos_nucompsolve(__complex128 z)
{
	return rb_complex128_cc128(cacosq(z));
}

/*
 *  call-seq:
 *    QuadMath.acos(x) -> Float128 | Complex128
 *  
 *  xの逆余弦を返す．
 *  xが(-1<=x<=1)の範囲なら実数解，ほかは複素数解として各々返却する．
 *  
 *    QuadMath.acos(-1/4r) # => 1.8234765819369752727169791286334
 *    QuadMath.acos(5/4r) # => (0.0-0.693147180559945309417232121458176i)
 *    QuadMath.acos(1+1i) # => (0.904556894302381364127316795661958-1.0612750619050356520330189162135i)
 */
static VALUE
quadmath_acos(VALUE unused_obj, VALUE x)
{
	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
		return quadmath_acos_realsolve(fixnum_to_cf128(x));
		break;
	case NUM_BIGNUM:
		return quadmath_acos_realsolve(bignum_to_cf128(x));
		break;
	case NUM_RATIONAL:
		return quadmath_acos_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_acos_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		VALUE nucomp = quadmath_acos_nucompsolve(nucomp_to_cc128(x));
		return complex128_to_nucomp(nucomp);
		break;
	case NUM_FLOAT128:
		return quadmath_acos_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_acos_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_acos_realsolve(GetF128(x));
		else
			return quadmath_acos_nucompsolve(GetC128(x));
		break;
	}
}

static inline VALUE
quadmath_atan_realsolve(__float128 x)
{
	return rb_float128_cf128(atanq(x));
}

static inline VALUE
quadmath_atan_nucompsolve(__complex128 z)
{
	return rb_complex128_cc128(catanq(z));
}

/*
 *  call-seq:
 *    QuadMath.atan(x) -> Float128 | Complex128
 *  
 *  xの逆正接を返す．
 *  xが実数なら実数解，複素数なら複素数解として各々返却する．
 *  
 *    QuadMath.atan(-Float128::INFINITY) == (-QuadMath::PI/2) # => true
 *    QuadMath.atan(+Float128::INFINITY) == (+QuadMath::PI/2) # => true
 *    QuadMath.atan(1+1i) # => (1.0172219678978513677227889615504+0.402359478108525093650189833306547i)
 */
static VALUE
quadmath_atan(VALUE unused_obj, VALUE x)
{
	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
		return quadmath_atan_realsolve(fixnum_to_cf128(x));
		break;
	case NUM_BIGNUM:
		return quadmath_atan_realsolve(bignum_to_cf128(x));
		break;
	case NUM_RATIONAL:
		return quadmath_atan_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_atan_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		VALUE nucomp = quadmath_atan_nucompsolve(nucomp_to_cc128(x));
		return complex128_to_nucomp(nucomp);
		break;
	case NUM_FLOAT128:
		return quadmath_atan_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_atan_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_atan_realsolve(GetF128(x));
		else
			return quadmath_atan_nucompsolve(GetC128(x));
		break;
	}
}


static inline VALUE
quadmath_quadrant_realsolve(__float128 x, __float128 y)
{
	return rb_float128_cf128(atan2q(y, x));
}

static inline __complex128
catan2q(__complex128 w, __complex128 z)
{
	if (cimagq(z) == 0 && cimagq(w) == 0)
	{
		return (__complex128)atan2q(crealq(w), crealq(z));
	}
	else
	{
		__complex128 c1 = 0-1i;
		__complex128 c2 = z + 0+1i * w;
		__complex128 c3 = csqrtq(z * z + w * w);
		return c1 * clogq(c2 / c3);
	}
}

static inline VALUE
quadmath_quadrant_nucompsolve(__complex128 z, __complex128 w)
{
	return rb_complex128_cc128(catan2q(w, z));
}

static VALUE
quadrant_inline(VALUE xsh, VALUE ysh)
{
	VALUE nucomp;
	static __float128 x, y;
	static __complex128 z, w;
	bool x_nucomp_p = false, y_nucomp_p = false, using_rb_complex = false;
	
	switch (convertion_num_types(xsh)) {
	case NUM_FIXNUM:
		x = fixnum_to_cf128(xsh);
		break;
	case NUM_BIGNUM:
		x = bignum_to_cf128(xsh);
		break;
	case NUM_RATIONAL:
		x = rational_to_cf128(xsh);
		break;
	case NUM_FLOAT:
		x = float_to_cf128(xsh);
		break;
	case NUM_COMPLEX:
		z = nucomp_to_cc128(xsh);
		x_nucomp_p = true;
		using_rb_complex = true;
		break;
	case NUM_FLOAT128:
		x = GetF128(xsh);
		break;
	case NUM_COMPLEX128:
		z = GetC128(xsh);
		x_nucomp_p = true;
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(xsh))
			x = GetF128(xsh);
		else
		{
			z = GetC128(xsh);
			x_nucomp_p = true;
		}
		break;
	}

	switch (convertion_num_types(ysh)) {
	case NUM_FIXNUM:
		y = fixnum_to_cf128(ysh);
		break;
	case NUM_BIGNUM:
		y = bignum_to_cf128(ysh);
		break;
	case NUM_RATIONAL:
		y = rational_to_cf128(ysh);
		break;
	case NUM_FLOAT:
		y = float_to_cf128(ysh);
		break;
	case NUM_COMPLEX:
		w = nucomp_to_cc128(ysh);
		y_nucomp_p = true;
		using_rb_complex = true;
		break;
	case NUM_FLOAT128:
		y = GetF128(ysh);
		break;
	case NUM_COMPLEX128:
		w = GetC128(ysh);
		y_nucomp_p = true;
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(ysh))
			y = GetF128(ysh);
		else
		{
			w = GetC128(ysh);
			y_nucomp_p = true;
		}
		break;
	}
	
	if (x_nucomp_p && y_nucomp_p)
		nucomp = quadmath_quadrant_nucompsolve(z, w);
	else if (x_nucomp_p && !y_nucomp_p)
		nucomp = quadmath_quadrant_nucompsolve(z, y);
	else if (!x_nucomp_p && y_nucomp_p)
		nucomp = quadmath_quadrant_nucompsolve(x, w);
	else
		return quadmath_quadrant_realsolve(x, y);
	if (using_rb_complex)
		return complex128_to_nucomp(nucomp);
	else
		return nucomp;
}

/*
 *  call-seq:
 *    QuadMath.atan2(y, x) -> Float128 | Complex128
 *  
 *  xとyが象限のどのあたりにあるかを考慮し，atan(y/x)の答を返す．
 *  2変数に虚数が現れているならば `-i log( (x+iy) sqrt(x^2 + y^2))`を計算する．
 *  
 *    x = 2; y = 1.2r
 *    QuadMath.atan2(y, x) # => 0.5404195002705841554435783646086
 *    x = -2.1r; y = 1.2r+1i
 *    QuadMath.atan2(y, x) # => (2.5425012953584908680165071231190-0.356967696991407745539638230102346i)
 */
static VALUE
quadmath_atan2(VALUE unused_obj, VALUE y, VALUE x)
{
	return quadrant_inline(x, y);
}

/*
 *  call-seq:
 *    QuadMath.quadrant(x, y) -> Float128 | Complex128
 *  
 *  xとyが象限のどのあたりにあるかを考慮し，atan(y/x)の答を返す．
 *  2変数に虚数が現れているならば `-i log( (x+iy) sqrt(x^2 + y^2))`を計算する．
 *  この関数はatan2のaliasであるものの，引数の順番を考慮している．
 *  
 *    x = 2; y = 1.2r
 *    QuadMath.quadrant(x, y) # => 0.5404195002705841554435783646086
 *    x = -2.1r; y = 1.2r+1i
 *    QuadMath.quadrant(x, y) # => (2.5425012953584908680165071231190-0.356967696991407745539638230102346i)
 */
static VALUE
quadmath_quadrant(VALUE unused_obj, VALUE x, VALUE y)
{
	return quadrant_inline(x, y);
}

static inline VALUE
quadmath_sinh_realsolve(__float128 x)
{
	return rb_float128_cf128(sinhq(x));
}

static inline VALUE
quadmath_sinh_nucompsolve(__complex128 x)
{
	return rb_complex128_cc128(csinhq(x));
}

/*
 *  call-seq:
 *    QuadMath.sinh(x) -> Float128 | Complex128
 *  
 *  xの双曲線正弦を返す．
 *  xが実数なら実数解，複素数なら複素数解として各々返却する．
 *  
 *    QuadMath.sinh(1.2r) # => 1.5094613554121726964428949112592
 *    QuadMath.sinh(1+1i) # => (0.634963914784736108255082202991509+1.2984575814159772948260423658078i)
 */
static VALUE
quadmath_sinh(VALUE unused_obj, VALUE x)
{
	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
		return quadmath_sinh_realsolve(fixnum_to_cf128(x));
		break;
	case NUM_BIGNUM:
		return quadmath_sinh_realsolve(bignum_to_cf128(x));
		break;
	case NUM_RATIONAL:
		return quadmath_sinh_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_sinh_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		VALUE nucomp = quadmath_sinh_nucompsolve(nucomp_to_cc128(x));
		return complex128_to_nucomp(nucomp);
		break;
	case NUM_FLOAT128:
		return quadmath_sinh_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_sinh_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_sinh_realsolve(GetF128(x));
		else
			return quadmath_sinh_nucompsolve(GetC128(x));
		break;
	}
}

static inline VALUE
quadmath_cosh_realsolve(__float128 x)
{
	return rb_float128_cf128(coshq(x));
}

static inline VALUE
quadmath_cosh_nucompsolve(__complex128 x)
{
	return rb_complex128_cc128(ccoshq(x));
}

/*
 *  call-seq:
 *    QuadMath.cosh(x) -> Float128 | Complex128
 *  
 *  xの双曲線余弦を返す．
 *  xが実数なら実数解，複素数なら複素数解として各々返却する．
 *  
 *    QuadMath.cosh(1.2r) # => 1.8106555673243747930878725183424
 *    QuadMath.cosh(1+1i) # => (0.833730025131149048883885394335094+0.988897705762865096382129540892686i)
 */
static VALUE
quadmath_cosh(VALUE unused_obj, VALUE x)
{
	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
		return quadmath_cosh_realsolve(fixnum_to_cf128(x));
		break;
	case NUM_BIGNUM:
		return quadmath_cosh_realsolve(bignum_to_cf128(x));
		break;
	case NUM_RATIONAL:
		return quadmath_cosh_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_cosh_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		VALUE nucomp = quadmath_cosh_nucompsolve(nucomp_to_cc128(x));
		return complex128_to_nucomp(nucomp);
		break;
	case NUM_FLOAT128:
		return quadmath_cosh_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_cosh_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_cosh_realsolve(GetF128(x));
		else
			return quadmath_cosh_nucompsolve(GetC128(x));
		break;
	}
}

static inline VALUE
quadmath_tanh_realsolve(__float128 x)
{
	return rb_float128_cf128(tanhq(x));
}

static inline VALUE
quadmath_tanh_nucompsolve(__complex128 x)
{
	return rb_complex128_cc128(ctanhq(x));
}

/*
 *  call-seq:
 *    QuadMath.tanh(x) -> Float128 | Complex128
 *  
 *  xの双曲線正接を返す．
 *  xが実数なら実数解，複素数なら複素数解として各々返却する．
 *  
 *    QuadMath.tanh(+Float128::INFINITY) == +1 # => true
 *    QuadMath.tanh(-Float128::INFINITY) == -1 # => true
 *    QuadMath.tanh(1+1i) # => (1.0839233273386945434757520612119+0.271752585319511716528843722498589i)
 */
static VALUE
quadmath_tanh(VALUE unused_obj, VALUE x)
{
	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
		return quadmath_tanh_realsolve(fixnum_to_cf128(x));
		break;
	case NUM_BIGNUM:
		return quadmath_tanh_realsolve(bignum_to_cf128(x));
		break;
	case NUM_RATIONAL:
		return quadmath_tanh_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_tanh_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		VALUE nucomp = quadmath_tanh_nucompsolve(nucomp_to_cc128(x));
		return complex128_to_nucomp(nucomp);
		break;
	case NUM_FLOAT128:
		return quadmath_tanh_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_tanh_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_tanh_realsolve(GetF128(x));
		else
			return quadmath_tanh_nucompsolve(GetC128(x));
		break;
	}
}

static inline VALUE
quadmath_asinh_realsolve(__float128 x)
{
	return rb_float128_cf128(asinhq(x));
}

static inline VALUE
quadmath_asinh_nucompsolve(__complex128 x)
{
	return rb_complex128_cc128(casinhq(x));
}

/*
 *  call-seq:
 *    QuadMath.asinh(x) -> Float128 | Complex128
 *  
 *  xの双曲線逆正弦を返す．
 *  xが実数なら実数解，複素数なら複素数解として各々返却する．
 *  
 *    QuadMath.asinh(1) # => 0.881373587019543025232609324979792
 *    QuadMath.asinh(1+1i) # => (1.0612750619050356520330189162135+0.666239432492515255104004895977792i)
 */
static VALUE
quadmath_asinh(VALUE unused_obj, VALUE x)
{
	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
		return quadmath_asinh_realsolve(fixnum_to_cf128(x));
		break;
	case NUM_BIGNUM:
		return quadmath_asinh_realsolve(bignum_to_cf128(x));
		break;
	case NUM_RATIONAL:
		return quadmath_asinh_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_asinh_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		VALUE nucomp = quadmath_asinh_nucompsolve(nucomp_to_cc128(x));
		return complex128_to_nucomp(nucomp);
		break;
	case NUM_FLOAT128:
		return quadmath_asinh_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_asinh_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_asinh_realsolve(GetF128(x));
		else
			return quadmath_asinh_nucompsolve(GetC128(x));
		break;
	}
}

static inline VALUE
quadmath_acosh_realsolve(__float128 x)
{
	if (isnan(x))
		return rb_float128_cf128(nanq(""));
	if (x >= 1)
		return rb_float128_cf128(acoshq(x));
	else
		return rb_complex128_cc128(cacoshq((__complex128)x));
}

static inline VALUE
quadmath_acosh_nucompsolve(__complex128 x)
{
	return rb_complex128_cc128(cacoshq(x));
}

/*
 *  call-seq:
 *    QuadMath.acosh(x) -> Float128 | Complex128
 *  
 *  xの双曲線逆余弦を返す．
 *  xが実数なら定義域(1<=x<=∞)は実数解，それ以外は複素数解，複素数なら複素数解として各々返却する．
 *  
 *    QuadMath.acosh(0) # => (0.0+1.5707963267948966192313216916397i)
 *    QuadMath.acosh(1) # => 0.0
 *    QuadMath.acosh(2) # => 1.3169578969248167086250463473079
 *    QuadMath.acosh(1+1i) # => (1.0612750619050356520330189162135+0.904556894302381364127316795661958i)
 */
static VALUE
quadmath_acosh(VALUE unused_obj, VALUE x)
{
	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
		return quadmath_acosh_realsolve(fixnum_to_cf128(x));
		break;
	case NUM_BIGNUM:
		return quadmath_acosh_realsolve(bignum_to_cf128(x));
		break;
	case NUM_RATIONAL:
		return quadmath_acosh_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_acosh_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		VALUE nucomp = quadmath_acosh_nucompsolve(nucomp_to_cc128(x));
		return complex128_to_nucomp(nucomp);
		break;
	case NUM_FLOAT128:
		return quadmath_acosh_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_acosh_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_acosh_realsolve(GetF128(x));
		else
			return quadmath_acosh_nucompsolve(GetC128(x));
		break;
	}
}

static inline VALUE
quadmath_atanh_realsolve(__float128 x)
{
	if (isnan(x))
		return rb_float128_cf128(nanq(""));
	if (x >= -1 && x <= 1)
		return rb_float128_cf128(atanhq(x));
	else
		return rb_complex128_cc128(catanhq((__complex128)x));
}

static inline VALUE
quadmath_atanh_nucompsolve(__complex128 x)
{
	return rb_complex128_cc128(catanhq(x));
}

/*
 *  call-seq:
 *    QuadMath.atanh(x) -> Float128 | Complex128
 *  
 *  xの双曲線逆正接を返す．
 *  xが実数なら定義域(-1<=x<=1)は実数解，それ以外は複素数解，複素数なら複素数解として各々返却する．
 *  
 *    puts "%s %s" % [" (x)", " tanh^-1(x)"]
 *    -15.step(15, 5){|x|
 *      x/=10.0; 
 *      y = QuadMath.atanh(x)
 *      y_str = y.to_s
 *      if !y.real.negative? then y_str = " #{y.to_s}"; end
 *      puts "%s %s" % [quadmath_sprintf("% 1.1Qf", x), y_str]
 *    }
 *    # =>  (x)  tanh^-1(x)
 *    # => -1.5 -0.804718956217050187300379666613093+1.5707963267948966192313216916397i
 *    # => -1.0 -Infinity
 *    # => -0.5 -0.549306144334054845697622618461263
 *    # =>  0.0  0.0
 *    # =>  0.5  0.549306144334054845697622618461263
 *    # =>  1.0  Infinity
 *    # =>  1.5  0.804718956217050187300379666613093+1.5707963267948966192313216916397i
 *    QuadMath.atanh(1+1i) # => (0.402359478108525093650189833306547+1.0172219678978513677227889615504i)
 */
static VALUE
quadmath_atanh(VALUE unused_obj, VALUE x)
{
	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
		return quadmath_atanh_realsolve(fixnum_to_cf128(x));
		break;
	case NUM_BIGNUM:
		return quadmath_atanh_realsolve(bignum_to_cf128(x));
		break;
	case NUM_RATIONAL:
		return quadmath_atanh_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_atanh_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		VALUE nucomp = quadmath_atanh_nucompsolve(nucomp_to_cc128(x));
		return complex128_to_nucomp(nucomp);
		break;
	case NUM_FLOAT128:
		return quadmath_atanh_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_atanh_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_atanh_realsolve(GetF128(x));
		else
			return quadmath_atanh_nucompsolve(GetC128(x));
		break;
	}
}

#ifndef HAVE_CL2NORM2Q
__float128 cl2norm2q(__complex128, __complex128);

# include "missing/cl2norm2q.c"

#endif

static inline VALUE
quadmath_hypot_realsolve(__float128 x, __float128 y)
{
	return rb_float128_cf128(hypotq(x, y));
}

static inline VALUE
quadmath_hypot_nucompsolve(__complex128 z, __complex128 w)
{
	return rb_float128_cf128(cl2norm2q(z, w));
}

static VALUE
hypot_inline(VALUE xsh, VALUE ysh)
{
	static __float128 x, y;
	static __complex128 z, w;
	bool x_nucomp_p = false, y_nucomp_p = false;
	
	switch (convertion_num_types(xsh)) {
	case NUM_FIXNUM:
		x = fixnum_to_cf128(xsh);
		break;
	case NUM_BIGNUM:
		x = bignum_to_cf128(xsh);
		break;
	case NUM_RATIONAL:
		x = rational_to_cf128(xsh);
		break;
	case NUM_FLOAT:
		x = float_to_cf128(xsh);
		break;
	case NUM_COMPLEX:
		z = nucomp_to_cc128(xsh);
		x_nucomp_p = true;
		break;
	case NUM_FLOAT128:
		x = GetF128(xsh);
		break;
	case NUM_COMPLEX128:
		z = GetC128(xsh);
		x_nucomp_p = true;
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(xsh))
			x = GetF128(xsh);
		else
		{
			z = GetC128(xsh);
			x_nucomp_p = true;
		}
		break;
	}

	switch (convertion_num_types(ysh)) {
	case NUM_FIXNUM:
		y = fixnum_to_cf128(ysh);
		break;
	case NUM_BIGNUM:
		y = bignum_to_cf128(ysh);
		break;
	case NUM_RATIONAL:
		y = rational_to_cf128(ysh);
		break;
	case NUM_FLOAT:
		y = float_to_cf128(ysh);
		break;
	case NUM_COMPLEX:
		w = nucomp_to_cc128(ysh);
		y_nucomp_p = true;
		break;
	case NUM_FLOAT128:
		y = GetF128(ysh);
		break;
	case NUM_COMPLEX128:
		w = GetC128(ysh);
		y_nucomp_p = true;
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(ysh))
			y = GetF128(ysh);
		else
		{
			w = GetC128(ysh);
			y_nucomp_p = true;
		}
		break;
	}
	
	if (x_nucomp_p && y_nucomp_p)
		return quadmath_hypot_nucompsolve(z, w);
	else if (x_nucomp_p && !y_nucomp_p)
		return quadmath_hypot_nucompsolve(z, y);
	else if (!x_nucomp_p && y_nucomp_p)
		return quadmath_hypot_nucompsolve(x, w);
	else
		return quadmath_hypot_realsolve(x, y);
}

/*
 *  call-seq:
 *    QuadMath.hypot(x, y) -> Float128
 *  
 *  xとyの直角三角形の斜辺の長さを実数で返す．
 *  2変数に複素数が含まれているならL2ノルムの答を返す．答は実数解・複素数解ともに常に正の実数である．
 *  
 *    QuadMath.hypot(3, 4) # => 5.0
 *    
 *    z = w = 2+1i;
 *    5.times do
 *      ans = QuadMath.hypot(z, w)
 *      print "hypot(%s, %s)^2 = %s^2 " % [z, w, ans]
 *      puts "= %s" % [ans ** 2]
 *      z += 1; w += 1;
 *    end
 *    # => hypot(2+1i, 2+1i)^2 = 3.1622776601683793319988935444327^2 = 10.0
 *    # => hypot(3+1i, 3+1i)^2 = 4.4721359549995793928183473374625^2 = 20.0
 *    # => hypot(4+1i, 4+1i)^2 = 5.8309518948453004708741528775455^2 = 34.0
 *    # => hypot(5+1i, 5+1i)^2 = 7.2111025509279785862384425349409^2 = 52.0
 *    # => hypot(6+1i, 6+1i)^2 = 8.6023252670426267717294735350497^2 = 74.0
 */
static VALUE
quadmath_hypot(VALUE unused_obj, VALUE x, VALUE y)
{
	return hypot_inline(x, y);
}


#ifndef HAVE_CERFCQ
extern __complex128 cerfcq(__complex128);
# include "missing/cerfcq.c"
#endif

#ifndef HAVE_CERFQ
extern __complex128 cerfq(__complex128);
# include "missing/cerfq.c"
#endif


static inline VALUE
quadmath_erf_realsolve(__float128 x)
{
	return rb_float128_cf128(erfq(x));
}

static inline VALUE
quadmath_erf_nucompsolve(__complex128 z)
{
	if (cimagq(z) == 0)
		return rb_complex128_cc128((__complex128)erfq(crealq(z)));
	else
		return rb_complex128_cc128(cerfq(z));
}

/*
 *  call-seq:
 *    QuadMath.erf(x) -> Float128 | Complex128
 *  
 *  xの誤差関数を返す．
 *  xが実数なら実数解，複素数なら複素数解として各々返却する．
 *  
 *    QuadMath.erf(-2) # => -0.995322265018952734162069256367253
 *    QuadMath.erf( 2) # => 0.995322265018952734162069256367253
 *    QuadMath.erf(Complex128::I) # => (0.0+1.6504257587975428760253377295613i)
 *    QuadMath.erf(1+1i) # => (1.3161512816979476448802710802436+0.190453469237834686284108861969162i)
 */
static VALUE
quadmath_erf(VALUE unused_obj, VALUE x)
{
	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
		return quadmath_erf_realsolve(fixnum_to_cf128(x));
		break;
	case NUM_BIGNUM:
		return quadmath_erf_realsolve(bignum_to_cf128(x));
		break;
	case NUM_RATIONAL:
		return quadmath_erf_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_erf_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		VALUE nucomp = quadmath_erf_nucompsolve(nucomp_to_cc128(x));
		return complex128_to_nucomp(nucomp);
		break;
	case NUM_FLOAT128:
		return quadmath_erf_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_erf_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_erf_realsolve(GetF128(x));
		else
			return quadmath_erf_nucompsolve(GetC128(x));
		break;
	}
}

static inline VALUE
quadmath_erfc_realsolve(__float128 x)
{
	return rb_float128_cf128(erfcq(x));
}

static inline VALUE
quadmath_erfc_nucompsolve(__complex128 z)
{
	if (cimagq(z) == 0)
		return rb_complex128_cc128((__complex128)erfcq(crealq(z)));
	else
		return rb_complex128_cc128(cerfcq(z));
}

/*
 *  call-seq:
 *    QuadMath.erfc(x) -> Float128 | Complex128
 *  
 *  xの相補誤差関数を返す．
 *  xが実数なら実数解，複素数なら複素数解として各々返却する．
 *  
 *    QuadMath.erfc(-2) # => 1.9953222650189527341620692563672
 *    QuadMath.erfc( 2) # => 0.00467773498104726583793074363274707
 *    QuadMath.erfc(Complex128::I) # => (1.0-1.6504257587975428760253377295613i)
 *    QuadMath.erfc(1+1i) # => (-0.31615128169794764488027108024367-0.190453469237834686284108861969162i)
 */
static VALUE
quadmath_erfc(VALUE unused_obj, VALUE x)
{
	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
		return quadmath_erfc_realsolve(fixnum_to_cf128(x));
		break;
	case NUM_BIGNUM:
		return quadmath_erfc_realsolve(bignum_to_cf128(x));
		break;
	case NUM_RATIONAL:
		return quadmath_erfc_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_erfc_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		VALUE nucomp = quadmath_erfc_nucompsolve(nucomp_to_cc128(x));
		return complex128_to_nucomp(nucomp);
		break;
	case NUM_FLOAT128:
		return quadmath_erfc_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_erfc_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_erfc_realsolve(GetF128(x));
		else
			return quadmath_erfc_nucompsolve(GetC128(x));
		break;
	}
}



#ifndef HAVE_CLGAMMAQ
extern __complex128 clgammaq(__complex128);
# include "missing/clgammaq.c"
#endif

static inline __complex128
lgamma_negarg(__float128 x)
{
		__complex128 z;
		__real__ z = lgammaq(x);
		__imag__ z = floorq(x) * M_PIq;
		return z;
}


static inline VALUE
quadmath_lgamma_realsolve(__float128 x)
{
	if (isnan(x))
		return rb_float128_cf128(nanq(""));
	if (!signbitq(x))
		return rb_float128_cf128(lgammaq(x));
	else
		return rb_complex128_cc128(lgamma_negarg(x));
}



static inline VALUE
quadmath_lgamma_nucompsolve(__complex128 z)
{
	if (cimagq(z) == 0)
	{
		__float128 real = crealq(z);
		if (isnanq(real))
			return rb_complex128_cc128((__complex128)real);
		else if (!signbitq(real))
			return rb_complex128_cc128((__complex128)lgammaq(real));
		else
			return rb_complex128_cc128(lgamma_negarg(real));
	}
	else
		return rb_complex128_cc128(clgammaq(z));
}

/*
 *  call-seq:
 *    QuadMath.lgamma(x) -> Float128 | Complex128
 *  
 *  xの対数ガンマ関数を返す．
 *  xが実数で正なら実数解，負なら複素数解，複素数なら複素数解を返す．
 *  
 *    
 */
static VALUE
quadmath_lgamma(VALUE unused_obj, VALUE x)
{
	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
		return quadmath_lgamma_realsolve(fixnum_to_cf128(x));
		break;
	case NUM_BIGNUM:
		return quadmath_lgamma_realsolve(bignum_to_cf128(x));
		break;
	case NUM_RATIONAL:
		return quadmath_lgamma_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_lgamma_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		VALUE nucomp = quadmath_lgamma_nucompsolve(nucomp_to_cc128(x));
//		return complex128_to_nucomp(nucomp);
		return nucomp;
		break;
	case NUM_FLOAT128:
		return quadmath_lgamma_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_lgamma_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_lgamma_realsolve(GetF128(x));
		else
			return quadmath_lgamma_nucompsolve(GetC128(x));
		break;
	}
}

static inline VALUE
quadmath_lgamma_r_realsolve(__float128 x)
{
	int sign = 1;
	__float128 y = lgammaq(x);
	
	if (x < 0 && fmodq(floorq(x), 2))
		sign = -1;
	
	return rb_assoc_new(rb_float128_cf128(y), INT2FIX(sign));
}

static inline VALUE
quadmath_lgamma_r_nucompsolve(__complex128 z)
{
	int sign = 1;
	if (cimagq(z) == 0)
	{
		__float128 real = crealq(z);
		if (real < 0 && fmodq(floorq(real), 2))
			sign = -1;
		return rb_assoc_new(
			rb_float128_cf128(lgammaq(real)), 
			INT2FIX(sign)
			);
	}
	else
		return rb_assoc_new(Qnil, INT2FIX(0));
}

/*
 *  call-seq:
 *    QuadMath.lgamma_r(x) -> [Float128, Integer] | [nil, 0]
 *    QuadMath.signgam(x) -> [Float128, Integer] | [nil, 0]
 *  
 *  伝統的なlgamma_r()関数をquadmathモジュールとして提供する．対数ガンマ関数の実数解とその符号値が配列で渡される．
 *  値を指数に戻す場合，周期性$2\pi$は数値計算に精度を保証できない．オイラー積分で対数から指数に戻す場合は，大概にこのような関数を使用するべきである．
 *  虚数の現れている数値を渡すと，計算不能と判断し，nilと0のペアを返す．
 *  
 *    x = -0.5r
 *    lgamm, sign = QuadMath.lgamma_r(x)
 *    # => [1.2655121234846453964889457971347, -1]
 *    QuadMath.exp(lgamm) * sign
 *    # => -3.5449077018110320545963349666822
 *    QuadMath.gamma(x)
 *    # => -3.5449077018110320545963349666822
 */
static VALUE
quadmath_lgamma_r(VALUE unused_obj, VALUE x)
{
	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
		return quadmath_lgamma_r_realsolve(fixnum_to_cf128(x));
		break;
	case NUM_BIGNUM:
		return quadmath_lgamma_r_realsolve(bignum_to_cf128(x));
		break;
	case NUM_RATIONAL:
		return quadmath_lgamma_r_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_lgamma_r_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		VALUE nucomp = quadmath_lgamma_r_nucompsolve(nucomp_to_cc128(x));
//		return complex128_to_nucomp(nucomp);
		return nucomp;
		break;
	case NUM_FLOAT128:
		return quadmath_lgamma_r_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_lgamma_r_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_lgamma_r_realsolve(GetF128(x));
		else
			return quadmath_lgamma_r_nucompsolve(GetC128(x));
		break;
	}
}


#ifndef HAVE_CTGAMMAQ
extern __complex128 ctgammaq(__complex128);
# include "missing/ctgammaq.c"
#endif


static inline VALUE
quadmath_gamma_realsolve(__float128 x)
{
	return rb_float128_cf128(tgammaq(x));
}



static inline VALUE
quadmath_gamma_nucompsolve(__complex128 z)
{
	if (cimagq(z) == 0)
		return rb_complex128_cc128((__complex128)tgammaq(crealq(z)));
	else
//		return rb_complex128_cc128(ctgammaq(z));
		return Qnil;
}

/*
 *  call-seq:
 *    QuadMath.gamma(x) -> Float128 | Complex128
 *  
 *  xのガンマ関数を返す．
 *  xが実数なら実数解，素数なら複素数解として各々返却する．
 *  
 *    
 */
static VALUE
quadmath_gamma(VALUE unused_obj, VALUE x)
{
	switch (convertion_num_types(x)) {
	case NUM_FIXNUM:
		return quadmath_gamma_realsolve(fixnum_to_cf128(x));
		break;
	case NUM_BIGNUM:
		return quadmath_gamma_realsolve(bignum_to_cf128(x));
		break;
	case NUM_RATIONAL:
		return quadmath_gamma_realsolve(rational_to_cf128(x));
		break;
	case NUM_FLOAT:
		return quadmath_gamma_realsolve(float_to_cf128(x));
		break;
	case NUM_COMPLEX:
		VALUE nucomp = quadmath_gamma_nucompsolve(nucomp_to_cc128(x));
//		return complex128_to_nucomp(nucomp);
		return nucomp;
		break;
	case NUM_FLOAT128:
		return quadmath_gamma_realsolve(GetF128(x));
		break;
	case NUM_COMPLEX128:
		return quadmath_gamma_nucompsolve(GetC128(x));
		break;
	case NUM_OTHERTYPE:
	default:
		if (num_real_p(x))
			return quadmath_gamma_realsolve(GetF128(x));
		else
			return quadmath_gamma_nucompsolve(GetC128(x));
		break;
	}
}

void
InitVM_QuadMath(void)
{
	/* Math-Functions */
	rb_define_module_function(rb_mQuadMath, "exp", quadmath_exp, 1);
	rb_define_module_function(rb_mQuadMath, "exp2", quadmath_exp2, 1);
	rb_define_module_function(rb_mQuadMath, "expm1", quadmath_expm1, 1);
	rb_define_module_function(rb_mQuadMath, "log", quadmath_log, 1);
	rb_define_module_function(rb_mQuadMath, "log2", quadmath_log2, 1);
	rb_define_module_function(rb_mQuadMath, "log10", quadmath_log10, 1);
	rb_define_module_function(rb_mQuadMath, "log1p", quadmath_log1p, 1);
	rb_define_module_function(rb_mQuadMath, "sqrt", quadmath_sqrt, 1);
	rb_define_module_function(rb_mQuadMath, "sqrt3", quadmath_sqrt3, 1);
	rb_define_module_function(rb_mQuadMath, "cbrt", quadmath_cbrt, 1);
	rb_define_module_function(rb_mQuadMath, "sin", quadmath_sin, 1);
	rb_define_module_function(rb_mQuadMath, "cos", quadmath_cos, 1);
	rb_define_module_function(rb_mQuadMath, "tan", quadmath_tan, 1);
	rb_define_module_function(rb_mQuadMath, "asin", quadmath_asin, 1);
	rb_define_module_function(rb_mQuadMath, "acos", quadmath_acos, 1);
	rb_define_module_function(rb_mQuadMath, "atan", quadmath_atan, 1);
	rb_define_module_function(rb_mQuadMath, "atan2", quadmath_atan2, 2);
	rb_define_module_function(rb_mQuadMath, "quadrant", quadmath_quadrant, 2);
	rb_define_module_function(rb_mQuadMath, "sinh", quadmath_sinh, 1);
	rb_define_module_function(rb_mQuadMath, "cosh", quadmath_cosh, 1);
	rb_define_module_function(rb_mQuadMath, "tanh", quadmath_tanh, 1);
	rb_define_module_function(rb_mQuadMath, "asinh", quadmath_asinh, 1);
	rb_define_module_function(rb_mQuadMath, "acosh", quadmath_acosh, 1);
	rb_define_module_function(rb_mQuadMath, "atanh", quadmath_atanh, 1);
	rb_define_module_function(rb_mQuadMath, "hypot", quadmath_hypot, 2);
	rb_define_module_function(rb_mQuadMath, "erf", quadmath_erf, 1);
	rb_define_module_function(rb_mQuadMath, "erfc", quadmath_erfc, 1);
	rb_define_module_function(rb_mQuadMath, "lgamma", quadmath_lgamma, 1);
	rb_define_module_function(rb_mQuadMath, "lgamma_r", quadmath_lgamma_r, 1);
	rb_define_module_function(rb_mQuadMath, "signgam", quadmath_lgamma_r, 1);
	rb_define_module_function(rb_mQuadMath, "gamma", quadmath_gamma, 1);
	
	/* Math-Constants */
	rb_define_const(rb_mQuadMath, "E", rb_float128_cf128(M_Eq));
	rb_define_const(rb_mQuadMath, "LOG2E", rb_float128_cf128(M_LOG2Eq));
	rb_define_const(rb_mQuadMath, "LOG10E", rb_float128_cf128(M_LOG10Eq));
	rb_define_const(rb_mQuadMath, "LN2", rb_float128_cf128(M_LN2q));
	rb_define_const(rb_mQuadMath, "LN10", rb_float128_cf128(M_LN10q));
	rb_define_const(rb_mQuadMath, "PI", rb_float128_cf128(M_PIq));
	rb_define_const(rb_mQuadMath, "PI_2", rb_float128_cf128(M_PI_2q));
	rb_define_const(rb_mQuadMath, "PI_4", rb_float128_cf128(M_PI_4q));
	rb_define_const(rb_mQuadMath, "ONE_PI", rb_float128_cf128(M_1_PIq));
	rb_define_const(rb_mQuadMath, "TWO_PI", rb_float128_cf128(M_2_PIq));
	rb_define_const(rb_mQuadMath, "TWO_SQRTPI", rb_float128_cf128(M_2_SQRTPIq));
	rb_define_const(rb_mQuadMath, "SQRT2", rb_float128_cf128(M_SQRT2q));
	rb_define_const(rb_mQuadMath, "SQRT1_2", rb_float128_cf128(M_SQRT1_2q));
}
