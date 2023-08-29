// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <list>
#include <string>
#include <unordered_map>
#include <vector>

#include <run_time/asm/../attacha_abi_structs.hpp>
#include <run_time/asm/dynamic_call.hpp>
#include <util/exceptions.hpp>
#include <util/link_garbage_remover.hpp>
#include <util/threading.hpp>

namespace art {
    struct FuncHandle {
        typedef ValueItem* (*ProxyFunction)(void*, ValueItem*, uint32_t);
        struct inner_handle;
        void patch(inner_handle* handle);
        static FuncHandle* make_func_handle(inner_handle* handle = nullptr);
        static void release_func_handle(FuncHandle* handle);
        bool is_cheap();
        const std::vector<uint8_t>& code();
        ValueItem* compile_call(ValueItem* arguments, uint32_t arguments_size);
        void* get_trampoline_code();

    private:
        FuncHandle() = default;
        FuncHandle(const FuncHandle&) = delete;
        FuncHandle(FuncHandle&&) = delete;
        FuncHandle& operator=(const FuncHandle&) = delete;
        FuncHandle& operator=(FuncHandle&&) = delete;
        ~FuncHandle();
        mutex compile_lock;
        inner_handle* handle = nullptr;
        void** trampoline_jump = nullptr;
        void* trampoline_not_compiled_fallback; //do not use this
        void* art_ref;                          //internal use
        char trampoline_code[];
        //pseudo code
        //jump rip
        //rip: handle->env   if handle->env != nullptr
        //rip: not_compiled_fallback   if handle->env == nullptr(need compile)
        //not_compiled_fallback:
        //mov argr2, argr1
        //mov argr1, argr0
        //mov argr0, this
        //jmp [compile_call]
        friend class FuncEnvironment;
    };

    typedef std::list<std::pair<art::ustring, FuncHandle::inner_handle*>> patch_list;

    class FuncEnvironment {
        class FuncHandle* func_;
        uint8_t can_be_unloaded : 1 = false;
        FuncEnvironment(FuncHandle::inner_handle* env, bool can_be_unloaded = false);

    public:
        FuncEnvironment(Environment env, bool can_be_unloaded = false, bool is_cheap = false);
        FuncEnvironment(void* func, const DynamicCall::FunctionTemplate& template_func, bool can_be_unloaded = false, bool is_cheap = false);
        FuncEnvironment(void* func, FuncHandle::ProxyFunction proxy_func, bool can_be_unloaded = false, bool is_cheap = false);
        FuncEnvironment(const std::vector<uint8_t>& code, bool can_be_unloaded = false, bool is_cheap = false, art::ustring* cross_code_compiler_name_version = nullptr);
        FuncEnvironment(const std::vector<uint8_t>& code, const list_array<ValueItem>& values, bool can_be_unloaded = false, bool is_cheap = false, art::ustring* cross_code_compiler_name_version = nullptr);
        FuncEnvironment(const std::vector<uint8_t>& code, const list_array<ValueItem>& values, const std::vector<art::shared_ptr<FuncEnvironment>>& local_funcs, bool can_be_unloaded = false, bool is_cheap = false, art::ustring* cross_code_compiler_name_version = nullptr);
        FuncEnvironment(std::vector<uint8_t>&& code, bool can_be_unloaded = false, bool is_cheap = false, art::ustring* cross_code_compiler_name_version = nullptr);
        FuncEnvironment(std::vector<uint8_t>&& code, list_array<ValueItem>&& values, bool can_be_unloaded = false, bool is_cheap = false, art::ustring* cross_code_compiler_name_version = nullptr);
        FuncEnvironment(std::vector<uint8_t>&& code, list_array<ValueItem>&& values, std::vector<art::shared_ptr<FuncEnvironment>>&& local_funcs, bool can_be_unloaded = false, bool is_cheap = false, art::ustring* cross_code_compiler_name_version = nullptr);
        FuncEnvironment();
        FuncEnvironment(FuncEnvironment&& move) noexcept;
        ~FuncEnvironment();
        FuncEnvironment& operator=(FuncEnvironment&& move) noexcept;

        ValueItem* syncWrapper(ValueItem* arguments, uint32_t arguments_size);
        static ValueItem* asyncWrapper(art::shared_ptr<FuncEnvironment>* self, ValueItem* arguments, uint32_t arguments_size);

        static void fastHotPatch(const art::ustring& func_name, FuncHandle::inner_handle* new_enviro);
        static void fastHotPatch(const patch_list& patches);
        static art::shared_ptr<FuncEnvironment> environment(const art::ustring& func_name);
        static ValueItem* callFunc(const art::ustring& func_name, ValueItem* arguments, uint32_t arguments_size, bool run_async);

