/*******************************************************************************
	complex128.c -- Complex128 Class

	Author: Hironobu Inatsuka
*******************************************************************************/
#include <ruby.h>
#include <quadmath.h>
#include "rb_quadmath.h"
#include "internal/rb_quadmath.h"

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
	VALUE obj;
	ptr->value = x;
	obj = TypedData_Wrap_Struct(rb_cComplex128, &complex128_data_type, ptr);
	RB_OBJ_FREEZE(obj);
	return obj;
}

__complex128
GetC128(VALUE self)
{
	struct C128 *c128;
	
	TypedData_Get_Struct(self, struct C128, &complex128_data_type, c128);
	
	return c128->value;
}

/*
 *  call-seq:
 *    hash -> Integer
 *  
 *  +self+のHash値を返す．
 *  
 */
static VALUE
complex128_hash(VALUE self)
{
	struct C128 *c128;;
	st_index_t hash;
	
	TypedData_Get_Struct(self, struct C128, &complex128_data_type, c128);
	
	hash = rb_memhash(c128, sizeof(struct C128));
	
	return ST2FIX(hash);
}

/*
 *  call-seq:
 *    eql?(other) -> bool
 *  
 *  +self+と+other+が等しければ真を返す．
 */
static VALUE
complex128_eql_p(VALUE self, VALUE other)
{
	__complex128 left_c128, right_c128;
	
	if (CLASS_OF(other) != rb_cComplex128)
		return Qfalse;
	
	left_c128 = GetC128(self);
	right_c128  = GetC128(other);
	
	return left_c128 == right_c128 ? Qtrue : Qfalse;
}

#if 0
/*
 *  call-seq:
 *    -self -> Complex128
 *  
 *  単項マイナス．+self+をマイナスにして返す．
 */
static VALUE
complex128_uminus(VALUE self)
{
	__complex128 c128 = GetC128(self);
	
	return rb_complex128_cc128(-c128);
}
#endif

/*
 *  call-seq:
 *    infinite? -> nil | 1
 *  
 *  +self+が無限複素量であるかどうかを審議する．実部・虚部がともに有限であればnilを，そうでなければ1を返す．
 
 */
static VALUE
complex128_infinite_p(VALUE self)
{
	__complex128 c128 = GetC128(self);
	
	return (isinfq(crealq(c128)) || isinfq(cimagq(c128))) ?
		INT2FIX(1) : Qnil;
}

/*
 *  call-seq:
 *    finite? -> bool
 *  
 *  +self+の実部・虚部がともに有限であればtrueを，そうでなければfalseを返す．
 
 */
static VALUE
complex128_finite_p(VALUE self)
{
	__complex128 c128 = GetC128(self);
	__float128 real, imag;
	
	real = crealq(c128); imag = cimagq(c128);
	
	return (finiteq(real) && finiteq(imag)) ? Qtrue : Qfalse;
}

static char
member_format(__float128 x, VALUE s)
{
	char* str;
	int exp, sign;
	char format = ool_quad2str(fabsq(x), 'g', &exp, &sign, &str);
	
	switch (format) {
	case '0':
		rb_raise(rb_eRuntimeError, "error occured in ool_quad2str()");
		break;
	case '1':
		rb_str_concat(s, rb_sprintf("%s", str));
		break;
	case 'e':
		rb_str_concat(s, rb_sprintf("%se%+d", str, exp));
		break;
	case 'f':
		rb_str_concat(s, rb_sprintf("%s", str));
		break;
	default:
		rb_raise(rb_eRuntimeError, "format error");
		break;
	}
	return format;
}

static VALUE
f_format(VALUE self)
{
	VALUE retval = rb_str_new(0,0);
	__complex128 c128 = GetC128(self);
	char format;
	__float128 memb;
	
	memb = crealq(c128);
	if (signbitq(memb))  rb_str_cat2(retval, "-");
	member_format(memb, retval);
	
	memb = cimagq(c128);
	rb_str_cat2(retval, !signbitq(memb) ? "+" : "-");
	format = member_format(memb, retval);
	if (format == '1')  rb_str_cat2(retval, "*");
	rb_str_cat2(retval, "i");
	
	return retval;
}

/*
 *  call-seq:
 *    to_s -> String
 *  
 *  +self+をStringに変換する．
 */
static VALUE
complex128_to_s(VALUE self)
{
	return f_format(self);
}

/*
 *  call-seq:
 *    inspect -> String
 *  
 *  +self+を(Re+Im)の形で見やすくする．
 */
static VALUE
complex128_inspect(VALUE self)
{
	VALUE s;
	
	s = rb_usascii_str_new2("(");
	rb_str_concat(s, f_format(self));
	rb_str_cat2(s, ")");

	return s;
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
	__complex128 c128 = GetC128(self);
	
	if (cimagq(c128) == 0)
	{
		__float128 real = crealq(c128);
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
		  rb_String(self), rb_class2name(rb_cInteger));
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
	__complex128 c128 = GetC128(self);
	
	if (cimagq(c128) == 0)
		return DBL2NUM((double)crealq(c128));
	else
		rb_raise(rb_eRangeError, 
		  "can't convert %"PRIsVALUE" into %s", 
		  rb_String(self), rb_class2name(rb_cFloat));
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
	__complex128 c128 = GetC128(self);
	
	if (cimagq(c128) == 0)
		return rb_float128_cf128(crealq(c128));
	else
		rb_raise(rb_eRangeError, 
		  "can't convert %"PRIsVALUE" into %s", 
		  rb_String(self), rb_class2name(rb_cFloat128));
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
	__complex128 c128 = GetC128(self);
	
	return rb_Complex(
		rb_float128_cf128(crealq(c128)),
		rb_float128_cf128(cimagq(c128)));
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
	__complex128 c128 = GetC128(self);
	
	return rb_float128_cf128(crealq(c128));
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
	__complex128 c128 = GetC128(self);
	
	return rb_float128_cf128(cimagq(c128));
}

