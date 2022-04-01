#pragma once
namespace mem_tool {
	template<class T>
	void swap(T& val0, T& val1) {
		T swaper = val0;
		val0 = val1;
		val1 = swaper;
	}
	template<class T>
	struct ArrDeleter {
		T* del;
		ArrDeleter(T* ptr) { del = ptr; };
		~ArrDeleter() {
			delete[] del;
		}
	};
	template<class T>
	struct ValDeleter {
		T* del;
		ValDeleter(T* ptr) { del = ptr; };
		~ValDeleter() {
			delete del;
		}
	};
}