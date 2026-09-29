#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wfloat-equal"

/* Source: https://github.com/glitchub/arith64 */
#define arith64_u64 unsigned long long int
#define arith64_s64 signed long long int
#define arith64_u32 unsigned int
#define arith64_s32 int

typedef union
{
	arith64_u64 u64;
	arith64_s64 s64;
	struct
	{
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		arith64_u32 hi;
		arith64_u32 lo;
#else
		arith64_u32 lo;
		arith64_u32 hi;
#endif
	} u32;
	struct
	{
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		arith64_s32 hi;
		arith64_s32 lo;
#else
		arith64_s32 lo;
		arith64_s32 hi;
#endif
	} s32;
} arith64_word;

#define arith64_hi(n) (arith64_word){.u64 = n}.u32.hi
#define arith64_lo(n) (arith64_word){.u64 = n}.u32.lo
#define arith64_neg(a, b) (((a) ^ ((((arith64_s64)(b)) >= 0) - 1)) + (((arith64_s64)(b)) < 0))
#define arith64_abs(a) arith64_neg(a, a)

arith64_s64 __absvdi2(arith64_s64 a)
{
	return arith64_abs(a);
}

arith64_s64 __ashldi3(arith64_s64 a, int b)
{
	arith64_word w = {.s64 = a};

	b &= 63;

	if (b >= 32)
	{
		w.u32.hi = w.u32.lo << (b - 32);
		w.u32.lo = 0;
	}
	else if (b)
	{
		w.u32.hi = (w.u32.lo >> (32 - b)) | (w.u32.hi << b);
		w.u32.lo <<= b;
	}
	return w.s64;
}

arith64_s64 __ashrdi3(arith64_s64 a, int b)
{
	arith64_word w = {.s64 = a};

	b &= 63;

	if (b >= 32)
	{
		w.s32.lo = w.s32.hi >> (b - 32);
		w.s32.hi >>= 31; // 0xFFFFFFFF or 0
	}
	else if (b)
	{
		w.u32.lo = (w.u32.hi << (32 - b)) | (w.u32.lo >> b);
		w.s32.hi >>= b;
	}
	return w.s64;
}

int __clzsi2(arith64_u32 a)
{
	int b, n = 0;
	b = !(a & 0xffff0000) << 4;
	n += b;
	a <<= b;
	b = !(a & 0xff000000) << 3;
	n += b;
	a <<= b;
	b = !(a & 0xf0000000) << 2;
	n += b;
	a <<= b;
	b = !(a & 0xc0000000) << 1;
	n += b;
	a <<= b;
	return n + !(a & 0x80000000);
}

int __clzdi2(arith64_u64 a)
{
	int b, n = 0;
	b = !(a & 0xffffffff00000000ULL) << 5;
	n += b;
	a <<= b;
	b = !(a & 0xffff000000000000ULL) << 4;
	n += b;
	a <<= b;
	b = !(a & 0xff00000000000000ULL) << 3;
	n += b;
	a <<= b;
	b = !(a & 0xf000000000000000ULL) << 2;
	n += b;
	a <<= b;
	b = !(a & 0xc000000000000000ULL) << 1;
	n += b;
	a <<= b;
	return n + !(a & 0x8000000000000000ULL);
}

int __ctzsi2(arith64_u32 a)
{
	int b, n = 0;
	b = !(a & 0x0000ffff) << 4;
	n += b;
	a >>= b;
	b = !(a & 0x000000ff) << 3;
	n += b;
	a >>= b;
	b = !(a & 0x0000000f) << 2;
	n += b;
	a >>= b;
	b = !(a & 0x00000003) << 1;
	n += b;
	a >>= b;
	return n + !(a & 0x00000001);
}

int __ctzdi2(arith64_u64 a)
{
	int b, n = 0;
	b = !(a & 0x00000000ffffffffULL) << 5;
	n += b;
	a >>= b;
	b = !(a & 0x000000000000ffffULL) << 4;
	n += b;
	a >>= b;
	b = !(a & 0x00000000000000ffULL) << 3;
	n += b;
	a >>= b;
	b = !(a & 0x000000000000000fULL) << 2;
	n += b;
	a >>= b;
	b = !(a & 0x0000000000000003ULL) << 1;
	n += b;
	a >>= b;
	return n + !(a & 0x0000000000000001ULL);
}

arith64_u64 __divmoddi4(arith64_u64 a, arith64_u64 b, arith64_u64 *c)
{
	if (b > a)
	{
		if (c)
			*c = a;
		return 0;
	}
	if (!arith64_hi(b))
	{
		if (b == 0)
		{
			volatile char x = 0;
			x = 1 / x;
		}
		if (b == 1)
		{
			if (c)
				*c = 0;
			return a;
		}
		if (!arith64_hi(a))
		{
			if (c)
				*c = arith64_lo(a) % arith64_lo(b);
			return arith64_lo(a) / arith64_lo(b);
		}
	}

	char bits = __clzdi2(b) - __clzdi2(a) + 1;
	arith64_u64 rem = a >> bits;
	a <<= 64 - bits;
	arith64_u64 wrap = 0;
	while (bits-- > 0)
	{
		rem = (rem << 1) | (a >> 63);
		a = (a << 1) | (wrap & 1);
		wrap = ((arith64_s64)(b - rem - 1) >> 63);
		rem -= b & wrap;
	}
	if (c)
		*c = rem;
	return (a << 1) | (wrap & 1);
}

