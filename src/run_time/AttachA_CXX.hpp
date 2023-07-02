// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "func_enviro_builder.hpp"
#include "attacha_abi.hpp"
#include <type_traits>//std::forward
#include "../../configuration/agreement/symbols.hpp"
namespace AttachA {
	template<class ...Types>
	ValueItem cxxCall(typed_lgr<FuncEnvironment> func, Types... types) {
		ValueItem args[] = {ABI_IMPL::BVcast(types)...};
		ValueItem* res = func->syncWrapper(args, sizeof...(Types));
		if (res == nullptr)
			return {};
		ValueItem m(std::move(*res));
		delete res;
		return m;
	}
	inline ValueItem cxxCall(typed_lgr<FuncEnvironment> func) {
		ValueItem* res = func->syncWrapper(nullptr, 0);
		if (res == nullptr)
			return {};
		ValueItem m(std::move(*res));
		delete res;
		return m;
	}
	template<class ...Types>
	ValueItem cxxCall(const std::string& fun_name, Types... types) {
		return cxxCall(FuncEnvironment::enviropment(fun_name), std::forward<Types>(types)...);
	}
	inline ValueItem cxxCall(const std::string& fun_name) {
		ValueItem* res = FuncEnvironment::enviropment(fun_name)->syncWrapper(nullptr, 0);
		if (res == nullptr)
			return {};
		ValueItem m(std::move(*res));
		delete res;
		return m;
	}
	inline void excepted(ValueItem& v, VType type) {
		if (v.meta.vtype != type)
			throw InvalidArguments("Expected " + enum_to_string(type) + " got " + enum_to_string(v.meta.vtype));
	}
	inline void excepted(ValueItem& v, ValueMeta meta) {
		if (v.meta.encoded != meta.encoded){
			if(v.meta.vtype != meta.vtype)
				throw InvalidArguments("Expected " + enum_to_string(meta.vtype) + " got " + enum_to_string(v.meta.vtype));
			if(v.meta.allow_edit != meta.allow_edit)
				throw InvalidArguments("Expected " + std::string(meta.allow_edit ? "non const" : "const") + " value got, " +std::string(v.meta.allow_edit ? "non const" : "const"));
			if(v.meta.as_ref != meta.as_ref)
				throw InvalidArguments("Expected " + std::string(meta.as_ref ? "refrence" : "value") + " got, " +std::string(v.meta.as_ref ? "refrence" : "value"));
			if(v.meta.use_gc != meta.use_gc)
				throw InvalidArguments("Expected " + std::string(meta.use_gc ? "gc" : "non gc") + " got, " +std::string(v.meta.use_gc ? "gc" : "non gc"));
			if(is_raw_array(v.meta.vtype))
				if(v.meta.val_len != meta.val_len)
					throw InvalidArguments("Expected array size " + std::to_string(meta.val_len) + " got " + std::to_string(v.meta.val_len));
			throw InvalidArguments("Caught invalid encoded value, expected " + std::to_string(meta.encoded) + " got " + std::to_string(v.meta.encoded));
		}
	}
	inline void excepted_basic_array(ValueItem& v){
		if(!is_raw_array(v.meta.vtype) && v.meta.vtype != VType::faarr && v.meta.vtype != VType::saarr)
			throw InvalidArguments("Expected basic array got " + enum_to_string(v.meta.vtype));
	}
	inline void excepted_raw_array(ValueItem& v){
		if(!is_raw_array(v.meta.vtype))
			throw InvalidArguments("Expected raw array got " + enum_to_string(v.meta.vtype));
	}
	inline void excepted_array(ValueItem& v){
		if(!is_raw_array(v.meta.vtype) || v.meta.vtype == VType::uarr)
			throw InvalidArguments("Expected array got " + enum_to_string(v.meta.vtype));
	}
	
	inline void arguments_range(uint32_t argc, uint32_t min, uint32_t max) {
		if (argc < min || argc > max)
			throw InvalidArguments("Invalid arguments count, expected " + std::to_string(min) + " to " + std::to_string(max) + ", got " + std::to_string(argc));
	}
	inline void arguments_range(uint32_t argc, uint32_t min) {
		if (argc < min)
			throw InvalidArguments("Invalid arguments count, expected " + std::to_string(min) + " or more, got " + std::to_string(argc));
	}
	namespace Interface {
		inline ValueItem makeCall(ClassAccess access, Structure& c, const std::string& fun_name) {
			ValueItem arg(&c, as_refrence);
			ValueItem* res = c.table_get_dynamic(fun_name, access)(&arg, 1);
			if (res == nullptr)
				return {};
			ValueItem m(std::move(*res));
			delete res;
			return m;
		}
		inline ValueItem makeCall(ClassAccess access, ValueItem& c, const std::string& fun_name) {
			if(c.meta.vtype == VType::struct_){
				ValueItem* res = ((Structure&)c).table_get_dynamic(fun_name, access)(&c, 1);
				if (res == nullptr)
					return {};
				ValueItem m(std::move(*res));
				delete res;
				return m;
			}
			else throw InvalidArguments("Invalid type for call");
		}
		
		
		template<class ...Types>
		ValueItem makeCall(ClassAccess access, Structure& c, const std::string& fun_name, const Types&... types) {
			ValueItem args[] = { ValueItem(&c, as_refrence), ABI_IMPL::BVcast(types)... };
			ValueItem* res = c.table_get_dynamic(fun_name, access)(args, sizeof...(Types) + 1);
			if (res == nullptr)
				return {};
			ValueItem m(std::move(*res));
			delete res;
			return m;
		}
		template<class ...Types>
		ValueItem makeCall(ClassAccess access, ValueItem& c, const std::string& fun_name, const Types&... types) {
			if(c.meta.vtype == VType::struct_){
				ValueItem args[] = { ValueItem(c, as_refrence), ABI_IMPL::BVcast(types)... };
				ValueItem* res = ((Structure&)c).table_get_dynamic(fun_name, access)(args, sizeof...(Types) + 1);
				if (res == nullptr)
					return {};
				ValueItem m(std::move(*res));
				delete res;
				return m;
			}
			else throw InvalidArguments("Invalid type for call");
		}
		inline ValueItem makeCall(ClassAccess access, ValueItem& c, const std::string& fun_name, ValueItem* args, uint32_t len) {
			if(c.meta.vtype == VType::struct_){
				list_array<ValueItem> args_tmp(args, args + len, len);
				args_tmp.push_front(ValueItem(c, as_refrence));
				ValueItem* res = ((Structure&)c).table_get_dynamic(fun_name, access)(args_tmp.data(), len + 1);
				if (res == nullptr)
					return {};
				ValueItem m(std::move(*res));
				delete res;
				return m;
			}
			else throw InvalidArguments("Invalid type for call");
		}

