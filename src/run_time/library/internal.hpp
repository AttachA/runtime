// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef SRC_RUN_TIME_LIBRARY_INTERNAL
#define SRC_RUN_TIME_LIBRARY_INTERNAL
#include <run_time/attacha_abi_structs.hpp>

namespace art {
    namespace internal {
        struct tag_viewer {
            const void* _internal; //ref to tag
            art::ustring get_name() const;
            ValueItem get_value() const;
            art::shared_ptr<FuncEnvironment> get_enviro() const;

            void set_value(ValueItem&& value);
            void set_enviro(art::shared_ptr<FuncEnvironment> enviro);
        };

        struct method_viewer {
            const void* _internal; //ref to method
            art::ustring get_name() const;
            ClassAccess get_access() const;
            bool is_deletable() const;
            art::shared_ptr<FuncEnvironment> get_function() const;
            art::ustring get_owner_name() const;
            list_array<tag_viewer> get_tags() const;
            list_array<list_array<std::pair<ValueMeta, art::ustring>>> get_args() const;
            list_array<ValueMeta> get_return_values() const;

            void set_access(ClassAccess access);
            void set_function(art::shared_ptr<FuncEnvironment> fn, list_array<list_array<std::pair<ValueMeta, art::ustring>>>&& args = {}, list_array<ValueMeta>&& ret = {});
            void add_tag(const std::string& name, ValueItem&& value, art::shared_ptr<FuncEnvironment> enviro);
            void remove_tag(const std::string& name);
            void clear_tags();
        };

        struct static_viewer {
            const void* _internal; //ref to static value
            art::ustring get_name() const;
            ClassAccess get_access() const;
            list_array<tag_viewer> get_tags() const;
            ValueItem copy_value() const;


            void set_access(ClassAccess access);
            void add_tag(const std::string& name, ValueItem&& value, art::shared_ptr<FuncEnvironment> enviro);
            void remove_tag(const std::string& name);
            void clear_tags();

            void set_value(ValueItem&& value);
        };

        struct value_viewer {
            const void* _internal; //ref to value
            art::ustring get_name() const;
            ClassAccess get_access() const;
            ValueMeta get_type() const;
            bool is_allow_abstract_assign() const;
            bool is_inlined() const;
            uint8_t get_bit_offset() const;
            uint16_t get_bit_used() const;
            size_t get_offset() const;
            bool get_zero_after_cleanup() const;
            list_array<tag_viewer> get_tags() const;

            void set_access(ClassAccess access);
            void set_type(ValueMeta type);
            void set_allow_abstract_assign(bool allow);
            void set_inlined(bool inlined);
            void set_bit_offset(uint8_t offset);
            void set_bit_used(uint16_t used);
            void set_offset(size_t offset);
            void set_zero_after_cleanup(bool zero);
            void add_tag(const std::string& name, ValueItem&& value, art::shared_ptr<FuncEnvironment> enviro);
            void remove_tag(const std::string& name);
            void clear_tags();
        };

        struct vtable_viewer {
            void* vtable;
            Structure::VTableMode mode;

            art::shared_ptr<FuncEnvironment> get_destructor();
            art::shared_ptr<FuncEnvironment> get_copy();
            art::shared_ptr<FuncEnvironment> get_move();
            art::shared_ptr<FuncEnvironment> get_compare();
            art::shared_ptr<FuncEnvironment> get_constructor();

            art::ustring get_name();
            size_t get_structure_size();
            bool get_allow_auto_copy();

            list_array<method_viewer> get_methods();
            method_viewer get_method_by_id(uint64_t id);
            method_viewer get_method_by_name(const art::ustring& name, ClassAccess access);
            uint64_t get_method_id(const art::ustring& name, ClassAccess access);

            list_array<value_viewer> get_values();
            value_viewer get_value_by_id(uint64_t id);
            value_viewer get_value_by_name(const art::ustring& name, ClassAccess access);
            uint64_t get_value_id(const art::ustring& name, ClassAccess access);

            list_array<static_viewer> get_statics();
            static_viewer get_static_by_name(const art::ustring& name, ClassAccess access);

            list_array<tag_viewer> get_tags();
            tag_viewer get_tag_by_id(uint64_t id);
            tag_viewer get_tag_by_name(const art::ustring& name);
            uint64_t get_tag_id(const art::ustring& name);


            void set_destructor(art::shared_ptr<FuncEnvironment> fn);
            void set_copy(art::shared_ptr<FuncEnvironment> fn);
            void set_move(art::shared_ptr<FuncEnvironment> fn);
            void set_compare(art::shared_ptr<FuncEnvironment> fn);
            void set_constructor(art::shared_ptr<FuncEnvironment> fn);
            void set_name(const art::ustring& name);
            void set_structure_size(size_t size);
            void set_allow_auto_copy(bool allow);

