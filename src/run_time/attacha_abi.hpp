// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#ifndef SRC_RUN_TIME_ATTACHA_ABI
#define SRC_RUN_TIME_ATTACHA_ABI
    #include <base/run_time.hpp>
    #include <run_time/AttachA_CXX_struct.hpp>
    #include <run_time/attacha_abi_structs.hpp>
    #include <util/cxxException.hpp>
    #include <util/link_garbage_remover.hpp>
    #include <util/string_help.hpp>
    #include <util/threading.hpp>
    #include <util/ustring.hpp>

namespace art {
    constexpr bool needAllocType(VType type) {
        switch (type) {
        case VType::raw_arr_i8:
        case VType::raw_arr_i16:
        case VType::raw_arr_i32:
        case VType::raw_arr_i64:
        case VType::raw_arr_ui8:
        case VType::raw_arr_ui16:
        case VType::raw_arr_ui32:
        case VType::raw_arr_ui64:
        case VType::raw_arr_flo:
        case VType::raw_arr_doub:
        case VType::uarr:
        case VType::string:
        case VType::async_res:
        case VType::except_value:
        case VType::faarr:
        case VType::function:
        case VType::struct_:
        case VType::map:
        case VType::set:
        case VType::generator:
            return true;
        default:
            return false;
        }
    }

    constexpr bool needAlloc(ValueMeta type) {
        if (type.as_ref)
            return false;
        if (type.use_gc)
            return true;
        return needAllocType(type.vtype);
    }

    bool calc_safe_depth_arr(void* ptr);

    template <class T>
    void defaultDestructor(void* a) {
        delete (T*)a;
    }

    template <class T>
    void arrayDestructor(void* a) {
        delete[] (T*)a;
    }

    template <class T>
    void Allocate(void** a) {
        *a = new T();
    }

    void universalRemove(void** value);
    void universalAlloc(void** value, ValueMeta meta);

    ValueItem* getAsyncValueItem(void* val);
    void getValueItem(void** value, ValueItem* f_res);
    ValueItem* buildRes(void** value);
    ValueItem* buildResTake(void** value);


    void getAsyncResult(void*& value, ValueMeta& meta);
    void* copyValue(void*& val, ValueMeta& meta);


    void*& getValue(void*& value, ValueMeta& meta);
    const void* const& getValue(const void* const& value, const ValueMeta& meta);
    void*& getValue(void** value);
    void* getSpecificValue(void** value, VType typ);
    void** getSpecificValueLink(void** value, VType typ);

    bool is_integer(VType typ);
    bool integer_unsigned(VType typ);
    bool integer_floating(VType typ);
    bool is_raw_array(VType typ);

    //return equal,lower bool result
    std::pair<bool, bool> compareValue(ValueMeta cmp1, ValueMeta cmp2, void* val1, void* val2);
    RFLAGS compare(RFLAGS old, void** value_1, void** value_2);
    RFLAGS link_compare(RFLAGS old, void** value_1, void** value_2);

    namespace ABI_IMPL {
        ValueItem* _Vcast_callFN(const void* ptr);

        template <class T, class A, std::is_convertible<A, T>::value>
        T* AsPointer(void* val) {
            return new T[]{(T)(A)val};
        }

        template <class T, class A>
        T* AsPointer(void* val) {
            throw InvalidCast("Try convert unconvertible types");
        }

        art::ustring Scast(void*& val, ValueMeta& meta);
        art::ustring Scast(const void* const& val, const ValueMeta& meta);
        ValueItem SBcast(const art::ustring& str);

