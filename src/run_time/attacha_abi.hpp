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
    #include <util/ustring.hpp>

namespace art {
    bool needAlloc(ValueMeta type);
    bool needAllocType(VType type);


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

    template <class T>
    class __GetType {
        static constexpr VType Type_as_VType() {
            if constexpr (std::is_same_v<T, void>)
                return VType::noting;
            else if constexpr (std::is_same_v<T, bool>)
                return VType::boolean;
            else if constexpr (std::is_same_v<T, int8_t>)
                return VType::i8;
            else if constexpr (std::is_same_v<T, int16_t>)
                return VType::i16;
            else if constexpr (std::is_same_v<T, int32_t>)
                return VType::i32;
            else if constexpr (std::is_same_v<T, int64_t>)
                return VType::i64;
            else if constexpr (std::is_same_v<T, uint8_t>)
                return VType::ui8;
            else if constexpr (std::is_same_v<T, uint16_t>)
                return VType::ui16;
            else if constexpr (std::is_same_v<T, uint32_t>)
                return VType::ui32;
            else if constexpr (std::is_same_v<T, uint64_t>)
                return VType::ui64;
            else if constexpr (std::is_same_v<T, float>)
                return VType::flo;
            else if constexpr (std::is_same_v<T, double>)
                return VType::doub;
            else if constexpr (std::is_same_v<T, int8_t*>)
                return VType::raw_arr_i8;
            else if constexpr (std::is_same_v<T, int16_t*>)
                return VType::raw_arr_i16;
            else if constexpr (std::is_same_v<T, int32_t*>)
                return VType::raw_arr_i32;
            else if constexpr (std::is_same_v<T, int64_t*>)
                return VType::raw_arr_i64;
            else if constexpr (std::is_same_v<T, uint8_t*>)
                return VType::raw_arr_ui8;
            else if constexpr (std::is_same_v<T, uint16_t*>)
                return VType::raw_arr_ui16;
            else if constexpr (std::is_same_v<T, uint32_t*>)
                return VType::raw_arr_ui32;
            else if constexpr (std::is_same_v<T, uint64_t*>)
                return VType::raw_arr_ui64;
            else if constexpr (std::is_same_v<T, float*>)
                return VType::raw_arr_flo;
            else if constexpr (std::is_same_v<T, double*>)
                return VType::raw_arr_doub;
            else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
                return VType::uarr;
            else if constexpr (std::is_same_v<T, art::ustring>)
                return VType::string;
            else if constexpr (std::is_same_v<T, art::typed_lgr<Task>>)
                return VType::async_res;
            else if constexpr (std::is_same_v<T, void*>)
                return VType::undefined_ptr;
            else if constexpr (std::is_same_v<T, std::exception_ptr*>)
                return VType::except_value;
            else if constexpr (std::is_same_v<T, ValueItem*>)
                return VType::faarr;
            else if constexpr (std::is_same_v<T, Structure>)
                return VType::struct_;
            else if constexpr (std::is_same_v<T, ValueMeta>)
                return VType::type_identifier;
            else if constexpr (std::is_same_v<T, art::shared_ptr<FuncEnvironment>>)
                return VType::function;
            else
                return VType::noting;
        }

    public:
        inline static constexpr VType res = Type_as_VType();
        inline static constexpr bool usable = Type_as_VType() != VType::noting || std::is_same_v<T, void>;
    };

    template <>
    class __GetType<char> {
    public:
        inline static constexpr VType res = VType::i8;
        inline static constexpr bool usable = true;
    };

    template <class T>
    constexpr
        typename std::enable_if<__GetType<T>::usable, VType>::type
        __Type_as_VType() {
        return __GetType<T>::res;
    }

    template <class T>
    constexpr VType Type_as_VType() {
        return __Type_as_VType<T>();
    }

    template <class T>
    typename std::enable_if<
        std::is_array_v<T> && std::is_bounded_array_v<T> && !std::is_reference_v<T>,
        ValueMeta>::type
    Type_as_ValueMeta() {
        ValueMeta res = Type_as_VType<std::remove_cvref_t<T>*>();
        res.allow_edit = !std::is_const_v<T>;
        res.val_len = std::extent_v<T>;
        return res;
    }

    template <class T>
    typename std::enable_if<
        std::is_array_v<T> && std::is_bounded_array_v<T> && std::is_reference_v<T>,
        ValueMeta>::type
    Type_as_ValueMeta() {
        ValueMeta res = Type_as_VType<std::remove_cvref_t<T>*>();
        res.allow_edit = !std::is_const_v<T>;
        res.val_len = std::extent_v<T>;
        res.as_ref = true;
        return res;
    }

