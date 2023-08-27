// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/tasks.hpp>
#include <run_time/tasks/_internal.hpp>
#include <run_time/asm/FuncEnvironment.hpp>
#include <run_time/tasks/util/light_stack.hpp>
#include <run_time/ValueEnvironment.hpp>


namespace art{
    void prepare_generator(ValueItem& args,art::shared_ptr<FuncEnvironment>& func, art::shared_ptr<FuncEnvironment>& ex_handler, Generator*& weak_ref){
        weak_ref = &*loc.on_load_generator_ref;
        func = loc.on_load_generator_ref->func;
        ex_handler = loc.on_load_generator_ref->ex_handle;
        list_array<ValueItem> args_list;
        args_list.push_back(ValueItem(weak_ref));
        if(args.meta.vtype == VType::faarr || args.meta.vtype == VType::saarr){
            ValueItem* args_ptr = (ValueItem*)args.getSourcePtr();
            for(uint32_t i = 0; i < args.meta.val_len; i++)
                args_list.push_back(std::move(args_ptr[i]));
        }
        else
            args_list.push_back(std::move(args));

        size_t len = 0;
        ValueItem* extracted = args_list.take_raw(len);
        args = ValueItem(extracted, len, no_copy);
        weak_ref->args = nullptr;
        loc.on_load_generator_ref = nullptr;
    }
    
    boost::context::continuation generator_execute(boost::context::continuation&& sink){
        ValueItem args;
        art::shared_ptr<FuncEnvironment> func;
        art::shared_ptr<FuncEnvironment> ex_handler;
        Generator* weak_ref;
        prepare_generator(args, func, ex_handler, weak_ref);
        try{
            Generator::return_(weak_ref, func->syncWrapper((ValueItem*)args.getSourcePtr(), args.meta.val_len));
        }catch(...){
            std::exception_ptr except = std::current_exception();
            try{
                ValueItem ex = except;
                Generator::return_(weak_ref, ex_handler->syncWrapper(&ex, 1));
                return sink;
            }catch(...){}
            Generator::back_unwind(weak_ref, std::move(except));
        }
        return sink;
    }
    

    Generator::Generator(art::shared_ptr<FuncEnvironment> call_func, const ValueItem& arguments, bool used_generator_local, art::shared_ptr<FuncEnvironment> exception_handler){
        args = arguments;
        func = call_func;
        if(used_generator_local)
            _generator_local = new ValueEnvironment();
        else
            _generator_local = nullptr;
        
        ex_handle = exception_handler;
    }
    
    Generator::Generator(art::shared_ptr<FuncEnvironment> call_func, ValueItem&& arguments, bool used_generator_local, art::shared_ptr<FuncEnvironment> exception_handler){
        args = std::move(arguments);
        func = call_func;
        if(used_generator_local)
            _generator_local = new ValueEnvironment();
        else
            _generator_local = nullptr;
        
        ex_handle = exception_handler;
    }
    
    Generator::Generator(Generator&& mov) noexcept{
        func = mov.func;
        _generator_local = mov._generator_local;
        ex_handle = mov.ex_handle;
        context = mov.context;
        
        mov._generator_local = nullptr;
        mov.context = nullptr;
    }
    
    Generator::~Generator(){
        if(_generator_local)
            delete _generator_local;
        if(context)
            reinterpret_cast<boost::context::continuation&>(context).~continuation();
    }

    bool Generator::yield_iterate(art::shared_ptr<Generator>& generator){
        if(generator->context == nullptr) {
            *reinterpret_cast<boost::context::continuation*>(&generator->context) = boost::context::callcc(std::allocator_arg, light_stack(1048576/*1 mb*/), generator_execute);
            if(generator->ex_ptr)
                std::rethrow_exception(generator->ex_ptr);
            return true;
        }
        else if(!generator->end_of_life) {
            *reinterpret_cast<boost::context::continuation*>(&generator->context) = reinterpret_cast<boost::context::continuation*>(&generator->context)->resume();
            if(generator->ex_ptr)
                std::rethrow_exception(generator->ex_ptr);
            return true;
        }
        else return false;
    }
    
    ValueItem* Generator::get_result(art::shared_ptr<Generator>& generator){
        if(!generator->results.empty())
            return generator->results.take_front();
        if(generator->end_of_life)
            return nullptr;
        if(generator->context == nullptr) {
            loc.on_load_generator_ref = generator;
            *reinterpret_cast<boost::context::continuation*>(&generator->context) = boost::context::callcc(std::allocator_arg, light_stack(1048576/*1 mb*/), generator_execute);
            if(generator->ex_ptr)
                std::rethrow_exception(generator->ex_ptr);
            if(!generator->results.empty())
                return generator->results.take_front();
            return nullptr;
        }
        else {
            *reinterpret_cast<boost::context::continuation*>(&generator->context) = reinterpret_cast<boost::context::continuation*>(&generator->context)->resume();
            if(generator->ex_ptr)
                std::rethrow_exception(generator->ex_ptr);
            if(!generator->results.empty())
                return generator->results.take_front();
            return nullptr;
        }
    }
    
    bool Generator::has_result(art::shared_ptr<Generator>& generator){
        return !generator->results.empty() || (generator->context != nullptr && !generator->end_of_life);
    }
    
    list_array<ValueItem*> Generator::await_results(art::shared_ptr<Generator>& generator){
        list_array<ValueItem*> results;
        while(!generator->end_of_life)
            results.push_back(get_result(generator));
        return results;
    }
    
    list_array<ValueItem*> Generator::await_results(list_array<art::shared_ptr<Generator>>& generators){
        list_array<ValueItem*> results;
        for(auto& generator : generators){
            auto result = await_results(generator);
            results.push_back(result);
        }
        return results;
    }
    

    class ValueEnvironment* Generator::generator_local(Generator* generator_weak_ref){
        return generator_weak_ref->_generator_local;
    }
    
    void Generator::yield(Generator* generator_weak_ref, ValueItem* result){
        generator_weak_ref->results.push_back(result);
        *reinterpret_cast<boost::context::continuation*>(&generator_weak_ref->context) = reinterpret_cast<boost::context::continuation*>(&generator_weak_ref->context)->resume();
    }
    
    void Generator::result(Generator* generator_weak_ref, ValueItem* result){
        generator_weak_ref->results.push_back(result);
    }

    void Generator::back_unwind(Generator* generator_weak_ref, std::exception_ptr&& except){
        generator_weak_ref->ex_ptr = std::move(except);
        generator_weak_ref->end_of_life = true;
    }
    
    void Generator::return_(Generator* generator_weak_ref, ValueItem* result){
        generator_weak_ref->results.push_back(result);
        generator_weak_ref->end_of_life = true;
    }
}
