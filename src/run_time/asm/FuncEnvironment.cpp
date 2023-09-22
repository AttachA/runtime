// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <base/run_time.hpp>
#include <configuration/agreement/symbols.hpp>
#include <run_time/AttachA_CXX.hpp>
#include <run_time/asm/CASM.hpp>
#include <run_time/asm/FuncEnvironment.hpp>
#include <run_time/asm/compiler.hpp>
#include <run_time/asm/compiler/helper_functions.hpp>
#include <run_time/asm/dynamic_call_proxy.hpp>
#include <run_time/asm/exception.hpp>
#include <run_time/asm/il_header_decoder.hpp>
#include <run_time/attacha_abi.hpp>
#include <run_time/tasks.hpp>
#include <run_time/util/tools.hpp>
#include <util/threading.hpp>

namespace art {
    using namespace reader;
    art::shared_ptr<asmjit::JitRuntime> art = new asmjit::JitRuntime();
    std::unordered_map<art::ustring, art::shared_ptr<FuncEnvironment>, art::hash<art::ustring>> environments;
    TaskMutex environments_lock;

    art::ustring try_resolve_frame(FuncHandle::inner_handle* env) {
        for (auto& it : environments) {
            if (it.second) {
                if (it.second->inner_handle() == env)
                    return it.first;
            }
        }
        void* fn_ptr;
        switch (env->_type) {
        case FuncHandle::inner_handle::FuncType::own:
            fn_ptr = (void*)env->env;
            break;
        case FuncHandle::inner_handle::FuncType::native_c:
            fn_ptr = (void*)env->frame;
            break;
        case FuncHandle::inner_handle::FuncType::static_native_c:
            fn_ptr = (void*)env->values[0];
            break;
        default:
            fn_ptr = nullptr;
        }
        if (fn_ptr != nullptr)
            return "fn(" + FrameResult::JitResolveFrame(fn_ptr, true).fn_name + ")@" + string_help::hexstr((ptrdiff_t)fn_ptr);
        else
            return "unresolved_attach_a_symbol";
    }

    void _inner_handle_finalizer(void* data, size_t size, void* rsp) {
        (*(FuncHandle::inner_handle**)data)->reduce_usage();
    }

    class RuntimeCompileException : public asmjit::ErrorHandler {
    public:
        void handleError(Error err, const char* message, asmjit::BaseEmitter* origin) override {
            throw CompileTimeException(asmjit::DebugUtils::errorAsString(err) + art::ustring(message));
        }
    };

    FuncHandle::inner_handle::inner_handle(Environment env, bool is_cheap)
        : is_cheap(is_cheap) {
        _type = FuncType::own;
        this->env = env;
    }

    FuncHandle::inner_handle::inner_handle(void* func, const DynamicCall::FunctionTemplate& template_func, bool is_cheap)
        : is_cheap(is_cheap) {
        _type = FuncType::native_c;
        values.push_back(new DynamicCall::FunctionTemplate(template_func));
        frame = (uint8_t*)func;
    }

    FuncHandle::inner_handle::inner_handle(void* func, FuncHandle::ProxyFunction proxy_func, bool is_cheap)
        : is_cheap(is_cheap) {
        _type = FuncType::static_native_c;
        values.push_back(func);
        frame = (uint8_t*)proxy_func;
    }

    FuncHandle::inner_handle::inner_handle(const std::vector<uint8_t>& code, bool is_cheap, art::ustring* cross_code_compiler_name_version)
        : is_cheap(is_cheap), cross_code_compiler_name_version(cross_code_compiler_name_version) {
        _type = FuncType::own;
        this->cross_code = code;
    }

    FuncHandle::inner_handle::inner_handle(const std::vector<uint8_t>& code, const list_array<ValueItem>& values, bool is_cheap, art::ustring* cross_code_compiler_name_version)
        : is_cheap(is_cheap), cross_code_compiler_name_version(cross_code_compiler_name_version) {
        _type = FuncType::own;
        this->cross_code = code;
        this->values = values;
    }

    FuncHandle::inner_handle::inner_handle(const std::vector<uint8_t>& code, const list_array<ValueItem>& values, const std::vector<art::shared_ptr<FuncEnvironment>>& local_funcs, bool is_cheap, art::ustring* cross_code_compiler_name_version)
        : is_cheap(is_cheap), cross_code_compiler_name_version(cross_code_compiler_name_version) {
        _type = FuncType::own;
        this->cross_code = code;
        this->values = values;
        this->local_funcs = local_funcs;
    }

