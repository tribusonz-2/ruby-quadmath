#include <quadmath.h>

/* hypot()実装: Moler-Morrison法 */
static inline __float128
hypot_mm_method(__float128 x, __float128 y)
{
	__float128 t;
	static int iter_cnt = 4;  // float = 2, double = 3, __float128 = 4
	
	// x = fabs(x);  y = fabs(y);
	if (x < y)  {  t = x;  x = y;  y = t;  }
	if (y == 0)  return x;
	for (int i = 0; i < iter_cnt; i++)
	{
		t = y / x;  t *= t;  t /= 4 + t;
		x += 2 * x * t;  y *= t;
	}
	return x;
}


__float128
cl2normq(__complex128 z, __complex128 w)
{
	__float128 z_real = crealq(z), z_imag = cimagq(z),
	           w_real = crealq(w), w_imag = cimagq(w);
	
	if (z_imag == 0 && w_imag == 0)
		return hypotq(z_real, z_real);
	else if (finiteq(z_real) && finiteq(z_imag) &&
	         finiteq(w_real) && finiteq(w_imag))
	{
		__float128 abs_z = cabsq(z), abs_w = cabsq(w);
		return hypot_mm_method(abs_z, abs_w);
	}
	else if (isnanq(z_real) || isnanq(z_imag) ||
	         isnanq(w_real) || isnanq(w_imag))
	{
		return nanq("");
	}
	else
		return HUGE_VALQ;
}
