// Copyright Danyil Melnytskyi 2025-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <base/language/precompiled.hpp>
#include <run_time/AttachA_CXX.hpp>
#include <run_time/library/bytes.hpp>
#include <run_time/util/tools.hpp>
#include <util/exceptions.hpp>
using namespace art;

namespace language_parsers {
    template <class T>
    T read_value(files::FileHandle& file) {
        T value;
        file.read_fixed((uint8_t*)&value, sizeof(T));
        return bytes::convert_endian<bytes::Endian::little>(value);
    }

    std::vector<uint8_t> read_byte_arr(files::FileHandle& file) {
        uint64_t arr_length = read_value<uint64_t>(file);
        std::vector<uint8_t> array(arr_length);
        file.read_fixed(array.data(), arr_length);
        return array;
    }

    ustring read_string(files::FileHandle& file) {
        auto string_value = read_byte_arr(file);
        return ustring((char*)string_value.data(), string_value.size());
    }

    list_array<ustring> read_type_string(files::FileHandle& file) {
        uint64_t size = read_value<uint64_t>(file);
        list_array<ustring> res;
        res.reserve(size);
        for (uint64_t i = 0; i < size; i++)
            res.push_back(read_string(file));
        return res;
    }

    ValueMeta read_value_meta(files::FileHandle& file) {
        union tttt_ {
            struct {
                VType type;
                uint8_t use_gc : 1;
                uint8_t allow_edit : 1;
                uint8_t as_ref : 1;
                uint32_t val_len;
            };

            uint32_t raw;

            tttt_(uint32_t r)
                : raw(r) {}
        } res(read_value<uint32_t>(file));

        return ValueMeta(res.type, res.use_gc, res.allow_edit, res.val_len, res.as_ref);
    }

    ValueItem readAny(files::FileHandle& file) {
        auto data_res = read_byte_arr(file);
        size_t read_pointer = 0;
        auto res = art::reader::readAny(data_res, data_res.size(), read_pointer);
        if (read_pointer != data_res.size())
            throw InvalidEncodingException("Value size mismatch, readed less bytes than encoded");
        return std::move(res);
    }

    //symbol, decl, hash
    std::tuple<ustring, FuncHandle::inner_handle*, size_t> read_function(files::FileHandle& file) {
        ustring symbol = read_string(file);
        ustring cross_compiler_version = read_string(file);
        std::vector<uint8_t> _opcode = read_byte_arr(file);
        bool is_cheap = read_value<bool>(file);
        uint64_t hash = art::hash<uint8_t>()(_opcode.data(), _opcode.size());
        return {symbol, new FuncHandle::inner_handle(std::move(_opcode), is_cheap, cross_compiler_version.empty() ? nullptr : new ustring(cross_compiler_version)), hash};
    }

    VirtualTable read_type_info(files::FileHandle& file) {
        uint64_t structure_bytes = read_value<uint64_t>(file);
        bool allow_auto_copy;


        list_array<MethodInfo> methods;
        list_array<ValueInfo> values;
        art::shared_ptr<FuncEnvironment> destructor;
        art::shared_ptr<FuncEnvironment> copy;
        art::shared_ptr<FuncEnvironment> move;
        art::shared_ptr<FuncEnvironment> compare;
        art::shared_ptr<FuncEnvironment> constructor;
        {
            union ttttt_ {
                struct {
                    bool declared_destructor;
                    bool declared_copy;
                    bool declared_move;
                    bool declared_compare;
                    bool declared_constructor;
                    bool allow_auto_copy;
                };

                uint64_t raw;

                ttttt_(uint64_t r)
                    : raw(r) {}
            } flags(read_value<uint64_t>(file));

            allow_auto_copy = flags.allow_auto_copy;
            if (flags.declared_destructor)
                destructor = new FuncEnvironment(std::get<1>(read_function(file)), true);
            if (flags.declared_copy)
                copy = new FuncEnvironment(std::get<1>(read_function(file)), true);
            if (flags.declared_move)
                move = new FuncEnvironment(std::get<1>(read_function(file)), true);
            if (flags.declared_compare)
                compare = new FuncEnvironment(std::get<1>(read_function(file)), true);
            if (flags.declared_constructor)
                constructor = new FuncEnvironment(std::get<1>(read_function(file)), true);
        }

        {
            uint64_t methods_count = read_value<uint64_t>(file);
            methods.reserve(methods_count);
            for (uint64_t i = 0; i < methods_count; i++) {
                auto [name, decl, hash] = read_function(file);
                art::shared_ptr<FuncEnvironment> method = new FuncEnvironment(decl, true);
                ClassAccess access = read_value<ClassAccess>(file);
                art::ustring owner_name = read_string(file);
                list_array<ValueMeta> return_values;
                list_array<list_array<std::pair<ValueMeta, art::ustring>>> arguments;
                list_array<MethodTag> tags;
                {
                    uint64_t return_values_count = read_value<uint64_t>(file);
                    return_values.reserve(return_values_count);
                    for (uint64_t j = 0; j < return_values_count; j++)
                        return_values.push_back(read_value_meta(file));
                }
                {
                    uint64_t arguments_count = read_value<uint64_t>(file);
                    arguments.reserve(arguments_count);
                    for (uint64_t j = 0; j < arguments_count; j++) {
                        list_array<std::pair<ValueMeta, art::ustring>> argument_variant;
                        uint64_t argument_variant_count = read_value<uint64_t>(file);
                        argument_variant.reserve(argument_variant_count);
                        for (uint64_t n = 0; n < argument_variant_count; n++) {
                            auto meta = read_value_meta(file);
                            argument_variant.push_back({meta, read_string(file)});
                        }
                        arguments.push_back(std::move(argument_variant));
                    }
                }
                {
                    uint64_t tags_count = read_value<uint64_t>(file);
                    tags.reserve(tags_count);
                    for (uint64_t j = 0; j < tags_count; j++) {
                        MethodTag tag;
                        tag.name = read_string(file);
                        tag.enviro = new FuncEnvironment(std::get<1>(read_function(file)), true);
                        tag.value = readAny(file);
                        tags.push_back(std::move(tag));
                    }
                }
                methods.push_back(MethodInfo(name, method, access, std::move(return_values), std::move(arguments), std::move(tags), owner_name));
            }
        }
        {
            uint64_t values_count = read_value<uint64_t>(file);
            values.reserve(values_count);
            for (uint64_t i = 0; i < values_count; i++) {
                ustring name = read_string(file);
                size_t offset = read_value<uint64_t>(file);
                ValueMeta type = read_value_meta(file);
                uint16_t bit_used = read_value<uint16_t>(file);
                uint8_t bit_offset = read_value<uint8_t>(file);
                bool inlined = read_value<bool>(file);
                bool allow_abstract_assign = read_value<bool>(file);
                ClassAccess access = read_value<ClassAccess>(file);
                list_array<ValueTag> tags;
                {
                    uint64_t tags_count = read_value<uint64_t>(file);
                    tags.reserve(tags_count);
                    for (uint64_t j = 0; j < tags_count; j++) {
                        MethodTag tag;
                        tag.name = read_string(file);
                        tag.enviro = new FuncEnvironment(std::get<1>(read_function(file)), true);
                        tag.value = readAny(file);
                        tags.push_back(std::move(tag));
                    }
                }
                bool zero_after_cleanup = read_value<bool>(file);
                values.push_back(ValueInfo(name, offset, type, bit_used, bit_offset, inlined, allow_abstract_assign, access, tags, zero_after_cleanup));
            }
        }

        char mode = read_value<char>(file);
        switch (mode) {
        case 's':
            return AttachAVirtualTable::create(methods, values, destructor, copy, move, compare, structure_bytes, allow_auto_copy, constructor);
        case 'd': {
            auto res = new AttachADynamicVirtualTable(methods, values, destructor, copy, move, compare, structure_bytes, allow_auto_copy);
            res->constructor = constructor;
            return res;
        }
        default:
            throw InvalidEncodingException("Unrecognized virtual table type: " + std::string(1, mode));
        }
    }