    FuncHandle::inner_handle::inner_handle(std::vector<uint8_t>&& code, bool is_cheap, art::ustring* cross_code_compiler_name_version)
        : is_cheap(is_cheap), cross_code_compiler_name_version(cross_code_compiler_name_version) {
        _type = FuncType::own;
        this->cross_code = std::move(code);
    }

    FuncHandle::inner_handle::inner_handle(std::vector<uint8_t>&& code, list_array<ValueItem>&& values, bool is_cheap, art::ustring* cross_code_compiler_name_version)
        : is_cheap(is_cheap) {
        _type = FuncType::own;
        this->cross_code = std::move(code);
        this->values = std::move(values);
    }

    FuncHandle::inner_handle::inner_handle(std::vector<uint8_t>&& code, list_array<ValueItem>&& values, std::vector<art::shared_ptr<FuncEnvironment>>&& local_funcs, bool is_cheap, art::ustring* cross_code_compiler_name_version)
        : is_cheap(is_cheap), cross_code_compiler_name_version(cross_code_compiler_name_version) {
        _type = FuncType::own;
        this->cross_code = std::move(code);
        this->values = std::move(values);
        this->local_funcs = std::move(local_funcs);
    }

    ValueItem* FuncHandle::inner_handle::localWrapper(size_t indx, ValueItem* arguments, uint32_t arguments_size, bool run_async) {
        if (indx < local_funcs.size()) {
            if (run_async)
                return FuncEnvironment::async_call(local_funcs[indx], arguments, arguments_size);
            else
                return local_funcs[indx]->syncWrapper(arguments, arguments_size);
        } else
            throw InvalidArguments("Invalid local function index, index is out of range");
    }

    FuncHandle::inner_handle::~inner_handle() {
        if (frame != nullptr && _type == FuncType::own) {
            if (!FrameResult::deinit(frame, (void*)env, *art)) {
                ValueItem result{"Failed unload function:", frame};
                errors.async_notify(result);
            }
        }
        if (_type == FuncType::native_c) {
            delete (DynamicCall::FunctionTemplate*)(void*)values[0];
        }
        if (cross_code_compiler_name_version)
            delete cross_code_compiler_name_version;
    }

