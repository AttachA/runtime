// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../tasks_util/light_stack.hpp"
#include "../CASM.hpp"
#include "../AttachA_CXX.hpp"
#include "../dynamic_call.hpp"

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
	        AttachAVirtualTable* define_NativeLib;
	        AttachAVirtualTable* define_NativeTemplate;
	        AttachAVirtualTable* define_NativeValue;
            namespace construct{
                ValueItem* createProxy_NativeValue(ValueItem*, uint32_t){
                    return new ValueItem(AttachA::Interface::constructStructure<typed_lgr<DynamicCall::FunctionTemplate::ValueT>>(define_NativeValue, new DynamicCall::FunctionTemplate::ValueT()), no_copy);
                }
                ValueItem* createProxy_NativeTemplate(ValueItem*, uint32_t){
                    return new ValueItem(AttachA::Interface::constructStructure<typed_lgr<DynamicCall::FunctionTemplate>>(define_NativeTemplate, new DynamicCall::FunctionTemplate()), no_copy);
                }
                AttachAFun(createProxy_NativeLib, 1, {
                    return ValueItem(AttachA::Interface::constructStructure<typed_lgr<NativeLib>>(define_NativeLib, new NativeLib(((std::string)args[0]).c_str())), no_copy);
                })
            }
            AttachAFun(funs_NativeLib_get_function, 3, {
                auto& class_ = AttachA::Interface::getExtractAs<typed_lgr<NativeLib>>(args[0], define_NativeLib);
                auto fun_name = (std::string)args[1];
                auto& template_ = *AttachA::Interface::getExtractAs<typed_lgr<DynamicCall::FunctionTemplate>>(args[2], define_NativeTemplate);
                return ValueItem(class_->get_func_enviro(fun_name, template_));
            })
            
            AttachAFun(funs_NativeTemplate_add_argument, 3, {
                auto& class_ = AttachA::Interface::getExtractAs<typed_lgr<DynamicCall::FunctionTemplate>>(args[0], define_NativeTemplate);
                auto& value = *AttachA::Interface::getExtractAs<typed_lgr<DynamicCall::FunctionTemplate::ValueT>>(args[1], define_NativeValue);
                class_->arguments.push_back(value);
                return nullptr;
            })

            #define funs_setter(name, class, set_typ, extract_typ)\
                AttachAFun(funs_NativeTemplate_setter_ ## name, 2, {\
                    auto& class_ = AttachA::Interface::getExtractAs<typed_lgr<class>>(args[0], define_NativeTemplate);\
                    class_->name = set_typ((extract_typ)args[1]);\
                })
            #define funs_getter(name, class, mindle_cast)\
                AttachAFun(funs_NativeTemplate_getter_ ## name, 1, {\
                    auto& class_ = AttachA::Interface::getExtractAs<typed_lgr<class>>(args[0], define_NativeTemplate);\
                    return ValueItem((mindle_cast)class_->name);\
                })
            DynamicCall::FunctionTemplate::ValueT castValueT(uint64_t val){
                return *(DynamicCall::FunctionTemplate::ValueT*)&val;
            }
            funs_setter(result, DynamicCall::FunctionTemplate, castValueT, uint64_t);
            
            AttachAFun(funs_NativeTemplate_getter_result, 1, {\
                auto& class_ = AttachA::Interface::getExtractAs<typed_lgr<DynamicCall::FunctionTemplate>>(args[0], define_NativeTemplate);\
                return ValueItem(*(uint64_t*)&class_->result);\
            })
            funs_setter(is_variadic, DynamicCall::FunctionTemplate, bool, bool);
            funs_getter(is_variadic, DynamicCall::FunctionTemplate, bool);


            funs_setter(is_modifable, DynamicCall::FunctionTemplate::ValueT, bool, bool);
            funs_getter(is_modifable, DynamicCall::FunctionTemplate::ValueT, bool);
            funs_setter(vsize, DynamicCall::FunctionTemplate::ValueT, uint32_t, uint32_t);
            funs_getter(vsize, DynamicCall::FunctionTemplate::ValueT, uint32_t);
            funs_setter(ptype, DynamicCall::FunctionTemplate::ValueT, DynamicCall::FunctionTemplate::ValueT::PlaceType, uint8_t)
            funs_getter(ptype, DynamicCall::FunctionTemplate::ValueT, uint8_t);
            funs_setter(vtype, DynamicCall::FunctionTemplate::ValueT, DynamicCall::FunctionTemplate::ValueT::ValueType, uint8_t);
            funs_getter(vtype, DynamicCall::FunctionTemplate::ValueT, uint8_t);


            void init(){
                static bool is_init = false;
                if(is_init)
                    return;
                define_NativeLib = AttachA::Interface::createTable<typed_lgr<NativeLib>>("native_lib",
                    AttachA::Interface::direct_method("get_function", funs_NativeLib_get_function)
                );
                define_NativeTemplate = AttachA::Interface::createTable<typed_lgr<DynamicCall::FunctionTemplate>>("native_template",
                    AttachA::Interface::direct_method("add_argument", funs_NativeTemplate_add_argument),
                    AttachA::Interface::direct_method("set_return_type", funs_NativeTemplate_setter_result),
                    AttachA::Interface::direct_method("get_return_type", funs_NativeTemplate_getter_result),
                    AttachA::Interface::direct_method("set_is_variadic", funs_NativeTemplate_setter_is_variadic),
                    AttachA::Interface::direct_method("get_is_variadic", funs_NativeTemplate_getter_is_variadic)
                );
                define_NativeValue = AttachA::Interface::createTable<typed_lgr<DynamicCall::FunctionTemplate::ValueT>>("native_value",
                    AttachA::Interface::direct_method("set_is_modifable", funs_NativeTemplate_setter_is_modifable),
                    AttachA::Interface::direct_method("get_is_modifable", funs_NativeTemplate_getter_is_modifable),
                    AttachA::Interface::direct_method("set_vsize", funs_NativeTemplate_setter_vsize),
                    AttachA::Interface::direct_method("get_vsize", funs_NativeTemplate_getter_vsize),
                    AttachA::Interface::direct_method("set_place_type", funs_NativeTemplate_setter_ptype),
                    AttachA::Interface::direct_method("get_place_type", funs_NativeTemplate_getter_ptype),
                    AttachA::Interface::direct_method("set_value_type", funs_NativeTemplate_setter_vtype),
                    AttachA::Interface::direct_method("get_value_type", funs_NativeTemplate_getter_vtype)
                );
                AttachA::Interface::typeVTable<typed_lgr<NativeLib>>() = define_NativeLib;
                AttachA::Interface::typeVTable<typed_lgr<DynamicCall::FunctionTemplate>>() = define_NativeTemplate;
                AttachA::Interface::typeVTable<typed_lgr<DynamicCall::FunctionTemplate::ValueT>>() = define_NativeValue;
                is_init = true;
            }
        }
    }
    void init(){
        run_time::native::init();
    }
}