		inline ValueItem getValue(Structure& c, const std::string& val_name) {
			return c.dynamic_value_get(val_name);
		}
		inline ValueItem getValue(ValueItem& c, const std::string& val_name) {
			switch (c.meta.vtype) {
			case VType::struct_:
				return ((Structure&)c).dynamic_value_get(val_name);
			default:
				throw NotImplementedException();
			}
		}

		inline ValueItem getValue(ClassAccess access, Structure& c, const std::string& val_name) {
			return c.dynamic_value_get(val_name);
		}
		inline ValueItem getValue(ClassAccess access, ValueItem& c, const std::string& val_name) {
			switch (c.meta.vtype) {
			case VType::struct_:
				return ((Structure&)c).dynamic_value_get(val_name);
			default:
				throw NotImplementedException();
			}
		}


		inline void setValue(Structure& c, const std::string& val_name, ValueItem& set) {
			c.dynamic_value_set(val_name, set);
		}
		inline void setValue(ValueItem& c, const std::string& val_name, ValueItem& set) {
			switch (c.meta.vtype) {
			case VType::struct_:
				((Structure&)c).dynamic_value_set(val_name, set);
				break;
			default:
				throw NotImplementedException();
			}
		}
		inline void setValue(ClassAccess access, Structure& c, const std::string& val_name, ValueItem& set) {
			c.dynamic_value_set(val_name, set);
		}
		inline void setValue(ClassAccess access, ValueItem& c, const std::string& val_name, ValueItem& set) {
			switch (c.meta.vtype) {
			case VType::struct_:
				((Structure&)c).dynamic_value_set(val_name, set);
				break;
			default:
				throw NotImplementedException();
			}
		}


		inline bool hasImplement(Structure& c, const std::string& fun_name, ClassAccess access) {
			return c.has_method(fun_name, access);
		}
		inline bool hasImplement(ValueItem& c, const std::string& fun_name, ClassAccess access = ClassAccess::pub) {
			switch (c.meta.vtype) {
			case VType::struct_:
				return ((Structure&)c).has_method(fun_name, access);
			default:
				return false;
			}
		}
		inline std::string name(Structure& c) {
			return c.get_name();
		}
		inline std::string name(ValueItem& c) {
			switch (c.meta.vtype) {
			case VType::struct_:
				return ((Structure&)c).get_name();
			default:
				return "$ Not class $";
			}
		}
		inline bool is_interface(Structure& c) {
			return true;
		}
		inline bool is_interface(ValueItem& c) {
			switch (c.meta.vtype) {
			case VType::struct_:
				return true;
			default:
				return false;
			}
		}
		template<class ...Methods>
		struct single_method{
			std::tuple<Methods...> methods;
			std::string name;
			single_method(const std::string& name, Methods... methods):name(name), methods(std::forward_as_tuple(methods...)){}
		};
		struct direct_method{
			std::string name;
			Enviropment env;
			ClassAccess access = ClassAccess::pub;
		};
		namespace _createProxyTable_Impl_{
			
			template<typename T>
			struct store_value{
				static T value;
				store_value(T v) : value(v){};
			};
			template<typename T>
			T store_value<T>::value = nullptr;

			template<class T>
			void init_value(T func){
				store_value<decltype(func)>::value = func;
			}

			
			template<typename ReturmTyp, typename ...Argumetns>
			struct static_funtion_info {
				using return_type = ReturmTyp;
				inline static ValueMeta arguments[sizeof...(Argumetns)? sizeof...(Argumetns) : 1] = { Type_as_ValueMeta<Argumetns>()... };
				using arguments_type = std::tuple<Argumetns...>;
				constexpr static size_t arguments_count = sizeof...(Argumetns);
				constexpr static bool is_static = true;
				constexpr static bool always_perfect = 
					sizeof...(Argumetns) == 2
					&& std::is_same_v<ReturmTyp, ValueItem*>
					&& std::is_same_v<std::tuple<Argumetns...>, std::tuple<ValueItem*, uint32_t>>;
			};
			

			template<typename Class_, typename ReturmTyp, typename ...Argumetns>
			struct method_funtion_info {
				using return_type = ReturmTyp;
				inline static ValueMeta arguments[sizeof...(Argumetns)? sizeof...(Argumetns) : 1] = { Type_as_ValueMeta<Argumetns>()... };
				using arguments_type = std::tuple<Argumetns...>;
				using class_type = Class_;
				constexpr static size_t arguments_count = sizeof...(Argumetns);
				constexpr static bool is_static = false;
				constexpr static bool always_perfect = 
					sizeof...(Argumetns) == 2
					&& std::is_same_v<ReturmTyp, ValueItem*>
					&& std::is_same_v<std::tuple<Argumetns...>, std::tuple<ValueItem*, uint32_t>>;
			};
			

