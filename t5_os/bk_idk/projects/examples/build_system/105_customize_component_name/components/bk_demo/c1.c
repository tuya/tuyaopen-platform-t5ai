#include <stdio.h>

#if CONFIG_FEATUR1_EABLE
int c1(void)
{
	int i = 123;

	#ifdef CONFIG_FEATUR2_COUNT
	i = CONFIG_FEATUR2_COUNT;
	#endif

	printf("hello component c1\n");
	printf("CONFIG_DEMO_FEATURE1 is enable.\n");
	printf("CONFIG_FEATUR2_COUNT is %d.\n", i);
	return 0;
}

int c2(void)
{
	int i = 789;

	#ifdef CONFIG_FEATUR2_COUNT
	i = CONFIG_FEATUR2_COUNT;
	#endif

	printf("hello component c2\n");
	printf("CONFIG_DEMO_FEATURE1 is enable.\n");
	printf("CONFIG_FEATUR2_COUNT is %d.\n", i);
	printf("hello component c2\n");
	printf("CONFIG_DEMO_FEATURE1 is enable.\n");
	printf("CONFIG_FEATUR2_COUNT is %d.\n", i);
	return 0;
}
#else

int c1(void)
{
	int i = 456;

	#ifdef CONFIG_FEATUR2_COUNT
	i = CONFIG_FEATUR2_COUNT;
	#endif

	printf("hello component c1\n");
	printf("CONFIG_DEMO_FEATURE1 is disable.\n");
	printf("CONFIG_FEATUR2_COUNT is %d.\n", i);
	return 0;
}
#endif