    void FuncHandle::inner_handle::compile() {
        if (frame != nullptr)
            throw InvalidOperation("Function already compiled");
        used_environs.clear();
        RuntimeCompileException error_handler;
        CodeHolder code;
        code.setErrorHandler(&error_handler);
        code.init(art->environment());
        CASM a(code);
        BuildProlog b_prolog(a);
        ScopeManager scope(b_prolog);
        ScopeManagerMap scope_map(scope);


        Label self_function = a.newLabel();
        a.label_bind(self_function);
        il_header::decoded header;

        size_t to_be_skiped =
            cross_code_compiler_name_version ? header.decode(*cross_code_compiler_name_version, a, cross_code, 0, cross_code.size()) : header.decode(a, cross_code, 0, cross_code.size());


        uint32_t to_alloc_statics = header.flags.used_static ? uint32_t(header.used_static_values) + 1 : 0;
        if (header.flags.run_time_computable) {
            //local_funcs already contains all local functions
            if (values.size() > to_alloc_statics)
                values.remove(to_alloc_statics, values.size());
            else {
                values.reserve_push_front(to_alloc_statics - values.size());
                for (uint32_t i = values.size(); i < to_alloc_statics; i++)
                    values.push_front(nullptr);
            }
        } else {
            local_funcs = std::move(header.locals);
            values.resize(to_alloc_statics, nullptr);
        }
        header.constants_values += to_alloc_statics;
        values.reserve_push_back(std::clamp<uint32_t>(header.constants_values, 0, UINT32_MAX));
        uint32_t max_values = header.flags.used_enviro_vals ? uint32_t(header.used_enviro_vals) + 1 : 0;


        //OS dependent prolog begin
        size_t is_patchable_exception_finalizer = 0;
        if (header.flags.is_patchable) {
            a.atomic_increase(&ref_count); //increase usage count
            is_patchable_exception_finalizer = scope.createExceptionScope();
            auto self = this;
            scope.setExceptionFinal(is_patchable_exception_finalizer, _inner_handle_finalizer, &self, sizeof(self));
        }
        b_prolog.pushReg(frame_ptr);
        if (max_values)
            b_prolog.pushReg(enviro_ptr);
        b_prolog.pushReg(arg_ptr);
        b_prolog.pushReg(arg_len);
        b_prolog.alignPush();
        b_prolog.stackAlloc(0x20); //c++ abi
        b_prolog.setFrame();
        b_prolog.end_prolog();
        //OS dependent prolog end

        //Init environment
        a.mov(arg_ptr, argr0);
        a.mov(arg_len_32, argr1_32);
        a.mov(enviro_ptr, stack_ptr);
        if (max_values)
            a.stackIncrease(CASM::alignStackBytes(max_values << 1));
        a.stackAlign();

        if (header.flags.used_arguments) {
            Label correct = a.newLabel();
            a.cmp(arg_len_32, uint32_t(header.used_arguments));
            a.jmp_unsigned_more_or_eq(correct);
            BuildCall b(a, 2);
            b.addArg(arg_len_32);
            b.addArg(uint32_t(header.used_arguments));
            b.finalize((void (*)(uint32_t, uint32_t))art::CXX::arguments_range);
            a.label_bind(correct);
        }

        //Clean environment
        {
            std::vector<ValueItem*> empty_static_map;
            ValueIndexPos ipos;
            ipos.pos = ValuePos::in_enviro;
            for (size_t i = 0; i < max_values; i++) {
                ipos.index = i;
                a.mov_valindex({empty_static_map, values}, ipos, 0);
                a.mov_valindex_meta({empty_static_map, values}, ipos, 0);
                scope.createValueLifetimeScope(helper_functions::valueDestructDyn, i << 1);
            }
        }
        Label prolog = a.newLabel();
        Compiler compiler(a, scope, scope_map, prolog, self_function, cross_code, cross_code.size(), to_be_skiped, header.jump_list, values, header.flags.in_debug, this, to_alloc_statics, used_environs);
        header.compiler->build(cross_code, to_be_skiped, cross_code.size(), compiler, this);

        a.label_bind(prolog);
        a.push(resr);
        a.push(0);
        {
            BuildCall b(a, 1);
            ValueIndexPos ipos;
            ipos.pos = ValuePos::in_enviro;
            for (size_t i = 0; i < max_values; i++) {
                ipos.index = i;
                b.lea_valindex({compiler.get_static_map(), values}, ipos);
                b.finalize(helper_functions::valueDestructDyn);
                scope.endValueLifetime(i);
            }
        }
        a.pop();
        a.pop(resr);
        auto& tmp = b_prolog.finalize_epilog();

        if (header.flags.is_patchable) {
            scope.endExceptionScope(is_patchable_exception_finalizer);
            a.mov(argr0, -1);
            a.atomic_fetch_add(&ref_count, argr0);
            a.cmp(argr0, 1); //if old ref_count == 1

            Label last_usage = a.newLabel();
            a.jmp_equal(last_usage);
            a.ret();
            a.label_bind(last_usage);
            a.mov(argr0, this);
            a.mov(argr1, resr);
            auto func_ptr = &FuncHandle::inner_handle::last_usage_env;
            Label last_usage_function = a.add_data((char*)reinterpret_cast<void*&>(func_ptr), 8);
            a.jmp_in_label(last_usage_function);
        } else {
            a.ret();
        }

        tmp.use_handle = true;
        tmp.exHandleOff = a.offset() <= UINT32_MAX ? (uint32_t)a.offset() : throw InvalidFunction("Too big function");
        a.jmp((size_t)exception::__get_internal_handler());
        a.finalize();
        auto resolved_frame = try_resolve_frame(this);
        env = (Environment)tmp.init(frame, a.code(), *art, resolved_frame.data());
        //remove self from used_environs
        auto my_trampoline = parent ? parent->get_trampoline_code() : nullptr;
        used_environs.remove_if([my_trampoline](art::shared_ptr<FuncEnvironment>& a) { return a->get_func_ptr() == my_trampoline; });
    }

    ValueItem* FuncHandle::inner_handle::dynamic_call_helper(ValueItem* arguments, uint32_t arguments_size) {
        auto template_func = (DynamicCall::FunctionTemplate*)(void*)values[0];
        DynamicCall::FunctionCall call((DynamicCall::PROC)frame, *template_func, true);
        return __attacha___::NativeProxy_DynamicToStatic(call, *template_func, arguments, arguments_size);
    }

