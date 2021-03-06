// declare scu test function
extern int cdk_scu_test(int autotest);
extern int sq8000_scu_test(int autotest);

extern int
scu_test(int autotest)
{
	int (*test_func)(int);

#if defined (CONFIG_CDK) || (CONFIG_PC9002) || (CONFIG_SCDK)
	test_func = cdk_scu_test;
#elif defined (CONFIG_SQ8000)
	test_func = sq8000_scu_test;
#else
#error "SCU test is not well defined!"
#endif

	return test_func(autotest);
}

