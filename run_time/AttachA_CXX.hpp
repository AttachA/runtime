// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "run_time_compiler.hpp"
#include "attacha_abi.hpp"
namespace AttachA {
	template<class ...Types>
	ValueItem cxxCall(typed_lgr<FuncEnviropment> func, Types... types) {
		ValueItem args[] = {ABI_IMPL::BVcast(types)...};
		ValueItem* res = func->syncWrapper(args, sizeof(args) / sizeof(ValueItem));
		if (res == nullptr)
			return {};
		ValueItem m(std::move(*res));
		delete res;
		return m;
	}
	inline ValueItem cxxCall(typed_lgr<FuncEnviropment> func) {
		ValueItem* res = func->syncWrapper(nullptr, 0);
		if (res == nullptr)
			return {};
		ValueItem m(std::move(*res));
		delete res;
		return m;
	}
	template<class ...Types>
	ValueItem cxxCall(const std::string& fun_name, Types... types) {
		return cxxCall(FuncEnviropment::enviropment(fun_name), std::forward<Types>(types)...);
	}
	inline ValueItem cxxCall(const std::string& fun_name) {
		ValueItem* res = FuncEnviropment::enviropment(fun_name)->syncWrapper(nullptr, 0);
		if (res == nullptr)
			return {};
		ValueItem m(std::move(*res));
		delete res;
		return m;
	}


	namespace Interface {
		inline ValueItem makeCall(ClassAccess access, ClassValue& c, const std::string& fun_name) {
			ValueItem arg(&c, VType::class_, as_refrence);
			ValueItem* res = c.callFnPtr(fun_name, access)->syncWrapper(&arg, 1);
			if (res == nullptr)
				return {};
			ValueItem m(std::move(*res));
			delete res;
			return m;
		}
		template<class ...Types>
		ValueItem makeCall(ClassAccess access, ClassValue& c, const std::string& fun_name, const Types&... types) {
			ValueItem args[] = { ValueItem(&c,VType::class_, as_refrence), ABI_IMPL::BVcast(types)... };
			ValueItem* res = c.callFnPtr(fun_name, access)->syncWrapper(args, sizeof(args) / sizeof(ValueItem));
			if (res == nullptr)
				return {};
			ValueItem m(std::move(*res));
			delete res;
			return m;
		}
		inline ValueItem makeCall(ClassAccess access, MorphValue& c, const std::string& fun_name) {
			ValueItem args(&c, VType::morph, as_refrence);
			ValueItem* res = c.callFnPtr(fun_name, access)->syncWrapper(&args, 1);
			if (res == nullptr)
				return {};
			ValueItem m(std::move(*res));
			delete res;
			return m;
		}
		template<class ...Types>
		ValueItem makeCall(ClassAccess access, MorphValue& c, const std::string& fun_name, const Types&... types) {
			ValueItem args[] = { ValueItem(&c,VType::morph, as_refrence), ABI_IMPL::BVcast(types)... };
			ValueItem* res = c.callFnPtr(fun_name, access)->syncWrapper(args, sizeof(args) / sizeof(ValueItem));
			if (res == nullptr)
				return {};
			ValueItem m(std::move(*res));
			delete res;
			return m;
		}
		inline ValueItem makeCall(ClassAccess access, ProxyClass& c, const std::string& fun_name) {
			ValueItem arg(&c, VType::proxy, as_refrence);
			ValueItem* res = c.callFnPtr(fun_name, access)->syncWrapper(&arg, 1);
			if (res == nullptr)
				return {};
			ValueItem m(std::move(*res));
			delete res;
			return m;
		}
		template<class ...Types>
		ValueItem makeCall(ClassAccess access, ProxyClass& c, const std::string& fun_name, const Types&... types) {
			ValueItem args[] = { ValueItem(&c,VType::proxy, as_refrence), ABI_IMPL::BVcast(types)... };
			ValueItem* res = c.callFnPtr(fun_name, access)->syncWrapper(args, sizeof(args) / sizeof(ValueItem));
			if (res == nullptr)
				return {};
			ValueItem m(std::move(*res));
			delete res;
			return m;
		}

