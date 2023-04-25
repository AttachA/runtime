#pragma once
extern "C" {
	//functions starting with symbol '#' is constructors
	//functions starting with symbol '\1' is special functions that used by compilers and should not called by user
	//symbol '#' in names represent multiple constructors ex '# net ip#v6' is constructor for 
	//L>   '# net ip' that recuive only ip6 address in string, btw what contains afetr '#' is not important, that can be just numbers

	void initStandardLib();//init all,except CMath
	void initStandardLib_safe();//init all,except CMath and internal
	void initCMathLib();
	void initStandardLib_console();
	void initStandardLib_math();
	void initStandardLib_file();
	void initStandardLib_paralel();
	void initStandardLib_chanel();
	void initStandardLib_internal_memory();
	void initStandardLib_internal_run_time();
	void initStandardLib_internal_run_time_native();
	void initStandardLib_internal_stack();
	void initStandardLib_net();
}