        template <class T>
        ValueItem BVcast(T&& val) {
            static constexpr auto val_meta_ = ValueMeta::from_type<T>();
            if constexpr (val_meta_.vtype == VType::noting && !std::is_same_v<std::remove_cvref_t<T>, void>) {
                if constexpr (std::is_same_v<std::remove_cvref_t<T>, std::nullptr_t>) {
                    return ValueItem();
                } else if constexpr (std::is_convertible_v<T, std::string_view>) {
                    return ValueItem(new art::ustring(val), VType::string, no_copy);
                } else if constexpr (std::is_same_v<std::remove_cvref_t<T>, Environment>) {
                    return ValueItem(new art::shared_ptr<FuncEnvironment>(new FuncEnvironment(val, false)), VType::function, no_copy);
                } else if constexpr (std::is_same_v<std::remove_cvref_t<T>, ValueItem>) {
                    if constexpr (std::is_reference_v<T>)
                        return ValueItem(val, art::as_reference);
                    else
                        return val;
                } else if constexpr (std::is_same_v<std::remove_cvref_t<T>, art::thread::id>) {
                    return (size_t)val;
                } else if constexpr (std::is_aggregate_v<std::remove_cvref_t<T>> || std::is_class_v<std::remove_cvref_t<T>>) {
                    using un_ref_ = std::remove_const_t<std::remove_reference_t<T>>;
                    if (CXX::Interface::typeVTableReadOnly<un_ref_>() != nullptr) {
                        if constexpr (std::is_copy_constructible_v<T> && !std::is_reference_v<T>)
                            return ValueItem(new Structure(new T(val), CXX::Interface::typeVTableReadOnly<un_ref_>().vtable, CXX::Interface::typeVTableReadOnly<un_ref_>().mode, defaultDestructor<un_ref_>), no_copy);
                        else
                            return ValueItem(new Structure(&val, CXX::Interface::typeVTableReadOnly<un_ref_>().vtable, CXX::Interface::typeVTableReadOnly<un_ref_>().mode, nullptr), no_copy);
                    } else
                        throw InvalidArguments("This type is not has been registered");
                    throw NotImplementedException();
                } else if constexpr (std::is_enum_v<T>) {
                    if constexpr (!std::is_reference_v<T>) {
                        if constexpr (sizeof(T) == 1)
                            return (uint8_t)val;
                        else if constexpr (sizeof(T) == 2)
                            return (uint16_t)val;
                        else if constexpr (sizeof(T) == 4)
                            return (uint32_t)val;
                        else
                            return (uint64_t)val;
                    } else {
                        if constexpr (sizeof(T) == 1)
                            return ValueItem(&val, ValueMeta(VType::ui8, false, !std::is_const_v<T>, 0, true));
                        else if constexpr (sizeof(T) == 2)
                            return ValueItem(&val, ValueMeta(VType::ui16, false, !std::is_const_v<T>, 0, true));
                        else if constexpr (sizeof(T) == 4)
                            return ValueItem(&val, ValueMeta(VType::ui32, false, !std::is_const_v<T>, 0, true));
                        else
                            return ValueItem(&val, ValueMeta(VType::ui64, false, !std::is_const_v<T>, 0, true));
                    }
                } else {
                    static_assert(
                        (
                            (val_meta_.vtype == VType::noting && !std::is_same_v<std::remove_cvref_t<T>, void>) &&
                            (std::is_same_v<std::remove_cv_t<T>, char*> ||
                             std::is_same_v<std::remove_cvref_t<T>, std::string> ||
                             std::is_same_v<std::remove_cvref_t<T>, ValueItem> ||
                             std::is_same_v<std::remove_cvref_t<T>, std::nullptr_t> ||
                             std::is_same_v<std::remove_cvref_t<T>, Environment> ||
                             std::is_same_v<std::remove_cvref_t<T>, art::thread::id> ||
                             (std::is_aggregate_v<std::remove_cvref_t<T>> || std::is_class_v<std::remove_cvref_t<T>>) ||
                             std::is_enum_v<T>)
                        ),
                        "Invalid type for convert"
                    );
                    throw CompileTimeException("Invalid compiler, use correct compiler for compile AttachA, //ignored static_assert//");
                }
            } else {
                if constexpr (val_meta_.as_ref)
                    return ValueItem((void*)&val, val_meta_, art::as_reference);
                else {
                    if constexpr (needAlloc(val_meta_))
                        return ValueItem(new T(std::forward<T>(val)), val_meta_, art::no_copy);
                    else
                        return ValueItem(*(void**)&val, val_meta_, art::no_copy);
                }
            }
        }

