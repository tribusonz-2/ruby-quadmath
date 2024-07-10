#ifndef RB_QUADMATH_INTERNAL_TYPES_H
#define RB_QUADMATH_INTERNAL_TYPES_H

#if defined(__cplusplus)
extern "C" {
#endif

__float128 GetF128(VALUE);
__complex128 GetC128(VALUE);

VALUE float128_nucomp_pow(VALUE x, VALUE y);

enum NUMERIC_SUBCLASSES {
	NUM_FIXNUM,
	NUM_BIGNUM,
	NUM_RATIONAL,
	NUM_FLOAT,
	NUM_COMPLEX,
	NUM_FLOAT128,
	NUM_COMPLEX128,
	NUM_OTHERTYPE
};

static inline enum NUMERIC_SUBCLASSES
convertion_num_types(VALUE obj)
{
	switch (TYPE(obj)) {
	case T_FIXNUM:
		return NUM_FIXNUM;
		break;
	case T_BIGNUM:
		return NUM_BIGNUM;
		break;
	case T_RATIONAL:
		return NUM_RATIONAL;
		break;
	case T_FLOAT:
		return NUM_FLOAT;
		break;
	case T_COMPLEX:
		return NUM_COMPLEX;
		break;
	case T_NIL:
	case T_TRUE:
	case T_FALSE:
		rb_raise(rb_eTypeError, 
		  "can't convert %"PRIsVALUE" into %s|%s", 
		  rb_String(obj), rb_class2name(rb_cFloat128), 
		  rb_class2name(rb_cComplex128));
	default:
		if (CLASS_OF(obj) == rb_cFloat128)
			return NUM_FLOAT128;
		else if (CLASS_OF(obj) == rb_cComplex128)
			return NUM_COMPLEX128;
		else
		{
			VALUE num_subclasses = rb_class_subclasses(rb_cNumeric);
			if (RTEST(rb_ary_includes(num_subclasses, CLASS_OF(obj))))
				return NUM_OTHERTYPE;
			else
				rb_raise(rb_eTypeError, 
					  "can't convert %"PRIsVALUE" into %s|%s", 
					  CLASS_OF(obj), rb_class2name(rb_cFloat128), 
					  rb_class2name(rb_cComplex128));
				
		}
	}
}

#if defined(__cplusplus)
}
#endif


#endif /* RB_QUADMATH_INTERNAL_TYPES_H */

