// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../tasks_util/light_stack.hpp"
#include "../CASM.hpp"
#include "../AttachA_CXX.hpp"
#include "../dynamic_call.hpp"

template<class Class_>
inline typed_lgr<Class_> getClass(ValueItem* vals) {
	vals->getAsync();
	if (vals->meta.vtype == VType::proxy)
		return *(typed_lgr<Class_>*)((ProxyClass*)vals->getSourcePtr())->class_ptr;
	else
		throw InvalidOperation("That function used only in proxy class");
}
template<class Class_, bool as_ref>
inline Class_& getClass(ValueItem* vals) {
	vals->getAsync();
	if (vals->meta.vtype == VType::proxy)
        return *(Class_*)((ProxyClass*)vals->getSourcePtr())->class_ptr;
	else
		throw InvalidOperation("That function used only in proxy class");
}
namespace internal {
    namespace memory{
        //returns farr[farr[ptr from, ptr to, len, str desk, bool is_fault]...], arg: array/value ptr
        ValueItem* dump(ValueItem* vals, uint32_t len){
            if(len == 1)
                return light_stack::dump(vals[0].getSourcePtr());
            throw InvalidArguments("This function requires 1 argument");
        }
    }
    namespace stack {
        //reduce stack size, returns bool, args: shrink treeshold(optional)
        ValueItem* shrink(ValueItem* vals, uint32_t len){
            if(len == 1)
                return new ValueItem(light_stack::shrink_current((size_t)vals[0]));
            else if(len == 0)
                return new ValueItem(light_stack::shrink_current());
            throw InvalidArguments("This function requires 0 or 1 argument");
        }
        //grow stack size, returns bool, args: grow count
        ValueItem* prepare(ValueItem* vals, uint32_t len){
            if(len == 1)
                return new ValueItem(light_stack::prepare((size_t)vals[0]));
            else if(len == 0)
                return new ValueItem(light_stack::prepare());
            throw InvalidArguments("This function requires 0 or 1 argument");
        }
        //make sure stack size is enough and increase if too small, returns bool, args: grow count
        ValueItem* reserve(ValueItem* vals, uint32_t len){
            if(len == 1)
                return new ValueItem(light_stack::prepare((size_t)vals[0]));
            throw InvalidArguments("This function requires 1 argument");
        }


        //returns farr[farr[ptr from, ptr to, str desk, bool is_fault]...], args: none
        ValueItem* dump(ValueItem* vals, uint32_t len){
            if(len == 0)
                return light_stack::dump_current();
            throw InvalidArguments("This function requires 0 argument");
        }

        ValueItem* bs_supported(ValueItem* vals, uint32_t len){
            return new ValueItem(light_stack::is_supported());
        }


        ValueItem* used_size(ValueItem*, uint32_t){
            return new ValueItem(light_stack::used_size());
        }
        ValueItem* unused_size(ValueItem*, uint32_t){
            return new ValueItem(light_stack::unused_size());
        }
        ValueItem* allocated_size(ValueItem*, uint32_t){
            return new ValueItem(light_stack::allocated_size());
        }
        ValueItem* free_size(ValueItem*, uint32_t){
            return new ValueItem(light_stack::free_size());
        }


