// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#ifndef SRC_RUN_TIME_UTIL_TOOLS
#define SRC_RUN_TIME_UTIL_TOOLS
#include <vector>
#include <string>
#include <util/exceptions.hpp>
#include <run_time/attacha_abi_structs.hpp>
namespace art {
	namespace reader{
		template<class T>
		inline typename std::enable_if<!std::is_same_v<T,ValueIndexPos>, T>::type readData(const std::vector<uint8_t>& data, size_t data_len, size_t& i) {
			if (data_len < i + sizeof(T))
				throw InvalidFunction("Function is not full cause in pos " + std::to_string(i) + " try read " + std::to_string(sizeof(T)) + " bytes, but function length is " + std::to_string(data_len) + ", fail compile function");
			uint8_t res[sizeof(T)]{ 0 };
			for (size_t j = 0; j < sizeof(T); j++)
				res[j] = data[i++];
			return *(T*)res;
		}
		inline art::ustring readString(const std::vector<uint8_t>& data, size_t data_len, size_t& i) {
			uint32_t value_len = readData<uint32_t>(data, data_len, i);
			art::ustring res;
			for (uint32_t j = 0; j < value_len; j++)
				res += readData<char>(data, data_len, i);
			return res;
		}
		template<class T>
		inline T* extractRawArray(const std::vector<uint8_t>& data, size_t data_len, size_t& i, size_t len) {
			T* result = (T*)(data.data() + i);
			if (data_len < i + len * sizeof(T))
				throw InvalidFunction("Function is not full cause in pos " + std::to_string(i) + " try read " + std::to_string(len * sizeof(T)) + " bytes, but function length is " + std::to_string(data_len) + ", fail compile function");
			i += len * sizeof(T);
			return result;
		}
		template<class T>
		inline T* readRawArray(const std::vector<uint8_t>& data, size_t data_len, size_t& i,size_t len) {
			T* res = new T[len];
			for (uint32_t j = 0; j < len; j++)
				res[j] = readData<T>(data, data_len, i);
			return res;
		}
		inline uint32_t readLen(const std::vector<uint8_t>& data, size_t data_len, size_t& i) {
			return readData<uint32_t>(data, data_len, i);
		}
		inline ValueItem readAny(const std::vector<uint8_t>& data, size_t data_len, size_t& i);
		inline ValueItem* readRawAny(const std::vector<uint8_t>& data, size_t data_len, size_t& i, uint32_t len) {
			ValueItem* res = new ValueItem[len];
			for (uint32_t j = 0; j < len; j++)
				res[j] = readAny(data, data_len, i);
			return res;
		}

		inline list_array<ValueItem> readAnyUarr(const std::vector<uint8_t>& data, size_t data_len, size_t& i) {
			uint64_t len = readData<uint64_t>(data, data_len, i);
			list_array<ValueItem> uarr;
			uarr.reserve_push_back(len);
			while (len--) {
				uarr.push_back(readAny(data, data_len, i));
			}
			return uarr;
		}

