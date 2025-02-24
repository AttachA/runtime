// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/AttachA_CXX.hpp>
#include <run_time/asm/CASM.hpp>
#include <run_time/asm/attacha_environment.hpp>
#include <run_time/asm/dynamic_call.hpp>
#include <run_time/func_enviro_builder.hpp>
#include <run_time/library/internal.hpp>
#include <run_time/tasks/util/light_stack.hpp>

namespace art {
#pragma region vtable viewer

    namespace internal {
        art::ustring tag_viewer::get_name() const {
            return ((StructureTag*)_internal)->name;
        }

        ValueItem tag_viewer::get_value() const {
            return ((StructureTag*)_internal)->value;
        }

        art::shared_ptr<FuncEnvironment> tag_viewer::get_enviro() const {
            return ((StructureTag*)_internal)->enviro;
        }

        void tag_viewer::set_value(ValueItem&& value) {
            ((StructureTag*)_internal)->value = std::move(value);
        }

        void tag_viewer::set_enviro(art::shared_ptr<FuncEnvironment> enviro) {
            ((StructureTag*)_internal)->enviro = enviro;
        }

        art::ustring method_viewer::get_name() const {
            return ((MethodInfo*)_internal)->name;
        }

        ClassAccess method_viewer::get_access() const {
            return ((MethodInfo*)_internal)->access;
        }

        bool method_viewer::is_deletable() const {
            return ((MethodInfo*)_internal)->deletable;
        }

        art::shared_ptr<FuncEnvironment> method_viewer::get_function() const {
            return ((MethodInfo*)_internal)->ref;
        }

        art::ustring method_viewer::get_owner_name() const {
            return ((MethodInfo*)_internal)->owner_name;
        }

        list_array<tag_viewer> method_viewer::get_tags() const {
            MethodInfo* info = (MethodInfo*)_internal;
            if (info->optional == nullptr)
                return {};
            return info->optional->tags.convert<tag_viewer>([](const StructureTag& tag) { return tag_viewer{&tag}; });
        }

        list_array<list_array<std::pair<ValueMeta, art::ustring>>> method_viewer::get_args() const {
            MethodInfo* info = (MethodInfo*)_internal;
            if (info->optional == nullptr)
                return {};
            return info->optional->arguments;
        }

        list_array<ValueMeta> method_viewer::get_return_values() const {
            MethodInfo* info = (MethodInfo*)_internal;
            if (info->optional == nullptr)
                return {};
            return info->optional->return_values;
        }

        void method_viewer::set_access(ClassAccess access) {
            ((MethodInfo*)_internal)->access = access;
        }

        void method_viewer::set_function(art::shared_ptr<FuncEnvironment> fn, list_array<list_array<std::pair<ValueMeta, art::ustring>>>&& args, list_array<ValueMeta>&& ret) {
            MethodInfo* info = (MethodInfo*)_internal;
            info->ref = fn;
            if (!(args.empty() && ret.empty())) {
                if (info->optional == nullptr)
                    info->optional = new MethodInfo::Optional{std::move(ret), std::move(args)};
                else {
                    info->optional->return_values = std::move(ret);
                    info->optional->arguments = std::move(args);
                }
            }
        }

        void method_viewer::add_tag(const std::string& name, ValueItem&& value, art::shared_ptr<FuncEnvironment> enviro) {
            MethodInfo* info = (MethodInfo*)_internal;
            if (info->optional == nullptr)
                info->optional = new MethodInfo::Optional{};
            if (!info->optional->tags.contains_one([&](auto& it) { return it.name == name; }))
                info->optional->tags.push_back({name, enviro, std::move(value)});
            else
                throw AlreadyDefinedException(name + " tag already defined in " + info->name + "(" + enum_to_string(info->access) + ") method");
        }

        void method_viewer::remove_tag(const std::string& name) {
            MethodInfo* info = (MethodInfo*)_internal;
            if (info->optional == nullptr)
                return;
            info->optional->tags.remove_one([&](auto& it) { return it.name == name; });
        }

        void method_viewer::clear_tags() {
            MethodInfo* info = (MethodInfo*)_internal;
            if (info->optional == nullptr)
                return;
            info->optional->tags.clear();
        }

        art::ustring static_viewer::get_name() const {
            return ((StructStaticValue*)_internal)->name;
        }

        ClassAccess static_viewer::get_access() const {
            return ((StructStaticValue*)_internal)->access;
        }

        list_array<tag_viewer> static_viewer::get_tags() const {
            StructStaticValue* value = (StructStaticValue*)_internal;
            if (value->optional_tags == nullptr)
                return {};
            return value->optional_tags->convert<tag_viewer>([](const StructureTag& tag) { return tag_viewer{&tag}; });
        }

        ValueItem static_viewer::copy_value() const {
            return ((StructStaticValue*)_internal)->value;
        }

        void static_viewer::set_access(ClassAccess access) {
            ((StructStaticValue*)_internal)->access = access;
        }

        void static_viewer::add_tag(const std::string& name, ValueItem&& value, art::shared_ptr<FuncEnvironment> enviro) {
            StructStaticValue* info = (StructStaticValue*)_internal;
            if (info->optional_tags == nullptr)
                info->optional_tags = new list_array<StructureTag>{};
            if (!info->optional_tags->contains_one([&](auto& it) { return it.name == name; }))
                info->optional_tags->push_back({name, enviro, std::move(value)});
            else
                throw AlreadyDefinedException(name + " tag already defined in " + info->name + "(" + enum_to_string(info->access) + ") static value");
        }

        void static_viewer::remove_tag(const std::string& name) {
            StructStaticValue* info = (StructStaticValue*)_internal;
            if (info->optional_tags == nullptr)
                return;
            info->optional_tags->remove_one([&](auto& it) { return it.name == name; });
        }

        void static_viewer::clear_tags() {
            StructStaticValue* info = (StructStaticValue*)_internal;
            if (info->optional_tags == nullptr)
                return;
            info->optional_tags->clear();
        }

        void static_viewer::set_value(ValueItem&& value) {
            ((StructStaticValue*)_internal)->value = std::move(value);
        }

        art::ustring value_viewer::get_name() const {
            return ((ValueInfo*)_internal)->name;
        }

        ClassAccess value_viewer::get_access() const {
            return ((ValueInfo*)_internal)->access;
        }

        ValueMeta value_viewer::get_type() const {
            return ((ValueInfo*)_internal)->type;
        }

        bool value_viewer::is_allow_abstract_assign() const {
            return ((ValueInfo*)_internal)->allow_abstract_assign;
        }

        bool value_viewer::is_inlined() const {
            return ((ValueInfo*)_internal)->inlined;
        }

        uint8_t value_viewer::get_bit_offset() const {
            return ((ValueInfo*)_internal)->bit_offset;
        }

        uint16_t value_viewer::get_bit_used() const {
            return ((ValueInfo*)_internal)->bit_used;
        }

        size_t value_viewer::get_offset() const {
            return ((ValueInfo*)_internal)->offset;
        }

        bool value_viewer::get_zero_after_cleanup() const {
            return ((ValueInfo*)_internal)->zero_after_cleanup;
        }

        list_array<tag_viewer> value_viewer::get_tags() const {
            ValueInfo* info = (ValueInfo*)_internal;
            if (info->optional_tags == nullptr)
                return {};
            return info->optional_tags->convert<tag_viewer>([](const StructureTag& tag) { return tag_viewer{&tag}; });
        }

        void value_viewer::set_access(ClassAccess access) {
            ((ValueInfo*)_internal)->access = access;
        }

        void value_viewer::set_type(ValueMeta type) {
            ((ValueInfo*)_internal)->type = type;
        }

        void value_viewer::set_allow_abstract_assign(bool allow) {
            ((ValueInfo*)_internal)->allow_abstract_assign = allow;
        }

        void value_viewer::set_inlined(bool inlined) {
            ((ValueInfo*)_internal)->inlined = inlined;
        }

        void value_viewer::set_bit_offset(uint8_t offset) {
            ((ValueInfo*)_internal)->bit_offset = offset;
        }

        void value_viewer::set_bit_used(uint16_t used) {
            ((ValueInfo*)_internal)->bit_used = used;
        }

        void value_viewer::set_offset(size_t offset) {
            ((ValueInfo*)_internal)->offset = offset;
        }

        void value_viewer::set_zero_after_cleanup(bool zero) {
            ((ValueInfo*)_internal)->zero_after_cleanup = zero;
        }

        void value_viewer::add_tag(const std::string& name, ValueItem&& value, art::shared_ptr<FuncEnvironment> enviro) {
            ValueInfo* info = (ValueInfo*)_internal;
            if (info->optional_tags == nullptr)
                info->optional_tags = new list_array<StructureTag>{};
            if (!info->optional_tags->contains_one([&](auto& it) { return it.name == name; }))
                info->optional_tags->push_back({name, enviro, std::move(value)});
            else
                throw AlreadyDefinedException(name + " tag already defined in " + info->name + "(" + enum_to_string(info->access) + ") value");
        }

        void value_viewer::remove_tag(const std::string& name) {
            ValueInfo* info = (ValueInfo*)_internal;
            if (info->optional_tags == nullptr)
                return;
            info->optional_tags->remove_one([&](auto& it) { return it.name == name; });
        }

        void value_viewer::clear_tags() {
            ValueInfo* info = (ValueInfo*)_internal;
            if (info->optional_tags == nullptr)
                return;
            info->optional_tags->clear();
        }

        art::shared_ptr<FuncEnvironment> vtable_viewer::get_destructor() {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                return dyn()->destructor;
            case Structure::VTableMode::AttachAVirtualTable:
                return stat()->getAfterMethods()->destructor;
            default:
                throw NotImplementedException();
            }
        }