        ValueItem* trace(ValueItem* vals, uint32_t len){
            uint32_t framesToSkip = 0;
            bool include_native = true;
            uint32_t max_frames = 32;
            if(len >= 1)
                framesToSkip = (uint32_t)vals[0];
            if(len >= 2)
                include_native = (bool)vals[1];
            if(len >= 3)
                max_frames = (uint32_t)vals[2];

            auto res = FrameResult::JitCaptureStackTrace(framesToSkip, include_native, max_frames);
            auto res2 = new ValueItem[res.size()];
            for (size_t i = 0; i < res.size(); i++)
                res2[i] = ValueItem{res[i].file_path, res[i].fn_name, (uint64_t)res[i].line};
            return new ValueItem(res2, ValueMeta(VType::faarr, false, true, res.size()));
        }
        ValueItem* trace_frames(ValueItem* vals, uint32_t len){
            uint32_t framesToSkip = 0;
            bool include_native = true;
            uint32_t max_frames = 32;
            if(len >= 1)
                framesToSkip = (uint32_t)vals[0];
            if(len >= 2)
                include_native = (bool)vals[1];
            if(len >= 3)
                max_frames = (uint32_t)vals[2];
            auto res = FrameResult::JitCaptureStackChainTrace(framesToSkip, include_native, max_frames);
            auto res2 = new ValueItem[res.size()];
            for (size_t i = 0; i < res.size(); i++)
                res2[i] = res[i];
            return new ValueItem(res2, ValueMeta(VType::faarr, false, true, res.size()));
        }
        ValueItem* resolve_frame(ValueItem* vals, uint32_t len){
            StackTraceItem res;
            if(len == 1)
                res = FrameResult::JitResolveFrame((void*)vals[0]);
            else if(len == 2)
                res = FrameResult::JitResolveFrame((void*)vals[0], (bool)vals[1]);
            else 
                throw InvalidArguments("This function requires 1 argument and second optional: (rip, include_native)");
            return new ValueItem{res.file_path, res.fn_name, res.line};
        }
    }
    namespace run_time{
        ValueItem* gc_pause(ValueItem*, uint32_t){
            //lgr not support pause
            return nullptr;
        }
        ValueItem* gc_resume(ValueItem*, uint32_t){
            //lgr not support resume
            return nullptr;
        }

        //gc can ignore this hint
        ValueItem* gc_hinit_collect(ValueItem*, uint32_t){
            //lgr is determited, not need to hint
            return nullptr;
        }

        namespace native{
	        ProxyClassDefine define_NativeLib;
	        ProxyClassDefine define_NativeTemplate;
	        ProxyClassDefine define_NativeValue;
            namespace construct{
                ValueItem* createProxy_NativeValue(ValueItem*, uint32_t){
                    throw NotImplementedException();
                }
                ValueItem* createProxy_NativeTemplate(ValueItem*, uint32_t){
                    return new ValueItem(new ProxyClass(new typed_lgr(new DynamicCall::FunctionTemplate()), &define_NativeTemplate), VType::proxy);
                }
                ValueItem* createProxy_NativeLib(ValueItem* args, uint32_t len){
                    if(len != 1)
                        throw InvalidArguments("This function requires 1 argument: string");
                    return new ValueItem(new ProxyClass(new typed_lgr(new NativeLib(((std::string)args[0]).c_str())), &define_NativeLib), VType::proxy);
                }
            }
            ValueItem* funs_NativeLib_get_function(ValueItem* vals, uint32_t len) {
                if (len == 3)
                    if (vals->meta.vtype == VType::proxy)
                        if (vals[1].meta.vtype == VType::string)
                            if (vals[2].meta.vtype == VType::proxy) {
                                auto res = getClass<NativeLib>(vals)->get_func_enviro(((std::string)vals[1]).c_str(), *getClass<DynamicCall::FunctionTemplate>(vals + 2));
                                return new ValueItem(new typed_lgr(res), VType::function);
                            }
                throw InvalidArguments("This function requires 3 arguments: this(native_lib), string, native_template");
            }
            ValueItem* funs_NativeTemplate_add_argument(ValueItem* vals, uint32_t len) {
                if (len == 2)
                    if (vals->meta.vtype == VType::proxy)
                        if (vals[1].meta.vtype == VType::proxy) {
                            getClass<DynamicCall::FunctionTemplate>(vals)->arguments.push_back(getClass<DynamicCall::FunctionTemplate::ValueT, false>(vals + 1));
                            return nullptr;
                        }
                throw InvalidArguments("This function requires 2 arguments: this(native_template), native_value");
            }