			template<typename> struct funtion_info {};
			template<typename ReturmTyp, typename ...Argumetns>
			struct funtion_info<ReturmTyp(Argumetns...)> : static_funtion_info<ReturmTyp, Argumetns...> {};
			template<typename ReturmTyp, typename ...Argumetns>
			struct funtion_info<ReturmTyp(Argumetns...) &>  : static_funtion_info<ReturmTyp, Argumetns...> {};
			template<typename ReturmTyp, typename ...Argumetns>
			struct funtion_info<ReturmTyp(Argumetns...) &&>  : static_funtion_info<ReturmTyp, Argumetns...> {};
			template<typename ReturmTyp, typename ...Argumetns>
			struct funtion_info<ReturmTyp(Argumetns...) const> : static_funtion_info<ReturmTyp, Argumetns...> {};
			template<typename ReturmTyp, typename ...Argumetns>
			struct funtion_info<ReturmTyp(Argumetns...) const &> : static_funtion_info<ReturmTyp, Argumetns...> {};
			template<typename ReturmTyp, typename ...Argumetns>
			struct funtion_info<ReturmTyp(Argumetns...) const &&> : static_funtion_info<ReturmTyp, Argumetns...> {};
			template<typename ReturmTyp, typename ...Argumetns>
			struct funtion_info<ReturmTyp(Argumetns...) volatile> : static_funtion_info<ReturmTyp, Argumetns...> {};
			template<typename ReturmTyp, typename ...Argumetns>
			struct funtion_info<ReturmTyp(Argumetns...) volatile &> : static_funtion_info<ReturmTyp, Argumetns...> {};
			template<typename ReturmTyp, typename ...Argumetns>
			struct funtion_info<ReturmTyp(Argumetns...) volatile &&> : static_funtion_info<ReturmTyp, Argumetns...> {};
			template<typename ReturmTyp, typename ...Argumetns>
			struct funtion_info<ReturmTyp(Argumetns...) const volatile> : static_funtion_info<ReturmTyp, Argumetns...> {};
			template<typename ReturmTyp, typename ...Argumetns>
			struct funtion_info<ReturmTyp(Argumetns...) const volatile &> : static_funtion_info<ReturmTyp, Argumetns...> {};
			template<typename ReturmTyp, typename ...Argumetns>
			struct funtion_info<ReturmTyp(Argumetns...) const volatile &&> : static_funtion_info<ReturmTyp, Argumetns...> {};



			template<typename Class_, typename ReturmTyp, typename ...Argumetns>
			struct funtion_info<ReturmTyp(Class_::*)(Argumetns...)> : method_funtion_info<Class_, ReturmTyp, Argumetns...> {};
			template<typename Class_, typename ReturmTyp, typename ...Argumetns>
			struct funtion_info<ReturmTyp(Class_::*)(Argumetns...) &> : method_funtion_info<Class_, ReturmTyp, Argumetns...> {};
			template<typename Class_, typename ReturmTyp, typename ...Argumetns>
			struct funtion_info<ReturmTyp(Class_::*)(Argumetns...) &&> : method_funtion_info<Class_, ReturmTyp, Argumetns...> {};
			template<typename Class_, typename ReturmTyp, typename ...Argumetns>
			struct funtion_info<ReturmTyp(Class_::*)(Argumetns...) const> : method_funtion_info<Class_, ReturmTyp, Argumetns...> {};
			template<typename Class_, typename ReturmTyp, typename ...Argumetns>
			struct funtion_info<ReturmTyp(Class_::*)(Argumetns...) const &> : method_funtion_info<Class_, ReturmTyp, Argumetns...> {};
			template<typename Class_, typename ReturmTyp, typename ...Argumetns>
			struct funtion_info<ReturmTyp(Class_::*)(Argumetns...) const &&> : method_funtion_info<Class_, ReturmTyp, Argumetns...> {};
			template<typename Class_, typename ReturmTyp, typename ...Argumetns>
			struct funtion_info<ReturmTyp(Class_::*)(Argumetns...) volatile> : method_funtion_info<Class_, ReturmTyp, Argumetns...> {};
			template<typename Class_, typename ReturmTyp, typename ...Argumetns>
			struct funtion_info<ReturmTyp(Class_::*)(Argumetns...) volatile &> : method_funtion_info<Class_, ReturmTyp, Argumetns...> {};
			template<typename Class_, typename ReturmTyp, typename ...Argumetns>
			struct funtion_info<ReturmTyp(Class_::*)(Argumetns...) volatile &&> : method_funtion_info<Class_, ReturmTyp, Argumetns...> {};
			template<typename Class_, typename ReturmTyp, typename ...Argumetns>
			struct funtion_info<ReturmTyp(Class_::*)(Argumetns...) const volatile> : method_funtion_info<Class_, ReturmTyp, Argumetns...> {};
			template<typename Class_, typename ReturmTyp, typename ...Argumetns>
			struct funtion_info<ReturmTyp(Class_::*)(Argumetns...) const volatile &> : method_funtion_info<Class_, ReturmTyp, Argumetns...> {};
			template<typename Class_, typename ReturmTyp, typename ...Argumetns>
			struct funtion_info<ReturmTyp(Class_::*)(Argumetns...) const volatile &&> : method_funtion_info<Class_, ReturmTyp, Argumetns...> {};


	
			template<typename ReturmTyp>
			struct funtion_info<ReturmTyp()> : static_funtion_info<ReturmTyp> {};
			template<typename ReturmTyp>
			struct funtion_info<ReturmTyp() &>  : static_funtion_info<ReturmTyp> {};
			template<typename ReturmTyp>
			struct funtion_info<ReturmTyp() &&>  : static_funtion_info<ReturmTyp> {};
			template<typename ReturmTyp>
			struct funtion_info<ReturmTyp() const> : static_funtion_info<ReturmTyp> {};
			template<typename ReturmTyp>
			struct funtion_info<ReturmTyp() const &> : static_funtion_info<ReturmTyp> {};
			template<typename ReturmTyp>
			struct funtion_info<ReturmTyp() const &&> : static_funtion_info<ReturmTyp> {};
			template<typename ReturmTyp>
			struct funtion_info<ReturmTyp() volatile> : static_funtion_info<ReturmTyp> {};
			template<typename ReturmTyp>
			struct funtion_info<ReturmTyp() volatile &> : static_funtion_info<ReturmTyp> {};
			template<typename ReturmTyp>
			struct funtion_info<ReturmTyp() volatile &&> : static_funtion_info<ReturmTyp> {};
			template<typename ReturmTyp>
			struct funtion_info<ReturmTyp() const volatile> : static_funtion_info<ReturmTyp> {};
			template<typename ReturmTyp>
			struct funtion_info<ReturmTyp() const volatile &> : static_funtion_info<ReturmTyp> {};
			template<typename ReturmTyp>
			struct funtion_info<ReturmTyp() const volatile &&> : static_funtion_info<ReturmTyp> {};



