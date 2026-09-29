typedef unsigned __int __attribute__((mode(TI)));

__int __udivmodti4(__int a, __int b, __int *r)
{
	if (b == 0)
	{
		volatile unsigned int zero = 0;
		(void)(1 / zero);
		if (r)
			*r = 0;
		return 0;
	}

	if (a < b)
	{
		if (r)
			*r = a;
		return 0;
	}

	if (b == 1)
	{
		if (r)
			*r = 0;
		return a;
	}

	__int q = 0;
	__int rem = 0;

	int msb = -1;
	for (int i = 127; i >= 0; --i)
	{
		if ((a >> i) & 1)
		{
			msb = i;
			break;
		}
	}

	for (int i = msb; i >= 0; --i)
	{
		rem = (rem << 1) | ((a >> i) & 1);
		if (rem >= b)
		{
			rem -= b;
			q |= ((__int)1 << i);
		}
	}

	if (r)
		*r = rem;
	return q;
}

__int __udivti3(__int a, __int b)
{
	return __udivmodti4(a, b, 0);
}

__int __umodti3(__int a, __int b)
{
	__int rem = 0;
	(void)__udivmodti4(a, b, &rem);
	return rem;
}