            void init(){
                static bool is_init = false;
                if(is_init)
                    return;
                define_NativeLib.copy = AttachA::Interface::special::proxyCopy<NativeLib, true>;
                define_NativeLib.destructor = AttachA::Interface::special::proxyDestruct<NativeLib, true>;
                define_NativeLib.name = "native_lib";
		        define_NativeLib.funs["get_function"] = { new FuncEnviropment(funs_NativeLib_get_function,false),false,ClassAccess::pub };
            

                define_NativeTemplate.copy = AttachA::Interface::special::proxyCopy<DynamicCall::FunctionTemplate, true>;
                define_NativeTemplate.destructor = AttachA::Interface::special::proxyDestruct<DynamicCall::FunctionTemplate, true>;
                define_NativeTemplate.name = "native_template";
                define_NativeTemplate.value_seter["return_type"] = [](void* class_val, ValueItem& item){
                    if(item.meta.vtype == VType::proxy)
                        (*(typed_lgr<DynamicCall::FunctionTemplate>*)class_val)->result = getClass<DynamicCall::FunctionTemplate::ValueT, false>(&item);
                    else
                        throw InvalidArguments("This function requires 2 arguments: this(native_template) native_value");
                };
                define_NativeTemplate.value_seter["is_variadic"] = [](void* class_val, ValueItem& item){
                    (*(typed_lgr<DynamicCall::FunctionTemplate>*)class_val)->is_variadic = (bool)item;
                };
                define_NativeTemplate.funs["add_argument"] = { new FuncEnviropment(funs_NativeTemplate_add_argument,false),false,ClassAccess::pub };
            

                define_NativeValue.copy = AttachA::Interface::special::proxyCopy<DynamicCall::FunctionTemplate::ValueT, false>;
                define_NativeValue.destructor = AttachA::Interface::special::proxyDestruct<DynamicCall::FunctionTemplate::ValueT, false>;
                define_NativeValue.name = "native_value";
                define_NativeValue.value_seter["is_modifable"] = [](void* class_val, ValueItem& item){
                    (*(DynamicCall::FunctionTemplate::ValueT*)class_val).is_modifable = (bool)item;
                };
                define_NativeValue.value_seter["vsize"] = [](void* class_val, ValueItem& item){
                    (*(DynamicCall::FunctionTemplate::ValueT*)class_val).vsize = (uint32_t)item;
                };
                define_NativeValue.value_seter["place_type"] = [](void* class_val, ValueItem& item){
                    if(item.meta.vtype == VType::proxy)
                        (*(DynamicCall::FunctionTemplate::ValueT*)class_val).ptype = (DynamicCall::FunctionTemplate::ValueT::PlaceType)(bool)item;
                    else
                        throw InvalidArguments("This function requires 2 arguments: this(native_value) native_value");
                };
                define_NativeValue.value_seter["value_type"] = [](void* class_val, ValueItem& item){
                    if(item.meta.vtype == VType::proxy)
                        (*(DynamicCall::FunctionTemplate::ValueT*)class_val).vtype = (DynamicCall::FunctionTemplate::ValueT::ValueType)(uint8_t)item;
                    else
                        throw InvalidArguments("This function requires 2 arguments: this(native_value) native_value");
                }; 
                define_NativeValue.value_geter["is_modifable"] = [](void* class_val){
                    return new ValueItem((*(DynamicCall::FunctionTemplate::ValueT*)class_val).is_modifable);
                };
                define_NativeValue.value_geter["vsize"] = [](void* class_val){
                    return new ValueItem((*(DynamicCall::FunctionTemplate::ValueT*)class_val).vsize);
                };
                define_NativeValue.value_geter["place_type"] = [](void* class_val){
                    return new ValueItem((bool)(*(DynamicCall::FunctionTemplate::ValueT*)class_val).ptype);
                };
                define_NativeValue.value_geter["value_type"] = [](void* class_val){
                    return new ValueItem((uint8_t)(*(DynamicCall::FunctionTemplate::ValueT*)class_val).vtype);
                };
                is_init = true;
            }
        }
    }
    void init(){
        run_time::native::init();
    }
}