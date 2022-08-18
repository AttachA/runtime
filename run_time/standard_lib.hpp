#pragma once
extern "C" {
	//functions starting with symbol '#' is constructors
	//functions starting with symbol '\1' is special functions that used by compilers and should not called by user
	void initStandardFunctions();
	void initCMathLib();
}