// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

namespace art {
	//functions starting with symbol '#' is constructors
	//functions starting with symbol '\1' is special functions that used by compilers and should not called by user
	//symbol '#' in names represent multiple constructors ex '# net ip#v6' is constructor for 
	//L>   '# net ip' that recuive only ip6 address in string, btw what contains afetr '#' is not important, that can be just numbers

	void initStandardLib();//init all,except CMath and debug
	void initStandardLib_safe();//init all,except CMath, internal, debug and start_debug

	void initCMathLib();
	void initStandardLib_exception();
	void initStandardLib_bytes();
	void initStandardLib_console();
	void initStandardLib_math();
	void initStandardLib_file();
	void initStandardLib_parallel();
	void initStandardLib_chanel();
	void initStandardLib_internal();
	void initStandardLib_internal_memory();
	void initStandardLib_internal_run_time();
	void initStandardLib_internal_run_time_native();
	void initStandardLib_internal_stack();
	void initStandardLib_net();
	void initStandardLib_debug();//debug tools
	void initStandardLib_start_debug();//allow enable debug tools

}