			template<typename Class_, typename ReturmTyp>
			struct funtion_info<ReturmTyp(Class_::*)()> : method_funtion_info<Class_, ReturmTyp> {};
			template<typename Class_, typename ReturmTyp>
			struct funtion_info<ReturmTyp(Class_::*)() &> : method_funtion_info<Class_, ReturmTyp> {};
			template<typename Class_, typename ReturmTyp>
			struct funtion_info<ReturmTyp(Class_::*)() &&> : method_funtion_info<Class_, ReturmTyp> {};
			template<typename Class_, typename ReturmTyp>
			struct funtion_info<ReturmTyp(Class_::*)() const> : method_funtion_info<Class_, ReturmTyp> {};
			template<typename Class_, typename ReturmTyp>
			struct funtion_info<ReturmTyp(Class_::*)() const &> : method_funtion_info<Class_, ReturmTyp> {};
			template<typename Class_, typename ReturmTyp>
			struct funtion_info<ReturmTyp(Class_::*)() const &&> : method_funtion_info<Class_, ReturmTyp> {};
			template<typename Class_, typename ReturmTyp>
			struct funtion_info<ReturmTyp(Class_::*)() volatile> : method_funtion_info<Class_, ReturmTyp> {};
			template<typename Class_, typename ReturmTyp>
			struct funtion_info<ReturmTyp(Class_::*)() volatile &> : method_funtion_info<Class_, ReturmTyp> {};
			template<typename Class_, typename ReturmTyp>
			struct funtion_info<ReturmTyp(Class_::*)() volatile &&> : method_funtion_info<Class_, ReturmTyp> {};
			template<typename Class_, typename ReturmTyp>
			struct funtion_info<ReturmTyp(Class_::*)() const volatile> : method_funtion_info<Class_, ReturmTyp> {};
			template<typename Class_, typename ReturmTyp>
			struct funtion_info<ReturmTyp(Class_::*)() const volatile &> : method_funtion_info<Class_, ReturmTyp> {};
			template<typename Class_, typename ReturmTyp>
			struct funtion_info<ReturmTyp(Class_::*)() const volatile &&> : method_funtion_info<Class_, ReturmTyp> {};


			template<class Method, size_t i>
			inline std::pair<size_t,size_t> _proceed__find_best_method(ValueItem* args, uint32_t len) {
				using method_info = funtion_info<decltype(Method::value)>;
				if (method_info::arguments_count == len) {
					size_t score = 0;
					for (size_t k = 0; k < len; k++) {
						if (method_info::arguments[k].encoded == args[k].meta.encoded) {
							score++;
						}
					}
					return { i,score };
				}
				else if(method_info::always_perfect)
					return { i,len };
				return { 0,0 };
			}
			template<size_t index, class Head, class ...Methods>
			void _iterate__find_best_method(std::pair<size_t,size_t>& res, ValueItem* args, uint32_t len) {
				auto [method, score] = _proceed__find_best_method<Head, index>(args, len);
				if (score == len){
					res = { method,score };
					return;
				}
				if constexpr (sizeof...(Methods) > 0)
					_iterate__find_best_method<index+1, Methods...>(res, args, len);
			}
			template<class ...Methods>
			size_t _find_best_method_index(ValueItem* args, uint32_t len) {
				std::pair<size_t, size_t> res = { -1,0 };
				_iterate__find_best_method<0, Methods...>(res, args, len);
				return res.first;
			}
			

			template<class Class_,class Method>
			ValueItem* callMethod(ValueItem* args, uint32_t len) {
				using method_info = funtion_info<decltype(Method::value)>;
				if constexpr(method_info::is_static){
					if constexpr(method_info::always_perfect)
						return Method::value(args + 1, len - 1);
					else {
						size_t i = 0;
						DynamicCall::FunctionTemplate template_fun;
						DynamicCall::buildFn(Method::value,template_fun);
						auto fnptr = Method::value; 
						DynamicCall::PROC as_proc = reinterpret_cast<DynamicCall::PROC&>(fnptr);
						DynamicCall::FunctionCall call(as_proc,template_fun, true);
						return NativeProxy_DynamicToStatic(call, template_fun, args, len);
					}
				}else{
					auto& s = (Structure&)*args;
					if constexpr(method_info::always_perfect)
						return (((Class_*)s.get_data_no_vtable())->*Method::value)(args + 1, len - 1);
					else {
						size_t i = 0;
						DynamicCall::FunctionTemplate template_fun;
						DynamicCall::buildFn(Method::value,template_fun);
						auto fnptr = Method::value; 
						DynamicCall::PROC as_proc = reinterpret_cast<DynamicCall::PROC&>(fnptr);
						DynamicCall::FunctionCall call(as_proc, s.get_data_no_vtable(),template_fun, true);
						return NativeProxy_DynamicToStatic(call, template_fun, args, len);
					}
				}
			}
			
			template<class Class_,class ...Methods>
			ValueItem* selectorMethod(ValueItem* args, uint32_t len, size_t method_index) {
				bool called = false;
				ValueItem* res = nullptr;
				size_t i = 0;
				([&](){
					if(method_index == i && !called){
						res = callMethod<Class_,Methods>(args,len);
						called = true;
					}
				}(),...);
				return called ? res : throw InvalidArguments("Fail automaticaly find best method for proxy");
			}
			//will be passed store_value<T> in Methods type
			template<class Class_, class ...Methods>
			inline ValueItem* proxyMethod(ValueItem* args, uint32_t len) {
				if constexpr (sizeof...(Methods) == 0) {
					return nullptr;
				}else{
					size_t best_method = _find_best_method_index<Methods...>(args, len);
					if (best_method != (size_t)-1) 
						return selectorMethod<Class_, Methods...>(args, len, best_method);
					else
						throw InvalidArguments("Fail automaticaly find best method for proxy");
				}
			}