        template <class T>
        T Vcast(const void* const& ref_val, const ValueMeta& meta) {
            static constexpr auto ret_meta_ = ValueMeta::from_type<T>();
            const void* const& val = getValue(ref_val, meta);

            if constexpr (std::is_same_v<T, art::ustring> || std::is_same_v<T, std::string>) {
                return Scast(val, meta);
            } else if constexpr (std::is_enum_v<T>) {
                if (meta.vtype == VType::ui8)
                    return (T)(uint8_t&)val;
                else if (meta.vtype == VType::ui16)
                    return (T)(uint16_t&)val;
                else if (meta.vtype == VType::ui32)
                    return (T)(uint32_t&)val;
                else if (meta.vtype == VType::ui64)
                    return (T)(uint64_t&)val;
                else if (meta.vtype == VType::i8)
                    return (T)(int8_t&)val;
                else if (meta.vtype == VType::i16)
                    return (T)(int16_t&)val;
                else if (meta.vtype == VType::i32)
                    return (T)(int32_t&)val;
                else if (meta.vtype == VType::i64)
                    return (T)(int64_t&)val;
                else
                    throw InvalidCast("Failed to cast enum, excepted non floating number");
            } else if constexpr (std::is_pointer_v<T>) { //strict handle
                if (meta.vtype != ret_meta_.vtype && meta.allow_edit != ret_meta_.allow_edit)
                    throw InvalidCast("Type mismatch excepted " + ret_meta_.to_string() + " but got " + meta.to_string());
                return (T)val;
            } else if constexpr (std::is_reference_v<T>) { //strict handle
                if (meta.vtype != ret_meta_.vtype && meta.allow_edit != ret_meta_.allow_edit)
                    throw InvalidCast("Type mismatch excepted " + ret_meta_.to_string() + " but got " + meta.to_string());
                return *(std::remove_reference_t<T>*)val;
            } else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
                switch (meta.vtype) {
                case VType::raw_arr_i8:
                    return list_array<int8_t>((int8_t*)val, meta.val_len).convert<ValueItem>();
                case VType::raw_arr_i16:
                    return list_array<int16_t>((int16_t*)val, meta.val_len).convert<ValueItem>();
                case VType::raw_arr_i32:
                    return list_array<int32_t>((int32_t*)val, meta.val_len).convert<ValueItem>();
                case VType::raw_arr_i64:
                    return list_array<int64_t>((int64_t*)val, meta.val_len).convert<ValueItem>();
                case VType::raw_arr_ui8:
                    return list_array<uint8_t>((uint8_t*)val, meta.val_len).convert<ValueItem>();
                case VType::raw_arr_ui16:
                    return list_array<uint16_t>((uint16_t*)val, meta.val_len).convert<ValueItem>();
                case VType::raw_arr_ui32:
                    return list_array<uint32_t>((uint32_t*)val, meta.val_len).convert<ValueItem>();
                case VType::raw_arr_ui64:
                    return list_array<uint64_t>((uint64_t*)val, meta.val_len).convert<ValueItem>();
                case VType::raw_arr_flo:
                    return list_array<float>((float*)val, meta.val_len).convert<ValueItem>();
                case VType::raw_arr_doub:
                    return list_array<double>((double*)val, meta.val_len).convert<ValueItem>();
                case VType::faarr:
                    return list_array<ValueItem>((ValueItem*)val, meta.val_len);
                case VType::set:
                    return list_array<ValueItem>(*(std::unordered_set<ValueItem, art::hash<ValueItem>>*)val);
                case VType::uarr:
                    return *(list_array<ValueItem>*)val;
                case VType::struct_: {
                    ValueItem args[]{ValueItem(const_cast<void*>(ref_val), (ValueMeta)meta, art::as_reference), ValueItem(ret_meta_)};
                    art::Environment env;
                    try {
                        env = ((const Structure&)val).table_get_dynamic("()", ClassAccess::pub);
                    } catch (const NotImplementedException&) {
                        env = nullptr;
                    }
                    if (env) {
                        ValueItem* res = env(args, 2);
                        if (!res)
                            return {ValueItem(ref_val, meta)};
                        ValueItem m(std::move(*res));
                        delete res;
                        return Vcast<T>(m.val, m.meta);
                    } else
                        return {ValueItem(ref_val, meta)};
                }
                default:
                    return {ValueItem(ref_val, meta)};
                }
            } else if constexpr (std::is_same_v<T, std::unordered_set<ValueItem, art::hash<ValueItem>>>) {
                switch (meta.vtype) {
                case VType::raw_arr_i8:
                    return list_array<int8_t>((int8_t*)val, meta.val_len).to_set<std::unordered_set<ValueItem, art::hash<ValueItem>>>();
                case VType::raw_arr_i16:
                    return list_array<int16_t>((int16_t*)val, meta.val_len).to_set<std::unordered_set<ValueItem, art::hash<ValueItem>>>();
                case VType::raw_arr_i32:
                    return list_array<int32_t>((int32_t*)val, meta.val_len).to_set<std::unordered_set<ValueItem, art::hash<ValueItem>>>();
                case VType::raw_arr_i64:
                    return list_array<int64_t>((int64_t*)val, meta.val_len).to_set<std::unordered_set<ValueItem, art::hash<ValueItem>>>();
                case VType::raw_arr_ui8:
                    return list_array<uint8_t>((uint8_t*)val, meta.val_len).to_set<std::unordered_set<ValueItem, art::hash<ValueItem>>>();
                case VType::raw_arr_ui16:
                    return list_array<uint16_t>((uint16_t*)val, meta.val_len).to_set<std::unordered_set<ValueItem, art::hash<ValueItem>>>();
                case VType::raw_arr_ui32:
                    return list_array<uint32_t>((uint32_t*)val, meta.val_len).to_set<std::unordered_set<ValueItem, art::hash<ValueItem>>>();
                case VType::raw_arr_ui64:
                    return list_array<uint64_t>((uint64_t*)val, meta.val_len).to_set<std::unordered_set<ValueItem, art::hash<ValueItem>>>();
                case VType::raw_arr_flo:
                    return list_array<float>((float*)val, meta.val_len).to_set<std::unordered_set<ValueItem, art::hash<ValueItem>>>();
                case VType::raw_arr_doub:
                    return list_array<double>((double*)val, meta.val_len).to_set<std::unordered_set<ValueItem, art::hash<ValueItem>>>();
                case VType::faarr:
                    return list_array<ValueItem>((ValueItem*)val, meta.val_len).to_set<std::unordered_set<ValueItem, art::hash<ValueItem>>>();
                case VType::uarr:
                    return ((list_array<ValueItem>*)val)->to_set<std::unordered_set<ValueItem, art::hash<ValueItem>>>();
                case VType::set:
                    return *(std::unordered_set<ValueItem, art::hash<ValueItem>>*)val;
                case VType::struct_: {
                    ValueItem args[]{ValueItem(const_cast<void*>(ref_val), (ValueMeta)meta, art::as_reference), ValueItem(ret_meta_)};
                    art::Environment env;
                    try {
                        env = ((const Structure&)val).table_get_dynamic("()", ClassAccess::pub);
                    } catch (const NotImplementedException&) {
                        env = nullptr;
                    }
                    if (env) {
                        ValueItem* res = env(args, 2);
                        if (!res)
                            return {ValueItem(ref_val, meta)};
                        ValueItem m(std::move(*res));
                        delete res;
                        return Vcast<T>(m.val, m.meta);
                    } else
                        return {ValueItem(ref_val, meta)};
                }
                default:
                    return {ValueItem(ref_val, meta)};
                }
            } else if constexpr (std::is_same_v<T, std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>>) {
                if (meta.vtype == VType::struct_) {
                    ValueItem args[]{ValueItem(const_cast<void*>(ref_val), (ValueMeta)meta, art::as_reference), ValueItem(ret_meta_)};
                    art::Environment env;
                    try {
                        env = ((const Structure&)val).table_get_dynamic("()", ClassAccess::pub);
                    } catch (const NotImplementedException&) {
                        env = nullptr;
                    }
                    if (env) {
                        ValueItem* res = env(args, 2);
                        if (res) {
                            ValueItem m(std::move(*res));
                            delete res;
                            return Vcast<T>(m.val, m.meta);
                        }
                    }
                }
                throw InvalidCast("Failed to cast value to map, not implemented");
            } else if constexpr (std::is_same_v<T, ValueItem>) {
                return ValueItem(ref_val, meta);
            } else if constexpr (std::is_arithmetic_v<T> || std::is_same_v<T, bool> || std::is_same_v<T, char32_t> || std::is_same_v<T, std::chrono::high_resolution_clock::time_point>) {
                switch (meta.vtype) {
                case VType::noting:
                    return 0;
                case VType::boolean:
                    return *(bool*)&val;
                case VType::i8:
                    return *(int8_t*)&val;
                case VType::i16:
                    return *(int16_t*)&val;
                case VType::i32:
                    return *(int32_t*)&val;
                case VType::i64:
                    return *(int64_t*)&val;
                case VType::ui8:
                    return *(uint8_t*)&val;
                case VType::ui16:
                    return *(uint16_t*)&val;
                case VType::ui32:
                    return *(uint32_t*)&val;
                case VType::ui64:
                    return *(uint64_t*)&val;
                case VType::flo:
                    return *(float*)&val;
                case VType::doub:
                    return *(double*)&val;
                case VType::character:
                    return *(char32_t*)&val;
                case VType::time_point:
                    return (T) reinterpret_cast<const std::chrono::high_resolution_clock::time_point*>(&val)->time_since_epoch().count();
                default:
                    break;
                }
            }
            throw InvalidCast("Failed to cast undefined type");
        }