    patch_list precompiled::handle_init(files::FileHandle& file) {
        lock_guard guard(mutex);
        patch_list build_patch_list;
        auto& local_functions = declared_functions[file.get_path()];
        auto& local_types = declared_types[file.get_path()];
        {
            std::unordered_set<ustring, hash<ustring>> readed_functions;
            uint64_t _functions_count = read_value<uint64_t>(file);
            for (uint64_t i = 0; i < _functions_count; i++) {
                auto [symbol, decl, hash] = read_function(file);

                if (symbol.starts_with('\2')) {
                    CXX::cxxCall(new FuncEnvironment(decl, true));
                    continue;
                }

                readed_functions.insert(symbol);
                auto it = local_functions.find(symbol);
                if (it == local_functions.end())
                    local_functions[symbol] = hash;
                else if (it->second == hash)
                    continue;

                if (symbol.starts_with('\3')) {
                    CXX::cxxCall(new FuncEnvironment(decl, true));
                    continue;
                }

                build_patch_list.define_function(symbol, decl);
            }

            list_array<art::ustring> functions;
            for (auto& it : local_functions) {
                if (!readed_functions.contains(it.first)) {
                    build_patch_list.undefine_function(it.first);
                    functions.push_back(it.first);
                }
            }
            functions.for_each([&](const art::ustring& symbol) {
                local_functions.erase(symbol);
            });
        }
        {
            std::unordered_set<list_array<art::ustring>, hash<list_array<art::ustring>>> readed_types;
            uint64_t _types_count = read_value<uint64_t>(file);
            for (uint64_t i = 0; i < _types_count; i++) {
                auto type_namespace = read_type_string(file);
                uint64_t hash = read_value<uint64_t>(file);
                auto type_d = read_type_info(file);

                readed_types.insert(type_namespace);
                auto it = local_types.find(type_namespace);
                if (it == local_types.end())
                    local_types[type_namespace] = hash;
                else if (it->second == hash)
                    continue;
                build_patch_list.define_type(type_namespace, std::move(type_d));
            }


            list_array<list_array<art::ustring>> functions;
            for (auto& it : local_types) {
                if (!readed_types.contains(it.first)) {
                    build_patch_list.undefine_type(it.first);
                    functions.push_back(it.first);
                }
            }
            functions.for_each([&](const list_array<art::ustring>& symbol) {
                local_types.erase(symbol);
            });
        }
        return build_patch_list;
    }

    patch_list precompiled::handle_init_complete() {
        return {};
    }

    patch_list precompiled::handle_create(files::FileHandle& file) {
        return handle_init(file);
    }

    patch_list precompiled::handle_renamed(const ustring& old, files::FileHandle& file) {
        {
            lock_guard guard(mutex);
            declared_functions[file.get_path()] = declared_functions[old];
        }
        return handle_init(file);
    }

    patch_list precompiled::handle_changed(files::FileHandle& file) {
        return handle_init(file);
    }

    patch_list precompiled::handle_removed(const ustring& removed) {
        lock_guard guard(mutex);
        patch_list build_patch_list;
        for (auto& it : declared_functions[removed])
            build_patch_list.undefine_function(it.first);
        for (auto& it : declared_types[removed])
            build_patch_list.undefine_type(it.first);
        declared_functions.erase(removed);
        return build_patch_list;
    }
}