			template<class Tuple,size_t tuple_len, size_t i = 0>
			//iterate over tuple types
			inline void tuple_to_ValueMeta(list_array<ValueMeta>& res) {
				if constexpr (tuple_len > 0) {
					using type = typename std::tuple_element<i, Tuple>::type;
					res.push_back(Type_as_ValueMeta<type>());
					tuple_to_ValueMeta<Tuple, tuple_len - 1, i+1>(res);
				}
			}
			template<size_t tuple_len,class Tuple, size_t i = 0>
			inline void init_tuple(Tuple& tuple) {
				if constexpr (tuple_len > 0) {
					init_value(std::get<i>(tuple));
					init_tuple<tuple_len - 1,Tuple, i+1>(tuple);
				}
			}
			
			template<class Class_,class ...Methods>
			MethodInfo createMethodInfo(const single_method<Methods...>& method){
				list_array<
					std::pair<
						ValueMeta, 
						list_array<ValueMeta>
					>
				> functtions_meta = {
					([&]() {
						using method_info = funtion_info<Methods>;
						return std::pair<ValueMeta, list_array<ValueMeta>>{
							Type_as_ValueMeta<method_info::return_type>(),
							([&]() -> list_array<ValueMeta> {
								list_array<ValueMeta> args;
								args.reserve_push_back(method_info::arguments_count);
								tuple_to_ValueMeta<method_info::arguments_type, method_info::arguments_count>(args);
								return args;
							}())
						};
					}())...
				};
				list_array<ValueMeta> return_values = functtions_meta.convert<ValueMeta>(
					[](const std::pair<ValueMeta, list_array<ValueMeta>>& v) {return v.first; }
				);
				list_array<list_array<ValueMeta>> arguments = functtions_meta.convert<list_array<ValueMeta>>(
					[](const std::pair<ValueMeta, list_array<ValueMeta>>& v) {return v.second; }
				);
				init_tuple<sizeof...(Methods)>(method.methods);
				return MethodInfo(
					method.name,
					proxyMethod<Class_,store_value<Methods>...>,
					ClassAccess::pub,
					return_values,
					arguments,
					list_array<MethodTag>(),
					""
				);
			}
			
			template<class To_compare, class Head, class... Tail>
			struct are_same : std::bool_constant<
				std::conjunction<std::is_same<Head, Tail>...>::value
				&& std::is_same_v<To_compare, Head>
			>{};

			template<class Class_>
			typename std::enable_if<std::is_destructible_v<Class_>, ValueItem*>::type
			_destructor(ValueItem* args, uint32_t){
				auto& s = (Structure&)*args;
				((Class_*)s.get_data_no_vtable())->~Class_();
				return nullptr;
			}
			template<class Class_>
			auto destructor = [](){
				if constexpr(std::is_destructible_v<Class_>)
					return _destructor<Class_>;
				else
					return nullptr;
			}();

			template<class Class_> requires std::copyable<Class_>
			ValueItem*_copy(ValueItem* args, uint32_t){
				auto& dest = (Structure&)*args;
				auto& src = (Structure&)*(args + 1);
				if(dest.get_vtable() != src.get_vtable())
					return nullptr;
				bool in_constructor = (bool)args[2];
				if(in_constructor)
					new(dest.get_data_no_vtable()) Class_(*(Class_*)src.get_data_no_vtable());
				else
					*(Class_*)dest.get_data_no_vtable() = *(Class_*)src.get_data_no_vtable();
				return nullptr;
			}
			template<class Class_>
			auto copy = [](){
				if constexpr(std::is_copy_constructible_v<Class_> && std::is_copy_assignable_v<Class_>)
					return _copy<Class_>;
				else
					return nullptr;
			}();
			template<class Class_> requires std::movable<Class_>
			ValueItem* _move(ValueItem* args, uint32_t){
				auto& dest = (Structure&)*args;
				auto& src = (Structure&)*(args + 1);
				if(dest.get_vtable() != src.get_vtable())
					return nullptr;

				bool in_constructor = (bool)args[2];
				if(in_constructor)
					new(dest.get_data_no_vtable()) Class_(std::move(*(Class_*)src.get_data_no_vtable()));
				else
					*(Class_*)dest.get_data_no_vtable() = std::move(*(Class_*)src.get_data_no_vtable());
				return nullptr;
			}

			template<class Class_>
			auto move = [](){
				if constexpr(std::movable<Class_>)
					return _move<Class_>;
				else
					return nullptr;
			}();
			