        template <class T>
        T Vcast(ValueItem& it) {
            static constexpr bool is_aggregate_or_class = std::is_aggregate_v<std::remove_cvref_t<T>> || std::is_class_v<std::remove_cvref_t<T>>;
            if constexpr (
                is_aggregate_or_class && !std::is_same_v<std::remove_cvref_t<T>, art::ustring> && !std::is_same_v<std::remove_cvref_t<T>, ValueMeta> && !std::is_same_v<std::remove_cvref_t<T>, ValueItem> && !std::is_same_v<std::remove_cvref_t<T>, list_array<ValueItem>> && !std::is_same_v<std::remove_cvref_t<T>, Structure>
            ) {
                auto& struct_ = (Structure&)it;
                switch (struct_.vtable_mode) {
                case Structure::VTableMode::AttachAVirtualTable:
                    return CXX::Interface::getExtractAsStatic<std::remove_cvref_t<T>>(struct_);
                case Structure::VTableMode::AttachADynamicVirtualTable:
                    return CXX::Interface::getExtractAsDynamic<std::remove_cvref_t<T>>(struct_);
                default:
                    throw InvalidCast("Failed to cast structure (unknown vtable mode)");
                }
            } else
                return Vcast<T>(it.val, it.meta);
        }
    }

    void DynSum(void** val0, void** val1);
    void DynMinus(void** val0, void** val1);
    void DynMul(void** val0, void** val1);
    void DynDiv(void** val0, void** val1);
    void DynRest(void** val0, void** val1);


    void DynBitXor(void** val0, void** val1);
    void DynBitOr(void** val0, void** val1);
    void DynBitAnd(void** val0, void** val1);
    void DynBitNot(void** val0);
    void DynBitShiftLeft(void** val0, void** val1);
    void DynBitShiftRight(void** val0, void** val1);


    void* AsArg(void** val);
    void AsArr(void** val);
    void AsMap(void** val);
    void AsSet(void** val);

    size_t getSize(void** value);

    void asValue(void** val, VType type);
    bool isValue(void** val, VType type);
    bool isTrueValue(void** value);
    void setBoolValue(bool, void** value);
}
#endif /* SRC_RUN_TIME_ATTACHA_ABI */