    template <class T>
    typename std::enable_if<std::is_array_v<T>, ValueMeta>::type
    Type_as_ValueMeta() {
        ValueMeta res = Type_as_VType<std::remove_cvref_t<T>>();
        if (std::is_pointer_v<T>)
            res.as_ref = true;
        else if (std::is_reference_v<T>)
            res.as_ref = true;
        res.allow_edit = !std::is_const_v<T>;
        return res;
    }

    template <typename T>
    struct is_typed_lgr {
        static constexpr bool value = false;
    };

    template <typename T>
    struct is_typed_lgr<typed_lgr<T>> {
        static constexpr bool value = true;
    };

    template <class T>
    typename std::enable_if<
        !(std::is_array_v<T> && std::is_bounded_array_v<T> || is_typed_lgr<T>::value),
        ValueMeta>::type
    Type_as_ValueMeta() {
        ValueMeta res = Type_as_VType<std::remove_cvref_t<T>>();
        if (std::is_pointer_v<T>)
            res.as_ref = true;
        else if (std::is_reference_v<T>)
            res.as_ref = true;
        res.allow_edit = !std::is_const_v<T>;
        return res;
    }

    template <typename T>
    typename std::enable_if<
        !(std::is_array_v<T> && std::is_bounded_array_v<T>)&&is_typed_lgr<T>::value,
        ValueMeta>::type
    Type_as_ValueMeta() {
        ValueMeta res = Type_as_VType<std::remove_cvref_t<T>>();
        res.allow_edit = !std::is_const_v<T>;
        res.use_gc = true;
        return res;
    }

    void universalRemove(void** value);
    void universalAlloc(void** value, ValueMeta meta);

    ValueItem* getAsyncValueItem(void* val);
    void getValueItem(void** value, ValueItem* f_res);
    ValueItem* buildRes(void** value);
    ValueItem* buildResTake(void** value);


    void getAsyncResult(void*& value, ValueMeta& meta);
    void* copyValue(void*& val, ValueMeta& meta);


    void** preSetValue(void** value, ValueMeta set_meta, bool match_gc_dif);
    void*& getValue(void*& value, ValueMeta& meta);
    const void* const& getValue(const void* const& value, const ValueMeta& meta);
    void*& getValue(void** value);
    void* getSpecificValue(void** value, VType typ);
    void** getSpecificValueLink(void** value, VType typ);