        art::shared_ptr<FuncEnvironment> vtable_viewer::get_copy() {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                return dyn()->copy;
            case Structure::VTableMode::AttachAVirtualTable:
                return stat()->getAfterMethods()->copy;
            default:
                throw NotImplementedException();
            }
        }

        art::shared_ptr<FuncEnvironment> vtable_viewer::get_move() {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                return dyn()->move;
            case Structure::VTableMode::AttachAVirtualTable:
                return stat()->getAfterMethods()->move;
            default:
                throw NotImplementedException();
            }
        }

        art::shared_ptr<FuncEnvironment> vtable_viewer::get_compare() {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                return dyn()->compare;
            case Structure::VTableMode::AttachAVirtualTable:
                return stat()->getAfterMethods()->compare;
            default:
                throw NotImplementedException();
            }
        }

        art::shared_ptr<FuncEnvironment> vtable_viewer::get_constructor() {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                return dyn()->constructor;
            case Structure::VTableMode::AttachAVirtualTable:
                return stat()->getAfterMethods()->constructor;
            default:
                throw NotImplementedException();
            }
        }

        art::ustring vtable_viewer::get_name() {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                return dyn()->name;
            case Structure::VTableMode::AttachAVirtualTable:
                return stat()->getName();
            default:
                throw NotImplementedException();
            }
        }

        size_t vtable_viewer::get_structure_size() {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                return dyn()->structure_bytes;
            case Structure::VTableMode::AttachAVirtualTable:
                return stat()->structure_bytes;
            default:
                throw NotImplementedException();
            }
        }

        bool vtable_viewer::get_allow_auto_copy() {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                return dyn()->allow_auto_copy;
            case Structure::VTableMode::AttachAVirtualTable:
                return stat()->allow_auto_copy;
            default:
                throw NotImplementedException();
            }
        }

        list_array<method_viewer> vtable_viewer::get_methods() {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                return dyn()->methods.convert<method_viewer>([](const MethodInfo& info) {
                    return method_viewer{&info};
                });
            case Structure::VTableMode::AttachAVirtualTable: {
                size_t len = 0;
                MethodInfo* methods = stat()->getMethodsInfo(len);
                list_array<method_viewer> res;
                res.reserve_back(len);
                for (size_t i = 0; i < len; i++)
                    res.push_back(method_viewer{&methods[i]});
                return res;
            }
            default:
                throw NotImplementedException();
            }
        }

        method_viewer vtable_viewer::get_method_by_id(uint64_t id) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                return method_viewer{&dyn()->getMethodInfo(id)};
            case Structure::VTableMode::AttachAVirtualTable:
                return method_viewer{&stat()->getMethodInfo(id)};
            default:
                throw NotImplementedException();
            }
        }

        method_viewer vtable_viewer::get_method_by_name(const art::ustring& name, ClassAccess access) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                return method_viewer{&dyn()->getMethodInfo(name, access)};
            case Structure::VTableMode::AttachAVirtualTable:
                return method_viewer{&stat()->getMethodInfo(name, access)};
            default:
                throw NotImplementedException();
            }
        }

        uint64_t vtable_viewer::get_method_id(const art::ustring& name, ClassAccess access) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                return dyn()->getMethodIndex(name, access);
            case Structure::VTableMode::AttachAVirtualTable:
                return stat()->getMethodIndex(name, access);
            default:
                throw NotImplementedException();
            }
        }

        list_array<value_viewer> vtable_viewer::get_values() {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                return dyn()->values.convert<value_viewer>([](const ValueInfo& info) {
                    return value_viewer{&info};
                });
            case Structure::VTableMode::AttachAVirtualTable: {
                size_t len = 0;
                ValueInfo* values = stat()->getValuesInfo(len);
                list_array<value_viewer> res;
                res.reserve_back(len);
                for (size_t i = 0; i < len; i++)
                    res.push_back(value_viewer{&values[i]});
                return res;
            }
            default:
                throw NotImplementedException();
            }
        }

        value_viewer vtable_viewer::get_value_by_id(uint64_t id) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                return value_viewer{&dyn()->getValueInfo(id)};
            case Structure::VTableMode::AttachAVirtualTable:
                return value_viewer{&stat()->getValueInfo(id)};
            default:
                throw NotImplementedException();
            }
        }

        value_viewer vtable_viewer::get_value_by_name(const art::ustring& name, ClassAccess access) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                return value_viewer{&dyn()->getValueInfo(name, access)};
            case Structure::VTableMode::AttachAVirtualTable:
                return value_viewer{&stat()->getValueInfo(name, access)};
            default:
                throw NotImplementedException();
            }
        }

        uint64_t vtable_viewer::get_value_id(const art::ustring& name, ClassAccess access) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                return dyn()->getValueIndex(name, access);
            case Structure::VTableMode::AttachAVirtualTable:
                return stat()->getValueIndex(name, access);
            default:
                throw NotImplementedException();
            }
        }

        list_array<static_viewer> vtable_viewer::get_statics() {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                return dyn()->staticValues().convert<static_viewer>([](const StructStaticValue& info) {
                    return static_viewer{&info};
                });
            case Structure::VTableMode::AttachAVirtualTable:
                return stat()->staticValues().convert<static_viewer>([](const StructStaticValue& info) {
                    return static_viewer{&info};
                });
            default:
                throw NotImplementedException();
            }
        }

        static_viewer vtable_viewer::get_static_by_name(const art::ustring& name, ClassAccess access) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                return static_viewer{dyn()->getStaticValue(name, access, false)};
            case Structure::VTableMode::AttachAVirtualTable:
                return static_viewer{stat()->getStaticValue(name, access, false)};
            default:
                throw NotImplementedException();
            }
        }

        list_array<tag_viewer> vtable_viewer::get_tags() {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                if (dyn()->tags != nullptr)
                    return dyn()->tags->convert<tag_viewer>([](const StructureTag& tag) { return tag_viewer{&tag}; });
                else
                    return {};
            case Structure::VTableMode::AttachAVirtualTable: {
                list_array<StructureTag>* tags = stat()->getStructureTags();
                if (tags != nullptr)
                    return tags->convert<tag_viewer>([](const StructureTag& tag) { return tag_viewer{&tag}; });
                else
                    return {};
            }
            default:
                throw NotImplementedException();
            }
        }

        tag_viewer vtable_viewer::get_tag_by_id(uint64_t id) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                if (dyn()->tags != nullptr)
                    return tag_viewer{&dyn()->tags->at(id)};
                else
                    return {};
            case Structure::VTableMode::AttachAVirtualTable: {
                list_array<StructureTag>* tags = stat()->getStructureTags();
                if (tags != nullptr)
                    return tag_viewer{&tags->at(id)};
                else
                    return {};
            }
            default:
                throw NotImplementedException();
            }
        }

        tag_viewer vtable_viewer::get_tag_by_name(const art::ustring& name) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable: {
                list_array<StructureTag>* tags = dyn()->tags;
                if (tags != nullptr)
                    return tag_viewer{&(*tags)[tags->find_if([&name](const StructureTag& tag) { return tag.name == name; })]};
                else
                    throw InvalidOperation("Value not found");
            }
            case Structure::VTableMode::AttachAVirtualTable: {
                list_array<StructureTag>* tags = stat()->getStructureTags();
                if (tags != nullptr)
                    return tag_viewer{&(*tags)[tags->find_if([&name](const StructureTag& tag) { return tag.name == name; })]};
                else
                    throw InvalidOperation("Value not found");
            }
            default:
                throw NotImplementedException();
            }
        }

        uint64_t vtable_viewer::get_tag_id(const art::ustring& name) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable: {
                list_array<StructureTag>* tags = dyn()->tags;
                if (tags != nullptr)
                    return tags->find_if([&name](const StructureTag& tag) { return tag.name == name; });
                else
                    throw InvalidOperation("Value not found");
            }
            case Structure::VTableMode::AttachAVirtualTable: {
                list_array<StructureTag>* tags = stat()->getStructureTags();
                if (tags != nullptr)
                    return tags->find_if([&name](const StructureTag& tag) { return tag.name == name; });
                else
                    throw InvalidOperation("Value not found");
            }
            default:
                throw NotImplementedException();
            }
        }

        void vtable_viewer::set_destructor(art::shared_ptr<FuncEnvironment> fn) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                dyn()->destructor = fn;
                break;
            case Structure::VTableMode::AttachAVirtualTable:
                stat()->getAfterMethods()->destructor = fn;
                break;
            default:
                throw NotImplementedException();
            }
        }

        void vtable_viewer::set_copy(art::shared_ptr<FuncEnvironment> fn) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                dyn()->copy = fn;
                break;
            case Structure::VTableMode::AttachAVirtualTable:
                stat()->getAfterMethods()->copy = fn;
                break;
            default:
                throw NotImplementedException();
            }
        }

        void vtable_viewer::set_move(art::shared_ptr<FuncEnvironment> fn) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                dyn()->move = fn;
                break;
            case Structure::VTableMode::AttachAVirtualTable:
                stat()->getAfterMethods()->move = fn;
                break;
            default:
                throw NotImplementedException();
            }
        }

        void vtable_viewer::set_compare(art::shared_ptr<FuncEnvironment> fn) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                dyn()->compare = fn;
                break;
            case Structure::VTableMode::AttachAVirtualTable:
                stat()->getAfterMethods()->compare = fn;
                break;
            default:
                throw NotImplementedException();
            }
        }

        void vtable_viewer::set_constructor(art::shared_ptr<FuncEnvironment> fn) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                dyn()->constructor = fn;
                break;
            case Structure::VTableMode::AttachAVirtualTable:
                stat()->getAfterMethods()->constructor = fn;
                break;
            default:
                throw NotImplementedException();
            }
        }

        void vtable_viewer::set_name(const art::ustring& name) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                dyn()->name = name;
                break;
            case Structure::VTableMode::AttachAVirtualTable:
                stat()->setName(name);
                break;
            default:
                throw NotImplementedException();
            }
        }

        void vtable_viewer::set_structure_size(size_t size) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                dyn()->structure_bytes = size;
                break;
            case Structure::VTableMode::AttachAVirtualTable:
                stat()->structure_bytes = size;
                break;
            default:
                throw NotImplementedException();
            }
        }

        void vtable_viewer::set_allow_auto_copy(bool allow) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                dyn()->allow_auto_copy = allow;
                break;
            case Structure::VTableMode::AttachAVirtualTable:
                stat()->allow_auto_copy = allow;
                break;
            default:
                throw NotImplementedException();
            }
        }

        void vtable_viewer::add_method(const art::ustring& name, ClassAccess access, art::shared_ptr<FuncEnvironment> fn, list_array<list_array<std::pair<ValueMeta, art::ustring>>>&& args, list_array<ValueMeta>&& ret, const art::ustring& owner_name) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                dyn()->addMethod(name, fn, access, std::move(ret), std::move(args), {}, owner_name);
                break;
            case Structure::VTableMode::AttachAVirtualTable:
                throw InvalidOperation("Cannot add method to static vtable");
                break;
            default:
                throw NotImplementedException();
            }
        }

        void vtable_viewer::remove_method(const art::ustring& name, ClassAccess access) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                dyn()->removeMethod(name, access);
                break;
            case Structure::VTableMode::AttachAVirtualTable:
                throw InvalidOperation("Cannot remove method from static vtable");
                break;
            default:
                throw NotImplementedException();
            }
        }

        void vtable_viewer::rename_method(const art::ustring& old_name, const art::ustring& new_name, ClassAccess access) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                dyn()->getMethodInfo(old_name, access).name = new_name;
                break;
            case Structure::VTableMode::AttachAVirtualTable:
                throw InvalidOperation("Cannot rename method in static vtable");
                break;
            default:
                throw NotImplementedException();
            }
        }

        void vtable_viewer::clear_methods() {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                dyn()->methods.clear();
                break;
            case Structure::VTableMode::AttachAVirtualTable:
                throw InvalidOperation("Cannot clear methods in static vtable");
                break;
            default:
                throw NotImplementedException();
            }
        }

        void vtable_viewer::add_value(const art::ustring& name, ClassAccess access, ValueMeta type, bool allow_abstract_assign, bool inlined, uint8_t bit_offset, uint16_t bit_used, size_t offset, bool zero_after_cleanup) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                dyn()->addValue(art::ValueInfo{name, offset, type, bit_used, bit_offset, inlined, allow_abstract_assign, access, {}, zero_after_cleanup});
                break;
            case Structure::VTableMode::AttachAVirtualTable:
                throw InvalidOperation("Cannot add value to static vtable");
                break;
            default:
                throw NotImplementedException();
            }
        }

        void vtable_viewer::remove_value(const art::ustring& name, ClassAccess access) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                dyn()->removeValue(name, access);
                break;
            case Structure::VTableMode::AttachAVirtualTable:
                throw InvalidOperation("Cannot remove value from static vtable");
                break;
            default:
                throw NotImplementedException();
            }
        }

        void vtable_viewer::rename_value(const art::ustring& old_name, const art::ustring& new_name, ClassAccess access) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                dyn()->getValueInfo(old_name, access).name = new_name;
                break;
            case Structure::VTableMode::AttachAVirtualTable:
                throw InvalidOperation("Cannot rename value in static vtable");
                break;
            default:
                throw NotImplementedException();
            }
        }

        void vtable_viewer::clear_values() {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                dyn()->values.clear();
                break;
            case Structure::VTableMode::AttachAVirtualTable:
                throw InvalidOperation("Cannot clear values in static vtable");
                break;
            default:
                throw NotImplementedException();
            }
        }

        void vtable_viewer::add_static(const art::ustring& name, ClassAccess access, ValueItem&& value) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                dyn()->staticValues().push_back(StructStaticValue{name, nullptr, std::move(value), access});
                break;
            case Structure::VTableMode::AttachAVirtualTable:
                stat()->staticValues().push_back(StructStaticValue{name, nullptr, std::move(value), access});
                break;
            default:
                throw NotImplementedException();
            }
        }

        void vtable_viewer::remove_static(const art::ustring& name, ClassAccess access) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                dyn()->static_values.remove_one([&](auto& it) {
                    return it.name == name && it.access == access;
                });
                break;
            case Structure::VTableMode::AttachAVirtualTable:
                stat()->staticValues().remove_one([&](auto& it) {
                    return it.name == name && it.access == access;
                });
                break;
            default:
                throw NotImplementedException();
            }
        }

        void vtable_viewer::rename_static(const art::ustring& old_name, const art::ustring& new_name, ClassAccess access) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                dyn()->getStaticValue(old_name, access, false)->name = new_name;
                break;
            case Structure::VTableMode::AttachAVirtualTable:
                stat()->getStaticValue(old_name, access, false)->name = new_name;
                break;
            default:
                throw NotImplementedException();
            }
        }

        void vtable_viewer::clear_statics() {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                dyn()->static_values.clear();
                break;
            case Structure::VTableMode::AttachAVirtualTable:
                stat()->staticValues().clear();
                break;
            default:
                throw NotImplementedException();
            }
        }

        void vtable_viewer::add_tag(const std::string& name, ValueItem&& value, art::shared_ptr<FuncEnvironment> enviro) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                if (dyn()->tags == nullptr)
                    dyn()->tags = new list_array<StructureTag>{};
                if (!dyn()->tags->contains_one([&](auto& it) { return it.name == name; }))
                    dyn()->tags->push_back({name, enviro, std::move(value)});
                else
                    throw AlreadyDefinedException(name + " tag already defined in " + dyn()->name + " dynamic vtable");
                break;
            case Structure::VTableMode::AttachAVirtualTable: {
                list_array<StructureTag>* tags = stat()->getStructureTags();
                if (tags == nullptr)
                    tags = stat()->getAfterMethods()->tags = new list_array<StructureTag>{};
                if (!tags->contains_one([&](auto& it) { return it.name == name; }))
                    tags->push_back({name, enviro, std::move(value)});
                else
                    throw AlreadyDefinedException(name + " tag already defined in " + stat()->getName() + " static vtable");
                break;
            }
            default:
                throw NotImplementedException();
            }
        }

        void vtable_viewer::remove_tag(const std::string& name) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                if (dyn()->tags != nullptr)
                    dyn()->tags->remove_one([&](auto& it) { return it.name == name; });
                break;
            case Structure::VTableMode::AttachAVirtualTable: {
                list_array<StructureTag>* tags = stat()->getStructureTags();
                if (tags != nullptr)
                    tags->remove_one([&](auto& it) { return it.name == name; });
                break;
            }
            default:
                throw NotImplementedException();
            }
        }

        void vtable_viewer::rename_tag(const art::ustring& old_name, const art::ustring& new_name) {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                if (dyn()->tags != nullptr) {
                    auto& tags = *dyn()->tags;
                    auto it = tags.find_if([&](auto& it) { return it.name == old_name; });
                    if (it != tags.npos)
                        tags[it].name = new_name;
                }
                break;
            case Structure::VTableMode::AttachAVirtualTable: {
                list_array<StructureTag>* tags = stat()->getStructureTags();
                if (tags != nullptr) {
                    auto it = tags->find_if([&](auto& it) { return it.name == old_name; });
                    if (it != tags->npos)
                        (*tags)[it].name = new_name;
                }
                break;
            }
            default:
                throw NotImplementedException();
            }
        }

        void vtable_viewer::clear_tags() {
            switch (mode) {
            case Structure::VTableMode::AttachADynamicVirtualTable:
                if (dyn()->tags != nullptr) {
                    delete dyn()->tags;
                    dyn()->tags = nullptr;
                }
                break;
            case Structure::VTableMode::AttachAVirtualTable: {
                list_array<StructureTag>* tags = stat()->getStructureTags();
                if (tags != nullptr) {
                    delete tags;
                    stat()->getAfterMethods()->tags = nullptr;
                }
                break;
            }
            default:
                throw NotImplementedException();
            }
        }

        AttachAVirtualTable* method_view;
        AttachAVirtualTable* static_view;
        AttachAVirtualTable* value_view;
        AttachAVirtualTable* tag_view;
        AttachAVirtualTable* vtable_view;

        namespace method_view_impl {
            AttachAFunc(funcs_method_view_get_name, 1) {
                return CXX::Interface::getExtractAs<method_viewer>(args[0], method_view).get_name();
            }

            AttachAFunc(funcs_method_view_get_access, 1) {
                return (uint8_t)CXX::Interface::getExtractAs<method_viewer>(args[0], method_view).get_access();
            }

            AttachAFunc(funcs_method_view_is_deletable, 1) {
                return CXX::Interface::getExtractAs<method_viewer>(args[0], method_view).is_deletable();
            }

            AttachAFunc(funcs_method_view_get_function, 1) {
                return CXX::Interface::getExtractAs<method_viewer>(args[0], method_view).get_function();
            }

            AttachAFunc(funcs_method_view_get_owner_name, 1) {
                return CXX::Interface::getExtractAs<method_viewer>(args[0], method_view).get_owner_name();
            }

            AttachAFunc(funcs_method_view_get_tags, 1) {
                return CXX::Interface::getExtractAs<method_viewer>(args[0], method_view).get_tags().convert<ValueItem>([](const tag_viewer& tag) {
                    return ValueItem(CXX::Interface::constructStructure<tag_viewer>(tag_view, tag), no_copy);
                });
            }

            AttachAFunc(funcs_method_view_get_args, 1) {
                return CXX::Interface::getExtractAs<method_viewer>(args[0], method_view).get_args().convert<ValueItem>([](const list_array<std::pair<ValueMeta, art::ustring>>& args) -> ValueItem {
                    return args.convert<ValueItem>([](const std::pair<ValueMeta, art::ustring>& arg) {
                        return ValueItem{ValueItem(arg.first), ValueItem(arg.second)};
                    });
                });
            }

            AttachAFunc(funcs_method_view_get_return_values, 1) {
                return CXX::Interface::getExtractAs<method_viewer>(args[0], method_view).get_return_values().convert<ValueItem>([](ValueMeta&& arg) -> ValueItem { return arg; });
            }

            AttachAFunc(funcs_method_view_set_access, 2) {
                CXX::Interface::getExtractAs<method_viewer>(args[0], method_view).set_access((ClassAccess)(uint8_t)args[1]);
                return nullptr;
            }

            AttachAFunc(funcs_method_view_set_function, 2) {
                list_array<list_array<std::pair<ValueMeta, art::ustring>>> fun_args;
                list_array<ValueMeta> results;
                if (len > 2)
                    fun_args = ((list_array<ValueItem>)args[2]).convert_take<list_array<std::pair<ValueMeta, art::ustring>>>([](ValueItem&& inner) {
                        return ((list_array<ValueItem>)inner).convert<std::pair<ValueMeta, art::ustring>>([](ValueItem&& item) {
                            return std::pair<ValueMeta, art::ustring>((ValueMeta)item[0], item[1]);
                        });
                    });
                if (len > 3)
                    results = ((list_array<ValueItem>)args[3]).convert_fn([](ValueItem&& item) {
                        return (ValueMeta)item;
                    });

                CXX::Interface::getExtractAs<method_viewer>(args[0], method_view).set_function(args[1].funPtr() ? *args[1].funPtr() : nullptr, std::move(fun_args), std::move(results));
                return nullptr;
            }

            AttachAFunc(funcs_method_view_add_tag, 4) {
                CXX::Interface::getExtractAs<method_viewer>(args[0], method_view).add_tag((art::ustring)args[1], std::move(args[2]), args[1].funPtr() ? *args[1].funPtr() : nullptr);
                return nullptr;
            }

            AttachAFunc(funcs_method_view_remove_tag, 2) {
                CXX::Interface::getExtractAs<method_viewer>(args[0], method_view).remove_tag((art::ustring)args[1]);
                return nullptr;
            }

            AttachAFunc(funcs_method_view_clear_tags, 1) {
                CXX::Interface::getExtractAs<method_viewer>(args[0], method_view).clear_tags();
                return nullptr;
            }
        }

        namespace static_view_impl {
            AttachAFunc(funcs_static_view_get_name, 1) {
                return CXX::Interface::getExtractAs<static_viewer>(args[0], value_view).get_name();
            }

            AttachAFunc(funcs_static_view_get_access, 1) {
                return (uint8_t)CXX::Interface::getExtractAs<static_viewer>(args[0], value_view).get_access();
            }

            AttachAFunc(funcs_static_view_copy_value, 1) {
                return CXX::Interface::getExtractAs<static_viewer>(args[0], value_view).copy_value();
            }

            AttachAFunc(funcs_static_view_get_tags, 1) {
                return CXX::Interface::getExtractAs<static_viewer>(args[0], value_view).get_tags().convert<ValueItem>([](tag_viewer&& tag) {
                    return ValueItem(CXX::Interface::constructStructure<tag_viewer>(tag_view, tag), no_copy);
                });
            }

            AttachAFunc(funcs_static_view_set_access, 2) {
                CXX::Interface::getExtractAs<static_viewer>(args[0], value_view).set_access((ClassAccess)(uint8_t)args[1]);
                return nullptr;
            }

            AttachAFunc(funcs_static_view_add_tag, 4) {
                CXX::Interface::getExtractAs<static_viewer>(args[0], value_view).add_tag((art::ustring)args[1], std::move(args[2]), args[1].funPtr() ? *args[1].funPtr() : nullptr);
                return nullptr;
            }

            AttachAFunc(funcs_static_view_remove_tag, 2) {
                CXX::Interface::getExtractAs<static_viewer>(args[0], value_view).remove_tag((art::ustring)args[1]);
                return nullptr;
            }

            AttachAFunc(funcs_static_view_clear_tags, 1) {
                CXX::Interface::getExtractAs<static_viewer>(args[0], value_view).clear_tags();
                return nullptr;
            }

            AttachAFunc(funcs_static_view_set_value, 2) {
                CXX::Interface::getExtractAs<static_viewer>(args[0], value_view).set_value(std::move(CXX::Interface::getExtractAs<ValueItem>(args[1], value_view)));
                return nullptr;
            }
        }

        namespace value_view_impl {
            AttachAFunc(funcs_value_view_get_name, 1) {
                return CXX::Interface::getExtractAs<value_viewer>(args[0], value_view).get_name();
            }

            AttachAFunc(funcs_value_view_get_access, 1) {
                return (uint8_t)CXX::Interface::getExtractAs<value_viewer>(args[0], value_view).get_access();
            }

            AttachAFunc(funcs_value_view_get_type, 1) {
                return CXX::Interface::getExtractAs<value_viewer>(args[0], value_view).get_type();
            }

            AttachAFunc(funcs_value_view_is_allow_abstract_assign, 1) {
                return CXX::Interface::getExtractAs<value_viewer>(args[0], value_view).is_allow_abstract_assign();
            }

            AttachAFunc(funcs_value_view_get_bit_offset, 1) {
                return CXX::Interface::getExtractAs<ValueInfo*>(args[0], value_view)->bit_offset;
            }

            AttachAFunc(funcs_value_view_get_bit_used, 1) {
                return CXX::Interface::getExtractAs<ValueInfo*>(args[0], value_view)->bit_used;
            }

            AttachAFunc(funcs_value_view_is_inlined, 1) {
                return CXX::Interface::getExtractAs<ValueInfo*>(args[0], value_view)->inlined;
            }

            AttachAFunc(funcs_value_view_get_offset, 1) {
                return CXX::Interface::getExtractAs<ValueInfo*>(args[0], value_view)->offset;
            }

            AttachAFunc(funcs_value_view_get_zero_after_cleanup, 1) {
                return CXX::Interface::getExtractAs<ValueInfo*>(args[0], value_view)->zero_after_cleanup;
            }

            AttachAFunc(funcs_value_view_get_tags, 1) {
                return CXX::Interface::getExtractAs<value_viewer>(args[0], value_view).get_tags().convert<ValueItem>([](const tag_viewer& tag) {
                    return ValueItem(CXX::Interface::constructStructure<tag_viewer>(tag_view, tag), no_copy);
                });
            }

            AttachAFunc(funcs_value_view_set_access, 2) {
                CXX::Interface::getExtractAs<value_viewer>(args[0], value_view).set_access((ClassAccess)(uint8_t)args[1]);
                return nullptr;
            }

            AttachAFunc(funcs_value_view_set_type, 2) {
                CXX::Interface::getExtractAs<value_viewer>(args[0], value_view).set_type(CXX::Interface::getExtractAs<ValueMeta>(args[1], value_view));
                return nullptr;
            }

            AttachAFunc(funcs_value_view_set_allow_abstract_assign, 2) {
                CXX::Interface::getExtractAs<value_viewer>(args[0], value_view).set_allow_abstract_assign((bool)args[1]);
                return nullptr;
            }

            AttachAFunc(funcs_value_view_set_inlined, 2) {
                CXX::Interface::getExtractAs<value_viewer>(args[0], value_view).set_inlined((bool)args[1]);
                return nullptr;
            }

            AttachAFunc(funcs_value_view_set_bit_offset, 2) {
                CXX::Interface::getExtractAs<value_viewer>(args[0], value_view).set_bit_offset((uint8_t)args[1]);
                return nullptr;
            }

            AttachAFunc(funcs_value_view_set_bit_used, 2) {
                CXX::Interface::getExtractAs<value_viewer>(args[0], value_view).set_bit_used((uint16_t)args[1]);
                return nullptr;
            }

            AttachAFunc(funcs_value_view_set_offset, 2) {
                CXX::Interface::getExtractAs<value_viewer>(args[0], value_view).set_offset((size_t)args[1]);
                return nullptr;
            }

            AttachAFunc(funcs_value_view_set_zero_after_cleanup, 2) {
                CXX::Interface::getExtractAs<value_viewer>(args[0], value_view).set_zero_after_cleanup((bool)args[1]);
                return nullptr;
            }

            AttachAFunc(funcs_value_view_add_tag, 4) {
                CXX::Interface::getExtractAs<value_viewer>(args[0], value_view).add_tag((art::ustring)args[1], std::move(args[2]), args[1].funPtr() ? *args[1].funPtr() : nullptr);
                return nullptr;
            }

            AttachAFunc(funcs_value_view_remove_tag, 2) {
                CXX::Interface::getExtractAs<value_viewer>(args[0], value_view).remove_tag((art::ustring)args[1]);
                return nullptr;
            }

            AttachAFunc(funcs_value_view_clear_tags, 1) {
                CXX::Interface::getExtractAs<value_viewer>(args[0], value_view).clear_tags();
                return nullptr;
            }
        }

        namespace tag_view_impl {
            AttachAFunc(funcs_tag_view_get_name, 1) {
                return CXX::Interface::getExtractAs<tag_viewer>(args[0], tag_view).get_name();
            }

            AttachAFunc(funcs_tag_view_get_value, 1) {
                return CXX::Interface::getExtractAs<tag_viewer>(args[0], tag_view).get_value();
            }

            AttachAFunc(funcs_tag_view_get_enviro, 1) {
                return CXX::Interface::getExtractAs<tag_viewer>(args[0], tag_view).get_enviro();
            }

            AttachAFunc(funcs_tag_view_set_value, 2) {
                CXX::Interface::getExtractAs<tag_viewer>(args[0], tag_view).set_value(std::move(CXX::Interface::getExtractAs<ValueItem>(args[1], value_view)));
                return nullptr;
            }

            AttachAFunc(funcs_tag_view_set_enviro, 2) {
                CXX::Interface::getExtractAs<tag_viewer>(args[0], tag_view).set_enviro(args[1].funPtr() ? *args[1].funPtr() : nullptr);
                return nullptr;
            }
        }

        namespace vtable_view_impl {
            AttachAFunc(funcs_vtable_view_get_destructor, 1) {
                return CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).get_destructor();
            }

            AttachAFunc(funcs_vtable_view_get_copy, 1) {
                return CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).get_copy();
            }

            AttachAFunc(funcs_vtable_view_get_move, 1) {
                return CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).get_move();
            }

            AttachAFunc(funcs_vtable_view_get_compare, 1) {
                return CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).get_compare();
            }

            AttachAFunc(funcs_vtable_view_get_constructor, 1) {
                return CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).get_constructor();
            }

            AttachAFunc(funcs_vtable_view_get_name, 1) {
                return CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).get_name();
            }

            AttachAFunc(funcs_vtable_view_get_structure_size, 1) {
                return CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).get_structure_size();
            }

            AttachAFunc(funcs_vtable_view_get_allow_auto_copy, 1) {
                return CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).get_allow_auto_copy();
            }

            AttachAFunc(funcs_vtable_view_get_methods, 1) {

                return CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).get_methods().convert<ValueItem>([](method_viewer&& method) {
                    return ValueItem(CXX::Interface::constructStructure<method_viewer>(method_view, method), no_copy);
                });
            }

            AttachAFunc(funcs_vtable_view_get_method_by_id, 2) {
                return ValueItem(CXX::Interface::constructStructure<method_viewer>(method_view, CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).get_method_by_id((uint64_t)args[1])), no_copy);
            }

            AttachAFunc(funcs_vtable_view_get_method_by_name, 3) {
                return ValueItem(CXX::Interface::constructStructure<method_viewer>(method_view, CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).get_method_by_name((art::ustring)args[1], (ClassAccess)(uint8_t)args[2])), no_copy);
            }

            AttachAFunc(funcs_vtable_view_get_method_id, 3) {
                return CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).get_method_id((art::ustring)args[1], (ClassAccess)(uint8_t)args[2]);
            }

            AttachAFunc(funcs_vtable_view_get_values, 1) {
                return CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).get_values().convert<ValueItem>([](value_viewer&& value) {
                    return ValueItem(CXX::Interface::constructStructure<value_viewer>(value_view, value), no_copy);
                });
            }

            AttachAFunc(funcs_vtable_view_get_value_by_id, 2) {
                return ValueItem(CXX::Interface::constructStructure<value_viewer>(value_view, CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).get_value_by_id((uint64_t)args[1])), no_copy);
            }

            AttachAFunc(funcs_vtable_view_get_value_by_name, 3) {
                return ValueItem(CXX::Interface::constructStructure<value_viewer>(value_view, CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).get_value_by_name((art::ustring)args[1], (ClassAccess)(uint8_t)args[2])), no_copy);
            }

            AttachAFunc(funcs_vtable_view_get_value_id, 3) {
                return CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).get_value_id((art::ustring)args[1], (ClassAccess)(uint8_t)args[2]);
            }

            AttachAFunc(funcs_vtable_view_get_statics, 1) {
                return CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).get_statics().convert<ValueItem>([](const static_viewer& tag) {
                    return ValueItem(CXX::Interface::constructStructure<static_viewer>(static_view, tag), no_copy);
                });
            }

            AttachAFunc(funcs_vtable_view_get_static_by_name, 3) {
                return ValueItem(CXX::Interface::constructStructure<static_viewer>(static_view, CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).get_static_by_name((art::ustring)args[1], (ClassAccess)(uint8_t)args[2])), no_copy);
            }

            AttachAFunc(funcs_vtable_view_get_tags, 1) {
                return CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).get_tags().convert<ValueItem>([](tag_viewer&& tag) {
                    return ValueItem(CXX::Interface::constructStructure<tag_viewer>(tag_view, tag), no_copy);
                });
            }

            AttachAFunc(funcs_vtable_view_get_tag_by_id, 2) {
                return ValueItem(CXX::Interface::constructStructure<tag_viewer>(tag_view, CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).get_tag_by_id((uint64_t)args[1])), no_copy);
            }

            AttachAFunc(funcs_vtable_view_get_tag_by_name, 2) {
                return ValueItem(CXX::Interface::constructStructure<tag_viewer>(tag_view, CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).get_tag_by_name((art::ustring)args[1])), no_copy);
            }

            AttachAFunc(funcs_vtable_view_get_tag_id, 2) {
                return CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).get_tag_id((art::ustring)args[1]);
            }

            AttachAFunc(funcs_vtable_view_get_mode, 1) {
                return (uint8_t)CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).mode;
            }

            AttachAFunc(funcs_vtable_view_set_destructor, 2) {
                CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).set_destructor(args[1].funPtr() ? *args[1].funPtr() : nullptr);
                return nullptr;
            }

            AttachAFunc(funcs_vtable_view_set_copy, 2) {
                CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).set_copy(args[1].funPtr() ? *args[1].funPtr() : nullptr);
                return nullptr;
            }

            AttachAFunc(funcs_vtable_view_set_move, 2) {
                CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).set_move(args[1].funPtr() ? *args[1].funPtr() : nullptr);
                return nullptr;
            }

            AttachAFunc(funcs_vtable_view_set_compare, 2) {
                CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).set_compare(args[1].funPtr() ? *args[1].funPtr() : nullptr);
                return nullptr;
            }

            AttachAFunc(funcs_vtable_view_set_constructor, 2) {
                CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).set_constructor(args[1].funPtr() ? *args[1].funPtr() : nullptr);
                return nullptr;
            }

            AttachAFunc(funcs_vtable_view_set_name, 2) {
                CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).set_name((art::ustring)args[1]);
                return nullptr;
            }

            AttachAFunc(funcs_vtable_view_set_structure_size, 2) {
                CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).set_structure_size((size_t)args[1]);
                return nullptr;
            }

            AttachAFunc(funcs_vtable_view_set_allow_auto_copy, 2) {
                CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).set_allow_auto_copy((bool)args[1]);
                return nullptr;
            }

            AttachAFunc(funcs_vtable_view_add_method, 7) {
                list_array<list_array<std::pair<ValueMeta, art::ustring>>> fun_args = ((list_array<ValueItem>)args[4]).convert_take<list_array<std::pair<ValueMeta, art::ustring>>>([](ValueItem&& inner) {
                    return ((list_array<ValueItem>)inner).convert<std::pair<ValueMeta, art::ustring>>([](ValueItem&& item) {
                        return std::pair<ValueMeta, art::ustring>(item[0], item[1]);
                    });
                });
                list_array<ValueMeta> results = ((list_array<ValueItem>)args[5]).convert_fn([](ValueItem&& item) {
                    return (ValueMeta)item;
                });

                CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view)
                    .add_method((art::ustring)args[1], (ClassAccess)(uint8_t)args[2], args[3].funPtr() ? *args[3].funPtr() : nullptr, std::move(fun_args), std::move(results), (art::ustring)args[6]);
                return nullptr;
            }

            AttachAFunc(funcs_vtable_view_remove_method, 3) {
                CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).remove_method((art::ustring)args[1], (ClassAccess)(uint8_t)args[2]);
                return nullptr;
            }

            AttachAFunc(funcs_vtable_view_rename_method, 4) {
                CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).rename_method((art::ustring)args[1], (art::ustring)args[2], (ClassAccess)(uint8_t)args[3]);
                return nullptr;
            }

            AttachAFunc(funcs_vtable_view_clear_methods, 1) {
                CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).clear_methods();
                return nullptr;
            }

            AttachAFunc(funcs_vtable_view_add_value, 8) {
                CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).add_value((art::ustring)args[1], (ClassAccess)(uint8_t)args[2], CXX::Interface::getExtractAs<ValueMeta>(args[3], value_view), (bool)args[4], (bool)args[5], (uint8_t)args[6], (uint16_t)args[7], (size_t)args[8], (bool)args[9]);
                return nullptr;
            }

            AttachAFunc(funcs_vtable_view_remove_value, 3) {
                CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).remove_value((art::ustring)args[1], (ClassAccess)(uint8_t)args[2]);
                return nullptr;
            }

            AttachAFunc(funcs_vtable_view_rename_value, 4) {
                CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).rename_value((art::ustring)args[1], (art::ustring)args[2], (ClassAccess)(uint8_t)args[3]);
                return nullptr;
            }

            AttachAFunc(funcs_vtable_view_clear_values, 1) {
                CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).clear_values();
                return nullptr;
            }

            AttachAFunc(funcs_vtable_view_add_static, 5) {
                CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).add_static((art::ustring)args[1], (ClassAccess)(uint8_t)args[2], std::move(CXX::Interface::getExtractAs<ValueItem>(args[3], value_view)));
                return nullptr;
            }

            AttachAFunc(funcs_vtable_view_remove_static, 3) {
                CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).remove_static((art::ustring)args[1], (ClassAccess)(uint8_t)args[2]);
                return nullptr;
            }

            AttachAFunc(funcs_vtable_view_rename_static, 4) {
                CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).rename_static((art::ustring)args[1], (art::ustring)args[2], (ClassAccess)(uint8_t)args[3]);
                return nullptr;
            }

            AttachAFunc(funcs_vtable_view_clear_statics, 1) {
                CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).clear_statics();
                return nullptr;
            }

            AttachAFunc(funcs_vtable_view_add_tag, 4) {
                CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).add_tag((art::ustring)args[1], std::move(CXX::Interface::getExtractAs<ValueItem>(args[2], value_view)), args[3].funPtr() ? *args[3].funPtr() : nullptr);
                return nullptr;
            }

            AttachAFunc(funcs_vtable_view_remove_tag, 2) {
                CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).remove_tag((art::ustring)args[1]);
                return nullptr;
            }

            AttachAFunc(funcs_vtable_view_rename_tag, 3) {
                CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).rename_tag((art::ustring)args[1], (art::ustring)args[2]);
                return nullptr;
            }

            AttachAFunc(funcs_vtable_view_clear_tags, 1) {
                CXX::Interface::getExtractAs<vtable_viewer>(args[0], vtable_view).clear_tags();
                return nullptr;
            }
        }

        AttachAFunc(view_structure, 1) {
            Structure& struct_ = (Structure&)args[0];
            return CXX::Interface::constructStructure<vtable_viewer>(vtable_view, vtable_viewer(struct_.vtable, struct_.vtable_mode));
        }

        vtable_viewer view_structure(Structure& structure_) {
            return vtable_viewer(structure_.vtable, structure_.vtable_mode);
        }

        void init_vtable_views() {
            method_view = CXX::Interface::createTable<method_viewer>(
                "method_viewer",
                CXX::Interface::direct_method("get_name", method_view_impl::funcs_method_view_get_name),
                CXX::Interface::direct_method("get_access", method_view_impl::funcs_method_view_get_access),
                CXX::Interface::direct_method("is_deletable", method_view_impl::funcs_method_view_is_deletable),
                CXX::Interface::direct_method("get_function", method_view_impl::funcs_method_view_get_function),
                CXX::Interface::direct_method("get_owner_name", method_view_impl::funcs_method_view_get_owner_name),
                CXX::Interface::direct_method("get_tags", method_view_impl::funcs_method_view_get_tags),
                CXX::Interface::direct_method("get_args", method_view_impl::funcs_method_view_get_args),
                CXX::Interface::direct_method("get_return_values", method_view_impl::funcs_method_view_get_return_values)
            );
            CXX::Interface::typeVTable<method_viewer>() = method_view;
            attacha_environment::get_type({"internal", "method_viewer"})->value = method_view;


            static_view = CXX::Interface::createTable<static_viewer>(
                "static_viewer",
                CXX::Interface::direct_method("get_name", static_view_impl::funcs_static_view_get_name),
                CXX::Interface::direct_method("get_access", static_view_impl::funcs_static_view_get_access),
                CXX::Interface::direct_method("get_tags", static_view_impl::funcs_static_view_get_tags),
                CXX::Interface::direct_method("copy_value", static_view_impl::funcs_static_view_copy_value)
            );
            CXX::Interface::typeVTable<static_viewer>() = static_view;
            attacha_environment::get_type({"internal", "static_viewer"})->value = static_view;

            value_view = CXX::Interface::createTable<value_viewer>(
                "value_viewer",
                CXX::Interface::direct_method("get_name", value_view_impl::funcs_value_view_get_name),
                CXX::Interface::direct_method("get_access", value_view_impl::funcs_value_view_get_access),
                CXX::Interface::direct_method("get_type", value_view_impl::funcs_value_view_get_type),
                CXX::Interface::direct_method("is_allow_abstract_assign", value_view_impl::funcs_value_view_is_allow_abstract_assign),
                CXX::Interface::direct_method("get_bit_offset", value_view_impl::funcs_value_view_get_bit_offset),
                CXX::Interface::direct_method("get_bit_used", value_view_impl::funcs_value_view_get_bit_used),
                CXX::Interface::direct_method("is_inlined", value_view_impl::funcs_value_view_is_inlined),
                CXX::Interface::direct_method("get_offset", value_view_impl::funcs_value_view_get_offset),
                CXX::Interface::direct_method("get_zero_after_cleanup", value_view_impl::funcs_value_view_get_zero_after_cleanup),
                CXX::Interface::direct_method("get_tags", value_view_impl::funcs_value_view_get_tags)
            );
            CXX::Interface::typeVTable<value_viewer>() = value_view;
            attacha_environment::get_type({"internal", "value_viewer"})->value = value_view;

            tag_view = CXX::Interface::createTable<tag_viewer>(
                "tag_viewer",
                CXX::Interface::direct_method("get_name", tag_view_impl::funcs_tag_view_get_name),
                CXX::Interface::direct_method("get_value", tag_view_impl::funcs_tag_view_get_value),
                CXX::Interface::direct_method("get_enviro", tag_view_impl::funcs_tag_view_get_enviro)
            );
            CXX::Interface::typeVTable<tag_viewer>() = tag_view;
            attacha_environment::get_type({"internal", "tag_viewer"})->value = tag_view;

            vtable_view = CXX::Interface::createTable<vtable_viewer>(
                "vtable_viewer",
                CXX::Interface::direct_method("get_destructor", vtable_view_impl::funcs_vtable_view_get_destructor),
                CXX::Interface::direct_method("get_copy", vtable_view_impl::funcs_vtable_view_get_copy),
                CXX::Interface::direct_method("get_move", vtable_view_impl::funcs_vtable_view_get_move),
                CXX::Interface::direct_method("get_compare", vtable_view_impl::funcs_vtable_view_get_compare),
                CXX::Interface::direct_method("get_constructor", vtable_view_impl::funcs_vtable_view_get_constructor),
                CXX::Interface::direct_method("get_name", vtable_view_impl::funcs_vtable_view_get_name),
                CXX::Interface::direct_method("get_structure_size", vtable_view_impl::funcs_vtable_view_get_structure_size),
                CXX::Interface::direct_method("get_allow_auto_copy", vtable_view_impl::funcs_vtable_view_get_allow_auto_copy),
                CXX::Interface::direct_method("get_methods", vtable_view_impl::funcs_vtable_view_get_methods),
                CXX::Interface::direct_method("get_method_by_id", vtable_view_impl::funcs_vtable_view_get_method_by_id),
                CXX::Interface::direct_method("get_method_by_name", vtable_view_impl::funcs_vtable_view_get_method_by_name),
                CXX::Interface::direct_method("get_method_id", vtable_view_impl::funcs_vtable_view_get_method_id),
                CXX::Interface::direct_method("get_values", vtable_view_impl::funcs_vtable_view_get_values),
                CXX::Interface::direct_method("get_value_by_id", vtable_view_impl::funcs_vtable_view_get_value_by_id),
                CXX::Interface::direct_method("get_value_by_name", vtable_view_impl::funcs_vtable_view_get_value_by_name),
                CXX::Interface::direct_method("get_value_id", vtable_view_impl::funcs_vtable_view_get_value_id),
                CXX::Interface::direct_method("get_statics", vtable_view_impl::funcs_vtable_view_get_statics),
                CXX::Interface::direct_method("get_static_by_name", vtable_view_impl::funcs_vtable_view_get_static_by_name),
                CXX::Interface::direct_method("get_tags", vtable_view_impl::funcs_vtable_view_get_tags),
                CXX::Interface::direct_method("get_tag_by_id", vtable_view_impl::funcs_vtable_view_get_tag_by_id),
                CXX::Interface::direct_method("get_tag_by_name", vtable_view_impl::funcs_vtable_view_get_tag_by_name),
                CXX::Interface::direct_method("get_tag_id", vtable_view_impl::funcs_vtable_view_get_tag_id),
                CXX::Interface::direct_method("get_mode", vtable_view_impl::funcs_vtable_view_get_mode)
            );
            CXX::Interface::typeVTable<vtable_viewer>() = vtable_view;
            vtable_view->getAfterMethods()->constructor = new FuncEnvironment(view_structure);
            attacha_environment::get_type({"internal", "vtable_viewer"})->value = vtable_view;
        }

        void init_vtable_views_full_mode() {
            method_view = CXX::Interface::createTable<method_viewer>(
                "method_viewer",
                CXX::Interface::direct_method("get_name", method_view_impl::funcs_method_view_get_name),
                CXX::Interface::direct_method("get_access", method_view_impl::funcs_method_view_get_access),
                CXX::Interface::direct_method("is_deletable", method_view_impl::funcs_method_view_is_deletable),
                CXX::Interface::direct_method("get_function", method_view_impl::funcs_method_view_get_function),
                CXX::Interface::direct_method("get_owner_name", method_view_impl::funcs_method_view_get_owner_name),
                CXX::Interface::direct_method("get_tags", method_view_impl::funcs_method_view_get_tags),
                CXX::Interface::direct_method("get_args", method_view_impl::funcs_method_view_get_args),
                CXX::Interface::direct_method("get_return_values", method_view_impl::funcs_method_view_get_return_values),
                CXX::Interface::direct_method("set_access", method_view_impl::funcs_method_view_set_access),
                CXX::Interface::direct_method("add_tag", method_view_impl::funcs_method_view_add_tag),
                CXX::Interface::direct_method("remove_tag", method_view_impl::funcs_method_view_remove_tag),
                CXX::Interface::direct_method("clear_tags", method_view_impl::funcs_method_view_clear_tags)
            );
            CXX::Interface::typeVTable<method_viewer>() = method_view;
            attacha_environment::get_type({"internal", "method_viewer"})->value = method_view;

            static_view = CXX::Interface::createTable<static_viewer>(
                "static_viewer",
                CXX::Interface::direct_method("get_name", static_view_impl::funcs_static_view_get_name),
                CXX::Interface::direct_method("get_access", static_view_impl::funcs_static_view_get_access),
                CXX::Interface::direct_method("get_tags", static_view_impl::funcs_static_view_get_tags),
                CXX::Interface::direct_method("copy_value", static_view_impl::funcs_static_view_copy_value),
                CXX::Interface::direct_method("set_access", static_view_impl::funcs_static_view_set_access),
                CXX::Interface::direct_method("add_tag", static_view_impl::funcs_static_view_add_tag),
                CXX::Interface::direct_method("remove_tag", static_view_impl::funcs_static_view_remove_tag),
                CXX::Interface::direct_method("clear_tags", static_view_impl::funcs_static_view_clear_tags)
            );
            CXX::Interface::typeVTable<static_viewer>() = static_view;
            attacha_environment::get_type({"internal", "static_viewer"})->value = static_view;

            value_view = CXX::Interface::createTable<value_viewer>(
                "value_viewer",
                CXX::Interface::direct_method("get_name", value_view_impl::funcs_value_view_get_name),
                CXX::Interface::direct_method("get_access", value_view_impl::funcs_value_view_get_access),
                CXX::Interface::direct_method("get_type", value_view_impl::funcs_value_view_get_type),
                CXX::Interface::direct_method("is_allow_abstract_assign", value_view_impl::funcs_value_view_is_allow_abstract_assign),
                CXX::Interface::direct_method("get_bit_offset", value_view_impl::funcs_value_view_get_bit_offset),
                CXX::Interface::direct_method("get_bit_used", value_view_impl::funcs_value_view_get_bit_used),
                CXX::Interface::direct_method("is_inlined", value_view_impl::funcs_value_view_is_inlined),
                CXX::Interface::direct_method("get_offset", value_view_impl::funcs_value_view_get_offset),
                CXX::Interface::direct_method("get_zero_after_cleanup", value_view_impl::funcs_value_view_get_zero_after_cleanup),
                CXX::Interface::direct_method("get_tags", value_view_impl::funcs_value_view_get_tags),
                CXX::Interface::direct_method("set_access", value_view_impl::funcs_value_view_set_access),
                CXX::Interface::direct_method("set_type", value_view_impl::funcs_value_view_set_type),
                CXX::Interface::direct_method("set_allow_abstract_assign", value_view_impl::funcs_value_view_set_allow_abstract_assign),
                CXX::Interface::direct_method("set_inlined", value_view_impl::funcs_value_view_set_inlined),
                CXX::Interface::direct_method("set_bit_offset", value_view_impl::funcs_value_view_set_bit_offset),
                CXX::Interface::direct_method("set_bit_used", value_view_impl::funcs_value_view_set_bit_used),
                CXX::Interface::direct_method("set_offset", value_view_impl::funcs_value_view_set_offset),
                CXX::Interface::direct_method("set_zero_after_cleanup", value_view_impl::funcs_value_view_set_zero_after_cleanup),
                CXX::Interface::direct_method("add_tag", value_view_impl::funcs_value_view_add_tag),
                CXX::Interface::direct_method("remove_tag", value_view_impl::funcs_value_view_remove_tag),
                CXX::Interface::direct_method("clear_tags", value_view_impl::funcs_value_view_clear_tags)
            );
            CXX::Interface::typeVTable<value_viewer>() = value_view;
            attacha_environment::get_type({"internal", "value_viewer"})->value = value_view;

            tag_view = CXX::Interface::createTable<tag_viewer>(
                "tag_viewer",
                CXX::Interface::direct_method("get_name", tag_view_impl::funcs_tag_view_get_name),
                CXX::Interface::direct_method("get_value", tag_view_impl::funcs_tag_view_get_value),
                CXX::Interface::direct_method("get_enviro", tag_view_impl::funcs_tag_view_get_enviro),
                CXX::Interface::direct_method("set_value", tag_view_impl::funcs_tag_view_set_value),
                CXX::Interface::direct_method("set_enviro", tag_view_impl::funcs_tag_view_set_enviro)
            );
            CXX::Interface::typeVTable<tag_viewer>() = tag_view;
            attacha_environment::get_type({"internal", "tag_viewer"})->value = tag_view;

            vtable_view = CXX::Interface::createTable<vtable_viewer>(
                "vtable_viewer",
                CXX::Interface::direct_method("get_destructor", vtable_view_impl::funcs_vtable_view_get_destructor),
                CXX::Interface::direct_method("get_copy", vtable_view_impl::funcs_vtable_view_get_copy),
                CXX::Interface::direct_method("get_move", vtable_view_impl::funcs_vtable_view_get_move),
                CXX::Interface::direct_method("get_compare", vtable_view_impl::funcs_vtable_view_get_compare),
                CXX::Interface::direct_method("get_constructor", vtable_view_impl::funcs_vtable_view_get_constructor),
                CXX::Interface::direct_method("get_name", vtable_view_impl::funcs_vtable_view_get_name),
                CXX::Interface::direct_method("get_structure_size", vtable_view_impl::funcs_vtable_view_get_structure_size),
                CXX::Interface::direct_method("get_allow_auto_copy", vtable_view_impl::funcs_vtable_view_get_allow_auto_copy),
                CXX::Interface::direct_method("get_methods", vtable_view_impl::funcs_vtable_view_get_methods),
                CXX::Interface::direct_method("get_method_by_id", vtable_view_impl::funcs_vtable_view_get_method_by_id),
                CXX::Interface::direct_method("get_method_by_name", vtable_view_impl::funcs_vtable_view_get_method_by_name),
                CXX::Interface::direct_method("get_method_id", vtable_view_impl::funcs_vtable_view_get_method_id),
                CXX::Interface::direct_method("get_values", vtable_view_impl::funcs_vtable_view_get_values),
                CXX::Interface::direct_method("get_value_by_id", vtable_view_impl::funcs_vtable_view_get_value_by_id),
                CXX::Interface::direct_method("get_value_by_name", vtable_view_impl::funcs_vtable_view_get_value_by_name),
                CXX::Interface::direct_method("get_value_id", vtable_view_impl::funcs_vtable_view_get_value_id),
                CXX::Interface::direct_method("get_statics", vtable_view_impl::funcs_vtable_view_get_statics),
                CXX::Interface::direct_method("get_static_by_name", vtable_view_impl::funcs_vtable_view_get_static_by_name),
                CXX::Interface::direct_method("get_tags", vtable_view_impl::funcs_vtable_view_get_tags),
                CXX::Interface::direct_method("get_tag_by_id", vtable_view_impl::funcs_vtable_view_get_tag_by_id),
                CXX::Interface::direct_method("get_tag_by_name", vtable_view_impl::funcs_vtable_view_get_tag_by_name),
                CXX::Interface::direct_method("get_tag_id", vtable_view_impl::funcs_vtable_view_get_tag_id),
                CXX::Interface::direct_method("get_mode", vtable_view_impl::funcs_vtable_view_get_mode),
                CXX::Interface::direct_method("set_destructor", vtable_view_impl::funcs_vtable_view_set_destructor),
                CXX::Interface::direct_method("set_copy", vtable_view_impl::funcs_vtable_view_set_copy),
                CXX::Interface::direct_method("set_move", vtable_view_impl::funcs_vtable_view_set_move),
                CXX::Interface::direct_method("set_compare", vtable_view_impl::funcs_vtable_view_set_compare),
                CXX::Interface::direct_method("set_constructor", vtable_view_impl::funcs_vtable_view_set_constructor),
                CXX::Interface::direct_method("set_name", vtable_view_impl::funcs_vtable_view_set_name),
                CXX::Interface::direct_method("set_structure_size", vtable_view_impl::funcs_vtable_view_set_structure_size),
                CXX::Interface::direct_method("set_allow_auto_copy", vtable_view_impl::funcs_vtable_view_set_allow_auto_copy),
                CXX::Interface::direct_method("add_method", vtable_view_impl::funcs_vtable_view_add_method),
                CXX::Interface::direct_method("remove_method", vtable_view_impl::funcs_vtable_view_remove_method),
                CXX::Interface::direct_method("rename_method", vtable_view_impl::funcs_vtable_view_rename_method),
                CXX::Interface::direct_method("clear_methods", vtable_view_impl::funcs_vtable_view_clear_methods),
                CXX::Interface::direct_method("add_value", vtable_view_impl::funcs_vtable_view_add_value),
                CXX::Interface::direct_method("remove_value", vtable_view_impl::funcs_vtable_view_remove_value),
                CXX::Interface::direct_method("rename_value", vtable_view_impl::funcs_vtable_view_rename_value),
                CXX::Interface::direct_method("clear_values", vtable_view_impl::funcs_vtable_view_clear_values),
                CXX::Interface::direct_method("add_static", vtable_view_impl::funcs_vtable_view_add_static),
                CXX::Interface::direct_method("remove_static", vtable_view_impl::funcs_vtable_view_remove_static),
                CXX::Interface::direct_method("rename_static", vtable_view_impl::funcs_vtable_view_rename_static),
                CXX::Interface::direct_method("clear_statics", vtable_view_impl::funcs_vtable_view_clear_statics),
                CXX::Interface::direct_method("add_tag", vtable_view_impl::funcs_vtable_view_add_tag),
                CXX::Interface::direct_method("remove_tag", vtable_view_impl::funcs_vtable_view_remove_tag),
                CXX::Interface::direct_method("rename_tag", vtable_view_impl::funcs_vtable_view_rename_tag),
                CXX::Interface::direct_method("clear_tags", vtable_view_impl::funcs_vtable_view_clear_tags)
            );
            CXX::Interface::typeVTable<vtable_viewer>() = vtable_view;
            vtable_view->getAfterMethods()->constructor = new FuncEnvironment(view_structure);
            attacha_environment::get_type({"internal", "vtable_viewer"})->value = vtable_view;
        }
    }