        static bool Exists(const art::ustring& symbol_name);
        static void Load(art::shared_ptr<FuncEnvironment> fn, const art::ustring& symbol_name);
        static void Unload(const art::ustring& func_name);
        static void ForceUnload(const art::ustring& func_name);
        static void AddNative(Environment env, const art::ustring& func_name, bool can_be_unloaded = true, bool is_cheap = false);
        template <class _FN>
        static void AddNative(_FN env, const art::ustring& func_name, bool can_be_unloaded = true, bool is_cheap = false);

        static ValueItem* async_call(art::shared_ptr<FuncEnvironment> f, ValueItem* args, uint32_t arguments_size);
        static ValueItem* sync_call(art::shared_ptr<FuncEnvironment> f, ValueItem* args, uint32_t arguments_size);

        bool isCheap() {
            return func_ != nullptr ? func_->is_cheap() : false;
        }

        void* get_func_ptr() const {
            return func_ != nullptr ? func_->get_trampoline_code() : nullptr;
        }

        void* inner_handle() const {
            if (func_ == nullptr)
                return nullptr;
            return func_->handle;
        }

        void patch(FuncHandle::inner_handle* handle) {
            if (func_ != nullptr)
                func_->patch(handle);
            else
                func_ = FuncHandle::make_func_handle(handle);
        }

        art::ustring to_string() const;
        const std::vector<uint8_t>& get_cross_code();
        void forceUnload();
        static void clear_environs();
    };

    struct FuncHandle::inner_handle {
        enum class FuncType : uint8_t {
            own,             //direct call c++ ValueItem* func(ValueItem* args, uint32_t args_count)
            native_c,        //c abi calls with DynamicCall
            static_native_c, //c abi calls, but using c++ proxy template function
            python,
            csharp,
            java,
            _,
            __,
            ___,
            ____,
            _____,
            ______,
            _______,
            ________,
            _________,
            force_unloaded
        };

        struct usage_scope {
            inner_handle* handle;

            usage_scope(inner_handle* handle)
                : handle(handle) {
                handle->increase_usage();
            }

            ~usage_scope() {
                handle->reduce_usage();
            }
        };

        std::vector<art::shared_ptr<FuncEnvironment>> local_funcs;
        list_array<art::shared_ptr<FuncEnvironment>> used_environs;
        list_array<ValueItem> values;
        std::vector<uint8_t> cross_code;
        Environment env = nullptr;
        FuncHandle* parent = nullptr;
        uint8_t* frame = nullptr;
        art::ustring* cross_code_compiler_name_version = nullptr;
        std::atomic_size_t ref_count = 0;

        FuncType _type : 4;
        bool is_cheap : 1;            //function without context switches and with fast code
        bool is_patchable : 1 = true; //function without context switches and with fast code


        inner_handle(Environment env, bool is_cheap);
        inner_handle(void* func, const DynamicCall::FunctionTemplate& template_func, bool is_cheap);
        inner_handle(void* func, ProxyFunction proxy_func, bool is_cheap);
        inner_handle(const std::vector<uint8_t>& code, bool is_cheap, art::ustring* cross_code_compiler_name_version);
        inner_handle(const std::vector<uint8_t>& code, const list_array<ValueItem>& values, bool is_cheap, art::ustring* cross_code_compiler_name_version);
        inner_handle(const std::vector<uint8_t>& code, const list_array<ValueItem>& values, const std::vector<art::shared_ptr<FuncEnvironment>>& local_funcs, bool is_cheap, art::ustring* cross_code_compiler_name_version);

        inner_handle(std::vector<uint8_t>&& code, bool is_cheap, art::ustring* cross_code_compiler_name_version);
        inner_handle(std::vector<uint8_t>&& code, list_array<ValueItem>&& values, bool is_cheap, art::ustring* cross_code_compiler_name_version);
        inner_handle(std::vector<uint8_t>&& code, list_array<ValueItem>&& values, std::vector<art::shared_ptr<FuncEnvironment>>&& local_funcs, bool is_cheap, art::ustring* cross_code_compiler_name_version);

        size_t localFnSize() {
            return local_funcs.size();
        }

        ValueItem* localWrapper(size_t indx, ValueItem* arguments, uint32_t arguments_size, bool run_async);

        art::shared_ptr<FuncEnvironment>& localFn(size_t indx) {
            return local_funcs[indx];
        }

        void reduce_usage() {
            if (ref_count.fetch_sub(1) == 1)
                delete this;
        }

        void increase_usage() {
            ref_count++;
        }

        void compile();
        ValueItem* dynamic_call_helper(ValueItem* arguments, uint32_t arguments_size);
        ValueItem* static_call_helper(ValueItem* arguments, uint32_t arguments_size);

    private:
        ValueItem* last_usage_env(ValueItem* result) {
            delete this;
            return result;
        }

        ~inner_handle();
    };

    //use make_func_handle and release_func_handle to create and destroy FuncHandle


}