    bool is_integer(VType typ);
    bool integer_unsigned(VType typ);
    bool is_raw_array(VType typ);
    bool has_interface(VType typ);

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
        ValueItem BVcast(const T& val) {
            if constexpr (std::is_same_v<std::remove_cvref_t<T>, std::nullptr_t> || std::is_same_v<std::remove_cvref_t<T>, void>)
                return ValueItem();
            if constexpr (std::is_same_v<std::remove_cvref_t<T>, bool>)
                return ValueItem((void*)(0ull + val), VType::boolean);
            else if constexpr (std::is_same_v<std::remove_cvref_t<T>, int8_t>)
                return ValueItem((void*)(0ll + val), VType::i8);
            else if constexpr (std::is_same_v<std::remove_cvref_t<T>, uint8_t>)
                return ValueItem((void*)(0ull + val), VType::ui8);
            else if constexpr (std::is_same_v<std::remove_cvref_t<T>, int16_t>)
                return ValueItem((void*)(0ll + val), VType::i16);
            else if constexpr (std::is_same_v<std::remove_cvref_t<T>, uint16_t>)
                return ValueItem((void*)(0ull + val), VType::ui16);
            else if constexpr (std::is_same_v<std::remove_cvref_t<T>, int32_t>)
                return ValueItem((void*)(0ll + val), VType::i32);
            else if constexpr (std::is_same_v<std::remove_cvref_t<T>, uint32_t>)
                return ValueItem((void*)(0ull + val), VType::ui32);
            else if constexpr (std::is_same_v<std::remove_cvref_t<T>, int64_t>)
                return ValueItem((void*)val, VType::i64);
            else if constexpr (std::is_same_v<std::remove_cvref_t<T>, uint64_t>)
                return ValueItem((void*)val, VType::ui64);
            else if constexpr (std::is_same_v<std::remove_cvref_t<T>, float>)
                return ValueItem(*(void**)&val, VType::flo);
            else if constexpr (std::is_same_v<std::remove_cvref_t<T>, double>)
                return ValueItem(*(void**)&val, VType::doub);
            else if constexpr (std::is_same_v<std::remove_cvref_t<T>, art::ustring> || std::is_same_v<T, const char*>)
                return ValueItem(new art::ustring(val), VType::string, no_copy);
            else if constexpr (std::is_same_v<std::remove_cvref_t<T>, list_array<ValueItem>>)
                return ValueItem(new list_array<ValueItem>(val), VType::uarr, no_copy);
            else if constexpr (std::is_same_v<std::remove_cvref_t<T>, ValueMeta>)
                return ValueItem(*(void**)&val, VType::type_identifier);
            else if constexpr (std::is_same_v<std::remove_cvref_t<T>, Structure>)
                return ValueItem(Structure::copy(val), no_copy);
            else if constexpr (std::is_same_v<std::remove_cvref_t<T>, art::shared_ptr<FuncEnvironment>>)
                return ValueItem(new art::shared_ptr<FuncEnvironment>(val), VType::function, no_copy);
            else if constexpr (std::is_same_v<std::remove_cvref_t<T>, Environment>)
                return ValueItem(new art::shared_ptr<FuncEnvironment>(new FuncEnvironment(val, false)), VType::function, no_copy);
            else if constexpr (std::is_same_v<std::remove_cvref_t<T>, ValueItem>)
                return val;
            else if constexpr (std::is_same_v<std::remove_cvref_t<T>, void*>)
                return val;
            else if constexpr (std::is_reference_v<T> && (std::is_aggregate_v<std::remove_cvref_t<T>> || std::is_class_v<std::remove_cvref_t<T>>)) {
                throw NotImplementedException(); //TODO: implement
            } else {
                static_assert(
                    (
                        std::is_arithmetic_v<std::remove_cvref_t<T>> ||
                        std::is_same_v<std::remove_cv_t<T>, char*> ||
                        std::is_same_v<std::remove_cvref_t<T>, art::ustring> ||
                        std::is_same_v<std::remove_cvref_t<T>, ValueItem> ||
                        std::is_same_v<std::remove_cvref_t<T>, ValueMeta> ||
                        std::is_same_v<std::remove_cvref_t<T>, Structure> ||
                        std::is_same_v<std::remove_cvref_t<T>, ValueItem> ||
                        std::is_same_v<std::remove_cvref_t<T>, list_array<ValueItem>> ||
                        std::is_same_v<std::remove_cvref_t<T>, std::nullptr_t> ||
                        std::is_same_v<std::remove_cvref_t<T>, void> ||
                        std::is_same_v<std::remove_cvref_t<T>, art::shared_ptr<FuncEnvironment>> ||
                        std::is_same_v<std::remove_cvref_t<T>, Environment> ||
                        std::is_same_v<std::remove_cvref_t<T>, bool> ||
                        std::is_same_v<std::remove_cvref_t<T>, void*> ||
                        (std::is_reference_v<T> && (std::is_aggregate_v<std::remove_cvref_t<T>> || std::is_class_v<std::remove_cvref_t<T>>))
                    ),
                    "Invalid type for convert"
                );
                throw CompileTimeException("Invalid compiler, use correct compiler for compile AttachA, //ignored static_assert//");
            }
        }

        template <size_t N>
        ValueItem BVcast(const char (&str)[N]) {
            return art::ustring(str);
        }