/*
 *  call-seq:
 *    rect -> [Float128, Float128]
 *    rectangular -> [Float128, Float128]
 *  
 *  実部と虚部を配列にして返す．
 *  見た目はComplex型と変わりないが，成分はFloat128型のみなのが異なる．
 *  
 *  Complex128(3).rect    # => [3.0, 0.0]
 *  Complex128(3.5).rect  # => [3.5, 0.0]
 *  Complex128(3, 2).rect # => [3.0, 2.0]
 *  Complex128(1, 1).rect.all?(Float128) # => true
 */
static VALUE
complex128_rect(VALUE self)
{
	__complex128 c128 = GetC128(self);
	
	return rb_assoc_new(
		rb_float128_cf128(crealq(c128)), 
		rb_float128_cf128(cimagq(c128)));
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

/*
 *  call-seq:
 *    to_c64 -> Complex
 *  
 *  +self+を二倍精度に精度引き下げしてこれをComplexに変換し返す.
 *  
 *    QuadMath.asin(1.1r).to_c64 # => (1.5707963267948966+0.4435682543851152i)
 */
static VALUE
complex128_to_c64(VALUE self)
{
	__complex128 c128 = GetC128(self);
	return rb_dbl_complex_new((double)crealq(c128), (double)cimagq(c128));
}

static VALUE
f128_to_f64(VALUE obj)
{
	__float128 elem = GetF128(obj);
	return DBL2NUM((double)elem);
}

/*
 *  call-seq:
 *    to_c64 -> Complex
 *  
 *  +self+の成分が四倍精度の場合，二倍精度に精度引き下げして返す．
 *  
 *    z = Complex(3.to_f128) # => (3.0+0i)
 *    z.real.class        # => Float128
 *    z.to_c64.real.class # => Float

 */
static VALUE
nucomp_to_c64(VALUE self)
{
	VALUE real = rb_complex_real(self);
	VALUE imag = rb_complex_imag(self);
	
	if (CLASS_OF(real) == rb_cFloat128)
		real = f128_to_f64(real);
	if (CLASS_OF(imag) == rb_cFloat128)
		imag = f128_to_f64(imag);
	
	return rb_Complex(real, imag);
}


void
InitVM_Complex128(void)
{
	/* Class methods */
	rb_undef_alloc_func(rb_cComplex128);
	rb_undef_method(CLASS_OF(rb_cComplex128), "new");
	
	/* Object methods */
	rb_define_method(rb_cComplex128, "hash", complex128_hash, 0);
	rb_define_method(rb_cComplex128, "eql?", complex128_eql_p, 1);
	
	/* The unique methods */
	rb_define_method(rb_cComplex128, "real?", complex128_real_p, 0);
	rb_define_method(rb_cComplex128, "real", complex128_real, 0);
	rb_define_method(rb_cComplex128, "imag", complex128_imag, 0);
	rb_define_alias(rb_cComplex128, "imaginary", "imag");
	rb_define_method(rb_cComplex128, "rect", complex128_rect, 0);
	rb_define_alias(rb_cComplex128, "rectangular", "rect");
	rb_undef_method(rb_cComplex128, "i");
	rb_define_method(rb_cComplex128, "infinite?", complex128_infinite_p, 0);
	rb_define_method(rb_cComplex128, "finite?", complex128_finite_p, 0);
	
	/* Operators & Evals */
#if 0
	rb_define_method(rb_cComplex128, "-@", complex128_uminus, 0);
#endif
	
	/* Type convertion methods */
	rb_define_method(rb_cComplex128, "to_s", complex128_to_s, 0);
	rb_define_method(rb_cComplex128, "inspect", complex128_inspect, 0);
	
	rb_define_method(rb_cComplex128, "to_f", complex128_to_f, 0);
	rb_define_method(rb_cComplex128, "to_f128", complex128_to_f128, 0);
	
	rb_define_method(rb_cComplex128, "to_i", complex128_to_i, 0);
	rb_define_method(rb_cComplex128, "to_int", complex128_to_i, 0);
	
	rb_define_method(rb_cComplex128, "to_c", complex128_to_c, 0);
	rb_define_method(rb_cComplex128, "to_c128", complex128_to_c128, 0);
	
	rb_define_method(rb_cComplex128, "to_c64", complex128_to_c64, 0);
	rb_define_method(rb_cComplex, "to_c64", nucomp_to_c64, 0);
	
	/* Constants */
	rb_define_const(rb_cComplex128, "I", rb_complex128_cc128(0+1i));
}

VALUE
rb_complex128_cc128(__complex128 x)
{
	VALUE obj = complex128_allocate(x);
	return obj;
}

__complex128
rb_complex128_value(VALUE x)
{
	struct C128 *c128 = rb_check_typeddata(x, &complex128_data_type);
	
	return c128->value;
}