    ValueItem* FuncHandle::inner_handle::static_call_helper(ValueItem* arguments, uint32_t arguments_size) {
        FuncHandle::ProxyFunction proxy = (FuncHandle::ProxyFunction)frame;
        void* func = (void*)values[0];
        return proxy(func, arguments, arguments_size);
    }

#pragma endregion

#pragma region FuncHandle

    FuncHandle* FuncHandle::make_func_handle(inner_handle* handle) {
        if (handle ? (bool)handle->parent : false)
            throw InvalidArguments("Handle already in use");
        RuntimeCompileException error_handler;
        CodeHolder trampoline_code;
        trampoline_code.setErrorHandler(&error_handler);
        trampoline_code.init(art->environment());
        CASM a(trampoline_code);
        char fake_data[8]{(char)0xFF};
        Label compile_call_label = a.add_data(fake_data, 8);
        Label handle_label = a.add_data(fake_data, 8);

        Label not_compiled_code_fallback = a.newLabel();
        Label data_label;
        if (handle ? (bool)handle->env : false)
            data_label = a.add_data((char*)handle->env, sizeof(handle->env));
        else
            data_label = a.add_label_ptr(not_compiled_code_fallback);
        a.jmp_in_label(data_label);               //jmp [env | not_compiled_code_fallback]
        a.label_bind(not_compiled_code_fallback); //not_compiled_code_fallback:
        a.mov(argr2, argr1);                      //mov argr2, argr1
        a.mov(argr1, argr0);                      //mov argr1, argr0
        a.mov_long(argr0, handle_label, 0);       //mov argr0, [FuncHandle]
        a.jmp_in_label(compile_call_label);       //jmp [FuncHandle::compile_call]
        a.finalize();
        size_t code_size = trampoline_code.textSection()->realSize();
        code_size = code_size + asmjit::Support::alignUp(code_size, trampoline_code.textSection()->alignment());
        FuncHandle* code;
        CASM::allocate_and_prepare_code(sizeof(FuncHandle), (uint8_t*&)code, &trampoline_code, art->allocator(), 0);
        new (code) FuncHandle();
        char* code_raw = (char*)code + sizeof(FuncHandle);
        char* code_data = (char*)code + sizeof(FuncHandle) + code_size - 1;


        char* trampoline_jump = (char*)code_data + trampoline_code.labelOffset(data_label) + 8; //24
        char* trampoline_not_compiled_fallback = (char*)code_raw + trampoline_code.labelOffset(not_compiled_code_fallback);
        char* handle_ptr = (char*)code_data + trampoline_code.labelOffset(handle_label) + 8;         //16
        char* function_ptr = (char*)code_data + trampoline_code.labelOffset(compile_call_label) + 8; //8


        auto func_ptr = &art::FuncHandle::compile_call;
        if (handle ? (bool)handle->env : false)
            *(void**)trampoline_jump = (char*)handle->env;
        else
            *(void**)trampoline_jump = trampoline_not_compiled_fallback;
        *(void**)handle_ptr = code;
        *(void**)function_ptr = reinterpret_cast<void*&>(func_ptr);

        code->trampoline_not_compiled_fallback = (void*)trampoline_not_compiled_fallback;
        code->trampoline_jump = (void**)trampoline_jump;
        code->handle = handle;
        if (handle) {
            handle->increase_usage();
            handle->parent = code;
        }
        code->art_ref = new art::shared_ptr<asmjit::JitRuntime>(art);
        return code;
    }

    void FuncHandle::release_func_handle(FuncHandle* handle) {
        art::shared_ptr<asmjit::JitRuntime> art = *(art::shared_ptr<asmjit::JitRuntime>*)handle->art_ref;
        handle->~FuncHandle();
        CASM::release_code((uint8_t*)handle, art->allocator());
    }

    FuncHandle::~FuncHandle() {
        lock_guard lock(compile_lock);
        if (handle)
            handle->reduce_usage();
        delete (art::shared_ptr<asmjit::JitRuntime>*)art_ref;
    }