		inline ValueItem makeCall(ClassAccess access, ValueItem& c, const std::string& fun_name) {
			ValueItem* res;
			switch (c.meta.vtype) {
			case VType::class_:
				res = ((ClassValue&)c).callFnPtr(fun_name, access)->syncWrapper(&c, 1);
				break;
			case VType::morph:
				res = ((MorphValue&)c).callFnPtr(fun_name, access)->syncWrapper(&c, 1);
				break;
			case VType::proxy:
				res = ((ProxyClass&)c).callFnPtr(fun_name, access)->syncWrapper(&c, 1);
				break;
			default:
				throw NotImplementedException();
			}
			if (res == nullptr)
				return {};
			ValueItem m(std::move(*res));
			delete res;
			return m;
		}
		template<class ...Types>
		ValueItem makeCall(ClassAccess access, ValueItem& c, const std::string& fun_name, const Types&... types) {
			ValueItem args[] = { ValueItem(c.val, c.meta, as_refrence) , ABI_IMPL::BVcast(types)... };
			ValueItem* res;
			switch (c.meta.vtype) {
			case VType::class_:
				res = ((ClassValue&)c).callFnPtr(fun_name, access)->syncWrapper(args, sizeof(args) / sizeof(ValueItem));
				break;
			case VType::morph:
				res = ((MorphValue&)c).callFnPtr(fun_name, access)->syncWrapper(args, sizeof(args) / sizeof(ValueItem));
				break;
			case VType::proxy:
				res = ((ProxyClass&)c).callFnPtr(fun_name, access)->syncWrapper(args, sizeof(args) / sizeof(ValueItem));
				break;
			default:
				throw NotImplementedException();
			}
			if (res == nullptr)
				return {};
			ValueItem m(std::move(*res));
			delete res;
			return m;
		}


		inline ValueItem makeCall(ClassAccess access, ClassValue& c, const std::string& fun_name, ValueItem* args, uint32_t len) {
			list_array<ValueItem> args_tmp(args, args + len, len);
			args_tmp.push_front(ValueItem(&c, VType::class_, as_refrence));
			ValueItem* res = ((ClassValue&)c).callFnPtr(fun_name, access)->syncWrapper(args_tmp.data(), len + 1);
			if (res == nullptr)
				return {};
			ValueItem m(std::move(*res));
			delete res;
			return m;
		}
		inline ValueItem makeCall(ClassAccess access, MorphValue& c, const std::string& fun_name, ValueItem* args, uint32_t len) {
			list_array<ValueItem> args_tmp(args, args + len, len);
			args_tmp.push_front(ValueItem(&c, VType::morph, as_refrence));
			ValueItem* res = ((MorphValue&)c).callFnPtr(fun_name, access)->syncWrapper(args_tmp.data(), len + 1);
			if (res == nullptr)
				return {};
			ValueItem m(std::move(*res));
			delete res;
			return m;
		}
		inline ValueItem makeCall(ClassAccess access, ProxyClass& c, const std::string& fun_name, ValueItem* args, uint32_t len) {
			list_array<ValueItem> args_tmp(args, args + len, len);
			args_tmp.push_front(ValueItem(&c, VType::proxy, as_refrence));
			ValueItem* res = ((ProxyClass&)c).callFnPtr(fun_name, access)->syncWrapper(args_tmp.data(), len + 1);
			if (res == nullptr)
				return {};
			ValueItem m(std::move(*res));
			delete res;
			return m;
		}
		inline ValueItem makeCall(ClassAccess access, ValueItem& c, const std::string& fun_name, ValueItem* args, uint32_t len) {
			list_array<ValueItem> args_tmp(args, args + len, len);
			args_tmp.push_front(ValueItem(c.val, c.meta, as_refrence));
			ValueItem* res;
			switch (c.meta.vtype) {
			case VType::class_:
				res = ((ClassValue&)c).callFnPtr(fun_name, access)->syncWrapper(args_tmp.data(), len + 1);
				break;
			case VType::morph:
				res = ((MorphValue&)c).callFnPtr(fun_name, access)->syncWrapper(args_tmp.data(), len + 1);
				break;
			case VType::proxy:
				res = ((ProxyClass&)c).callFnPtr(fun_name, access)->syncWrapper(args_tmp.data(), len + 1);
				break;
			default:
				throw NotImplementedException();
			}
			if (res == nullptr)
				return {};
			ValueItem m(std::move(*res));
			delete res;
			return m;
		}

		inline ValueItem getValue(ClassValue& c, const std::string& val_name) {
			return c.getValue(val_name, ClassAccess::pub);
		}
		inline ValueItem getValue(MorphValue& c, const std::string& val_name) {
			return c.getValue(val_name, ClassAccess::pub);
		}
		inline ValueItem getValue(ProxyClass& c, const std::string& val_name) {
			return c.getValue(val_name);
		}
		inline ValueItem getValue(ValueItem& c, const std::string& val_name) {
			switch (c.meta.vtype) {
			case VType::class_:
				return ((ClassValue&)c).getValue(val_name, ClassAccess::pub);
			case VType::morph:
				return ((MorphValue&)c).getValue(val_name, ClassAccess::pub);
			case VType::proxy:
				return ((ProxyClass&)c).getValue(val_name);
			default:
				throw NotImplementedException();
			}
		}

		inline ValueItem getValue(ClassAccess access, ClassValue& c, const std::string& val_name) {
			return c.getValue(val_name, access);
		}
		inline ValueItem getValue(ClassAccess access, MorphValue& c, const std::string& val_name) {
			return c.getValue(val_name, access);
		}
		inline ValueItem getValue(ClassAccess access, ProxyClass& c, const std::string& val_name) {
			return c.getValue(val_name);
		}
		inline ValueItem getValue(ClassAccess access, ValueItem& c, const std::string& val_name) {
			switch (c.meta.vtype) {
			case VType::class_:
				return ((ClassValue&)c).getValue(val_name, access);
			case VType::morph:
				return ((MorphValue&)c).getValue(val_name, access);
			case VType::proxy:
				return ((ProxyClass&)c).getValue(val_name);
			default:
				throw NotImplementedException();
			}
		}