		inline ValueItem readAny(const std::vector<uint8_t>& data, size_t data_len, size_t& i) {
			ValueMeta meta = readData<ValueMeta>(data, data_len, i);
			meta.as_ref = false;
			ValueItem res;
			res.meta = meta;
			switch (meta.vtype) {
			case VType::noting:
				break;
			case VType::i8:
				res.val = (void*)(0ll+readData<int8_t>(data, data_len, i));
				break;
			case VType::i16:
				res.val = (void*)(0ll+readData<int16_t>(data, data_len, i));
				break;
			case VType::i32:
				res.val = (void*)(0ll+readData<int32_t>(data, data_len, i));
				break;
			case VType::i64:
				res.val = (void*)readData<int64_t>(data, data_len, i);
				break;
			case VType::ui8:
				res.val = (void*)(0ull+readData<uint8_t>(data, data_len, i));
				break;
			case VType::ui16:
				res.val = (void*)(0ull+readData<uint16_t>(data, data_len, i));
				break;
			case VType::ui32:
				res.val = (void*)(0ull+readData<uint32_t>(data, data_len, i));
				break;
			case VType::ui64:
			case VType::undefined_ptr:
			case VType::time_point:
			case VType::type_identifier:
				res.val = (void*)readData<uint64_t>(data, data_len, i);
				break;
			case VType::flo: {
				float r = readData<float>(data, data_len, i);
				res.val = *(void**)&r;
				break;
			}
			case VType::doub:{
				double r = readData<double>(data, data_len, i);
				res.val = *(void**)&r;
				break;
			}
			case VType::raw_arr_i8:
				res.val = readRawArray<int8_t>(data, data_len, i, readData<uint32_t>(data,data_len,i));
				break;
			case VType::raw_arr_i16:
				res.val = readRawArray<int16_t>(data, data_len, i, readData<uint32_t>(data, data_len, i));
				break;
			case VType::raw_arr_i32:
				res.val = readRawArray<int32_t>(data, data_len, i, readData<uint32_t>(data, data_len, i));
				break;
			case VType::raw_arr_i64:
				res.val = readRawArray<int64_t>(data, data_len, i, readData<uint32_t>(data, data_len, i));
				break;
			case VType::raw_arr_ui8:
				res.val = readRawArray<uint8_t>(data, data_len, i, readData<uint32_t>(data, data_len, i));
				break;
			case VType::raw_arr_ui16:
				res.val = readRawArray<uint16_t>(data, data_len, i, readData<uint32_t>(data, data_len, i));
				break;
			case VType::raw_arr_ui32:
				res.val = readRawArray<uint32_t>(data, data_len, i, readData<uint32_t>(data, data_len, i));
				break;
			case VType::raw_arr_ui64:
				res.val = readRawArray<uint64_t>(data, data_len, i, readData<uint32_t>(data, data_len, i));
				break;
			case VType::raw_arr_flo:
				res.val = readRawArray<float>(data, data_len, i, readData<uint32_t>(data, data_len, i));
				break;
			case VType::raw_arr_doub:
				res.val = readRawArray<double>(data, data_len, i, readData<uint32_t>(data, data_len, i));
				break;
			case VType::uarr:
				res.val = new list_array<ValueItem>(readAnyUarr(data, data_len, i));
				break;
			case VType::string:
				res.val = new art::ustring(readString(data, data_len, i));
				break;
			case VType::faarr:
				res.val = readRawAny(data, data_len, i, readLen(data, data_len, i));
				break;
			default:
				throw NotImplementedException();
			}
			if (meta.use_gc)
				res.val = new lgr(res.val);
			return res;
		}
		inline uint64_t readPackedLen(const std::vector<uint8_t>& data, size_t data_len, size_t& i){
			switch(readData<uint8_t>(data, data_len, i)){
				case 0: return 0;
				case 1: return readData<uint8_t>(data, data_len, i);
				case 2: return readData<uint16_t>(data, data_len, i);
				case 3: return readData<uint32_t>(data, data_len, i);
				case 4: return readData<uint64_t>(data, data_len, i);
				default: throw NotImplementedException();
			}
		}


		inline ValueIndexPos readIndexPos(const std::vector<uint8_t>& data, size_t data_len, size_t& i) {
			ValueIndexPos res;
			res.index = readData<uint16_t>(data, data_len, i);
			res.pos = readData<ValuePos>(data, data_len, i);
			return res;
		}
	}