        template <class T>
        T Vcast(const void* const& ref_val, const ValueMeta& meta) {
            const void* val = getValue(ref_val, meta);

            if constexpr (std::is_same_v<T, art::ustring>) {
                return Scast(val, meta);
            } else {
                switch (meta.vtype) {
                case VType::noting:
                    return T();
                case VType::boolean:
                case VType::i8: {
                    if constexpr (std::is_same_v<T, void*>)
                        return const_cast<void*>(val);
                    else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
                        return {ValueItem(val, meta)};
                    else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>)
                        return (T)(int8_t)(ptrdiff_t)val;
                    else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>> || std::is_same_v<T, ValueItem*>)
                        return AsPointer<std::remove_pointer_t<T>, int8_t>(val);
                    else
                        return (T)val;
                    break;
                }
                case VType::i16: {
                    if constexpr (std::is_same_v<T, void*>)
                        return const_cast<void*>(val);
                    else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
                        return {ValueItem(val, meta)};
                    else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>)
                        return (T)(int16_t)(ptrdiff_t)val;
                    else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>> || std::is_same_v<T, ValueItem*>)
                        return AsPointer<std::remove_pointer_t<T>, int16_t>(val);
                    else
                        return (T)val;
                    break;
                }
                case VType::i32: {
                    if constexpr (std::is_same_v<T, void*>)
                        return const_cast<void*>(val);
                    else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
                        return {ValueItem(val, meta)};
                    else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>)
                        return (T)(int32_t)(ptrdiff_t)val;
                    else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>> || std::is_same_v<T, ValueItem*>)
                        return AsPointer<std::remove_pointer_t<T>, int32_t>(val);
                    else
                        return (T)val;
                    break;
                }
                case VType::i64: {
                    if constexpr (std::is_same_v<T, void*>)
                        return const_cast<void*>(val);
                    else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
                        return {ValueItem(val, meta)};
                    else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>)
                        return (T)(int64_t)(ptrdiff_t)val;
                    else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>> || std::is_same_v<T, ValueItem*>)
                        return AsPointer<std::remove_pointer_t<T>, int64_t>(val);
                    else
                        return (T)val;
                    break;
                }
                case VType::ui8: {
                    if constexpr (std::is_same_v<T, void*>)
                        return const_cast<void*>(val);
                    else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
                        return {ValueItem(val, meta)};
                    else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>)
                        return (T)(uint8_t)val;
                    else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>> || std::is_same_v<T, ValueItem*>)
                        return AsPointer<std::remove_pointer_t<T>, uint8_t>(val);
                    else
                        return (T)val;
                    break;
                }
                case VType::ui16: {
                    if constexpr (std::is_same_v<T, void*>)
                        return const_cast<void*>(val);
                    else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
                        return {ValueItem(val, meta)};
                    else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>)
                        return (T)(uint16_t)val;
                    else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>> || std::is_same_v<T, ValueItem*>)
                        return AsPointer<std::remove_pointer_t<T>, uint16_t>(val);
                    else
                        return (T)val;
                    break;
                }
                case VType::ui32: {
                    if constexpr (std::is_same_v<T, void*>)
                        return const_cast<void*>(val);
                    else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
                        return {ValueItem(val, meta)};
                    else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>)
                        return (T)(uint32_t)val;
                    else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>> || std::is_same_v<T, ValueItem*>)
                        return AsPointer<std::remove_pointer_t<T>, uint32_t>(val);
                    else
                        return (T)val;
                    break;
                }
                case VType::ui64: {
                    if constexpr (std::is_same_v<T, void*>)
                        return const_cast<void*>(val);
                    else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
                        return {ValueItem(val, meta)};
                    else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>)
                        return (T)(uint64_t)val;
                    else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>> || std::is_same_v<T, ValueItem*>)
                        return AsPointer<std::remove_pointer_t<T>, uint64_t>(val);
                    else
                        return (T)val;
                    break;
                }
                case VType::flo: {
                    if constexpr (std::is_same_v<T, void*>)
                        return const_cast<void*>(val);
                    else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
                        return {ValueItem(val, meta)};
                    else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>)
                        return (T) * (float*)&val;
                    else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>> || std::is_same_v<T, ValueItem*>)
                        return AsPointer<std::remove_pointer_t<T>, float>(val);
                    else
                        return (T)val;
                    break;
                }
                case VType::doub: {
                    if constexpr (std::is_same_v<T, void*>)
                        return const_cast<void*>(val);
                    else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
                        return {ValueItem(val, meta)};
                    else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>)
                        return (T) * (double*)&val;
                    else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>> || std::is_same_v<T, ValueItem*>)
                        return AsPointer<std::remove_pointer_t<T>, double>(val);
                    else
                        return (T)val;
                    break;
                }
                case VType::raw_arr_i8: {
                    if constexpr (std::is_same_v<T, void*>)
                        return const_cast<void*>(val);
                    else if constexpr (std::is_arithmetic_v<T>)
                        return (T) * reinterpret_cast<const int8_t*>(val);
                    else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>> || std::is_same_v<T, ValueItem*>) {
                        if (meta.val_len) {
                            std::remove_pointer_t<T>* tmp = new std::remove_pointer_t<T>[meta.val_len];
                            for (uint32_t i = 0; i < meta.val_len; i++)
                                tmp[i] = (std::remove_pointer_t<T>)reinterpret_cast<const int8_t*>(val)[i];
                            return tmp;
                        } else
                            return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)*reinterpret_cast<const int8_t*>(val) };
                    } else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
                        if (meta.val_len) {
                            list_array<ValueItem> res;
                            res.resize(meta.val_len);
                            for (uint32_t i = 0; i < meta.val_len; i++)
                                res[i] = ValueItem((void*)reinterpret_cast<const int8_t*>(val)[i], VType::i8);
                            return res;
                        } else
                            return {ValueItem(val, meta)};
                    } else
                        throw InvalidCast("Fail cast raw_arr_i8");
                    break;
                }
                case VType::raw_arr_i16: {
                    if constexpr (std::is_same_v<T, void*>)
                        return const_cast<void*>(val);
                    else if constexpr (std::is_arithmetic_v<T>)
                        return (T) * reinterpret_cast<const int16_t*>(val);
                    else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>> || std::is_same_v<T, ValueItem*>) {
                        if (meta.val_len) {
                            std::remove_pointer_t<T>* tmp = new std::remove_pointer_t<T>[meta.val_len];
                            for (uint32_t i = 0; i < meta.val_len; i++)
                                tmp[i] = (std::remove_pointer_t<T>)reinterpret_cast<const int16_t*>(val)[i];
                            return tmp;
                        } else
                            return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)*reinterpret_cast<const int16_t*>(val) };
                    } else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
                        if (meta.val_len) {
                            list_array<ValueItem> res;
                            res.resize(meta.val_len);
                            for (uint32_t i = 0; i < meta.val_len; i++)
                                res[i] = ValueItem((void*)reinterpret_cast<const int16_t*>(val)[i], VType::i16);
                            return res;
                        } else
                            return {ValueItem(val, meta)};
                    } else
                        throw InvalidCast("Fail cast raw_arr_i16");
                    break;
                }
                case VType::raw_arr_i32: {
                    if constexpr (std::is_same_v<T, void*>)
                        return const_cast<void*>(val);
                    else if constexpr (std::is_arithmetic_v<T>)
                        return (T) * reinterpret_cast<const int32_t*>(val);
                    else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>> || std::is_same_v<T, ValueItem*>) {
                        if (meta.val_len) {
                            std::remove_pointer_t<T>* tmp = new std::remove_pointer_t<T>[meta.val_len];
                            for (uint32_t i = 0; i < meta.val_len; i++)
                                tmp[i] = (std::remove_pointer_t<T>)reinterpret_cast<const int32_t*>(val)[i];
                            return tmp;
                        } else
                            return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)*reinterpret_cast<const int32_t*>(val) };
                    } else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
                        if (meta.val_len) {
                            list_array<ValueItem> res;
                            res.resize(meta.val_len);
                            for (uint32_t i = 0; i < meta.val_len; i++)
                                res[i] = ValueItem((void*)(ptrdiff_t) reinterpret_cast<const int32_t*>(val)[i], VType::i32);
                            return res;
                        } else
                            return {ValueItem(val, meta)};
                    } else
                        throw InvalidCast("Fail cast raw_arr_i32");
                    break;
                }
                case VType::raw_arr_i64: {
                    if constexpr (std::is_same_v<T, void*>)
                        return const_cast<void*>(val);
                    else if constexpr (std::is_arithmetic_v<T>)
                        return (T) * (int64_t*)val;
                    else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>> || std::is_same_v<T, ValueItem*>) {
                        if (meta.val_len) {
                            std::remove_pointer_t<T>* tmp = new std::remove_pointer_t<T>[meta.val_len];
                            for (uint32_t i = 0; i < meta.val_len; i++)
                                tmp[i] = (std::remove_pointer_t<T>)reinterpret_cast<const int64_t*>(val)[i];
                            return tmp;
                        } else
                            return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)*reinterpret_cast<const int64_t*>(val) };
                    } else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
                        if (meta.val_len) {
                            list_array<ValueItem> res;
                            res.resize(meta.val_len);
                            for (uint32_t i = 0; i < meta.val_len; i++)
                                res[i] = ValueItem((void*)reinterpret_cast<const int64_t*>(val)[i], VType::i64);
                            return res;
                        } else
                            return {ValueItem(val, meta)};
                    } else
                        throw InvalidCast("Fail cast raw_arr_i64");
                    break;
                }
                case VType::raw_arr_ui8: {
                    if constexpr (std::is_same_v<T, void*>)
                        return const_cast<void*>(val);
                    else if constexpr (std::is_arithmetic_v<T>)
                        return (T) * reinterpret_cast<const uint8_t*>(val);
                    else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>> || std::is_same_v<T, ValueItem*>) {
                        if (meta.val_len) {
                            std::remove_pointer_t<T>* tmp = new std::remove_pointer_t<T>[meta.val_len];
                            for (uint32_t i = 0; i < meta.val_len; i++)
                                tmp[i] = (std::remove_pointer_t<T>)reinterpret_cast<const uint8_t*>(val)[i];
                            return tmp;
                        } else
                            return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)*reinterpret_cast<const uint8_t*>(val) };
                    } else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
                        if (meta.val_len) {
                            list_array<ValueItem> res;
                            res.resize(meta.val_len);
                            for (uint32_t i = 0; i < meta.val_len; i++)
                                res[i] = ValueItem((void*)reinterpret_cast<const uint8_t*>(val)[i], VType::ui8);
                            return res;
                        } else
                            return {ValueItem(val, meta)};
                    } else
                        throw InvalidCast("Fail cast raw_arr_ui8");
                    break;
                }
                case VType::raw_arr_ui16: {
                    if constexpr (std::is_same_v<T, void*>)
                        return const_cast<void*>(val);
                    else if constexpr (std::is_arithmetic_v<T>)
                        return (T) * reinterpret_cast<const uint16_t*>(val);
                    else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>> || std::is_same_v<T, ValueItem*>) {
                        if (meta.val_len) {
                            std::remove_pointer_t<T>* tmp = new std::remove_pointer_t<T>[meta.val_len];
                            for (uint32_t i = 0; i < meta.val_len; i++)
                                tmp[i] = (std::remove_pointer_t<T>)reinterpret_cast<const uint16_t*>(val)[i];
                            return tmp;
                        } else
                            return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)*reinterpret_cast<const uint16_t*>(val) };
                    } else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
                        if (meta.val_len) {
                            list_array<ValueItem> res;
                            res.resize(meta.val_len);
                            for (uint32_t i = 0; i < meta.val_len; i++)
                                res[i] = ValueItem((void*)reinterpret_cast<const uint16_t*>(val)[i], VType::ui16);
                            return res;
                        } else
                            return {ValueItem(val, meta)};
                    } else
                        throw InvalidCast("Fail cast raw_arr_ui16");
                    break;
                }
                case VType::raw_arr_ui32: {
                    if constexpr (std::is_same_v<T, void*>)
                        return const_cast<void*>(val);
                    else if constexpr (std::is_arithmetic_v<T>)
                        return (T) * reinterpret_cast<const uint32_t*>(val);
                    else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>> || std::is_same_v<T, ValueItem*>) {
                        if (meta.val_len) {
                            std::remove_pointer_t<T>* tmp = new std::remove_pointer_t<T>[meta.val_len];
                            for (uint32_t i = 0; i < meta.val_len; i++)
                                tmp[i] = (std::remove_pointer_t<T>)reinterpret_cast<const uint32_t*>(val)[i];
                            return tmp;
                        } else
                            return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)*reinterpret_cast<const uint32_t*>(val) };
                    } else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
                        if (meta.val_len) {
                            list_array<ValueItem> res;
                            res.resize(meta.val_len);
                            for (uint32_t i = 0; i < meta.val_len; i++)
                                res[i] = ValueItem((void*)(size_t) reinterpret_cast<const uint32_t*>(val)[i], VType::ui32);
                            return res;
                        } else
                            return {ValueItem(val, meta)};
                    } else
                        throw InvalidCast("Fail cast raw_arr_ui32");
                    break;
                }
                case VType::raw_arr_ui64: {
                    if constexpr (std::is_same_v<T, void*>)
                        return const_cast<void*>(val);
                    else if constexpr (std::is_arithmetic_v<T>)
                        return (T) * reinterpret_cast<const uint64_t*>(val);
                    else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>> || std::is_same_v<T, ValueItem*>) {
                        if (meta.val_len) {
                            std::remove_pointer_t<T>* tmp = new std::remove_pointer_t<T>[meta.val_len];
                            for (uint32_t i = 0; i < meta.val_len; i++)
                                tmp[i] = (std::remove_pointer_t<T>)reinterpret_cast<const uint64_t*>(val)[i];
                            return tmp;
                        } else
                            return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)*reinterpret_cast<const uint64_t*>(val) };
                    } else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
                        if (meta.val_len) {
                            list_array<ValueItem> res;
                            res.resize(meta.val_len);
                            for (uint32_t i = 0; i < meta.val_len; i++)
                                res[i] = ValueItem((void*)reinterpret_cast<const uint64_t*>(val)[i], VType::ui64);
                            return res;
                        } else
                            return {ValueItem(val, meta)};
                    } else
                        throw InvalidCast("Fail cast raw_arr_ui64");
                    break;
                }
                case VType::raw_arr_flo: {
                    if constexpr (std::is_same_v<T, void*>)
                        return const_cast<void*>(val);
                    else if constexpr (std::is_arithmetic_v<T>)
                        return (T) * reinterpret_cast<const float*>(val);
                    else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>> || std::is_same_v<T, ValueItem*>) {
                        if (meta.val_len) {
                            std::remove_pointer_t<T>* tmp = new std::remove_pointer_t<T>[meta.val_len];
                            for (uint32_t i = 0; i < meta.val_len; i++)
                                tmp[i] = (std::remove_pointer_t<T>)reinterpret_cast<const float*>(val)[i];
                            return tmp;
                        } else
                            return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)*reinterpret_cast<const float*>(val) };
                    } else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
                        if (meta.val_len) {
                            list_array<ValueItem> res;
                            res.resize(meta.val_len);
                            for (uint32_t i = 0; i < meta.val_len; i++)
                                res[i] = ValueItem(*(void**)&reinterpret_cast<const float*>(val)[i], VType::flo);
                            return res;
                        } else
                            return {ValueItem(val, meta)};
                    } else
                        throw InvalidCast("Fail cast raw_arr_flo");
                    break;
                }
                case VType::raw_arr_doub: {
                    if constexpr (std::is_same_v<T, void*>)
                        return const_cast<void*>(val);
                    else if constexpr (std::is_arithmetic_v<T>)
                        return (T) * reinterpret_cast<const double*>(val);
                    else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>> || std::is_same_v<T, ValueItem*>) {
                        if (meta.val_len) {
                            std::remove_pointer_t<T>* tmp = new std::remove_pointer_t<T>[meta.val_len];
                            for (uint32_t i = 0; i < meta.val_len; i++)
                                tmp[i] = (std::remove_pointer_t<T>)reinterpret_cast<const double*>(val)[i];
                            return tmp;
                        } else
                            return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)*reinterpret_cast<const double*>(val) };
                    } else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
                        if (meta.val_len) {
                            list_array<ValueItem> res;
                            res.resize(meta.val_len);
                            for (uint32_t i = 0; i < meta.val_len; i++)
                                res[i] = ValueItem(*(void**)&(reinterpret_cast<const double*>(val)[i]), VType::doub);
                            return res;
                        } else
                            return {ValueItem(val, meta)};
                    } else
                        throw InvalidCast("Fail cast raw_arr_doub");
                    break;
                }
                case VType::uarr: {
                    if constexpr (std::is_same_v<T, void*>)
                        return const_cast<void*>(val);
                    else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
                        return reinterpret_cast<const list_array<ValueItem>&>(val);
                    else if constexpr (std::is_arithmetic_v<T>) {
                        if (((list_array<ValueItem>*)val)->size() != 1)
                            throw InvalidCast("Fail cast uarr");
                        else {
                            auto& tmp = (*(const list_array<ValueItem>*)val)[0];
                            return (T)tmp;
                        }
                    } else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>> || std::is_same_v<T, ValueItem*>) {
                        if (meta.val_len) {
                            const list_array<ValueItem>& ref = reinterpret_cast<const list_array<ValueItem>&>(val);
                            std::remove_pointer_t<T>* res = new std::remove_pointer_t<T>[meta.val_len];
                            for (uint32_t i = 0; i < meta.val_len; i++) {
                                ValueItem& tmp = ref[i];
                                res[i] = (std::remove_pointer_t<T>)tmp;
                            }
                            return res;
                        } else
                            throw InvalidCast("Fail cast uarr");
                    } else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
                        return reinterpret_cast<const list_array<ValueItem>&>(val);
                    } else
                        throw InvalidCast("Fail cast uarr");
                    break;
                }
                case VType::string: {
                    if constexpr (std::is_same_v<T, void*>)
                        return const_cast<void*>(val);
                    else if constexpr (std::is_same_v<T, art::ustring>)
                        return reinterpret_cast<const art::ustring&>(val);
                    else if constexpr (!std::is_pointer_v<T>) {
                        return (T)SBcast(reinterpret_cast<const art::ustring&>(val));
                    } else
                        throw InvalidCast("Fail cast string");
                    break;
                }
                case VType::undefined_ptr: {
                    if constexpr (std::is_same_v<T, void*>)
                        return const_cast<void*>(val);
                    else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
                        return {ValueItem(val, meta)};
                    else if constexpr (std::is_arithmetic_v<T>)
                        return *reinterpret_cast<const T*>(&val);
                    else if constexpr (std::is_floating_point_v<std::remove_pointer_t<T>>)
                        return new std::remove_pointer_t<T>[] { *reinterpret_cast<std::remove_pointer_t<const T>*>(&val) };
                    else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>> || std::is_same_v<T, ValueItem*>)
                        return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)val };
                    else if constexpr (std::is_pointer_v<T>)
                        return (T)val;
                    else
                        throw InvalidCast("Fail cast undefined_ptr");
                    break;
                }
                case VType::type_identifier: {
                    if constexpr (std::is_same_v<T, list_array<ValueItem>>)
                        return {ValueItem(val, meta)};
                    else if constexpr (std::is_arithmetic_v<T>)
                        return (T)meta.encoded;
                    else if constexpr (std::is_same_v<T, VType>)
                        return meta.vtype;
                    else if constexpr (std::is_same_v<T, ValueMeta>)
                        return meta;
                    else
                        throw InvalidCast("Fail cast type_identifier");
                    break;
                }
                case VType::faarr: {
                    if constexpr (std::is_same_v<T, list_array<ValueItem>>)
                        return list_array<ValueItem>((ValueItem*)val, ((ValueItem*)val) + meta.val_len);
                    else if constexpr (std::is_same_v<T, void*>)
                        return const_cast<void*>(val);
                    else if constexpr (std::is_arithmetic_v<T>)
                        return (T)meta.val_len;
                    else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>> || std::is_same_v<T, ValueItem*>) {
                        if (meta.val_len) {
                            std::remove_pointer_t<T>* tmp = new std::remove_pointer_t<T>[meta.val_len];
                            for (uint32_t i = 0; i < meta.val_len; i++)
                                tmp[i] = (std::remove_pointer_t<T>)reinterpret_cast<const ValueItem*>(val)[i];
                            return tmp;
                        } else
                            return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)*reinterpret_cast<const ValueItem*>(val) };
                    } else
                        throw InvalidCast("Fail cast faarr");
                    break;
                }
                case VType::struct_: {
                    if constexpr (std::is_same_v<T, Structure>)
                        return (Structure&)val;
                    else if constexpr (std::is_same_v<T, void*>) {
                        return const_cast<void*>(val);
                    } else {
                        ValueItem& tmp = const_cast<ValueItem&>(reinterpret_cast<const ValueItem&>(ref_val));
                        ValueItem* res = ((const Structure&)val).table_get_dynamic("()", ClassAccess::pub)(&tmp, 1);
                        if (!res)
                            return T();
                        ValueItem m(std::move(*res));
                        delete res;
                        return Vcast<T>(m.val, m.meta);
                    }
                    break;
                }
                case VType::function: {
                    if constexpr (std::is_same_v<T, void*>)
                        return const_cast<void*>(val);
                    else if constexpr (!std::is_pointer_v<T>) {
                        auto tmp = _Vcast_callFN(val);
                        if (!tmp)
                            return T();
                        T res;
                        try {
                            res = (T)*tmp;
                        } catch (...) {
                            delete tmp;
                            throw;
                        }
                        delete tmp;
                        return res;
                    } else
                        throw InvalidCast("Fail cast function");
                    break;
                }
                default:
                    throw InvalidCast("Fail cast undefined type");
                }
            }
        }

        template <class T>
        T Vcast(ValueItem& it) {
            static constexpr bool is_aggregate_or_class = std::is_reference_v<T> && (std::is_aggregate_v<std::remove_cvref_t<T>> || std::is_class_v<std::remove_cvref_t<T>>);
            if constexpr (
                is_aggregate_or_class && !std::is_same_v<std::remove_cvref_t<T>, art::ustring> && !std::is_same_v<std::remove_cvref_t<T>, ValueMeta> && !std::is_same_v<std::remove_cvref_t<T>, ValueItem> && !std::is_same_v<std::remove_cvref_t<T>, list_array<ValueItem>> && !std::is_same_v<std::remove_cvref_t<T>, Structure>
            ) {
                auto& struct_ = (Structure&)it;
                switch (struct_.get_vtable_mode()) {
                case Structure::VTableMode::disabled:
                    throw InvalidCast("Fail cast structure (vtable disabled)");
                case Structure::VTableMode::AttachAVirtualTable:
                    return CXX::Interface::getExtractAsStatic<std::remove_cvref_t<T>>(struct_);
                case Structure::VTableMode::AttachADynamicVirtualTable:
                    return CXX::Interface::getExtractAsDynamic<std::remove_cvref_t<T>>(struct_);
                case Structure::VTableMode::CXX:
                    throw InvalidCast("Fail cast structure (CXX vtable not implemented)");
                default:
                    throw InvalidCast("Fail cast structure (unknown vtable mode)");
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

    size_t getSize(void** value);

    void asValue(void** val, VType type);
    bool isValue(void** val, VType type);
    bool isTrueValue(void** value);
    void setBoolValue(bool, void** value);
}
#endif /* SRC_RUN_TIME_ATTACHA_ABI */
