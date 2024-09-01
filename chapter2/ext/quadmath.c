/*******************************************************************************
    quadmath.c -- module QuadMath
    
    Author: Hironobu Inatsuka
*******************************************************************************/
#include <ruby.h>
#include <quadmath.h>
#include "abi.h"

void
InitVM_QuadMath(void)
{
	/* Constants */
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