	namespace builder {
		template<class T>
		typename std::enable_if<!std::is_same_v<T,ValueIndexPos>, void>::type
		write(std::vector<uint8_t>& data, const T& v) {
			const uint8_t* res = reinterpret_cast<const uint8_t*>(&v);
			for (size_t i = 0; i < sizeof(T); i++)
				data.push_back(res[i]);
		}
		template<class T>
		void writeRaw(std::vector<uint8_t>& data, const T& v,uint32_t len) {
			write(data, len);
			for (size_t i = 0; i < len; i++)
				write(data,v[i]);
		}
		inline void writeString(std::vector<uint8_t>& data, const art::ustring& str) {
			if (str.size() > (size_t)UINT32_MAX)
				throw NumericOverflowException();
			write(data, (uint32_t)str.size());
			data.insert(data.end(), str.begin(), str.end());
		}
		inline void writeAny(std::vector<uint8_t>& data, ValueItem& it);
		inline void writeAnyUarr(std::vector<uint8_t>& data, list_array<ValueItem>& v) {
			write(data, (uint64_t)v.size());
			for (auto& it : v)
				writeAny(data, it);
		}
		inline void writeAnyRaw(std::vector<uint8_t>& data, ValueItem* v,uint32_t len) {
			write(data, len);
			for (size_t i = 0; i < len; i++)
				writeAny(data, v[i]);
		}
		//return gc mode
		inline void writeAny(std::vector<uint8_t>& data, ValueItem& it) {
			it.getAsync();
			write(data, it.meta.encoded);
			switch (it.meta.vtype) {
			case VType::noting:
				break;
			case VType::ui8:
			case VType::i8:
				write(data, (uint8_t)it);
				break;
			case VType::ui16:
			case VType::i16:
				write(data, (uint16_t)it);
				break;
			case VType::ui32:
			case VType::i32:
			case VType::flo:
				write(data, (uint32_t)it);
				break;
			case VType::ui64:
			case VType::i64:
			case VType::undefined_ptr:
			case VType::time_point:
			case VType::type_identifier:
			case VType::doub:
				write(data, (uint64_t)it);
				break;
			case VType::raw_arr_ui8:
			case VType::raw_arr_i8:
				writeRaw(data, (uint8_t*)it.getSourcePtr(), it.meta.val_len);
				break;
			case VType::raw_arr_ui16:
			case VType::raw_arr_i16:
				writeRaw(data, (uint16_t*)it.getSourcePtr(), it.meta.val_len);
				break;
			case VType::raw_arr_ui32:
			case VType::raw_arr_i32:
			case VType::raw_arr_flo:
				writeRaw(data, (uint32_t*)it.getSourcePtr(), it.meta.val_len);
				break;
			case VType::raw_arr_ui64:
			case VType::raw_arr_i64:
			case VType::raw_arr_doub:
				writeRaw(data, (uint64_t*)it.getSourcePtr(), it.meta.val_len);
				break;
			case VType::uarr:
				writeAnyUarr(data, *(list_array<ValueItem>*)it.getSourcePtr());
				break;
			case VType::string:
				writeString(data, *(art::ustring*)it.getSourcePtr());
				break;
			case VType::faarr:
				writeAnyRaw(data, (ValueItem*)it.val, it.meta.val_len);
				break;

			default:
				throw NotImplementedException();
			}
		}
		inline void writePackedLen(std::vector<uint8_t>& data, uint64_t len){
			if (len == 0) 
				builder::write(data, (uint8_t)0);
			else if (len <= UINT8_MAX) {
				builder::write(data, (uint8_t)1);
				builder::write(data, (uint8_t)len);
			}
			else if (len <= UINT16_MAX) {
				builder::write(data, (uint8_t)2);
				builder::write(data, (uint16_t)len);
			}
			else if (len <= UINT32_MAX) {
				builder::write(data, (uint8_t)4);
				builder::write(data, (uint32_t)len);
			}
			else {
				builder::write(data, (uint8_t)8);
				builder::write(data, len);
			}
		}


		inline void writeIndexPos(std::vector<uint8_t>& data, ValueIndexPos ipos) {
			write(data, ipos.index);
			write(data, ipos.pos);
		}
	}
	namespace internal{
		template<typename T>
		T readFromArrayAsValue(uint8_t*& arr) {
			T& val = *(T*)arr;
			arr += sizeof(T);
			return val;
		}
		template<typename T>
		T* readFromArrayAsArray(uint8_t*& arr, size_t& size) {
			size = readFromArrayAsValue<size_t>(arr);
			T* ret = new T[size];
			for (size_t i = 0; i < size; i++)
				ret[i] = readFromArrayAsValue<T>(arr);
			return ret;
		}
		template<typename T>
		void skipArray(uint8_t*& arr) {
			size_t size = readFromArrayAsValue<size_t>(arr);
			arr += sizeof(T) * size;
		}
	}
}
#endif /* SRC_RUN_TIME_UTIL_TOOLS */