			template<class Class_> requires std::equality_comparable<Class_> && std::totally_ordered<Class_>
			ValueItem* _compare(ValueItem* args, uint32_t){
				auto& dest = (Structure&)*args;
				auto& src = (Structure&)*(args + 1);
				if(dest.get_vtable() != src.get_vtable())
					return new ValueItem((int8_t)-1);

				if(*(Class_*)dest.get_data_no_vtable() == *(Class_*)src.get_data_no_vtable())
					return nullptr;
				else if(*(Class_*)dest.get_data_no_vtable() < *(Class_*)src.get_data_no_vtable())
					return new ValueItem((int8_t)-1);
				else
					return new ValueItem((int8_t)1);
			}
			template<class Class_> requires std::equality_comparable<Class_> && (!std::totally_ordered<Class_>)
			ValueItem* _compare(ValueItem* args, uint32_t){
				auto& dest = (Structure&)*args;
				auto& src = (Structure&)*(args + 1);
				if(dest.get_vtable() != src.get_vtable())
					return new ValueItem((int8_t)-1);
				
				if(*(Class_*)dest.get_data_no_vtable() == *(Class_*)src.get_data_no_vtable())
					return nullptr;
				else
					return new ValueItem((int8_t)-1);
			}
			template<class Class_> requires (!std::equality_comparable<Class_> && !std::totally_ordered<Class_>)
			ValueItem* _compare(ValueItem* args, uint32_t){
				auto& dest = (Structure&)*args;
				auto& src = (Structure&)args[1];
				if(dest.get_vtable() != src.get_vtable())
					return new ValueItem((int8_t)-1);
				
				if(dest.has_method(symbols::structures::less_operator, ClassAccess::pub)){
					if((bool)makeCall(ClassAccess::pub, dest, symbols::structures::less_operator, args + 1, 1))
						return new ValueItem((int8_t)-1);
					else{
						if(src.has_method(symbols::structures::greater_operator, ClassAccess::pub)){
							if((bool)makeCall(ClassAccess::pub, dest, symbols::structures::less_operator, args + 1, 1))
								return new ValueItem((int8_t)-1);
							else 
								return nullptr;
						}
						else
							return args[0].getSourcePtr() == args[1].getSourcePtr() ? nullptr : new ValueItem((int8_t)-1);
					}
				}else
					return args[0].getSourcePtr() == args[1].getSourcePtr() ? nullptr : new ValueItem((int8_t)-1);
			}
		
			template<class Class_>
			auto compare = [](){
				if constexpr(std::equality_comparable<Class_>)
					return _compare<Class_>;
				else
					return nullptr;
			}();

	
			template<class Class_>
			static typed_lgr<FuncEnvironment> ref_destructor = _createProxyTable_Impl_::destructor<Class_> ? new FuncEnvironment(_createProxyTable_Impl_::destructor<Class_>, false) : nullptr;
			template<class Class_>
			static typed_lgr<FuncEnvironment> ref_copy = _createProxyTable_Impl_::copy<Class_> ? new FuncEnvironment(_createProxyTable_Impl_::copy<Class_>, false) : nullptr;
			template<class Class_>
			static typed_lgr<FuncEnvironment> ref_move = _createProxyTable_Impl_::move<Class_> ? new FuncEnvironment(_createProxyTable_Impl_::move<Class_>, false) : nullptr;
			template<class Class_>
			static typed_lgr<FuncEnvironment> ref_compare = _createProxyTable_Impl_::compare<Class_> ? new FuncEnvironment(_createProxyTable_Impl_::compare<Class_>, false) : nullptr;
		}
		template<class Class_,class ...Methods>
		MethodInfo make_method(const std::string& name, Methods... methods) {
			return _createProxyTable_Impl_::createMethodInfo<Class_>(single_method(name, methods...));
		}

		template<class Class_, class ...Methods>
		//std::enable_if Methods is MethodInfos
		typename std::enable_if<
		_createProxyTable_Impl_::are_same<MethodInfo,Methods...>::value
		,AttachADynamicVirtualTable*>::type
		createProxyDTable(Methods... methods){
			list_array<MethodInfo> proxed;
			proxed.resize(sizeof...(Methods));
			size_t i = 0;
			std::string owner_name = typeid(Class_).name();
			([&](){
				proxed[i].owner_name = owner_name;
				proxed[i++] = methods;
			}(),...);

			auto res = new AttachADynamicVirtualTable(
				proxed,
				_createProxyTable_Impl_::ref_destructor<Class_>,
				_createProxyTable_Impl_::ref_copy<Class_>,
				_createProxyTable_Impl_::ref_move<Class_>,
				_createProxyTable_Impl_::ref_compare<Class_>
			);
			res->name = owner_name;
			return res;
		}
		template<class Class_, class ...Methods>
		//std::enable_if Methods is MethodInfos
		typename std::enable_if<_createProxyTable_Impl_::are_same<MethodInfo,Methods...>::value,AttachAVirtualTable*>::type
		createProxyTable(Methods... methods){
			list_array<MethodInfo> proxed;
			proxed.resize(sizeof...(Methods));
			size_t i = 0;
			std::string owner_name = typeid(Class_).name();
			([&](){
				proxed[i].owner_name = owner_name;
				proxed[i++] = methods;
			}(),...);
			auto res = AttachAVirtualTable::create(
				proxed,
				_createProxyTable_Impl_::ref_destructor<Class_>,
				_createProxyTable_Impl_::ref_copy<Class_>,
				_createProxyTable_Impl_::ref_move<Class_>,
				_createProxyTable_Impl_::ref_compare<Class_>
			);
			res->setName(owner_name);
			return res;
		}
		template<class Class_, class ...DirectMethod>
		typename std::enable_if<_createProxyTable_Impl_::are_same<direct_method,DirectMethod...>::value,AttachADynamicVirtualTable*>::type
		createDTable(DirectMethod... methods){
			list_array<MethodInfo> proxed;
			proxed.resize(sizeof...(DirectMethod));
			size_t i = 0;
			std::string owner_name = typeid(Class_).name();
			([&](){
				proxed[i].owner_name = owner_name;
				proxed[i].name = methods.name;
				proxed[i].access = methods.access;
				proxed[i++].ref = new FuncEnvironment(methods.env,false);
			}(),...);

			auto res = new AttachADynamicVirtualTable(
				proxed,
				_createProxyTable_Impl_::ref_destructor<Class_>,
				_createProxyTable_Impl_::ref_copy<Class_>,
				_createProxyTable_Impl_::ref_move<Class_>,
				_createProxyTable_Impl_::ref_compare<Class_>
			);
			res->name = owner_name;
			return res;
		}
		template<class Class_, class ...DirectMethod>
		typename std::enable_if<_createProxyTable_Impl_::are_same<direct_method,DirectMethod...>::value,AttachAVirtualTable*>::type
		createTable(DirectMethod... methods){
			list_array<MethodInfo> proxed;
			proxed.resize(sizeof...(DirectMethod));
			size_t i = 0;
			std::string owner_name = typeid(Class_).name();
			([&](){
				proxed[i].owner_name = owner_name;
				proxed[i].name = methods.name;
				proxed[i].access = methods.access;
				proxed[i++].ref = new FuncEnvironment(methods.env,false);
			}(),...);
			auto res = AttachAVirtualTable::create(
				proxed,
				_createProxyTable_Impl_::ref_destructor<Class_>,
				_createProxyTable_Impl_::ref_copy<Class_>,
				_createProxyTable_Impl_::ref_move<Class_>,
				_createProxyTable_Impl_::ref_compare<Class_>
			);
			res->setName(owner_name);
			return res;
		}