    void FuncHandle::patch(inner_handle* handle) {
        lock_guard lock(compile_lock);
        if (handle) {
            if (handle->parent != nullptr)
                throw InvalidArguments("Handle already in use");

            if (handle->env != nullptr) {
                if (trampoline_jump != nullptr)
                    *trampoline_jump = (void*)handle->env;
            } else {
                if (trampoline_jump != nullptr)
                    *trampoline_jump = trampoline_not_compiled_fallback;
            }
        } else {
            if (trampoline_jump != nullptr)
                *trampoline_jump = trampoline_not_compiled_fallback;
        }
        if (this->handle != nullptr) {
            if (!this->handle->is_patchable)
                throw InvalidOperation("Tried patch unpatchable function");
            this->handle->reduce_usage();
        }
        handle->increase_usage();
        this->handle = handle;
        handle->parent = this;
    }

    bool FuncHandle::is_cheap() {
        lock_guard lock(compile_lock);
        return handle ? handle->is_cheap : false;
    }

    const std::vector<uint8_t>& FuncHandle::code() {
        lock_guard lock(compile_lock);
        static std::vector<uint8_t> empty;
        return handle ? handle->cross_code : empty;
    }

    ValueItem* FuncHandle::compile_call(ValueItem* arguments, uint32_t arguments_size) {
        unique_lock lock(compile_lock);
        if (handle) {
            inner_handle::usage_scope scope(handle);
            inner_handle* compile_handle = handle;
            if (handle->_type == inner_handle::FuncType::own) {
                if (trampoline_jump != nullptr && handle->env != nullptr) {
                    if (*trampoline_jump != handle->env) {
                        *trampoline_jump = (void*)handle->env;
                        goto not_need_compile;
                    }
                }
                lock.unlock();
                compile_handle->compile();
                lock.lock();
                if (trampoline_jump != nullptr)
                    *trampoline_jump = (void*)compile_handle->env;
            }
        not_need_compile:
            lock.unlock();
            switch (compile_handle->_type) {
            case inner_handle::FuncType::own:
                return compile_handle->env(arguments, arguments_size);
            case inner_handle::FuncType::native_c:
                return compile_handle->dynamic_call_helper(arguments, arguments_size);
            case inner_handle::FuncType::static_native_c:
                return compile_handle->static_call_helper(arguments, arguments_size);
            default:
                throw NotImplementedException(); //function not implemented
            }
        }
        throw NotImplementedException(); //function not implemented
    }

    void* FuncHandle::get_trampoline_code() {
        unique_lock lock(compile_lock);
        if (handle)
            if (!handle->is_patchable)
                if (handle->_type == inner_handle::FuncType::own)
                    return (void*)handle->env;
        return trampoline_code;
    }

#pragma endregion
#pragma region FuncEnvironment

    ValueItem* FuncEnvironment::async_call(art::shared_ptr<FuncEnvironment> f, ValueItem* args, uint32_t args_len) {
        ValueItem* res = new ValueItem();
        res->meta = ValueMeta(VType::async_res, false, false).encoded;
        res->val = new typed_lgr(new Task(f, ValueItem(args, ValueMeta(VType::saarr, false, true, args_len), no_copy)));
        Task::start(*(art::typed_lgr<Task>*)res->val);
        return res;
    }

    ValueItem* FuncEnvironment::syncWrapper(ValueItem* args, uint32_t arguments_size) {
        if (func_ == nullptr)
            throw InvalidFunction("Function is force unloaded");
        return ((Environment)&func_->trampoline_code)(args, arguments_size);
    }

    ValueItem* FuncEnvironment::asyncWrapper(art::shared_ptr<FuncEnvironment>* self, ValueItem* arguments, uint32_t arguments_size) {
        return FuncEnvironment::async_call(*self, arguments, arguments_size);
    }

    art::ustring FuncEnvironment::to_string() const {
        if (!func_)
            return "fn(unloaded)@0";

        if (func_->handle == nullptr)
            return "fn(unknown)@0";
        void* fn_ptr;
        switch (func_->handle->_type) {
        case FuncHandle::inner_handle::FuncType::own:
            fn_ptr = (void*)func_->handle->env;
            break;
        case FuncHandle::inner_handle::FuncType::native_c:
            fn_ptr = (void*)func_->handle->frame;
            break;
        case FuncHandle::inner_handle::FuncType::static_native_c:
            fn_ptr = (void*)func_->handle->values[0];
            break;
        default:
            fn_ptr = nullptr;
        }
        {
            unique_lock guard(environments_lock);
            for (auto& it : environments)
                if (&*it.second == this)
                    return "fn(" + it.first + ")@" + string_help::hexstr((ptrdiff_t)fn_ptr);
        }
        if (fn_ptr == nullptr)
            return "fn(unresolved)@unknown";
        return "fn(" + FrameResult::JitResolveFrame(fn_ptr, true).fn_name + ")@" + string_help::hexstr((ptrdiff_t)fn_ptr);
    }

