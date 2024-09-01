#include <quadmath.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define BUF_SIZE 0x2000
#define RADIX10_OFFSET  1
#define RADIX2_OFFSET  2
#define FRAC_DIG (1.0e-5)

static inline int
index_of_point(char *str)
{
	for (volatile int i = 0; i < (int)strlen(str); i++)
	{
		if (str[i] == '.')  return i;
	}
	return -1;
}

static inline __float128
foo(__float128 x)
{
	return x;
}

/**
 * ool_quad2str()
 * オブジェクト指向言語仕様として，__float128型をC文字列に変換する．
 * @x ... 変換元の__float128型．
 * @format ... 変換する書式．
 *              'a' 'A' .. 十六進表記． 例: 0x1p+0 #=> 1.0
 *              'b' 'B' .. 二進法の指数表記 (0~1) 例: 0.1e+1 #=> 1.0
 *              'e' 'E' .. 十進法の指数表記 (1~10) 例: 1.0e+0 #=> 1.0
 *              'f' 'F' .. 浮動小数点表記 例: 1.0 #=> 1.0
 *              'g' 'G' .. ジェネリック．FLT128_DIGに応じて値を見やすくする．
 * @exp ... 値の指数値．二進法の場合は正では1加算，負では1減算される．
 * @sign ... xの符号値．非数の場合0，正の実数では1，負の実数では-1．
 * @buf ... 変換された文字列．静的クラスを与えた内部変数sへのポインタを渡す．
 * @@retval ... 結果を書式formatで返す．
 *              '0' .. 失敗．
 *              '1' .. 有限でない値であった場合に返す．無限大か非数のどちらかだが，signが-1は負の無限大，0は非数，1は正の無限大でスイッチできる．
 *              'a' .. 十六進表記 例: 1.0 #=> 0x1p+0
 *              'b' .. 二進法の指数表記 (0~1) 例: 1.0 #=> 0.1e+1
 *              'e' .. 十進法の指数表記 (1~10) 例: 1.0 #=> 1.0e+0
 *              'f' .. 浮動小数点表記 例: 1.0 #=> 1.0
 */