		template<class Class_, class ...Methods>
		typename std::enable_if<
		_createProxyTable_Impl_::are_same<MethodInfo,Methods...>::value
		,AttachADynamicVirtualTable*>::type
		createProxyDTable(const std::string& owner_name, Methods... methods){
			list_array<MethodInfo> proxed;
			proxed.resize(sizeof...(Methods));
			size_t i = 0;
			([&](){
				proxed[i].owner_name = owner_name;
				proxed[i++] = methods;
			}(),...);
			auto res = new AttachADynamicVirtualTable(
				proxed,
				_createProxyTable_Impl_::ref_destructor<Class_>,
				_createProxyTable_Impl_::ref_copy<Class_>,
				_createProxyTable_Impl_::ref_move<Class_>,
				_createProxyTable_Impl_::ref_compare<Class_>
			);
			res->name = owner_name;
			return res;
		}
		template<class Class_, class ...Methods>
		typename std::enable_if<_createProxyTable_Impl_::are_same<MethodInfo,Methods...>::value,AttachAVirtualTable*>::type
		createProxyTable(const std::string& owner_name, Methods... methods){
			list_array<MethodInfo> proxed;
			proxed.resize(sizeof...(Methods));
			size_t i = 0;
			([&](){
				proxed[i].owner_name = owner_name;
				proxed[i++] = methods;
			}(),...);
			auto res = AttachAVirtualTable::create(
				proxed,
				_createProxyTable_Impl_::ref_destructor<Class_>,
				_createProxyTable_Impl_::ref_copy<Class_>,
				_createProxyTable_Impl_::ref_move<Class_>,
				_createProxyTable_Impl_::ref_compare<Class_>
			);
			res->setName(owner_name);
			return res;
		}
		template<class Class_, class ...DirectMethod>
		typename std::enable_if<_createProxyTable_Impl_::are_same<direct_method,DirectMethod...>::value,AttachADynamicVirtualTable*>::type
		createDTable(const std::string& owner_name, DirectMethod... methods){
			list_array<MethodInfo> proxed;
			proxed.resize(sizeof...(DirectMethod));
			size_t i = 0;
			([&](){
				proxed[i].owner_name = owner_name;
				proxed[i].name = methods.name;
				proxed[i].access = methods.access;
				proxed[i++].ref = new FuncEnvironment(methods.env,false);
			}(),...);

			auto res = new AttachADynamicVirtualTable(
				proxed,
				_createProxyTable_Impl_::ref_destructor<Class_>,
				_createProxyTable_Impl_::ref_copy<Class_>,
				_createProxyTable_Impl_::ref_move<Class_>,
				_createProxyTable_Impl_::ref_compare<Class_>
			);
			res->name = owner_name;
			return res;
		}
		template<class Class_, class ...DirectMethod>
		typename std::enable_if<_createProxyTable_Impl_::are_same<direct_method,DirectMethod...>::value,AttachAVirtualTable*>::type
		createTable(const std::string& owner_name, DirectMethod... methods){
			list_array<MethodInfo> proxed;
			proxed.resize(sizeof...(DirectMethod));
			size_t i = 0;
			([&](){
				proxed[i].owner_name = owner_name;
				proxed[i].name = methods.name;
				proxed[i].access = methods.access;
				proxed[i++].ref = new FuncEnvironment(methods.env,false);
			}(),...);

			auto res = AttachAVirtualTable::create(
				proxed,
				_createProxyTable_Impl_::ref_destructor<Class_>,
				_createProxyTable_Impl_::ref_copy<Class_>,
				_createProxyTable_Impl_::ref_move<Class_>,
				_createProxyTable_Impl_::ref_compare<Class_>
			);
			res->setName(owner_name);
			return res;
		}
		
		template<class Class_>
		AttachADynamicVirtualTable* createProxyDTable(const std::string& owner_name){
			list_array<MethodInfo> proxed;
			auto res = new AttachADynamicVirtualTable(
				proxed,
				_createProxyTable_Impl_::ref_destructor<Class_>,
				_createProxyTable_Impl_::ref_copy<Class_>,
				_createProxyTable_Impl_::ref_move<Class_>,
				_createProxyTable_Impl_::ref_compare<Class_>
			);
			res->name = owner_name;
			return res;
		}
		template<class Class_>
		AttachAVirtualTable* createProxyTable(const std::string& owner_name){
			list_array<MethodInfo> proxed;
			auto res = AttachAVirtualTable::create(
				proxed,
				_createProxyTable_Impl_::ref_destructor<Class_>,
				_createProxyTable_Impl_::ref_copy<Class_>,
				_createProxyTable_Impl_::ref_move<Class_>,
				_createProxyTable_Impl_::ref_compare<Class_>
			);
			res->setName(owner_name);
			return res;
		}
		template<class Class_>
		AttachADynamicVirtualTable* createDTable(const std::string& owner_name){
			list_array<MethodInfo> proxed;
			auto res = new AttachADynamicVirtualTable(
				proxed,
				_createProxyTable_Impl_::ref_destructor<Class_>,
				_createProxyTable_Impl_::ref_copy<Class_>,
				_createProxyTable_Impl_::ref_move<Class_>,
				_createProxyTable_Impl_::ref_compare<Class_>
			);
			res->name = owner_name;
			return res;
		}
		template<class Class_>
		AttachAVirtualTable* createTable(const std::string& owner_name){
			list_array<MethodInfo> proxed;
			auto res = AttachAVirtualTable::create(
				proxed,
				_createProxyTable_Impl_::ref_destructor<Class_>,
				_createProxyTable_Impl_::ref_copy<Class_>,
				_createProxyTable_Impl_::ref_move<Class_>,
				_createProxyTable_Impl_::ref_compare<Class_>
			);
			res->setName(owner_name);
			return res;
		}