		inline void setValue(ClassValue& c, const std::string& val_name, ValueItem& set) {
			c.getValue(val_name, ClassAccess::pub) = set;
		}
		inline void setValue(MorphValue& c, const std::string& val_name, ValueItem& set) {
			c.getValue(val_name, ClassAccess::pub) = set;
		}
		inline void setValue(ProxyClass& c, const std::string& val_name, ValueItem& set) {
			c.setValue(val_name, set);
		}
		inline void setValue(ValueItem& c, const std::string& val_name, ValueItem& set) {
			switch (c.meta.vtype) {
			case VType::class_:
				((ClassValue&)c).getValue(val_name, ClassAccess::pub) = set;
				break;
			case VType::morph:
				((MorphValue&)c).getValue(val_name, ClassAccess::pub) = set;
				break;
			case VType::proxy:
				((ProxyClass&)c).setValue(val_name, set);
				break;
			default:
				throw NotImplementedException();
			}
		}
		inline void setValue(ClassAccess access, ClassValue& c, const std::string& val_name, ValueItem& set) {
			c.getValue(val_name, access) = set;
		}
		inline void setValue(ClassAccess access, MorphValue& c, const std::string& val_name, ValueItem& set) {
			c.getValue(val_name, access) = set;
		}
		inline void setValue(ClassAccess access, ProxyClass& c, const std::string& val_name, ValueItem& set) {
			return c.setValue(val_name, set);
		}
		inline void setValue(ClassAccess access, ValueItem& c, const std::string& val_name, ValueItem& set) {
			switch (c.meta.vtype) {
			case VType::class_:
				((ClassValue&)c).getValue(val_name, access) = set;
				break;
			case VType::morph:
				((MorphValue&)c).getValue(val_name, access) = set;
				break;
			case VType::proxy:
				((ProxyClass&)c).setValue(val_name, set);
				break;
			default:
				throw NotImplementedException();
			}
		}



		inline bool hasImplement(ClassValue& c, const std::string& fun_name) {
			return c.containsFn(fun_name);
		}
		inline bool hasImplement(MorphValue& c, const std::string& fun_name) {
			return c.containsFn(fun_name);
		}
		inline bool hasImplement(ProxyClass& c, const std::string& fun_name) {
			return c.containsFn(fun_name);
		}
		inline bool hasImplement(ValueItem& c, const std::string& fun_name) {
			switch (c.meta.vtype) {
			case VType::class_:
				return ((ClassValue&)c).containsFn(fun_name);
			case VType::morph:
				return ((MorphValue&)c).containsFn(fun_name);
			case VType::proxy:
				return ((ProxyClass&)c).containsFn(fun_name);
			default:
				return false;
			}
		}
		inline std::string name(ClassValue& c) {
			return c.define->name;
		}
		inline std::string name(MorphValue& c) {
			return c.define.name;
		}
		inline std::string name(ProxyClass& c) {
			return c.declare_ty->name;
		}
		inline std::string name(ValueItem& c) {
			switch (c.meta.vtype) {
			case VType::class_:
				return ((ClassValue&)c).define->name;
			case VType::morph:
				return ((MorphValue&)c).define.name;
			case VType::proxy:
				return ((ProxyClass&)c).declare_ty->name;
			default:
				return "$ Not class $";
			}
		}

		inline bool is_interface(ClassValue& c) {
			return true;
		}
		inline bool is_interface(MorphValue& c) {
			return true;
		}
		inline bool is_interface(ProxyClass& c) {
			return true;
		}
		inline bool is_interface(ValueItem& c) {
			switch (c.meta.vtype) {
			case VType::class_:
			case VType::morph:
			case VType::proxy:
				return true;
			default:
				return false;
			}
		}

		namespace special {
			template<class Class_, bool as_ref>
			inline void* proxyCopy(void* val) {
				if constexpr (as_ref)
					return new typed_lgr<Class_>(*(typed_lgr<Class_>*)val);
				else 
					return new Class_(*(Class_*)val);
			}
			template<class Class_, bool as_ref>
			inline void proxyDestruct(void* val) {
				if constexpr (as_ref)
					delete (typed_lgr<Class_>*)val;
				else
					delete (Class_*)val;
			}
			template<class Class_>
			inline typed_lgr<Class_> proxy_get_as_native(ValueItem& vals) {
				if (vals.meta.vtype == VType::proxy)
					return (*(typed_lgr<Class_>*)(((ProxyClass*)vals.getSourcePtr()))->class_ptr);
				else
					throw InvalidClassDeclarationException("The not proxy type, use this function only with proxy values");
			}
		}
	}
}