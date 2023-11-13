// Copyright Danyil Melnytskyi 2023-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/AttachA_CXX.hpp>

namespace art {
    namespace structure_helpers {
        ValueItem getAnyObj(char* ptr, const ValueInfo& item) {
            if (item.inlined)
                return static_value_get_ref<ValueItem>(ptr, item.offset, item.bit_used, item.bit_offset);
            else
                return *static_value_get_ref<ValueItem*>(ptr, item.offset, item.bit_used, item.bit_offset);
        }

        ValueItem& getAnyObjRealRef(char* ptr, const ValueInfo& item) {
            if (item.inlined)
                return static_value_get_ref<ValueItem>(ptr, item.offset, 0, item.bit_offset);
            else
                return *static_value_get_ref<ValueItem*>(ptr, item.offset, 0, item.bit_offset);
        }

        ValueItem getAnyObjRef(char* ptr, const ValueInfo& item) {
            return ValueItem(getAnyObjRealRef(ptr, item), as_reference);
        }

        ValueItem _static_value_get_ref(char* ptr, const ValueInfo& item) {
            switch (item.type.vtype) {
            case VType::noting:
                return ValueItem();
            case VType::boolean:
                return ValueItem(static_value_get_ref<bool>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::i8:
                return ValueItem(static_value_get_ref<int8_t>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::i16:
                return ValueItem(static_value_get_ref<int16_t>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::i32:
                return ValueItem(static_value_get_ref<int32_t>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::i64:
                return ValueItem(static_value_get_ref<int64_t>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::ui8:
                return ValueItem(static_value_get_ref<uint8_t>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::ui16:
                return ValueItem(static_value_get_ref<uint16_t>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::ui32:
                return ValueItem(static_value_get_ref<uint32_t>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::ui64:
                return ValueItem(static_value_get_ref<uint64_t>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::flo:
                return ValueItem(static_value_get_ref<float>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::doub:
                return ValueItem(static_value_get_ref<double>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::raw_arr_ui8:
                return getRawArrayRef<uint8_t>(ptr, item);
            case VType::raw_arr_ui16:
                return getRawArrayRef<uint16_t>(ptr, item);
            case VType::raw_arr_ui32:
                return getRawArrayRef<uint32_t>(ptr, item);
            case VType::raw_arr_ui64:
                return getRawArrayRef<uint64_t>(ptr, item);
            case VType::raw_arr_i8:
                return getRawArrayRef<int8_t>(ptr, item);
            case VType::raw_arr_i16:
                return getRawArrayRef<int16_t>(ptr, item);
            case VType::raw_arr_i32:
                return getRawArrayRef<int32_t>(ptr, item);
            case VType::raw_arr_i64:
                return getRawArrayRef<int64_t>(ptr, item);
            case VType::raw_arr_flo:
                return getRawArrayRef<float>(ptr, item);
            case VType::raw_arr_doub:
                return getRawArrayRef<double>(ptr, item);
            case VType::faarr:
                return getRawArrayRef<ValueItem>(ptr, item);
            case VType::uarr:
                return getTypeRef<list_array<ValueItem>>(ptr, item);
            case VType::string:
                return getTypeRef<art::ustring>(ptr, item);
            case VType::undefined_ptr:
                return ValueItem(static_value_get_ref<void*>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::type_identifier:
                return ValueItem(static_value_get_ref<ValueMeta>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::map:
                return getTypeRef<std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>>(ptr, item);
            case VType::set:
                return getTypeRef<std::unordered_set<ValueItem, art::hash<ValueItem>>>(ptr, item);
            case VType::time_point:
                return ValueItem(static_value_get_ref<std::chrono::high_resolution_clock::time_point>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::any_obj:
                return ValueItem(static_value_get<ValueItem>(ptr, item.offset, item.bit_used, item.bit_offset));
            default:
                throw InvalidArguments("type not supported");
            }
        }

        ValueItem _static_value_get(char* ptr, const ValueInfo& item) {
            switch (item.type.vtype) {
            case VType::noting:
                return ValueItem();
            case VType::boolean:
                return ValueItem(static_value_get<bool>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::i8:
                return ValueItem(static_value_get<int8_t>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::i16:
                return ValueItem(static_value_get<int16_t>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::i32:
                return ValueItem(static_value_get<int32_t>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::i64:
                return ValueItem(static_value_get<int64_t>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::ui8:
                return ValueItem(static_value_get<uint8_t>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::ui16:
                return ValueItem(static_value_get<uint16_t>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::ui32:
                return ValueItem(static_value_get<uint32_t>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::ui64:
                return ValueItem(static_value_get<uint64_t>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::flo:
                return ValueItem(static_value_get<float>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::doub:
                return ValueItem(static_value_get<double>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::raw_arr_ui8:
                return getRawArray<uint8_t>(ptr, item);
            case VType::raw_arr_ui16:
                return getRawArray<uint16_t>(ptr, item);
            case VType::raw_arr_ui32:
                return getRawArray<uint32_t>(ptr, item);
            case VType::raw_arr_ui64:
                return getRawArray<uint64_t>(ptr, item);
            case VType::raw_arr_i8:
                return getRawArray<int8_t>(ptr, item);
            case VType::raw_arr_i16:
                return getRawArray<int16_t>(ptr, item);
            case VType::raw_arr_i32:
                return getRawArray<int32_t>(ptr, item);
            case VType::raw_arr_i64:
                return getRawArray<int64_t>(ptr, item);
            case VType::raw_arr_flo:
                return getRawArray<float>(ptr, item);
            case VType::raw_arr_doub:
                return getRawArray<double>(ptr, item);
            case VType::faarr:
                return getRawArray<ValueItem>(ptr, item);
            case VType::uarr:
                return getType<list_array<ValueItem>>(ptr, item);
            case VType::string:
                return getType<art::ustring>(ptr, item);
            case VType::undefined_ptr:
                return ValueItem(static_value_get<void*>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::type_identifier:
                return ValueItem(static_value_get<ValueMeta>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::map:
                return getType<std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>>(ptr, item);
            case VType::set:
                return getType<std::unordered_set<ValueItem, art::hash<ValueItem>>>(ptr, item);
            case VType::time_point:
                return ValueItem(static_value_get<std::chrono::high_resolution_clock::time_point>(ptr, item.offset, item.bit_used, item.bit_offset));
            case VType::any_obj:
                return getAnyObj(ptr, item);
            default:
                throw InvalidArguments("type not supported");
            }
        }

        void _static_value_set(char* ptr, const ValueInfo& item, ValueItem& set) {
            if (item.type.vtype != VType::any_obj)
                set.getAsync();
            if (item.type.use_gc) {
                ptr += item.offset;
                ptr += item.bit_offset / 8;
                ValueItem(ptr, item.type, as_reference) = set;
                return;
            }
            switch (item.type.vtype) {
            case VType::noting:
                return;
            case VType::boolean:
                static_value_set<bool>(ptr, item.offset, item.bit_used, item.bit_offset, (bool)set);
                return;
            case VType::i8:
                static_value_set<int8_t>(ptr, item.offset, item.bit_used, item.bit_offset, (int8_t)set);
                return;
            case VType::i16:
                static_value_set<int16_t>(ptr, item.offset, item.bit_used, item.bit_offset, (int16_t)set);
                return;
            case VType::i32:
                static_value_set<int32_t>(ptr, item.offset, item.bit_used, item.bit_offset, (int32_t)set);
                return;
            case VType::i64:
                static_value_set<int64_t>(ptr, item.offset, item.bit_used, item.bit_offset, (int64_t)set);
                return;
            case VType::ui8:
                static_value_set<uint8_t>(ptr, item.offset, item.bit_used, item.bit_offset, (uint8_t)set);
                return;
            case VType::ui16:
                static_value_set<uint16_t>(ptr, item.offset, item.bit_used, item.bit_offset, (uint16_t)set);
                return;
            case VType::ui32:
                static_value_set<uint32_t>(ptr, item.offset, item.bit_used, item.bit_offset, (uint32_t)set);
                return;
            case VType::ui64:
                static_value_set<uint64_t>(ptr, item.offset, item.bit_used, item.bit_offset, (uint64_t)set);
                return;
            case VType::flo:
                static_value_set<float>(ptr, item.offset, item.bit_used, item.bit_offset, (float)set);
                return;
            case VType::doub:
                static_value_set<double>(ptr, item.offset, item.bit_used, item.bit_offset, (double)set);
                return;
            case VType::undefined_ptr:
                static_value_set<void*>(ptr, item.offset, item.bit_used, item.bit_offset, (void*)set);
                return;
            case VType::type_identifier:
                static_value_set<ValueMeta>(ptr, item.offset, item.bit_used, item.bit_offset, (ValueMeta)set);
                return;
            case VType::time_point:
                static_value_set<std::chrono::high_resolution_clock::time_point>(ptr, item.offset, item.bit_used, item.bit_offset, (std::chrono::high_resolution_clock::time_point)set);
                return;
            case VType::any_obj:
                getAnyObjRealRef(ptr, item) = set;
                return;
            default:
                throw InvalidArguments("type not supported");
            }
        }

        void _static_value_set_ref(char* ptr, const ValueInfo& item, ValueItem& set) {
            if (item.type.vtype != VType::any_obj)
                set.getAsync();
            if (item.type.use_gc) {
                ptr += item.offset;
                ptr += item.bit_offset / 8;
                ValueItem(ptr, item.type, as_reference) = set;
                return;
            }
            switch (item.type.vtype) {
            case VType::noting:
                return;
            case VType::boolean:
                static_value_set_ref<bool>(ptr, item.offset, item.bit_used, item.bit_offset, (bool)set);
                return;
            case VType::i8:
                static_value_set_ref<int8_t>(ptr, item.offset, item.bit_used, item.bit_offset, (int8_t)set);
                return;
            case VType::i16:
                static_value_set_ref<int16_t>(ptr, item.offset, item.bit_used, item.bit_offset, (int16_t)set);
                return;
            case VType::i32:
                static_value_set_ref<int32_t>(ptr, item.offset, item.bit_used, item.bit_offset, (int32_t)set);
                return;
            case VType::i64:
                static_value_set_ref<int64_t>(ptr, item.offset, item.bit_used, item.bit_offset, (int64_t)set);
                return;
            case VType::ui8:
                static_value_set_ref<uint8_t>(ptr, item.offset, item.bit_used, item.bit_offset, (uint8_t)set);
                return;
            case VType::ui16:
                static_value_set_ref<uint16_t>(ptr, item.offset, item.bit_used, item.bit_offset, (uint16_t)set);
                return;
            case VType::ui32:
                static_value_set_ref<uint32_t>(ptr, item.offset, item.bit_used, item.bit_offset, (uint32_t)set);
                return;
            case VType::ui64:
                static_value_set_ref<uint64_t>(ptr, item.offset, item.bit_used, item.bit_offset, (uint64_t)set);
                return;
            case VType::flo:
                static_value_set_ref<float>(ptr, item.offset, item.bit_used, item.bit_offset, (float)set);
                return;
            case VType::doub:
                static_value_set_ref<double>(ptr, item.offset, item.bit_used, item.bit_offset, (double)set);
                return;
            case VType::undefined_ptr:
                static_value_set_ref<void*>(ptr, item.offset, item.bit_used, item.bit_offset, (void*)set);
                return;
            case VType::type_identifier:
                static_value_set_ref<ValueMeta>(ptr, item.offset, item.bit_used, item.bit_offset, (ValueMeta)set);
                return;
            case VType::time_point:
                static_value_set_ref<std::chrono::high_resolution_clock::time_point>(ptr, item.offset, item.bit_used, item.bit_offset, (std::chrono::high_resolution_clock::time_point)set);
                return;
            case VType::any_obj:
                getAnyObjRealRef(ptr, item) = set;
                return;
            default:
                throw InvalidArguments("type not supported");
            }
        }

        void cleanup_item(char* ptr, ValueInfo& item) {
            switch (item.type.vtype) {
            case VType::noting:
                break;
            case VType::boolean:
            case VType::i8:
            case VType::i16:
            case VType::i32:
            case VType::i64:
            case VType::ui8:
            case VType::ui16:
            case VType::ui32:
            case VType::ui64:
            case VType::flo:
            case VType::doub:
            case VType::undefined_ptr:
            case VType::type_identifier:
            case VType::time_point:
                if (item.zero_after_cleanup) {
                    ValueItem zero(0);
                    _static_value_set(ptr, item, zero);
                }
                break;
            case VType::raw_arr_ui8:
                if (!item.inlined)
                    delete[] static_value_get<uint8_t*>(ptr, item.offset, item.bit_used, item.bit_offset);
                break;
            case VType::raw_arr_ui16:
                if (!item.inlined)
                    delete[] static_value_get<uint16_t*>(ptr, item.offset, item.bit_used, item.bit_offset);
                break;
            case VType::raw_arr_ui32:
                if (!item.inlined)
                    delete[] static_value_get<uint32_t*>(ptr, item.offset, item.bit_used, item.bit_offset);
                break;
            case VType::raw_arr_ui64:
                if (!item.inlined)
                    delete[] static_value_get<uint64_t*>(ptr, item.offset, item.bit_used, item.bit_offset);
                break;
            case VType::raw_arr_i8:
                if (!item.inlined)
                    delete[] static_value_get<int8_t*>(ptr, item.offset, item.bit_used, item.bit_offset);
                break;
            case VType::raw_arr_i16:
                if (!item.inlined)
                    delete[] static_value_get<int16_t*>(ptr, item.offset, item.bit_used, item.bit_offset);
                break;
            case VType::raw_arr_i32:
                if (!item.inlined)
                    delete[] static_value_get<int32_t*>(ptr, item.offset, item.bit_used, item.bit_offset);
                break;
            case VType::raw_arr_i64:
                if (!item.inlined)
                    delete[] static_value_get<int64_t*>(ptr, item.offset, item.bit_used, item.bit_offset);
                break;
            case VType::raw_arr_flo:
                if (!item.inlined)
                    delete[] static_value_get<float*>(ptr, item.offset, item.bit_used, item.bit_offset);
                break;
            case VType::raw_arr_doub:
                if (!item.inlined)
                    delete[] static_value_get<double*>(ptr, item.offset, item.bit_used, item.bit_offset);
                break;
            case VType::faarr:
                if (!item.inlined)
                    delete[] static_value_get<ValueItem*>(ptr, item.offset, item.bit_used, item.bit_offset);
                else {
                    ValueItem* val = static_value_get<ValueItem*>(ptr, item.offset, item.bit_used, item.bit_offset);
                    for (uint32_t i = 0; i < item.type.val_len; i++)
                        val[i].~ValueItem();
                }
                break;
            case VType::uarr:
                if (!item.inlined)
                    delete static_value_get<list_array<ValueItem>*>(ptr, item.offset, item.bit_used, item.bit_offset);
                else {
                    static_value_get<list_array<ValueItem>*>(ptr, item.offset, item.bit_used, item.bit_offset)->~list_array<ValueItem>();
                }
                break;
            case VType::string:
                if (!item.inlined)
                    delete static_value_get<art::ustring*>(ptr, item.offset, item.bit_used, item.bit_offset);
                else {
                    static_value_get<art::ustring*>(ptr, item.offset, item.bit_used, item.bit_offset)->~ustring();
                }
                break;
            case VType::map:
                if (!item.inlined)
                    delete static_value_get<std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>*>(ptr, item.offset, item.bit_used, item.bit_offset);
                else {
                    static_value_get<std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>*>(ptr, item.offset, item.bit_used, item.bit_offset)->~unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>();
                }
                break;
            case VType::set:
                if (!item.inlined)
                    delete static_value_get<std::unordered_set<ValueItem, art::hash<ValueItem>>*>(ptr, item.offset, item.bit_used, item.bit_offset);
                else {
                    static_value_get<std::unordered_set<ValueItem, art::hash<ValueItem>>*>(ptr, item.offset, item.bit_used, item.bit_offset)->~unordered_set<ValueItem, art::hash<ValueItem>>();
                }
                break;
            case VType::any_obj:
                if (!item.inlined)
                    delete (ValueItem*)ptr;
                else {
                    ValueItem* val = (ValueItem*)ptr;
                    val->~ValueItem();
                }
                break;
            default:
                break;
            }
        }

        int8_t compare_items(char* ptr_a, const ValueInfo& item_a, char* ptr_b, const ValueInfo& item_b) {
            return _static_value_get(ptr_a, item_a).compare(_static_value_get(ptr_b, item_b));
        }

        void copy_items(char* ptr_a, char* ptr_b, ValueInfo* items, size_t items_size) {
            for (size_t i = 0; i < items_size; i++) {
                ValueInfo& item = items[i];
                if (item.type.vtype == VType::any_obj) {
                    ValueItem& val = getAnyObjRealRef(ptr_b, item);
                    getAnyObjRealRef(ptr_a, item) = val;
                } else {
                    ValueItem val = _static_value_get(ptr_b, item);
                    _static_value_set(ptr_a, item, val);
                }
            }
        }

        void copy_items_abstract(char* ptr_a, ValueInfo* items_a, size_t items_a_size, char* ptr_b, ValueInfo* items_b, size_t items_b_size) {
            for (size_t i = 0; i < items_a_size; i++) {
                ValueInfo& item_a = items_a[i];
                art::ustring& name = item_a.name;
                for (size_t j = 0; j < items_b_size; j++) {
                    ValueInfo& item_b = items_b[i];

                    if (item_b.name == name) {
                        if (item_a.type.vtype == VType::any_obj) {
                            ValueItem& val = getAnyObjRealRef(ptr_b, item_b);
                            getAnyObjRealRef(ptr_a, item_a) = val;
                        } else {
                            ValueItem val = _static_value_get(ptr_b, item_b);
                            _static_value_set(ptr_a, item_a, val);
                        }
                        break;
                    }
                }
            }
        }
    }

#pragma region ValueInfo

    ValueInfo::ValueInfo() {
        optional_tags = nullptr;
        offset = 0;
        type = ValueMeta();
        bit_used = 0;
        bit_offset = 0;
        inlined = false;
        allow_abstract_assign = false;
        zero_after_cleanup = false;
        access = ClassAccess::intern;
    }

    ValueInfo::ValueInfo(const art::ustring& name, size_t offset, ValueMeta type, uint16_t bit_used, uint8_t bit_offset, bool inlined, bool allow_abstract_assign, ClassAccess access, const list_array<ValueTag>& tags, bool zero_after_cleanup)
        : name(name), offset(offset), type(type), bit_used(bit_used), bit_offset(bit_offset), inlined(inlined), allow_abstract_assign(allow_abstract_assign), access(access), zero_after_cleanup(zero_after_cleanup) {
        if (!tags.empty()) {
            optional_tags = new list_array<ValueTag>(tags);
        } else {
            optional_tags = nullptr;
        }
    }

    ValueInfo::~ValueInfo() {
        if (optional_tags)
            delete optional_tags;
    }

    ValueInfo::ValueInfo(const ValueInfo& copy) {
        *this = copy;
    }

    ValueInfo::ValueInfo(ValueInfo&& move) {
        *this = std::move(move);
    }

    ValueInfo& ValueInfo::operator=(const ValueInfo& copy) {
        if (this == &copy)
            return *this;
        name = copy.name;
        offset = copy.offset;
        type = copy.type;
        bit_used = copy.bit_used;
        bit_offset = copy.bit_offset;
        inlined = copy.inlined;
        allow_abstract_assign = copy.allow_abstract_assign;
        access = copy.access;
        zero_after_cleanup = copy.zero_after_cleanup;
        if (optional_tags)
            delete optional_tags;
        optional_tags = copy.optional_tags ? new list_array<ValueTag>(*copy.optional_tags) : nullptr;
        return *this;
    }

    ValueInfo& ValueInfo::operator=(ValueInfo&& move) {
        if (this == &move)
            return *this;
        name = std::move(move.name);
        offset = move.offset;
        type = move.type;
        bit_used = move.bit_used;
        bit_offset = move.bit_offset;
        inlined = move.inlined;
        allow_abstract_assign = move.allow_abstract_assign;
        access = move.access;
        zero_after_cleanup = move.zero_after_cleanup;
        if (optional_tags)
            delete optional_tags;
        optional_tags = move.optional_tags;
        move.optional_tags = nullptr;
        return *this;
    }

#pragma endregion
#pragma region MethodInfo

    MethodInfo::MethodInfo()
        : ref(nullptr), name(), owner_name(), optional(nullptr), access(ClassAccess::pub), deletable(true) {}

    MethodInfo::MethodInfo(const art::ustring& name, Environment method, ClassAccess access, const list_array<ValueMeta>& return_values, const list_array<list_array<std::pair<ValueMeta, art::ustring>>>& arguments, const list_array<MethodTag>& tags, const art::ustring& owner_name)
        : name(name), ref(new FuncEnvironment(method, false)), owner_name(owner_name) {
        this->access = access;
        if (!return_values.empty() || !arguments.empty() || !tags.empty()) {
            this->optional = new Optional();
            this->optional->return_values = return_values;
            this->optional->arguments = arguments;
            this->optional->tags = tags;
        } else
            this->optional = nullptr;
        this->deletable = true;
    }

    MethodInfo::MethodInfo(const art::ustring& name, art::shared_ptr<FuncEnvironment> method, ClassAccess access, const list_array<ValueMeta>& return_values, const list_array<list_array<std::pair<ValueMeta, art::ustring>>>& arguments, const list_array<MethodTag>& tags, const art::ustring& owner_name)
        : name(name), ref(method), owner_name(owner_name) {
        this->access = access;
        if (!return_values.empty() || !arguments.empty() || !tags.empty()) {
            this->optional = new Optional();
            this->optional->return_values = return_values;
            this->optional->arguments = arguments;
            this->optional->tags = tags;
        } else
            this->optional = nullptr;
        this->deletable = true;
    }

    MethodInfo::~MethodInfo() {
        if (optional)
            delete optional;
    }

    MethodInfo::MethodInfo(const MethodInfo& other)
        : MethodInfo() {
        *this = other;
    }

    MethodInfo::MethodInfo(MethodInfo&& move)
        : MethodInfo() {
        *this = std::move(move);
    }

    MethodInfo& MethodInfo::operator=(const MethodInfo& copy) {
        if (this == &copy)
            return *this;
        ref = copy.ref;
        name = copy.name;
        owner_name = copy.owner_name;
        if (optional)
            delete (optional);
        optional = copy.optional ? new Optional(*copy.optional) : nullptr;
        access = copy.access;
        return *this;
    }

    MethodInfo& MethodInfo::operator=(MethodInfo&& move) {
        if (this == &move)
            return *this;
        ref = std::move(move.ref);
        name = std::move(move.name);
        owner_name = std::move(move.owner_name);
        if (optional)
            delete (optional);
        optional = move.optional;
        move.optional = nullptr;
        access = move.access;
        return *this;
    }

#pragma endregion
#pragma region AttachAVirtualTable

    AttachAVirtualTable::AttachAVirtualTable(list_array<MethodInfo>& methods, list_array<ValueInfo>& values, art::shared_ptr<FuncEnvironment> destructor, art::shared_ptr<FuncEnvironment> copy, art::shared_ptr<FuncEnvironment> move, art::shared_ptr<FuncEnvironment> compare, size_t structure_bytes, bool allow_auto_copy) {
        this->destructor = destructor ? (Environment)destructor->get_func_ptr() : nullptr;
        this->copy = copy ? (Environment)copy->get_func_ptr() : nullptr;
        this->move = move ? (Environment)move->get_func_ptr() : nullptr;
        this->compare = compare ? (Environment)compare->get_func_ptr() : nullptr;
        this->structure_bytes = structure_bytes;
        this->allow_auto_copy = allow_auto_copy;
        call_table_size = methods.size();
        Environment* table = getMethods(call_table_size);
        MethodInfo* table_additional_info = getMethodsInfo();
        for (uint64_t i = 0; i < call_table_size; i++) {
            table[i] = (Environment)(methods[i].ref ? methods[i].ref->get_func_ptr() : nullptr);
            new (table_additional_info + i) MethodInfo(methods[i]);
        }
        value_table_size = values.size();
        ValueInfo* values_table = getValuesInfo();
        for (uint64_t i = 0; i < value_table_size; i++)
            new (values_table + i) ValueInfo(values[i]);

        auto tmp = getAfterMethods();
        new (&tmp->destructor) art::shared_ptr<FuncEnvironment>(destructor);
        new (&tmp->copy) art::shared_ptr<FuncEnvironment>(copy);
        new (&tmp->move) art::shared_ptr<FuncEnvironment>(move);
        new (&tmp->compare) art::shared_ptr<FuncEnvironment>(compare);
        new (&tmp->name) art::ustring();
        tmp->tags = nullptr;
    }

    AttachAVirtualTable* AttachAVirtualTable::create(list_array<MethodInfo>& methods, list_array<ValueInfo>& values, art::shared_ptr<FuncEnvironment> destructor, art::shared_ptr<FuncEnvironment> copy, art::shared_ptr<FuncEnvironment> move, art::shared_ptr<FuncEnvironment> compare, size_t structure_bytes, bool allow_auto_copy) {
        size_t to_allocate =
            sizeof(AttachAVirtualTable)            //
            + sizeof(Environment) * methods.size() //
            + sizeof(MethodInfo) * methods.size()  //
            + sizeof(ValueInfo) * values.size()    //
            + sizeof(AfterMethods)                 //
            ;
        to_allocate = to_allocate + (8 - to_allocate % 8);
        AttachAVirtualTable* table = (AttachAVirtualTable*)malloc(to_allocate);
        if (!table)
            throw std::bad_alloc();
        new (table) AttachAVirtualTable(methods, values, destructor, copy, move, compare, structure_bytes, allow_auto_copy);
        return table;
    }

    void AttachAVirtualTable::destroy(AttachAVirtualTable* table) {
        table->~AttachAVirtualTable();
        free(table);
    }

    AttachAVirtualTable::~AttachAVirtualTable() {
        MethodInfo* table_additional_info = (MethodInfo*)(data + sizeof(Environment) * call_table_size);
        for (uint64_t i = 0; i < call_table_size; i++)
            table_additional_info[i].~MethodInfo();
        for (uint64_t i = 0; i < value_table_size; i++)
            getValuesInfo()[i].~ValueInfo();
        auto tmp = getAfterMethods();
        if (tmp->tags)
            delete tmp->tags;
        tmp->~AfterMethods();
    }

    list_array<StructureTag>* AttachAVirtualTable::getStructureTags() {
        return getAfterMethods()->tags;
    }

    list_array<MethodTag>* AttachAVirtualTable::getMethodTags(uint64_t index) {
        MethodInfo& info = getMethodInfo(index);
        if (info.optional == nullptr)
            return nullptr;
        return &info.optional->tags;
    }

    list_array<MethodTag>* AttachAVirtualTable::getMethodTags(const art::ustring& name, ClassAccess access) {
        MethodInfo& info = getMethodInfo(name, access);
        if (info.optional == nullptr)
            return nullptr;
        return &info.optional->tags;
    }

    list_array<list_array<std::pair<ValueMeta, art::ustring>>>* AttachAVirtualTable::getMethodArguments(uint64_t index) {
        MethodInfo& info = getMethodInfo(index);
        if (info.optional == nullptr)
            return nullptr;
        return &info.optional->arguments;
    }

    list_array<list_array<std::pair<ValueMeta, art::ustring>>>* AttachAVirtualTable::getMethodArguments(const art::ustring& name, ClassAccess access) {
        MethodInfo& info = getMethodInfo(name, access);
        if (info.optional == nullptr)
            return nullptr;
        return &info.optional->arguments;
    }

    list_array<ValueMeta>* AttachAVirtualTable::getMethodReturnValues(uint64_t index) {
        MethodInfo& info = getMethodInfo(index);
        if (info.optional == nullptr)
            return nullptr;
        return &info.optional->return_values;
    }

    list_array<ValueMeta>* AttachAVirtualTable::getMethodReturnValues(const art::ustring& name, ClassAccess access) {
        MethodInfo& info = getMethodInfo(name, access);
        if (info.optional == nullptr)
            return nullptr;
        return &info.optional->return_values;
    }

    MethodInfo* AttachAVirtualTable::getMethodsInfo(uint64_t& size) {
        size = call_table_size;
        return (MethodInfo*)(data + sizeof(Environment) * call_table_size);
    }

    const MethodInfo* AttachAVirtualTable::getMethodsInfo(uint64_t& size) const {
        size = call_table_size;
        return (MethodInfo*)(data + sizeof(Environment) * call_table_size);
    }

    MethodInfo& AttachAVirtualTable::getMethodInfo(uint64_t index) {
        if (index >= call_table_size)
            throw InvalidOperation("Index out of range");
        return getMethodsInfo(call_table_size)[index];
    }

    MethodInfo& AttachAVirtualTable::getMethodInfo(const art::ustring& name, ClassAccess access) {
        MethodInfo* table_additional_info = getMethodsInfo(call_table_size);
        for (uint64_t i = 0; i < call_table_size; i++) {
            if (table_additional_info[i].name == name && Structure::checkAccess(table_additional_info[i].access, access))
                return table_additional_info[i];
        }
        throw InvalidOperation("Method not found");
    }

    const MethodInfo& AttachAVirtualTable::getMethodInfo(uint64_t index) const {
        if (index >= call_table_size)
            throw InvalidOperation("Index out of range");
        uint64_t size;
        return getMethodsInfo(size)[index];
    }

    const MethodInfo& AttachAVirtualTable::getMethodInfo(const art::ustring& name, ClassAccess access) const {
        uint64_t size;
        const MethodInfo* table_additional_info = getMethodsInfo(size);
        for (uint64_t i = 0; i < size; i++) {
            if (table_additional_info[i].name == name && Structure::checkAccess(table_additional_info[i].access, access))
                return table_additional_info[i];
        }
        throw InvalidOperation("Method not found");
    }

    Environment* AttachAVirtualTable::getMethods(uint64_t& size) {
        size = call_table_size;
        return (Environment*)data;
    }

    Environment AttachAVirtualTable::getMethod(uint64_t index) const {
        if (index >= call_table_size)
            throw InvalidOperation("Index out of range");
        Environment* table = (Environment*)data;
        return table[index];
    }

    Environment AttachAVirtualTable::getMethod(const art::ustring& name, ClassAccess access) const {
        return (Environment)getMethodInfo(name, access).ref->get_func_ptr();
    }

    uint64_t AttachAVirtualTable::getMethodIndex(const art::ustring& name, ClassAccess access) const {
        uint64_t size;
        const MethodInfo* table_additional_info = getMethodsInfo(size);
        for (uint64_t i = 0; i < size; i++)
            if (table_additional_info[i].name == name && Structure::checkAccess(table_additional_info[i].access, access))
                return i;
        throw InvalidOperation("Method not found");
    }

    bool AttachAVirtualTable::hasMethod(const art::ustring& name, ClassAccess access) const {
        uint64_t size;
        const MethodInfo* table_additional_info = getMethodsInfo(size);
        for (uint64_t i = 0; i < size; i++)
            if (table_additional_info[i].name == name && Structure::checkAccess(table_additional_info[i].access, access))
                return true;
        return false;
    }

    art::ustring AttachAVirtualTable::getName() const {
        return getAfterMethods()->name;
    }

    void AttachAVirtualTable::setName(const art::ustring& name) {
        getAfterMethods()->name = name;
    }

    MethodInfo* AttachAVirtualTable::getMethodsInfo() const {
        return (MethodInfo*)(data + sizeof(Environment) * call_table_size);
    }

    ValueInfo* AttachAVirtualTable::getValuesInfo() const {
        return (ValueInfo*)(((char*)getMethodsInfo()) + sizeof(MethodInfo) * call_table_size);
    }

    AttachAVirtualTable::AfterMethods* AttachAVirtualTable::getAfterMethods() {
        return (AfterMethods*)(((char*)getValuesInfo()) + sizeof(ValueInfo) * value_table_size);
    }

    const AttachAVirtualTable::AfterMethods* AttachAVirtualTable::getAfterMethods() const {
        return (const AfterMethods*)(((char*)getValuesInfo()) + sizeof(ValueInfo) * value_table_size);
    }

#pragma region Values

    ValueInfo* AttachAVirtualTable::getValuesInfo(uint64_t& size) {
        size = value_table_size;
        return getValuesInfo();
    }

    ValueInfo& AttachAVirtualTable::getValueInfo(uint64_t index) {
        if (index >= value_table_size)
            throw InvalidOperation("Index out of range");
        return getValuesInfo()[index];
    }

    const ValueInfo& AttachAVirtualTable::getValueInfo(uint64_t index) const {
        if (index >= value_table_size)
            throw InvalidOperation("Index out of range");
        return getValuesInfo()[index];
    }

    ValueInfo& AttachAVirtualTable::getValueInfo(const art::ustring& name, ClassAccess access) {
        ValueInfo* table = getValuesInfo();
        for (uint64_t i = 0; i < value_table_size; i++)
            if (table[i].name == name && Structure::checkAccess(table[i].access, access))
                return table[i];
        throw InvalidOperation("Value not found");
    }

    const ValueInfo& AttachAVirtualTable::getValueInfo(const art::ustring& name, ClassAccess access) const {
        ValueInfo* table = getValuesInfo();
        for (uint64_t i = 0; i < value_table_size; i++)
            if (table[i].name == name && Structure::checkAccess(table[i].access, access))
                return table[i];
        throw InvalidOperation("Value not found");
    }

    list_array<ValueTag>* AttachAVirtualTable::getValueTags(uint64_t index) {
        return getValueInfo(index).optional_tags;
    }

    list_array<ValueTag>* AttachAVirtualTable::getValueTags(const art::ustring& name, ClassAccess access) {
        return getValueInfo(name, access).optional_tags;
    }

    ValueItem AttachAVirtualTable::getValue(void* self, uint64_t index) const {
        return structure_helpers::_static_value_get((char*)self, getValueInfo(index));
    }

    ValueItem AttachAVirtualTable::getValue(void* self, const art::ustring& name, ClassAccess access) const {
        return structure_helpers::_static_value_get((char*)self, getValueInfo(name, access));
    }

    ValueItem AttachAVirtualTable::getValueRef(void* self, uint64_t index) const {
        return structure_helpers::_static_value_get_ref((char*)self, getValueInfo(index));
    }

    ValueItem AttachAVirtualTable::getValueRef(void* self, const art::ustring& name, ClassAccess access) const {
        return structure_helpers::_static_value_get_ref((char*)self, getValueInfo(name, access));
    }

    void AttachAVirtualTable::setValue(void* self, uint64_t index, ValueItem& value) const {
        structure_helpers::_static_value_set((char*)self, getValueInfo(index), value);
    }

    void AttachAVirtualTable::setValue(void* self, const art::ustring& name, ClassAccess access, ValueItem& value) const {
        structure_helpers::_static_value_set((char*)self, getValueInfo(name, access), value);
    }

    bool AttachAVirtualTable::hasValue(const art::ustring& name, ClassAccess access) const {
        ValueInfo* table = getValuesInfo();
        for (uint64_t i = 0; i < value_table_size; i++)
            if (table[i].name == name && Structure::checkAccess(table[i].access, access))
                return true;
        return false;
    }

    uint64_t AttachAVirtualTable::getValueIndex(const art::ustring& name, ClassAccess access) const {
        ValueInfo* table = getValuesInfo();
        for (uint64_t i = 0; i < value_table_size; i++)
            if (table[i].name == name && Structure::checkAccess(table[i].access, access))
                return i;
        throw InvalidOperation("Value not found");
    }

#pragma endregion


#pragma endregion

#pragma region AttachADynamicVirtualTable

    AttachADynamicVirtualTable::AttachADynamicVirtualTable(list_array<MethodInfo>& methods, list_array<ValueInfo>& values, art::shared_ptr<FuncEnvironment> destructor, art::shared_ptr<FuncEnvironment> copy, art::shared_ptr<FuncEnvironment> move, art::shared_ptr<FuncEnvironment> compare, size_t structure_bytes, bool allow_auto_copy)
        : destructor(destructor), values(values), copy(copy), move(move), methods(methods), compare(compare), structure_bytes(structure_bytes), allow_auto_copy(allow_auto_copy) {
        tags = nullptr;
    }

    AttachADynamicVirtualTable::~AttachADynamicVirtualTable() {
        if (tags)
            delete tags;
    }

    AttachADynamicVirtualTable::AttachADynamicVirtualTable(const AttachADynamicVirtualTable& copy) {
        destructor = copy.destructor;
        move = copy.move;
        this->copy = copy.copy;
        methods = copy.methods;
        structure_bytes = copy.structure_bytes;
        allow_auto_copy = copy.allow_auto_copy;
        tags = copy.tags ? new list_array<StructureTag>(*copy.tags) : nullptr;
    }

    list_array<StructureTag>* AttachADynamicVirtualTable::getStructureTags() {
        return tags;
    }

    list_array<MethodTag>* AttachADynamicVirtualTable::getMethodTags(uint64_t index) {
        MethodInfo& info = getMethodInfo(index);
        if (info.optional == nullptr)
            return nullptr;
        return &info.optional->tags;
    }

    list_array<MethodTag>* AttachADynamicVirtualTable::getMethodTags(const art::ustring& name, ClassAccess access) {
        MethodInfo& info = getMethodInfo(name, access);
        if (info.optional == nullptr)
            return nullptr;
        return &info.optional->tags;
    }

    list_array<list_array<std::pair<ValueMeta, art::ustring>>>* AttachADynamicVirtualTable::getMethodArguments(uint64_t index) {
        MethodInfo& info = getMethodInfo(index);
        if (info.optional == nullptr)
            return nullptr;
        return &info.optional->arguments;
    }

    list_array<list_array<std::pair<ValueMeta, art::ustring>>>* AttachADynamicVirtualTable::getMethodArguments(const art::ustring& name, ClassAccess access) {
        MethodInfo& info = getMethodInfo(name, access);
        if (info.optional == nullptr)
            return nullptr;
        return &info.optional->arguments;
    }

    list_array<ValueMeta>* AttachADynamicVirtualTable::getMethodReturnValues(uint64_t index) {
        MethodInfo& info = getMethodInfo(index);
        if (info.optional == nullptr)
            return nullptr;
        return &info.optional->return_values;
    }

    list_array<ValueMeta>* AttachADynamicVirtualTable::getMethodReturnValues(const art::ustring& name, ClassAccess access) {
        MethodInfo& info = getMethodInfo(name, access);
        if (info.optional == nullptr)
            return nullptr;
        return &info.optional->return_values;
    }

    MethodInfo* AttachADynamicVirtualTable::getMethodsInfo(uint64_t& size) {
        size = methods.size();
        return methods.data();
    }

    MethodInfo& AttachADynamicVirtualTable::getMethodInfo(uint64_t index) {
        if (index >= methods.size())
            throw InvalidOperation("Index out of range");
        return methods[index];
    }

    MethodInfo& AttachADynamicVirtualTable::getMethodInfo(const art::ustring& name, ClassAccess access) {
        for (uint64_t i = 0; i < methods.size(); i++) {
            if (methods[i].name == name && Structure::checkAccess(methods[i].access, access))
                return methods[i];
        }
        throw InvalidOperation("Method not found");
    }

    const MethodInfo& AttachADynamicVirtualTable::getMethodInfo(uint64_t index) const {
        if (index >= methods.size())
            throw InvalidOperation("Index out of range");
        return methods[index];
    }

    const MethodInfo& AttachADynamicVirtualTable::getMethodInfo(const art::ustring& name, ClassAccess access) const {
        for (uint64_t i = 0; i < methods.size(); i++) {
            if (methods[i].name == name && Structure::checkAccess(methods[i].access, access))
                return methods[i];
        }
        throw InvalidOperation("Method not found");
    }

    Environment AttachADynamicVirtualTable::getMethod(uint64_t index) const {
        if (index >= methods.size())
            throw InvalidOperation("Index out of range");
        return (Environment)methods[index].ref->get_func_ptr();
    }

    Environment AttachADynamicVirtualTable::getMethod(const art::ustring& name, ClassAccess access) const {
        return (Environment)getMethodInfo(name, access).ref->get_func_ptr();
    }

    void AttachADynamicVirtualTable::addMethod(const art::ustring& name, Environment method, ClassAccess access, const list_array<ValueMeta>& return_values, const list_array<list_array<std::pair<ValueMeta, art::ustring>>>& arguments, const list_array<MethodTag>& tags, const art::ustring& owner_name) {
        methods.push_back(MethodInfo(name, method, access, return_values, arguments, tags, owner_name));
    }

    void AttachADynamicVirtualTable::addMethod(const art::ustring& name, const art::shared_ptr<FuncEnvironment>& method, ClassAccess access, const list_array<ValueMeta>& return_values, const list_array<list_array<std::pair<ValueMeta, art::ustring>>>& arguments, const list_array<MethodTag>& tags, const art::ustring& owner_name) {
        methods.push_back(MethodInfo(name, method, access, return_values, arguments, tags, owner_name));
    }

    void AttachADynamicVirtualTable::removeMethod(const art::ustring& name, ClassAccess access) {
        for (uint64_t i = 0; i < methods.size(); i++)
            if (methods[i].deletable)
                if (methods[i].name == name && Structure::checkAccess(methods[i].access, access)) {
                    methods.remove(i);
                    return;
                }
    }

    void AttachADynamicVirtualTable::deriveMethods(AttachADynamicVirtualTable& parent, bool as_friend) {
        for (MethodInfo& info : parent.methods) {
            size_t i;
            if (!as_friend)
                i = methods.find_it([&info](const MethodInfo& f_info) { return f_info.name == info.name && Structure::checkAccess(f_info.access, info.access); });
            else
                i = methods.find_it([&info](const MethodInfo& f_info) { return f_info.name == info.name; });
            if (i == list_array<MethodInfo>::npos)
                methods.push_back(info);
            else if (!methods[i].deletable)
                methods[i] = info;
            else
                throw InvalidOperation("Can not override method");
        }
    }

    void AttachADynamicVirtualTable::deriveMethods(AttachAVirtualTable& parent, bool as_friend) {
        size_t size = 0;
        MethodInfo* methods = parent.getMethodsInfo(size);
        for (size_t i = 0; i < size; i++) {
            size_t j;
            if (!as_friend)
                j = this->methods.find_it([&methods, i](const MethodInfo& info) { return info.name == methods[i].name && Structure::checkAccess(info.access, methods[i].access); });
            else
                j = this->methods.find_it([&methods, i](const MethodInfo& info) { return info.name == methods[i].name; });
            if (j == list_array<MethodInfo>::npos)
                this->methods.push_back(methods[i]);
            else if (!this->methods[j].deletable)
                this->methods[j] = methods[i];
            else
                throw InvalidOperation("Can not override method");
        }
    }

    void AttachADynamicVirtualTable::addTag(const art::ustring& name, const ValueItem& value) {
        if (tags == nullptr)
            tags = new list_array<StructureTag>();
        StructureTag tag;
        tag.name = name;
        tag.value = value;
        tags->push_back(tag);
    }

    void AttachADynamicVirtualTable::addTag(const art::ustring& name, ValueItem&& value) {
        if (tags == nullptr)
            tags = new list_array<StructureTag>(1);
        StructureTag tag;
        tag.name = name;
        tag.value = std::move(value);
        tags->push_back(tag);
    }

    void AttachADynamicVirtualTable::removeTag(const art::ustring& name) {
        if (tags == nullptr)
            return;
        for (uint64_t i = 0; i < tags->size(); i++)
            if ((*tags)[i].name == name) {
                tags->remove(i);
                return;
            }
    }

    uint64_t AttachADynamicVirtualTable::getMethodIndex(const art::ustring& name, ClassAccess access) const {
        for (uint64_t i = 0; i < methods.size(); i++)
            if (methods[i].name == name && Structure::checkAccess(methods[i].access, access))
                return i;
        throw InvalidOperation("Method not found");
    }

    bool AttachADynamicVirtualTable::hasMethod(const art::ustring& name, ClassAccess access) const {
        for (uint64_t i = 0; i < methods.size(); i++)
            if (methods[i].name == name && Structure::checkAccess(methods[i].access, access))
                return true;
        return false;
    }

    void AttachADynamicVirtualTable::derive(AttachADynamicVirtualTable& parent, bool as_friend) {
        deriveMethods(parent, as_friend);
        deriveValues(parent, as_friend);
    }

    void AttachADynamicVirtualTable::derive(AttachAVirtualTable& parent, bool as_friend) {
        deriveMethods(parent, as_friend);
        deriveValues(parent, as_friend);
    }

#pragma region Values

    ValueInfo* AttachADynamicVirtualTable::getValuesInfo(uint64_t& size) {
        size = values.size();
        return values.data();
    }

    ValueInfo& AttachADynamicVirtualTable::getValueInfo(uint64_t index) {
        if (index >= values.size())
            throw InvalidOperation("Index out of range");
        return values[index];
    }

    const ValueInfo& AttachADynamicVirtualTable::getValueInfo(uint64_t index) const {
        if (index >= values.size())
            throw InvalidOperation("Index out of range");
        return values[index];
    }

    ValueInfo& AttachADynamicVirtualTable::getValueInfo(const art::ustring& name, ClassAccess access) {
        for (uint64_t i = 0; i < values.size(); i++)
            if (values[i].name == name && Structure::checkAccess(values[i].access, access))
                return values[i];
        throw InvalidOperation("Value not found");
    }

    const ValueInfo& AttachADynamicVirtualTable::getValueInfo(const art::ustring& name, ClassAccess access) const {
        for (uint64_t i = 0; i < values.size(); i++)
            if (values[i].name == name && Structure::checkAccess(values[i].access, access))
                return values[i];
        throw InvalidOperation("Value not found");
    }

    list_array<ValueTag>* AttachADynamicVirtualTable::getValueTags(uint64_t index) {
        return getValueInfo(index).optional_tags;
    }

    list_array<ValueTag>* AttachADynamicVirtualTable::getValueTags(const art::ustring& name, ClassAccess access) {
        return getValueInfo(name, access).optional_tags;
    }

    ValueItem AttachADynamicVirtualTable::getValue(void* self, uint64_t index) const {
        return structure_helpers::_static_value_get((char*)self, getValueInfo(index));
    }

    ValueItem AttachADynamicVirtualTable::getValue(void* self, const art::ustring& name, ClassAccess access) const {
        return structure_helpers::_static_value_get((char*)self, getValueInfo(name, access));
    }

    ValueItem AttachADynamicVirtualTable::getValueRef(void* self, uint64_t index) const {
        return structure_helpers::_static_value_get_ref((char*)self, getValueInfo(index));
    }

    ValueItem AttachADynamicVirtualTable::getValueRef(void* self, const art::ustring& name, ClassAccess access) const {
        return structure_helpers::_static_value_get_ref((char*)self, getValueInfo(name, access));
    }

    void AttachADynamicVirtualTable::setValue(void* self, uint64_t index, ValueItem& value) const {
        structure_helpers::_static_value_set((char*)self, getValueInfo(index), value);
    }

    void AttachADynamicVirtualTable::setValue(void* self, const art::ustring& name, ClassAccess access, ValueItem& value) const {
        structure_helpers::_static_value_set((char*)self, getValueInfo(name, access), value);
    }

    void AttachADynamicVirtualTable::addValue(const ValueInfo& info) {
        values.push_back(info);
    }

    void AttachADynamicVirtualTable::removeValue(const art::ustring& name, ClassAccess access) {
        for (uint64_t i = 0; i < values.size(); i++)
            if (values[i].name == name && Structure::checkAccess(values[i].access, access)) {
                values.remove(i);
                return;
            }
    }

    bool AttachADynamicVirtualTable::hasValue(const art::ustring& name, ClassAccess access) const {
        for (uint64_t i = 0; i < values.size(); i++)
            if (values[i].name == name && Structure::checkAccess(values[i].access, access))
                return true;
        return false;
    }

    uint64_t AttachADynamicVirtualTable::getValueIndex(const art::ustring& name, ClassAccess access) const {
        for (uint64_t i = 0; i < values.size(); i++)
            if (values[i].name == name && Structure::checkAccess(values[i].access, access))
                return i;
        throw InvalidOperation("Value not found");
    }

    void AttachADynamicVirtualTable::deriveValues(AttachADynamicVirtualTable& parent, bool as_friend) {
        for (ValueInfo& value : parent.values) {
            size_t i;
            if (!as_friend)
                i = values.find_it([&value](const ValueInfo& f_value) { return f_value.name == value.name && Structure::checkAccess(f_value.access, value.access); });
            else
                i = values.find_it([&value](const ValueInfo& f_value) { return f_value.name == value.name; });

            if (i == list_array<ValueInfo>::npos)
                this->values.push_back(value);
            else
                throw InvalidOperation("Can not override value");
        }
    }

    void AttachADynamicVirtualTable::deriveValues(AttachAVirtualTable& parent, bool as_friend) {
        uint64_t total_values;
        auto values = parent.getValuesInfo(total_values);
        for (uint64_t i = 0; i < total_values; i++) {
            size_t j;
            if (!as_friend)
                j = this->values.find_it([&values, i](const ValueInfo& f_value) { return f_value.name == values[i].name && Structure::checkAccess(f_value.access, values[i].access); });
            else
                j = this->values.find_it([&values, i](const ValueInfo& f_value) { return f_value.name == values[i].name; });

            if (j == list_array<ValueInfo>::npos)
                this->values.push_back(values[i]);
            else
                throw InvalidOperation("Can not override value");
        }
    }

#pragma endregion

#pragma endregion

#pragma region Structure

    bool Structure::checkAccess(ClassAccess access, ClassAccess access_to_check) {
        if (access == ClassAccess::pub)
            return true;
        if (access == ClassAccess::prot)
            if (access_to_check != ClassAccess::pub)
                return true;
        if (access == ClassAccess::priv)
            if (access_to_check != ClassAccess::pub && access_to_check != ClassAccess::prot)
                return true;
        return access == access_to_check; //ClassAccess::intern
    }

    AttachAVirtualTable* Structure::createAAVTable(list_array<MethodInfo>& methods, list_array<ValueInfo>& values, art::shared_ptr<FuncEnvironment> destructor, art::shared_ptr<FuncEnvironment> copy, art::shared_ptr<FuncEnvironment> move, art::shared_ptr<FuncEnvironment> compare, const list_array<std::tuple<void*, VTableMode>>& derive_vtables, size_t structure_bytes, bool allow_auto_copy) {
        list_array<MethodInfo> methods_copy = methods;
        for (auto& table : derive_vtables) {
            switch (std::get<1>(table)) {
            case VTableMode::AttachADynamicVirtualTable:
                methods_copy.push_back(reinterpret_cast<AttachADynamicVirtualTable*>(std::get<0>(table))->methods);
                break;
            case VTableMode::AttachAVirtualTable: {
                uint64_t total_methods;
                auto methods = reinterpret_cast<AttachAVirtualTable*>(std::get<0>(table))->getMethodsInfo(total_methods);
                methods_copy.push_back(methods, total_methods);
                break;
            }
            default:
                throw NotImplementedException();
            }
        }
        return AttachAVirtualTable::create(methods_copy, values, destructor, copy, move, compare, structure_bytes, allow_auto_copy);
    }

    AttachADynamicVirtualTable* Structure::createAADVTable(list_array<MethodInfo>& methods, list_array<ValueInfo>& values, art::shared_ptr<FuncEnvironment> destructor, art::shared_ptr<FuncEnvironment> copy, art::shared_ptr<FuncEnvironment> move, art::shared_ptr<FuncEnvironment> compare, const list_array<std::tuple<void*, VTableMode>>& derive_vtables, size_t structure_bytes, bool allow_auto_copy) {
        AttachADynamicVirtualTable* vtable = new AttachADynamicVirtualTable(methods, values, destructor, copy, move, compare, structure_bytes, allow_auto_copy);
        for (auto& table : derive_vtables) {
            switch (std::get<1>(table)) {
            case VTableMode::AttachADynamicVirtualTable:
                vtable->derive(*reinterpret_cast<AttachADynamicVirtualTable*>(std::get<0>(table)));
                break;
            case VTableMode::AttachAVirtualTable:
                vtable->derive(*reinterpret_cast<AttachAVirtualTable*>(std::get<0>(table)));
                break;
            default:
                delete vtable;
                throw NotImplementedException();
            }
        }
        return vtable;
    }

    void Structure::destroyVTable(void* table, VTableMode mode) {
        switch (mode) {
        case VTableMode::AttachADynamicVirtualTable:
            delete reinterpret_cast<AttachADynamicVirtualTable*>(table);
            break;
        case VTableMode::AttachAVirtualTable:
            //freed by user
            break;
        default:
            throw NotImplementedException();
        }
    }

    Structure::Structure(void* self, void* vtable, VTableMode table_mode, void (*self_cleanup)(void* self))
        : self(self), vtable(vtable), vtable_mode(table_mode), self_cleanup(self_cleanup) {}

    Structure::~Structure() noexcept(false) {
        switch (vtable_mode) {
        case VTableMode::AttachAVirtualTable: {
            auto table = (AttachAVirtualTable*)vtable;
            if (table->destructor) {
                ValueItem item(this, as_reference);
                item.meta.as_ref = true;
                table->destructor(&item, 1);
            }
            size_t table_size = 0;
            ValueInfo* value_table = table->getValuesInfo(table_size);
            for (size_t i = 0; i < table_size; i++)
                structure_helpers::cleanup_item((char*)self, value_table[i]);
            break;
        }
        case VTableMode::AttachADynamicVirtualTable: {
            auto table = (AttachADynamicVirtualTable*)vtable;
            if (table->destructor) {
                ValueItem item(this, as_reference);
                item.meta.as_ref = true;
                art::CXX::cxxCall(table->destructor, item);
            }
            size_t table_size = 0;
            ValueInfo* value_table = table->getValuesInfo(table_size);
            for (size_t i = 0; i < table_size; i++)
                structure_helpers::cleanup_item((char*)self, value_table[i]);
            break;
        }
        default:
            break;
        }
    }

    ValueItem Structure::static_value_get(size_t value_data_index) const {
        switch (vtable_mode) {
        case VTableMode::AttachAVirtualTable:
            return ((AttachAVirtualTable*)vtable)->getValue((char*)this, value_data_index);
        case VTableMode::AttachADynamicVirtualTable:
            return ((AttachADynamicVirtualTable*)vtable)->getValue((char*)this, value_data_index);
        default:
            throw NotImplementedException();
        }
    }

    ValueItem Structure::static_value_get_ref(size_t value_data_index) {
        switch (vtable_mode) {
        case VTableMode::AttachAVirtualTable:
            return ((AttachAVirtualTable*)vtable)->getValueRef((char*)this, value_data_index);
        case VTableMode::AttachADynamicVirtualTable:
            return ((AttachADynamicVirtualTable*)vtable)->getValueRef((char*)this, value_data_index);
        default:
            throw NotImplementedException();
        }
    }

    void Structure::static_value_set(size_t value_data_index, ValueItem value) {
        switch (vtable_mode) {
        case VTableMode::AttachAVirtualTable:
            ((AttachAVirtualTable*)vtable)->setValue((char*)this, value_data_index, value);
            return;
        case VTableMode::AttachADynamicVirtualTable:
            ((AttachADynamicVirtualTable*)vtable)->setValue((char*)this, value_data_index, value);
            return;
        default:
            throw NotImplementedException();
        }
    }

    ValueItem Structure::dynamic_value_get(const art::ustring& name, ClassAccess access) const {

        switch (vtable_mode) {
        case VTableMode::AttachAVirtualTable:
            return ((AttachAVirtualTable*)vtable)->getValue((char*)this, name, access);
        case VTableMode::AttachADynamicVirtualTable:
            return ((AttachADynamicVirtualTable*)vtable)->getValue((char*)this, name, access);
        default:
            throw NotImplementedException();
        }
    }

    ValueItem Structure::dynamic_value_get_ref(const art::ustring& name, ClassAccess access) {
        switch (vtable_mode) {
        case VTableMode::AttachAVirtualTable:
            return ((AttachAVirtualTable*)vtable)->getValueRef((char*)this, name, access);
        case VTableMode::AttachADynamicVirtualTable:
            return ((AttachADynamicVirtualTable*)vtable)->getValueRef((char*)this, name, access);
        default:
            throw NotImplementedException();
        }
    }

    void Structure::dynamic_value_set(const art::ustring& name, ClassAccess access, ValueItem value) {
        switch (vtable_mode) {
        case VTableMode::AttachAVirtualTable:
            ((AttachAVirtualTable*)vtable)->setValue((char*)this, name, access, value);
            return;
        case VTableMode::AttachADynamicVirtualTable:
            ((AttachADynamicVirtualTable*)vtable)->setValue((char*)this, name, access, value);
            return;
        default:
            throw NotImplementedException();
        }
    }

    uint64_t Structure::table_get_id(const art::ustring& name, ClassAccess access) const {
        switch (vtable_mode) {
        case VTableMode::AttachAVirtualTable:
            return (*(const AttachAVirtualTable* const*)vtable)->getMethodIndex(name, access);
        case VTableMode::AttachADynamicVirtualTable:
            return (*(const AttachADynamicVirtualTable* const*)vtable)->getMethodIndex(name, access);
        default:
            throw NotImplementedException();
        }
    }

    Environment Structure::table_get(uint64_t fn_id) const {
        switch (vtable_mode) {
        case VTableMode::AttachAVirtualTable:
            return ((AttachAVirtualTable*)vtable)->getMethod(fn_id);
        case VTableMode::AttachADynamicVirtualTable:
            return ((AttachADynamicVirtualTable*)vtable)->getMethod(fn_id);
        default:
            throw NotImplementedException();
        }
    }

    Environment Structure::table_get_dynamic(const art::ustring& name, ClassAccess access) const {
        switch (vtable_mode) {
        case VTableMode::AttachAVirtualTable:
            return ((AttachAVirtualTable*)vtable)->getMethod(name, access);
        case VTableMode::AttachADynamicVirtualTable:
            return ((AttachADynamicVirtualTable*)vtable)->getMethod(name, access);
        default:
            throw NotImplementedException();
        }
    }

    void Structure::add_method(const art::ustring& name, Environment method, ClassAccess access, const list_array<ValueMeta>& return_values, const list_array<list_array<std::pair<ValueMeta, art::ustring>>>& arguments, const list_array<MethodTag>& tags, const art::ustring& owner_name) {
        if (vtable_mode != VTableMode::AttachADynamicVirtualTable)
            throw InvalidOperation("vtable must be dynamic to add new method");
        ((AttachADynamicVirtualTable*)vtable)->addMethod(name, method, access, return_values, arguments, tags, owner_name);
    }

    void Structure::add_method(const art::ustring& name, const art::shared_ptr<FuncEnvironment>& method, ClassAccess access, const list_array<ValueMeta>& return_values, const list_array<list_array<std::pair<ValueMeta, art::ustring>>>& arguments, const list_array<MethodTag>& tags, const art::ustring& owner_name) {
        if (vtable_mode != VTableMode::AttachADynamicVirtualTable)
            throw InvalidOperation("vtable must be dynamic to add new method");
        ((AttachADynamicVirtualTable*)vtable)->addMethod(name, method, access, return_values, arguments, tags, owner_name);
    }

    bool Structure::has_method(const art::ustring& name, ClassAccess access) const {
        if (vtable_mode == VTableMode::AttachAVirtualTable)
            return ((AttachAVirtualTable*)vtable)->hasMethod(name, access);
        else if (vtable_mode == VTableMode::AttachADynamicVirtualTable)
            return ((AttachADynamicVirtualTable*)vtable)->hasMethod(name, access);
        else
            throw NotImplementedException();
    }

    void Structure::remove_method(const art::ustring& name, ClassAccess access) {
        if (vtable_mode != VTableMode::AttachADynamicVirtualTable)
            throw InvalidOperation("vtable must be dynamic to remove method");
        ((AttachADynamicVirtualTable*)vtable)->removeMethod(name, access);
    }

    art::shared_ptr<FuncEnvironment> Structure::get_method(uint64_t fn_id) const {
        switch (vtable_mode) {
        case VTableMode::AttachAVirtualTable:
            return ((AttachAVirtualTable*)vtable)->getMethodInfo(fn_id).ref;
        case VTableMode::AttachADynamicVirtualTable:
            return ((AttachADynamicVirtualTable*)vtable)->getMethodInfo(fn_id).ref;
        default:
            throw NotImplementedException();
        }
    }

    art::shared_ptr<FuncEnvironment> Structure::get_method_dynamic(const art::ustring& name, ClassAccess access) const {
        switch (vtable_mode) {
        case VTableMode::AttachAVirtualTable:
            return ((AttachAVirtualTable*)vtable)->getMethodInfo(name, access).ref;
        case VTableMode::AttachADynamicVirtualTable:
            return ((AttachADynamicVirtualTable*)vtable)->getMethodInfo(name, access).ref;
        default:
            throw NotImplementedException();
        }
    }

    void Structure::table_derive(void* vtable, Structure::VTableMode vtable_mode) {
        if (this->vtable_mode != VTableMode::AttachADynamicVirtualTable)
            throw InvalidOperation("vtable must be dynamic to derive");
        if (vtable_mode == VTableMode::AttachAVirtualTable)
            ((AttachADynamicVirtualTable*)this->vtable)->derive(*(AttachAVirtualTable*)vtable);
        else if (vtable_mode == VTableMode::AttachADynamicVirtualTable)
            ((AttachADynamicVirtualTable*)this->vtable)->derive(*(AttachADynamicVirtualTable*)vtable);
        else
            throw NotImplementedException();
    }

    void Structure::change_table(void* vtable, Structure::VTableMode vtable_mode) {
        if (this->vtable_mode == VTableMode::AttachADynamicVirtualTable)
            Structure::destroyVTable(this->vtable, VTableMode::AttachADynamicVirtualTable);
        this->vtable = vtable;
        this->vtable_mode = vtable_mode;
    }

    void Structure::destruct(Structure* structure) {
        switch (structure->vtable_mode) {
        case VTableMode::AttachAVirtualTable:
            if (((AttachAVirtualTable*)structure->vtable)->destructor) {
                ValueItem item(structure, as_reference);
                ((AttachAVirtualTable*)structure->vtable)->destructor(&item, 1);
                size_t table_size = 0;
                ValueInfo* value_table = ((AttachAVirtualTable*)structure->vtable)->getValuesInfo(table_size);
                for (size_t i = 0; i < table_size; i++)
                    structure_helpers::cleanup_item((char*)structure->self, value_table[i]);
            }
            break;
        case VTableMode::AttachADynamicVirtualTable:
            if (((AttachADynamicVirtualTable*)structure->vtable)->destructor) {
                ValueItem item(structure, as_reference);
                art::CXX::cxxCall(((AttachADynamicVirtualTable*)structure->vtable)->destructor, item);
                size_t table_size = 0;
                ValueInfo* value_table = ((AttachADynamicVirtualTable*)structure->vtable)->getValuesInfo(table_size);
                for (size_t i = 0; i < table_size; i++)
                    structure_helpers::cleanup_item((char*)structure->self, value_table[i]);
            }
            break;
        default:
            break;
        }
        if (structure->self_cleanup)
            structure->self_cleanup(structure->self);
    }

    void Structure::copy(Structure* dst, Structure* src, bool at_construct) {
        void* vtable = dst->vtable;
        switch (dst->vtable_mode) {
        case VTableMode::AttachAVirtualTable: {
            auto table_dst = reinterpret_cast<AttachAVirtualTable*>(vtable);
            if (table_dst->copy) {
                ValueItem _dst(dst, no_copy);
                _dst.meta.as_ref = true;
                ValueItem _src(src, no_copy);
                _src.meta.as_ref = true;
                ValueItem args = {_dst, _src, at_construct};
                table_dst->copy(&args, 3);
            } else if (table_dst->allow_auto_copy) {
                size_t dst_table_size = 0;
                ValueInfo* dst_value_table = table_dst->getValuesInfo(dst_table_size);
                if (dst->vtable == src->vtable) {
                    structure_helpers::copy_items((char*)dst->self, (char*)src->self, dst_value_table, dst_table_size);
                } else if (src->vtable_mode == VTableMode::AttachAVirtualTable) {
                    auto table_src = reinterpret_cast<AttachAVirtualTable*>(src->vtable);
                    size_t src_table_size = 0;
                    ValueInfo* src_value_table = table_src->getValuesInfo(src_table_size);
                    structure_helpers::copy_items_abstract((char*)dst->self, dst_value_table, dst_table_size, (char*)src->self, src_value_table, src_table_size);
                } else if (src->vtable_mode == VTableMode::AttachADynamicVirtualTable) {
                    auto table_src = reinterpret_cast<AttachADynamicVirtualTable*>(src->vtable);
                    size_t src_table_size = 0;
                    ValueInfo* src_value_table = table_src->getValuesInfo(src_table_size);
                    structure_helpers::copy_items_abstract((char*)dst->self, dst_value_table, dst_table_size, (char*)src->self, src_value_table, src_table_size);
                } else
                    throw NotImplementedException();
            } else
                throw NotImplementedException();
            break;
        }
        case VTableMode::AttachADynamicVirtualTable: {
            auto table_dst = reinterpret_cast<AttachADynamicVirtualTable*>(vtable);
            if (table_dst->copy) {
                ValueItem _dst(dst, no_copy);
                _dst.meta.as_ref = true;
                ValueItem _src(src, no_copy);
                _src.meta.as_ref = true;
                art::CXX::cxxCall(table_dst->copy, _dst, _src, at_construct);
            } else if (table_dst->allow_auto_copy) {
                size_t dst_table_size = 0;
                ValueInfo* dst_value_table = table_dst->getValuesInfo(dst_table_size);
                if (dst->vtable == src->vtable) {
                    structure_helpers::copy_items((char*)dst->self, (char*)src->self, dst_value_table, dst_table_size);
                } else if (src->vtable_mode == VTableMode::AttachAVirtualTable) {
                    auto table_src = reinterpret_cast<AttachAVirtualTable*>(src->vtable);
                    size_t src_table_size = 0;
                    ValueInfo* src_value_table = table_src->getValuesInfo(src_table_size);
                    structure_helpers::copy_items_abstract((char*)dst->self, dst_value_table, dst_table_size, (char*)src->self, src_value_table, src_table_size);
                } else if (src->vtable_mode == VTableMode::AttachADynamicVirtualTable) {
                    auto table_src = reinterpret_cast<AttachADynamicVirtualTable*>(src->vtable);
                    size_t src_table_size = 0;
                    ValueInfo* src_value_table = table_src->getValuesInfo(src_table_size);
                    structure_helpers::copy_items_abstract((char*)dst->self, dst_value_table, dst_table_size, (char*)src->self, src_value_table, src_table_size);
                } else
                    throw NotImplementedException();
            } else
                throw NotImplementedException();
            break;
        }
        default:
            throw NotImplementedException();
        }
    }

    struct __Structure_destruct {
        void operator()(Structure* structure) {
            Structure::destruct(structure);
        }
    };

    Structure* Structure::copy(Structure* src) {
        void* vtable = src->vtable;
        size_t _count;
        Structure* dst;
        switch (src->vtable_mode) {
        case VTableMode::AttachADynamicVirtualTable: {
            auto* table = reinterpret_cast<AttachADynamicVirtualTable*>(vtable);
            dst = new Structure(malloc(table->structure_bytes), new AttachADynamicVirtualTable(*table), VTableMode::AttachADynamicVirtualTable, free);
            break;
        }
        case VTableMode::AttachAVirtualTable: {
            auto* table = reinterpret_cast<AttachAVirtualTable*>(vtable);
            dst = new Structure(malloc(table->structure_bytes), new AttachAVirtualTable(*table), VTableMode::AttachAVirtualTable, free);
            break;
        }
        default:
            throw NotImplementedException();
        }
        std::unique_ptr<Structure, __Structure_destruct> dst_ptr(dst);
        switch (dst->vtable_mode) {
        case VTableMode::AttachAVirtualTable: {
            auto* table = reinterpret_cast<AttachAVirtualTable*>(vtable);
            if (table->copy) {
                art::CXX::cxxCall(table->copy, ValueItem(dst, as_reference), ValueItem(src, as_reference), true);
            } else if (table->allow_auto_copy) {
                size_t table_size = 0;
                ValueInfo* value_table = table->getValuesInfo(table_size);
                structure_helpers::copy_items((char*)dst, (char*)src, value_table, table_size);
            } else
                throw NotImplementedException();
            break;
        }
        case VTableMode::AttachADynamicVirtualTable: {
            auto* table = reinterpret_cast<AttachADynamicVirtualTable*>(vtable);
            if (table->copy) {
                art::CXX::cxxCall(table->copy, ValueItem(dst, as_reference), ValueItem(src, as_reference), true);
            } else if (table->allow_auto_copy) {
                size_t table_size = 0;
                ValueInfo* value_table = table->getValuesInfo(table_size);
                structure_helpers::copy_items((char*)dst, (char*)src, value_table, table_size);
            } else
                throw NotImplementedException();
            break;
        }
        default:
            throw NotImplementedException();
        }
        return dst_ptr.release();
    }

    void Structure::move(Structure* dst, Structure* src, bool at_construct) {
        void* vtable = dst->vtable;
        switch (dst->vtable_mode) {
        case VTableMode::AttachAVirtualTable:
            if (reinterpret_cast<AttachAVirtualTable*>(vtable)->move) {
                ValueItem _dst(dst, no_copy);
                _dst.meta.as_ref = true;
                ValueItem _src(src, no_copy);
                _src.meta.as_ref = true;
                ValueItem args = {_dst, _src, at_construct};
                auto res = reinterpret_cast<AttachAVirtualTable*>(vtable)->move((ValueItem*)args.getSourcePtr(), 3);
                if (res)
                    delete res;
            } else
                throw NotImplementedException();
            break;
        case VTableMode::AttachADynamicVirtualTable:
            if (reinterpret_cast<AttachADynamicVirtualTable*>(vtable)->move) {
                ValueItem _dst(dst, no_copy);
                _dst.meta.as_ref = true;
                ValueItem _src(src, no_copy);
                _src.meta.as_ref = true;
                art::CXX::cxxCall(reinterpret_cast<AttachADynamicVirtualTable*>(vtable)->move, _dst, _src, at_construct);
            } else
                throw NotImplementedException();
            break;
        default:
            throw NotImplementedException();
        }
    }

    int8_t Structure::compare(Structure* a, Structure* b) {
        if (a == b)
            return 0;
        void* vtable = a->vtable;
        switch (a->vtable_mode) {
        case VTableMode::AttachAVirtualTable:
            if (reinterpret_cast<AttachAVirtualTable*>(vtable)->compare) {
                ValueItem _a(a, no_copy);
                _a.meta.as_ref = true;
                ValueItem _b(b, no_copy);
                _b.meta.as_ref = true;
                ValueItem args = {_a, _b};
                ValueItem* res = reinterpret_cast<AttachAVirtualTable*>(vtable)->compare((ValueItem*)args.getSourcePtr(), 2);
                if (res) {
                    int8_t ret = (int8_t)*res;
                    delete res;
                    return ret;
                }
                return 0;
            } else
                throw NotImplementedException();
        case VTableMode::AttachADynamicVirtualTable:
            if (reinterpret_cast<AttachADynamicVirtualTable*>(vtable)->compare) {
                ValueItem _a(a, no_copy);
                _a.meta.as_ref = true;
                ValueItem _b(b, no_copy);
                _b.meta.as_ref = true;
                return (int8_t)art::CXX::cxxCall(reinterpret_cast<AttachADynamicVirtualTable*>(vtable)->compare, _a, _b);
            } else
                throw NotImplementedException();
        default:
            throw NotImplementedException();
        }
    }

    int8_t Structure::compare_reference(Structure* a, Structure* b) {
        ptrdiff_t diff = (char*)a - (char*)b;
        return diff < 0 ? -1 : diff > 0 ? 1
                                        : 0;
    }

    int8_t Structure::compare_object(Structure* a, Structure* b) {
        if (a == b)
            return 0;
        size_t a_info_size;
        ValueInfo* a_info;

        size_t b_info_size;
        ValueInfo* b_info;

        switch (a->vtable_mode) {
        case VTableMode::AttachAVirtualTable:
            a_info = ((AttachAVirtualTable*)a->vtable)->getValuesInfo(a_info_size);
            break;
        case VTableMode::AttachADynamicVirtualTable:
            a_info = ((AttachADynamicVirtualTable*)a->vtable)->getValuesInfo(a_info_size);
            break;
        default:
            return -1;
        }

        switch (b->vtable_mode) {
        case VTableMode::AttachAVirtualTable:
            b_info = ((AttachAVirtualTable*)b->vtable)->getValuesInfo(b_info_size);
            break;
        case VTableMode::AttachADynamicVirtualTable:
            b_info = ((AttachADynamicVirtualTable*)b->vtable)->getValuesInfo(b_info_size);
            break;
        default:
            return -1;
        }

        if (a_info_size != b_info_size)
            return a_info_size < b_info_size ? -1 : 1;

        for (size_t i = 0; i < a_info_size; i++) {
            int8_t res = structure_helpers::compare_items((char*)a->self, a_info[i], (char*)b->self, b_info[i]);
            if (res != 0)
                return res;
        }
        return 0;
    }

    int8_t Structure::compare_full(Structure* a, Structure* b) {
        int8_t res = compare(a, b);
        if (res != 0)
            return res;
        return compare_object(a, b);
    }

    art::ustring Structure::get_name() const {
        switch (vtable_mode) {
        case VTableMode::AttachAVirtualTable:
            return reinterpret_cast<const AttachAVirtualTable*>(vtable)->getName();
        case VTableMode::AttachADynamicVirtualTable:
            return reinterpret_cast<const AttachADynamicVirtualTable*>(vtable)->name;
        default:
            throw NotImplementedException();
        }
    }

#pragma endregion
}