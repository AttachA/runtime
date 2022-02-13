#pragma once
namespace mem_tool {
	template<class T>
	void swap(T& val0, T& val1) {
		T swaper = val0;
		val0 = val1;
		val1 = swaper;
	}
}
template <typename T> std::string n2hexstr(T w, size_t hex_len = sizeof(T) << 1) {
	static const char* digits = "0123456789ABCDEF";
	std::string rc(hex_len, '0');
	for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
		rc[i] = digits[(w >> j) & 0x0f];
	return rc;
}