#pragma endregion

    namespace internal {
        namespace memory {
            //returns faarr[faarr[ptr from, ptr to, len, str desk, bool is_fault]...], arg: array/value ptr
            ValueItem* dump(ValueItem* vals, uint32_t len) {
                if (len == 1)
                    return light_stack::dump(vals[0].getSourcePtr());
                throw InvalidArguments("This function requires 1 argument");
            }
        }

        namespace stack {
            //reduce stack size, returns bool, args: shrink threshold(optional)
            ValueItem* shrink(ValueItem* vals, uint32_t len) {
                if (len == 1)
                    return new ValueItem(light_stack::shrink_current((size_t)vals[0]));
                else if (len == 0)
                    return new ValueItem(light_stack::shrink_current());
                throw InvalidArguments("This function requires 0 or 1 argument");
            }

            //grow stack size, returns bool, args: grow count
            ValueItem* prepare(ValueItem* vals, uint32_t len) {
                if (len == 1)
                    return new ValueItem(light_stack::prepare((size_t)vals[0]));
                else if (len == 0)
                    return new ValueItem(light_stack::prepare());
                throw InvalidArguments("This function requires 0 or 1 argument");
            }

            //make sure stack size is enough and increase if too small, returns bool, args: grow count
            ValueItem* reserve(ValueItem* vals, uint32_t len) {
                if (len == 1)
                    return new ValueItem(light_stack::prepare((size_t)vals[0]));
                throw InvalidArguments("This function requires 1 argument");
            }

            //returns faarr[faarr[ptr from, ptr to, str desk, bool is_fault]...], args: none
            ValueItem* dump(ValueItem* vals, uint32_t len) {
                if (len == 0)
                    return light_stack::dump_current();
                throw InvalidArguments("This function requires 0 argument");
            }

            ValueItem* bs_supported(ValueItem* vals, uint32_t len) {
                return new ValueItem(light_stack::is_supported());
            }

            ValueItem* used_size(ValueItem*, uint32_t) {
                return new ValueItem(light_stack::used_size());
            }

            ValueItem* unused_size(ValueItem*, uint32_t) {
                return new ValueItem(light_stack::unused_size());
            }

            ValueItem* allocated_size(ValueItem*, uint32_t) {
                return new ValueItem(light_stack::allocated_size());
            }

            ValueItem* free_size(ValueItem*, uint32_t) {
                return new ValueItem(light_stack::free_size());
            }

            ValueItem* trace(ValueItem* vals, uint32_t len) {
                uint32_t framesToSkip = 0;
                bool include_native = true;
                uint32_t max_frames = 32;
                if (len >= 1)
                    framesToSkip = (uint32_t)vals[0];
                if (len >= 2)
                    include_native = (bool)vals[1];
                if (len >= 3)
                    max_frames = (uint32_t)vals[2];

                auto res = FrameResult::JitCaptureStackTrace(framesToSkip, include_native, max_frames);
                auto res2 = new ValueItem[res.size()];
                for (size_t i = 0; i < res.size(); i++)
                    res2[i] = ValueItem{res[i].file_path, res[i].fn_name, (uint64_t)res[i].line, res[i].column};
                return new ValueItem(res2, ValueMeta(VType::faarr, false, true, res.size()));
            }

            ValueItem* clean_trace(ValueItem* vals, uint32_t len) {
                uint32_t framesToSkip = 0;
                bool include_native = true;
                uint32_t max_frames = 32;
                if (len >= 1)
                    framesToSkip = (uint32_t)vals[0];
                if (len >= 2)
                    include_native = (bool)vals[1];
                if (len >= 3)
                    max_frames = (uint32_t)vals[2];

                auto res = FrameResult::JitCaptureStackTrace(framesToSkip, include_native, max_frames);
                list_array<StackTraceItem> res2;
                for (size_t i = 0; i < res.size(); i++) {
                    if (res[i].fn_name == "art::internal::stack::clean_trace")
                        continue;
                    if (res[i].fn_name.starts_with("std::invoke"))
                        continue;
                    if (res[i].fn_name.starts_with("std::apply"))
                        continue;
                    if (res[i].fn_name.starts_with("std::_Apply_impl"))
                        continue;
                    if (res[i].fn_name.starts_with("art::CXX::Proxy"))
                        continue;
                    if (res[i].fn_name.starts_with("art::FuncHandle::"))
                        continue;
                    if (res[i].fn_name.starts_with("art::FuncEnvironment::"))
                        continue;
                    if (res[i].fn_name == "art::CXX::cxxCall")
                        continue;
                    if (res[i].fn_name == "art::CXX::aCall")
                        continue;
                    if (res[i].fn_name == "invoke_main")
                        break;
                    if (res[i].fn_name == "art::context_exec")
                        break;
                    res2.push_back(std::move(res[i]));
                }

                auto res3 = new ValueItem[res2.size()];
                for (size_t i = 0; i < res2.size(); i++)
                    res3[i] = ValueItem{res2[i].file_path, res2[i].fn_name, res2[i].line, res2[i].column};
                return new ValueItem(res3, ValueMeta(VType::faarr, false, true, res2.size()));
            }

            ValueItem* trace_frames(ValueItem* vals, uint32_t len) {
                uint32_t framesToSkip = 0;
                bool include_native = true;
                uint32_t max_frames = 32;
                if (len >= 1)
                    framesToSkip = (uint32_t)vals[0];
                if (len >= 2)
                    include_native = (bool)vals[1];
                if (len >= 3)
                    max_frames = (uint32_t)vals[2];
                auto res = FrameResult::JitCaptureStackChainTrace(framesToSkip, include_native, max_frames);
                auto res2 = new ValueItem[res.size()];
                for (size_t i = 0; i < res.size(); i++)
                    res2[i] = res[i];
                return new ValueItem(res2, ValueMeta(VType::faarr, false, true, res.size()));
            }

            ValueItem* resolve_frame(ValueItem* vals, uint32_t len) {
                StackTraceItem res;
                if (len == 1)
                    res = FrameResult::JitResolveFrame((void*)vals[0]);
                else if (len == 2)
                    res = FrameResult::JitResolveFrame((void*)vals[0], (bool)vals[1]);
                else
                    throw InvalidArguments("This function requires 1 argument and second optional: (rip, include_native)");
                return new ValueItem{res.file_path, res.fn_name, res.line, res.column};
            }
        }

        namespace run_time {
            ValueItem* gc_pause(ValueItem*, uint32_t) {
                //lgr not support pause
                return nullptr;
            }

            ValueItem* gc_resume(ValueItem*, uint32_t) {
                //lgr not support resume
                return nullptr;
            }

            //gc can ignore this hint
            ValueItem* gc_hint_collect(ValueItem*, uint32_t) {
                //lgr is determined, not need to hint
                return nullptr;
            }

            namespace native {
                AttachAVirtualTable* define_NativeLib;
                AttachAVirtualTable* define_NativeTemplate;
                AttachAVirtualTable* define_NativeValue;

                namespace constructor {
                    ValueItem* createProxy_NativeValue(ValueItem*, uint32_t) {
                        return new ValueItem(CXX::Interface::constructStructure<typed_lgr<DynamicCall::FunctionTemplate::ValueT>>(define_NativeValue, new DynamicCall::FunctionTemplate::ValueT()), no_copy);
                    }

                    ValueItem* createProxy_NativeTemplate(ValueItem*, uint32_t) {
                        return new ValueItem(CXX::Interface::constructStructure<typed_lgr<DynamicCall::FunctionTemplate>>(define_NativeTemplate, new DynamicCall::FunctionTemplate()), no_copy);
                    }

                    AttachAFunc(createProxy_NativeLib, 1) {
                        return ValueItem(CXX::Interface::constructStructure<typed_lgr<NativeLib>>(define_NativeLib, new NativeLib(((art::ustring)args[0]).c_str())), no_copy);
                    }
                }

                AttachAFunc(funs_NativeLib_get_function, 3) {
                    auto& class_ = CXX::Interface::getExtractAs<typed_lgr<NativeLib>>(args[0], define_NativeLib);
                    auto fun_name = (art::ustring)args[1];
                    auto& template_ = *CXX::Interface::getExtractAs<typed_lgr<DynamicCall::FunctionTemplate>>(args[2], define_NativeTemplate);
                    return ValueItem(class_->get_func_enviro(fun_name, template_));
                }

                AttachAFunc(funs_NativeLib_get_own_function, 2) {
                    auto& class_ = CXX::Interface::getExtractAs<typed_lgr<NativeLib>>(args[0], define_NativeLib);
                    auto fun_name = (art::ustring)args[1];
                    return ValueItem(class_->get_own_enviro(fun_name));
                }

                AttachAFunc(funs_NativeTemplate_add_argument, 3) {
                    auto& class_ = CXX::Interface::getExtractAs<typed_lgr<DynamicCall::FunctionTemplate>>(args[0], define_NativeTemplate);
                    auto& value = *CXX::Interface::getExtractAs<typed_lgr<DynamicCall::FunctionTemplate::ValueT>>(args[1], define_NativeValue);
                    class_->arguments.push_back(value);
                    return nullptr;
                }

#define funs_setter(name, class, set_typ, extract_typ)                                                 \
    AttachAFunc(funs_NativeTemplate_setter_##name, 2) {                                                \
        auto& class_ = CXX::Interface::getExtractAs<typed_lgr<class>>(args[0], define_NativeTemplate); \
        class_->name = set_typ((extract_typ)args[1]);                                                  \
        return nullptr;                                                                                \
    }
#define funs_getter(name, class, middle_cast)                                                          \
    AttachAFunc(funs_NativeTemplate_getter_##name, 1) {                                                \
        auto& class_ = CXX::Interface::getExtractAs<typed_lgr<class>>(args[0], define_NativeTemplate); \
        return ValueItem((middle_cast)class_->name);                                                   \
    }

                DynamicCall::FunctionTemplate::ValueT castValueT(uint64_t val) {
                    return *(DynamicCall::FunctionTemplate::ValueT*)&val;
                }

                funs_setter(result, DynamicCall::FunctionTemplate, castValueT, uint64_t);

                AttachAFunc(funs_NativeTemplate_getter_result, 1) {
                    auto& class_ = CXX::Interface::getExtractAs<typed_lgr<DynamicCall::FunctionTemplate>>(args[0], define_NativeTemplate);
                    return ValueItem(*(uint64_t*)&class_->result);
                }

                funs_setter(is_variadic, DynamicCall::FunctionTemplate, bool, bool);
                funs_getter(is_variadic, DynamicCall::FunctionTemplate, bool);


                funs_setter(is_modifiable, DynamicCall::FunctionTemplate::ValueT, bool, bool);
                funs_getter(is_modifiable, DynamicCall::FunctionTemplate::ValueT, bool);
                funs_setter(vsize, DynamicCall::FunctionTemplate::ValueT, uint32_t, uint32_t);
                funs_getter(vsize, DynamicCall::FunctionTemplate::ValueT, uint32_t);
                funs_setter(ptype, DynamicCall::FunctionTemplate::ValueT, DynamicCall::FunctionTemplate::ValueT::PlaceType, uint8_t);
                funs_getter(ptype, DynamicCall::FunctionTemplate::ValueT, uint8_t);
                funs_setter(vtype, DynamicCall::FunctionTemplate::ValueT, DynamicCall::FunctionTemplate::ValueT::ValueType, uint8_t);
                funs_getter(vtype, DynamicCall::FunctionTemplate::ValueT, uint8_t);

                void init() {
                    static bool is_init = false;
                    if (is_init)
                        return;
                    define_NativeLib = CXX::Interface::createTable<typed_lgr<NativeLib>>(
                        "native_lib",
                        CXX::Interface::direct_method("get_function", funs_NativeLib_get_function),
                        CXX::Interface::direct_method("get_own_function", funs_NativeLib_get_own_function)
                    );
                    define_NativeTemplate = CXX::Interface::createTable<typed_lgr<DynamicCall::FunctionTemplate>>(
                        "native_template",
                        CXX::Interface::direct_method("add_argument", funs_NativeTemplate_add_argument),
                        CXX::Interface::direct_method("set_return_type", funs_NativeTemplate_setter_result),
                        CXX::Interface::direct_method("get_return_type", funs_NativeTemplate_getter_result),
                        CXX::Interface::direct_method("set_is_variadic", funs_NativeTemplate_setter_is_variadic),
                        CXX::Interface::direct_method("get_is_variadic", funs_NativeTemplate_getter_is_variadic)
                    );
                    define_NativeValue = CXX::Interface::createTable<typed_lgr<DynamicCall::FunctionTemplate::ValueT>>(
                        "native_value",
                        CXX::Interface::direct_method("set_is_modifiable", funs_NativeTemplate_setter_is_modifiable),
                        CXX::Interface::direct_method("get_is_modifiable", funs_NativeTemplate_getter_is_modifiable),
                        CXX::Interface::direct_method("set_vsize", funs_NativeTemplate_setter_vsize),
                        CXX::Interface::direct_method("get_vsize", funs_NativeTemplate_getter_vsize),
                        CXX::Interface::direct_method("set_place_type", funs_NativeTemplate_setter_ptype),
                        CXX::Interface::direct_method("get_place_type", funs_NativeTemplate_getter_ptype),
                        CXX::Interface::direct_method("set_value_type", funs_NativeTemplate_setter_vtype),
                        CXX::Interface::direct_method("get_value_type", funs_NativeTemplate_getter_vtype)
                    );
                    CXX::Interface::typeVTable<typed_lgr<NativeLib>>() = define_NativeLib;
                    CXX::Interface::typeVTable<typed_lgr<DynamicCall::FunctionTemplate>>() = define_NativeTemplate;
                    CXX::Interface::typeVTable<typed_lgr<DynamicCall::FunctionTemplate::ValueT>>() = define_NativeValue;
                    define_NativeLib->getAfterMethods()->constructor = new FuncEnvironment(constructor::createProxy_NativeLib);
                    define_NativeTemplate->getAfterMethods()->constructor = new FuncEnvironment(constructor::createProxy_NativeTemplate);
                    define_NativeValue->getAfterMethods()->constructor = new FuncEnvironment(constructor::createProxy_NativeValue);
                    attacha_environment::get_type({"internal", "run_time", "native", "native_lib"})->value = define_NativeLib;
                    attacha_environment::get_type({"internal", "run_time", "native", "native_template"})->value = define_NativeTemplate;
                    attacha_environment::get_type({"internal", "run_time", "native", "native_value"})->value = define_NativeValue;
                    is_init = true;
                }
            }

            void must_init() {
                static bool is_init = false;
                if (is_init)
                    return;
                is_init = true;
                attacha_environment::get_value({"run_time", "event", "unhandled_exception"}) = ValueItem(CXX::Interface::constructStructure<typed_lgr<EventSystem>>((AttachAVirtualTable*)CXX::Interface::typeVTableReadOnly<typed_lgr<EventSystem>>(), new EventSystem()), no_copy);
                attacha_environment::get_value({"run_time", "event", "ex_fault"}) = ValueItem(CXX::Interface::constructStructure<typed_lgr<EventSystem>>((AttachAVirtualTable*)CXX::Interface::typeVTableReadOnly<typed_lgr<EventSystem>>(), new EventSystem()), no_copy);
                attacha_environment::get_value({"run_time", "event", "error"}) = ValueItem(CXX::Interface::constructStructure<typed_lgr<EventSystem>>((AttachAVirtualTable*)CXX::Interface::typeVTableReadOnly<typed_lgr<EventSystem>>(), new EventSystem()), no_copy);
                attacha_environment::get_value({"run_time", "event", "warning"}) = ValueItem(CXX::Interface::constructStructure<typed_lgr<EventSystem>>((AttachAVirtualTable*)CXX::Interface::typeVTableReadOnly<typed_lgr<EventSystem>>(), new EventSystem()), no_copy);
                attacha_environment::get_value({"run_time", "event", "info"}) = ValueItem(CXX::Interface::constructStructure<typed_lgr<EventSystem>>((AttachAVirtualTable*)CXX::Interface::typeVTableReadOnly<typed_lgr<EventSystem>>(), new EventSystem()), no_copy);
            }
        }

        namespace constructor {
            AttachAVirtualTable* define_FuncBuilder;
            AttachAVirtualTable* define_ValueIndexPos;
            AttachAVirtualTable* define_FuncEnviroBuilder_line_info;

            ValueItem makeVIP(ValueIndexPos vpos) {
                return ValueItem(CXX::Interface::constructStructure<ValueIndexPos>(define_ValueIndexPos, vpos));
            }

            ValueItem makeLineInfo(art::line_info lf, uint64_t line, uint64_t column) {
                return ValueItem(CXX::Interface::constructStructure<art::line_info>(define_FuncEnviroBuilder_line_info, lf));
            }

            ValueIndexPos getVIP(ValueItem vip) {
                return CXX::Interface::getExtractAs<ValueIndexPos>(vip, define_ValueIndexPos);
            }

            art::line_info getLineInfo(ValueItem lf) {
                return CXX::Interface::getExtractAs<art::line_info>(lf, define_FuncEnviroBuilder_line_info);
            }

            AttachAFunc(funs_FuncBuilder_create_constant, 2) {
                return makeVIP(CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->create_constant(args[1]));
            }

            AttachAFunc(funs_FuncBuilder_set_stack_any_array, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->set_stack_any_array(getVIP(args[1]), (uint32_t)args[2]);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_remove, 2) {
                if (len > 2)
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->remove(getVIP(args[1]), (ValueMeta)args[2]);
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->remove(getVIP(args[1]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_sum, 3) {
                if (len > 4)
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->sum(getVIP(args[1]), getVIP(args[2]), (ValueMeta)args[3], (ValueMeta)args[4]);
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->sum(getVIP(args[1]), getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_minus, 3) {
                if (len > 4)
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->minus(getVIP(args[1]), getVIP(args[2]), (ValueMeta)args[3], (ValueMeta)args[4]);
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->minus(getVIP(args[1]), getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_div, 3) {
                if (len > 4)
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->div(getVIP(args[1]), getVIP(args[2]), (ValueMeta)args[3], (ValueMeta)args[4]);
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->div(getVIP(args[1]), getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_mul, 3) {
                if (len > 4)
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->mul(getVIP(args[1]), getVIP(args[2]), (ValueMeta)args[3], (ValueMeta)args[4]);
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->mul(getVIP(args[1]), getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_rest, 3) {
                if (len > 4)
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->rest(getVIP(args[1]), getVIP(args[2]), (ValueMeta)args[3], (ValueMeta)args[4]);
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->rest(getVIP(args[1]), getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_bit_xor, 3) {
                if (len > 4)
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->bit_xor(getVIP(args[1]), getVIP(args[2]), (ValueMeta)args[3], (ValueMeta)args[4]);
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->bit_xor(getVIP(args[1]), getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_bit_or, 3) {
                if (len > 4)
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->bit_or(getVIP(args[1]), getVIP(args[2]), (ValueMeta)args[3], (ValueMeta)args[4]);
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->bit_or(getVIP(args[1]), getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_bit_and, 3) {
                if (len > 4)
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->bit_and(getVIP(args[1]), getVIP(args[2]), (ValueMeta)args[3], (ValueMeta)args[4]);
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->bit_and(getVIP(args[1]), getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_bit_not, 2) {
                if (len > 2)
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->bit_not(getVIP(args[1]), (ValueMeta)args[2]);
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->bit_not(getVIP(args[1]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_log_not, 1) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->log_not();
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_compare, 3) {
                if (len > 4)
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->compare(getVIP(args[1]), getVIP(args[2]), (ValueMeta)args[3], (ValueMeta)args[4]);
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->compare(getVIP(args[1]), getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_jump, 2) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->jump((JumpCondition)(uint8_t)args[1], (art::ustring)args[2]);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_arg_set, 2) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arg_set(getVIP(args[1]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_call, 2) {
                bool is_async = false;
                if (len > 2) {
                    if (args[2].meta.vtype == VType::struct_) {
                        if (len > 3)
                            is_async = (bool)args[3];
                        CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->call(getVIP(args[1]), getVIP(args[2]), is_async);
                        return nullptr;
                    }
                    is_async = (bool)args[2];
                }
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->call(getVIP(args[1]), is_async);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_call_self, 1) {
                bool is_async = false;
                if (len > 1) {
                    if (args[1].meta.vtype == VType::struct_) {
                        if (len > 2)
                            is_async = (bool)args[2];
                        CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->call_self(getVIP(args[1]), is_async);
                        return nullptr;
                    }
                    is_async = (bool)args[1];
                }
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->call_self(is_async);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_add_local_fn, 1) {
                return CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->add_local_fn((art::shared_ptr<FuncEnvironment>)args[1]);
            }

            AttachAFunc(funs_FuncBuilder_call_local, 2) {
                bool is_async = false;
                if (len > 2) {
                    if (args[2].meta.vtype == VType::struct_) {
                        if (len > 3)
                            is_async = (bool)args[3];
                        CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->call_local(getVIP(args[1]), getVIP(args[2]), is_async);
                        return nullptr;
                    }
                    is_async = (bool)args[2];
                }
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->call_local(getVIP(args[1]), is_async);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_call_and_ret, 2) {
                bool is_async = len > 2 ? (bool)args[2] : false;
                bool fn_mem_only_str = len > 3 ? (bool)args[3] : false;
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->call_and_ret(getVIP(args[1]), is_async, fn_mem_only_str);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_call_self_and_ret, 1) {
                bool is_async = len > 1 ? (bool)args[1] : false;
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->call_self_and_ret(is_async);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_call_local_and_ret, 2) {
                bool is_async = len > 2 ? (bool)args[2] : false;
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->call_local_and_ret(getVIP(args[1]), is_async);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_ret, 1) {
                if (len == 1)
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->ret();
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->ret(getVIP(args[1]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_ret_take, 2) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->ret_take(getVIP(args[1]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_copy, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->copy(getVIP(args[1]), getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_move, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->move(getVIP(args[1]), getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_debug_break, 1) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->debug_break();
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_force_debug_break, 1) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->force_debug_break();
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_throw_ex, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->throw_ex(getVIP(args[1]), getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_as, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->as(getVIP(args[1]), ((ValueMeta)args[2]).vtype);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_is, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->is(getVIP(args[1]), ((ValueMeta)args[2]).vtype);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_is_gc, 2) {
                if (len == 2)
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->is_gc(getVIP(args[1]));
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->is_gc(getVIP(args[1]), getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_store_bool, 2) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->store_bool(getVIP(args[1]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_load_bool, 2) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->load_bool(getVIP(args[1]));
                return nullptr;
            }

            //internal
            AttachAFunc(funs_FuncBuilder_inline_native_opcode, 2) {
                if (allow_intern_access == false)
                    throw NotImplementedException(); //act like we don't have this function
                if (args[1].meta.vtype != VType::raw_arr_i8 && args[1].meta.vtype != VType::raw_arr_ui8)
                    CXX::excepted(args[1], VType::raw_arr_ui8);
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->inline_native_opcode((uint8_t*)args[1].getSourcePtr(), args[1].meta.val_len);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_bind_pos, 2) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->bind_pos((art::ustring)args[1]);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_arr_set, 5) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).set(getVIP(args[3]), getVIP(args[4]), len > 5 ? (bool)args[5] : true, len > 6 ? (ArrCheckMode)(uint8_t)args[6] : ArrCheckMode::no_check);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_arr_insert, 5) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).insert(getVIP(args[3]), getVIP(args[4]), len > 5 ? (bool)args[5] : true);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_arr_push_end, 4) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).push_end(getVIP(args[3]), len > 4 ? (bool)args[4] : true);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_arr_push_start, 4) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).push_start(getVIP(args[3]), len > 4 ? (bool)args[4] : true);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_arr_insert_range, 7) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).insert_range(getVIP(args[3]), getVIP(args[4]), len > 5 ? (bool)args[5] : true);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_arr_get, 5) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).get(getVIP(args[3]), getVIP(args[4]), len > 5 ? (bool)args[5] : true);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_arr_take, 5) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).take(getVIP(args[3]), getVIP(args[4]), len > 5 ? (bool)args[5] : true);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_arr_take_end, 4) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).take_end(getVIP(args[3]), len > 4 ? (bool)args[4] : true);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_arr_take_start, 4) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).take_start(getVIP(args[3]), len > 4 ? (bool)args[4] : true);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_arr_get_range, 6) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).get_range(getVIP(args[3]), getVIP(args[4]), getVIP(args[5]), len > 6 ? (bool)args[6] : true);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_arr_take_range, 6) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).take_range(getVIP(args[3]), getVIP(args[4]), getVIP(args[5]), len > 6 ? (bool)args[6] : true);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_arr_pop_end, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).pop_end();
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_arr_pop_start, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).pop_start();
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_arr_remove_item, 4) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).remove_item(getVIP(args[3]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_arr_remove_range, 5) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).remove_range(getVIP(args[3]), getVIP(args[4]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_arr_resize, 4) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).resize(getVIP(args[3]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_arr_resize_default, 5) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).resize_default(getVIP(args[3]), getVIP(args[4]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_arr_reserve_push_end, 4) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).reserve_push_end(getVIP(args[3]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_arr_reserve_push_start, 4) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).reserve_push_start(getVIP(args[3]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_arr_commit, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).commit();
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_arr_decommit, 4) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).decommit(getVIP(args[3]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_arr_remove_reserved, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).remove_reserved();
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_arr_size, 4) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).size(getVIP(args[3]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_arr_set, 4) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).set(getVIP(args[2]), getVIP(args[3]), len > 4 ? (bool)args[4] : true, len > 5 ? (ArrCheckMode)(uint8_t)args[5] : ArrCheckMode::no_check);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_arr_insert, 4) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).insert(getVIP(args[2]), getVIP(args[3]), len > 4 ? (bool)args[4] : true);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_arr_push_end, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).push_end(getVIP(args[2]), len > 3 ? (bool)args[3] : true);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_arr_push_start, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).push_start(getVIP(args[2]), len > 3 ? (bool)args[3] : true);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_arr_insert_range, 6) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).insert_range(getVIP(args[2]), getVIP(args[3]), len > 4 ? (bool)args[4] : true);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_arr_get, 4) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).get(getVIP(args[2]), getVIP(args[3]), len > 4 ? (bool)args[4] : true, len > 5 ? (ArrCheckMode)(uint8_t)args[5] : ArrCheckMode::no_check);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_arr_take, 4) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).take(getVIP(args[2]), getVIP(args[3]), len > 4 ? (bool)args[4] : true);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_arr_take_end, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).take_end(getVIP(args[2]), len > 3 ? (bool)args[3] : true);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_arr_take_start, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).take_start(getVIP(args[2]), len > 3 ? (bool)args[3] : true);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_arr_get_range, 5) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).get_range(getVIP(args[2]), getVIP(args[3]), getVIP(args[4]), len > 5 ? (bool)args[5] : true);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_arr_take_range, 5) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).take_range(getVIP(args[2]), getVIP(args[3]), getVIP(args[4]), len > 5 ? (bool)args[5] : true);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_arr_pop_end, 2) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).pop_end();
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_arr_pop_start, 2) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).pop_start();
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_arr_remove_item, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).remove_item(getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_arr_remove_range, 4) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).remove_range(getVIP(args[2]), getVIP(args[3]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_arr_resize, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).resize(getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_arr_resize_default, 4) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).resize_default(getVIP(args[2]), getVIP(args[3]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_arr_reserve_push_end, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).reserve_push_end(getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_arr_reserve_push_start, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).reserve_push_start(getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_arr_commit, 2) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).commit();
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_arr_decommit, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).decommit(getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_arr_remove_reserved, 2) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).remove_reserved();
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_arr_size, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).size(getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_call_value_interface, 4) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ClassAccess access = (ClassAccess)(uint8_t)args[1];
                ValueIndexPos class_val = getVIP(args[2]);
                auto vpos = getVIP(args[3]);
                if (len > 4) {
                    if (args[4].meta.vtype == VType::boolean)
                        builder->call_value_interface(access, class_val, vpos, (bool)args[4]);
                    else
                        builder->call_value_interface(access, class_val, vpos, getVIP(args[4]), len > 5 ? (bool)args[5] : false);
                } else
                    builder->call_value_interface(access, class_val, vpos);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_call_value_interface_id, 3) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ValueIndexPos class_val = getVIP(args[1]);
                uint64_t class_fun_id = (uint64_t)args[2];
                if (len > 3) {
                    if (args[3].meta.vtype == VType::boolean)
                        builder->call_value_interface_id(class_val, class_fun_id, (bool)args[3]);
                    else
                        builder->call_value_interface_id(class_val, class_fun_id, getVIP(args[3]), len > 4 ? (bool)args[4] : false);
                } else
                    builder->call_value_interface_id(class_val, class_fun_id);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_call_value_interface_and_ret, 4) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ClassAccess access = (ClassAccess)(uint8_t)args[1];
                ValueIndexPos class_val = getVIP(args[2]);
                builder->call_value_interface_and_ret(access, class_val, getVIP(args[3]), len > 4 ? (bool)args[4] : false);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_call_value_interface_id_and_ret, 3) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ValueIndexPos class_val = getVIP(args[1]);
                uint64_t class_fun_id = (uint64_t)args[2];
                builder->call_value_interface_id_and_ret(class_val, class_fun_id, len > 3 ? (bool)args[3] : false);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_call_value_interface, 4) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ClassAccess access = (ClassAccess)(uint8_t)args[1];
                ValueIndexPos class_val = getVIP(args[2]);
                auto vpos = getVIP(args[3]);
                if (len > 4) {
                    if (args[4].meta.vtype == VType::boolean)
                        builder->static_call_value_interface(access, class_val, vpos, (bool)args[4]);
                    else
                        builder->static_call_value_interface(access, class_val, vpos, getVIP(args[4]), len > 5 ? (bool)args[5] : false);
                } else
                    builder->static_call_value_interface(access, class_val, vpos);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_call_value_interface_id, 4) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ValueIndexPos class_val = getVIP(args[1]);
                uint64_t fun_id = (uint64_t)args[2];

                if (len == 2)
                    builder->static_call_value_interface_id(class_val, fun_id);
                else {
                    if (args[3].meta.vtype == VType::boolean)
                        builder->static_call_value_interface_id(class_val, fun_id, (bool)args[3]);
                    else
                        builder->static_call_value_interface_id(class_val, fun_id, getVIP(args[3]), len > 4 ? (bool)args[4] : false);
                }
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_call_value_interface_and_ret, 4) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ClassAccess access = (ClassAccess)(uint8_t)args[1];
                ValueIndexPos class_val = getVIP(args[2]);
                builder->static_call_value_interface_and_ret(access, class_val, getVIP(args[3]), len > 4 ? (bool)args[4] : false);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_call_value_interface_id_and_ret, 3) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ValueIndexPos class_val = getVIP(args[1]);
                uint64_t fun_id = (uint64_t)args[2];
                builder->static_call_value_interface_id_and_ret(class_val, fun_id, len > 3 ? (bool)args[3] : false);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_get_interface_value, 5) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ClassAccess access = (ClassAccess)(uint8_t)args[1];
                ValueIndexPos class_val = getVIP(args[2]);
                builder->get_interface_value(access, class_val, getVIP(args[3]), getVIP(args[4]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_set_interface_value, 5) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ClassAccess access = (ClassAccess)(uint8_t)args[1];
                ValueIndexPos class_val = getVIP(args[2]);
                builder->set_interface_value(access, class_val, getVIP(args[3]), getVIP(args[4]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_call_value_interface_opt, 7) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ClassAccess access = (ClassAccess)(uint8_t)args[1];
                ValueIndexPos class_val = getVIP(args[2]);
                ValueIndexPos class_name = getVIP(args[3]);
                ValueIndexPos separator = getVIP(args[4]);
                auto vpos = getVIP(args[5]);
                auto meta = (ValueMeta)args[6];
                if (len == 8) {
                    builder->call_value_interface(access, class_val, class_name, separator, vpos, meta, (bool)args[7]);
                } else if (len > 8) {
                    builder->call_value_interface(access, class_val, class_name, separator, vpos, meta, getVIP(args[7]), (ValueMeta)args[8], len > 9 ? (bool)args[9] : false);
                } else
                    builder->call_value_interface(access, class_val, class_name, separator, vpos, meta);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_call_value_interface_id_opt, 3) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ValueIndexPos class_val = getVIP(args[1]);
                ValueIndexPos class_name = getVIP(args[2]);
                ValueIndexPos separator = getVIP(args[3]);
                uint64_t class_fun_id = (uint64_t)args[4];
                if (len == 6) {
                    builder->call_value_interface_id(class_val, class_name, separator, class_fun_id, (bool)args[5]);
                } else if (len > 6) {
                    builder->call_value_interface_id(class_val, class_name, separator, class_fun_id, getVIP(args[5]), (ValueMeta)args[6], len > 7 ? (bool)args[7] : false);
                } else
                    builder->call_value_interface_id(class_val, class_name, separator, class_fun_id);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_call_value_interface_and_ret_opt, 7) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ClassAccess access = (ClassAccess)(uint8_t)args[1];
                ValueIndexPos class_name = getVIP(args[2]);
                ValueIndexPos separator = getVIP(args[3]);
                ValueIndexPos class_val = getVIP(args[2]);
                auto vpos = getVIP(args[5]);
                auto meta = (ValueMeta)args[6];
                builder->call_value_interface_and_ret(access, class_val, class_name, separator, vpos, meta, len > 7 ? (bool)args[7] : false);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_call_value_interface_id_and_ret_opt, 5) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ValueIndexPos class_val = getVIP(args[1]);
                ValueIndexPos class_name = getVIP(args[2]);
                ValueIndexPos separator = getVIP(args[3]);
                uint64_t class_fun_id = (uint64_t)args[4];
                builder->call_value_interface_id_and_ret(class_val, class_name, separator, class_fun_id, len > 5 ? (bool)args[5] : false);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_call_value_interface_opt, 7) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ClassAccess access = (ClassAccess)(uint8_t)args[1];
                ValueIndexPos class_val = getVIP(args[2]);
                ValueIndexPos class_name = getVIP(args[3]);
                ValueIndexPos separator = getVIP(args[4]);
                auto vpos = getVIP(args[5]);
                auto meta = (ValueMeta)args[6];
                if (len == 8) {
                    builder->static_call_value_interface(access, class_val, class_name, separator, vpos, meta, (bool)args[7]);
                } else if (len > 8) {
                    builder->static_call_value_interface(access, class_val, class_name, separator, vpos, meta, getVIP(args[7]), (ValueMeta)args[8], len > 9 ? (bool)args[9] : false);
                } else
                    builder->static_call_value_interface(access, class_val, class_name, separator, vpos, meta);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_call_value_interface_id_opt, 5) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ValueIndexPos class_val = getVIP(args[1]);
                ValueIndexPos class_name = getVIP(args[2]);
                ValueIndexPos separator = getVIP(args[3]);
                uint64_t class_fun_id = (uint64_t)args[4];
                if (len == 6) {
                    builder->static_call_value_interface_id(class_val, class_name, separator, class_fun_id, (bool)args[5]);
                } else if (len > 6) {
                    builder->static_call_value_interface_id(class_val, class_name, separator, class_fun_id, getVIP(args[5]), (ValueMeta)args[6], len > 7 ? (bool)args[7] : false);
                } else
                    builder->static_call_value_interface_id(class_val, class_name, separator, class_fun_id);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_call_value_interface_and_ret_opt, 7) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ClassAccess access = (ClassAccess)(uint8_t)args[1];
                ValueIndexPos class_name = getVIP(args[2]);
                ValueIndexPos separator = getVIP(args[3]);
                ValueIndexPos class_val = getVIP(args[2]);
                auto vpos = getVIP(args[5]);
                auto meta = (ValueMeta)args[6];
                builder->static_call_value_interface_and_ret(access, class_val, class_name, separator, vpos, meta, len > 7 ? (bool)args[7] : false);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_call_value_interface_id_and_ret_opt, 5) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ValueIndexPos class_val = getVIP(args[1]);
                ValueIndexPos class_name = getVIP(args[2]);
                ValueIndexPos separator = getVIP(args[3]);
                uint64_t class_fun_id = (uint64_t)args[4];
                builder->static_call_value_interface_id_and_ret(class_val, class_name, separator, class_fun_id, len > 5 ? (bool)args[5] : false);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_get_interface_value_opt, 8) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ClassAccess access = (ClassAccess)(uint8_t)args[1];
                ValueIndexPos class_val = getVIP(args[2]);
                ValueIndexPos class_name = getVIP(args[3]);
                ValueIndexPos separator = getVIP(args[4]);
                ValueIndexPos val_name = getVIP(args[5]);
                auto get_v = getVIP(args[6]);
                auto meta = (ValueMeta)args[7];

                builder->get_interface_value(access, class_val, class_name, separator, val_name, get_v, meta);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_set_interface_value_opt, 8) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ClassAccess access = (ClassAccess)(uint8_t)args[1];
                ValueIndexPos class_val = getVIP(args[2]);
                ValueIndexPos class_name = getVIP(args[3]);
                ValueIndexPos separator = getVIP(args[4]);
                ValueIndexPos val_name = getVIP(args[5]);
                auto set_v = getVIP(args[6]);
                auto meta = (ValueMeta)args[7];
                builder->set_interface_value(access, class_val, class_name, separator, val_name, set_v, meta);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_explicit_await, 2) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->explicit_await(getVIP(args[1]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_to_gc, 2) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->to_gc(getVIP(args[1]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_localize_gc, 2) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->localize_gc(getVIP(args[1]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_from_gc, 2) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->from_gc(getVIP(args[1]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_xarray_slice, 3) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ValueIndexPos result = getVIP(args[1]);
                ValueIndexPos val = getVIP(args[2]);
                bool first_arg_variant = false;
                bool second_arg_variant = false;
                if (len > 3) {
                    if (args[3].meta.vtype == VType::noting)
                        first_arg_variant = true;
                    else
                        first_arg_variant = false;
                }
                if (len > 4) {
                    if (args[4].meta.vtype == VType::noting)
                        second_arg_variant = true;
                    else
                        second_arg_variant = false;
                }

                if (first_arg_variant) {
                    if (second_arg_variant)
                        builder->xarray_slice(result, val);
                    else
                        builder->xarray_slice(result, val, false, getVIP(args[4]));
                } else {
                    if (second_arg_variant)
                        builder->xarray_slice(result, val, getVIP(args[3]));
                    else
                        builder->xarray_slice(result, val, getVIP(args[3]), getVIP(args[4]));
                }
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_table_jump, 2) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                std::vector<art::ustring> table;
                switch (args[1].meta.vtype) {
                case VType::uarr:
                    for (ValueItem& it : (list_array<ValueItem>&)args[1])
                        table.emplace_back((art::ustring)it);
                    break;
                case VType::string:
                    table.emplace_back((art::ustring)args[1]);
                    break;
                case VType::saarr:
                case VType::faarr: {
                    ValueItem* it = (ValueItem*)args[1].getSourcePtr();
                    uint32_t len = args[1].meta.val_len;
                    for (uint32_t i = 0; i < len; i++)
                        table.emplace_back((art::ustring)it[i]);
                    break;
                }
                default:
                    CXX::excepted(args[1], VType::uarr);
                    break;
                }
                ValueIndexPos check_val = getVIP(args[2]);
                bool is_signed = len > 3 ? (bool)args[3] : false;
                TableJumpCheckFailAction too_large = len > 4 ? (TableJumpCheckFailAction)(uint8_t)args[4] : TableJumpCheckFailAction::throw_exception;
                art::ustring too_large_label = len > 5 ? (art::ustring)args[5] : "";
                TableJumpCheckFailAction too_small = len > 6 ? (TableJumpCheckFailAction)(uint8_t)args[6] : TableJumpCheckFailAction::throw_exception;
                art::ustring too_small_label = len > 7 ? (art::ustring)args[7] : "";
                builder->table_jump(table, check_val, is_signed, too_large, too_large_label, too_small, too_small_label);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_get_reference, 2) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->get_reference(getVIP(args[1]), getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_make_as_const, 2) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->make_as_const(getVIP(args[1]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_remove_const_protect, 2) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->remove_const_protect(getVIP(args[1]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_copy_un_constant, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->copy_un_constant(getVIP(args[1]), getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_copy_un_reference, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->copy_un_reference(getVIP(args[1]), getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_move_un_reference, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->move_un_reference(getVIP(args[1]), getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_remove_qualifiers, 2) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->remove_qualifiers(getVIP(args[1]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_global_get, 4) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->global_get(getVIP(args[1]), getVIP(args[2]), getVIP(args[3]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_global_set, 4) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->global_set(getVIP(args[1]), getVIP(args[2]), getVIP(args[3]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_map_set, 2) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->map(getVIP(args[1])).set(getVIP(args[2]), getVIP(args[3]), len > 4 ? (bool)args[4] : false);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_map_get, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->map(getVIP(args[1])).get(getVIP(args[2]), getVIP(args[3]), len > 4 ? (bool)args[4] : false);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_map_remove, 2) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->map(getVIP(args[1])).remove(getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_map_has_key, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->map(getVIP(args[1])).has_key(getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_map_reserve, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->map(getVIP(args[1])).reserve(getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_map_size, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->map(getVIP(args[1])).size(getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_map_set, 2) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_map(getVIP(args[1])).set(getVIP(args[2]), getVIP(args[3]), len > 4 ? (bool)args[4] : false);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_map_get, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_map(getVIP(args[1])).get(getVIP(args[2]), getVIP(args[3]), len > 4 ? (bool)args[4] : false);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_map_remove, 2) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_map(getVIP(args[1])).remove(getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_map_has_key, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_map(getVIP(args[1])).has_key(getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_map_reserve, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_map(getVIP(args[1])).reserve(getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_map_size, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_map(getVIP(args[1])).size(getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_set_set, 2) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->set(getVIP(args[1])).set(getVIP(args[2]), getVIP(args[3]), len > 4 ? (bool)args[4] : false);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_set_remove, 2) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->set(getVIP(args[1])).remove(getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_set_has_key, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->set(getVIP(args[1])).has_key(getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_set_reserve, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->set(getVIP(args[1])).reserve(getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_set_size, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->set(getVIP(args[1])).size(getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_set_set, 2) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_set(getVIP(args[1])).set(getVIP(args[2]), getVIP(args[3]), len > 4 ? (bool)args[4] : false);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_set_remove, 2) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_set(getVIP(args[1])).remove(getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_set_has_key, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_set(getVIP(args[1])).has_key(getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_set_reserve, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_set(getVIP(args[1])).reserve(getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_static_set_size, 3) {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_set(getVIP(args[1])).size(getVIP(args[2]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_O_flag_can_be_unloaded, 2) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                builder->O_flag_can_be_unloaded((bool)args[1]);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_O_flag_is_translated, 2) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                builder->O_flag_is_translated((bool)args[1]);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_O_flag_is_cheap, 2) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                builder->O_flag_is_cheap((bool)args[1]);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_O_flag_used_vec128, 2) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                builder->O_flag_used_vec128((uint8_t)args[1]);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_O_flag_is_patchable, 2) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                builder->O_flag_is_patchable((bool)args[1]);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_O_line_info_begin, 2) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                return makeLineInfo(builder->O_line_info_begin(), (uint64_t)args[1], len > 2 ? (uint64_t)args[2] : 0);
            }

            AttachAFunc(funs_FuncBuilder_O_line_info_end, 2) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                builder->O_line_info_end(getLineInfo(args[1]));
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_O_prepare_func, 1) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                return builder->O_prepare_func();
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_O_build_func, 1) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                auto result = builder->O_build_func();
                if (result.size() == (uint32_t)result.size())
                    return ValueItem(result.data(), (uint32_t)result.size());
                else {
                    list_array<ValueItem> map_result;
                    map_result.reserve_back(result.size());
                    for (uint8_t it : result)
                        map_result.push_back(ValueItem(it));
                    return map_result;
                }
            }

            AttachAFunc(funs_FuncBuilder_O_load_func, 2) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                builder->O_load_func((art::ustring)args[1]);
                return nullptr;
            }

            AttachAFunc(funs_FuncBuilder_O_patch_func, 2) {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                builder->O_patch_func((art::ustring)args[1]);
                return nullptr;
            }

            AttachAFunc(funs_FuncEnviroBuilder_line_info_set_line, 2) {
                auto& line_info = CXX::Interface::getExtractAs<art::line_info>(args[0], define_FuncEnviroBuilder_line_info);
                line_info.line = (uint64_t)args[1];
                return nullptr;
            }

            AttachAFunc(funs_FuncEnviroBuilder_line_info_set_column, 2) {
                auto& line_info = CXX::Interface::getExtractAs<art::line_info>(args[0], define_FuncEnviroBuilder_line_info);
                line_info.column = (uint64_t)args[1];
                return nullptr;
            }

            AttachAFunc(funs_FuncEnviroBuilder_line_info_get_line, 1) {
                auto& line_info = CXX::Interface::getExtractAs<art::line_info>(args[0], define_FuncEnviroBuilder_line_info);
                return line_info.line;
            }

            AttachAFunc(funs_FuncEnviroBuilder_line_info_get_column, 1) {
                auto& line_info = CXX::Interface::getExtractAs<art::line_info>(args[0], define_FuncEnviroBuilder_line_info);
                return line_info.column;
            }

            AttachAFunc(funs_ValueIndexPos_set_index, 2) {
                auto& vpos = CXX::Interface::getExtractAs<ValueIndexPos>(args[0], define_ValueIndexPos);
                vpos.index = (uint16_t)args[1];
                return nullptr;
            }

            AttachAFunc(funs_ValueIndexPos_set_pos, 2) {
                auto& vpos = CXX::Interface::getExtractAs<ValueIndexPos>(args[0], define_ValueIndexPos);
                vpos.pos = (ValuePos)(uint8_t)args[1];
                return nullptr;
            }

            AttachAFunc(funs_ValueIndexPos_get_index, 1) {
                auto& vpos = CXX::Interface::getExtractAs<ValueIndexPos>(args[0], define_ValueIndexPos);
                return vpos.index;
            }

            AttachAFunc(funs_ValueIndexPos_get_pos, 1) {
                auto& vpos = CXX::Interface::getExtractAs<ValueIndexPos>(args[0], define_ValueIndexPos);
                return (uint8_t)vpos.pos;
            }

            void init() {
                define_ValueIndexPos = CXX::Interface::createTable<ValueIndexPos>(
                    "index_pos",
                    CXX::Interface::direct_method("set_index", funs_ValueIndexPos_set_index),
                    CXX::Interface::direct_method("set_pos", funs_ValueIndexPos_set_pos),
                    CXX::Interface::direct_method("get_index", funs_ValueIndexPos_get_index),
                    CXX::Interface::direct_method("get_pos", funs_ValueIndexPos_get_pos)
                );
                CXX::Interface::typeVTable<ValueIndexPos>() = define_ValueIndexPos;
                define_ValueIndexPos->getAfterMethods()->constructor = new FuncEnvironment(createProxy_index_pos);
                attacha_environment::get_type({"internal", "index_pos"})->value = define_ValueIndexPos;

                define_FuncEnviroBuilder_line_info = CXX::Interface::createTable<art::line_info>(
                    "line_info",
                    CXX::Interface::direct_method("set_line", funs_FuncEnviroBuilder_line_info_set_line),
                    CXX::Interface::direct_method("set_column", funs_FuncEnviroBuilder_line_info_set_column),
                    CXX::Interface::direct_method("get_line", funs_FuncEnviroBuilder_line_info_get_line),
                    CXX::Interface::direct_method("get_column", funs_FuncEnviroBuilder_line_info_get_column)
                );
                CXX::Interface::typeVTable<art::line_info>() = define_FuncEnviroBuilder_line_info;
                attacha_environment::get_type({"internal", "line_info"})->value = define_FuncEnviroBuilder_line_info;

                define_FuncBuilder = CXX::Interface::createTable<typed_lgr<FuncEnviroBuilder>>(
                    "func_builder",
                    CXX::Interface::direct_method("create_constant", funs_FuncBuilder_create_constant),
                    CXX::Interface::direct_method("set_stack_any_array", funs_FuncBuilder_set_stack_any_array),
                    CXX::Interface::direct_method("remove", funs_FuncBuilder_remove),
                    CXX::Interface::direct_method("sum", funs_FuncBuilder_sum),
                    CXX::Interface::direct_method("minus", funs_FuncBuilder_minus),
                    CXX::Interface::direct_method("div", funs_FuncBuilder_div),
                    CXX::Interface::direct_method("mul", funs_FuncBuilder_mul),
                    CXX::Interface::direct_method("rest", funs_FuncBuilder_rest),
                    CXX::Interface::direct_method("bit_xor", funs_FuncBuilder_bit_xor),
                    CXX::Interface::direct_method("bit_or", funs_FuncBuilder_bit_or),
                    CXX::Interface::direct_method("bit_and", funs_FuncBuilder_bit_and),
                    CXX::Interface::direct_method("bit_not", funs_FuncBuilder_bit_not),
                    CXX::Interface::direct_method("log_not", funs_FuncBuilder_log_not),
                    CXX::Interface::direct_method("compare", funs_FuncBuilder_compare),
                    CXX::Interface::direct_method("jump", funs_FuncBuilder_jump),
                    CXX::Interface::direct_method("arg_set", funs_FuncBuilder_arg_set),
                    CXX::Interface::direct_method("call", funs_FuncBuilder_call),
                    CXX::Interface::direct_method("call_self", funs_FuncBuilder_call_self),
                    CXX::Interface::direct_method("add_local_fn", funs_FuncBuilder_add_local_fn),
                    CXX::Interface::direct_method("call_local", funs_FuncBuilder_call_local),
                    CXX::Interface::direct_method("call_and_ret", funs_FuncBuilder_call_and_ret),
                    CXX::Interface::direct_method("call_self_and_ret", funs_FuncBuilder_call_self_and_ret),
                    CXX::Interface::direct_method("call_local_and_ret", funs_FuncBuilder_call_local_and_ret),
                    CXX::Interface::direct_method("ret", funs_FuncBuilder_ret),
                    CXX::Interface::direct_method("ret_take", funs_FuncBuilder_ret_take),
                    CXX::Interface::direct_method("copy", funs_FuncBuilder_copy),
                    CXX::Interface::direct_method("move", funs_FuncBuilder_move),
                    CXX::Interface::direct_method("debug_break", funs_FuncBuilder_debug_break),
                    CXX::Interface::direct_method("force_debug_break", funs_FuncBuilder_force_debug_break),
                    CXX::Interface::direct_method("throw_ex", funs_FuncBuilder_throw_ex),
                    CXX::Interface::direct_method("as", funs_FuncBuilder_as),
                    CXX::Interface::direct_method("is", funs_FuncBuilder_is),
                    CXX::Interface::direct_method("is_gc", funs_FuncBuilder_is_gc),
                    CXX::Interface::direct_method("store_bool", funs_FuncBuilder_store_bool),
                    CXX::Interface::direct_method("load_bool", funs_FuncBuilder_load_bool),
                    CXX::Interface::direct_method("inline_native_opcode", funs_FuncBuilder_inline_native_opcode, ClassAccess::intern),
                    CXX::Interface::direct_method("bind_pos", funs_FuncBuilder_bind_pos),

                    CXX::Interface::direct_method("arr_set", funs_FuncBuilder_arr_set),
                    CXX::Interface::direct_method("arr_insert", funs_FuncBuilder_arr_insert),
                    CXX::Interface::direct_method("arr_push_end", funs_FuncBuilder_arr_push_end),
                    CXX::Interface::direct_method("arr_push_start", funs_FuncBuilder_arr_push_start),
                    CXX::Interface::direct_method("arr_insert_range", funs_FuncBuilder_arr_insert_range),
                    CXX::Interface::direct_method("arr_get", funs_FuncBuilder_arr_get),
                    CXX::Interface::direct_method("arr_take", funs_FuncBuilder_arr_take),
                    CXX::Interface::direct_method("arr_take_end", funs_FuncBuilder_arr_take_end),
                    CXX::Interface::direct_method("arr_take_start", funs_FuncBuilder_arr_take_start),
                    CXX::Interface::direct_method("arr_get_range", funs_FuncBuilder_arr_get_range),
                    CXX::Interface::direct_method("arr_take_range", funs_FuncBuilder_arr_take_range),
                    CXX::Interface::direct_method("arr_pop_end", funs_FuncBuilder_arr_pop_end),
                    CXX::Interface::direct_method("arr_pop_start", funs_FuncBuilder_arr_pop_start),
                    CXX::Interface::direct_method("arr_remove_item", funs_FuncBuilder_arr_remove_item),
                    CXX::Interface::direct_method("arr_remove_range", funs_FuncBuilder_arr_remove_range),
                    CXX::Interface::direct_method("arr_resize", funs_FuncBuilder_arr_resize),
                    CXX::Interface::direct_method("arr_resize_default", funs_FuncBuilder_arr_resize_default),
                    CXX::Interface::direct_method("arr_reserve_push_end", funs_FuncBuilder_arr_reserve_push_end),
                    CXX::Interface::direct_method("arr_reserve_push_start", funs_FuncBuilder_arr_reserve_push_start),
                    CXX::Interface::direct_method("arr_commit", funs_FuncBuilder_arr_commit),
                    CXX::Interface::direct_method("arr_decommit", funs_FuncBuilder_arr_decommit),
                    CXX::Interface::direct_method("arr_remove_reserved", funs_FuncBuilder_arr_remove_reserved),
                    CXX::Interface::direct_method("arr_size", funs_FuncBuilder_arr_size),

                    CXX::Interface::direct_method("static_arr_set", funs_FuncBuilder_static_arr_set),
                    CXX::Interface::direct_method("static_arr_insert", funs_FuncBuilder_static_arr_insert),
                    CXX::Interface::direct_method("static_arr_push_end", funs_FuncBuilder_static_arr_push_end),
                    CXX::Interface::direct_method("static_arr_push_start", funs_FuncBuilder_static_arr_push_start),
                    CXX::Interface::direct_method("static_arr_insert_range", funs_FuncBuilder_static_arr_insert_range),
                    CXX::Interface::direct_method("static_arr_get", funs_FuncBuilder_static_arr_get),
                    CXX::Interface::direct_method("static_arr_take", funs_FuncBuilder_static_arr_take),
                    CXX::Interface::direct_method("static_arr_take_end", funs_FuncBuilder_static_arr_take_end),
                    CXX::Interface::direct_method("static_arr_take_start", funs_FuncBuilder_static_arr_take_start),
                    CXX::Interface::direct_method("static_arr_get_range", funs_FuncBuilder_static_arr_get_range),
                    CXX::Interface::direct_method("static_arr_take_range", funs_FuncBuilder_static_arr_take_range),
                    CXX::Interface::direct_method("static_arr_pop_end", funs_FuncBuilder_static_arr_pop_end),
                    CXX::Interface::direct_method("static_arr_pop_start", funs_FuncBuilder_static_arr_pop_start),
                    CXX::Interface::direct_method("static_arr_remove_item", funs_FuncBuilder_static_arr_remove_item),
                    CXX::Interface::direct_method("static_arr_remove_range", funs_FuncBuilder_static_arr_remove_range),
                    CXX::Interface::direct_method("static_arr_resize", funs_FuncBuilder_static_arr_resize),
                    CXX::Interface::direct_method("static_arr_resize_default", funs_FuncBuilder_static_arr_resize_default),
                    CXX::Interface::direct_method("static_arr_reserve_push_end", funs_FuncBuilder_static_arr_reserve_push_end),
                    CXX::Interface::direct_method("static_arr_reserve_push_start", funs_FuncBuilder_static_arr_reserve_push_start),
                    CXX::Interface::direct_method("static_arr_commit", funs_FuncBuilder_static_arr_commit),
                    CXX::Interface::direct_method("static_arr_decommit", funs_FuncBuilder_static_arr_decommit),
                    CXX::Interface::direct_method("static_arr_remove_reserved", funs_FuncBuilder_static_arr_remove_reserved),
                    CXX::Interface::direct_method("static_arr_size", funs_FuncBuilder_static_arr_size),

                    CXX::Interface::direct_method("call_value_interface", funs_FuncBuilder_call_value_interface),
                    CXX::Interface::direct_method("call_value_interface_id", funs_FuncBuilder_call_value_interface_id),
                    CXX::Interface::direct_method("call_value_interface_and_ret", funs_FuncBuilder_call_value_interface_and_ret),
                    CXX::Interface::direct_method("call_value_interface_id_and_ret", funs_FuncBuilder_call_value_interface_id_and_ret),
                    CXX::Interface::direct_method("static_call_value_interface", funs_FuncBuilder_static_call_value_interface),
                    CXX::Interface::direct_method("static_call_value_interface_id", funs_FuncBuilder_static_call_value_interface_id),
                    CXX::Interface::direct_method("static_call_value_interface_and_ret", funs_FuncBuilder_static_call_value_interface_and_ret),
                    CXX::Interface::direct_method("static_call_value_interface_id_and_ret", funs_FuncBuilder_static_call_value_interface_id_and_ret),
                    CXX::Interface::direct_method("get_interface_value", funs_FuncBuilder_get_interface_value),
                    CXX::Interface::direct_method("set_interface_value", funs_FuncBuilder_set_interface_value),

                    CXX::Interface::direct_method("call_value_interface_opt", funs_FuncBuilder_call_value_interface_opt),
                    CXX::Interface::direct_method("call_value_interface_id_opt ", funs_FuncBuilder_call_value_interface_id_opt),
                    CXX::Interface::direct_method("call_value_interface_and_ret_opt", funs_FuncBuilder_call_value_interface_and_ret_opt),
                    CXX::Interface::direct_method("call_value_interface_id_and_ret_opt", funs_FuncBuilder_call_value_interface_id_and_ret_opt),
                    CXX::Interface::direct_method("static_call_value_interface_opt ", funs_FuncBuilder_static_call_value_interface_opt),
                    CXX::Interface::direct_method("static_call_value_interface_id_opt ", funs_FuncBuilder_static_call_value_interface_id_opt),
                    CXX::Interface::direct_method("static_call_value_interface_and_ret_opt", funs_FuncBuilder_static_call_value_interface_and_ret_opt),
                    CXX::Interface::direct_method("static_call_value_interface_id_and_ret_opt", funs_FuncBuilder_static_call_value_interface_id_and_ret_opt),
                    CXX::Interface::direct_method("get_interface_value_opt", funs_FuncBuilder_get_interface_value_opt),
                    CXX::Interface::direct_method("set_interface_value_opt", funs_FuncBuilder_set_interface_value_opt),

                    CXX::Interface::direct_method("explicit_await", funs_FuncBuilder_explicit_await),
                    CXX::Interface::direct_method("to_gc", funs_FuncBuilder_to_gc),
                    CXX::Interface::direct_method("localize_gc", funs_FuncBuilder_localize_gc),
                    CXX::Interface::direct_method("from_gc", funs_FuncBuilder_from_gc),
                    CXX::Interface::direct_method("xarray_slice", funs_FuncBuilder_xarray_slice),
                    CXX::Interface::direct_method("table_jump", funs_FuncBuilder_table_jump),

                    CXX::Interface::direct_method("get_reference", funs_FuncBuilder_get_reference),
                    CXX::Interface::direct_method("make_as_const", funs_FuncBuilder_make_as_const),
                    CXX::Interface::direct_method("remove_const_protect", funs_FuncBuilder_remove_const_protect),
                    CXX::Interface::direct_method("copy_un_constant", funs_FuncBuilder_copy_un_constant),
                    CXX::Interface::direct_method("copy_un_reference", funs_FuncBuilder_copy_un_reference),
                    CXX::Interface::direct_method("move_un_reference", funs_FuncBuilder_move_un_reference),
                    CXX::Interface::direct_method("remove_qualifiers", funs_FuncBuilder_remove_qualifiers),

                    CXX::Interface::direct_method("global_get", funs_FuncBuilder_global_get),
                    CXX::Interface::direct_method("global_set", funs_FuncBuilder_global_set),

                    CXX::Interface::direct_method("map_set", funs_FuncBuilder_map_set),
                    CXX::Interface::direct_method("map_get", funs_FuncBuilder_map_get),
                    CXX::Interface::direct_method("map_remove", funs_FuncBuilder_map_remove),
                    CXX::Interface::direct_method("map_has_key", funs_FuncBuilder_map_has_key),
                    CXX::Interface::direct_method("map_reserve", funs_FuncBuilder_map_reserve),
                    CXX::Interface::direct_method("map_size", funs_FuncBuilder_map_size),

                    CXX::Interface::direct_method("static_map_set", funs_FuncBuilder_static_map_set),
                    CXX::Interface::direct_method("static_map_get", funs_FuncBuilder_static_map_get),
                    CXX::Interface::direct_method("static_map_remove", funs_FuncBuilder_static_map_remove),
                    CXX::Interface::direct_method("static_map_has_key", funs_FuncBuilder_static_map_has_key),
                    CXX::Interface::direct_method("static_map_reserve", funs_FuncBuilder_static_map_reserve),
                    CXX::Interface::direct_method("static_map_size", funs_FuncBuilder_static_map_size),

                    CXX::Interface::direct_method("set_set", funs_FuncBuilder_set_set),
                    CXX::Interface::direct_method("set_remove", funs_FuncBuilder_set_remove),
                    CXX::Interface::direct_method("set_has_key", funs_FuncBuilder_set_has_key),
                    CXX::Interface::direct_method("set_reserve", funs_FuncBuilder_set_reserve),
                    CXX::Interface::direct_method("set_size", funs_FuncBuilder_set_size),

                    CXX::Interface::direct_method("static_set_set", funs_FuncBuilder_static_set_set),
                    CXX::Interface::direct_method("static_set_remove", funs_FuncBuilder_static_set_remove),
                    CXX::Interface::direct_method("static_set_has_key", funs_FuncBuilder_static_set_has_key),
                    CXX::Interface::direct_method("static_set_reserve", funs_FuncBuilder_static_set_reserve),
                    CXX::Interface::direct_method("static_set_size", funs_FuncBuilder_static_set_size),


                    CXX::Interface::direct_method("O_flag_can_be_unloaded", funs_FuncBuilder_O_flag_can_be_unloaded),
                    CXX::Interface::direct_method("O_flag_is_translated", funs_FuncBuilder_O_flag_is_translated),
                    CXX::Interface::direct_method("O_flag_is_cheap", funs_FuncBuilder_O_flag_is_cheap),
                    CXX::Interface::direct_method("O_flag_used_vec128", funs_FuncBuilder_O_flag_used_vec128),
                    CXX::Interface::direct_method("O_flag_is_patchable", funs_FuncBuilder_O_flag_is_patchable),
                    CXX::Interface::direct_method("O_line_info_begin", funs_FuncBuilder_O_line_info_begin),
                    CXX::Interface::direct_method("O_line_info_end", funs_FuncBuilder_O_line_info_end),
                    CXX::Interface::direct_method("O_prepare_func", funs_FuncBuilder_O_prepare_func),
                    CXX::Interface::direct_method("O_build_func", funs_FuncBuilder_O_build_func),
                    CXX::Interface::direct_method("O_load_func", funs_FuncBuilder_O_load_func),
                    CXX::Interface::direct_method("O_patch_func", funs_FuncBuilder_O_patch_func)
                );
                CXX::Interface::typeVTable<typed_lgr<FuncEnviroBuilder>>() = define_FuncBuilder;
                define_FuncBuilder->getAfterMethods()->constructor = new FuncEnvironment(createProxy_function_builder);
                attacha_environment::get_type({"internal", "func_builder"})->value = define_FuncBuilder;
            }

            ValueItem* createProxy_function_builder(ValueItem* args, uint32_t len) {
                bool strict_mode = len >= 1 ? (bool)args[0] : true;
                bool use_dynamic_values = len >= 2 ? (bool)args[1] : true;
                return new ValueItem(CXX::Interface::constructStructure<typed_lgr<FuncEnviroBuilder>>(define_FuncBuilder, new FuncEnviroBuilder(strict_mode, use_dynamic_values)));
            }

            ValueItem* createProxy_index_pos(ValueItem* args, uint32_t len) {
                uint16_t index = len >= 1 ? (uint16_t)args[0] : 0;
                ValuePos pos = len >= 2 ? (ValuePos)(uint8_t)args[1] : ValuePos::in_enviro;
                return new ValueItem(CXX::Interface::constructStructure<ValueIndexPos>(define_ValueIndexPos, index, pos));
            }
        }

        void init(bool vtable_full_mode, bool allow_self_build) {
            if (allow_self_build)
                constructor::init();
            if (vtable_full_mode)
                init_vtable_views_full_mode();
            else
                init_vtable_views();
        }
    }
}