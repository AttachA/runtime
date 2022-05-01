#pragma once
#include <vector>
#include <string>
namespace run_time {
	template<class T>
	inline T readData(const std::vector<uint8_t>& data, size_t data_len, size_t& i) {
		if (data_len < i + sizeof(T))
			throw InvalidFunction("Function is not full cause in pos " + std::to_string(i) + " try read " + std::to_string(sizeof(T)) + " bytes, but function length is " + std::to_string(data_len) + ", fail compile function");
		uint8_t res[sizeof(T)]{ 0 };
		for (size_t j = 0; j < sizeof(T); j++)
			res[j] = data[i++];
		return *(T*)res;
	}
	inline std::string readString(const std::vector<uint8_t>& data, size_t data_len, size_t& i) {
		uint32_t value_len = readData<uint32_t>(data, data_len, i);
		std::string res;
		for (uint32_t j = 0; j < value_len; j++)
			res += readData<char>(data, data_len, i);
		return res;
	}
	template<class T>
	inline T* readRawArray(const std::vector<uint8_t>& data, size_t data_len, size_t& i,size_t len) {
		T* res = new T[len];
		for (uint32_t j = 0; j < len; j++)
			res[i] = readData<T>(data, data_len, i);
		return res;
	}
	inline uint32_t readLen(const std::vector<uint8_t>& data, size_t data_len, size_t& i) {
		return readData<uint32_t>(data, data_len, i);
	}
}