/*******************************************************************************
    main.c -- QuadMath EntryPoint
    
    Author: Hironobu Inatsuka
*******************************************************************************/
#include <ruby.h>
#include <quadmath.h>
#define USE_GLOBAL_VARIABLE
#include "rb_quadmath.h"
#include "internal/rb_quadmath.h"
#include "missing/ool_quad2str.c"

void InitVM_Float128(void);
void InitVM_Complex128(void);
void InitVM_Numerable(void);
void InitVM_QuadMath(void);

// EntryPoint
void
Init_quadmath(void)
{
	rb_cFloat128 = rb_define_class("Float128", rb_cNumeric);
	rb_cComplex128 = rb_define_class("Complex128", rb_cNumeric);
	rb_mQuadMath = rb_define_module("QuadMath");
	
	InitVM(Float128);
	InitVM(Complex128);
	InitVM(Numerable);
	InitVM(QuadMath);
}