            void add_method(const art::ustring& name, ClassAccess access, art::shared_ptr<FuncEnvironment> fn, list_array<list_array<std::pair<ValueMeta, art::ustring>>>&& args = {}, list_array<ValueMeta>&& ret = {}, const art::ustring& owner_name = art::ustring());
            void remove_method(const art::ustring& name, ClassAccess access);
            void rename_method(const art::ustring& old_name, const art::ustring& new_name, ClassAccess access);
            void clear_methods();

            void add_value(const art::ustring& name, ClassAccess access, ValueMeta type, bool allow_abstract_assign, bool inlined, uint8_t bit_offset, uint16_t bit_used, size_t offset, bool zero_after_cleanup);
            void remove_value(const art::ustring& name, ClassAccess access);
            void rename_value(const art::ustring& old_name, const art::ustring& new_name, ClassAccess access);
            void clear_values();

            void add_static(const art::ustring& name, ClassAccess access, ValueItem&& value);
            void remove_static(const art::ustring& name, ClassAccess access);
            void rename_static(const art::ustring& old_name, const art::ustring& new_name, ClassAccess access);
            void clear_statics();

            void add_tag(const std::string& name, ValueItem&& value, art::shared_ptr<FuncEnvironment> enviro);
            void remove_tag(const std::string& name);
            void rename_tag(const art::ustring& old_name, const art::ustring& new_name);
            void clear_tags();


        private:
            inline AttachADynamicVirtualTable* dyn() {
                return reinterpret_cast<AttachADynamicVirtualTable*>(vtable);
            }

            inline AttachAVirtualTable* stat() {
                return reinterpret_cast<AttachAVirtualTable*>(vtable);
            }
        };


        //not thread safe!
        namespace memory {
            //returns faarr[faarr[ptr from, ptr to, len, str desk, bool is_fault]...], args: array/value ptr
            ValueItem* dump(ValueItem*, uint32_t);
        }

        //not thread safe!
        namespace stack {
            //reduce stack size, returns bool, args: shrink threshold(optional)
            ValueItem* shrink(ValueItem*, uint32_t);
            //grow stack size, returns bool, args: grow count
            ValueItem* prepare(ValueItem*, uint32_t);
            //make sure stack size is enough and increase if too small, returns bool, args: grow count
            ValueItem* reserve(ValueItem*, uint32_t);


            //returns faarr[faarr[ptr from, ptr to, str desk, bool is_fault]...], args: none
            ValueItem* dump(ValueItem*, uint32_t);

            //better stack is supported?
            ValueItem* bs_supported(ValueItem*, uint32_t);
            //better stack is os depended implementation, for example, on windows supported because that use guard pages for auto grow stack, linux may be support that too but not sure

            //in windows we can deallocate unused stacks and set guard page to let windows auto increase stack size
            //also we can manually increase stack size by manually allocating pages and set guard page to another position
            // that allow reduce memory usage and increase application performance

            //in linux stack can be increased automatically by mmap(null, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_GROWSDOWN, -1, 0)
            //but we may be cannot deallocate unused stacks

            //TODO: after reading manuals may be better use PROT_NONE instead of guard pages and catch SIGSEGV to increase stack size
            ValueItem* used_size(ValueItem*, uint32_t);
            ValueItem* unused_size(ValueItem*, uint32_t);
            ValueItem* allocated_size(ValueItem*, uint32_t);
            ValueItem* free_size(ValueItem*, uint32_t);


            //returns [{file_path, fun_name, line, column},...], args: framesToSkip, include_native, max_frames
            ValueItem* trace(ValueItem*, uint32_t);
            //returns [{file_path, fun_name, line, column},...], args: framesToSkip, include_native, max_frames
            ValueItem* clean_trace(ValueItem*, uint32_t);
            //returns [rip,...], args: framesToSkip, include_native, max_frames
            ValueItem* trace_frames(ValueItem*, uint32_t);
            //returns {file_path, fun_name, line, column}, args: frame,(optional include_native)
            ValueItem* resolve_frame(ValueItem*, uint32_t);
        }

        namespace run_time {
            //not recommended to use, use only for debug
            ValueItem* gc_pause(ValueItem*, uint32_t);
            ValueItem* gc_resume(ValueItem*, uint32_t);

            //gc can ignore this hint
            ValueItem* gc_hint_collect(ValueItem*, uint32_t);

            namespace native {
                namespace constructor {
                    ValueItem* createProxy_NativeValue(ValueItem*, uint32_t);    // used in NativeTemplate
                    ValueItem* createProxy_NativeTemplate(ValueItem*, uint32_t); // used in NativeLib
                    ValueItem* createProxy_NativeLib(ValueItem*, uint32_t);      // args: str lib path(resolved by os), do not use functions from this instance when destructor called
                }

                void init();
            }
        }

        namespace constructor {
            ValueItem* createProxy_function_builder(ValueItem*, uint32_t);
            ValueItem* createProxy_index_pos(ValueItem*, uint32_t);
        }

        vtable_viewer view_structure(Structure& str);
        ValueItem* view_structure(ValueItem*, uint32_t);

        void init(bool vtable_full_mode = false, bool allow_self_build = false);
    }
}

#endif /* SRC_RUN_TIME_LIBRARY_INTERNAL */