arith64_s64 __divdi3(arith64_s64 a, arith64_s64 b)
{
	arith64_u64 q = __divmoddi4(arith64_abs(a), arith64_abs(b), (void *)0);
	return arith64_neg(q, a ^ b);
}

int __ffsdi2(arith64_u64 a) { return a ? __ctzdi2(a) + 1 : 0; }

arith64_u64 __lshrdi3(arith64_u64 a, int b)
{
	arith64_word w = {.u64 = a};

	b &= 63;

	if (b >= 32)
	{
		w.u32.lo = w.u32.hi >> (b - 32);
		w.u32.hi = 0;
	}
	else if (b)
	{
		w.u32.lo = (w.u32.hi << (32 - b)) | (w.u32.lo >> b);
		w.u32.hi >>= b;
	}
	return w.u64;
}

arith64_s64 __moddi3(arith64_s64 a, arith64_s64 b)
{
	arith64_u64 r;
	__divmoddi4(arith64_abs(a), arith64_abs(b), &r);
	return arith64_neg(r, a);
}

int __popcountsi2(arith64_u32 a)
{

	a = a - ((a >> 1) & 0x55555555);
	a = ((a >> 2) & 0x33333333) + (a & 0x33333333);
	a = (a + (a >> 4)) & 0x0F0F0F0F;
	a = (a + (a >> 16));

	return (a + (a >> 8)) & 63;
}

int __popcountdi2(arith64_u64 a)
{

	a = a - ((a >> 1) & 0x5555555555555555ULL);
	a = ((a >> 2) & 0x3333333333333333ULL) + (a & 0x3333333333333333ULL);
	a = (a + (a >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
	a = (a + (a >> 32));
	a = (a + (a >> 16));

	return (a + (a >> 8)) & 127;
}

arith64_u64 __udivdi3(arith64_u64 a, arith64_u64 b) { return __divmoddi4(a, b, (void *)0); }

arith64_u64 __umoddi3(arith64_u64 a, arith64_u64 b)
{
	arith64_u64 r;
	__divmoddi4(a, b, &r);
	return r;
}

/* Good documentation: https://splichal.eu/scripts/sphinx/gccint/_build/html/the-gcc-low-level-runtime-library/routines-for-floating-point-emulation.html */

double __adddf3(double a, double b) { return a + b; }
double __muldf3(double a, double b) { return a * b; }
double __floatsidf(int i) { return (double)i; }
int __ltdf2(double a, double b) { return a < b; }
int __gtdf2(double a, double b) { return a > b; }
int __nedf2(double a, double b) { return a != b; }
int __eqdf2(double a, double b) { return a == b; }
double __floatdidf(long i) { return (double)i; }
double __divdf3(double a, double b) { return a / b; }
double __subdf3(double a, double b) { return a - b; }
int __gedf2(double a, double b) { return a >= b; }
int __fixdfsi(double a) { return (int)a; }
long __fixdfdi(double a) { return (long)a; }
int __ledf2(double a, double b) { return a <= b; }

/* FIXME: Check if these functions are implemented correctly */

typedef long long int64_t;
typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
typedef struct
{
	uint64_t value;
} atomic_uint64_t;

/* No longer needed? */

// uint64_t __atomic_load_8(const atomic_uint64_t *p)
// {
// 	uint64_t value;
// 	__asm__ volatile("lock cmpxchg8b %1"
// 					 : "=A"(value)
// 					 : "m"(*p)
// 					 : "memory");
// 	return value;
// }

// void __atomic_store_8(atomic_uint64_t *p, uint64_t value)
// {
// 	__asm__ volatile("lock cmpxchg8b %0"
// 					 : "=m"(p->value)
// 					 : "a"((uint32_t)value), "d"((uint32_t)(value >> 32)), "m"(*p)
// 					 : "memory");
// }

int __fixsfsi(float a)
{
	return (int)a;
}

int __ltsf2(float a, float b)
{
	return -(a < b);
}

int __nesf2(float a, float b)
{
	return a == b;
}

int __eqsf2(float a, float b)
{
	return !(a == b);
}

float __divsf3(float a, float b)
{
	return (a / b);
}

double __extendsfdf2(float a)
{
	return (double)a;
}

float __truncdfsf2(double a)
{
	return (float)a;
}

float __subsf3(float a, float b)
{
	return (a - b);
}

float __floatsisf(int a)
{
	return (float)a;
}

int __fixunssfsi(float a)
{
	return (int)a;
}

float __mulsf3(float a, float b)
{
	return (a * b);
}

float __addsf3(float a, float b)
{
	return (a + b);
}
