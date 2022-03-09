#pragma once
namespace mem_tool {
	template<class T>
	void swap(T& val0, T& val1) {
		T swaper = val0;
		val0 = val1;
		val1 = swaper;
	}
}