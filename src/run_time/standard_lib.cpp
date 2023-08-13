// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "library/bytes.hpp"
#include "library/console.hpp"
#include "library/parallel.hpp"
#include "library/chanel.hpp"
#include "library/internal.hpp"
#include "library/net.hpp"
#include "library/file.hpp"
#include "library/exceptions.hpp"
#include "AttachA_CXX.hpp"
#include "attacha_abi_structs.hpp"
#include "exceptions.hpp"
#include <cmath>
#include <math.h>
namespace art{
#pragma region math
#pragma region math_abs
	ValueItem math_abs_impl(ValueItem& val) {
		if (val.meta.vtype == VType::async_res)
			val.getAsync();
		switch (val.meta.vtype) {
		case VType::i8:
			return ValueItem((void*)(0ll + std::abs(int8_t(val))), val.meta);
		case VType::i16:
			return ValueItem((void*)(0ll + std::abs(int16_t(val))), val.meta);
		case VType::i32:
			return ValueItem((void*)(0ll + std::abs(int32_t(val))), val.meta);
		case VType::i64:
			return ValueItem((void*)std::abs(int64_t(val)), val.meta);
		case VType::flo: {
			union {
				float abs;
				void* res = nullptr;
			};
			abs = std::abs((float)val);
			return ValueItem(res, val.meta);
			
		}
		case VType::doub: {
			union {
				double abs;
				void* res = nullptr;
			};
			abs = +((double)val);
			return ValueItem(res, val.meta);
		}
		default:
			return val;
		}
	}
	template<VType type, class T>
	ValueItem* math_abs(T* args, uint32_t args_len) {
		T* res = new T[args_len];
		for (size_t i = 1; i < args_len; i++) {
			if constexpr (std::is_same_v<T, ValueItem>)
				res[i] = math_abs_impl(args[i]);
			else if constexpr (std::is_same_v<T, int64_t>)
				res[i] = std::abs(args[i]);
			else if constexpr (std::is_same_v<T, uint64_t>)
				res[i] = (T)std::abs((int64_t)args[i]);
			else
				res[i] = (T)std::abs((double)args[i]);
		}
		return new ValueItem(res, type, no_copy);
	}
	ValueItem* math_abs(ValueItem* args, uint32_t args_len) {
		if (args) {
			if (args_len == 1) {
				switch (args->meta.vtype) {
				case VType::uarr: {
					list_array<ValueItem>* tmp = new list_array<ValueItem>(*(list_array<ValueItem>*)args->getSourcePtr());
					for (auto& it : *tmp) {
						it = math_abs_impl(it);
					}
					return new ValueItem(tmp, VType::uarr, no_copy);
				}
				case VType::faarr:
				case VType::saarr:
					return math_abs<VType::faarr>((ValueItem*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i8:
					return math_abs<VType::raw_arr_i8>((int8_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i16:
					return math_abs<VType::raw_arr_i16>((int16_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i32:
					return math_abs<VType::raw_arr_i32>((int32_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i64:
					return math_abs<VType::raw_arr_i64>((int64_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui8:
					return math_abs<VType::raw_arr_ui8>((uint8_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui16:
					return math_abs<VType::raw_arr_ui16>((uint16_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui32:
					return math_abs<VType::raw_arr_ui32>((uint32_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui64:
					return math_abs<VType::raw_arr_ui64>((uint64_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_flo:
					return math_abs<VType::raw_arr_flo>((float*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_doub:
					return math_abs<VType::raw_arr_doub>((float*)args->getSourcePtr(), args->meta.val_len);
				default:
					return new ValueItem(math_abs_impl(*args));
				}
			}
			else {
				return math_abs<VType::faarr>(args, args_len);
			}
		}
		return nullptr;
	}
#pragma endregion
#pragma region math_max
	template<class T>
	ValueItem* math_max(T* args, uint32_t args_len) {
		T* max = args;
		for (size_t i = 1; i < args_len; i++) {
			if (*max < args[i])
				max = &args[i];
		}
		return new ValueItem(*max);
	}
	ValueItem* math_max(ValueItem* args, uint32_t args_len) {
		if (args) {
			if (args_len == 1) {
				switch (args->meta.vtype) {
				case VType::uarr: {
					list_array<ValueItem>& tmp = *(list_array<ValueItem>*)args->getSourcePtr();
					ValueItem* max = &tmp[0];
					size_t len = tmp.size();
					for (size_t i = 1; i < len; i++) {
						if (*max < tmp[i])
							max = &tmp[i];
					}
					return new ValueItem(*max);
				}
				case VType::faarr:
				case VType::saarr:
					return math_max<ValueItem>((ValueItem*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i8:
					return math_max((int8_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i16:
					return math_max((int16_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i32:
					return math_max((int32_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i64:
					return math_max((int64_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui8:
					return math_max((uint8_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui16:
					return math_max((uint16_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui32:
					return math_max((uint32_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui64:
					return math_max((uint64_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_flo:
					return math_max((float*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_doub:
					return math_max((double*)args->getSourcePtr(), args->meta.val_len);
				default:
					return new ValueItem(*args);
				}
			}
			else {
				return math_max<ValueItem>(args, args_len);
			}
		}
		return nullptr;
	}
#pragma endregion
#pragma region math_min
	template<class T>
	ValueItem* math_min(T* args, uint32_t args_len) {
		T* min = args;
		for (size_t i = 1; i < args_len; i++) {
			if (*min > args[i])
				min = &args[i];
		}
		return new ValueItem(*min);
	}
	ValueItem* math_min(ValueItem* args, uint32_t args_len) {
		if (args) {
			if (args_len == 1) {
				switch (args->meta.vtype) {
				case VType::uarr: {
					list_array<ValueItem>& tmp = *(list_array<ValueItem>*)args->getSourcePtr();
					ValueItem* min = &tmp[0];
					size_t len = tmp.size();
					for (size_t i = 1; i < len; i++) {
						if (*min > tmp[i])
							min = &tmp[i];
					}
					return new ValueItem(*min); 
				}
				case VType::faarr:
				case VType::saarr:
					return math_min<ValueItem>((ValueItem*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i8:
					return math_min((int8_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i16:
					return math_min((int16_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i32:
					return math_min((int32_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i64:
					return math_min((int64_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui8:
					return math_min((uint8_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui16:
					return math_min((uint16_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui32:
					return math_min((uint32_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui64:
					return math_min((uint64_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_flo:
					return math_min((float*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_doub:
					return math_min((double*)args->getSourcePtr(), args->meta.val_len);
				default:
					return new ValueItem(*args);
				}
			}
			else {
				return math_min<ValueItem>(args, args_len);
			}
		}
		return nullptr;
	}
#pragma endregion
#pragma region math_median
	template<class T>
	ValueItem* math_median(T* args, uint32_t args_len) {
		size_t pos = args_len / 2;
		if (args_len % 2)
			return new ValueItem(args[pos]);
		else 
			return new ValueItem((args[pos] + args[pos + 1]) / 2);
	}
	ValueItem* math_median(ValueItem* args, uint32_t args_len) {
		if (args) {
			if (args_len == 1) {
				switch (args->meta.vtype) {
				case VType::uarr: {
					list_array<ValueItem>& tmp = *(list_array<ValueItem>*)args->getSourcePtr();
					size_t pos = tmp.size() / 2;
					if (tmp.size() % 2)
						return new ValueItem(tmp[pos]);
					else 
						return new ValueItem((tmp[pos] + tmp[pos + 1])/ ValueItem((void*)2, VType::i64));
				}
				case VType::faarr:
				case VType::saarr:
					return math_median<ValueItem>((ValueItem*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i8:
					return math_median((int8_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i16:
					return math_median((int16_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i32:
					return math_median((int32_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i64:
					return math_median((int64_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui8:
					return math_median((uint8_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui16:
					return math_median((uint16_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui32:
					return math_median((uint32_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui64:
					return math_median((uint64_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_flo:
					return math_median((float*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_doub:
					return math_median((double*)args->getSourcePtr(), args->meta.val_len);
				default:
					return new ValueItem(*args);
				}
			}
			else {
				return math_median<ValueItem>(args, args_len);
			}
		}
		return nullptr;
	}
#pragma endregion
#pragma region math_range
	template<class T>
	ValueItem* math_range(T* args, uint32_t args_len) {
		T* max = args;
		T* min = args;
		for (uint32_t i = 0; i < args_len; i++) {
			T& it = args[i];
			if (*max < it)
				max = &it;
			if (*min > it)
				min = &it;
		}
		if (args_len)
			return new ValueItem(*max - *min);
		else
			return nullptr;
	}
	ValueItem* math_range(ValueItem* args, uint32_t args_len) {
		if (args) {
			if (args_len == 1) {
				switch (args->meta.vtype) {
				case VType::uarr: {
					list_array<ValueItem>& tmp = *(list_array<ValueItem>*)args->getSourcePtr();
					bool ist_first = true;
					ValueItem* max = nullptr;
					ValueItem* min = nullptr;
					for (auto& it : tmp) {
						if (ist_first) {
							max = &it;
							min = &it;
							ist_first = false;
							continue;
						}
						if (*max < it)
							max = &it;
						if (*min > it)
							min = &it;
					}
					if (max)
						return new ValueItem(*max - *min);
					else
						return nullptr;
				}
				case VType::faarr:
				case VType::saarr:
					return math_range<ValueItem>((ValueItem*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i8:
					return math_range((int8_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i16:
					return math_range((int16_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i32:
					return math_range((int32_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i64:
					return math_range((int64_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui8:
					return math_range((uint8_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui16:
					return math_range((uint16_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui32:
					return math_range((uint32_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui64:
					return math_range((uint64_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_flo:
					return math_range((float*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_doub:
					return math_range((double*)args->getSourcePtr(), args->meta.val_len);
				default:
					return new ValueItem(*args);
				}
			}
			else {
				return math_range<ValueItem>(args, args_len);
			}
		}
		return nullptr;
	}
#pragma endregion
#pragma region math_mode
	template<class T>
	ValueItem* math_mode(T* args, uint32_t args_len) {
		list_array<T> copy(args, args + args_len);
		copy.sort();
		size_t max_count = 0;
		T* max_it = nullptr;
		size_t cur_count = 0;
		T* cur_it = nullptr;
		for (auto& it : copy) {
			if (!cur_it) {
				cur_it = &it;
				++cur_count;
				continue;
			}
			if (*cur_it != it) {
				if (!max_it) {
					max_it = cur_it;
					max_count = cur_count;
				}
				else if (max_count < cur_count) {
					max_it = cur_it;
					max_count = cur_count;
				}
				cur_it = &it;
				cur_count = 0;
			}
			++cur_count;
		}
		if (!max_it)
			if (!cur_it)
				return nullptr;
			else
				return new ValueItem(*cur_it);
		else
			return new ValueItem(*max_it);
	}
	ValueItem* math_mode(ValueItem* args, uint32_t args_len) {
		if (args) {
			if (args_len == 1) {
				switch (args->meta.vtype) {
				case VType::uarr: {
					size_t max_count = 0;
					ValueItem* max_it = nullptr;
					size_t cur_count = 0;
					ValueItem* cur_it = nullptr;
					for (auto& it : ((list_array<ValueItem>*)args->getSourcePtr())->sort_copy()) {
						if (!cur_it) {
							cur_it = &it;
							++cur_count;
							continue;
						}
						if (*cur_it != it) {
							if (!max_it) {
								max_it = cur_it;
								max_count = cur_count;
							}
							else if (max_count < cur_count) {
								max_it = cur_it;
								max_count = cur_count;
							}
							cur_it = &it;
							cur_count = 0;
						}
						++cur_count;
					}
					if (!max_it)
						if (!cur_it)
							return nullptr;
						else
							return new ValueItem(*cur_it);
					else
						return new ValueItem(*max_it);
				}
				case VType::faarr:
				case VType::saarr:
					return math_mode<ValueItem>((ValueItem*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i8:
					return math_mode((int8_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i16:
					return math_mode((int16_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i32:
					return math_mode((int32_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i64:
					return math_mode((int64_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui8:
					return math_mode((uint8_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui16:
					return math_mode((uint16_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui32:
					return math_mode((uint32_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui64:
					return math_mode((uint64_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_flo:
					return math_mode((float*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_doub:
					return math_mode((double*)args->getSourcePtr(), args->meta.val_len);
				default:
					return new ValueItem(*args);
				}
			}
			else {
				return math_mode<ValueItem>(args, args_len);
			}
		}
		return nullptr;
	}
#pragma endregion
#pragma region math_transform_fn
	template<float(*ffn)(float), double(*dfn)(double)>
	ValueItem math_transform_impl(ValueItem& val) {
		if (val.meta.vtype == VType::async_res)
			val.getAsync();
		switch (val.meta.vtype) {
		case VType::flo: {
			union {
				float abs;
				void* res = nullptr;
			};
			abs = ffn((float)val);
			return ValueItem(res, val.meta);
		}
		case VType::doub: {
			union {
				double abs;
				void* res = nullptr;
			};
			abs = dfn((double)val);
			return ValueItem(res, val.meta);
		}
		default:
			return val;
		}
	}

	template<VType arr_ty, float(*ffn)(float), double(*dfn)(double), class T>
	ValueItem* math_transform(T* args, uint32_t args_len) {
		T* new_arr = new T[args_len];
		for (size_t i = 0; i < args_len; i++) {
			if constexpr (std::is_same_v<T, ValueItem>)
				new_arr[i] = math_transform_impl<ffn, dfn>(args[i]);
			else if constexpr (std::is_same_v<T, float>) {
				new_arr[i] = ffn(args[i]);
			}
			else if constexpr (std::is_same_v<T, double>) {
				new_arr[i] = dfn(args[i]);
			}
		}
		return new ValueItem(new_arr, ValueMeta(arr_ty,false,true, args_len));
	}
	template<float(*ffn)(float), double(*dfn)(double)>
	ValueItem* math_transform_fn(ValueItem* args, uint32_t args_len) {
		if (args) {
			if (args_len == 1) {
				switch (args->meta.vtype) {
				case VType::uarr: {
					auto& args_r = *(list_array<ValueItem>*)args->getSourcePtr();
					list_array<ValueItem>* res = new list_array<ValueItem>;
					res->reserve_push_back(args_r.size());
					for (auto& it : args_r)
						res->push_back(math_transform_impl<ffn, dfn>(it));
					return new ValueItem(res, VType::uarr, no_copy);
				}
				case VType::faarr:
				case VType::saarr:
					return math_transform<VType::faarr, ffn, dfn>((ValueItem*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_flo:
					return math_transform<VType::raw_arr_flo, ffn, dfn>((float*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_doub:
					return math_transform<VType::raw_arr_doub, ffn, dfn>((double*)args->getSourcePtr(), args->meta.val_len);
				default:
					return new ValueItem(*args);
				}
			}
			else {
				return math_transform<VType::faarr, ffn, dfn>(args, args_len);
			}
		}
		return nullptr;
	}
#pragma endregion


#pragma region _math_stuffs_
	uint64_t math_factorial_impl__FAST_FACTOR(uint64_t m) {
		switch (m) {
		case 0:return 0;
		case 1:return 1;
		case 2:return 2;
		case 3:return 6;
		case 4:return 24;
		case 5:return 120;
		case 6:return 720;
		case 7:return 5040;
		case 8:return 40320;
		case 9:return 362880;
		case 10:return 3628800;
		default:
			return 0;
		}

	}
	uint64_t math_factorial_impl_FACTORu(uint64_t m) {
		if (m < 0)
			return -1;
		if (m < 10)
			return math_factorial_impl__FAST_FACTOR(m);
		uint64_t res = 39916800;
		for (uint64_t i = 12; i < m; i++)
			res *= i;
		res *= m;
		return res;
	}
	int64_t math_factorial_impl_FACTORs(int64_t m) {
		if (m < 0)
			return -1;
		if (m < 10)
			return math_factorial_impl__FAST_FACTOR((uint64_t)m);
		int64_t res = 39916800;
		for (int64_t i = 12; i < m; i++)
			res *= i;
		res *= m;
		return res;
	}
	ValueItem math_factorial_impl(ValueItem& val) {
		if (val.meta.vtype == VType::async_res)
			val.getAsync();
		
		switch (val.meta.vtype) {
		case VType::i8:
			return ValueItem((void*)math_factorial_impl_FACTORs((int8_t)val), val.meta);
		case VType::ui8:
			return ValueItem((void*)math_factorial_impl_FACTORu((uint8_t)val), val.meta);
		case VType::i16:
			return ValueItem((void*)math_factorial_impl_FACTORs((int16_t)val), val.meta);
		case VType::ui16:
			return ValueItem((void*)math_factorial_impl_FACTORu((uint16_t)val), val.meta);
		case VType::i32:
			return ValueItem((void*)math_factorial_impl_FACTORs((int32_t)val), val.meta);
		case VType::ui32:
			return ValueItem((void*)math_factorial_impl_FACTORu((uint32_t)val), val.meta);
		case VType::i64:
			return ValueItem((void*)math_factorial_impl_FACTORs((int64_t)val), val.meta);
		case VType::ui64:
			return ValueItem((void*)math_factorial_impl_FACTORu((uint64_t)val), val.meta);
		case VType::undefined_ptr:
			return ValueItem((void*)math_factorial_impl_FACTORu((size_t)val), val.meta);
		case VType::flo: {
			union {
				float x;
				void* res = nullptr;
			};
			x = (float)std::tgamma((double)val);
			return ValueItem(res, val.meta);
		}
		case VType::doub: {
			union {
				double x;
				void* res = nullptr;
			};
			x = std::tgamma((double)val);
			return ValueItem(res, val.meta);
		}
		default:
			return val;
		}
	}
	template<class T>
	ValueItem* math_factorial(T* args, uint32_t args_len) {
		T* new_arr = new T[args_len];
		for (size_t i = 0; i < args_len; i++) {
			if constexpr (std::is_same_v<T, ValueItem>)
				new_arr[i] = math_factorial_impl(args[i]);
			else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
				new_arr[i] = std::tgamma(args[i]);
			}
			else if constexpr(std::is_unsigned_v<T>) {
				new_arr[i] = (T)math_factorial_impl_FACTORu((uint64_t)args[i]);
			}
			else {
				new_arr[i] = (T)math_factorial_impl_FACTORs((int64_t)args[i]);
			}
		}
		return new ValueItem(new_arr, args_len);
	}
	ValueItem* math_factorial(ValueItem* args, uint32_t args_len) {
		if (args) {
			if (args_len == 1) {
				switch (args->meta.vtype) {
				case VType::uarr: {
					list_array<ValueItem> new_uarr;
					new_uarr.reserve_push_back((((list_array<ValueItem>*)args->getSourcePtr())->size()));
					for (auto& it : *(list_array<ValueItem>*)args->getSourcePtr())
						new_uarr.push_back(math_factorial_impl(it));
					return new ValueItem(std::move(new_uarr));
				}
				case VType::faarr:
				case VType::saarr:
					return math_factorial<ValueItem>((ValueItem*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i8:
					return math_factorial((int8_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i16:
					return math_factorial((int16_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i32:
					return math_factorial((int32_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i64:
					return math_factorial((int64_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui8:
					return math_factorial((uint8_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui16:
					return math_factorial((uint16_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui32:
					return math_factorial((uint32_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui64:
					return math_factorial((uint64_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_flo:
					return math_factorial((float*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_doub:
					return math_factorial((double*)args->getSourcePtr(), args->meta.val_len);
				default:
					return new ValueItem(*args);
				}
			}
			else {
				return math_factorial<ValueItem>(args, args_len);
			}
		}
		return nullptr;
	}




	template<double(*fn)(double)>
	ValueItem math_trigonometric_impl(ValueItem& val) {
		if (val.meta.vtype == VType::async_res)
			val.getAsync();
		switch (val.meta.vtype) {
		case VType::i8:
		case VType::ui8:
		case VType::i16:
		case VType::ui16:
		case VType::i32:
		case VType::ui32:
		case VType::i64:
		case VType::ui64:
			ValueItem tmp((double)val);
			return math_trigonometric_impl<fn>(tmp);
		}
		switch (val.meta.vtype) {
		case VType::flo:
			return ValueItem((float)fn((float)val));
		case VType::doub: 
			return ValueItem(fn((double)val));
		default:
			return val;
		}
	}
	template<double(*fn)(double), class T>
	ValueItem* math_trigonometric_(T* args, uint32_t args_len) {
		T* new_arr = new T[args_len];
		for (size_t i = 0; i < args_len; i++) {
			if constexpr (std::is_same_v<T, ValueItem>)
				new_arr[i] = math_trigonometric_impl<fn>(args[i]);
			else {
				new_arr[i] = (T)fn((double)args[i]);
			}
		}
		return new ValueItem(new_arr, args_len);
	}
	template<double(*fn)(double)>
	ValueItem* math_trigonometric(ValueItem* args, uint32_t args_len) {
		if (args) {
			if (args_len == 1) {
				switch (args->meta.vtype) {
				case VType::uarr: {
					list_array<ValueItem> new_uarr;
					new_uarr.reserve_push_back((((list_array<ValueItem>*)args->getSourcePtr())->size()));
					for (auto& it : *(list_array<ValueItem>*)args->getSourcePtr())
						new_uarr.push_back(math_trigonometric_impl<fn>(it));
					return new ValueItem(std::move(new_uarr));
				}
				case VType::faarr:
				case VType::saarr:
					return math_trigonometric_<fn,ValueItem>((ValueItem*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i8:
					return math_trigonometric_<fn, int8_t>((int8_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i16:
					return math_trigonometric_<fn, int16_t>((int16_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i32:
					return math_trigonometric_<fn, int32_t>((int32_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_i64:
					return math_trigonometric_<fn, int64_t>((int64_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui8:
					return math_trigonometric_<fn, uint8_t>((uint8_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui16:
					return math_trigonometric_<fn, uint16_t >((uint16_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui32:
					return math_trigonometric_<fn, uint32_t>((uint32_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_ui64:
					return math_trigonometric_<fn, uint64_t>((uint64_t*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_flo:
					return math_trigonometric_<fn, float>((float*)args->getSourcePtr(), args->meta.val_len);
				case VType::raw_arr_doub:
					return math_trigonometric_<fn, double>((double*)args->getSourcePtr(), args->meta.val_len);
				default:
					return new ValueItem(*args);
				}
			}
			else {
				return math_trigonometric_<fn, ValueItem>(args, args_len);
			}
		}
		return nullptr;
	}

	double csc(double d) {
		return 1 / sin(d);
	}
	double acsc(double d) {
		return asin(1/d);
	}
	double sec(double d) {
		return 1 / cos(d);
	}
	double asec(double d) {
		return acos(1 / d);
	}
	double cot(double d) {
		return 1 / tan(d);
	}
	double acot(double d) {
		return tan(1 / d);
	}

	double csch(double d) {
		return 1 / sinh(d);
	}
	double acsch(double d) {
		return asinh(1 / d);
	}
	double sech(double d) {
		return 1 / cosh(d);
	}
	double asech(double d) {
		return acosh(1 / d);
	}
	double coth(double d) {
		return 1 / tanh(d);
	}
	double acoth(double d) {
		return tanh(1 / d);
	}

	template<class T>
	ValueItem* math_pow(T* args, uint32_t args_len,double power) {
		T* new_arr = new T[args_len];
		for (size_t i = 0; i < args_len; i++)
			new_arr[i] = (T)pow((double)args[i], power);
		return new ValueItem(new_arr, args_len);
	}
	ValueItem* math_pow(ValueItem* args, uint32_t args_len) {
		if (args) {
			double power = 2;
			if (args_len <= 2) {
				double power;
				if (args_len == 2)
					power = (double)args[1];
				else
					power = 2;
				switch (args->meta.vtype) {
				case VType::uarr: {
					list_array<ValueItem> new_uarr;
					new_uarr.reserve_push_back((((list_array<ValueItem>*)args->getSourcePtr())->size()));
					for (auto& it : *(list_array<ValueItem>*)args->getSourcePtr())
						new_uarr.push_back(ValueItem(pow((double)it, power)));
					return new ValueItem(std::move(new_uarr));
				}
				case VType::faarr:
				case VType::saarr:
					return math_pow<ValueItem>((ValueItem*)args->getSourcePtr(), args->meta.val_len, power);
				case VType::raw_arr_i8:
					return math_pow((int8_t*)args->getSourcePtr(), args->meta.val_len, power);
				case VType::raw_arr_i16:
					return math_pow((int16_t*)args->getSourcePtr(), args->meta.val_len, power);
				case VType::raw_arr_i32:
					return math_pow((int32_t*)args->getSourcePtr(), args->meta.val_len, power);
				case VType::raw_arr_i64:
					return math_pow((int64_t*)args->getSourcePtr(), args->meta.val_len, power);
				case VType::raw_arr_ui8:
					return math_pow((uint8_t*)args->getSourcePtr(), args->meta.val_len, power);
				case VType::raw_arr_ui16:
					return math_pow((uint16_t*)args->getSourcePtr(), args->meta.val_len, power);
				case VType::raw_arr_ui32:
					return math_pow((uint32_t*)args->getSourcePtr(), args->meta.val_len, power);
				case VType::raw_arr_ui64:
					return math_pow((uint64_t*)args->getSourcePtr(), args->meta.val_len, power);
				case VType::raw_arr_flo:
					return math_pow((float*)args->getSourcePtr(), args->meta.val_len, power);
				case VType::raw_arr_doub:
					return math_pow((double*)args->getSourcePtr(), args->meta.val_len, power);
				default:
					return new ValueItem(*args);
				}
			}
			else {
				return math_pow<ValueItem>(args, args_len, 2);
			}
		}
		return nullptr;
	}

#pragma endregion
#pragma endregion
#define INIT_CHECK static bool is_init = false; if (is_init) return; is_init = true;
	
	void initStandardLib_exception(){
		INIT_CHECK
		FuncEnvironment::AddNative(exceptions::get_current_exception_name, "exception current_get_name", false);
		FuncEnvironment::AddNative(exceptions::get_current_exception_description, "exception current_get_description", false);
		FuncEnvironment::AddNative(exceptions::get_current_exception_full_description, "exception current_get_full_description", false);
		FuncEnvironment::AddNative(exceptions::has_current_exception_inner_exception, "exception current_has_inner", false);
		FuncEnvironment::AddNative(exceptions::unpack_current_exception, "exception current_unpack", false);
		FuncEnvironment::AddNative(exceptions::in_exception, "exception now_in_exception", false);
		FuncEnvironment::AddNative(exceptions::current_exception_catched, "exception set_as_catched", false);
		//print full stacktrace if used in filter function
		FuncEnvironment::AddNative(internal::stack::trace, "exception filter stacktrace", false);
	}
	void initStandardLib_bytes(){
		INIT_CHECK
		FuncEnvironment::AddNative(bytes::to_bytes, "bytes to_bytes", false);
		FuncEnvironment::AddNative(bytes::from_bytes, "bytes from_bytes", false);
		FuncEnvironment::AddNative(bytes::convert_endian, "bytes convert_endian", false);
		FuncEnvironment::AddNative(bytes::current_endian, "bytes current_endian", false);
		FuncEnvironment::AddNative(bytes::swap_bytes, "bytes swap_bytes", false);
	}
	void initStandardLib_console(){
		INIT_CHECK
		FuncEnvironment::AddNative(console::printLine, "console print_line", false);
		FuncEnvironment::AddNative(console::print, "console print", false);

		FuncEnvironment::AddNative(console::register_format_operator, "console register_format_operator", false);
		FuncEnvironment::AddNative(console::format, "console format", false);//return string, not print
		FuncEnvironment::AddNative(console::printf, "console printf", false);//format and print

		FuncEnvironment::AddNative(console::resetModifiers, "console reset_modifiers", false);
		FuncEnvironment::AddNative(console::boldText, "console bold_text", false);
		FuncEnvironment::AddNative(console::italicText, "console italic_text", false);
		FuncEnvironment::AddNative(console::underlineText, "console underline_text", false);
		FuncEnvironment::AddNative(console::slowBlink, "console slow_blink", false);
		FuncEnvironment::AddNative(console::rapidBlink, "console rapid_blink", false);
		FuncEnvironment::AddNative(console::invertColors, "console invert_colors", false);
		FuncEnvironment::AddNative(console::notBoldText, "console not_bold_text", false);
		FuncEnvironment::AddNative(console::notUnderlinedText, "console not_underlined_text", false);
		FuncEnvironment::AddNative(console::hideBlinkText, "console hide_blink_text", false);

		FuncEnvironment::AddNative(console::resetTextColor, "console reset_text_color", false);
		FuncEnvironment::AddNative(console::resetBgColor, "console reset_bg_color", false);
		FuncEnvironment::AddNative(console::setTextColor, "console set_text_color", false);
		FuncEnvironment::AddNative(console::setBgColor, "console set_bg_color", false);
		FuncEnvironment::AddNative(console::setPos, "console set_pos", false);
		FuncEnvironment::AddNative(console::saveCurPos, "console save_cur_pos", false);
		FuncEnvironment::AddNative(console::loadCurPos, "console load_cur_pos", false);
		FuncEnvironment::AddNative(console::setLine, "console set_line", false);
		FuncEnvironment::AddNative(console::showCursor, "console show_cursor", false);
		FuncEnvironment::AddNative(console::hideCursor, "console hide_cursor", false);

		FuncEnvironment::AddNative(console::readWord, "console read_word", false);
		FuncEnvironment::AddNative(console::readLine, "console read_line", false);
		FuncEnvironment::AddNative(console::readInput, "console read_input", false);
		FuncEnvironment::AddNative(console::readValue, "console read_value", false);
		FuncEnvironment::AddNative(console::readInt, "console read_int", false);
	}
	void initStandardLib_math(){
		INIT_CHECK
		FuncEnvironment::AddNative(math_abs, "math abs", false);

		FuncEnvironment::AddNative(math_min, "math min", false);
		FuncEnvironment::AddNative(math_max, "math max", false);
		FuncEnvironment::AddNative(math_median, "math median", false);
		FuncEnvironment::AddNative(math_range, "math range", false);
		FuncEnvironment::AddNative(math_mode, "math mode", false);

		FuncEnvironment::AddNative(math_transform_fn<(float(*)(float))round, (double(*)(double))round>, "math round", false);
		FuncEnvironment::AddNative(math_transform_fn<(float(*)(float))floor, (double(*)(double))floor>, "math floor", false);
		FuncEnvironment::AddNative(math_transform_fn<(float(*)(float))ceil, (double(*)(double))ceil>, "math ceil", false);
		FuncEnvironment::AddNative(math_transform_fn<(float(*)(float))trunc, (double(*)(double))trunc>, "math fix", false);
		
		FuncEnvironment::AddNative(math_factorial, "math factorial", false);
		FuncEnvironment::AddNative(math_trigonometric<sin>, "math sin", false);
		FuncEnvironment::AddNative(math_trigonometric<asin>, "math asin", false);
		FuncEnvironment::AddNative(math_trigonometric<cos>, "math cos", false);
		FuncEnvironment::AddNative(math_trigonometric<acos>, "math acos", false);
		FuncEnvironment::AddNative(math_trigonometric<tan>, "math tan", false);
		FuncEnvironment::AddNative(math_trigonometric<atan>, "math atan", false);
		FuncEnvironment::AddNative(math_trigonometric<csc>, "math csc", false);
		FuncEnvironment::AddNative(math_trigonometric<acsc>, "math acsc", false);
		FuncEnvironment::AddNative(math_trigonometric<sec>, "math sec", false);
		FuncEnvironment::AddNative(math_trigonometric<asec>, "math asec", false);
		FuncEnvironment::AddNative(math_trigonometric<cot>, "math cot", false);
		FuncEnvironment::AddNative(math_trigonometric<acot>, "math acot", false);
		FuncEnvironment::AddNative(math_trigonometric<sinh>, "math sinh", false);
		FuncEnvironment::AddNative(math_trigonometric<asinh>, "math asinh", false);
		FuncEnvironment::AddNative(math_trigonometric<cosh>, "math cosh", false);
		FuncEnvironment::AddNative(math_trigonometric<acosh>, "math acosh", false);
		FuncEnvironment::AddNative(math_trigonometric<tanh>, "math tanh", false);
		FuncEnvironment::AddNative(math_trigonometric<atanh>, "math atanh", false);
		FuncEnvironment::AddNative(math_trigonometric<csch>, "math csch", false);
		FuncEnvironment::AddNative(math_trigonometric<acsch>, "math acsch", false);
		FuncEnvironment::AddNative(math_trigonometric<sech>, "math sech", false);
		FuncEnvironment::AddNative(math_trigonometric<asech>, "math asech", false);
		FuncEnvironment::AddNative(math_trigonometric<coth>, "math coth", false);
		FuncEnvironment::AddNative(math_trigonometric<acoth>, "math acoth", false);

		FuncEnvironment::AddNative(math_trigonometric<sqrt>, "math sqrt", false);
		FuncEnvironment::AddNative(math_trigonometric<cbrt>, "math cbrt", false);
		FuncEnvironment::AddNative(math_pow, "math pow", false);
		FuncEnvironment::AddNative(math_trigonometric<log>, "math log", false);
		FuncEnvironment::AddNative(math_trigonometric<log2>, "math log2", false);
		FuncEnvironment::AddNative(math_trigonometric<log10>, "math log10", false);
	}
	void initStandardLib_file(){
		INIT_CHECK
		file::init();
		FuncEnvironment::AddNative(file::constructor::createProxy_FileHandle, "# file file_handle", false);
		FuncEnvironment::AddNative(file::constructor::createProxy_BlockingFileHandle, "# file blocking_file_handle", false);
		FuncEnvironment::AddNative(file::constructor::createProxy_TextFile, "# file text_file", false);
		FuncEnvironment::AddNative(file::constructor::createProxy_FolderBrowser, "# file folder_browser", false);
		FuncEnvironment::AddNative(file::constructor::createProxy_FolderChangesMonitor, "# file folder_changes_monitor", false);
		FuncEnvironment::AddNative(file::remove, "file remove", false);
		FuncEnvironment::AddNative(file::rename, "file rename", false);
		FuncEnvironment::AddNative(file::copy, "file copy", false);
	}
	void initStandardLib_parallel(){
		INIT_CHECK
		parallel::init();
		FuncEnvironment::AddNative(parallel::constructor::createProxy_Mutex, "# parallel mutex", false);
		FuncEnvironment::AddNative(parallel::constructor::createProxy_RecursiveMutex, "# parallel recursive_mutex", false);
		FuncEnvironment::AddNative(parallel::constructor::createProxy_ConditionVariable, "# parallel condition_variable", false);
		FuncEnvironment::AddNative(parallel::constructor::createProxy_Semaphore, "# parallel semaphore", false);
		FuncEnvironment::AddNative(parallel::constructor::createProxy_EventSystem, "# parallel event_system", false);
		FuncEnvironment::AddNative(parallel::constructor::createProxy_TaskLimiter, "# parallel task_limiter", false);
		FuncEnvironment::AddNative(parallel::constructor::createProxy_TaskQuery, "# parallel task_query", false);
		FuncEnvironment::AddNative(parallel::constructor::construct_Task, "# parallel task", false);
		FuncEnvironment::AddNative(parallel::constructor::createProxy_TaskGroup, "# parallel task_group", false);
		FuncEnvironment::AddNative(parallel::createThread, "parallel create_thread", false);
		FuncEnvironment::AddNative(parallel::createThreadAndWait, "parallel create_thread_and_wait", false);
		FuncEnvironment::AddNative(parallel::createAsyncThread, "parallel create_async_thread", false);
		FuncEnvironment::AddNative(parallel::createTask, "parallel create_task", false);
		FuncEnvironment::AddNative(parallel::task_runtime::await_end_tasks, "parallel task_runtime await_end_tasks", false);
		FuncEnvironment::AddNative(parallel::task_runtime::await_no_tasks, "parallel task_runtime await_no_tasks", false);
		FuncEnvironment::AddNative(parallel::task_runtime::become_task_executor, "parallel task_runtime become_task_executor", false);
		FuncEnvironment::AddNative(parallel::task_runtime::clean_up, "parallel task_runtime clean_up", false);
		FuncEnvironment::AddNative(parallel::task_runtime::create_executor, "parallel task_runtime create_executor", false);
		FuncEnvironment::AddNative(parallel::task_runtime::explicitStartTimer, "parallel task_runtime explicitStartTimer", false);
		FuncEnvironment::AddNative(parallel::task_runtime::reduce_executor, "parallel task_runtime reduce_executor", false);
		FuncEnvironment::AddNative(parallel::task_runtime::total_executors, "parallel task_runtime total_executors", false);
		FuncEnvironment::AddNative(parallel::this_task::check_cancellation, "parallel this_task check_cancellation", false);
		FuncEnvironment::AddNative(parallel::this_task::is_task, "parallel this_task is_task", false);
		FuncEnvironment::AddNative(parallel::this_task::self_cancel, "parallel this_task self_cancel", false);
		FuncEnvironment::AddNative(parallel::this_task::sleep, "parallel this_task sleep", false);
		FuncEnvironment::AddNative(parallel::this_task::sleep_until, "parallel this_task sleep_until", false);
		FuncEnvironment::AddNative(parallel::this_task::task_id, "parallel this_task task_id", false);
		FuncEnvironment::AddNative(parallel::this_task::yield, "parallel this_task yield", false);
		FuncEnvironment::AddNative(parallel::this_task::yield_result, "parallel this_task yield_result", false);
		FuncEnvironment::AddNative(parallel::atomic::constructor::createProxy_Any, "# parallel atomic any", false);
		FuncEnvironment::AddNative(parallel::atomic::constructor::createProxy_Bool, "# parallel atomic boolean", false);
		FuncEnvironment::AddNative(parallel::atomic::constructor::createProxy_I8, "# parallel atomic i8", false);
		FuncEnvironment::AddNative(parallel::atomic::constructor::createProxy_I16, "# parallel atomic i16", false);
		FuncEnvironment::AddNative(parallel::atomic::constructor::createProxy_I32, "# parallel atomic i32", false);
		FuncEnvironment::AddNative(parallel::atomic::constructor::createProxy_I64, "# parallel atomic i64", false);
		FuncEnvironment::AddNative(parallel::atomic::constructor::createProxy_UI8, "# parallel atomic ui8", false);
		FuncEnvironment::AddNative(parallel::atomic::constructor::createProxy_UI16, "# parallel atomic ui16", false);
		FuncEnvironment::AddNative(parallel::atomic::constructor::createProxy_UI32, "# parallel atomic ui32", false);
		FuncEnvironment::AddNative(parallel::atomic::constructor::createProxy_UI64, "# parallel atomic ui64", false);
		FuncEnvironment::AddNative(parallel::atomic::constructor::createProxy_Float, "# parallel atomic float", false);
		FuncEnvironment::AddNative(parallel::atomic::constructor::createProxy_Double, "# parallel atomic double", false);
		FuncEnvironment::AddNative(parallel::atomic::constructor::createProxy_UndefinedPtr, "# parallel atomic undefined_ptr", false);
	}
	void initStandardLib_chanel(){
		INIT_CHECK
		FuncEnvironment::AddNative(chanel::constructor::createProxy_Chanel, "# chanel chanel", false);
		FuncEnvironment::AddNative(chanel::constructor::createProxy_ChanelHandler, "# chanel chanel_handler", false);
	}
	void initStandardLib_internal(){
		INIT_CHECK
		internal::init();
		FuncEnvironment::AddNative(internal::construct::createProxy_function_builder, "# internal function_builder", false);
		FuncEnvironment::AddNative(internal::construct::createProxy_index_pos, "# internal index_pos", false);
		FuncEnvironment::AddNative(internal::construct::createProxy_line_info, "# internal line_info", false);
	}
	void initStandardLib_internal_memory(){
		INIT_CHECK
		FuncEnvironment::AddNative(internal::memory::dump, "internal memory dump", false);

	}
	void initStandardLib_internal_run_time(){
		INIT_CHECK
		FuncEnvironment::AddNative(internal::run_time::gc_hint_collect, "internal run_time gc_hint_collect", false);
		FuncEnvironment::AddNative(internal::run_time::gc_pause, "internal run_time gc_pause", false);
		FuncEnvironment::AddNative(internal::run_time::gc_resume, "internal run_time gc_resume", false);
	}
	void initStandardLib_internal_run_time_native(){
		INIT_CHECK
		internal::run_time::native::init();
		FuncEnvironment::AddNative(internal::run_time::native::construct::createProxy_NativeLib, "# internal run_time native native_lib", false);
		FuncEnvironment::AddNative(internal::run_time::native::construct::createProxy_NativeValue, "# internal run_time native native_value", false);
		FuncEnvironment::AddNative(internal::run_time::native::construct::createProxy_NativeTemplate, "# internal run_time native native_template", false);

	}
	void initStandardLib_internal_stack(){
		INIT_CHECK
		FuncEnvironment::AddNative(internal::stack::dump, "internal stack dump", false);

		FuncEnvironment::AddNative(internal::stack::bs_supported, "internal stack bs_supported", false);
		FuncEnvironment::AddNative(internal::stack::allocated_size, "internal stack allocated_size", false);
		FuncEnvironment::AddNative(internal::stack::free_size, "internal stack free_size", false);
		FuncEnvironment::AddNative(internal::stack::prepare, "internal stack prepare", false);
		FuncEnvironment::AddNative(internal::stack::reserve, "internal stack reserve", false);
		FuncEnvironment::AddNative(internal::stack::shrink, "internal stack shrink", false);
		FuncEnvironment::AddNative(internal::stack::unused_size, "internal stack unused_size", false);
		FuncEnvironment::AddNative(internal::stack::used_size, "internal stack used_size", false);

		FuncEnvironment::AddNative(internal::stack::trace, "internal stack trace", false);
		FuncEnvironment::AddNative(internal::stack::trace_frames, "internal stack trace_frames", false);
		FuncEnvironment::AddNative(internal::stack::resolve_frame, "internal stack resolve_frame", false);
	}

	void initStandardLib_net(){
		INIT_CHECK
		net::init();
		FuncEnvironment::AddNative(net::constructor::createProxy_TcpServer, "# net tcp_server", false);
		FuncEnvironment::AddNative(net::constructor::createProxy_Address, "# net ip#address", false);
		FuncEnvironment::AddNative(net::constructor::createProxy_UdpSocket, "# net udp_socket", false);
		FuncEnvironment::AddNative(net::constructor::createProxy_IP4, "# net ip#v4", false);
		FuncEnvironment::AddNative(net::constructor::createProxy_IP6, "# net ip#v6", false);
		FuncEnvironment::AddNative(net::constructor::createProxy_IP, "# net ip", false);
		FuncEnvironment::AddNative(net::ipv6_supported, "net ipv6_supported", false);
		FuncEnvironment::AddNative(net::tcp_client_connect, "net tcp_client_connect", false);
	}
	namespace configuration{
		ValueItem* modify_configuration(ValueItem* args, uint32_t len){
			if(len < 2)
				throw InvalidArguments("configuration modify_configuration: invalid arguments count, expected 2");
			if(args[0].meta.vtype == VType::string){
				if(args[1].meta.vtype == VType::string)
					modify_run_time_config((const std::string&)args[0].getSourcePtr(), (const std::string&)args[1].getSourcePtr());
				else 
					modify_run_time_config((const std::string&)args[0].getSourcePtr(), (std::string)args[1]);
			}
			else{
				std::string key = (std::string)args[0];
				if(args[1].meta.vtype == VType::string)
					modify_run_time_config(key, (const std::string&)args[1].getSourcePtr());
				else 
					modify_run_time_config(key, (std::string)args[1]);
			}
			return nullptr;
		}
		ValueItem* get_configuration(ValueItem* args, uint32_t len){
			if(len < 1)
				throw InvalidArguments("configuration get_configuration: invalid arguments count, expected 1");
			if(args[0].meta.vtype == VType::string)
				return new ValueItem(get_run_time_config((const std::string&)args[0].getSourcePtr()));
			else 
				return new ValueItem(get_run_time_config((std::string)args[0]));
		}
	}
	void initStandardLib_configuration(){
		INIT_CHECK
		FuncEnvironment::AddNative(configuration::modify_configuration, "configuration modify", false);
		FuncEnvironment::AddNative(configuration::get_configuration, "configuration get", false);
	}

	namespace debug{
		ValueItem* thread_id(ValueItem*, uint32_t){
			return new ValueItem(_thread_id());
		}
		ValueItem* set_thread_name(ValueItem* args, uint32_t len){
			if(len >= 1)
				throw InvalidArguments("debug set_thread_name: invalid arguments count, expected 1");
			if(args[0].meta.vtype == VType::string)
				_set_name_thread_dbg((const std::string&)args[0].getSourcePtr());
			else 
				_set_name_thread_dbg((std::string)args[0]);
			return nullptr;
		}
		ValueItem* get_thread_name(ValueItem* args, uint32_t len){
			if(len > 1)
				return new ValueItem(_get_name_thread_dbg(_thread_id()));
			else
				return new ValueItem(_get_name_thread_dbg((int)args[0]));
		}

		ValueItem* invite(ValueItem* args, uint32_t len){
			if(args[0].meta.vtype == VType::string)
				invite_to_debugger((const std::string&)args[0].getSourcePtr());
			else 
				invite_to_debugger((std::string)args[0]);
			return nullptr;
		}
	}
	void initStandardLib_debug(){
		INIT_CHECK
		FuncEnvironment::AddNative(debug::thread_id, "debug thread_id", false);
		FuncEnvironment::AddNative(debug::set_thread_name, "debug set_thread_name", false);
		FuncEnvironment::AddNative(debug::get_thread_name, "debug get_thread_name", false);
		FuncEnvironment::AddNative(debug::invite, "debug invite", false);
	}

	ValueItem* start_debug(ValueItem*, uint32_t){
		initStandardLib_debug();
		return nullptr;
	}
	void initStandardLib_start_debug(){
		INIT_CHECK
		FuncEnvironment::AddNative(start_debug, "debug start", false);
	}

	void initStandardLib() {
		initStandardLib_exception();
		initStandardLib_bytes();
		initStandardLib_console();
		initStandardLib_math();
		initStandardLib_file();
		initStandardLib_parallel();
		initStandardLib_chanel();
		initStandardLib_internal();
		initStandardLib_internal_memory();
		initStandardLib_internal_run_time();
		initStandardLib_internal_run_time_native();
		initStandardLib_internal_stack();
		initStandardLib_net();
		initStandardLib_start_debug();
		initStandardLib_configuration();
	}

	void initStandardLib_safe(){
		initStandardLib_exception();
		initStandardLib_bytes();
		initStandardLib_console();
		initStandardLib_math();
		initStandardLib_file();
		initStandardLib_parallel();
		initStandardLib_chanel();
		initStandardLib_net();
		initStandardLib_internal();
	}


	ValueItem* cmath_frexp(ValueItem* args, uint32_t args_len) {
		double doub;
		if (args && args_len >= 1)
			doub = (double)args[0];
		else
			doub = 0;
		ValueItem* rs = new ValueItem[2];
		int res1;
		rs[0] = std::frexp(doub, &res1);
		rs[1] = res1;
		return new ValueItem(rs, 2, no_copy);
	}
	ValueItem* cmath_modf(ValueItem* args, uint32_t args_len) {
		double doub;
		if (args && args_len >= 1)
			doub = (double)args[0];
		else
			doub = 0;
		ValueItem* rs = new ValueItem[2];
		double res1;
		rs[0] = std::modf(doub, &res1);
		rs[1] = res1;
		return new ValueItem(rs, 2, no_copy);
	}
	ValueItem* cmath_remquo(ValueItem* args, uint32_t args_len) {
		double doub0;
		double doub1;
		if (args && args_len >= 2) {
			doub0 = (double)args[0];
			doub1 = (double)args[1];
		}
		else {
			doub0 = 0;
			doub1 = 0;
		}
		ValueItem* rs = new ValueItem[2];
		int res1;
		rs[0] = std::remquo(doub0, doub1, &res1);
		rs[1] = res1;
		return new ValueItem(rs, 2, no_copy);
	}
	ValueItem* cmath_nexttoward(ValueItem* args, uint32_t args_len) {
		float doub0;
		long double doub1;
		if (args && args_len >= 2) {
			doub0 = (float)args[0];
			doub1 = (double)args[1];
		}
		else {
			doub0 = 0;
			doub1 = 0;
		}
		return new ValueItem(std::nexttoward(doub0, doub1));
	}
	extern "C" void initCMathLib() {
		FuncEnvironment::AddNative((double(*)(double))std::acos, "cmath acos", false);
		FuncEnvironment::AddNative((double(*)(double))std::acosh, "cmath acosh", false);
		FuncEnvironment::AddNative((double(*)(double))std::asin, "cmath asin", false);
		FuncEnvironment::AddNative((double(*)(double))std::asinh, "cmath asinh", false);
		FuncEnvironment::AddNative((double(*)(double))std::atan, "cmath atan", false);
		FuncEnvironment::AddNative((double(*)(double))std::atanh, "cmath atanh", false);
		FuncEnvironment::AddNative((double(*)(double, double))std::atan2, "cmath atan2", false);
		FuncEnvironment::AddNative((double(*)(double))std::cbrt, "cmath cbrt", false);
		FuncEnvironment::AddNative((double(*)(double))std::ceil, "cmath ceil", false);
		FuncEnvironment::AddNative((double(*)(double, double))std::copysign, "cmath copysign", false);
		FuncEnvironment::AddNative((double(*)(double))std::cos, "cmath cos", false);
		FuncEnvironment::AddNative((double(*)(double))std::cosh, "cmath cosh", false);
		FuncEnvironment::AddNative((double(*)(double))std::erf, "cmath erf", false);
		FuncEnvironment::AddNative((double(*)(double))std::erfc, "cmath erfc", false);
		FuncEnvironment::AddNative((double(*)(double))std::exp, "cmath exp", false);
		FuncEnvironment::AddNative((double(*)(double))std::exp2, "cmath exp2", false);
		FuncEnvironment::AddNative((double(*)(double))std::expm1, "cmath expm1", false);
		FuncEnvironment::AddNative((double(*)(double))std::fabs, "cmath fabs", false);
		FuncEnvironment::AddNative((double(*)(double, double))std::fdim, "cmath fdim", false);
		FuncEnvironment::AddNative((double(*)(double))std::floor, "cmath floor", false);
		FuncEnvironment::AddNative((double(*)(double, double, double))std::fma, "cmath fma", false);
		FuncEnvironment::AddNative((double(*)(double, double))std::fmax, "cmath fmax", false);
		FuncEnvironment::AddNative((double(*)(double, double))std::fmin, "cmath fmin", false);
		FuncEnvironment::AddNative((double(*)(double, double))std::fmod, "cmath fmod", false);
		FuncEnvironment::AddNative(cmath_frexp, "cmath frexp", false);
		FuncEnvironment::AddNative((double(*)(double, double))std::hypot, "cmath hypot", false);
		FuncEnvironment::AddNative((int(*)(double))std::ilogb, "cmath ilogb", false);
		FuncEnvironment::AddNative((double(*)(double, int))std::ldexp, "cmath ldexp", false);
		FuncEnvironment::AddNative((double(*)(double))std::lgamma, "cmath lgamma", false);
		FuncEnvironment::AddNative((long long(*)(double))std::llrint, "cmath llrint", false);
		FuncEnvironment::AddNative((long long(*)(double))std::llround, "cmath llround", false);
		FuncEnvironment::AddNative((double(*)(double))std::log, "cmath log", false);
		FuncEnvironment::AddNative((double(*)(double))std::log10, "cmath log10", false);
		FuncEnvironment::AddNative((double(*)(double))std::log1p, "cmath log1p", false);
		FuncEnvironment::AddNative((double(*)(double))std::log2, "cmath log2", false);
		FuncEnvironment::AddNative((double(*)(double))std::logb, "cmath logb", false);
		FuncEnvironment::AddNative((long(*)(double))std::lrint, "cmath lrint", false);
		FuncEnvironment::AddNative((long(*)(double))std::lround, "cmath lround", false);
		FuncEnvironment::AddNative(cmath_modf, "cmath modf", false);
		FuncEnvironment::AddNative((double(*)(double))std::nearbyint, "cmath nearbyint", false);
		FuncEnvironment::AddNative((double(*)(double, double))std::nextafter, "cmath nextafter", false);
		FuncEnvironment::AddNative(cmath_nexttoward, "cmath nexttoward", false);
		FuncEnvironment::AddNative((double(*)(double, double))std::pow, "cmath pow", false);
		FuncEnvironment::AddNative((double(*)(double, double))std::remainder, "cmath remainder", false);
		FuncEnvironment::AddNative(cmath_remquo, "math remquo", false);
		FuncEnvironment::AddNative((double(*)(double))std::round, "cmath round", false);
		FuncEnvironment::AddNative((double(*)(double, long))std::scalbln, "cmath scalbln", false);
		FuncEnvironment::AddNative((double(*)(double, int))std::scalbn, "cmath scalbn", false);
		FuncEnvironment::AddNative((double(*)(double))std::sin, "cmath sin", false);
		FuncEnvironment::AddNative((double(*)(double))std::sinh, "cmath sinh", false);
		FuncEnvironment::AddNative((double(*)(double))std::sqrt, "cmath sqrt", false);
		FuncEnvironment::AddNative((double(*)(double))std::tan, "cmath tan", false);
		FuncEnvironment::AddNative((double(*)(double))std::tanh, "cmath tanh", false);
		FuncEnvironment::AddNative((double(*)(double))std::tgamma, "cmath tgamma", false);
		FuncEnvironment::AddNative((double(*)(double))std::trunc, "cmath trunc", false);
	}
}