    const std::vector<uint8_t>& FuncEnvironment::get_cross_code() {
        static std::vector<uint8_t> empty;
        return func_ ? func_->code() : empty;
    }

    void FuncEnvironment::fastHotPatch(const art::ustring& func_name, FuncHandle::inner_handle* new_enviro) {
        unique_lock guard(environments_lock);
        auto& tmp = environments[func_name];
        guard.unlock();
        tmp->patch(new_enviro);
    }

    void FuncEnvironment::fastHotPatch(const patch_list& patches) {
        for (auto& it : patches)
            if (it.second->parent)
                throw InvalidOperation("Can't patch function with bounded handle: " + it.first);
        for (auto& it : patches)
            fastHotPatch(it.first, it.second);
    }

    art::shared_ptr<FuncEnvironment> FuncEnvironment::environment(const art::ustring& func_name) {
        return environments[func_name];
    }

    ValueItem* FuncEnvironment::callFunc(const art::ustring& func_name, ValueItem* arguments, uint32_t arguments_size, bool run_async) {
        unique_lock guard(environments_lock);
        auto found = environments.find(func_name);
        guard.unlock();
        if (found != environments.end()) {
            if (run_async)
                return async_call(found->second, arguments, arguments_size);
            else
                return found->second->syncWrapper(arguments, arguments_size);
        }
        throw NotImplementedException();
    }

    void FuncEnvironment::AddNative(Environment function, const art::ustring& symbol_name, bool can_be_unloaded, bool is_cheap) {
        art::lock_guard guard(environments_lock);
        if (environments.contains(symbol_name))
            throw SymbolException("Fail allocate symbol: \"" + symbol_name + "\" cause them already exists");
        auto symbol = new FuncHandle::inner_handle(function, is_cheap);
        environments[symbol_name] = new FuncEnvironment(symbol, can_be_unloaded);
    }

    bool FuncEnvironment::Exists(const art::ustring& symbol_name) {
        art::lock_guard guard(environments_lock);
        return environments.contains(symbol_name);
    }

    void FuncEnvironment::Load(art::shared_ptr<FuncEnvironment> fn, const art::ustring& symbol_name) {
        art::lock_guard guard(environments_lock);
        auto found = environments.find(symbol_name);
        if (found != environments.end()) {
            if (found->second->func_ != nullptr)
                if (found->second->func_->handle != nullptr)
                    throw SymbolException("Fail load symbol: \"" + symbol_name + "\" cause them already exists");
            found->second = fn;
        } else
            environments[symbol_name] = fn;
    }

    void FuncEnvironment::Unload(const art::ustring& func_name) {
        art::lock_guard guard(environments_lock);
        auto found = environments.find(func_name);
        if (found != environments.end()) {
            if (!found->second->can_be_unloaded)
                throw SymbolException("Fail unload symbol: \"" + func_name + "\" cause them can't be unloaded");
            environments.erase(found);
        }
    }

    void FuncEnvironment::ForceUnload(const art::ustring& func_name) {
        art::lock_guard guard(environments_lock);
        auto found = environments.find(func_name);
        if (found != environments.end())
            environments.erase(found);
    }

    void FuncEnvironment::forceUnload() {
        art::unique_lock guard(environments_lock);
        auto begin = environments.begin();
        auto end = environments.end();
        while (begin != end) {
            if (begin->second->func_ == func_) {
                environments.erase(begin);
                break;
            }
        }
        FuncHandle* handle = func_;
        func_ = nullptr;
        guard.unlock();
        if (handle)
            FuncHandle::release_func_handle(handle);
    }

    void FuncEnvironment::clear_environs() {
        environments.clear();
    }

    FuncEnvironment::FuncEnvironment(FuncHandle::inner_handle* env, bool can_be_unloaded) {
        this->can_be_unloaded = can_be_unloaded;
        func_ = FuncHandle::make_func_handle(env);
    }

    FuncEnvironment::FuncEnvironment(Environment env, bool can_be_unloaded, bool is_cheap)
        : can_be_unloaded(can_be_unloaded) {
        func_ = FuncHandle::make_func_handle(new FuncHandle::inner_handle(env, is_cheap));
    }

