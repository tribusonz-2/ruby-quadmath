#ifndef RUBY_QUADMATH_ABI_H
#define RUBY_QUADMATH_ABI_H 1

#if defined(__cplusplus)
extern "C" {
#endif

// bigdecimal.cより拝借
#ifdef HAVE_RB_OPTS_EXCEPTION_P
int rb_opts_exception_p(VALUE opts, int default_value);
#define opts_exception_p(opts) rb_opts_exception_p((opts), 1)
#else
#include "missing/opts_exception_p.c"
#endif

#ifdef     USE_GLOBAL_VARIABLE
# define   RUBY_EXT_EXTERN
# define   GLOBAL_VAL(v) = (v)
#else
# define   RUBY_EXT_EXTERN    extern
# define   GLOBAL_VAL(v)
#endif

RUBY_EXT_EXTERN VALUE rb_cFloat128;
RUBY_EXT_EXTERN VALUE rb_cComplex128;
RUBY_EXT_EXTERN VALUE rb_mQuadMath;

/*
 * C API: rb_float128_cf128(x)
 * 
 * Cの__float128型をRubyのFloat128型オブジェクトへ変換する．
 * 
 * @x ... __float128型
 * @@retval ... 変換されたRubyのFloat128型
 */
VALUE rb_float128_cf128(__float128);

/*
 * C API: rb_float128_value(x)
 * 
 * RubyのFloat128型より実体を取り出し渡す．
 * 
 * @x ... Float128型．他の型を渡すとTypeErrorを出す．
 * @@retval ... 取り出されたCの__float128型
 */
__float128 rb_float128_value(VALUE);

/*
 * C API: rb_complex128_cc128(z)
 * 
 * Cの__complex128型をRubyのComplex128型オブジェクトへ変換する．
 * 
 * @z ... __complex128型
 * @@retval ... 変換されたRubyのComplex128型
 */
VALUE rb_complex128_cc128(__complex128);

/*
 * C API: rb_complex128_value(x)
 * 
 * RubyのComplex128型より実体を取り出し渡す．
 * 
 * @x ... Complex128型．他の型を渡すとTypeErrorを出す．
 * @@retval ... 取り出されたCの__complex128型
 */
__complex128 rb_complex128_value(VALUE);

#define MAKE_SYM(str)  ID2SYM(rb_intern(str))

#if defined(__cplusplus)
}
#endif

#endif /* RUBY_QUADMATH_ABI_H */

