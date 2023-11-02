// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/ValueEnvironment.hpp>
#include <run_time/asm/FuncEnvironment.hpp>
#include <run_time/tasks.hpp>
#include <run_time/tasks/_internal.hpp>
#include <run_time/tasks/util/light_stack.hpp>

namespace art {
    void prepare_generator(ValueItem& set_args, art::shared_ptr<FuncEnvironment>& func, art::shared_ptr<FuncEnvironment>& ex_handler, Generator*& weak_ref) {
        weak_ref = &*loc.on_load_generator_ref;
        func = loc.on_load_generator_ref->func;
        ex_handler = loc.on_load_generator_ref->ex_handle;
        list_array<ValueItem> args_list;
        args_list.push_back(ValueItem(weak_ref));
        ValueItem& args = weak_ref->args;
        if (args.meta.vtype == VType::faarr || args.meta.vtype == VType::saarr) {
            ValueItem* args_ptr = (ValueItem*)args.getSourcePtr();
            for (uint32_t i = 0; i < args.meta.val_len; i++)
                args_list.push_back(std::move(args_ptr[i]));
        } else
            args_list.push_back(std::move(args));

        size_t len = 0;
        ValueItem* extracted = args_list.take_raw(len);
        set_args = ValueItem(extracted, len, no_copy);
        loc.on_load_generator_ref = nullptr;
    }

    void*& __Generator_get_context(Generator* generator_weak_ref) {
        return generator_weak_ref->context;
    }

    boost::context::continuation generator_execute(boost::context::continuation&& sink) {
        ValueItem args;
        art::shared_ptr<FuncEnvironment> func;
        art::shared_ptr<FuncEnvironment> ex_handler;
        Generator* weak_ref;
        prepare_generator(args, func, ex_handler, weak_ref);
        *reinterpret_cast<boost::context::continuation*>(&__Generator_get_context(weak_ref)) = std::move(sink);
        try {
            Generator::return_(weak_ref, func->syncWrapper((ValueItem*)args.getSourcePtr(), args.meta.val_len));
        } catch (TaskCancellation& cancellation) {
            forceCancelCancellation(cancellation);
            Generator::back_cancel(weak_ref);
        } catch (GeneratorRestart& restart) {
            forceCancelCancellation(restart);
            Generator::back_cancel(weak_ref);
        } catch (...) {
            std::exception_ptr except = std::current_exception();
            try {
                ValueItem ex = except;
                Generator::return_(weak_ref, ex_handler->syncWrapper(&ex, 1));
                return std::move(*reinterpret_cast<boost::context::continuation*>(&__Generator_get_context(weak_ref)));
            } catch (...) {
            }
            Generator::back_unwind(weak_ref, std::move(except));
        }
        return std::move(*reinterpret_cast<boost::context::continuation*>(&__Generator_get_context(weak_ref)));
    }

    Generator::Generator(art::shared_ptr<FuncEnvironment> call_func, const ValueItem& arguments, bool used_generator_local, art::shared_ptr<FuncEnvironment> exception_handler) {
        args = arguments;
        func = call_func;
        if (used_generator_local)
            _generator_local = new ValueEnvironment();
        else
            _generator_local = nullptr;

        ex_handle = exception_handler;
    }

    Generator::Generator(art::shared_ptr<FuncEnvironment> call_func, ValueItem&& arguments, bool used_generator_local, art::shared_ptr<FuncEnvironment> exception_handler) {
        args = std::move(arguments);
        func = call_func;
        if (used_generator_local)
            _generator_local = new ValueEnvironment();
        else
            _generator_local = nullptr;

        ex_handle = exception_handler;
    }

    Generator::Generator(Generator&& mov) noexcept {
        func = mov.func;
        _generator_local = mov._generator_local;
        ex_handle = mov.ex_handle;
        context = mov.context;

        mov._generator_local = nullptr;
        mov.context = nullptr;
    }

    Generator::~Generator() {
        if (_generator_local)
            delete _generator_local;
        if (context)
            reinterpret_cast<boost::context::continuation&>(context).~continuation();
    }

    bool Generator::yield_iterate(art::shared_ptr<Generator>& generator) {
        if (generator->context == nullptr) {
            *reinterpret_cast<boost::context::continuation*>(&generator->context) = boost::context::callcc(std::allocator_arg, light_stack(1048576 /*1 mb*/), generator_execute);
            if (generator->ex_ptr)
                std::rethrow_exception(generator->ex_ptr);
            return true;
        } else if (!generator->end_of_life) {
            *reinterpret_cast<boost::context::continuation*>(&generator->context) = reinterpret_cast<boost::context::continuation*>(&generator->context)->resume();
            if (generator->ex_ptr)
                std::rethrow_exception(generator->ex_ptr);
            return true;
        } else
            return false;
    }