		template<class Class_, class ...Args>
		Structure* constructStructure(AttachAVirtualTable* table,Args&&... args){
			Structure* str = Structure::construct(sizeof(Class_) + 8, nullptr, 0, table, Structure::VTableMode::AttachAVirtualTable);
			new(str->get_data_no_vtable()) Class_(std::forward<Args>(args)...);
			str->fully_constructed = true;
			return str;
		}
		template<class Class_, class ...Args>
		Structure* constructStructure(AttachADynamicVirtualTable* table,Args&&... args){
			Structure* str = Structure::construct(sizeof(Class_) + 8, nullptr, 0, table, Structure::VTableMode::AttachADynamicVirtualTable);
			new(str->get_data_no_vtable()) Class_(std::forward<Args>(args)...);
			str->fully_constructed = true;
			return str;
		}

		template<class Class_>
		Class_& getAs(Structure& str){
			return *(Class_*)str.get_data_no_vtable();
		}
		template<class Class_>
		Class_& getAs(ValueItem& str){
			if(str.meta.vtype == VType::struct_){
				return *(Class_*)((Structure&)str).get_data_no_vtable();
			}else
				throw InvalidArguments("getAs: ValueItem is not a struct");
		}

		template<class Class_>
		Class_& getExtractAs(Structure& proxy, AttachADynamicVirtualTable* vtable){
			if(proxy.get_vtable() != vtable){
				if(proxy.get_name() != vtable->name)
					throw InvalidArguments(vtable->name + ", excepted " + vtable->name +", got " + proxy.get_name());
				else
					throw InvalidArguments(vtable->name + ", excepted " + vtable->name+ ", got non native" + vtable->name);
			}
			return AttachA::Interface::getAs<Class_>(proxy);
		}
		template<class Class_>
		Class_& getExtractAs(ValueItem& str, AttachADynamicVirtualTable* vtable){
			if(str.meta.vtype == VType::struct_){
				Structure& proxy = (Structure&)str;
				if(proxy.get_vtable() != vtable){
					if(proxy.get_name() != vtable->name)
						throw InvalidArguments(vtable->name + ", excepted " + vtable->name +", got " + proxy.get_name());
					else
						throw InvalidArguments(vtable->name + ", excepted " + vtable->name+ ", got non native" + vtable->name);
				}
				return AttachA::Interface::getAs<Class_>(proxy);
			}else
				throw InvalidArguments(vtable->name + ", type missmatch, excepted struct_, got " + enum_to_string(str.meta.vtype));
		}
		template<class Class_>
		Class_& getExtractAs(Structure& proxy, AttachAVirtualTable* vtable){
			if(proxy.get_vtable() != vtable){
				if(proxy.get_name() != vtable->getName())
					throw InvalidArguments(vtable->getName() + ", excepted " + vtable->getName() +", got " + proxy.get_name());
				else
					throw InvalidArguments(vtable->getName() + ", excepted " + vtable->getName()+ ", got non native" + vtable->getName());
			}
			return AttachA::Interface::getAs<Class_>(proxy);
		}
		template<class Class_>
		Class_& getExtractAs(ValueItem& str, AttachAVirtualTable* vtable){
			if(str.meta.vtype == VType::struct_){
				Structure& proxy = (Structure&)str;
				if(proxy.get_vtable() != vtable){
					if(proxy.get_name() != vtable->getName())
						throw InvalidArguments(vtable->getName() + ", excepted " + vtable->getName() +", got " + proxy.get_name());
					else
						throw InvalidArguments(vtable->getName() + ", excepted " + vtable->getName()+ ", got non native" + vtable->getName());
				}
				return AttachA::Interface::getAs<Class_>(proxy);
			}else
				throw InvalidArguments(vtable->getName() + ", type missmatch, excepted struct_, got " + enum_to_string(str.meta.vtype));
		}

		template<class Class_>
		void*& typeVTable(){
			static void* hold = nullptr;
			return hold;
		}

		template<class Class_>
		Class_& getExtractAsDynamic(Structure& proxy){
			return AttachA::Interface::getExtractAs<Class_>(proxy, (AttachADynamicVirtualTable*)typeVTable<Class_>());
		}
		template<class Class_>
		Class_& getExtractAsDynamic(ValueItem& str){
			return AttachA::Interface::getExtractAs<Class_>(str, (AttachADynamicVirtualTable*)typeVTable<Class_>());
		}
		template<class Class_>
		Class_& getExtractAsStatic(Structure& proxy){
			return AttachA::Interface::getExtractAs<Class_>(proxy, (AttachAVirtualTable*)typeVTable<Class_>());
		}
		template<class Class_>
		Class_& getExtractAsStatic(ValueItem& str){
			return AttachA::Interface::getExtractAs<Class_>(str, (AttachAVirtualTable*)typeVTable<Class_>());
		}
	}
}

#define AttachAFun(name, min_args, content)\
	ValueItem* name(ValueItem* args, uint32_t len)\
	{\
		if(len >= min_args) \
			return new ValueItem([&]() -> ValueItem{\
				do\
				content\
				while(false);\
				return nullptr;\
			}());\
		else\
			AttachA::arguments_range(len, min_args);\
		return nullptr;\
	}
#define AttachAManagedFun(name, min_args, content)\
	ValueItem* name(ValueItem* args, uint32_t len)\
	{\
		if(len >= min_args){ \
			do\
			content\
			while(false);\
		}else\
			AttachA::arguments_range(len, min_args);\
		return nullptr;\
	}