char
ool_quad2str(__float128 x, char format, int *exp, int *sign, char **buf)
{
	static char s[BUF_SIZE];
	static char near_one[BUF_SIZE];
	int expv = 0, signv = 0, offset = RADIX10_OFFSET;
	int pos = -1;
	char retval = '0';
	
	if (near_one[0] == '\0')
	{
		near_one[0] = '0'; near_one[1] = '.';
		for (long i = 0; i < FLT128_DIG; i++)
		{
			near_one[i+2] = '9';
		}
		near_one[FLT128_DIG+2] = '\0';
	}
	
	switch (format) {
	case 'a': case 'A':
		retval = 'a';
		break;
	case 'b': case 'B':
		retval = 'b'; offset = RADIX2_OFFSET;
		break;
	case 'e': case 'E':
		retval = 'e';
		break;
	case 'f': case 'F':
		retval = 'f';
		break;
	case 'g': case 'G':
		retval = 'g';
		break;
	default:
		goto end;
		break;
	}
	if (isnanq(x))
	{
		signv = 0;
		expv = 0;
		s[0] = 'N'; s[1] = 'a'; s[2] = 'N'; s[3] = 0;
		*buf = s;
		retval = '1';
	}
	else
	{
		__float128 absx = fabsq(x);
		
		if (signbitq(x))  signv = -1;
		else              signv =  1;
		
		if (isinfq(x))
		{
			expv = 0;
			s[0] = 'I'; s[1] = 'n'; s[2] = 'f';
			s[3] = 'i'; s[4] = 'n'; s[5] = 'i';
			s[6] = 't'; s[7] = 'y'; s[8] = '\0';
			*buf = s;
			retval = '1';
		}
		else if (retval == 'a')
		{
			int size = quadmath_snprintf(s, BUF_SIZE, "%Qa", absx);
			int is_exp_part = 0;
			int exp_sign = 0;
			for (int i = 0; i < size; i++)
			{
				if (is_exp_part)
				{
					switch (s[i]) {
					case '+':
						exp_sign = 1;
						break;
					case '-':
						exp_sign = -1;
						break;
					case '0': case '1': case '2': case '3': case '4': 
					case '5': case '6': case '7': case '8': case '9':
						if (expv == 0)  expv = s[i] - '0';
						else            expv = expv * 10 + (s[i] - '0');
						break;
					default:
						break;
					}
				}
				if (s[i] == 'p' && !is_exp_part)
				{
					s[i] = '\0';
					is_exp_part = 1;
				}
			}
			expv *= exp_sign;
			*buf = s;
		}
		else if (absx == 0)
		{
			if (retval == 'g')  retval = 'f';
			expv = 0;
			s[0] = '0'; s[1] = '.'; s[2] = '0'; s[3] = '\0';
			*buf = s;
		}
		else if (absx >= 10)
		{
			quadmath_snprintf(s+offset, BUF_SIZE-offset, "%*.*Qf", FLT128_DIG, FLT128_DIG, absx);
			pos = index_of_point(s+offset);
			expv = (int)pos - 1;
			switch (retval) {
			case 'b':
				expv += 1;
				break;
			case 'g':
				retval = ((expv + 1) > FLT128_DIG) ? 'e' : 'f';
				break;
			default:
				break;
			}
			switch (retval) {
			case 'b': case 'e':
				if ((expv + 1) > FLT128_DIG)
				{
					int index = FLT128_DIG;
					switch (s[index+offset]) {
					case '9':
						s[index+offset] = '\0';
						for (; (index+offset-1) >= offset; index--)
						{
							if (s[index+offset-1] == '9') 
							{
								s[index+offset-1] = '\0';
								if ((index+offset-1) == offset)
								{
									s[index+offset-1] = '\0'; offset--; s[index+offset-1] = '1';
									expv++;
									break;
								}
							}
							else
							{
								s[index+offset-1] += 1;
								break;
							}
						}
						break;
					default:
						s[offset+index] = '\0';
						index--;
						
						if (s[offset+index] == '0')
						{
							for (; (index+offset-1) >= offset; index--)
							{
								if (s[index+offset-1] == '0')
									s[index+offset-1] = '\0';
								else
									break;
							}
						}
						break;
					}
					switch (retval) {
					case 'b':

						if (offset == RADIX2_OFFSET)
						{
							s[0] = '0'; s[1] = '.'; offset = 0;
						}
						else
						{
							s[2] = s[1]; s[0] = '0'; s[1] = '.'; offset = 0;
						}
						break;
					case 'e':
						if (offset == RADIX10_OFFSET)
						{
							s[0] = s[1]; s[1] = '.'; offset = 0;
							if (s[2] == 0)  s[2] = '0';
						}
						else
						{
							s[1] = '.'; s[2] = '0';
						}
						break;
					default:
						break;
					}
				}
				else
				{
					for (; pos > 0; pos--)
					{
						s[pos+offset] = s[pos+offset-1]; s[pos+offset-1] = '.';
					}
					pos = strlen(s+offset);
					if (pos > FLT128_DIG+3)
					{
						s[offset+FLT128_DIG+3] = '\0'; pos = FLT128_DIG+3; 
					}
					switch (s[offset+pos-2]) {
					case '9':

						for (; s[offset+pos-1] != '.'; pos--)
						{
							if (s[offset+pos-1] == '9') 
							{
								s[offset+pos-1] = '\0';
								if ((offset+pos-1) == offset)
								{
									s[offset+pos-1] = '1';
									expv++;
								}
							}
							else
							{
								s[offset+pos-1] += 1;
								break;
							}
						}
						break;
					case '0':
						for (; pos != 0; pos--)
						{
							if (s[offset+pos-1] == '0')
								s[offset+pos-1] = '\0';
							else
								break;
						}
						break;
					default:
						break;
					}
					switch (retval) {
					case 'b':
						offset--; s[offset] = '0';
						break;
					case 'e':
						s[offset] = s[offset+1]; s[offset+1] = '.';
						if (s[offset+2] == 0)  s[offset+2] = '0'; 
						break;
					default:
						break;
					}
				}
				break;
			case 'f':
				int intdigit = expv + 1, fradigit = FLT128_DIG - intdigit; 
				if (fradigit < 0)  fradigit = 0;
				if ((expv + 1) > FLT128_DIG)
				{
					for (volatile int i = strlen(s+offset); (intdigit + 2) < i; i--)
					{
						if   (s[i] != '0')  break;
						else  s[i] = '\0';
					}
				}
				else
				{
					const int fra_pos = intdigit + 1;
					int s_fra_size = strlen(s+offset) - intdigit - 1;
					if (s_fra_size < fradigit)
					{
						for (; s_fra_size < fradigit; s_fra_size++)
						{
							s[offset+fra_pos+s_fra_size] = '0';
						}
					}
					if (s[offset+fra_pos+fradigit] == '9')
					{
						s[offset+fra_pos+fradigit] = '\0';
						for (int i = 0; i < fradigit; i++)
						{
							char *ptr = (s + offset + fra_pos + fradigit - i - 1);
							if (*ptr == '9')
								*ptr = 0;
							else
							{
								*ptr += 1;
								break;
							}
						}
						if (s[offset+fra_pos] == 0)
						{
							s[offset+fra_pos] = '0'; s[offset+fra_pos+1] = '\0'; 
							for (int i = 0; i < intdigit; i++)
							{
								char *ptr = (s + offset + intdigit - i - 1);
								if (*ptr == '9')
									*ptr = '0';
								else
								{
									*ptr += 1;
									break;
								}
							}
							if (s[offset] == '0')
							{
								offset--; s[offset] = '1';
								expv++;
							}
						}
					}
					else
					{
						s[offset+fra_pos+fradigit] = '\0';
						if (s[offset+fra_pos+fradigit-1] == '0')
						{
							for (int i = 0; i < (fradigit - 1); i++)
							{
								char *ptr = (s + offset + fra_pos + fradigit - i - 1);
								if (*ptr == '0')  *ptr = 0;
								else  break;
							}
							if (s[offset+fra_pos] == 0)  { s[offset+fra_pos] = '0'; s[offset+fra_pos+1] = '\0'; }
						}
					}
				}
			default:
				break;
			}
			*buf = s+offset;
		}
		else if (absx < 10 && absx > 1)
		{
			int denormal_first_digit = 0;
			expv = 0;
			if (retval == 'g')
			{
				retval = 'f';
			}
			quadmath_snprintf(s+offset, BUF_SIZE-offset, "%.*Qf", FLT128_DIG, absx / 10);
			denormal_first_digit = strlen(s+offset) - 1;

			if (s[offset+denormal_first_digit] == 9)
			{
				s[offset+denormal_first_digit] = '\0';
				for (int i = 0; i < (denormal_first_digit - 1); i++)
				{
					char *ptr = (s + offset + denormal_first_digit - i - 1);
					if (*ptr == '9')
						*ptr = 0;
					else if (*ptr == '.')
					{
						ptr[1] = '1';
						expv++;
						break;
					}
					else
					{
						*ptr += 1;
						break;
					}
				}
			}
			else
			{
				s[offset+denormal_first_digit] = '\0';

				if (s[offset+denormal_first_digit-1] == '0')
				{
					for (int i = 0; i < (denormal_first_digit - 2); i++)
					{
						char *ptr = (s + offset + denormal_first_digit - i - 2);
						if (*ptr == '0')  *ptr = 0;
						else  break;
					}
				}
			}
			switch (retval) {
			case 'b':
				expv++;
				break;
			case 'e': case 'f':
				if (expv != 0)
				{
					char *ptr = s+offset;
					
					if (retval == 'e')
					{
						*ptr++ = '1'; *ptr++ = '.'; *ptr++ = '0'; *ptr++ = 0;
					}
					else
					{
						*ptr++ = '1'; *ptr++ = '0'; *ptr++ = '.'; *ptr++ = '0'; *ptr++ = 0;
					}
				}
				else
				{
					s[offset] = '\0'; offset++;
					s[offset] = s[offset+1];
					s[offset+1] = '.';
					if (s[offset+2] == 0)  s[offset+2] = '0';
				}
				break;
			default:
				break;
			}
			*buf = s+offset;
		}
		else if (absx == 1)
		{
			if (retval == 'g')
			{
				retval = 'f';
			}
			switch (retval) {
			case 'b': /* radix == 2  */
				s[0] = '0'; s[1] = '.'; s[2] = '1'; s[3] = '\0';
				expv = 1; 
				break;
			case 'e': case 'f': /* radix == 10 */
				s[0] = '1'; s[1] = '.'; s[2] = '0'; s[3] = '\0';
				expv = 0;
				break;
			default:
				break;
			}
			*buf = s;
		}
		else if (absx >= 0.1)
		{
			int len = quadmath_snprintf(s+offset, BUF_SIZE-offset, "%.*Qf", FLT128_DIG, absx);
			if (retval == 'g')  retval = 'f';
			
			if (s[offset] == '1')
				strcpy(s+offset, near_one);
			else
			{
				if (s[offset+len-1] == '0')
				{
					for (int i = 0; i < (len - 2); i++)
					{
						if (s[offset+len-i-1] == '0')
							s[offset+len-i-1] = '\0';
						else
							break;
					}
				}
			}
			
			if (retval == 'e')
			{
				s[offset] = '\0'; s[offset+1] = s[offset+2]; s[offset+2] = '.';
				offset++;
				expv--;
			}
			
			*buf = s+offset;
		}
		else /* if (absx < 0.1) */
		{
			__float128 w = absx;
			int len = 0;
			if (retval == 'g')
			{
				retval =  (w <= FRAC_DIG) ? 'e' : 'f';
			}
			switch (retval) {
			case 'b':
				for (expv = 0; w < 0.1; w = foo(w * 10))  expv++;
				expv = -expv;
				len = quadmath_snprintf(s+offset, BUF_SIZE-offset, "%.*Qf", FLT128_DIG, w);
				if (s[offset] != '0')
				{
					s[offset+len-1] = '\0';
					s[offset+1] = s[offset]; s[offset] = '.';
					offset--;
					s[offset] = '0';
					expv--;
				}
				switch (s[offset+len-1]) {
				case '0':
					for (int i = 0; i < (len - 2); i++)
					{
						if (s[offset+len-i-1] == '0')
							s[offset+len-i-1] = '\0';
						else
							break;
					}
					break;
				default:
					break;
				}
				
				break;
			case 'e':

				for (expv = 0; w < 0.1; w = foo(w * 10))  expv++;
				expv = -expv;
				expv--;
				len = quadmath_snprintf(s+offset, BUF_SIZE-offset, "%.*Qf", FLT128_DIG, w);
				if (s[offset] != '0')
				{
					s[offset+len-1] = '\0';
					s[offset+1] = s[offset]; s[offset] = '.';
					offset--;
					s[offset] = '0';
					expv++;
				}
				switch (s[offset+len-1]) {
				case '0':
					for (int i = 0; i < (len - 2); i++)
					{
						if (s[offset+len-i-1] == '0')
							s[offset+len-i-1] = '\0';
						else
							break;
					}
					break;
				default:
					break;
				}
				
				s[offset] = '\0'; offset++; s[offset] = s[offset+1]; s[offset+1] = '.';
				if (s[offset+2] == '\0')  s[offset+2] = '0';
				
				break;
			case 'f':
				for (expv = 0; w < 1; w = foo(w * 10))  expv++;
				expv--;
				len = quadmath_snprintf(s+offset, BUF_SIZE-offset, "%.*Qf", expv+FLT128_DIG+1, absx);
				expv = -expv;
				if (s[offset+len-1] == '9')
				{
					s[offset+len-1] = '\0';
					len--;
					for (int i = 0; i <= FLT128_DIG; i++)
					{
						if (s[offset+len-i-1] == '9')
						{
							s[offset+len-i-1] = '\0';
						}
						else
						{
							s[offset+len-i-1] += 1;
							break;
						}
					}
				}
				else
				{
					s[offset+len-1] = '\0';
					len--;
					if (s[offset+len-1] == '0')
					{
						for (int i = 0; i <= FLT128_DIG; i++)
						{
							if (s[offset+len-i-1] == '0')
							{
								s[offset+len-i-1] = '\0';
							}
							else
							{
								break;
							}
						}
					}
					break;
				}
			default:
				break;
			}
			*buf = s+offset;
		}
	}
end:
	*exp = expv;
	*sign = signv;
	return retval;
}

#ifdef TEST
int 
main(int argc, char const *argv[])
{
	char* str;
	__float128 x;
	int exp, sign;
	char format;
	if (argc != 3)
	{
		fprintf(stderr, "Usage: %s [format (b,e,f,g)] [float-number]\n", argv[0]);
		exit(1);
	}
	format = argv[1][0];
	x = strtoflt128 (argv[2], NULL);
	
	switch (ool_quad2str(x, format, &exp, &sign, &str)) {
	case '0':
		break;
	case '1':
		if (sign == -1)  printf("-%s\n", str);
		else printf("%s\n", str);
		break;
	case 'a':
		if (sign == -1)  printf("-%sp%+d\n", str, exp);
		else printf("%sp%+d\n", str, exp);
		break;
	case 'b':
		if (sign == -1)  printf("-%se%+d\n", str, exp);
		else printf("%se%+d\n", str, exp);
		break;
	case 'e':
		if (sign == -1)  printf("-%se%+d\n", str, exp);
		else printf("%se%+d\n", str, exp);
		break;
	case 'f':
		if (sign == -1)  printf("-%s\n", str);
		else printf("%s\n", str);
		break;
	default:
		break;
	}

	return 0;
}
#endif