    bool Generator::execute(art::shared_ptr<Generator>& generator) {
        if (generator->end_of_life)
            return false;
        if (generator->context == nullptr) {
            loc.on_load_generator_ref = generator;
            *reinterpret_cast<boost::context::continuation*>(&generator->context) = boost::context::callcc(std::allocator_arg, light_stack(1048576 /*1 mb*/), generator_execute);
            if (generator->ex_ptr)
                std::rethrow_exception(generator->ex_ptr);
            if (generator->restart_flag)
                generator->cancellation_flag = false;
            if (generator->cancellation_flag)
                throw TaskCancellation();
            if (!generator->results.empty())
                return true;
            return false;
        } else {
            *reinterpret_cast<boost::context::continuation*>(&generator->context) = reinterpret_cast<boost::context::continuation*>(&generator->context)->resume();
            if (generator->ex_ptr)
                std::rethrow_exception(generator->ex_ptr);
            if (generator->restart_flag)
                generator->cancellation_flag = false;
            if (generator->cancellation_flag)
                throw TaskCancellation();
            if (!generator->results.empty())
                return true;
            return false;
        }
    }

    ValueItem* Generator::get_result(art::shared_ptr<Generator>& generator) {
        if (!generator->results.empty())
            return generator->results.take_front();
        else if (execute(generator))
            return generator->results.take_front();
        else
            return nullptr;
    }

    list_array<ValueItem*> Generator::get_results(art::shared_ptr<Generator>& gen) {
        if (gen->results.empty()) {
            if (execute(gen))
                return gen->results.take();
            else
                return {};
        } else
            return gen->results.take();
    }
    bool Generator::has_result(art::shared_ptr<Generator>& generator) {
        return !generator->results.empty() || (generator->context != nullptr && !generator->end_of_life);
    }

    Generator::iterator Generator::cxx_iterate(art::shared_ptr<Generator>& gen) {
        return iterator(gen);
    }

    list_array<ValueItem> Generator::await_results(art::shared_ptr<Generator>& generator) {
        list_array<ValueItem> results;
        while (!generator->end_of_life || !generator->results.empty()) {
            ValueItem* result = get_result(generator);
            if (result) {
                results.push_back(std::move(*result));
                delete result;
            } else
                results.push_back(nullptr);
        }
        return results;
    }

    list_array<ValueItem> Generator::await_results(list_array<art::shared_ptr<Generator>>& generators) {
        list_array<ValueItem> results;
        for (auto& generator : generators) {
            auto result = await_results(generator);
            results.push_back(result);
        }
        return results;
    }

    void Generator::restart_context(art::shared_ptr<Generator>& gen) {
        if (gen->context != nullptr) {
            gen->restart_flag = true;
            *reinterpret_cast<boost::context::continuation*>(&gen->context) = reinterpret_cast<boost::context::continuation*>(&gen->context)->resume();
            gen->restart_flag = false;
            gen->cancellation_flag = false;
        }
        gen->end_of_life = false;
    }

    class ValueEnvironment* Generator::generator_local(Generator* generator_weak_ref) {
        return generator_weak_ref->_generator_local;
    }

    void Generator::yield(Generator* generator_weak_ref, ValueItem* result) {
        generator_weak_ref->results.push_back(result);
        *reinterpret_cast<boost::context::continuation*>(&generator_weak_ref->context) = reinterpret_cast<boost::context::continuation*>(&generator_weak_ref->context)->resume();
        if (generator_weak_ref->restart_flag)
            throw TaskCancellation();
    }

    void Generator::result(Generator* generator_weak_ref, ValueItem* result) {
        generator_weak_ref->results.push_back(result);
    }

    void Generator::back_unwind(Generator* generator_weak_ref, std::exception_ptr&& except) {
        generator_weak_ref->ex_ptr = std::move(except);
        generator_weak_ref->end_of_life = true;
    }

    void Generator::back_cancel(Generator* generator_weak_ref) {
        generator_weak_ref->cancellation_flag = true;
        generator_weak_ref->end_of_life = true;
    }

    void Generator::return_(Generator* generator_weak_ref, ValueItem* result) {
        generator_weak_ref->results.push_back(result);
        generator_weak_ref->end_of_life = true;
    }

    Generator::iterator::iterator(art::shared_ptr<Generator> generator)
        : generator(generator) {}

    Generator::iterator::iterator(const iterator& mov) {
        generator = mov.generator;
    }

    Generator::iterator::iterator(iterator&& mov) noexcept {
        generator = std::move(mov.generator);
    }

    Generator::iterator& Generator::iterator::operator=(const iterator& mov) {
        generator = mov.generator;
        return *this;
    }

    Generator::iterator& Generator::iterator::operator=(iterator&& mov) noexcept {
        generator = std::move(mov.generator);
        return *this;
    }

    bool Generator::iterator::operator==(const iterator& mov) {
        return generator->end_of_life == false || !generator->results.empty();
    }

    bool Generator::iterator::operator!=(const iterator& mov) {
        return generator->end_of_life == false || !generator->results.empty();
    }

    Generator::iterator& Generator::iterator::operator++() {
        if (generator->end_of_life)
            return *this;
        if (generator->results.empty())
            Generator::yield_iterate(generator);
        return *this;
    }

    Generator::iterator Generator::iterator::begin() {
        return *this;
    }

    Generator::iterator Generator::iterator::end() {
        return *this;
    }

    ValueItem Generator::iterator::operator*() {
        ValueItem* ptr_result = Generator::get_result(generator);
        if (ptr_result != nullptr) {
            ValueItem mov_result = std::move(*ptr_result);
            delete ptr_result;
            return mov_result;
        } else {
            return nullptr;
        }
    }
}