    FuncEnvironment::FuncEnvironment(void* func, const DynamicCall::FunctionTemplate& template_func, bool can_be_unloaded, bool is_cheap)
        : can_be_unloaded(can_be_unloaded) {
        func_ = FuncHandle::make_func_handle(new FuncHandle::inner_handle(func, template_func, is_cheap));
    }

    FuncEnvironment::FuncEnvironment(void* func, FuncHandle::ProxyFunction proxy_func, bool can_be_unloaded, bool is_cheap)
        : can_be_unloaded(can_be_unloaded) {
        func_ = FuncHandle::make_func_handle(new FuncHandle::inner_handle(func, proxy_func, is_cheap));
    }

    FuncEnvironment::FuncEnvironment(const std::vector<uint8_t>& code, bool can_be_unloaded, bool is_cheap, art::ustring* cross_code_compiler_name_version)
        : can_be_unloaded(can_be_unloaded) {
        func_ = FuncHandle::make_func_handle(new FuncHandle::inner_handle(code, is_cheap, cross_code_compiler_name_version));
    }

    FuncEnvironment::FuncEnvironment(const std::vector<uint8_t>& code, const list_array<ValueItem>& values, bool can_be_unloaded, bool is_cheap, art::ustring* cross_code_compiler_name_version)
        : can_be_unloaded(can_be_unloaded) {
        func_ = FuncHandle::make_func_handle(new FuncHandle::inner_handle(code, values, is_cheap, cross_code_compiler_name_version));
    }

    FuncEnvironment::FuncEnvironment(const std::vector<uint8_t>& code, const list_array<ValueItem>& values, const std::vector<art::shared_ptr<FuncEnvironment>>& local_funcs, bool can_be_unloaded, bool is_cheap, art::ustring* cross_code_compiler_name_version)
        : can_be_unloaded(can_be_unloaded) {
        func_ = FuncHandle::make_func_handle(new FuncHandle::inner_handle(code, values, local_funcs, is_cheap, cross_code_compiler_name_version));
    }

    FuncEnvironment::FuncEnvironment(std::vector<uint8_t>&& code, bool can_be_unloaded, bool is_cheap, art::ustring* cross_code_compiler_name_version)
        : can_be_unloaded(can_be_unloaded) {
        func_ = FuncHandle::make_func_handle(new FuncHandle::inner_handle(std::move(code), is_cheap, cross_code_compiler_name_version));
    }

    FuncEnvironment::FuncEnvironment(std::vector<uint8_t>&& code, list_array<ValueItem>&& values, bool can_be_unloaded, bool is_cheap, art::ustring* cross_code_compiler_name_version)
        : can_be_unloaded(can_be_unloaded) {
        func_ = FuncHandle::make_func_handle(new FuncHandle::inner_handle(std::move(code), std::move(values), is_cheap, cross_code_compiler_name_version));
    }

    FuncEnvironment::FuncEnvironment(std::vector<uint8_t>&& code, list_array<ValueItem>&& values, std::vector<art::shared_ptr<FuncEnvironment>>&& local_funcs, bool can_be_unloaded, bool is_cheap, art::ustring* cross_code_compiler_name_version)
        : can_be_unloaded(can_be_unloaded) {
        func_ = FuncHandle::make_func_handle(new FuncHandle::inner_handle(std::move(code), std::move(values), std::move(local_funcs), is_cheap, cross_code_compiler_name_version));
    }

    FuncEnvironment::FuncEnvironment() {
        func_ = FuncHandle::make_func_handle();
    }

    FuncEnvironment::FuncEnvironment(FuncEnvironment&& move) noexcept {
        func_ = nullptr;
        operator=(std::move(move));
    }

    FuncEnvironment::~FuncEnvironment() {
        if (func_)
            FuncHandle::release_func_handle(func_);
        func_ = nullptr;
    }

    FuncEnvironment& FuncEnvironment::operator=(FuncEnvironment&& move) noexcept {
        can_be_unloaded = move.can_be_unloaded;
        if (func_ != nullptr)
            FuncHandle::release_func_handle(func_);
        func_ = move.func_;
        move.func_ = nullptr;
        return *this;
    }

    ValueItem* FuncEnvironment::sync_call(art::shared_ptr<FuncEnvironment> f, ValueItem* args, uint32_t arguments_size) {
        return f->syncWrapper(args, arguments_size);
    }

#pragma endregion
}