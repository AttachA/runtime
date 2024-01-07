// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <string>

#include <attacha/configuration/agreement/symbols.hpp>
#include <run_time/AttachA_CXX.hpp>
#include <util/hash.hpp>

namespace art {

    bool needAlloc(ValueMeta type) {
        if (type.as_ref)
            return false;
        if (type.use_gc)
            return true;
        return needAllocType(type.vtype);
    }

    bool needAllocType(VType type) {
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
            return true;
        default:
            return false;
        }
    }

    bool calc_safe_depth_arr(void* ptr) {
        list_array<ValueItem>& items = *(list_array<ValueItem>*)ptr;
        for (ValueItem& it : items)
            if (it.meta.use_gc)
                if (!((lgr*)it.val)->depth_safety())
                    return false;
        return true;
    }

    void universalFree(void** value, ValueMeta meta) {
        if (!value)
            return;
        if (!*value)
            return;
        if (meta.as_ref)
            return;
        if (meta.use_gc)
            goto gc_destruct;
        switch (meta.vtype) {
        case VType::raw_arr_i8:
        case VType::raw_arr_ui8:
            delete[] (uint8_t*)*value;
            break;
        case VType::raw_arr_i16:
        case VType::raw_arr_ui16:
            delete[] (uint16_t*)*value;
            break;
        case VType::raw_arr_i32:
        case VType::raw_arr_ui32:
        case VType::raw_arr_flo:
            delete[] (uint32_t*)*value;
            break;
        case VType::raw_arr_i64:
        case VType::raw_arr_ui64:
        case VType::raw_arr_doub:
            delete[] (uint64_t*)*value;
            break;
        case VType::uarr:
            delete (list_array<ValueItem>*)*value;
            break;
        case VType::string:
            delete (art::ustring*)*value;
            break;
        case VType::async_res:
            delete (art::typed_lgr<Task>*)*value;
            break;
        case VType::generator:
            delete (art::shared_ptr<Generator>*)*value;
            break;
        case VType::undefined_ptr:
            break;
        case VType::except_value:
            delete (std::exception_ptr*)*value;
            break;
        case VType::faarr: {
            ValueItem* arr = (ValueItem*)*value;
            uint32_t count = meta.val_len;
            for (uint32_t i = 0; i < count; i++)
                universalFree((void**)&arr[i], arr[i].meta);
            delete[] (ValueItem*)*value;
            break;
        }
        case VType::struct_:
            Structure::destruct((Structure*)*value);
            break;
        case VType::function:
            delete (art::shared_ptr<FuncEnvironment>*)*value;
            break;
        default:
            if (needAlloc(meta))
                throw InvalidOperation("Fail free value, notify dev's via github, reason: used function for free value but value type is " + enum_to_string(meta.vtype) + " and it's not supported");
            break;
        }
        *value = nullptr;
        return;
    gc_destruct:
        delete *(lgr**)value;
    }

    void universalRemove(void** value) {
        ValueMeta& meta = *reinterpret_cast<ValueMeta*>(value + 1);
        if (!value)
            return;
        if (!*value)
            return;
        if (!meta.encoded)
            return;
        if (!meta.as_ref)
            if (needAlloc(meta))
                universalFree(value, meta);
        if (meta.vtype == VType::saarr) {
            ValueItem* begin = (ValueItem*)*value;
            uint32_t count = meta.val_len;
            while (count--)
                universalRemove((void**)begin++);
        }
        meta.encoded = 0;
        *value = nullptr;
    }

    void universalAlloc(void** value, ValueMeta meta) {
        if (*value)
            universalRemove(value);
        if (needAllocType(meta.vtype)) {
            switch (meta.vtype) {
            case VType::raw_arr_i8:
            case VType::raw_arr_ui8:
                *value = new uint8_t[meta.val_len];
                break;
            case VType::raw_arr_i16:
            case VType::raw_arr_ui16:
                *value = new uint16_t[meta.val_len];
                break;
            case VType::raw_arr_i32:
            case VType::raw_arr_ui32:
            case VType::raw_arr_flo:
                *value = new uint32_t[meta.val_len];
                break;
            case VType::raw_arr_ui64:
            case VType::raw_arr_i64:
            case VType::raw_arr_doub:
                *value = new uint64_t[meta.val_len];
                break;
            case VType::uarr:
                *value = new list_array<ValueItem>();
                break;
            case VType::string:
                *value = new art::ustring();
                break;
            case VType::except_value:
                try {
                    throw AException("Undefined exception", "No description");
                } catch (...) {
                    *value = new std::exception_ptr(std::current_exception());
                    break;
                }
            case VType::faarr:
                *value = new ValueItem[meta.val_len]();
                break;
            case VType::saarr:
                throw InvalidOperation("Fail allocate local stack value");
                break;
            default:
                break;
            }
        }
        if (meta.use_gc) {
            void (*destructor)(void*) = nullptr;
            bool (*depth)(void*) = nullptr;
            switch (meta.vtype) {
            case VType::noting:
                break;
            case VType::i8:
            case VType::ui8:
                destructor = defaultDestructor<uint8_t>;
                *value = new uint8_t(0);
                break;
            case VType::i16:
            case VType::ui16:
                destructor = defaultDestructor<uint16_t>;
                *value = new uint16_t(0);
                break;
            case VType::i32:
            case VType::ui32:
            case VType::flo:
                destructor = defaultDestructor<uint32_t>;
                *value = new uint32_t(0);
                break;
            case VType::i64:
            case VType::ui64:
            case VType::undefined_ptr:
            case VType::doub:
            case VType::type_identifier:
                destructor = defaultDestructor<uint64_t>;
                *value = new uint64_t(0);
                break;
            case VType::raw_arr_i8:
            case VType::raw_arr_ui8:
                destructor = arrayDestructor<uint8_t>;
                break;
            case VType::raw_arr_i16:
            case VType::raw_arr_ui16:
                destructor = arrayDestructor<uint16_t>;
                break;
            case VType::raw_arr_i32:
            case VType::raw_arr_ui32:
            case VType::raw_arr_flo:
                destructor = arrayDestructor<uint32_t>;
                break;
            case VType::raw_arr_i64:
            case VType::raw_arr_ui64:
            case VType::raw_arr_doub:
                destructor = arrayDestructor<uint64_t>;
                break;
            case VType::uarr:
                destructor = defaultDestructor<list_array<ValueItem>>;
                depth = calc_safe_depth_arr;
                break;
            case VType::string:
                destructor = defaultDestructor<art::ustring>;
                break;
            case VType::except_value:
                destructor = defaultDestructor<std::exception_ptr>;
                break;
            case VType::faarr:
                destructor = arrayDestructor<ValueItem>;
                break;
            case VType::saarr:
                throw InvalidOperation("Fail allocate local stack value");
                break;
            case VType::async_res:
                destructor = defaultDestructor<art::typed_lgr<Task>>;
                break;
            case VType::generator:
                destructor = defaultDestructor<art::shared_ptr<Generator>>;
                break;
            case VType::function:
                destructor = defaultDestructor<art::shared_ptr<FuncEnvironment>>;
                break;
            default:
                break;
            }
            *value = new lgr(value, depth, destructor);
        }
        *(value + 1) = (void*)meta.encoded;
    }

    void removeArgsEnvironnement(list_array<ValueItem>* env) {
        delete env;
    }

    ValueItem* getAsyncValueItem(void* val) {
        if (!val)
            return nullptr;
        art::typed_lgr<Task>& tmp = *(art::typed_lgr<Task>*)val;
        if (!tmp) {
            return nullptr;
        }
        auto res = Task::await_results(tmp);
        if (res.size() == 1)
            return new ValueItem(std::move(res[0]));
        else
            return new ValueItem(std::move(res));
    }

    void getValueItem(void** value, ValueItem* f_res) {
        universalRemove(value);
        if (f_res) {
            *value = f_res->val;
            *(value + 1) = (void*)f_res->meta.encoded;
            f_res->val = nullptr;
            f_res->meta = 0;
            delete f_res;
        }
    }

    ValueItem* buildRes(void** value) {
        try {
            return new ValueItem(*(ValueItem*)value);
        } catch (const std::bad_alloc&) {
            throw EnvironmentRuinException();
        }
    }

    ValueItem* buildResTake(void** value) {
        try {
            return new ValueItem(std::move(*(ValueItem*)value));
        } catch (const std::bad_alloc&) {
            throw EnvironmentRuinException();
        }
    }

    void getAsyncResult(void*& value, ValueMeta& meta) {
        while (meta.vtype == VType::async_res) {
            ValueItem* vaal = (ValueItem*)&value;
            ValueItem* res = nullptr;
            if (!value)
                return;
            try {
                res = getAsyncValueItem(meta.use_gc ? ((lgr*)value)->getPtr() : value);
                if (res) {
                    *vaal = std::move(*res);
                    delete res;
                }
            } catch (...) {
                if (res)
                    delete res;
            }
        }
    }

    void* copyValue(void*& val, ValueMeta& meta) {
        getAsyncResult(val, meta);
        if (meta.as_ref)
            return val;
        void* actual_val = val;
        if (meta.use_gc)
            actual_val = ((lgr*)val)->getPtr();
        if (actual_val == nullptr)
            return nullptr;
        if (needAllocType(meta.vtype)) {
            switch (meta.vtype) {
            case VType::raw_arr_i8:
            case VType::raw_arr_ui8: {
                uint8_t* cop = new uint8_t[meta.val_len];
                memcpy(cop, actual_val, meta.val_len);
                return cop;
            }
            case VType::raw_arr_i16:
            case VType::raw_arr_ui16: {
                uint16_t* cop = new uint16_t[meta.val_len];
                memcpy(cop, actual_val, size_t(meta.val_len) * 2);
                return cop;
            }
            case VType::raw_arr_i32:
            case VType::raw_arr_ui32:
            case VType::raw_arr_flo: {
                uint32_t* cop = new uint32_t[meta.val_len];
                memcpy(cop, actual_val, size_t(meta.val_len) * 4);
                return cop;
            }
            case VType::raw_arr_i64:
            case VType::raw_arr_ui64:
            case VType::raw_arr_doub: {
                uint64_t* cop = new uint64_t[meta.val_len];
                memcpy(cop, actual_val, size_t(meta.val_len) * 8);
                return cop;
            }
            case VType::uarr:
                return new list_array<ValueItem>(*(list_array<ValueItem>*)actual_val);
            case VType::string:
                return new art::ustring(*(art::ustring*)actual_val);
            case VType::async_res:
                return new art::typed_lgr<Task>(*(art::typed_lgr<Task>*)actual_val);
            case VType::generator:
                return new art::shared_ptr<Generator>(*(art::shared_ptr<Generator>*)actual_val);
            case VType::except_value:
                return new std::exception_ptr(*(std::exception_ptr*)actual_val);
            case VType::saarr:
            case VType::faarr: {
                ValueItem* cop = new ValueItem[meta.val_len]();
                for (uint32_t i = 0; i < meta.val_len; i++)
                    cop[i] = reinterpret_cast<ValueItem*>(val)[i];
                return cop;
            }
            case VType::struct_:
                return Structure::copy((Structure*)actual_val);
            case VType::function:
                return new art::shared_ptr<FuncEnvironment>(*(art::shared_ptr<FuncEnvironment>*)actual_val);
            default:
                throw NotImplementedException();
            }
        } else
            return actual_val;
    }

    void** preSetValue(void** value, ValueMeta set_meta, bool match_gc_dif) {
        void** res = &getValue(value);
        ValueMeta& meta = *(ValueMeta*)(value + 1);
        if (!needAlloc(meta)) {
            if (match_gc_dif) {
                if (meta.allow_edit && meta.vtype == set_meta.vtype && meta.use_gc == set_meta.use_gc)
                    return res;
            } else if (meta.allow_edit && meta.vtype == set_meta.vtype)
                return res;
        }
        if (!meta.as_ref)
            universalRemove(value);
        *(value + 1) = (void*)set_meta.encoded;
        return &getValue(value);
    }

    void*& getValue(void*& value, ValueMeta& meta) {
        if (meta.vtype == VType::async_res)
            getAsyncResult(value, meta);
        if (meta.use_gc) {
            if (!value) {
                meta = ValueMeta(0);
                return value;
            }
            if (((lgr*)value)->is_deleted()) {
                universalRemove(&value);
                meta = ValueMeta(0);
            }
        }
        return meta.use_gc ? (**(lgr*)value) : value;
    }

    const void* const& getValue(const void* const& value, const ValueMeta& meta) {
        if (meta.use_gc) {
            if (!value) {
                return value;
            }
            if (((lgr*)value)->is_deleted())
                universalRemove((void**)&value);
        }
        return meta.use_gc ? (**(const lgr*)value) : value;
    }

    void*& getValue(void** value) {
        ValueMeta& meta = *(ValueMeta*)(value + 1);
        return getValue(*value, meta);
    }

    void* getSpecificValue(void** value, VType typ) {
        ValueMeta& meta = *(ValueMeta*)(value + 1);
        if (meta.vtype == VType::async_res)
            getAsyncResult(*value, meta);
        if (meta.vtype != typ)
            throw InvalidType("Requested specifed type but received another");
        if (meta.use_gc)
            if (((lgr*)value)->is_deleted()) {
                universalRemove(value);
                meta = ValueMeta(0);
            }
        return meta.use_gc ? ((lgr*)value)->getPtr() : *value;
    }

    void** getSpecificValueLink(void** value, VType typ) {
        ValueMeta& meta = *(ValueMeta*)(value + 1);
        if (meta.vtype == VType::async_res)
            getAsyncResult(*value, meta);
        if (meta.vtype != typ)
            throw InvalidType("Requested specifed type but received another");
        if (meta.use_gc)
            if (((lgr*)value)->is_deleted()) {
                universalFree(value, meta);
                meta = ValueMeta(0);
            }
        return meta.use_gc ? (&**(lgr*)value) : value;
    }

    bool is_integer(VType typ) {
        switch (typ) {
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
            return true;
        default:
            return false;
        }
    }

    bool integer_unsigned(VType typ) {
        switch (typ) {
        case VType::ui8:
        case VType::ui16:
        case VType::ui32:
        case VType::ui64:
            return true;
        default:
            return false;
        }
    }

    bool integer_floating(VType typ) {
        switch (typ) {
        case VType::flo:
        case VType::doub:
            return true;
        default:
            return false;
        }
    }

    bool is_raw_array(VType typ) {
        switch (typ) {
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
        case VType::faarr:
        case VType::saarr:
            return true;
        default:
            return false;
        }
    }

    bool has_interface(VType typ) {
        switch (typ) {
        case VType::struct_:
            return true;
        default:
            return false;
        }
    }

#pragma warning(push)
#pragma warning(disable : 4311)
#pragma warning(disable : 4302)

    std::pair<bool, bool> compareArrays(ValueMeta cmp1, ValueMeta cmp2, void* val1, void* val2) {
        if (!calc_safe_depth_arr(val1))
            return {false, false};
        else if (!calc_safe_depth_arr(val2))
            return {false, false};
        else {
            auto& arr1 = *(list_array<ValueItem>*)val1;
            auto& arr2 = *(list_array<ValueItem>*)val2;
            if (arr1.size() < arr2.size())
                return {false, true};
            else if (arr1.size() == arr2.size()) {
                auto tmp = arr2.begin();
                for (auto& it : arr1) {
                    auto res = compareValue(it.meta, tmp->meta, it.val, tmp->val);
                    if (!res.first)
                        return res;
                    ++tmp;
                }
                return {true, false};
            } else
                return {false, false};
        }
    }

    //uarr and raw_arr_* or faarr/saarr
    std::pair<bool, bool> compareUarrARawArr(ValueMeta cmp1, ValueMeta cmp2, void* val1, void* val2, bool flip_args = false) {
        if (!calc_safe_depth_arr(val1))
            return {false, false};
        else {
            auto& arr1 = *(list_array<ValueItem>*)val1;
            if (arr1.size() < cmp2.val_len)
                return {false, true};
            else if (arr1.size() == cmp2.val_len) {
                switch (cmp2.vtype) {
                case VType::raw_arr_i8: {
                    auto arr2 = (int8_t*)val2;
                    for (auto& it : arr1) {
                        auto first = *arr2;
                        auto res = compareValue(it.meta, VType::i8, it.val, &first);
                        if (!res.first)
                            return flip_args ? std::pair<bool, bool>{false, res.second} : res;
                        ++arr2;
                    }
                    break;
                }
                case VType::raw_arr_i16: {
                    auto arr2 = (int16_t*)val2;
                    for (auto& it : arr1) {
                        auto first = *arr2;
                        auto res = compareValue(it.meta, VType::i16, it.val, &first);
                        if (!res.first)
                            return flip_args ? std::pair<bool, bool>{false, res.second} : res;
                        ++arr2;
                    }
                    break;
                }
                case VType::raw_arr_i32: {
                    auto arr2 = (int32_t*)val2;
                    for (auto& it : arr1) {
                        auto first = *arr2;
                        auto res = compareValue(it.meta, VType::i32, it.val, &first);
                        if (!res.first)
                            return flip_args ? std::pair<bool, bool>{false, res.second} : res;
                        ++arr2;
                    }
                    break;
                }
                case VType::raw_arr_i64: {
                    auto arr2 = (int64_t*)val2;
                    for (auto& it : arr1) {
                        auto first = *arr2;
                        auto res = compareValue(it.meta, VType::i64, it.val, &first);
                        if (!res.first)
                            return flip_args ? std::pair<bool, bool>{false, res.second} : res;
                        ++arr2;
                    }
                    break;
                }
                case VType::raw_arr_ui8: {
                    auto arr2 = (uint8_t*)val2;
                    for (auto& it : arr1) {
                        auto first = *arr2;
                        auto res = compareValue(it.meta, VType::ui8, it.val, &first);
                        if (!res.first)
                            return flip_args ? std::pair<bool, bool>{false, res.second} : res;
                        ++arr2;
                    }
                    break;
                }
                case VType::raw_arr_ui16: {
                    auto arr2 = (uint16_t*)val2;
                    for (auto& it : arr1) {
                        auto first = *arr2;
                        auto res = compareValue(it.meta, VType::ui16, it.val, &first);
                        if (!res.first)
                            return flip_args ? std::pair<bool, bool>{false, res.second} : res;
                        ++arr2;
                    }
                    break;
                }
                case VType::raw_arr_ui32: {
                    auto arr2 = (uint32_t*)val2;
                    for (auto& it : arr1) {
                        auto first = *arr2;
                        auto res = compareValue(it.meta, VType::ui32, it.val, &first);
                        if (!res.first)
                            return flip_args ? std::pair<bool, bool>{false, res.second} : res;
                        ++arr2;
                    }
                    break;
                }
                case VType::raw_arr_ui64: {
                    auto arr2 = (uint64_t*)val2;
                    for (auto& it : arr1) {
                        auto first = *arr2;
                        auto res = compareValue(it.meta, VType::ui64, it.val, &first);
                        if (!res.first)
                            return flip_args ? std::pair<bool, bool>{false, res.second} : res;
                        ++arr2;
                    }
                    break;
                }
                case VType::raw_arr_flo: {
                    auto arr2 = (float*)val2;
                    for (auto& it : arr1) {
                        auto first = *arr2;
                        auto res = compareValue(it.meta, VType::flo, it.val, &first);
                        if (!res.first)
                            return flip_args ? std::pair<bool, bool>{false, res.second} : res;
                        ++arr2;
                    }
                    break;
                }
                case VType::raw_arr_doub: {
                    auto arr2 = (double*)val2;
                    for (auto& it : arr1) {
                        auto first = *arr2;
                        auto res = compareValue(it.meta, VType::doub, it.val, &first);
                        if (!res.first)
                            return flip_args ? std::pair<bool, bool>{false, res.second} : res;
                        ++arr2;
                    }
                    break;
                }
                case VType::faarr:
                case VType::saarr: {
                    auto arr2 = (ValueItem*)val2;
                    for (auto& it : arr1) {
                        auto res = compareValue(it.meta, arr2->meta, it.val, arr2->val);
                        if (!res.first)
                            return flip_args ? std::pair<bool, bool>{false, res.second} : res;
                        ++arr2;
                    }
                    break;
                }
                default:
                    throw InvalidOperation("Wrong compare operation, notify dev's via github, reason: used function for compare uarr and raw_arr_* but second operand is actually " + enum_to_string(cmp2.vtype));
                }
                return {true, false};
            } else
                return {false, false};
        }
    }

    //uarr and raw_arr_* or faarr/saarr
    std::pair<bool, bool> compareUarrAInterface(ValueMeta cmp1, ValueMeta cmp2, void* val1, void* val2, bool flip_args = false) {
        if (!calc_safe_depth_arr(val1))
            return {false, false};
        else {
            auto& arr1 = *(list_array<ValueItem>*)val1;
            uint64_t length;
            switch (cmp2.vtype) {
            case VType::struct_:
                length = (uint64_t)art::CXX::Interface::makeCall(ClassAccess::pub, *(Structure*)val2, symbols::structures::size);
                break;
            default:
                throw AException("Implementation exception", "Wrong function usage compareUarrAInterface");
            }
            if (arr1.size() < length)
                return {false, true};
            else if (arr1.size() == length) {
                auto iter = arr1.begin();
                if ((*(Structure*)val2).has_method(symbols::structures::iterable::begin, ClassAccess::pub)) {
                    auto iter2 = art::CXX::Interface::makeCall(ClassAccess::pub, *(Structure*)val2, symbols::structures::iterable::begin);
                    for (uint64_t i = 0; i < length; i++) {
                        auto& it1 = *iter;
                        auto it2 = art::CXX::Interface::makeCall(ClassAccess::pub, iter2, symbols::structures::iterable::next);
                        auto res = compareValue(it1.meta, it2.meta, it1.val, it2.val);
                        if (!res.first)
                            return flip_args ? std::pair<bool, bool>{false, res.second} : res;
                        ++iter;
                    }
                } else if ((*(Structure*)val2).has_method(symbols::structures::index_operator, ClassAccess::pub)) {
                    for (uint64_t i = 0; i < length; i++) {
                        auto& it1 = *iter;
                        auto it2 = art::CXX::Interface::makeCall(ClassAccess::pub, *(Structure*)val2, symbols::structures::index_operator, i);
                        auto res = compareValue(it1.meta, it2.meta, it1.val, it2.val);
                        if (!res.first)
                            return flip_args ? std::pair<bool, bool>{false, res.second} : res;
                        ++iter;
                    }
                } else
                    throw NotImplementedException();

                return {true, false};
            } else
                return {false, false};
        }
    }

    template <class T, VType T_VType>
    std::tuple<bool, bool, bool> compareRawArrAInterface_Worst0(void* val1, void* val2, uint64_t length) {
        auto arr2 = (T*)val2;
        if ((*(Structure*)val2).has_method(symbols::structures::iterable::begin, ClassAccess::pub)) {
            auto iter2 = art::CXX::Interface::makeCall(ClassAccess::pub, *(Structure*)val2, symbols::structures::iterable::begin);
            for (uint64_t i = 0; i < length; i++) {
                T first = *arr2;
                ValueItem it2 = art::CXX::Interface::makeCall(ClassAccess::pub, iter2, symbols::structures::iterable::next);
                auto res = compareValue(T_VType, it2.meta, &first, it2.val);
                if (!res.first)
                    return {false, res.second, true};
                ++arr2;
            }
        } else if ((*(Structure*)val2).has_method(symbols::structures::index_operator, ClassAccess::pub)) {
            for (uint64_t i = 0; i < length; i++) {
                T first = *arr2;
                ValueItem it2 = art::CXX::Interface::makeCall(ClassAccess::pub, *(Structure*)val2, symbols::structures::index_operator, i);
                auto res = compareValue(T_VType, it2.meta, &first, it2.val);
                if (!res.first)
                    return {false, res.second, true};
                ++arr2;
            }
        } else
            throw NotImplementedException();
        return {false, false, false};
    }

    std::tuple<bool, bool, bool> compareRawArrAInterface_Worst1(void* val1, void* val2, uint64_t length) {
        auto arr2 = (ValueItem*)val2;
        if ((*(Structure*)val2).has_method(symbols::structures::iterable::begin, ClassAccess::pub)) {
            auto iter2 = art::CXX::Interface::makeCall(ClassAccess::pub, *(Structure*)val2, symbols::structures::iterable::begin);
            for (uint64_t i = 0; i < length; i++) {
                ValueItem& first = *arr2;
                ValueItem it2 = art::CXX::Interface::makeCall(ClassAccess::pub, iter2, symbols::structures::iterable::next);
                auto res = compareValue(first.meta, it2.meta, first.val, it2.val);
                if (!res.first)
                    return {false, res.second, true};
                ++arr2;
            }
        } else if ((*(Structure*)val2).has_method(symbols::structures::index_operator, ClassAccess::pub)) {
            for (uint64_t i = 0; i < length; i++) {
                ValueItem& first = *arr2;
                ValueItem it2 = art::CXX::Interface::makeCall(ClassAccess::pub, *(Structure*)val2, symbols::structures::index_operator, i);
                auto res = compareValue(first.meta, it2.meta, first.val, it2.val);
                if (!res.first)
                    return {false, res.second, true};
                ++arr2;
            }
        } else
            throw NotImplementedException();
        return {false, false, false};
    }

    std::pair<bool, bool> compareRawArrAInterface(ValueMeta cmp1, ValueMeta cmp2, void* val1, void* val2, bool flip_args = false) {
        if (!calc_safe_depth_arr(val1))
            return {false, false};
        else {
            auto& arr1 = *(list_array<ValueItem>*)val1;
            uint64_t length;
            switch (cmp2.vtype) {
            case VType::struct_:
                length = (uint64_t)art::CXX::Interface::makeCall(ClassAccess::pub, *(Structure*)val2, symbols::structures::size);
                break;
            default:
                throw AException("Implementation exception", "Wrong function usage compareRawArrAInterface");
            }
            if (arr1.size() < length)
                return {false, true};
            else if (arr1.size() == length) {
                std::tuple<bool, bool, bool> res;
                switch (cmp2.vtype) {
                case VType::raw_arr_i8:
                    res = compareRawArrAInterface_Worst0<int8_t, VType::i8>(val1, val2, length);
                    break;
                case VType::raw_arr_i16:
                    res = compareRawArrAInterface_Worst0<int16_t, VType::i16>(val1, val2, length);
                    break;
                case VType::raw_arr_i32:
                    res = compareRawArrAInterface_Worst0<int32_t, VType::i32>(val1, val2, length);
                    break;
                case VType::raw_arr_i64:
                    res = compareRawArrAInterface_Worst0<int64_t, VType::i64>(val1, val2, length);
                    break;
                case VType::raw_arr_ui8:
                    res = compareRawArrAInterface_Worst0<uint8_t, VType::ui8>(val1, val2, length);
                    break;
                case VType::raw_arr_ui16:
                    res = compareRawArrAInterface_Worst0<uint16_t, VType::ui16>(val1, val2, length);
                    break;
                case VType::raw_arr_ui32:
                    res = compareRawArrAInterface_Worst0<uint32_t, VType::ui32>(val1, val2, length);
                    break;
                case VType::raw_arr_ui64:
                    res = compareRawArrAInterface_Worst0<uint64_t, VType::ui64>(val1, val2, length);
                    break;
                case VType::raw_arr_flo:
                    res = compareRawArrAInterface_Worst0<float, VType::flo>(val1, val2, length);
                    break;
                case VType::raw_arr_doub:
                    res = compareRawArrAInterface_Worst0<double, VType::doub>(val1, val2, length);
                    break;
                case VType::faarr:
                case VType::saarr:
                    res = compareRawArrAInterface_Worst1(val1, val2, length);
                    break;
                default:
                    throw InvalidOperation("Wrong compare operation, notify dev's via github, reason: used function for compare uarr and raw_arr_* but second operand is actually " + enum_to_string(cmp2.vtype));
                }
                auto& [eq, low, has_res] = res;
                if (has_res)
                    return {eq, low};
                else
                    return {true, false};
            } else
                return {false, false};
        }
    }

    //return equal,lower bool result
    std::pair<bool, bool> compareValue(ValueMeta cmp1, ValueMeta cmp2, void* val1, void* val2) {
        bool cmp_int;
        if ((cmp_int = (is_integer(cmp1.vtype) || cmp1.vtype == VType::undefined_ptr)) != (is_integer(cmp2.vtype) || cmp1.vtype == VType::undefined_ptr))
            return {false, false};

        if (cmp_int) {
            if (val1 == val2)
                return {true, false};

            bool temp1 = (integer_unsigned(cmp1.vtype) || cmp1.vtype == VType::undefined_ptr);
            bool temp2 = (integer_unsigned(cmp2.vtype) || cmp2.vtype == VType::undefined_ptr);
            if (temp1 && temp2)
                return {false, uint64_t(val1) < uint64_t(val2)};
            else if (temp1)
                switch (cmp2.vtype) {
                case VType::i8:
                    return {false, uint64_t(val1) < int8_t((ptrdiff_t)val2)};
                case VType::i16:
                    return {false, uint64_t(val1) < int16_t((ptrdiff_t)val2)};
                case VType::i32:
                    return {false, uint64_t(val1) < int32_t((ptrdiff_t)val2)};
                case VType::i64:
                    return {false, int64_t(val2) < 0 ? false : uint64_t(val1) < uint64_t(val2)};
                case VType::flo:
                    return {false, uint64_t(val1) < *(float*)&val2};
                case VType::doub:
                    return {false, uint64_t(val1) < *(double*)&val2};
                default:
                    break;
                }
            else if (temp2)
                switch (cmp1.vtype) {
                case VType::i8:
                    return {false, int8_t((ptrdiff_t)val1) < uint64_t(val2)};
                case VType::i16:
                    return {false, int16_t((ptrdiff_t)val1) < uint64_t(val2)};
                case VType::i32:
                    return {false, int32_t((ptrdiff_t)val1) < uint64_t(val2)};
                case VType::i64:
                    return {false, int64_t(val1) < 0 ? true : uint64_t(val1) < uint64_t(val2)};
                case VType::flo:
                    return {false, *(float*)&val1 < uint64_t(val2)};
                case VType::doub:
                    return {false, *(double*)&val1 < uint64_t(val2)};
                default:
                    break;
                }
            else
                switch (cmp1.vtype) {
                case VType::i8:
                    switch (cmp2.vtype) {
                    case VType::i8:
                        return {false, int8_t((ptrdiff_t)val1) < int8_t((ptrdiff_t)val2)};
                    case VType::i16:
                        return {false, int8_t((ptrdiff_t)val1) < int16_t((ptrdiff_t)val2)};
                    case VType::i32:
                        return {false, int8_t((ptrdiff_t)val1) < int32_t((ptrdiff_t)val2)};
                    case VType::i64:
                        return {false, int8_t((ptrdiff_t)val1) < int64_t((ptrdiff_t)val2)};
                    case VType::flo:
                        return {false, int8_t((ptrdiff_t)val1) < *(float*)&(val2)};
                    case VType::doub:
                        return {false, int8_t((ptrdiff_t)val1) < *(double*)&(val2)};
                    default:
                        break;
                    }
                    break;
                case VType::i16:
                    switch (cmp2.vtype) {
                    case VType::i8:
                        return {false, int16_t((ptrdiff_t)val1) < int8_t((ptrdiff_t)val2)};
                    case VType::i16:
                        return {false, int16_t((ptrdiff_t)val1) < int16_t((ptrdiff_t)val2)};
                    case VType::i32:
                        return {false, int16_t((ptrdiff_t)val1) < int32_t((ptrdiff_t)val2)};
                    case VType::i64:
                        return {false, int16_t((ptrdiff_t)val1) < int64_t((ptrdiff_t)val2)};
                    case VType::flo:
                        return {false, int16_t((ptrdiff_t)val1) < *(float*)&(val2)};
                    case VType::doub:
                        return {false, int16_t((ptrdiff_t)val1) < *(double*)&(val2)};
                    default:
                        break;
                    }
                    break;
                case VType::i32:
                    switch (cmp2.vtype) {
                    case VType::i8:
                        return {false, int32_t((ptrdiff_t)val1) < int8_t((ptrdiff_t)val2)};
                    case VType::i16:
                        return {false, int32_t((ptrdiff_t)val1) < int16_t((ptrdiff_t)val2)};
                    case VType::i32:
                        return {false, int32_t((ptrdiff_t)val1) < int32_t((ptrdiff_t)val2)};
                    case VType::i64:
                        return {false, int32_t((ptrdiff_t)val1) < int64_t((ptrdiff_t)val2)};
                    case VType::flo:
                        return {false, int32_t((ptrdiff_t)val1) < *(float*)&(val2)};
                    case VType::doub:
                        return {false, int32_t((ptrdiff_t)val1) < *(double*)&(val2)};
                    default:
                        break;
                    }

                    break;
                case VType::i64:
                    switch (cmp2.vtype) {
                    case VType::i8:
                        return {false, int64_t(val1) < int8_t((ptrdiff_t)val2)};
                    case VType::i16:
                        return {false, int64_t(val1) < int16_t((ptrdiff_t)val2)};
                    case VType::i32:
                        return {false, int64_t(val1) < int32_t((ptrdiff_t)val2)};
                    case VType::i64:
                        return {false, int64_t(val1) < int64_t((ptrdiff_t)val2)};
                    case VType::flo:
                        return {false, int64_t(val1) < *(float*)&(val2)};
                    case VType::doub:
                        return {false, int64_t(val1) < *(double*)&(val2)};
                    default:
                        break;
                    }
                    break;
                case VType::flo:
                    switch (cmp2.vtype) {
                    case VType::i8:
                        return {false, *(float*)&val1 < int8_t((ptrdiff_t)val2)};
                    case VType::i16:
                        return {false, *(float*)&val1 < int16_t((ptrdiff_t)val2)};
                    case VType::i32:
                        return {false, *(float*)&val1 < int32_t((ptrdiff_t)val2)};
                    case VType::i64:
                        return {false, *(float*)&val1 < int64_t((ptrdiff_t)val2)};
                    case VType::flo:
                        return {false, *(float*)&val1 < *(float*)&(val2)};
                    case VType::doub:
                        return {false, *(float*)&val1 < *(double*)&(val2)};
                    default:
                        break;
                    }
                    break;
                case VType::doub:
                    switch (cmp2.vtype) {
                    case VType::i8:
                        return {false, *(double*)&val1 < int8_t((ptrdiff_t)val2)};
                    case VType::i16:
                        return {false, *(double*)&val1 < int16_t((ptrdiff_t)val2)};
                    case VType::i32:
                        return {false, *(double*)&val1 < int32_t((ptrdiff_t)val2)};
                    case VType::i64:
                        return {false, *(double*)&val1 < int64_t((ptrdiff_t)val2)};
                    case VType::flo:
                        return {false, *(double*)&val1 < *(float*)&(val2)};
                    case VType::doub:
                        return {false, *(double*)&val1 < *(double*)&(val2)};
                    default:
                        break;
                    }

                default:
                    break;
                }
            return {false, false};
        } else if (cmp1.vtype == VType::string && cmp2.vtype == VType::string) {
            if (*(art::ustring*)val1 == *(art::ustring*)val2)
                return {true, false};
            else
                return {false, *(art::ustring*)val1 < *(art::ustring*)val2};
        } else if (cmp1.vtype == VType::uarr && cmp2.vtype == VType::uarr)
            return compareArrays(cmp1, cmp2, val1, val2);
        else if (cmp1.vtype == VType::uarr && is_raw_array(cmp2.vtype))
            return compareUarrARawArr(cmp1, cmp2, val1, val2);
        else if (is_raw_array(cmp1.vtype) && cmp2.vtype == VType::uarr)
            return compareUarrARawArr(cmp2, cmp1, val2, val1, true);
        else if (cmp1.vtype == VType::uarr && has_interface(cmp2.vtype))
            return compareUarrAInterface(cmp1, cmp2, val1, val2);
        else if (has_interface(cmp1.vtype) && cmp2.vtype == VType::uarr)
            return compareUarrAInterface(cmp2, cmp1, val2, val1, true);
        else if (is_raw_array(cmp1.vtype) && has_interface(cmp2.vtype))
            return compareRawArrAInterface(cmp1, cmp2, val1, val2);
        else if (has_interface(cmp1.vtype) && is_raw_array(cmp2.vtype))
            return compareRawArrAInterface(cmp2, cmp1, val2, val1, true);
        else
            return {cmp1.vtype == cmp2.vtype, false};
    }

    RFLAGS compare(RFLAGS old, void** value_1, void** value_2) {
        void* val1 = getValue(value_1);
        void* val2 = getValue(value_2);
        ValueMeta cmp1 = *(ValueMeta*)(value_1 + 1);
        ValueMeta cmp2 = *(ValueMeta*)(value_2 + 1);

        old.parity = old.auxiliary_carry = old.sign_f = old.overflow = 0;
        auto res = compareValue(cmp1, cmp2, val1, val2);
        old.zero = res.first;
        old.carry = res.second;
        return old;
    }

    RFLAGS link_compare(RFLAGS old, void** value_1, void** value_2) {
        void* val1 = getValue(value_1);
        void* val2 = getValue(value_2);
        ValueMeta cmp1 = *(ValueMeta*)(value_1 + 1);
        ValueMeta cmp2 = *(ValueMeta*)(value_2 + 1);

        old.parity = old.auxiliary_carry = old.sign_f = old.overflow = 0;
        if (*value_1 == *value_2) {
            old.zero = true;
            old.carry = false;
        } else {
            old.zero = false;

            old.carry = uint64_t(val1) < uint64_t(val2);
        }
        return old;
    }

    namespace ABI_IMPL {

        ValueItem* _Vcast_callFN(const void* ptr) {
            return FuncEnvironment::sync_call(*(const class art::shared_ptr<FuncEnvironment>*)ptr, nullptr, 0);
        }

        template <class T>
        art::ustring raw_arr_to_string(void* arr, size_t size) {
            art::ustring res = "*[";
            for (size_t i = 0; i < size; i++) {
                res += std::to_string(reinterpret_cast<T*>(arr)[i]);
                if (i != size - 1)
                    res += ", ";
            }
            res += "]";
            return res;
        }

        template <class T>
        art::ustring raw_arr_to_string(const void* arr, size_t size) {
            art::ustring res = "*[";
            for (size_t i = 0; i < size; i++) {
                res += std::to_string(reinterpret_cast<const T*>(arr)[i]);
                if (i != size - 1)
                    res += ", ";
            }
            res += "]";
            return res;
        }

        art::ustring Scast(void*& ref_val, ValueMeta& meta) {
            void* val = getValue(ref_val, meta);
            switch (meta.vtype) {
            case VType::noting:
                return "noting";
            case VType::boolean:
                return *(bool*)val ? "true" : "false";
            case VType::i8:
                return std::to_string(*reinterpret_cast<int8_t*>(&val));
            case VType::i16:
                return std::to_string(*reinterpret_cast<int16_t*>(&val));
            case VType::i32:
                return std::to_string(*reinterpret_cast<int32_t*>(&val));
            case VType::i64:
                return std::to_string(*reinterpret_cast<int64_t*>(&val));
            case VType::ui8:
                return std::to_string(*reinterpret_cast<uint8_t*>(&val));
            case VType::ui16:
                return std::to_string(*reinterpret_cast<uint16_t*>(&val));
            case VType::ui32:
                return std::to_string(*reinterpret_cast<uint32_t*>(&val));
            case VType::ui64:
                return std::to_string(*reinterpret_cast<uint64_t*>(&val));
            case VType::flo:
                return std::to_string(*reinterpret_cast<float*>(&val));
            case VType::doub:
                return std::to_string(*reinterpret_cast<double*>(&val));
            case VType::raw_arr_i8:
                return raw_arr_to_string<int8_t>(val, meta.val_len);
            case VType::raw_arr_i16:
                return raw_arr_to_string<int16_t>(val, meta.val_len);
            case VType::raw_arr_i32:
                return raw_arr_to_string<int32_t>(val, meta.val_len);
            case VType::raw_arr_i64:
                return raw_arr_to_string<int64_t>(val, meta.val_len);
            case VType::raw_arr_ui8:
                return raw_arr_to_string<uint8_t>(val, meta.val_len);
            case VType::raw_arr_ui16:
                return raw_arr_to_string<uint16_t>(val, meta.val_len);
            case VType::raw_arr_ui32:
                return raw_arr_to_string<uint32_t>(val, meta.val_len);
            case VType::raw_arr_ui64:
                return raw_arr_to_string<uint64_t>(val, meta.val_len);
            case VType::raw_arr_flo:
                return raw_arr_to_string<float>(val, meta.val_len);
            case VType::raw_arr_doub:
                return raw_arr_to_string<double>(val, meta.val_len);
            case VType::faarr:
            case VType::saarr: {
                art::ustring res = "*[";
                for (uint32_t i = 0; i < meta.val_len; i++) {
                    ValueItem& it = reinterpret_cast<ValueItem*>(val)[i];
                    res += Scast(it.val, it.meta) + (i + 1 < meta.val_len ? ',' : ']');
                }
                if (!meta.val_len)
                    res += ']';
                return res;
            }
            case VType::string:
                return *reinterpret_cast<art::ustring*>(val);
            case VType::uarr: {
                art::ustring res("[");
                bool before = false;
                for (auto& it : *reinterpret_cast<list_array<ValueItem>*>(val)) {
                    if (before)
                        res += ',';
                    res += Scast(it.val, it.meta);
                    before = true;
                }
                res += ']';
                return res;
            }
            case VType::undefined_ptr:
                return "0x" + string_help::hexstr(val);
            case VType::type_identifier:
                return (*(ValueMeta*)&val).to_string();
            case VType::function:
                return (*reinterpret_cast<art::shared_ptr<FuncEnvironment>*>(val))->to_string();
            case VType::map: {
                art::ustring res("{");
                bool before = false;
                for (auto& it : *reinterpret_cast<std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>*>(val)) {
                    if (before)
                        res += ',';
                    ValueItem& key = const_cast<ValueItem&>(it.first);
                    ValueItem& item = it.second;
                    res += Scast(key.val, key.meta) + ':' + Scast(item.val, item.meta);
                    before = true;
                }
                res += '}';
                return res;
            }
            case VType::set: {
                art::ustring res("(");
                bool before = false;
                for (auto& it : *reinterpret_cast<std::unordered_set<ValueItem, art::hash<ValueItem>>*>(val)) {
                    if (before)
                        res += ',';
                    ValueItem& item = const_cast<ValueItem&>(it);
                    res += Scast(item.val, item.meta);
                    before = true;
                }
                res += ')';
                return res;
            }
            case VType::time_point:
                return "t(" + std::to_string(reinterpret_cast<std::chrono::high_resolution_clock::time_point*>(val)->time_since_epoch().count()) + ')';
            case VType::struct_:
                return (art::ustring)art::CXX::Interface::makeCall(ClassAccess::pub, *reinterpret_cast<Structure*>(val), symbols::structures::convert::to_string);
            default:
                throw InvalidCast("Fail cast undefined type");
            }
        }

        art::ustring Scast(const void* const& ref_val, const ValueMeta& meta) {
            const void* val = getValue(ref_val, meta);
            switch (meta.vtype) {
            case VType::noting:
                return "noting";
            case VType::boolean:
                return *(bool*)val ? "true" : "false";
            case VType::i8:
                return std::to_string(*reinterpret_cast<int8_t*>(&val));
            case VType::i16:
                return std::to_string(*reinterpret_cast<int16_t*>(&val));
            case VType::i32:
                return std::to_string(*reinterpret_cast<int32_t*>(&val));
            case VType::i64:
                return std::to_string(*reinterpret_cast<int64_t*>(&val));
            case VType::ui8:
                return std::to_string(*reinterpret_cast<uint8_t*>(&val));
            case VType::ui16:
                return std::to_string(*reinterpret_cast<uint16_t*>(&val));
            case VType::ui32:
                return std::to_string(*reinterpret_cast<uint32_t*>(&val));
            case VType::ui64:
                return std::to_string(*reinterpret_cast<uint64_t*>(&val));
            case VType::flo:
                return std::to_string(*reinterpret_cast<float*>(&val));
            case VType::doub:
                return std::to_string(*reinterpret_cast<double*>(&val));
            case VType::raw_arr_i8:
                return raw_arr_to_string<int8_t>(val, meta.val_len);
            case VType::raw_arr_i16:
                return raw_arr_to_string<int16_t>(val, meta.val_len);
            case VType::raw_arr_i32:
                return raw_arr_to_string<int32_t>(val, meta.val_len);
            case VType::raw_arr_i64:
                return raw_arr_to_string<int64_t>(val, meta.val_len);
            case VType::raw_arr_ui8:
                return raw_arr_to_string<uint8_t>(val, meta.val_len);
            case VType::raw_arr_ui16:
                return raw_arr_to_string<uint16_t>(val, meta.val_len);
            case VType::raw_arr_ui32:
                return raw_arr_to_string<uint32_t>(val, meta.val_len);
            case VType::raw_arr_ui64:
                return raw_arr_to_string<uint64_t>(val, meta.val_len);
            case VType::raw_arr_flo:
                return raw_arr_to_string<float>(val, meta.val_len);
            case VType::raw_arr_doub:
                return raw_arr_to_string<double>(val, meta.val_len);
            case VType::faarr:
            case VType::saarr: {
                art::ustring res = "*[";
                for (uint32_t i = 0; i < meta.val_len; i++) {
                    const ValueItem& it = reinterpret_cast<const ValueItem*>(val)[i];
                    res += Scast(it.val, it.meta) + (i + 1 < meta.val_len ? ',' : ']');
                }
                if (!meta.val_len)
                    res += ']';
                return res;
            }
            case VType::string:
                return *reinterpret_cast<const art::ustring*>(val);
            case VType::uarr: {
                art::ustring res("[");
                bool before = false;
                for (auto& it : *reinterpret_cast<const list_array<ValueItem>*>(val)) {
                    if (before)
                        res += ',';
                    res += Scast(it.val, it.meta);
                    before = true;
                }
                res += ']';
                return res;
            }
            case VType::undefined_ptr:
                return "0x" + string_help::hexstr(val);
            case VType::type_identifier:
                return enum_to_string(*(VType*)&val);
            case VType::function:
                return (*reinterpret_cast<const art::shared_ptr<FuncEnvironment>*>(val))->to_string();
            case VType::map: {
                art::ustring res("{");
                bool before = false;
                for (auto& it : *reinterpret_cast<const std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>*>(val)) {
                    if (before)
                        res += ',';
                    const ValueItem& key = it.first;
                    const ValueItem& item = it.second;
                    res += Scast(key.val, key.meta) + ':' + Scast(item.val, item.meta);
                    before = true;
                }
                res += '}';
                return res;
            }
            case VType::set: {
                art::ustring res("(");
                bool before = false;
                for (auto& it : *reinterpret_cast<const std::unordered_set<ValueItem, art::hash<ValueItem>>*>(val)) {
                    if (before)
                        res += ',';
                    ValueItem& item = const_cast<ValueItem&>(it);
                    res += Scast(item.val, item.meta);
                    before = true;
                }
                res += ')';
                return res;
            }
            case VType::time_point:
                return "t(" + std::to_string(reinterpret_cast<const std::chrono::high_resolution_clock::time_point*>(val)->time_since_epoch().count()) + ')';
            case VType::struct_:
                return (art::ustring)art::CXX::Interface::makeCall(ClassAccess::pub, *reinterpret_cast<const Structure*>(val), symbols::structures::convert::to_string);
            default:
                throw InvalidCast("Fail cast undefined type");
            }
        }

        list_array<ValueItem> string_to_array(const art::ustring& str, uint32_t start) {
            list_array<ValueItem> res;
            art::ustring tmp;
            bool in_str = false;
            for (uint32_t i = start; i < str.size(); i++) {
                if (str[i] == '"' && str[i - 1] != '\\')
                    in_str = !in_str;
                if (str[i] == ',' && !in_str) {
                    res.push_back(SBcast(tmp));
                    tmp.clear();
                } else
                    tmp += str[i];
            }
            if (tmp.size())
                res.push_back(SBcast(tmp));
            return res;
        }

        ValueItem SBcast(const art::ustring& str) {
            if (str == "noting" || str == "null" || str == "undefined")
                return nullptr;
            else if (str == "true")
                return true;
            else if (str == "false")
                return false;
            else if (str.starts_with('"') && str.ends_with('"'))
                return str.substr(1, str.size() - 2);
            else if (str.starts_with('\'') && str.ends_with('\'') && str.size() > 2)
                return str.substr(1, str.size() - 2);
            else if (str == "\'\\\'\'") //'\'' -> '
                return ValueItem("\'");
            else if (str.starts_with("0x"))
                return ValueItem((void*)std::stoull(str, nullptr, 16), VType::undefined_ptr);
            else if (str.starts_with('['))
                return ValueItem(string_to_array(str, 1));
            else if (str.starts_with("*[")) {
                auto tmp = string_to_array(str, 2);
                ValueItem res;
                res.meta.vtype = VType::faarr;
                res.meta.allow_edit = true;
                size_t size = 0;
                res.val = tmp.take_raw(size);
                res.meta.val_len = size;
                return res;
            } else if (str.starts_with('{')) {
                std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>> res;
                art::ustring key;
                art::ustring value;
                bool in_str = false;
                bool is_key = true;
                for (uint32_t i = 1; i < str.size(); i++) {
                    if (str[i] == '"' && str[i - 1] != '\\')
                        in_str = !in_str;
                    if (str[i] == ':' && !in_str) {
                        is_key = false;
                    } else if (str[i] == ',' && !in_str) {
                        res[SBcast(key)] = SBcast(value);
                        key.clear();
                        value.clear();
                        is_key = true;
                    } else {
                        if (is_key)
                            key += str[i];
                        else
                            value += str[i];
                    }
                }
                if (key.size())
                    res[SBcast(key)] = SBcast(value);
                return ValueItem(std::move(res));
            } else if (str.starts_with('(')) {
                std::unordered_set<ValueItem, art::hash<ValueItem>> res;
                art::ustring tmp;
                bool in_str = false;
                for (uint32_t i = 1; i < str.size(); i++) {
                    if (str[i] == '"' && str[i - 1] != '\\')
                        in_str = !in_str;
                    if (str[i] == ',' && !in_str) {
                        res.emplace(SBcast(tmp));
                        tmp.clear();
                    } else
                        tmp += str[i];
                }
                if (tmp.size())
                    res.emplace(SBcast(tmp));
                return ValueItem(std::move(res));
            } else if (str.starts_with("t(")) {
                art::ustring tmp;
                for (uint32_t i = 11; i < str.size(); i++) {
                    if (str[i] == ')')
                        break;
                    tmp += str[i];
                }
                auto res = std::chrono::high_resolution_clock::time_point(std::chrono::nanoseconds(std::stoull(tmp)));
                return ValueItem(*(void**)&res, VType::time_point);
            } else {
                try {
                    try {
                        try {
                            try {
                                try {
                                    try {
                                        int32_t res = std::stoi(str);
                                        return ValueItem(*(void**)&res, VType::i32);
                                    } catch (...) {
                                        uint32_t res = std::stoul(str);
                                        return ValueItem(*(void**)&res, VType::ui32);
                                    }
                                } catch (...) {
                                    int64_t res = std::stoll(str);
                                    return ValueItem(*(void**)&res, VType::i64);
                                }
                            } catch (...) {
                                uint64_t res = std::stoull(str);
                                return ValueItem(*(void**)&res, VType::ui64);
                            }
                        } catch (...) {
                            float res = std::stof(str);
                            return ValueItem(*(void**)&res, VType::flo);
                        }
                    } catch (...) {
                        double res = std::stod(str);
                        return ValueItem(*(void**)&res, VType::doub);
                    }
                } catch (...) {
                    return ValueItem(new art::ustring(str), VType::string);
                }
            }
        }

        template <class T>
        void setValue(const T& val, void** set_val) {
            universalRemove(set_val);
            if constexpr (std::is_same_v<T, int8_t*>) {
                *reinterpret_cast<int8_t**>(set_val) = val;
                *reinterpret_cast<ValueMeta*>(set_val + 1) = VType::i8;
            } else if constexpr (std::is_same_v<T, uint8_t*>) {
                *reinterpret_cast<uint8_t**>(set_val) = val;
                *reinterpret_cast<ValueMeta*>(set_val + 1) = VType::ui8;
            } else if constexpr (std::is_same_v<T, int16_t*>) {
                *reinterpret_cast<int16_t**>(set_val) = val;
                *reinterpret_cast<ValueMeta*>(set_val + 1) = VType::i16;
            } else if constexpr (std::is_same_v<T, uint16_t*>) {
                *reinterpret_cast<uint16_t**>(set_val) = val;
                *reinterpret_cast<ValueMeta*>(set_val + 1) = VType::ui16;
            } else if constexpr (std::is_same_v<T, int32_t*>) {
                *reinterpret_cast<int32_t**>(set_val) = val;
                *reinterpret_cast<ValueMeta*>(set_val + 1) = VType::i32;
            } else if constexpr (std::is_same_v<T, uint32_t*>) {
                *reinterpret_cast<uint32_t**>(set_val) = val;
                *reinterpret_cast<ValueMeta*>(set_val + 1) = VType::ui32;
            } else if constexpr (std::is_same_v<T, int64_t**>) {
                *reinterpret_cast<int64_t**>(set_val) = val;
                *reinterpret_cast<ValueMeta*>(set_val + 1) = VType::i64;
            } else if constexpr (std::is_same_v<T, uint64_t*>) {
                *reinterpret_cast<uint64_t**>(set_val) = val;
                *reinterpret_cast<ValueMeta*>(set_val + 1) = VType::ui64;
            } else if constexpr (std::is_same_v<T, bool>) {
                *reinterpret_cast<bool*>(set_val) = val;
                *reinterpret_cast<ValueMeta*>(set_val + 1) = VType::boolean;
            } else if constexpr (std::is_same_v<T, int8_t>) {
                *reinterpret_cast<int8_t*>(set_val) = val;
                *reinterpret_cast<ValueMeta*>(set_val + 1) = VType::i8;
            } else if constexpr (std::is_same_v<T, uint8_t>) {
                *reinterpret_cast<uint8_t*>(set_val) = val;
                *reinterpret_cast<ValueMeta*>(set_val + 1) = VType::ui8;
            } else if constexpr (std::is_same_v<T, int16_t>) {
                *reinterpret_cast<int16_t*>(set_val) = val;
                *reinterpret_cast<ValueMeta*>(set_val + 1) = VType::i16;
            } else if constexpr (std::is_same_v<T, uint16_t>) {
                *reinterpret_cast<uint16_t*>(set_val) = val;
                *reinterpret_cast<ValueMeta*>(set_val + 1) = VType::ui16;
            } else if constexpr (std::is_same_v<T, int32_t>) {
                *reinterpret_cast<int32_t*>(set_val) = val;
                *reinterpret_cast<ValueMeta*>(set_val + 1) = VType::i32;
            } else if constexpr (std::is_same_v<T, uint32_t>) {
                *reinterpret_cast<uint32_t*>(set_val) = val;
                *reinterpret_cast<ValueMeta*>(set_val + 1) = VType::ui32;
            } else if constexpr (std::is_same_v<T, int64_t>) {
                *reinterpret_cast<int64_t*>(set_val) = val;
                *reinterpret_cast<ValueMeta*>(set_val + 1) = VType::i64;
            } else if constexpr (std::is_same_v<T, uint64_t>) {
                *reinterpret_cast<uint64_t*>(set_val) = val;
                *reinterpret_cast<ValueMeta*>(set_val + 1) = VType::ui64;
            } else if constexpr (std::is_same_v<T, float>) {
                *reinterpret_cast<float*>(set_val) = val;
                *reinterpret_cast<ValueMeta*>(set_val + 1) = VType::flo;
            } else if constexpr (std::is_same_v<T, double>) {
                *reinterpret_cast<double*>(set_val) = val;
                *reinterpret_cast<ValueMeta*>(set_val + 1) = VType::doub;
            } else if constexpr (std::is_same_v<T, art::ustring>) {
                *reinterpret_cast<art::ustring**>(set_val) = new art::ustring(val);
                *reinterpret_cast<ValueMeta*>(set_val + 1) = VType::string;
            } else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
                *reinterpret_cast<list_array<ValueItem>**>(set_val) = new list_array<ValueItem>(val);
                *reinterpret_cast<ValueMeta*>(set_val + 1) = VType::uarr;
            } else if constexpr (std::is_same_v<T, void*>) {
                *reinterpret_cast<void**>(set_val) = val;
                *reinterpret_cast<ValueMeta*>(set_val + 1) = VType::undefined_ptr;
            } else
                throw NotImplementedException();
        }
    }

    void DynSum(void** val0, void** val1) {
        ValueItem& val0_r = *reinterpret_cast<ValueItem*>(val0);
        ValueItem& val1_r = *reinterpret_cast<ValueItem*>(val1);
        val0_r.getAsync();
        val1_r.getAsync();
        void*& actual_val0 = val0_r.getSourcePtr();
        void*& actual_val1 = val1_r.getSourcePtr();

        if (!val0_r.meta.allow_edit)
            throw UnmodifiableValue();

        switch (val0_r.meta.vtype) {
        case VType::noting:
            val0_r = val1_r;
            break;
        case VType::i8:
            reinterpret_cast<int8_t&>(actual_val0) += (int8_t)val1_r;
            break;
        case VType::i16:
            reinterpret_cast<int16_t&>(actual_val0) += (int16_t)val1_r;
            break;
        case VType::i32:
            reinterpret_cast<int32_t&>(actual_val0) += (int32_t)val1_r;
            break;
        case VType::i64:
            reinterpret_cast<int64_t&>(actual_val0) += (int64_t)val1_r;
            break;
        case VType::ui8:
            reinterpret_cast<uint8_t&>(actual_val0) += (uint8_t)val1_r;
            break;
        case VType::ui16:
            reinterpret_cast<uint16_t&>(actual_val0) += (uint16_t)val1_r;
            break;
        case VType::ui32:
            reinterpret_cast<uint32_t&>(actual_val0) += (uint32_t)val1_r;
            break;
        case VType::ui64:
            reinterpret_cast<uint64_t&>(actual_val0) += (uint64_t)val1_r;
            break;
        case VType::flo:
            reinterpret_cast<float&>(actual_val0) += (float)val1_r;
            break;
        case VType::doub:
            reinterpret_cast<double&>(actual_val0) += (double)val1_r;
            break;
        case VType::uarr:
            reinterpret_cast<list_array<ValueItem>*>(actual_val0)->push_back(val1_r);
            break;
        case VType::string:
            *reinterpret_cast<art::ustring*>(actual_val0) += (art::ustring)val1_r;
            break;
        case VType::undefined_ptr:
            reinterpret_cast<size_t&>(actual_val0) += (size_t)val1_r;
            break;
        case VType::time_point: {
            auto& val0_time = reinterpret_cast<std::chrono::high_resolution_clock::time_point&>(actual_val0);
            auto& val1_time = reinterpret_cast<std::chrono::high_resolution_clock::time_point&>(actual_val1);
            val0_time = val0_time + (std::chrono::nanoseconds)val1_time.time_since_epoch();
            break;
        }
        case VType::struct_:
            art::CXX::Interface::makeCall(ClassAccess::pub, val0_r, symbols::structures::add_operator, val1_r);
            break;
        default:
            throw InvalidCast("Fail cast value for add operation, cause value type is unsupported");
        }
    }

    void DynMinus(void** val0, void** val1) {
        ValueItem& val0_r = *reinterpret_cast<ValueItem*>(val0);
        ValueItem& val1_r = *reinterpret_cast<ValueItem*>(val1);
        val0_r.getAsync();
        val1_r.getAsync();
        void*& actual_val0 = val0_r.getSourcePtr();
        void*& actual_val1 = val1_r.getSourcePtr();

        if (!val0_r.meta.allow_edit)
            throw UnmodifiableValue();

        switch (val0_r.meta.vtype) {
        case VType::noting:
            val0_r = val1_r;
            break;
        case VType::i8:
            reinterpret_cast<int8_t&>(actual_val0) -= (int8_t)val1_r;
            break;
        case VType::i16:
            reinterpret_cast<int16_t&>(actual_val0) -= (int16_t)val1_r;
            break;
        case VType::i32:
            reinterpret_cast<int32_t&>(actual_val0) -= (int32_t)val1_r;
            break;
        case VType::i64:
            reinterpret_cast<int64_t&>(actual_val0) -= (int64_t)val1_r;
            break;
        case VType::ui8:
            reinterpret_cast<uint8_t&>(actual_val0) -= (uint8_t)val1_r;
            break;
        case VType::ui16:
            reinterpret_cast<uint16_t&>(actual_val0) -= (uint16_t)val1_r;
            break;
        case VType::ui32:
            reinterpret_cast<uint32_t&>(actual_val0) -= (uint32_t)val1_r;
            break;
        case VType::ui64:
            reinterpret_cast<uint64_t&>(actual_val0) -= (uint64_t)val1_r;
            break;
        case VType::flo:
            reinterpret_cast<float&>(actual_val0) -= (float)val1_r;
            break;
        case VType::doub:
            reinterpret_cast<double&>(actual_val0) -= (double)val1_r;
            break;
        case VType::uarr:
            reinterpret_cast<list_array<ValueItem>*>(actual_val0)->push_back(val1_r);
            break;
        case VType::string:
            *reinterpret_cast<art::ustring*>(actual_val0) = (art::ustring)val1_r + *reinterpret_cast<art::ustring*>(actual_val0);
            break;
        case VType::undefined_ptr:
            reinterpret_cast<size_t&>(actual_val0) -= (size_t)val1_r;
            break;
        case VType::time_point: {
            auto& val0_time = reinterpret_cast<std::chrono::high_resolution_clock::time_point&>(actual_val0);
            auto& val1_time = reinterpret_cast<std::chrono::high_resolution_clock::time_point&>(actual_val1);
            val0_time = val0_time - (std::chrono::nanoseconds)val1_time.time_since_epoch();
            break;
        }
        case VType::struct_:
            art::CXX::Interface::makeCall(ClassAccess::pub, val0_r, symbols::structures::subtract_operator, val1_r);
            break;
        default:
            throw InvalidCast("Fail cast value for minus operation, cause value type is unsupported");
        }
    }

    void DynMul(void** val0, void** val1) {
        ValueItem& val0_r = *reinterpret_cast<ValueItem*>(val0);
        ValueItem& val1_r = *reinterpret_cast<ValueItem*>(val1);
        val0_r.getAsync();
        val1_r.getAsync();
        void*& actual_val0 = val0_r.getSourcePtr();
        void*& actual_val1 = val1_r.getSourcePtr();

        if (!val0_r.meta.allow_edit)
            throw UnmodifiableValue();

        switch (val0_r.meta.vtype) {
        case VType::noting:
            val0_r = val1_r;
            break;
        case VType::i8:
            reinterpret_cast<int8_t&>(actual_val0) *= (int8_t)val1_r;
            break;
        case VType::i16:
            reinterpret_cast<int16_t&>(actual_val0) *= (int16_t)val1_r;
            break;
        case VType::i32:
            reinterpret_cast<int32_t&>(actual_val0) *= (int32_t)val1_r;
            break;
        case VType::i64:
            reinterpret_cast<int64_t&>(actual_val0) *= (int64_t)val1_r;
            break;
        case VType::ui8:
            reinterpret_cast<uint8_t&>(actual_val0) *= (uint8_t)val1_r;
            break;
        case VType::ui16:
            reinterpret_cast<uint16_t&>(actual_val0) *= (uint16_t)val1_r;
            break;
        case VType::ui32:
            reinterpret_cast<uint32_t&>(actual_val0) *= (uint32_t)val1_r;
            break;
        case VType::ui64:
            reinterpret_cast<uint64_t&>(actual_val0) *= (uint64_t)val1_r;
            break;
        case VType::flo:
            reinterpret_cast<float&>(actual_val0) *= (float)val1_r;
            break;
        case VType::doub:
            reinterpret_cast<double&>(actual_val0) *= (double)val1_r;
            break;
        case VType::uarr:
            reinterpret_cast<list_array<ValueItem>*>(actual_val0)->push_back(val1_r);
            break;
        case VType::undefined_ptr:
            reinterpret_cast<size_t&>(actual_val0) *= (size_t)val1_r;
            break;
        case VType::struct_:
            art::CXX::Interface::makeCall(ClassAccess::pub, val0_r, symbols::structures::multiply_operator, val1_r);
            break;
        default:
            throw InvalidCast("Fail cast value for multiply operation, cause value type is unsupported");
        }
    }

    void DynDiv(void** val0, void** val1) {
        ValueItem& val0_r = *reinterpret_cast<ValueItem*>(val0);
        ValueItem& val1_r = *reinterpret_cast<ValueItem*>(val1);
        val0_r.getAsync();
        val1_r.getAsync();
        void*& actual_val0 = val0_r.getSourcePtr();
        void*& actual_val1 = val1_r.getSourcePtr();

        if (!val0_r.meta.allow_edit)
            throw UnmodifiableValue();

        switch (val0_r.meta.vtype) {
        case VType::noting:
            val0_r = val1_r;
            break;
        case VType::i8:
            reinterpret_cast<int8_t&>(actual_val0) /= (int8_t)val1_r;
            break;
        case VType::i16:
            reinterpret_cast<int16_t&>(actual_val0) /= (int16_t)val1_r;
            break;
        case VType::i32:
            reinterpret_cast<int32_t&>(actual_val0) /= (int32_t)val1_r;
            break;
        case VType::i64:
            reinterpret_cast<int64_t&>(actual_val0) /= (int64_t)val1_r;
            break;
        case VType::ui8:
            reinterpret_cast<uint8_t&>(actual_val0) /= (uint8_t)val1_r;
            break;
        case VType::ui16:
            reinterpret_cast<uint16_t&>(actual_val0) /= (uint16_t)val1_r;
            break;
        case VType::ui32:
            reinterpret_cast<uint32_t&>(actual_val0) /= (uint32_t)val1_r;
            break;
        case VType::ui64:
            reinterpret_cast<uint64_t&>(actual_val0) /= (uint64_t)val1_r;
            break;
        case VType::flo:
            reinterpret_cast<float&>(actual_val0) /= (float)val1_r;
            break;
        case VType::doub:
            reinterpret_cast<double&>(actual_val0) /= (double)val1_r;
            break;
        case VType::undefined_ptr:
            reinterpret_cast<size_t&>(actual_val0) /= (size_t)val1_r;
            break;
        case VType::struct_:
            art::CXX::Interface::makeCall(ClassAccess::pub, val0_r, symbols::structures::divide_operator, val1_r);
            break;
        default:
            throw InvalidCast("Fail cast value for divide operation, cause value type is unsupported");
        }
    }

    void DynRest(void** val0, void** val1) {
        ValueItem& val0_r = *reinterpret_cast<ValueItem*>(val0);
        ValueItem& val1_r = *reinterpret_cast<ValueItem*>(val1);
        val0_r.getAsync();
        val1_r.getAsync();
        void*& actual_val0 = val0_r.getSourcePtr();
        void*& actual_val1 = val1_r.getSourcePtr();

        if (!val0_r.meta.allow_edit)
            throw UnmodifiableValue();

        switch (val0_r.meta.vtype) {
        case VType::noting:
            val0_r = val1_r;
            break;
        case VType::i8:
            reinterpret_cast<int8_t&>(actual_val0) %= (int8_t)val1_r;
            break;
        case VType::i16:
            reinterpret_cast<int16_t&>(actual_val0) %= (int16_t)val1_r;
            break;
        case VType::i32:
            reinterpret_cast<int32_t&>(actual_val0) %= (int32_t)val1_r;
            break;
        case VType::i64:
            reinterpret_cast<int64_t&>(actual_val0) %= (int64_t)val1_r;
            break;
        case VType::ui8:
            reinterpret_cast<uint8_t&>(actual_val0) %= (uint8_t)val1_r;
            break;
        case VType::ui16:
            reinterpret_cast<uint16_t&>(actual_val0) %= (uint16_t)val1_r;
            break;
        case VType::ui32:
            reinterpret_cast<uint32_t&>(actual_val0) %= (uint32_t)val1_r;
            break;
        case VType::ui64:
            reinterpret_cast<uint64_t&>(actual_val0) %= (uint64_t)val1_r;
            break;
        case VType::undefined_ptr:
            reinterpret_cast<size_t&>(actual_val0) %= (size_t)val1_r;
            break;
        case VType::struct_:
            art::CXX::Interface::makeCall(ClassAccess::pub, val0_r, symbols::structures::modulo_operator, val1_r);
            break;
        default:
            throw InvalidCast("Fail cast value for rest operation, cause value type is unsupported");
        }
    }

    void DynInc(void** val0) {
        ValueItem& val0_r = *reinterpret_cast<ValueItem*>(val0);
        val0_r.getAsync();
        void*& actual_val0 = val0_r.getSourcePtr();

        if (!val0_r.meta.allow_edit)
            throw UnmodifiableValue();

        switch (val0_r.meta.vtype) {
        case VType::noting:
            val0_r = 1;
            break;
        case VType::i8:
            reinterpret_cast<int8_t&>(actual_val0)++;
            break;
        case VType::i16:
            reinterpret_cast<int16_t&>(actual_val0)++;
            break;
        case VType::i32:
            reinterpret_cast<int32_t&>(actual_val0)++;
            break;
        case VType::i64:
            reinterpret_cast<int64_t&>(actual_val0)++;
            break;
        case VType::ui8:
            reinterpret_cast<uint8_t&>(actual_val0)++;
            break;
        case VType::ui16:
            reinterpret_cast<uint16_t&>(actual_val0)++;
            break;
        case VType::ui32:
            reinterpret_cast<uint32_t&>(actual_val0)++;
            break;
        case VType::ui64:
            reinterpret_cast<uint64_t&>(actual_val0)++;
            break;
        case VType::flo:
            reinterpret_cast<float&>(actual_val0)++;
            break;
        case VType::doub:
            reinterpret_cast<double&>(actual_val0)++;
            break;
        case VType::undefined_ptr:
            reinterpret_cast<size_t&>(actual_val0)++;
            break;
        case VType::struct_:
            art::CXX::Interface::makeCall(ClassAccess::pub, val0_r, symbols::structures::increment_operator);
            break;
        default:
            throw InvalidCast("Fail cast value for increment operation, cause value type is unsupported");
        }
    }

    void DynDec(void** val0) {
        ValueItem& val0_r = *reinterpret_cast<ValueItem*>(val0);
        val0_r.getAsync();
        void*& actual_val0 = val0_r.getSourcePtr();

        if (!val0_r.meta.allow_edit)
            throw UnmodifiableValue();

        switch (val0_r.meta.vtype) {
        case VType::noting:
            val0_r = -1;
            break;
        case VType::i8:
            reinterpret_cast<int8_t&>(actual_val0)--;
            break;
        case VType::i16:
            reinterpret_cast<int16_t&>(actual_val0)--;
            break;
        case VType::i32:
            reinterpret_cast<int32_t&>(actual_val0)--;
            break;
        case VType::i64:
            reinterpret_cast<int64_t&>(actual_val0)--;
            break;
        case VType::ui8:
            reinterpret_cast<uint8_t&>(actual_val0)--;
            break;
        case VType::ui16:
            reinterpret_cast<uint16_t&>(actual_val0)--;
            break;
        case VType::ui32:
            reinterpret_cast<uint32_t&>(actual_val0)--;
            break;
        case VType::ui64:
            reinterpret_cast<uint64_t&>(actual_val0)--;
            break;
        case VType::flo:
            reinterpret_cast<float&>(actual_val0)--;
            break;
        case VType::doub:
            reinterpret_cast<double&>(actual_val0)--;
            break;
        case VType::undefined_ptr:
            reinterpret_cast<size_t&>(actual_val0)--;
            break;
        case VType::struct_:
            art::CXX::Interface::makeCall(ClassAccess::pub, val0_r, symbols::structures::decrement_operator);
            break;
        default:
            throw InvalidCast("Fail cast value for decrement operation, cause value type is unsupported");
        }
    }

    void DynBitXor(void** val0, void** val1) {
        ValueItem& val0_r = *reinterpret_cast<ValueItem*>(val0);
        ValueItem& val1_r = *reinterpret_cast<ValueItem*>(val1);
        val0_r.getAsync();
        val1_r.getAsync();
        void*& actual_val0 = val0_r.getSourcePtr();
        void*& actual_val1 = val1_r.getSourcePtr();

        if (!val0_r.meta.allow_edit)
            throw UnmodifiableValue();

        switch (val0_r.meta.vtype) {
        case VType::noting:
            val0_r = val1_r;
            break;
        case VType::i8:
            reinterpret_cast<int8_t&>(actual_val0) ^= (int8_t)val1_r;
            break;
        case VType::i16:
            reinterpret_cast<int16_t&>(actual_val0) ^= (int16_t)val1_r;
            break;
        case VType::i32:
            reinterpret_cast<int32_t&>(actual_val0) ^= (int32_t)val1_r;
            break;
        case VType::i64:
            reinterpret_cast<int64_t&>(actual_val0) ^= (int64_t)val1_r;
            break;
        case VType::ui8:
            reinterpret_cast<uint8_t&>(actual_val0) ^= (uint8_t)val1_r;
            break;
        case VType::ui16:
            reinterpret_cast<uint16_t&>(actual_val0) ^= (uint16_t)val1_r;
            break;
        case VType::ui32:
            reinterpret_cast<uint32_t&>(actual_val0) ^= (uint32_t)val1_r;
            break;
        case VType::ui64:
            reinterpret_cast<uint64_t&>(actual_val0) ^= (uint64_t)val1_r;
            break;
        case VType::undefined_ptr:
            reinterpret_cast<size_t&>(actual_val0) ^= (size_t)val1_r;
            break;
        case VType::struct_:
            art::CXX::Interface::makeCall(ClassAccess::pub, val0_r, symbols::structures::bitwise_xor_operator, val1_r);
            break;
        default:
            throw InvalidCast("Fail cast value for xor operation, cause value type is unsupported");
        }
    }

    void DynBitOr(void** val0, void** val1) {
        ValueItem& val0_r = *reinterpret_cast<ValueItem*>(val0);
        ValueItem& val1_r = *reinterpret_cast<ValueItem*>(val1);
        val0_r.getAsync();
        val1_r.getAsync();
        void*& actual_val0 = val0_r.getSourcePtr();
        void*& actual_val1 = val1_r.getSourcePtr();

        if (!val0_r.meta.allow_edit)
            throw UnmodifiableValue();

        switch (val0_r.meta.vtype) {
        case VType::noting:
            val0_r = val1_r;
            break;
        case VType::i8:
            reinterpret_cast<int8_t&>(actual_val0) |= (int8_t)val1_r;
            break;
        case VType::i16:
            reinterpret_cast<int16_t&>(actual_val0) |= (int16_t)val1_r;
            break;
        case VType::i32:
            reinterpret_cast<int32_t&>(actual_val0) |= (int32_t)val1_r;
            break;
        case VType::i64:
            reinterpret_cast<int64_t&>(actual_val0) |= (int64_t)val1_r;
            break;
        case VType::ui8:
            reinterpret_cast<uint8_t&>(actual_val0) |= (uint8_t)val1_r;
            break;
        case VType::ui16:
            reinterpret_cast<uint16_t&>(actual_val0) |= (uint16_t)val1_r;
            break;
        case VType::ui32:
            reinterpret_cast<uint32_t&>(actual_val0) |= (uint32_t)val1_r;
            break;
        case VType::ui64:
            reinterpret_cast<uint64_t&>(actual_val0) |= (uint64_t)val1_r;
            break;
        case VType::undefined_ptr:
            reinterpret_cast<size_t&>(actual_val0) |= (size_t)val1_r;
            break;
        case VType::struct_:
            art::CXX::Interface::makeCall(ClassAccess::pub, val0_r, symbols::structures::bitwise_or_operator, val1_r);
            break;
        default:
            throw InvalidCast("Fail cast value for or operation, cause value type is unsupported");
        }
    }

    void DynBitAnd(void** val0, void** val1) {
        ValueItem& val0_r = *reinterpret_cast<ValueItem*>(val0);
        ValueItem& val1_r = *reinterpret_cast<ValueItem*>(val1);
        val0_r.getAsync();
        val1_r.getAsync();
        void*& actual_val0 = val0_r.getSourcePtr();
        void*& actual_val1 = val1_r.getSourcePtr();

        if (!val0_r.meta.allow_edit)
            throw UnmodifiableValue();

        switch (val0_r.meta.vtype) {
        case VType::noting:
            val0_r = val1_r;
            break;
        case VType::i8:
            reinterpret_cast<int8_t&>(actual_val0) &= (int8_t)val1_r;
            break;
        case VType::i16:
            reinterpret_cast<int16_t&>(actual_val0) &= (int16_t)val1_r;
            break;
        case VType::i32:
            reinterpret_cast<int32_t&>(actual_val0) &= (int32_t)val1_r;
            break;
        case VType::i64:
            reinterpret_cast<int64_t&>(actual_val0) &= (int64_t)val1_r;
            break;
        case VType::ui8:
            reinterpret_cast<uint8_t&>(actual_val0) &= (uint8_t)val1_r;
            break;
        case VType::ui16:
            reinterpret_cast<uint16_t&>(actual_val0) &= (uint16_t)val1_r;
            break;
        case VType::ui32:
            reinterpret_cast<uint32_t&>(actual_val0) &= (uint32_t)val1_r;
            break;
        case VType::ui64:
            reinterpret_cast<uint64_t&>(actual_val0) &= (uint64_t)val1_r;
            break;
        case VType::undefined_ptr:
            reinterpret_cast<size_t&>(actual_val0) &= (size_t)val1_r;
            break;
        case VType::struct_:
            art::CXX::Interface::makeCall(ClassAccess::pub, val0_r, symbols::structures::bitwise_and_operator, val1_r);
            break;
        default:
            throw InvalidCast("Fail cast value for and operation, cause value type is unsupported");
        }
    }

    void DynBitShiftRight(void** val0, void** val1) {
        ValueItem& val0_r = *reinterpret_cast<ValueItem*>(val0);
        ValueItem& val1_r = *reinterpret_cast<ValueItem*>(val1);
        val0_r.getAsync();
        val1_r.getAsync();
        void*& actual_val0 = val0_r.getSourcePtr();
        void*& actual_val1 = val1_r.getSourcePtr();

        if (!val0_r.meta.allow_edit)
            throw UnmodifiableValue();

        switch (val0_r.meta.vtype) {
        case VType::noting:
            val0_r = val1_r;
            break;
        case VType::i8:
            reinterpret_cast<int8_t&>(actual_val0) >>= (int8_t)val1_r;
            break;
        case VType::i16:
            reinterpret_cast<int16_t&>(actual_val0) >>= (int16_t)val1_r;
            break;
        case VType::i32:
            reinterpret_cast<int32_t&>(actual_val0) >>= (int32_t)val1_r;
            break;
        case VType::i64:
            reinterpret_cast<int64_t&>(actual_val0) >>= (int64_t)val1_r;
            break;
        case VType::ui8:
            reinterpret_cast<uint8_t&>(actual_val0) >>= (uint8_t)val1_r;
            break;
        case VType::ui16:
            reinterpret_cast<uint16_t&>(actual_val0) >>= (uint16_t)val1_r;
            break;
        case VType::ui32:
            reinterpret_cast<uint32_t&>(actual_val0) >>= (uint32_t)val1_r;
            break;
        case VType::ui64:
            reinterpret_cast<uint64_t&>(actual_val0) >>= (uint64_t)val1_r;
            break;
        case VType::undefined_ptr:
            reinterpret_cast<size_t&>(actual_val0) >>= (size_t)val1_r;
            break;
        case VType::struct_:
            art::CXX::Interface::makeCall(ClassAccess::pub, val0_r, symbols::structures::bitwise_shift_right_operator, val1_r);
            break;
        default:
            throw InvalidCast("Fail cast value for shift right operation, cause value type is unsupported");
        }
    }

    void DynBitShiftLeft(void** val0, void** val1) {
        ValueItem& val0_r = *reinterpret_cast<ValueItem*>(val0);
        ValueItem& val1_r = *reinterpret_cast<ValueItem*>(val1);
        val0_r.getAsync();
        val1_r.getAsync();
        void*& actual_val0 = val0_r.getSourcePtr();
        void*& actual_val1 = val1_r.getSourcePtr();

        if (!val0_r.meta.allow_edit)
            throw UnmodifiableValue();

        switch (val0_r.meta.vtype) {
        case VType::noting:
            val0_r = val1_r;
            break;
        case VType::i8:
            reinterpret_cast<int8_t&>(actual_val0) <<= (int8_t)val1_r;
            break;
        case VType::i16:
            reinterpret_cast<int16_t&>(actual_val0) <<= (int16_t)val1_r;
            break;
        case VType::i32:
            reinterpret_cast<int32_t&>(actual_val0) <<= (int32_t)val1_r;
            break;
        case VType::i64:
            reinterpret_cast<int64_t&>(actual_val0) <<= (int64_t)val1_r;
            break;
        case VType::ui8:
            reinterpret_cast<uint8_t&>(actual_val0) <<= (uint8_t)val1_r;
            break;
        case VType::ui16:
            reinterpret_cast<uint16_t&>(actual_val0) <<= (uint16_t)val1_r;
            break;
        case VType::ui32:
            reinterpret_cast<uint32_t&>(actual_val0) <<= (uint32_t)val1_r;
            break;
        case VType::ui64:
            reinterpret_cast<uint64_t&>(actual_val0) <<= (uint64_t)val1_r;
            break;
        case VType::undefined_ptr:
            reinterpret_cast<size_t&>(actual_val0) <<= (size_t)val1_r;
            break;
        case VType::struct_:
            art::CXX::Interface::makeCall(ClassAccess::pub, val0_r, symbols::structures::bitwise_shift_left_operator, val1_r);
            break;
        default:
            throw InvalidCast("Fail cast value for shift left operation, cause value type is unsupported");
        }
    }

    void DynBitNot(void** val0) {
        ValueItem& val0_r = *reinterpret_cast<ValueItem*>(val0);
        val0_r.getAsync();
        void*& actual_val0 = val0_r.getSourcePtr();

        if (!val0_r.meta.allow_edit)
            throw UnmodifiableValue();

        switch (val0_r.meta.vtype) {
        case VType::noting:
            val0_r = ValueItem(0);
            break;
        case VType::i8:
            reinterpret_cast<int8_t&>(actual_val0) = ~reinterpret_cast<int8_t&>(actual_val0);
            break;
        case VType::i16:
            reinterpret_cast<int16_t&>(actual_val0) = ~reinterpret_cast<int16_t&>(actual_val0);
            break;
        case VType::i32:
            reinterpret_cast<int32_t&>(actual_val0) = ~reinterpret_cast<int32_t&>(actual_val0);
            break;
        case VType::i64:
            reinterpret_cast<int64_t&>(actual_val0) = ~reinterpret_cast<int64_t&>(actual_val0);
            break;
        case VType::ui8:
            reinterpret_cast<uint8_t&>(actual_val0) = ~reinterpret_cast<uint8_t&>(actual_val0);
            break;
        case VType::ui16:
            reinterpret_cast<uint16_t&>(actual_val0) = ~reinterpret_cast<uint16_t&>(actual_val0);
            break;
        case VType::ui32:
            reinterpret_cast<uint32_t&>(actual_val0) = ~reinterpret_cast<uint32_t&>(actual_val0);
            break;
        case VType::ui64:
            reinterpret_cast<uint64_t&>(actual_val0) = ~reinterpret_cast<uint64_t&>(actual_val0);
            break;
        case VType::undefined_ptr:
            reinterpret_cast<size_t&>(actual_val0) = ~reinterpret_cast<size_t&>(actual_val0);
            break;
        case VType::struct_:
            art::CXX::Interface::makeCall(ClassAccess::pub, val0_r, symbols::structures::bitwise_not_operator);
            break;
        default:
            throw InvalidCast("Fail cast value for not operation, cause value type is unsupported");
        }
    }

    void* AsArg(void** val) {
        ValueMeta& meta = *((ValueMeta*)val + 1);
        ValueItem* value = (ValueItem*)val;
        if (meta.vtype == VType::faarr || meta.vtype == VType::saarr)
            return *val;
        else if (meta.vtype == VType::uarr) {
            list_array<ValueItem>& vil = *reinterpret_cast<list_array<ValueItem>*>(*val);
            if (vil.size() > UINT32_MAX)
                throw InvalidCast("Fail cast uarr to faarr due too large array");
            *value = ValueItem(vil.to_array(), (uint32_t)vil.size());
            return value->getSourcePtr();
        } else {
            *value = ValueItem(std::initializer_list<ValueItem>{std::move(*(ValueItem*)(val))});
            return value->getSourcePtr();
        }
    }

    void AsArr(void** val) {
        ValueMeta& meta = *((ValueMeta*)val + 1);
        if (meta.vtype == VType::uarr)
            return;
        else {
            auto tmp = new list_array<ValueItem>(ABI_IMPL::Vcast<list_array<ValueItem>>(*val, meta));
            universalRemove(val);
            *val = tmp;
            meta.allow_edit = true;
            meta.use_gc = false;
            meta.vtype = VType::uarr;
        }
    }

    void asValue(void** val, VType type) {
        ValueMeta& meta = *reinterpret_cast<ValueMeta*>(val + 1);
        getAsyncResult(*val, meta);
        switch (type) {
        case VType::noting:
            universalRemove(val);
            break;
        case VType::i8:
            ABI_IMPL::setValue(ABI_IMPL::Vcast<int8_t>(*val, meta), val);
            break;
        case VType::i16:
            ABI_IMPL::setValue(ABI_IMPL::Vcast<int16_t>(*val, meta), val);
            break;
        case VType::i32:
            ABI_IMPL::setValue(ABI_IMPL::Vcast<int32_t>(*val, meta), val);
            break;
        case VType::i64:
            ABI_IMPL::setValue(ABI_IMPL::Vcast<int64_t>(*val, meta), val);
            break;
        case VType::ui8:
            ABI_IMPL::setValue(ABI_IMPL::Vcast<uint8_t>(*val, meta), val);
            break;
        case VType::ui16:
            ABI_IMPL::setValue(ABI_IMPL::Vcast<uint16_t>(*val, meta), val);
            break;
        case VType::ui32:
            ABI_IMPL::setValue(ABI_IMPL::Vcast<uint32_t>(*val, meta), val);
            break;
        case VType::ui64:
            ABI_IMPL::setValue(ABI_IMPL::Vcast<uint64_t>(*val, meta), val);
            break;
        case VType::flo:
            ABI_IMPL::setValue(ABI_IMPL::Vcast<float>(*val, meta), val);
            break;
        case VType::doub:
            ABI_IMPL::setValue(ABI_IMPL::Vcast<double>(*val, meta), val);
            break;
        case VType::uarr:
            ABI_IMPL::setValue(ABI_IMPL::Vcast<list_array<ValueItem>>(*val, meta), val);
            AsArg(val);
            break;
        case VType::string:
            ABI_IMPL::setValue(ABI_IMPL::Scast(*val, meta), val);
            break;
        case VType::undefined_ptr:
            ABI_IMPL::setValue(ABI_IMPL::Vcast<void*>(*val, meta), val);
            break;
        case VType::except_value:
            if (reinterpret_cast<ValueMeta*>(val + 1)->vtype != VType::except_value) {
                try {
                    throw AException("Undefined", ABI_IMPL::Scast(*val, meta), copyValue(*val, meta), meta.encoded);
                } catch (AException&) {
                    universalRemove(val);
                    *val = new std::exception_ptr(std::current_exception());
                    meta = VType::except_value;
                }
            }
            break;
        default:
            throw NotImplementedException();
        }
    }

    bool isValue(void** val, VType type) {
        ValueMeta& meta = *reinterpret_cast<ValueMeta*>(val + 1);
        getAsyncResult(*val, meta);
        return meta.vtype == type;
    }

    bool isTrueValue(void** value) {
        getAsyncResult(*value, *reinterpret_cast<ValueMeta*>(value + 1));
        void** val = &(getValue(value));
        switch (reinterpret_cast<ValueMeta*>(value + 1)->vtype) {
        case VType::noting:
            return false;
        case VType::boolean:
        case VType::i8:
        case VType::ui8:
            return *(uint8_t*)val;
        case VType::i16:
        case VType::ui16:
            return *(uint16_t*)val;
        case VType::i32:
        case VType::ui32:
            return *(uint32_t*)value;
        case VType::ui64:
        case VType::i64:
            return *(uint64_t*)value;
        case VType::flo:
            return *(float*)value;
        case VType::doub:
            return *(double*)value;
        case VType::uarr:
            return ((list_array<ValueItem>*)value)->size();
        case VType::string:
            return ((art::ustring*)value)->size();
        case VType::undefined_ptr:
            return *value;
        case VType::except_value:
            std::rethrow_exception(*(std::exception_ptr*)value);
        default:
            return false;
        }
    }

    void setBoolValue(bool boolean, void** value) {
        if (!boolean) {
            universalRemove(value);
            return;
        }
        ValueMeta& meta = *reinterpret_cast<ValueMeta*>(value + 1);
        switch (meta.vtype) {
        case VType::noting:
        case VType::undefined_ptr:
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
            getValue(value) = (void*)boolean;
            return;
        default:
            universalRemove(value);
            meta = VType::boolean;
            *value = (void*)boolean;
        }
    }

    size_t getSize(void** value) {
        void** res = &getValue(*value, *(ValueMeta*)(value + 1));
        ValueMeta& meta = *(ValueMeta*)(value + 1);
        int64_t sig;
        size_t actual;
        switch (meta.vtype) {
        case VType::i8:
            actual = sig = *(int8_t*)res;
            break;
        case VType::i16:
            actual = sig = *(int16_t*)res;
            break;
        case VType::i32:
            actual = sig = *(int32_t*)res;
            break;
        case VType::i64:
            actual = sig = *(int64_t*)res;
            break;
        case VType::ui8:
            return *(uint8_t*)res;
        case VType::ui16:
            return *(uint16_t*)res;
        case VType::ui32:
            return *(uint32_t*)res;
        case VType::ui64:
            return *(uint64_t*)res;
        case VType::flo: {
            float tmp = *(float*)res;
            actual = (size_t)tmp;
            if (tmp != actual)
                throw NumericUndererflowException();
            return actual;
        }
        case VType::doub: {
            double tmp = *(double*)res;
            actual = (size_t)tmp;
            if (tmp != actual)
                throw NumericUndererflowException();
            return actual;
        }
        default:
            throw InvalidType("Need sizable type");
        }
        if (sig != actual)
            throw NumericUndererflowException();
        return actual;
    }

    ValueItem::ValueItem(ValueItem&& move) {
        if (move.meta.vtype == VType::saarr && !move.meta.as_ref) {
            ValueItem* faarr = new ValueItem[move.meta.val_len];
            ValueItem* src = (ValueItem*)move.getSourcePtr();
            for (size_t i = 0; i < move.meta.val_len; i++)
                faarr[i] = std::move(src[i]);

            val = faarr;
            meta = move.meta;
            meta.vtype = VType::faarr;
            move.val = nullptr;
            return;
        }
        val = move.val;
        meta = move.meta;
        move.val = nullptr;
    }

#pragma region ValueItem constructors

    ValueItem::ValueItem(const void* val_, ValueMeta meta_)
        : val(0), meta(0) {
        auto tmp = (void*)val_;
        val = copyValue(tmp, meta_);
        meta = meta_;
    }

    ValueItem::ValueItem(void* val_, ValueMeta meta_, no_copy_t) {
        val = val_;
        meta = meta_;
    }

    ValueItem::ValueItem(void* val_, ValueMeta meta_, as_reference_t) {
        val = val_;
        meta = meta_;
        meta.as_ref = true;
    }

    ValueItem::ValueItem(const ValueItem& copy)
        : val(0), meta(0) {
        ValueItem& tmp = (ValueItem&)copy;
        val = copyValue(tmp.val, tmp.meta);
        meta = copy.meta;
    }

    ValueItem::ValueItem(nullptr_t)
        : val(0), meta(0) {}

    ValueItem::ValueItem(bool val)
        : val(*reinterpret_cast<void**>(&val)), meta(VType::boolean) {}

    ValueItem::ValueItem(int8_t val)
        : val(*reinterpret_cast<void**>(&val)), meta(VType::i8) {}

    ValueItem::ValueItem(uint8_t val)
        : val(*reinterpret_cast<void**>(&val)), meta(VType::ui8) {}

    ValueItem::ValueItem(int16_t val)
        : val(*reinterpret_cast<void**>(&val)), meta(VType::i16) {}

    ValueItem::ValueItem(uint16_t val)
        : val(*reinterpret_cast<void**>(&val)), meta(VType::ui16) {}

    ValueItem::ValueItem(int32_t val)
        : val(*reinterpret_cast<void**>(&val)), meta(VType::i32) {}

    ValueItem::ValueItem(uint32_t val)
        : val(*reinterpret_cast<void**>(&val)), meta(VType::ui32) {}

    ValueItem::ValueItem(int64_t val)
        : val(*reinterpret_cast<void**>(&val)), meta(VType::i64) {}

    ValueItem::ValueItem(uint64_t val)
        : val(*reinterpret_cast<void**>(&val)), meta(VType::ui64) {}

    ValueItem::ValueItem(float val)
        : val(*reinterpret_cast<void**>(&val)), meta(VType::flo) {}

    ValueItem::ValueItem(double val)
        : val(*reinterpret_cast<void**>(&val)), meta(VType::doub) {}

    ValueItem::ValueItem(const art::ustring& set)
        : val(new art::ustring(set)), meta(VType::string) {}

    ValueItem::ValueItem(art::ustring&& set)
        : val(new art::ustring(std::move(set))), meta(VType::string) {}

    ValueItem::ValueItem(const char* str)
        : val(new art::ustring(str)), meta(VType::string) {}

    ValueItem::ValueItem(const list_array<ValueItem>& val)
        : val(new list_array<ValueItem>(val)), meta(VType::uarr) {}

    ValueItem::ValueItem(list_array<ValueItem>&& val)
        : val(new list_array<ValueItem>(std::move(val))), meta(VType::uarr) {}

    ValueItem::ValueItem(ValueItem* vals, uint32_t len)
        : ValueItem(vals, ValueMeta(VType::faarr, false, true, len)) {}

    ValueItem::ValueItem(ValueItem* vals, uint32_t len, no_copy_t)
        : ValueItem(vals, ValueMeta(VType::faarr, false, true, len), no_copy) {}

    ValueItem::ValueItem(ValueItem* vals, uint32_t len, as_reference_t)
        : ValueItem(vals, ValueMeta(VType::faarr, false, true, len), as_reference) {}

    ValueItem::ValueItem(void* undefined_ptr)
        : val(undefined_ptr), meta(VType::undefined_ptr) {}

    ValueItem::ValueItem(const int8_t* vals, uint32_t len)
        : ValueItem(vals, ValueMeta(VType::raw_arr_i8, false, true, len)) {}

    ValueItem::ValueItem(const uint8_t* vals, uint32_t len)
        : ValueItem(vals, ValueMeta(VType::raw_arr_ui8, false, true, len)) {}

    ValueItem::ValueItem(const int16_t* vals, uint32_t len)
        : ValueItem(vals, ValueMeta(VType::raw_arr_i16, false, true, len)) {}

    ValueItem::ValueItem(const uint16_t* vals, uint32_t len)
        : ValueItem(vals, ValueMeta(VType::raw_arr_ui16, false, true, len)) {}

    ValueItem::ValueItem(const int32_t* vals, uint32_t len)
        : ValueItem(vals, ValueMeta(VType::raw_arr_i32, false, true, len)) {}

    ValueItem::ValueItem(const uint32_t* vals, uint32_t len)
        : ValueItem(vals, ValueMeta(VType::raw_arr_ui32, false, true, len)) {}

    ValueItem::ValueItem(const int64_t* vals, uint32_t len)
        : ValueItem(vals, ValueMeta(VType::raw_arr_i64, false, true, len)) {}

    ValueItem::ValueItem(const uint64_t* vals, uint32_t len)
        : ValueItem(vals, ValueMeta(VType::raw_arr_ui64, false, true, len)) {}

    ValueItem::ValueItem(const float* vals, uint32_t len)
        : ValueItem(vals, ValueMeta(VType::raw_arr_flo, false, true, len)) {}

    ValueItem::ValueItem(const double* vals, uint32_t len)
        : ValueItem(vals, ValueMeta(VType::raw_arr_doub, false, true, len)) {}

    ValueItem::ValueItem(Structure* str, no_copy_t)
        : ValueItem(str, VType::struct_, no_copy) {}

    ValueItem::ValueItem(int8_t* vals, uint32_t len, no_copy_t)
        : ValueItem(vals, ValueMeta(VType::raw_arr_i8, false, true, len), no_copy) {}

    ValueItem::ValueItem(uint8_t* vals, uint32_t len, no_copy_t)
        : ValueItem(vals, ValueMeta(VType::raw_arr_ui8, false, true, len), no_copy) {}

    ValueItem::ValueItem(int16_t* vals, uint32_t len, no_copy_t)
        : ValueItem(vals, ValueMeta(VType::raw_arr_i16, false, true, len), no_copy) {}

    ValueItem::ValueItem(uint16_t* vals, uint32_t len, no_copy_t)
        : ValueItem(vals, ValueMeta(VType::raw_arr_ui16, false, true, len), no_copy) {}

    ValueItem::ValueItem(int32_t* vals, uint32_t len, no_copy_t)
        : ValueItem(vals, ValueMeta(VType::raw_arr_i32, false, true, len), no_copy) {}

    ValueItem::ValueItem(uint32_t* vals, uint32_t len, no_copy_t)
        : ValueItem(vals, ValueMeta(VType::raw_arr_ui32, false, true, len), no_copy) {}

    ValueItem::ValueItem(int64_t* vals, uint32_t len, no_copy_t)
        : ValueItem(vals, ValueMeta(VType::raw_arr_i64, false, true, len), no_copy) {}

    ValueItem::ValueItem(uint64_t* vals, uint32_t len, no_copy_t)
        : ValueItem(vals, ValueMeta(VType::raw_arr_ui64, false, true, len), no_copy) {}

    ValueItem::ValueItem(float* vals, uint32_t len, no_copy_t)
        : ValueItem(vals, ValueMeta(VType::raw_arr_flo, false, true, len), no_copy) {}

    ValueItem::ValueItem(double* vals, uint32_t len, no_copy_t)
        : ValueItem(vals, ValueMeta(VType::raw_arr_doub, false, true, len), no_copy) {}

    ValueItem::ValueItem(int8_t* vals, uint32_t len, as_reference_t)
        : ValueItem(vals, ValueMeta(VType::raw_arr_i8, false, true, len), as_reference) {}

    ValueItem::ValueItem(uint8_t* vals, uint32_t len, as_reference_t)
        : ValueItem(vals, ValueMeta(VType::raw_arr_ui8, false, true, len), as_reference) {}

    ValueItem::ValueItem(int16_t* vals, uint32_t len, as_reference_t)
        : ValueItem(vals, ValueMeta(VType::raw_arr_i16, false, true, len), as_reference) {}

    ValueItem::ValueItem(uint16_t* vals, uint32_t len, as_reference_t)
        : ValueItem(vals, ValueMeta(VType::raw_arr_ui16, false, true, len), as_reference) {}

    ValueItem::ValueItem(int32_t* vals, uint32_t len, as_reference_t)
        : ValueItem(vals, ValueMeta(VType::raw_arr_i32, false, true, len), as_reference) {}

    ValueItem::ValueItem(uint32_t* vals, uint32_t len, as_reference_t)
        : ValueItem(vals, ValueMeta(VType::raw_arr_ui32, false, true, len), as_reference) {}

    ValueItem::ValueItem(int64_t* vals, uint32_t len, as_reference_t)
        : ValueItem(vals, ValueMeta(VType::raw_arr_i64, false, true, len), as_reference) {}

    ValueItem::ValueItem(uint64_t* vals, uint32_t len, as_reference_t)
        : ValueItem(vals, ValueMeta(VType::raw_arr_ui64, false, true, len), as_reference) {}

    ValueItem::ValueItem(float* vals, uint32_t len, as_reference_t)
        : ValueItem(vals, ValueMeta(VType::raw_arr_flo, false, true, len), as_reference) {}

    ValueItem::ValueItem(double* vals, uint32_t len, as_reference_t)
        : ValueItem(vals, ValueMeta(VType::raw_arr_doub, false, true, len), as_reference) {}

    ValueItem::ValueItem(const art::typed_lgr<Task>& task)
        : val(new art::typed_lgr<Task>(task)), meta(VType::async_res) {}

    ValueItem::ValueItem(const art::shared_ptr<Generator>& generator)
        : val(new art::shared_ptr<Generator>(generator)), meta(VType::generator) {}

    ValueItem::ValueItem(const std::initializer_list<ValueItem>& args)
        : val(0), meta(0) {
        if (args.size() > (size_t)UINT32_MAX)
            throw OutOfRange("Too large array");
        uint32_t len = (uint32_t)args.size();
        meta = ValueMeta(VType::faarr, false, true, len);
        if (args.size()) {
            ValueItem* res = new ValueItem[len];
            ValueItem* copy = const_cast<ValueItem*>(args.begin());
            for (uint32_t i = 0; i < len; i++)
                res[i] = std::move(copy[i]);
            val = res;
        }
    }

    ValueItem::ValueItem(const std::exception_ptr& ex) {
        val = new std::exception_ptr(ex);
        meta = VType::except_value;
    }

    ValueItem::ValueItem(const std::chrono::high_resolution_clock::time_point& time) {
        static_assert(sizeof(std::chrono::high_resolution_clock::time_point) <= sizeof(val), "Time point is too large");
        reinterpret_cast<std::chrono::high_resolution_clock::time_point&>(val) = time;
        meta = VType::time_point;
    }

    ValueItem::ValueItem(const std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>& map)
        : ValueItem(new std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>(map), VType::map) {}

    ValueItem::ValueItem(std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>&& map)
        : ValueItem(new std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>(std::move(map)), VType::map) {}

    ValueItem::ValueItem(const std::unordered_set<ValueItem, art::hash<ValueItem>>& set)
        : ValueItem(new std::unordered_set<ValueItem, art::hash<ValueItem>>(set), VType::set) {}

    ValueItem::ValueItem(std::unordered_set<ValueItem, art::hash<ValueItem>>&& set)
        : ValueItem(new std::unordered_set<ValueItem, art::hash<ValueItem>>(std::move(set)), VType::set) {}

    ValueItem::ValueItem(const art::shared_ptr<FuncEnvironment>& fun)
        : val(new art::shared_ptr<FuncEnvironment>(fun)), meta(VType::function) {}

    ValueItem::ValueItem(VType type) {
        meta = type;
        switch (type) {
        case VType::noting:
        case VType::async_res:
        case VType::undefined_ptr:
        case VType::type_identifier:
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
        case VType::time_point:
            val = nullptr;
            break;
        case VType::raw_arr_i8:
        case VType::raw_arr_ui8:
            val = new uint8_t[1]{0};
            meta.val_len = 1;
            break;
        case VType::raw_arr_i16:
        case VType::raw_arr_ui16:
            val = new uint16_t[1]{0};
            meta.val_len = 1;
            break;
        case VType::raw_arr_i32:
        case VType::raw_arr_ui32:
        case VType::raw_arr_flo:
            val = new uint32_t[1]{0};
            meta.val_len = 1;
            break;
        case VType::raw_arr_i64:
        case VType::raw_arr_ui64:
        case VType::raw_arr_doub:
            val = new uint64_t[1]{0};
            meta.val_len = 1;
            break;
        case VType::uarr:
            val = new list_array<ValueItem>();
            break;
        case VType::string:
            val = new art::ustring();
            break;
        case VType::except_value:
            try {
                throw AException("Undefined exception", "No description");
            } catch (...) {
                val = new std::exception_ptr(std::current_exception());
            }
            break;
        case VType::faarr:
            val = new ValueItem[1]{};
            meta.val_len = 1;
            break;
        case VType::function:
            val = new art::shared_ptr<FuncEnvironment>();
            break;
        default:
            throw NotImplementedException();
        }
    }

    ValueItem::ValueItem(ValueMeta type) {
        val = (void*)type.encoded;
        meta = VType::type_identifier;
    }

    ValueItem::ValueItem(Structure* str, as_reference_t) {
        val = str;
        meta = VType::struct_;
        meta.as_ref = true;
    }

    ValueItem::ValueItem(ValueItem& ref, as_reference_t) {
        if (
            is_integer(ref.meta.vtype) || ref.meta.vtype == VType::type_identifier || ref.meta.vtype == VType::undefined_ptr || ref.meta.vtype == VType::time_point || ref.meta.vtype == VType::boolean
        )
            val = &ref.val;
        else
            val = ref.val;
        meta = ref.meta;
        meta.as_ref = true;
    }

    ValueItem::ValueItem(bool& val, as_reference_t) {
        this->val = &val;
        meta = VType::boolean;
        meta.as_ref = true;
    }

    ValueItem::ValueItem(int8_t& val, as_reference_t) {
        this->val = &val;
        meta = VType::i8;
        meta.as_ref = true;
    }

    ValueItem::ValueItem(uint8_t& val, as_reference_t) {
        this->val = &val;
        meta = VType::ui8;
        meta.as_ref = true;
    }

    ValueItem::ValueItem(int16_t& val, as_reference_t) {
        this->val = &val;
        meta = VType::i16;
        meta.as_ref = true;
    }

    ValueItem::ValueItem(uint16_t& val, as_reference_t) {
        this->val = &val;
        meta = VType::ui16;
        meta.as_ref = true;
    }

    ValueItem::ValueItem(int32_t& val, as_reference_t) {
        this->val = &val;
        meta = VType::i32;
        meta.as_ref = true;
    }

    ValueItem::ValueItem(uint32_t& val, as_reference_t) {
        this->val = &val;
        meta = VType::ui32;
        meta.as_ref = true;
    }

    ValueItem::ValueItem(int64_t& val, as_reference_t) {
        this->val = &val;
        meta = VType::i64;
        meta.as_ref = true;
    }

    ValueItem::ValueItem(uint64_t& val, as_reference_t) {
        this->val = &val;
        meta = VType::ui64;
        meta.as_ref = true;
    }

    ValueItem::ValueItem(float& val, as_reference_t) {
        this->val = &val;
        meta = VType::flo;
        meta.as_ref = true;
    }

    ValueItem::ValueItem(double& val, as_reference_t) {
        this->val = &val;
        meta = VType::doub;
        meta.as_ref = true;
    }

    ValueItem::ValueItem(art::ustring& val, as_reference_t) {
        this->val = &val;
        meta = VType::string;
        meta.as_ref = true;
    }

    ValueItem::ValueItem(list_array<ValueItem>& val, as_reference_t) {
        this->val = &val;
        meta = VType::uarr;
        meta.as_ref = true;
    }

    ValueItem::ValueItem(std::exception_ptr& val, as_reference_t) {
        this->val = &val;
        meta = VType::except_value;
        meta.as_ref = true;
    }

    ValueItem::ValueItem(std::chrono::high_resolution_clock::time_point& val, as_reference_t) {
        this->val = &val;
        meta = VType::time_point;
        meta.as_ref = true;
    }

    ValueItem::ValueItem(std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>& val, as_reference_t) {
        this->val = &val;
        meta = VType::map;
        meta.as_ref = true;
    }

    ValueItem::ValueItem(std::unordered_set<ValueItem, art::hash<ValueItem>>& val, as_reference_t) {
        this->val = &val;
        meta = VType::set;
        meta.as_ref = true;
    }

    ValueItem::ValueItem(class art::typed_lgr<Task>& val, as_reference_t) {
        this->val = &val;
        meta = VType::async_res;
        meta.as_ref = true;
    }

    ValueItem::ValueItem(class art::shared_ptr<Generator>& val, as_reference_t) {
        this->val = &val;
        meta = VType::generator;
        meta.as_ref = true;
    }

    ValueItem::ValueItem(ValueMeta& val, as_reference_t) {
        this->val = &val;
        meta = VType::type_identifier;
        meta.as_ref = true;
    }

    ValueItem::ValueItem(art::shared_ptr<FuncEnvironment>& val, as_reference_t) {
        this->val = &val;
        meta = VType::function;
        meta.as_ref = true;
    }

    ValueItem::ValueItem(const ValueItem& ref, as_reference_t) {
        val = ref.val;
        meta = ref.meta;
        meta.as_ref = true;
        meta.allow_edit = false;
    }

    ValueItem::ValueItem(const bool& val, as_reference_t) {
        this->val = (void*)&val;
        meta = VType::boolean;
        meta.as_ref = true;
        meta.allow_edit = false;
    }

    ValueItem::ValueItem(const int8_t& val, as_reference_t) {
        this->val = (void*)&val;
        meta = VType::i8;
        meta.as_ref = true;
        meta.allow_edit = false;
    }

    ValueItem::ValueItem(const uint8_t& val, as_reference_t) {
        this->val = (void*)&val;
        meta = VType::ui8;
        meta.as_ref = true;
        meta.allow_edit = false;
    }

    ValueItem::ValueItem(const int16_t& val, as_reference_t) {
        this->val = (void*)&val;
        meta = VType::i16;
        meta.as_ref = true;
        meta.allow_edit = false;
    }

    ValueItem::ValueItem(const uint16_t& val, as_reference_t) {
        this->val = (void*)&val;
        meta = VType::ui16;
        meta.as_ref = true;
        meta.allow_edit = false;
    }

    ValueItem::ValueItem(const int32_t& val, as_reference_t) {
        this->val = (void*)&val;
        meta = VType::i32;
        meta.as_ref = true;
        meta.allow_edit = false;
    }

    ValueItem::ValueItem(const uint32_t& val, as_reference_t) {
        this->val = (void*)&val;
        meta = VType::ui32;
        meta.as_ref = true;
        meta.allow_edit = false;
    }

    ValueItem::ValueItem(const int64_t& val, as_reference_t) {
        this->val = (void*)&val;
        meta = VType::i64;
        meta.as_ref = true;
        meta.allow_edit = false;
    }

    ValueItem::ValueItem(const uint64_t& val, as_reference_t) {
        this->val = (void*)&val;
        meta = VType::ui64;
        meta.as_ref = true;
        meta.allow_edit = false;
    }

    ValueItem::ValueItem(const float& val, as_reference_t) {
        this->val = (void*)&val;
        meta = VType::flo;
        meta.as_ref = true;
        meta.allow_edit = false;
    }

    ValueItem::ValueItem(const double& val, as_reference_t) {
        this->val = (void*)&val;
        meta = VType::doub;
        meta.as_ref = true;
        meta.allow_edit = false;
    }

    ValueItem::ValueItem(const Structure* str, as_reference_t) {
        val = const_cast<Structure*>(str);
        meta = VType::struct_;
        meta.as_ref = true;
        meta.allow_edit = false;
    }

    ValueItem::ValueItem(const art::ustring& val, as_reference_t) {
        this->val = (void*)&val;
        meta = VType::string;
        meta.as_ref = true;
        meta.allow_edit = false;
    }

    ValueItem::ValueItem(const list_array<ValueItem>& val, as_reference_t) {
        this->val = (void*)&val;
        meta = VType::uarr;
        meta.as_ref = true;
        meta.allow_edit = false;
    }

    ValueItem::ValueItem(const std::exception_ptr& val, as_reference_t) {
        this->val = (void*)&val;
        meta = VType::except_value;
        meta.as_ref = true;
        meta.allow_edit = false;
    }

    ValueItem::ValueItem(const std::chrono::high_resolution_clock::time_point& val, as_reference_t) {
        this->val = (void*)&val;
        meta = VType::time_point;
        meta.as_ref = true;
        meta.allow_edit = false;
    }

    ValueItem::ValueItem(const std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>& val, as_reference_t) {
        this->val = (void*)&val;
        meta = VType::map;
        meta.as_ref = true;
        meta.allow_edit = false;
    }

    ValueItem::ValueItem(const std::unordered_set<ValueItem, art::hash<ValueItem>>& val, as_reference_t) {
        this->val = (void*)&val;
        meta = VType::set;
        meta.as_ref = true;
        meta.allow_edit = false;
    }

    ValueItem::ValueItem(const art::typed_lgr<Task>& val, as_reference_t) {
        this->val = (void*)&val;
        meta = VType::async_res;
        meta.as_ref = true;
        meta.allow_edit = false;
    }

    ValueItem::ValueItem(const art::shared_ptr<Generator>& val, as_reference_t) {
        this->val = (void*)&val;
        meta = VType::generator;
        meta.as_ref = true;
        meta.allow_edit = false;
    }

    ValueItem::ValueItem(const ValueMeta& val, as_reference_t) {
        this->val = (void*)&val;
        meta = VType::type_identifier;
        meta.as_ref = true;
        meta.allow_edit = false;
    }

    ValueItem::ValueItem(const art::shared_ptr<FuncEnvironment>& val, as_reference_t) {
        this->val = (void*)&val;
        meta = VType::function;
        meta.as_ref = true;
        meta.allow_edit = false;
    }

    ValueItem::ValueItem(array_t<bool>&& val)
        : ValueItem((uint8_t*)val.data, val.length, no_copy) {
        val.release();
    }

    ValueItem::ValueItem(array_t<int8_t>&& val)
        : ValueItem(val.data, val.length, no_copy) {
        val.release();
    }

    ValueItem::ValueItem(array_t<uint8_t>&& val)
        : ValueItem(val.data, val.length, no_copy) {
        val.release();
    }

    ValueItem::ValueItem(array_t<char>&& val)
        : ValueItem(val.data, val.length, no_copy) {
        val.release();
    }

    ValueItem::ValueItem(array_t<int16_t>&& val)
        : ValueItem(val.data, val.length, no_copy) {
        val.release();
    }

    ValueItem::ValueItem(array_t<uint16_t>&& val)
        : ValueItem(val.data, val.length, no_copy) {
        val.release();
    }

    ValueItem::ValueItem(array_t<int32_t>&& val)
        : ValueItem(val.data, val.length, no_copy) {
        val.release();
    }

    ValueItem::ValueItem(array_t<uint32_t>&& val)
        : ValueItem(val.data, val.length, no_copy) {
        val.release();
    }

    ValueItem::ValueItem(array_t<int64_t>&& val)
        : ValueItem(val.data, val.length, no_copy) {
        val.release();
    }

    ValueItem::ValueItem(array_t<uint64_t>&& val)
        : ValueItem(val.data, val.length, no_copy) {
        val.release();
    }

    ValueItem::ValueItem(array_t<float>&& val)
        : ValueItem(val.data, val.length, no_copy) {
        val.release();
    }

    ValueItem::ValueItem(array_t<double>&& val)
        : ValueItem(val.data, val.length, no_copy) {
        val.release();
    }

    ValueItem::ValueItem(array_t<ValueItem>&& val)
        : ValueItem(val.data, val.length, no_copy) {
        val.release();
    }

    ValueItem::ValueItem(const array_t<bool>& val)
        : ValueItem((uint8_t*)val.data, val.length) {}

    ValueItem::ValueItem(const array_t<int8_t>& val)
        : ValueItem(val.data, val.length) {}

    ValueItem::ValueItem(const array_t<uint8_t>& val)
        : ValueItem(val.data, val.length) {}

    ValueItem::ValueItem(const array_t<char>& val)
        : ValueItem(val.data, val.length, no_copy) {
    }

    ValueItem::ValueItem(const array_t<int16_t>& val)
        : ValueItem(val.data, val.length) {}

    ValueItem::ValueItem(const array_t<uint16_t>& val)
        : ValueItem(val.data, val.length) {}

    ValueItem::ValueItem(const array_t<int32_t>& val)
        : ValueItem(val.data, val.length) {}

    ValueItem::ValueItem(const array_t<uint32_t>& val)
        : ValueItem(val.data, val.length) {}

    ValueItem::ValueItem(const array_t<int64_t>& val)
        : ValueItem(val.data, val.length) {}

    ValueItem::ValueItem(const array_t<uint64_t>& val)
        : ValueItem(val.data, val.length) {}

    ValueItem::ValueItem(const array_t<float>& val)
        : ValueItem(val.data, val.length) {}

    ValueItem::ValueItem(const array_t<double>& val)
        : ValueItem(val.data, val.length) {}

    ValueItem::ValueItem(const array_t<ValueItem>& val)
        : ValueItem(val.data, val.length) {}

    ValueItem::ValueItem(const array_ref_t<bool>& val)
        : ValueItem((uint8_t*)val.data, val.length, as_reference) {}

    ValueItem::ValueItem(const array_ref_t<int8_t>& val)
        : ValueItem(val.data, val.length, as_reference) {}

    ValueItem::ValueItem(const array_ref_t<uint8_t>& val)
        : ValueItem(val.data, val.length, as_reference) {}

    ValueItem::ValueItem(const array_ref_t<char>& val)
        : ValueItem(val.data, val.length, as_reference) {
    }

    ValueItem::ValueItem(const array_ref_t<int16_t>& val)
        : ValueItem(val.data, val.length, as_reference) {}

    ValueItem::ValueItem(const array_ref_t<uint16_t>& val)
        : ValueItem(val.data, val.length, as_reference) {}

    ValueItem::ValueItem(const array_ref_t<int32_t>& val)
        : ValueItem(val.data, val.length, as_reference) {}

    ValueItem::ValueItem(const array_ref_t<uint32_t>& val)
        : ValueItem(val.data, val.length, as_reference) {}

    ValueItem::ValueItem(const array_ref_t<int64_t>& val)
        : ValueItem(val.data, val.length, as_reference) {}

    ValueItem::ValueItem(const array_ref_t<uint64_t>& val)
        : ValueItem(val.data, val.length, as_reference) {}

    ValueItem::ValueItem(const array_ref_t<float>& val)
        : ValueItem(val.data, val.length, as_reference) {}

    ValueItem::ValueItem(const array_ref_t<double>& val)
        : ValueItem(val.data, val.length, as_reference) {}

    ValueItem::ValueItem(const array_ref_t<ValueItem>& val)
        : ValueItem(val.data, val.length, as_reference) {}

#pragma endregion

    ValueItem::~ValueItem() {
        if (val)
            if (!meta.as_ref)
                if (needAlloc(meta))
                    universalFree(&val, meta);
    }

    ValueItem& ValueItem::operator=(const ValueItem& copy) {
        ValueItem& tmp = (ValueItem&)copy;
        if (val)
            if (!meta.as_ref)
                if (needAlloc(meta))
                    universalFree(&val, meta);
        val = copyValue(tmp.val, tmp.meta);
        meta = copy.meta;
        return *this;
    }

    ValueItem& ValueItem::operator=(ValueItem&& move) {
        if (val)
            if (!meta.as_ref)
                if (needAlloc(meta))
                    universalFree(&val, meta);
        val = move.val;
        meta = move.meta;
        move.val = nullptr;
        return *this;
    }

#pragma region ValueItem operators

    bool ValueItem::operator<(const ValueItem& cmp) const {
        int8_t res = compare(cmp);
        return res == -1;
    }

    bool ValueItem::operator>(const ValueItem& cmp) const {
        int8_t res = compare(cmp);
        return res == 1;
    }

    bool ValueItem::operator==(const ValueItem& cmp) const {
        int8_t res = compare(cmp);
        return res == 0;
    }

    bool ValueItem::operator!=(const ValueItem& cmp) const {
        int8_t res = compare(cmp);
        return res != 0;
    }

    bool ValueItem::operator>=(const ValueItem& cmp) const {
        int8_t res = compare(cmp);
        return res != -1;
    }

    bool ValueItem::operator<=(const ValueItem& cmp) const {
        int8_t res = compare(cmp);
        return res != 1;
    }

    inline int64_t get_sinteger(void* ref, bool in_memory) {
        return in_memory ? *(int64_t*)(&ref) : *(int64_t*)ref;
    }

    inline uint64_t get_uinteger(void* ref, bool in_memory) {
        return in_memory ? *(uint64_t*)(&ref) : *(uint64_t*)ref;
    }

    int8_t ValueItem::compare(const ValueItem& cmp) const {
        void* self = needAlloc(meta) ? val : meta.as_ref ? *(void**)val
                                                         : (void*)&val;
        bool self_in_mem = meta.as_ref || meta.use_gc;
        if (meta.use_gc)
            self = ((lgr*)self)->getPtr();
        void* other = needAlloc(cmp.meta) ? cmp.val : cmp.meta.as_ref ? *(void**)cmp.val
                                                                      : (void*)&cmp.val;
        bool other_in_mem = meta.as_ref || meta.use_gc;
        if (cmp.meta.use_gc)
            other = ((lgr*)other)->getPtr();

        if ((is_integer(meta.vtype) || meta.vtype == VType::undefined_ptr) && (is_integer(cmp.meta.vtype) || cmp.meta.vtype == VType::undefined_ptr)) {
            if ((integer_unsigned(meta.vtype) || meta.vtype == VType::undefined_ptr) && (integer_unsigned(cmp.meta.vtype) || meta.vtype == VType::undefined_ptr)) {
                if (get_uinteger(self, self_in_mem) < get_uinteger(other, other_in_mem))
                    return -1;
                else if (get_uinteger(self, self_in_mem) > get_uinteger(other, other_in_mem))
                    return 1;
                else
                    return 0;
            } else {
                switch (meta.vtype) {
                case VType::i8:
                    switch (cmp.meta.vtype) {
                    case VType::i8:
                        if (*(int8_t*)self < *(int8_t*)other)
                            return -1;
                        else if (*(int8_t*)self > *(int8_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i16:
                        if (*(int8_t*)self < *(int16_t*)other)
                            return -1;
                        else if (*(int8_t*)self > *(int16_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i32:
                        if (*(int8_t*)self < *(int32_t*)other)
                            return -1;
                        else if (*(int8_t*)self > *(int32_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i64:
                        if (*(int8_t*)self < *(int64_t*)other)
                            return -1;
                        else if (*(int8_t*)self > *(int64_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::ui8:
                        if (*(int8_t*)self < *(uint8_t*)other)
                            return -1;
                        else if (*(int8_t*)self > *(uint8_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::ui16:
                        if (*(int8_t*)self < *(uint16_t*)other)
                            return -1;
                        else if (*(int8_t*)self > *(uint16_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::ui32:
                        if (*(int8_t*)self < *(uint32_t*)other)
                            return -1;
                        else if (*(int8_t*)self > *(uint32_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::ui64:
                        if (*(int8_t*)self < *(uint64_t*)other)
                            return -1;
                        else if (*(int8_t*)self > *(uint64_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::flo:
                        if (*(int8_t*)self < *(float*)other)
                            return -1;
                        else if (*(int8_t*)self > *(float*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::doub:
                        if (*(int8_t*)self < *(double*)other)
                            return -1;
                        else if (*(int8_t*)self > *(double*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    default:
                        break;
                    }
                    break;
                case VType::i16:
                    switch (cmp.meta.vtype) {
                    case VType::i8:
                        if (*(int16_t*)self < *(int8_t*)other)
                            return -1;
                        else if (*(int16_t*)self > *(int8_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i16:
                        if (*(int16_t*)self < *(int16_t*)other)
                            return -1;
                        else if (*(int16_t*)self > *(int16_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i32:
                        if (*(int16_t*)self < *(int32_t*)other)
                            return -1;
                        else if (*(int16_t*)self > *(int32_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i64:
                        if (*(int16_t*)self < *(int64_t*)other)
                            return -1;
                        else if (*(int16_t*)self > *(int64_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::ui8:
                        if (*(int16_t*)self < *(uint8_t*)other)
                            return -1;
                        else if (*(int16_t*)self > *(uint8_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::ui16:
                        if (*(int16_t*)self < *(uint16_t*)other)
                            return -1;
                        else if (*(int16_t*)self > *(uint16_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::ui32:
                        if (*(int16_t*)self < *(uint32_t*)other)
                            return -1;
                        else if (*(int16_t*)self > *(uint32_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::ui64:
                        if (*(int16_t*)self < *(uint64_t*)other)
                            return -1;
                        else if (*(int16_t*)self > *(uint64_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::flo:
                        if (*(int16_t*)self < *(float*)other)
                            return -1;
                        else if (*(int16_t*)self > *(float*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::doub:
                        if (*(int16_t*)self < *(double*)other)
                            return -1;
                        else if (*(int16_t*)self > *(double*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    default:
                        break;
                    }
                    break;
                case VType::i32:
                    switch (cmp.meta.vtype) {
                    case VType::i8:
                        if (*(int32_t*)self < *(int8_t*)other)
                            return -1;
                        else if (*(int32_t*)self > *(int8_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i16:
                        if (*(int32_t*)self < *(int16_t*)other)
                            return -1;
                        else if (*(int32_t*)self > *(int16_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i32:
                        if (*(int32_t*)self < *(int32_t*)other)
                            return -1;
                        else if (*(int32_t*)self > *(int32_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i64:
                        if (*(int32_t*)self < *(int64_t*)other)
                            return -1;
                        else if (*(int32_t*)self > *(int64_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::ui8:
                        if (*(int32_t*)self < *(uint8_t*)other)
                            return -1;
                        else if (*(int32_t*)self > *(uint8_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::ui16:
                        if (*(int32_t*)self < *(uint16_t*)other)
                            return -1;
                        else if (*(int32_t*)self > *(uint16_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::ui32:
                        if (*(int32_t*)self < *(uint32_t*)other)
                            return -1;
                        else if (*(int32_t*)self > *(uint32_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::ui64:
                        if (*(int32_t*)self < *(uint64_t*)other)
                            return -1;
                        else if (*(int32_t*)self > *(uint64_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::flo:
                        if (*(int32_t*)self < *(float*)other)
                            return -1;
                        else if (*(int32_t*)self > *(float*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::doub:
                        if (*(int32_t*)self < *(double*)other)
                            return -1;
                        else if (*(int32_t*)self > *(double*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    default:
                        break;
                    }
                    break;
                case VType::i64:
                    switch (cmp.meta.vtype) {
                    case VType::i8:
                        if (*(int64_t*)self < *(int8_t*)other)
                            return -1;
                        else if (*(int64_t*)self > *(int8_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i16:
                        if (*(int64_t*)self < *(int16_t*)other)
                            return -1;
                        else if (*(int64_t*)self > *(int16_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i32:
                        if (*(int64_t*)self < *(int32_t*)other)
                            return -1;
                        else if (*(int64_t*)self > *(int32_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i64:
                        if (*(int64_t*)self < *(int64_t*)other)
                            return -1;
                        else if (*(int64_t*)self > *(int64_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::ui8:
                        if (*(int64_t*)self < *(uint8_t*)other)
                            return -1;
                        else if (*(int64_t*)self > *(uint8_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::ui16:
                        if (*(int64_t*)self < *(uint16_t*)other)
                            return -1;
                        else if (*(int64_t*)self > *(uint16_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::ui32:
                        if (*(int64_t*)self < *(uint32_t*)other)
                            return -1;
                        else if (*(int64_t*)self > *(uint32_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::ui64:
                        if (*(int64_t*)self < *(uint64_t*)other)
                            return -1;
                        else if (*(int64_t*)self > *(uint64_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::flo:
                        if (*(int64_t*)self < *(float*)other)
                            return -1;
                        else if (*(int64_t*)self > *(float*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::doub:
                        if (*(int64_t*)self < *(double*)other)
                            return -1;
                        else if (*(int64_t*)self > *(double*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    default:
                        break;
                    }
                    break;
                case VType::ui8:
                    switch (cmp.meta.vtype) {
                    case VType::i8:
                        if (*(uint8_t*)self < *(int8_t*)other)
                            return -1;
                        else if (*(uint8_t*)self > *(int8_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i16:
                        if (*(uint8_t*)self < *(int16_t*)other)
                            return -1;
                        else if (*(uint8_t*)self > *(int16_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i32:
                        if (*(uint8_t*)self < *(int32_t*)other)
                            return -1;
                        else if (*(uint8_t*)self > *(int32_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i64:
                        if (*(uint8_t*)self < *(int64_t*)other)
                            return -1;
                        else if (*(uint8_t*)self > *(int64_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::flo:
                        if (*(uint8_t*)self < *(float*)other)
                            return -1;
                        else if (*(uint8_t*)self > *(float*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::doub:
                        if (*(uint8_t*)self < *(double*)other)
                            return -1;
                        else if (*(uint8_t*)self > *(double*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    default:
                        break;
                    }
                    break;
                case VType::ui16:
                    switch (cmp.meta.vtype) {
                    case VType::i8:
                        if (*(uint16_t*)self < *(int8_t*)other)
                            return -1;
                        else if (*(uint16_t*)self > *(int8_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i16:
                        if (*(uint16_t*)self < *(int16_t*)other)
                            return -1;
                        else if (*(uint16_t*)self > *(int16_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i32:
                        if (*(uint16_t*)self < *(int32_t*)other)
                            return -1;
                        else if (*(uint16_t*)self > *(int32_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i64:
                        if (*(uint16_t*)self < *(int64_t*)other)
                            return -1;
                        else if (*(uint16_t*)self > *(int64_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::flo:
                        if (*(uint16_t*)self < *(float*)other)
                            return -1;
                        else if (*(uint16_t*)self > *(float*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::doub:
                        if (*(uint16_t*)self < *(double*)other)
                            return -1;
                        else if (*(uint16_t*)self > *(double*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    default:
                        break;
                    }
                    break;
                case VType::ui32:
                    switch (cmp.meta.vtype) {
                    case VType::i8:
                        if (*(uint32_t*)self < *(int8_t*)other)
                            return -1;
                        else if (*(uint32_t*)self > *(int8_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i16:
                        if (*(uint32_t*)self < *(int16_t*)other)
                            return -1;
                        else if (*(uint32_t*)self > *(int16_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i32:
                        if (*(uint32_t*)self < *(int32_t*)other)
                            return -1;
                        else if (*(uint32_t*)self > *(int32_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i64:
                        if (*(uint32_t*)self < *(int64_t*)other)
                            return -1;
                        else if (*(uint32_t*)self > *(int64_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::flo:
                        if (*(uint32_t*)self < *(float*)other)
                            return -1;
                        else if (*(uint32_t*)self > *(float*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::doub:
                        if (*(uint32_t*)self < *(double*)other)
                            return -1;
                        else if (*(uint32_t*)self > *(double*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    default:
                        break;
                    }
                    break;
                case VType::ui64:
                    switch (cmp.meta.vtype) {
                    case VType::i8:
                        if (*(uint64_t*)self < *(int8_t*)other)
                            return -1;
                        else if (*(uint64_t*)self > *(int8_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i16:
                        if (*(uint64_t*)self < *(int16_t*)other)
                            return -1;
                        else if (*(uint64_t*)self > *(int16_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i32:
                        if (*(uint64_t*)self < *(int32_t*)other)
                            return -1;
                        else if (*(uint64_t*)self > *(int32_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i64:
                        if (*(uint64_t*)self < *(int64_t*)other)
                            return -1;
                        else if (*(uint64_t*)self > *(int64_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::flo:
                        if (*(uint64_t*)self < *(float*)other)
                            return -1;
                        else if (*(uint64_t*)self > *(float*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::doub:
                        if (*(uint64_t*)self < *(double*)other)
                            return -1;
                        else if (*(uint64_t*)self > *(double*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    default:
                        break;
                    }
                    break;
                case VType::flo:
                    switch (cmp.meta.vtype) {
                    case VType::i8:
                        if (*(float*)self < *(int8_t*)other)
                            return -1;
                        else if (*(float*)self > *(int8_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i16:
                        if (*(float*)self < *(int16_t*)other)
                            return -1;
                        else if (*(float*)self > *(int16_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i32:
                        if (*(float*)self < *(int32_t*)other)
                            return -1;
                        else if (*(float*)self > *(int32_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i64:
                        if (*(float*)self < *(int64_t*)other)
                            return -1;
                        else if (*(float*)self > *(int64_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::ui8:
                        if (*(float*)self < *(uint8_t*)other)
                            return -1;
                        else if (*(float*)self > *(uint8_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::ui16:
                        if (*(float*)self < *(uint16_t*)other)
                            return -1;
                        else if (*(float*)self > *(uint16_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::ui32:
                        if (*(float*)self < *(uint32_t*)other)
                            return -1;
                        else if (*(float*)self > *(uint32_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::ui64:
                        if (*(float*)self < *(uint64_t*)other)
                            return -1;
                        else if (*(float*)self > *(uint64_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::flo:
                        if (*(float*)self < *(float*)other)
                            return -1;
                        else if (*(float*)self > *(float*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::doub:
                        if (*(float*)self < *(double*)other)
                            return -1;
                        else if (*(float*)self > *(double*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    default:
                        break;
                    }
                    break;
                case VType::doub:
                    switch (cmp.meta.vtype) {
                    case VType::i8:
                        if (*(double*)self < *(int8_t*)other)
                            return -1;
                        else if (*(double*)self > *(int8_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i16:
                        if (*(double*)self < *(int16_t*)other)
                            return -1;
                        else if (*(double*)self > *(int16_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i32:
                        if (*(double*)self < *(int32_t*)other)
                            return -1;
                        else if (*(double*)self > *(int32_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::i64:
                        if (*(double*)self < *(int64_t*)other)
                            return -1;
                        else if (*(double*)self > *(int64_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::ui8:
                        if (*(double*)self < *(uint8_t*)other)
                            return -1;
                        else if (*(double*)self > *(uint8_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::ui16:
                        if (*(double*)self < *(uint16_t*)other)
                            return -1;
                        else if (*(double*)self > *(uint16_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::ui32:
                        if (*(double*)self < *(uint32_t*)other)
                            return -1;
                        else if (*(double*)self > *(uint32_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::ui64:
                        if (*(double*)self < *(uint64_t*)other)
                            return -1;
                        else if (*(double*)self > *(uint64_t*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::flo:
                        if (*(double*)self < *(float*)other)
                            return -1;
                        else if (*(double*)self > *(float*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    case VType::doub:
                        if (*(double*)self < *(double*)other)
                            return -1;
                        else if (*(double*)self > *(double*)other)
                            return 1;
                        else
                            return 0;
                        break;
                    default:
                        break;
                    }
                    break;
                default:
                    break;
                }
                throw NotImplementedException();
            }
        } else if (is_integer(meta.vtype) || meta.vtype == VType::undefined_ptr) {
            if (integer_unsigned(meta.vtype)) {
                uint64_t cmp_i = (uint64_t)cmp;
                if (get_uinteger(self, self_in_mem) < cmp_i)
                    return -1;
                else if (get_uinteger(self, self_in_mem) > cmp_i)
                    return 1;
                else
                    return 0;
            } else {
                return compare((int64_t)cmp);
            }
        } else if (meta.vtype == VType::string && cmp.meta.vtype == VType::string) {
            auto cmp = *(art::ustring*)self <=> *(art::ustring*)other;
            return cmp < 0 ? -1 : cmp > 0 ? 1
                                          : 0;
        } else if (meta.vtype == VType::string)
            return compare((art::ustring)cmp);
        else if (cmp.meta.vtype == VType::string)
            return cmp.compare((art::ustring) * this);
        else if (has_interface(meta.vtype) && has_interface(cmp.meta.vtype))
            return Structure::compare((Structure*)self, (Structure*)other);
        else if (meta.vtype == VType::uarr || is_raw_array(meta.vtype) || has_interface(meta.vtype)) {
            if (cmp.meta.vtype == VType::uarr || is_raw_array(cmp.meta.vtype) || has_interface(cmp.meta.vtype)) {
                size_t cur_siz = size();
                size_t extern_siz = cmp.size();
                if (ptrdiff_t(cur_siz) != cur_siz || ptrdiff_t(extern_siz) != extern_siz)
                    throw NumericOverflowException();
                ptrdiff_t dif_len = ptrdiff_t(cur_siz) - ptrdiff_t(extern_siz);
                if (dif_len == 0) {
                    auto begin = cbegin();
                    auto end = cend();
                    auto cmp_begin = cmp.cbegin();
                    while (begin != end) {
                        auto cmp = begin.get().compare(cmp_begin.get());
                        if (cmp != 0)
                            return cmp;
                        begin++;
                    }
                    return 0;
                } else {
                    return dif_len < 0 ? -1 : 1;
                }
            } else
                return -1;
        } else
            return meta.vtype == cmp.meta.vtype ? 0 : -1;
    }

    ValueItem& ValueItem::operator+=(const ValueItem& op) {
        DynSum(&val, (void**)&op.val);
        return *this;
    }

    ValueItem& ValueItem::operator-=(const ValueItem& op) {
        DynMinus(&val, (void**)&op.val);
        return *this;
    }

    ValueItem& ValueItem::operator*=(const ValueItem& op) {
        DynMul(&val, (void**)&op.val);
        return *this;
    }

    ValueItem& ValueItem::operator/=(const ValueItem& op) {
        DynDiv(&val, (void**)&op.val);
        return *this;
    }

    ValueItem& ValueItem::operator%=(const ValueItem& op) {
        DynRest(&val, (void**)&op.val);
        return *this;
    }

    ValueItem& ValueItem::operator^=(const ValueItem& op) {
        DynBitXor(&val, (void**)&op.val);
        return *this;
    }

    ValueItem& ValueItem::operator&=(const ValueItem& op) {
        DynBitAnd(&val, (void**)&op.val);
        return *this;
    }

    ValueItem& ValueItem::operator|=(const ValueItem& op) {
        DynBitOr(&val, (void**)&op.val);
        return *this;
    }

    ValueItem& ValueItem::operator<<=(const ValueItem& op) {
        DynBitShiftLeft(&val, (void**)&op.val);
        return *this;
    }

    ValueItem& ValueItem::operator>>=(const ValueItem& op) {
        DynBitShiftRight(&val, (void**)&op.val);
        return *this;
    }

    ValueItem& ValueItem::operator++() {
        DynInc(&val);
        return *this;
    }

    ValueItem& ValueItem::operator--() {
        DynDec(&val);
        return *this;
    }

    ValueItem& ValueItem::operator!() {
        DynBitNot(&val);
        return *this;
    }

    ValueItem ValueItem::operator+(const ValueItem& op) const {
        return ValueItem(*this) += op;
    }

    ValueItem ValueItem::operator-(const ValueItem& op) const {
        return ValueItem(*this) -= op;
    }

    ValueItem ValueItem::operator*(const ValueItem& op) const {
        return ValueItem(*this) *= op;
    }

    ValueItem ValueItem::operator/(const ValueItem& op) const {
        return ValueItem(*this) /= op;
    }

    ValueItem ValueItem::operator^(const ValueItem& op) const {
        return ValueItem(*this) ^= op;
    }

    ValueItem ValueItem::operator&(const ValueItem& op) const {
        return ValueItem(*this) &= op;
    }

    ValueItem ValueItem::operator|(const ValueItem& op) const {
        return ValueItem(*this) |= op;
    }

#pragma endregion

#pragma region ValueItem cast operators

    ValueItem::operator Structure&() {
        if (meta.vtype == VType::struct_)
            return *(Structure*)getSourcePtr();
        else
            throw InvalidCast("This type is not structure");
    }

    ValueItem::operator std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>&() {
        if (meta.vtype == VType::map)
            return *(std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>*)getSourcePtr();
        else
            throw InvalidCast("This type is not map");
    }

    ValueItem::operator std::unordered_set<ValueItem, art::hash<ValueItem>>&() {
        if (meta.vtype == VType::set)
            return *(std::unordered_set<ValueItem, art::hash<ValueItem>>*)getSourcePtr();
        else
            throw InvalidCast("This type is not set");
    }

    ValueItem::operator art::typed_lgr<Task>&() {
        if (meta.vtype == VType::async_res)
            return *(art::typed_lgr<Task>*)getSourcePtr();
        throw InvalidCast("This type is not async_res");
    }

    ValueItem::operator art::shared_ptr<Generator>&() {
        if (meta.vtype == VType::generator)
            return *(art::shared_ptr<Generator>*)getSourcePtr();
        throw InvalidCast("This type is not generator");
    }

    ValueItem::operator art::shared_ptr<FuncEnvironment>&() {
        return *funPtr();
    }

    ValueItem::operator bool() const {
        return ABI_IMPL::Vcast<bool>(val, meta);
    }

    ValueItem::operator int8_t() const {
        return ABI_IMPL::Vcast<int8_t>(val, meta);
    }

    ValueItem::operator uint8_t() const {
        return ABI_IMPL::Vcast<uint8_t>(val, meta);
    }

    ValueItem::operator int16_t() const {
        return ABI_IMPL::Vcast<int16_t>(val, meta);
    }

    ValueItem::operator uint16_t() const {
        return ABI_IMPL::Vcast<uint16_t>(val, meta);
    }

    ValueItem::operator int32_t() const {
        return ABI_IMPL::Vcast<int32_t>(val, meta);
    }

    ValueItem::operator uint32_t() const {
        return ABI_IMPL::Vcast<uint32_t>(val, meta);
    }

    ValueItem::operator int64_t() const {
        return ABI_IMPL::Vcast<int64_t>(val, meta);
    }

    ValueItem::operator uint64_t() const {
        return ABI_IMPL::Vcast<uint64_t>(val, meta);
    }

    ValueItem::operator float() const {
        return ABI_IMPL::Vcast<float>(val, meta);
    }

    ValueItem::operator double() const {
        return ABI_IMPL::Vcast<double>(val, meta);
    }

    ValueItem::operator void*() const {
        return ABI_IMPL::Vcast<void*>(val, meta);
    }

    ValueItem::operator art::ustring() const {
        if (meta.vtype == VType::string)
            return *(art::ustring*)getSourcePtr();
        else
            return ABI_IMPL::Scast(val, meta);
    }

    ValueItem::operator list_array<ValueItem>() const {
        return ABI_IMPL::Vcast<list_array<ValueItem>>(val, meta);
    }

    ValueItem::operator ValueMeta() const {
        if (meta.vtype == VType::type_identifier)
            return *(ValueMeta*)getSourcePtr();
        else
            return meta.vtype;
    }

    ValueItem::operator std::exception_ptr() const {
        if (meta.vtype == VType::except_value)
            return *(std::exception_ptr*)getSourcePtr();
        else {
            try {
                throw InvalidCast("This type is not proxy");
            } catch (...) {
                return std::current_exception();
            }
        }
    }

    ValueItem::operator std::chrono::high_resolution_clock::time_point() const {
        if (meta.vtype == VType::time_point)
            return *(std::chrono::high_resolution_clock::time_point*)getSourcePtr();
        else
            throw InvalidCast("This type is not time_point");
    }

    ValueItem::operator const Structure&() const {
        if (meta.vtype == VType::struct_)
            return *(const Structure*)getSourcePtr();
        else
            throw InvalidCast("This type is not structure");
    }

    ValueItem::operator const std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>&() const {
        if (meta.vtype == VType::map)
            return *(const std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>*)getSourcePtr();
        else
            throw InvalidCast("This type is not map");
    }

    ValueItem::operator const std::unordered_set<ValueItem, art::hash<ValueItem>>&() const {
        if (meta.vtype == VType::set)
            return *(const std::unordered_set<ValueItem, art::hash<ValueItem>>*)getSourcePtr();
        else
            throw InvalidCast("This type is not set");
    }

    ValueItem::operator const class art::typed_lgr<Task> &() const {
        if (meta.vtype == VType::async_res)
            return *(const art::typed_lgr<Task>*)getSourcePtr();
        throw InvalidCast("This type is not async_res");

    } ValueItem::operator const class art::shared_ptr<Generator> &() const {

        if (meta.vtype == VType::generator)
            return *(const art::shared_ptr<Generator>*)getSourcePtr();
        throw InvalidCast("This type is not generator");

    } ValueItem::operator const art::shared_ptr<FuncEnvironment>&() const {

        return *funPtr();
    }

    ValueItem::operator const array_t<bool>() const {
        const void* src = getSourcePtr();
        if (!meta.val_len)
            return array_t<bool>();
        bool* copy_arr = new bool[meta.val_len];
        switch (meta.vtype) {
        case VType::raw_arr_i8:
        case VType::raw_arr_ui8:
            memcpy(copy_arr, src, meta.val_len);
            break;
        case VType::raw_arr_i16:
        case VType::raw_arr_ui16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (bool)((const int16_t*)src)[i];
            break;
        case VType::raw_arr_i32:
        case VType::raw_arr_ui32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (bool)((const int32_t*)src)[i];
            break;
        case VType::raw_arr_i64:
        case VType::raw_arr_ui64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (bool)((const int64_t*)src)[i];
            break;
        case VType::raw_arr_flo:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (bool)((const float*)src)[i];
            break;
        case VType::raw_arr_doub:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (bool)((const double*)src)[i];
            break;
        case VType::faarr:
        case VType::saarr:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (bool)((const ValueItem*)src)[i];
            break;
        default:
            delete[] copy_arr;
            throw InvalidCast("This type is not array");
        }
        return array_t<bool>(meta.val_len, copy_arr);
    }

    ValueItem::operator const array_t<int8_t>() const {
        const void* src = getSourcePtr();
        if (!meta.val_len)
            return array_t<int8_t>();
        int8_t* copy_arr = new int8_t[meta.val_len];
        switch (meta.vtype) {
        case VType::raw_arr_i8:
            memcpy(copy_arr, src, meta.val_len);
            break;
        case VType::raw_arr_ui8:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int8_t)((const uint8_t*)src)[i];
            break;
        case VType::raw_arr_i16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int8_t)((const int16_t*)src)[i];
            break;
        case VType::raw_arr_ui16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int8_t)((const uint16_t*)src)[i];
            break;
        case VType::raw_arr_i32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int8_t)((const int32_t*)src)[i];
            break;
        case VType::raw_arr_ui32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int8_t)((const uint32_t*)src)[i];
            break;
        case VType::raw_arr_i64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int8_t)((const int64_t*)src)[i];
            break;
        case VType::raw_arr_ui64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int8_t)((const uint64_t*)src)[i];
            break;
        case VType::raw_arr_flo:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int8_t)((const float*)src)[i];
            break;
        case VType::raw_arr_doub:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int8_t)((const double*)src)[i];
            break;
        case VType::faarr:
        case VType::saarr:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int8_t)((const ValueItem*)src)[i];
            break;
        default:
            delete[] copy_arr;
            throw InvalidCast("This type is not array");
        }
        return array_t<int8_t>(meta.val_len, copy_arr);
    }

    ValueItem::operator const array_t<uint8_t>() const {
        const void* src = getSourcePtr();
        if (!meta.val_len)
            return array_t<uint8_t>();
        uint8_t* copy_arr = new uint8_t[meta.val_len];
        switch (meta.vtype) {
        case VType::raw_arr_i8:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint8_t)((const int8_t*)src)[i];
            break;
        case VType::raw_arr_ui8:
            memcpy(copy_arr, src, meta.val_len);
            break;
        case VType::raw_arr_i16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint8_t)((const int16_t*)src)[i];
            break;
        case VType::raw_arr_ui16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint8_t)((const uint16_t*)src)[i];
            break;
        case VType::raw_arr_i32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint8_t)((const int32_t*)src)[i];
            break;
        case VType::raw_arr_ui32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint8_t)((const uint32_t*)src)[i];
            break;
        case VType::raw_arr_i64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint8_t)((const int64_t*)src)[i];
            break;
        case VType::raw_arr_ui64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint8_t)((const uint64_t*)src)[i];
            break;
        case VType::raw_arr_flo:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint8_t)((const float*)src)[i];
            break;
        case VType::raw_arr_doub:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint8_t)((const double*)src)[i];
            break;
        case VType::faarr:
        case VType::saarr:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint8_t)((const ValueItem*)src)[i];
            break;
        default:
            delete[] copy_arr;
            throw InvalidCast("This type is not array");
        }
        return array_t<uint8_t>(meta.val_len, copy_arr);
    }

    ValueItem::operator const array_t<char>() const {
        const void* src = getSourcePtr();
        if (!meta.val_len)
            return array_t<char>();
        char* copy_arr = new char[meta.val_len];
        switch (meta.vtype) {
        case VType::raw_arr_i8:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (char)((const int8_t*)src)[i];
            break;
        case VType::raw_arr_ui8:
            memcpy(copy_arr, src, meta.val_len);
            break;
        case VType::raw_arr_i16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (char)((const int16_t*)src)[i];
            break;
        case VType::raw_arr_ui16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (char)((const uint16_t*)src)[i];
            break;
        case VType::raw_arr_i32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (char)((const int32_t*)src)[i];
            break;
        case VType::raw_arr_ui32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (char)((const uint32_t*)src)[i];
            break;
        case VType::raw_arr_i64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (char)((const int64_t*)src)[i];
            break;
        case VType::raw_arr_ui64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (char)((const uint64_t*)src)[i];
            break;
        case VType::raw_arr_flo:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (char)((const float*)src)[i];
            break;
        case VType::raw_arr_doub:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (char)((const double*)src)[i];
            break;
        case VType::faarr:
        case VType::saarr:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (char)(uint8_t)((const ValueItem*)src)[i];
            break;
        default:
            delete[] copy_arr;
            throw InvalidCast("This type is not array");
        }
        return array_t<char>(meta.val_len, copy_arr);
    }

    ValueItem::operator const array_t<int16_t>() const {
        const void* src = getSourcePtr();
        if (!meta.val_len)
            return array_t<int16_t>();
        int16_t* copy_arr = new int16_t[meta.val_len];
        switch (meta.vtype) {
        case VType::raw_arr_i8:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int16_t)((const int8_t*)src)[i];
            break;
        case VType::raw_arr_ui8:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int16_t)((const uint8_t*)src)[i];
            break;
        case VType::raw_arr_i16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int16_t)((const int16_t*)src)[i];
            break;
        case VType::raw_arr_ui16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int16_t)((const uint16_t*)src)[i];
            break;
        case VType::raw_arr_i32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int16_t)((const int32_t*)src)[i];
            break;
        case VType::raw_arr_ui32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int16_t)((const uint32_t*)src)[i];
            break;
        case VType::raw_arr_i64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int16_t)((const int64_t*)src)[i];
            break;
        case VType::raw_arr_ui64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int16_t)((const uint64_t*)src)[i];
            break;
        case VType::raw_arr_flo:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int16_t)((const float*)src)[i];
            break;
        case VType::raw_arr_doub:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int16_t)((const double*)src)[i];
            break;
        case VType::faarr:
        case VType::saarr:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int16_t)((const ValueItem*)src)[i];
            break;
        default:
            delete[] copy_arr;
            throw InvalidCast("This type is not array");
        }
        return array_t<int16_t>(meta.val_len, copy_arr);
    }

    ValueItem::operator const array_t<uint16_t>() const {
        const void* src = getSourcePtr();
        if (!meta.val_len)
            return array_t<uint16_t>();
        uint16_t* copy_arr = new uint16_t[meta.val_len];
        switch (meta.vtype) {
        case VType::raw_arr_i8:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint16_t)((const int8_t*)src)[i];
            break;
        case VType::raw_arr_ui8:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint16_t)((const uint8_t*)src)[i];
            break;
        case VType::raw_arr_i16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint16_t)((const int16_t*)src)[i];
            break;
        case VType::raw_arr_ui16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint16_t)((const uint16_t*)src)[i];
            break;
        case VType::raw_arr_i32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint16_t)((const int32_t*)src)[i];
            break;
        case VType::raw_arr_ui32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint16_t)((const uint32_t*)src)[i];
            break;
        case VType::raw_arr_i64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint16_t)((const int64_t*)src)[i];
            break;
        case VType::raw_arr_ui64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint16_t)((const uint64_t*)src)[i];
            break;
        case VType::raw_arr_flo:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint16_t)((const float*)src)[i];
            break;
        case VType::raw_arr_doub:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint16_t)((const double*)src)[i];
            break;
        case VType::faarr:
        case VType::saarr:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint16_t)((const ValueItem*)src)[i];
            break;
        default:
            delete[] copy_arr;
            throw InvalidCast("This type is not array");
        }
        return array_t<uint16_t>(meta.val_len, copy_arr);
    }

    ValueItem::operator const array_t<int32_t>() const {
        const void* src = getSourcePtr();
        if (!meta.val_len)
            return array_t<int32_t>();
        int32_t* copy_arr = new int32_t[meta.val_len];
        switch (meta.vtype) {
        case VType::raw_arr_i8:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int32_t)((const int8_t*)src)[i];
            break;
        case VType::raw_arr_ui8:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int32_t)((const uint8_t*)src)[i];
            break;
        case VType::raw_arr_i16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int32_t)((const int16_t*)src)[i];
            break;
        case VType::raw_arr_ui16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int32_t)((const uint16_t*)src)[i];
            break;
        case VType::raw_arr_i32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int32_t)((const int32_t*)src)[i];
            break;
        case VType::raw_arr_ui32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int32_t)((const uint32_t*)src)[i];
            break;
        case VType::raw_arr_i64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int32_t)((const int64_t*)src)[i];
            break;
        case VType::raw_arr_ui64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int32_t)((const uint64_t*)src)[i];
            break;
        case VType::raw_arr_flo:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int32_t)((const float*)src)[i];
            break;
        case VType::raw_arr_doub:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int32_t)((const double*)src)[i];
            break;
        case VType::faarr:
        case VType::saarr:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int32_t)((const ValueItem*)src)[i];
            break;
        default:
            delete[] copy_arr;
            throw InvalidCast("This type is not array");
        }
        return array_t<int32_t>(meta.val_len, copy_arr);
    }

    ValueItem::operator const array_t<uint32_t>() const {
        const void* src = getSourcePtr();
        if (!meta.val_len)
            return array_t<uint32_t>();
        uint32_t* copy_arr = new uint32_t[meta.val_len];
        switch (meta.vtype) {
        case VType::raw_arr_i8:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint32_t)((const int8_t*)src)[i];
            break;
        case VType::raw_arr_ui8:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint32_t)((const uint8_t*)src)[i];
            break;
        case VType::raw_arr_i16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint32_t)((const int16_t*)src)[i];
            break;
        case VType::raw_arr_ui16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint32_t)((const uint16_t*)src)[i];
            break;
        case VType::raw_arr_i32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint32_t)((const int32_t*)src)[i];
            break;
        case VType::raw_arr_ui32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint32_t)((const uint32_t*)src)[i];
            break;
        case VType::raw_arr_i64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint32_t)((const int64_t*)src)[i];
            break;
        case VType::raw_arr_ui64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint32_t)((const uint64_t*)src)[i];
            break;
        case VType::raw_arr_flo:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint32_t)((const float*)src)[i];
            break;
        case VType::raw_arr_doub:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint32_t)((const double*)src)[i];
            break;
        case VType::faarr:
        case VType::saarr:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint32_t)((const ValueItem*)src)[i];
            break;
        default:
            delete[] copy_arr;
            throw InvalidCast("This type is not array");
        }
        return array_t<uint32_t>(meta.val_len, copy_arr);
    }

    ValueItem::operator const array_t<int64_t>() const {
        const void* src = getSourcePtr();
        if (!meta.val_len)
            return array_t<int64_t>();
        int64_t* copy_arr = new int64_t[meta.val_len];
        switch (meta.vtype) {
        case VType::raw_arr_i8:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int64_t)((const int8_t*)src)[i];
            break;
        case VType::raw_arr_ui8:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int64_t)((const uint8_t*)src)[i];
            break;
        case VType::raw_arr_i16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int64_t)((const int16_t*)src)[i];
            break;
        case VType::raw_arr_ui16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int64_t)((const uint16_t*)src)[i];
            break;
        case VType::raw_arr_i32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int64_t)((const int32_t*)src)[i];
            break;
        case VType::raw_arr_ui32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int64_t)((const uint32_t*)src)[i];
            break;
        case VType::raw_arr_i64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int64_t)((const int64_t*)src)[i];
            break;
        case VType::raw_arr_ui64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int64_t)((const uint64_t*)src)[i];
            break;
        case VType::raw_arr_flo:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int64_t)((const float*)src)[i];
            break;
        case VType::raw_arr_doub:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int64_t)((const double*)src)[i];
            break;
        case VType::faarr:
        case VType::saarr:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (int64_t)((const ValueItem*)src)[i];
            break;
        default:
            delete[] copy_arr;
            throw InvalidCast("This type is not array");
        }
        return array_t<int64_t>(meta.val_len, copy_arr);
    }

    ValueItem::operator const array_t<uint64_t>() const {
        const void* src = getSourcePtr();
        if (!meta.val_len)
            return array_t<uint64_t>();
        uint64_t* copy_arr = new uint64_t[meta.val_len];
        switch (meta.vtype) {
        case VType::raw_arr_i8:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint64_t)((const int8_t*)src)[i];
            break;
        case VType::raw_arr_ui8:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint64_t)((const uint8_t*)src)[i];
            break;
        case VType::raw_arr_i16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint64_t)((const int16_t*)src)[i];
            break;
        case VType::raw_arr_ui16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint64_t)((const uint16_t*)src)[i];
            break;
        case VType::raw_arr_i32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint64_t)((const int32_t*)src)[i];
            break;
        case VType::raw_arr_ui32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint64_t)((const uint32_t*)src)[i];
            break;
        case VType::raw_arr_i64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint64_t)((const int64_t*)src)[i];
            break;
        case VType::raw_arr_ui64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint64_t)((const uint64_t*)src)[i];
            break;
        case VType::raw_arr_flo:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint64_t)((const float*)src)[i];
            break;
        case VType::raw_arr_doub:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint64_t)((const double*)src)[i];
            break;
        case VType::faarr:
        case VType::saarr:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (uint64_t)((const ValueItem*)src)[i];
            break;
        default:
            delete[] copy_arr;
            throw InvalidCast("This type is not array");
        }
        return array_t<uint64_t>(meta.val_len, copy_arr);
    }

    ValueItem::operator const array_t<float>() const {
        const void* src = getSourcePtr();
        if (!meta.val_len)
            return array_t<float>();
        float* copy_arr = new float[meta.val_len];
        switch (meta.vtype) {
        case VType::raw_arr_i8:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (float)((const int8_t*)src)[i];
            break;
        case VType::raw_arr_ui8:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (float)((const uint8_t*)src)[i];
            break;
        case VType::raw_arr_i16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (float)((const int16_t*)src)[i];
            break;
        case VType::raw_arr_ui16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (float)((const uint16_t*)src)[i];
            break;
        case VType::raw_arr_i32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (float)((const int32_t*)src)[i];
            break;
        case VType::raw_arr_ui32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (float)((const uint32_t*)src)[i];
            break;
        case VType::raw_arr_i64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (float)((const int64_t*)src)[i];
            break;
        case VType::raw_arr_ui64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (float)((const uint64_t*)src)[i];
            break;
        case VType::raw_arr_flo:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (float)((const float*)src)[i];
            break;
        case VType::raw_arr_doub:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (float)((const double*)src)[i];
            break;
        case VType::faarr:
        case VType::saarr:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (float)((const ValueItem*)src)[i];
            break;
        default:
            delete[] copy_arr;
            throw InvalidCast("This type is not array");
        }
        return array_t<float>(meta.val_len, copy_arr);
    }

    ValueItem::operator const array_t<double>() const {
        const void* src = getSourcePtr();
        if (!meta.val_len)
            return array_t<double>();
        double* copy_arr = new double[meta.val_len];
        switch (meta.vtype) {
        case VType::raw_arr_i8:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (double)((const int8_t*)src)[i];
            break;
        case VType::raw_arr_ui8:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (double)((const uint8_t*)src)[i];
            break;
        case VType::raw_arr_i16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (double)((const int16_t*)src)[i];
            break;
        case VType::raw_arr_ui16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (double)((const uint16_t*)src)[i];
            break;
        case VType::raw_arr_i32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (double)((const int32_t*)src)[i];
            break;
        case VType::raw_arr_ui32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (double)((const uint32_t*)src)[i];
            break;
        case VType::raw_arr_i64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (double)((const int64_t*)src)[i];
            break;
        case VType::raw_arr_ui64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (double)((const uint64_t*)src)[i];
            break;
        case VType::raw_arr_flo:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (double)((const float*)src)[i];
            break;
        case VType::raw_arr_doub:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (double)((const double*)src)[i];
            break;
        case VType::faarr:
        case VType::saarr:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (double)((const ValueItem*)src)[i];
            break;
        default:
            delete[] copy_arr;
            throw InvalidCast("This type is not array");
        }
        return array_t<double>(meta.val_len, copy_arr);
    }
#ifdef _WIN32
    ValueItem::operator const array_t<long>() const {
        const void* src = getSourcePtr();
        if (!meta.val_len)
            return array_t<long>();
        long* copy_arr = new long[meta.val_len];
        switch (meta.vtype) {
        case VType::raw_arr_i8:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (long)((const int8_t*)src)[i];
            break;
        case VType::raw_arr_ui8:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (long)((const uint8_t*)src)[i];
            break;
        case VType::raw_arr_i16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (long)((const int16_t*)src)[i];
            break;
        case VType::raw_arr_ui16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (long)((const uint16_t*)src)[i];
            break;
        case VType::raw_arr_i32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (long)((const int32_t*)src)[i];
            break;
        case VType::raw_arr_ui32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (long)((const uint32_t*)src)[i];
            break;
        case VType::raw_arr_i64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (long)((const int64_t*)src)[i];
            break;
        case VType::raw_arr_ui64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (long)((const uint64_t*)src)[i];
            break;
        case VType::raw_arr_flo:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (long)((const float*)src)[i];
            break;
        case VType::raw_arr_doub:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (long)((const double*)src)[i];
            break;
        case VType::faarr:
        case VType::saarr:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (long)((const ValueItem*)src)[i];
            break;
        default:
            delete[] copy_arr;
            throw InvalidCast("This type is not array");
        }
        return array_t<long>(meta.val_len, copy_arr);
    }
#endif
    ValueItem::operator const array_t<ValueItem>() const {
        const void* src = getSourcePtr();
        if (!meta.val_len)
            return array_t<ValueItem>();
        ValueItem* copy_arr = new ValueItem[meta.val_len];
        switch (meta.vtype) {
        case VType::raw_arr_i8:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (ValueItem)((const int8_t*)src)[i];
            break;
        case VType::raw_arr_ui8:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (ValueItem)((const uint8_t*)src)[i];
            break;
        case VType::raw_arr_i16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (ValueItem)((const int16_t*)src)[i];
            break;
        case VType::raw_arr_ui16:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (ValueItem)((const uint16_t*)src)[i];
            break;
        case VType::raw_arr_i32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (ValueItem)((const int32_t*)src)[i];
            break;
        case VType::raw_arr_ui32:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (ValueItem)((const uint32_t*)src)[i];
            break;
        case VType::raw_arr_i64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (ValueItem)((const int64_t*)src)[i];
            break;
        case VType::raw_arr_ui64:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (ValueItem)((const uint64_t*)src)[i];
            break;
        case VType::raw_arr_flo:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (ValueItem)((const float*)src)[i];
            break;
        case VType::raw_arr_doub:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = (ValueItem)((const double*)src)[i];
            break;
        case VType::faarr:
        case VType::saarr:
            for (int i = 0; i < meta.val_len; i++)
                copy_arr[i] = ((const ValueItem*)src)[i];
            break;
        default:
            delete[] copy_arr;
            throw InvalidCast("This type is not array");
        }
        return array_t<ValueItem>(meta.val_len, copy_arr);
    }

    ValueItem::operator const array_ref_t<bool>() const {
        if (meta.vtype != VType::raw_arr_ui8 && meta.vtype != VType::raw_arr_i8)
            throw InvalidCast("This type is not similar to bool array");
        return array_ref_t<bool>((bool*)getSourcePtr(), meta.val_len);
    }

    ValueItem::operator const array_ref_t<int8_t>() const {
        if (meta.vtype != VType::raw_arr_i8)
            throw InvalidCast("This type is not similar to int8_t array");
        return array_ref_t<int8_t>((int8_t*)getSourcePtr(), meta.val_len);
    }

    ValueItem::operator const array_ref_t<uint8_t>() const {
        if (meta.vtype != VType::raw_arr_ui8)
            throw InvalidCast("This type is not similar to uint8_t array");
        return array_ref_t<uint8_t>((uint8_t*)getSourcePtr(), meta.val_len);
    }

    ValueItem::operator const array_ref_t<char>() const {
        if (meta.vtype != VType::raw_arr_ui8 && meta.vtype != VType::raw_arr_i8)
            throw InvalidCast("This type is not similar to uint8_t array");
        return array_ref_t<char>((char*)getSourcePtr(), meta.val_len);
    }

    ValueItem::operator const array_ref_t<int16_t>() const {
        if (meta.vtype != VType::raw_arr_i16)
            throw InvalidCast("This type is not similar to int16_t array");
        return array_ref_t<int16_t>((int16_t*)getSourcePtr(), meta.val_len);
    }

    ValueItem::operator const array_ref_t<uint16_t>() const {
        if (meta.vtype != VType::raw_arr_ui16)
            throw InvalidCast("This type is not similar to uint16_t array");
        return array_ref_t<uint16_t>((uint16_t*)getSourcePtr(), meta.val_len);
    }

    ValueItem::operator const array_ref_t<int32_t>() const {
        if (meta.vtype != VType::raw_arr_i32)
            throw InvalidCast("This type is not similar to int32_t array");
        return array_ref_t<int32_t>((int32_t*)getSourcePtr(), meta.val_len);
    }

    ValueItem::operator const array_ref_t<uint32_t>() const {
        if (meta.vtype != VType::raw_arr_ui32)
            throw InvalidCast("This type is not similar to uint32_t array");
        return array_ref_t<uint32_t>((uint32_t*)getSourcePtr(), meta.val_len);
    }

    ValueItem::operator const array_ref_t<int64_t>() const {
        if (meta.vtype != VType::raw_arr_i64)
            throw InvalidCast("This type is not similar to int64_t array");
        return array_ref_t<int64_t>((int64_t*)getSourcePtr(), meta.val_len);
    }

    ValueItem::operator const array_ref_t<uint64_t>() const {
        if (meta.vtype != VType::raw_arr_ui64)
            throw InvalidCast("This type is not similar to uint64_t array");
        return array_ref_t<uint64_t>((uint64_t*)getSourcePtr(), meta.val_len);
    }

    ValueItem::operator const array_ref_t<float>() const {
        if (meta.vtype != VType::raw_arr_flo)
            throw InvalidCast("This type is not similar to float array");
        return array_ref_t<float>((float*)getSourcePtr(), meta.val_len);
    }

    ValueItem::operator const array_ref_t<double>() const {
        if (meta.vtype != VType::raw_arr_doub)
            throw InvalidCast("This type is not similar to double array");
        return array_ref_t<double>((double*)getSourcePtr(), meta.val_len);
    }
#ifdef _WIN32
    ValueItem::operator const array_ref_t<long>() const {
        if (meta.vtype != VType::raw_arr_i64)
            throw InvalidCast("This type is not similar to long array");
        return array_ref_t<long>((long*)getSourcePtr(), meta.val_len);
    }
#endif
    ValueItem::operator const array_ref_t<ValueItem>() const {
        if (meta.vtype != VType::faarr && meta.vtype != VType::saarr)
            throw InvalidCast("This type is not similar to ValueItem array");
        return array_ref_t<ValueItem>((ValueItem*)getSourcePtr(), meta.val_len);
    }

    ValueItem::operator array_ref_t<bool>() {
        if (meta.vtype != VType::raw_arr_ui8 && meta.vtype != VType::raw_arr_i8)
            throw InvalidCast("This type is not similar to bool array");
        return array_ref_t<bool>((bool*)getSourcePtr(), meta.val_len);
    }

    ValueItem::operator array_ref_t<int8_t>() {
        if (meta.vtype != VType::raw_arr_i8)
            throw InvalidCast("This type is not similar to int8_t array");
        return array_ref_t<int8_t>((int8_t*)getSourcePtr(), meta.val_len);
    }

    ValueItem::operator array_ref_t<uint8_t>() {
        if (meta.vtype != VType::raw_arr_ui8)
            throw InvalidCast("This type is not similar to uint8_t array");
        return array_ref_t<uint8_t>((uint8_t*)getSourcePtr(), meta.val_len);
    }

    ValueItem::operator array_ref_t<int16_t>() {
        if (meta.vtype != VType::raw_arr_i16)
            throw InvalidCast("This type is not similar to int16_t array");
        return array_ref_t<int16_t>((int16_t*)getSourcePtr(), meta.val_len);
    }

    ValueItem::operator array_ref_t<uint16_t>() {
        if (meta.vtype != VType::raw_arr_ui16)
            throw InvalidCast("This type is not similar to uint16_t array");
        return array_ref_t<uint16_t>((uint16_t*)getSourcePtr(), meta.val_len);
    }

    ValueItem::operator array_ref_t<int32_t>() {
        if (meta.vtype != VType::raw_arr_i32)
            throw InvalidCast("This type is not similar to int32_t array");
        return array_ref_t<int32_t>((int32_t*)getSourcePtr(), meta.val_len);
    }

    ValueItem::operator array_ref_t<uint32_t>() {
        if (meta.vtype != VType::raw_arr_ui32)
            throw InvalidCast("This type is not similar to uint32_t array");
        return array_ref_t<uint32_t>((uint32_t*)getSourcePtr(), meta.val_len);
    }

    ValueItem::operator array_ref_t<int64_t>() {
        if (meta.vtype != VType::raw_arr_i64)
            throw InvalidCast("This type is not similar to int64_t array");
        return array_ref_t<int64_t>((int64_t*)getSourcePtr(), meta.val_len);
    }

    ValueItem::operator array_ref_t<uint64_t>() {
        if (meta.vtype != VType::raw_arr_ui64)
            throw InvalidCast("This type is not similar to uint64_t array");
        return array_ref_t<uint64_t>((uint64_t*)getSourcePtr(), meta.val_len);
    }

    ValueItem::operator array_ref_t<float>() {
        if (meta.vtype != VType::raw_arr_flo)
            throw InvalidCast("This type is not similar to float array");
        return array_ref_t<float>((float*)getSourcePtr(), meta.val_len);
    }

    ValueItem::operator array_ref_t<double>() {
        if (meta.vtype != VType::raw_arr_doub)
            throw InvalidCast("This type is not similar to double array");
        return array_ref_t<double>((double*)getSourcePtr(), meta.val_len);
    }
#ifdef _WIN32
    ValueItem::operator array_ref_t<long>() {
        if (meta.vtype != VType::raw_arr_i64)
            throw InvalidCast("This type is not similar to long array");
        return array_ref_t<long>((long*)getSourcePtr(), meta.val_len);
    }
#endif
    ValueItem::operator array_ref_t<ValueItem>() {
        if (meta.vtype != VType::faarr && meta.vtype != VType::saarr)
            throw InvalidCast("This type is not similar to ValueItem array");
        return array_ref_t<ValueItem>((ValueItem*)getSourcePtr(), meta.val_len);
    }

#pragma endregion

    ValueItem* ValueItem::operator()(ValueItem* args, uint32_t len) {
        if (meta.vtype == VType::function)
            return FuncEnvironment::sync_call((*(class art::shared_ptr<FuncEnvironment>*)getValue(val, meta)), args, len);
        else
            return new ValueItem(*this);
    }

    ValueItem& ValueItem::getAsync() {
        if (val)
            while (meta.vtype == VType::async_res)
                art::getAsyncResult(val, meta);
        return *this;
    }

    void ValueItem::getAsyncResult(ValueItem& result, uint64_t index) {
        if (val) {
            while (meta.vtype == VType::async_res) {
                art::typed_lgr<Task>& task = *(art::typed_lgr<Task>*)getSourcePtr();
                ValueItem* res = Task::get_result(task, index);
                if (res) {
                    result = *res;
                    delete res;
                    return;
                } else {
                    result = nullptr;
                    return;
                }
            }
            throw InvalidCast("This type is not async_res");
        } else
            throw NullPointerException();
    }

    void ValueItem::getGeneratorResult(ValueItem& result) {
        if (val) {
            ValueItem* res = nullptr;
            switch (meta.vtype) {
            case VType::async_res:
                res = Task::get_result(*(art::typed_lgr<Task>*)getSourcePtr());
                break;
            case VType::generator:
                res = Generator::get_result(*(art::shared_ptr<Generator>*)getSourcePtr());
                break;
            default:
                throw InvalidCast("This type is not async_res");
            }
            if (res) {
                result = *res;
                delete res;
                return;
            } else {
                result = nullptr;
                return;
            }
        } else
            throw NullPointerException();
    }

    void*& ValueItem::getSourcePtr() {
        return getValue(val, meta);
    }

    const void*& ValueItem::getSourcePtr() const {
        return const_cast<const void*&>(getValue(*const_cast<void**>(&val), const_cast<ValueMeta&>(meta)));
    }

    void*& ValueItem::unRef() {
        if (needAllocType(meta.vtype))
            return val;
        else
            return meta.as_ref ? *(void**)val : val;
    }

    const void* const& ValueItem::unRef() const {
        if (needAllocType(meta.vtype))
            return val;
        else
            return meta.as_ref ? *(void**)val : val;
    }

    art::shared_ptr<FuncEnvironment>* ValueItem::funPtr() {
        if (meta.vtype == VType::function)
            return (art::shared_ptr<FuncEnvironment>*)getValue(val, meta);
        return nullptr;
    }

    const art::shared_ptr<FuncEnvironment>* ValueItem::funPtr() const {
        if (meta.vtype == VType::function)
            return (const art::shared_ptr<FuncEnvironment>*)getSourcePtr();
        return nullptr;
    }

    template <typename T>
    size_t array_hash(T* arr, size_t len) {
        return art::hash<T>()(arr, len);
    }

    template <typename T>
    size_t array_hash(T* arr, size_t len, size_t seed) {
        return art::hash<T>().seed_hash_array(arr, len, seed);
    }

    void ValueItem::make_gc() {
        if (meta.use_gc)
            return;
        if (needAllocType(meta.vtype)) {
            void (*destructor)(void*) = nullptr;
            bool (*depth)(void*) = nullptr;
            switch (meta.vtype) {
            case VType::raw_arr_i8:
            case VType::raw_arr_ui8:
                destructor = arrayDestructor<uint8_t>;
                break;
            case VType::raw_arr_i16:
            case VType::raw_arr_ui16:
                destructor = arrayDestructor<uint16_t>;
                break;
            case VType::raw_arr_i32:
            case VType::raw_arr_ui32:
            case VType::raw_arr_flo:
                destructor = arrayDestructor<uint32_t>;
                break;
            case VType::raw_arr_i64:
            case VType::raw_arr_ui64:
            case VType::raw_arr_doub:
                destructor = arrayDestructor<uint64_t>;
                break;
            case VType::uarr:
                destructor = defaultDestructor<list_array<ValueItem>>;
                depth = calc_safe_depth_arr;
                break;
            case VType::string:
                destructor = defaultDestructor<art::ustring>;
                break;
            case VType::except_value:
                destructor = defaultDestructor<std::exception_ptr>;
                break;
            case VType::faarr:
                destructor = arrayDestructor<ValueItem>;
                break;
            case VType::struct_:
                destructor = (void (*)(void*))Structure::destruct;
                break;
            case VType::async_res:
                destructor = defaultDestructor<art::typed_lgr<Task>>;
                break;
            case VType::generator:
                destructor = defaultDestructor<art::shared_ptr<Generator>>;
                break;
            case VType::function:
                destructor = defaultDestructor<art::shared_ptr<FuncEnvironment>>;
                break;
            default:
                break;
            }
            val = new lgr(val, depth, destructor);
        } else {
            void (*destructor)(void*) = nullptr;
            void* new_val = nullptr;
            switch (meta.vtype) {
            case VType::noting:
                break;
            case VType::i8:
                new_val = new int8_t(*(int8_t*)val);
                destructor = defaultDestructor<uint8_t>;
                break;
            case VType::ui8:
                new_val = new uint8_t(*(uint8_t*)val);
                destructor = defaultDestructor<uint8_t>;
                break;
            case VType::i16:
                new_val = new int16_t(*(int16_t*)val);
                destructor = defaultDestructor<uint16_t>;
                break;
            case VType::ui16:
                new_val = new uint16_t(*(uint16_t*)val);
                destructor = defaultDestructor<uint16_t>;
                break;
            case VType::i32:
                new_val = new int32_t(*(int32_t*)val);
                destructor = defaultDestructor<uint32_t>;
                break;
            case VType::ui32:
                new_val = new uint32_t(*(uint32_t*)val);
                destructor = defaultDestructor<uint32_t>;
                break;
            case VType::flo:
                new_val = new float(*(float*)val);
                destructor = defaultDestructor<uint32_t>;
                break;
            case VType::i64:
                new_val = new int64_t(*(int64_t*)val);
                destructor = defaultDestructor<uint64_t>;
                break;
            case VType::ui64:
                new_val = new uint64_t(*(uint64_t*)val);
                destructor = defaultDestructor<uint64_t>;
                break;
            case VType::undefined_ptr:
                new_val = new void*(*(void**)val);
                destructor = defaultDestructor<uint64_t>;
                break;
            case VType::doub:
                new_val = new double(*(double*)val);
                destructor = defaultDestructor<uint64_t>;
                break;
            case VType::type_identifier:
                new_val = new ValueMeta(*(ValueMeta*)val);
                destructor = defaultDestructor<uint64_t>;
                break;
            case VType::saarr:
                *this = ValueItem((ValueItem*)val, meta.val_len);
                make_gc();
                return;
            default:
                break;
            }
            val = new lgr(new_val, nullptr, destructor);
        }
        meta.use_gc = true;
    }

    void ValueItem::localize_gc() {
        if (meta.use_gc && val != nullptr) {
            ValueMeta fake_meta = meta;
            fake_meta.use_gc = false;
            meta.as_ref = false;
            void* new_val = copyValue(getSourcePtr(), fake_meta);
            new_val = new lgr(new_val, ((lgr*)val)->getCalcDepth(), ((lgr*)val)->getDestructor());
            delete (lgr*)val;
            val = new_val;
        }
    }

    void ValueItem::ungc() {
        if (meta.use_gc && val != nullptr) {
            meta.use_gc = false;
            meta.as_ref = false;
            lgr* tmp = (lgr*)val;
            if (tmp->is_deleted()) {
                meta.encoded = 0;
                val = nullptr;
                delete tmp;
                return;
            }
            void* new_val = tmp->try_take_ptr();
            if (new_val == nullptr)
                new_val = copyValue(getSourcePtr(), meta);
            delete tmp;
            val = new_val;
        }
    }

    bool ValueItem::is_gc() {
        return meta.use_gc;
    }

    ValueItem ValueItem::make_slice(uint32_t start, uint32_t end) const {
        if (meta.val_len < end)
            end = meta.val_len;
        if (start > end)
            start = end;
        ValueMeta res_meta = meta;
        res_meta.val_len = end - start;
        if (end == start)
            return ValueItem(nullptr, res_meta, as_reference);
        switch (meta.vtype) {
        case VType::saarr:
        case VType::faarr:
            return ValueItem((ValueItem*)val + start, res_meta, as_reference);
        case VType::raw_arr_ui8:
        case VType::raw_arr_i8:
            return ValueItem((uint8_t*)val + start, res_meta, as_reference);
        case VType::raw_arr_ui16:
        case VType::raw_arr_i16:
            return ValueItem((uint16_t*)val + start, res_meta, as_reference);
        case VType::raw_arr_ui32:
        case VType::raw_arr_i32:
        case VType::raw_arr_flo:
            return ValueItem((uint32_t*)val + start, res_meta, as_reference);
        case VType::raw_arr_ui64:
        case VType::raw_arr_i64:
        case VType::raw_arr_doub:
            return ValueItem((uint64_t*)val + start, res_meta, as_reference);

        default:
            throw InvalidOperation("Can't make slice of this type: " + enum_to_string(meta.vtype));
        }
    }

    size_t ValueItem::hash(uint32_t seed) const {
        switch (meta.vtype) {
        case VType::noting:
            return 0;
        case VType::type_identifier:
        case VType::boolean:
        case VType::i8:
            return art::hash<int8_t>().seed_hash((int8_t) * this, seed);
        case VType::i16:
            return art::hash<int16_t>().seed_hash((int16_t) * this, seed);
        case VType::i32:
            return art::hash<int32_t>().seed_hash((int32_t) * this, seed);
        case VType::i64:
            return art::hash<int64_t>().seed_hash((int64_t) * this, seed);
        case VType::ui8:
            return art::hash<uint8_t>().seed_hash((uint8_t) * this, seed);
        case VType::ui16:
            return art::hash<uint16_t>().seed_hash((uint16_t) * this, seed);
        case VType::ui32:
            return art::hash<uint32_t>().seed_hash((uint32_t) * this, seed);
        case VType::undefined_ptr:
            return art::hash<size_t>().seed_hash((size_t) * this, seed);
        case VType::ui64:
            return art::hash<uint64_t>().seed_hash((uint64_t) * this, seed);
        case VType::flo:
            return art::hash<float>().seed_hash((float)*this, seed);
        case VType::doub:
            return art::hash<double>().seed_hash((double)*this, seed);
        case VType::string:
            return art::hash<art::ustring>().seed_hash(*(art::ustring*)getSourcePtr(), seed);
        case VType::uarr:
            return art::hash<list_array<ValueItem>>().seed_hash(*(list_array<ValueItem>*)getSourcePtr(), seed);
        case VType::raw_arr_i8:
            return array_hash((int8_t*)getSourcePtr(), meta.val_len, seed);
        case VType::raw_arr_i16:
            return array_hash((int16_t*)getSourcePtr(), meta.val_len, seed);
        case VType::raw_arr_i32:
            return array_hash((int32_t*)getSourcePtr(), meta.val_len, seed);
        case VType::raw_arr_i64:
            return array_hash((int64_t*)getSourcePtr(), meta.val_len, seed);
        case VType::raw_arr_ui8:
            return array_hash((uint8_t*)getSourcePtr(), meta.val_len, seed);
        case VType::raw_arr_ui16:
            return array_hash((uint16_t*)getSourcePtr(), meta.val_len, seed);
        case VType::raw_arr_ui32:
            return array_hash((uint32_t*)getSourcePtr(), meta.val_len, seed);
        case VType::raw_arr_ui64:
            return array_hash((uint64_t*)getSourcePtr(), meta.val_len, seed);
        case VType::raw_arr_flo:
            return array_hash((float*)getSourcePtr(), meta.val_len, seed);
        case VType::raw_arr_doub:
            return array_hash((double*)getSourcePtr(), meta.val_len, seed);

        case VType::saarr:
        case VType::faarr:
            return array_hash((ValueItem*)getSourcePtr(), meta.val_len, seed);

        case VType::struct_: {
            if (art::CXX::Interface::hasImplement(*this, "hash"))
                return (size_t)art::CXX::Interface::makeCall(ClassAccess::pub, *this, "hash");
            else
                return art::hash<const void*>().seed_hash(((Structure*)getSourcePtr())->self, seed);
        }
        case VType::set: {
            uint64_t hash = 0;
            for (auto& i : operator const std::unordered_set<ValueItem, art::hash<ValueItem>>&())
                hash = mur_combine(hash, i.hash(seed));
            return hash;
        }
        case VType::map: {
            size_t hash = 0;
            for (auto& i : operator const std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>&())
                hash = mur_combine(hash, mur_combine(i.second.hash(seed), i.first.hash(seed)));
            return hash;
        }
        case VType::function: {
            auto fn = funPtr();
            if (fn)
                return art::hash<const void*>().seed_hash((*fn)->get_func_ptr(), seed);
            else
                return 0;
        }
        case VType::async_res:
            throw InternalException("getAsync() not work in hash function");
        case VType::except_value:
            throw InvalidOperation("Hash function for exception not available", *(std::exception_ptr*)getSourcePtr());
        default:
            throw NotImplementedException();
            break;
        }
    }

    size_t ValueItem::hash() const {
        switch (meta.vtype) {
        case VType::noting:
            return 0;
        case VType::type_identifier:
        case VType::boolean:
        case VType::i8:
            return art::hash<int8_t>()((int8_t) * this);
        case VType::i16:
            return art::hash<int16_t>()((int16_t) * this);
        case VType::i32:
            return art::hash<int32_t>()((int32_t) * this);
        case VType::i64:
            return art::hash<int64_t>()((int64_t) * this);
        case VType::ui8:
            return art::hash<uint8_t>()((uint8_t) * this);
        case VType::ui16:
            return art::hash<uint16_t>()((uint16_t) * this);
        case VType::ui32:
            return art::hash<uint32_t>()((uint32_t) * this);
        case VType::undefined_ptr:
            return art::hash<size_t>()((size_t) * this);
        case VType::ui64:
            return art::hash<uint64_t>()((uint64_t) * this);
        case VType::flo:
            return art::hash<float>()((float)*this);
        case VType::doub:
            return art::hash<double>()((double)*this);
        case VType::string:
            return art::hash<art::ustring>()(*(art::ustring*)getSourcePtr());
        case VType::uarr:
            return art::hash<list_array<ValueItem>>()(*(list_array<ValueItem>*)getSourcePtr());
        case VType::raw_arr_i8:
            return array_hash((int8_t*)getSourcePtr(), meta.val_len);
        case VType::raw_arr_i16:
            return array_hash((int16_t*)getSourcePtr(), meta.val_len);
        case VType::raw_arr_i32:
            return array_hash((int32_t*)getSourcePtr(), meta.val_len);
        case VType::raw_arr_i64:
            return array_hash((int64_t*)getSourcePtr(), meta.val_len);
        case VType::raw_arr_ui8:
            return array_hash((uint8_t*)getSourcePtr(), meta.val_len);
        case VType::raw_arr_ui16:
            return array_hash((uint16_t*)getSourcePtr(), meta.val_len);
        case VType::raw_arr_ui32:
            return array_hash((uint32_t*)getSourcePtr(), meta.val_len);
        case VType::raw_arr_ui64:
            return array_hash((uint64_t*)getSourcePtr(), meta.val_len);
        case VType::raw_arr_flo:
            return array_hash((float*)getSourcePtr(), meta.val_len);
        case VType::raw_arr_doub:
            return array_hash((double*)getSourcePtr(), meta.val_len);

        case VType::saarr:
        case VType::faarr:
            return array_hash((ValueItem*)getSourcePtr(), meta.val_len);

        case VType::struct_: {
            if (art::CXX::Interface::hasImplement(*this, "hash"))
                return (size_t)art::CXX::Interface::makeCall(ClassAccess::pub, *this, "hash");
            else
                return art::hash<const void*>()(getSourcePtr());
        }
        case VType::set: {
            size_t hash = 0;
            for (auto& i : operator const std::unordered_set<ValueItem, art::hash<ValueItem>>&())
                hash = mur_combine(hash, i.hash());
            return hash;
        }
        case VType::map: {
            size_t hash = 0;
            for (auto& i : operator const std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>&())
                hash = mur_combine(hash, mur_combine(i.second.hash(), i.first.hash()));
            return hash;
        }
        case VType::function: {
            auto fn = funPtr();
            if (fn)
                return art::hash<const void*>()((*fn)->get_func_ptr());
            else
                return 0;
        }
        case VType::async_res:
            throw InternalException("getAsync() not work in hash function");
        case VType::except_value:
            throw InvalidOperation("Hash function for exception not available", *(std::exception_ptr*)getSourcePtr());
        default:
            throw NotImplementedException();
            break;
        }
    }

    ValueItem& ValueItem::operator[](const ValueItem& index) {
        if (meta.vtype == VType::uarr)
            return (*(list_array<ValueItem>*)getSourcePtr())[(size_t)index];
        else if (meta.vtype == VType::faarr || meta.vtype == VType::saarr) {
            size_t i = (size_t)index;
            if (i >= meta.val_len)
                throw OutOfRange();
            return ((ValueItem*)getSourcePtr())[i];
        } else if (meta.vtype == VType::map) {
            return (*(std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>*)getSourcePtr())[index];
        } else
            throw InvalidOperation("operator[]& not available for that type: " + enum_to_string(meta.vtype));
    }

    const ValueItem& ValueItem::operator[](const ValueItem& index) const {
        if (meta.vtype == VType::uarr)
            return (*(list_array<ValueItem>*)getSourcePtr())[(size_t)index];
        else if (meta.vtype == VType::faarr || meta.vtype == VType::saarr) {
            size_t i = (size_t)index;
            if (i >= meta.val_len)
                throw OutOfRange();
            return ((ValueItem*)getSourcePtr())[i];
        } else if (meta.vtype == VType::map) {
            return (*(std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>*)getSourcePtr())[index];
        } else
            throw InvalidOperation("operator[]& not available for that type: " + enum_to_string(meta.vtype));
    }

    ValueItem ValueItem::get(const ValueItem& index) const {
        if (is_raw_array(meta.vtype)) {
            size_t i = (size_t)index;
            if (i >= meta.val_len)
                throw OutOfRange();
        }
        switch (meta.vtype) {
        case VType::uarr:
            return (*(list_array<ValueItem>*)getSourcePtr())[(size_t)index];
        case VType::faarr:
        case VType::saarr:
            return ((ValueItem*)getSourcePtr())[(size_t)index];
        case VType::map:
            return (*(std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>*)getSourcePtr())[index];
        case VType::struct_: {
            Structure& st = (Structure&)*this;
            if (st.has_method(symbols::structures::index_operator, ClassAccess::pub))
                return CXX::Interface::makeCall(ClassAccess::pub, *this, symbols::structures::index_operator, index);
            else
                throw InvalidOperation("operator[] not available for that type: " + enum_to_string(meta.vtype));
        }
        case VType::set:
            return (*(std::unordered_set<ValueItem, art::hash<ValueItem>>*)getSourcePtr()).find(index) != (*(std::unordered_set<ValueItem, art::hash<ValueItem>>*)getSourcePtr()).end();
        case VType::raw_arr_i8:
            return ((int8_t*)getSourcePtr())[(size_t)index];
        case VType::raw_arr_i16:
            return ((int16_t*)getSourcePtr())[(size_t)index];
        case VType::raw_arr_i32:
            return ((int32_t*)getSourcePtr())[(size_t)index];
        case VType::raw_arr_i64:
            return ((int64_t*)getSourcePtr())[(size_t)index];
        case VType::raw_arr_ui8:
            return ((uint8_t*)getSourcePtr())[(size_t)index];
        case VType::raw_arr_ui16:
            return ((uint16_t*)getSourcePtr())[(size_t)index];
        case VType::raw_arr_ui32:
            return ((uint32_t*)getSourcePtr())[(size_t)index];
        case VType::raw_arr_ui64:
            return ((uint64_t*)getSourcePtr())[(size_t)index];
        case VType::raw_arr_flo:
            return ((float*)getSourcePtr())[(size_t)index];
        case VType::raw_arr_doub:
            return ((double*)getSourcePtr())[(size_t)index];
        default:
            throw InvalidOperation("operator[] not available for that type: " + enum_to_string(meta.vtype));
        }
    }

    void ValueItem::set(const ValueItem& index, const ValueItem& value) {
        if (is_raw_array(meta.vtype)) {
            size_t i = (size_t)index;
            if (i >= meta.val_len)
                throw OutOfRange();
        }
        switch (meta.vtype) {
        case VType::uarr:
            (*(list_array<ValueItem>*)getSourcePtr())[(size_t)index] = value;
            break;
        case VType::faarr:
        case VType::saarr:
            ((ValueItem*)getSourcePtr())[(size_t)index] = value;
            break;
        case VType::map:
            (*(std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>*)getSourcePtr())[index] = value;
            break;
        case VType::struct_: {
            Structure& st = (Structure&)*this;
            if (st.has_method(symbols::structures::index_operator, ClassAccess::pub))
                CXX::Interface::makeCall(ClassAccess::pub, *this, symbols::structures::index_operator, index, value);
            else
                throw InvalidOperation("operator[]set not available for that type: " + enum_to_string(meta.vtype));
            break;
        }
        case VType::set:
            if (value != nullptr)
                (*(std::unordered_set<ValueItem, art::hash<ValueItem>>*)getSourcePtr()).insert(index);
            else
                (*(std::unordered_set<ValueItem, art::hash<ValueItem>>*)getSourcePtr()).erase(index);
            break;
        case VType::raw_arr_i8:
            ((int8_t*)getSourcePtr())[(size_t)index] = (int8_t)value;
            break;
        case VType::raw_arr_i16:
            ((int16_t*)getSourcePtr())[(size_t)index] = (int16_t)value;
            break;
        case VType::raw_arr_i32:
            ((int32_t*)getSourcePtr())[(size_t)index] = (int32_t)value;
            break;
        case VType::raw_arr_i64:
            ((int64_t*)getSourcePtr())[(size_t)index] = (int64_t)value;
            break;
        case VType::raw_arr_ui8:
            ((uint8_t*)getSourcePtr())[(size_t)index] = (uint8_t)value;
            break;
        case VType::raw_arr_ui16:
            ((uint16_t*)getSourcePtr())[(size_t)index] = (uint16_t)value;
            break;
        case VType::raw_arr_ui32:
            ((uint32_t*)getSourcePtr())[(size_t)index] = (uint32_t)value;
            break;
        case VType::raw_arr_ui64:
            ((uint64_t*)getSourcePtr())[(size_t)index] = (uint64_t)value;
            break;
        case VType::raw_arr_flo:
            ((float*)getSourcePtr())[(size_t)index] = (float)value;
            break;
        case VType::raw_arr_doub:
            ((double*)getSourcePtr())[(size_t)index] = (double)value;
            break;
        default:
            throw InvalidOperation("operator[]set not available for that type: " + enum_to_string(meta.vtype));
        }
    }

    bool ValueItem::has(const ValueItem& index) const {
        if (is_raw_array(meta.vtype)) {
            size_t i = (size_t)index;
            return i < meta.val_len;
        }
        switch (meta.vtype) {
        case VType::uarr:
            return (*(list_array<ValueItem>*)getSourcePtr()).size() > (size_t)index;
        case VType::map:
            return (*(std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>*)getSourcePtr()).find(index) != (*(std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>*)getSourcePtr()).end();
        case VType::struct_: {
            Structure& st = (Structure&)*this;
            if (st.has_method(symbols::structures::size, ClassAccess::pub))
                return (size_t)CXX::Interface::makeCall(ClassAccess::pub, st, symbols::structures::size) > (size_t)index;
            else
                throw InvalidOperation("symbols::structures::size not available for that type: " + enum_to_string(meta.vtype));
        }
        case VType::set:
            return (*(std::unordered_set<ValueItem, art::hash<ValueItem>>*)getSourcePtr()).find(index) != (*(std::unordered_set<ValueItem, art::hash<ValueItem>>*)getSourcePtr()).end();
        default:
            throw InvalidOperation("Cannot measure, this type has value: " + enum_to_string(meta.vtype));
        }
    }

    ValueItemIterator ValueItem::begin() {
        return ValueItemIterator(*this);
    }

    ValueItemIterator ValueItem::end() {
        return ValueItemIterator(*this, true);
    }

    ValueItemConstIterator ValueItem::begin() const {
        return ValueItemConstIterator(*this);
    }

    ValueItemConstIterator ValueItem::end() const {
        return ValueItemConstIterator(*this, true);
    }

    size_t ValueItem::size() const {
        if (is_raw_array(meta.vtype))
            return meta.val_len;
        else if (meta.vtype == VType::uarr)
            return ((list_array<ValueItem>*)getSourcePtr())->size();
        else if (meta.vtype == VType::map)
            return ((std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>*)getSourcePtr())->size();
        else if (meta.vtype == VType::set)
            return ((std::unordered_set<ValueItem, art::hash<ValueItem>>*)getSourcePtr())->size();
        else if (meta.vtype == VType::struct_) {
            Structure& st = (Structure&)*this;
            if (st.has_method(symbols::structures::size, ClassAccess::pub))
                return (size_t)CXX::Interface::makeCall(ClassAccess::pub, st, symbols::structures::size);
        }
        return 0;
    }

#pragma region ValueItemIterator

    ValueItemIterator::ValueItemIterator(ValueItem& item, bool end)
        : item(item) {
        if (is_raw_array(item.meta.vtype))
            this->iterator_data = (void*)size_t(end ? item.meta.val_len : 0);

        else if (item.meta.vtype == VType::uarr)
            this->iterator_data = new list_array<ValueItem>::iterator(end ? ((list_array<ValueItem>*)item.getSourcePtr())->end() : ((list_array<ValueItem>*)item.getSourcePtr())->begin());
        else if (item.meta.vtype == VType::map) {
            std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>& map = (std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>&)item;
            if (end)
                this->iterator_data = new std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>::iterator(map.end());
            else
                this->iterator_data = new std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>::iterator(map.begin());
        } else if (item.meta.vtype == VType::set) {
            std::unordered_set<ValueItem, art::hash<ValueItem>>& set = (std::unordered_set<ValueItem, art::hash<ValueItem>>&)item;
            if (end)
                this->iterator_data = new std::unordered_set<ValueItem, art::hash<ValueItem>>::iterator(set.end());
            else
                this->iterator_data = new std::unordered_set<ValueItem, art::hash<ValueItem>>::iterator(set.begin());
        } else if (item.meta.vtype == VType::struct_) {
            Structure& st = (Structure&)item;
            if (end)
                this->iterator_data = nullptr;
            else {
                if (st.has_method(symbols::structures::iterable::begin, ClassAccess::pub))
                    this->iterator_data = new ValueItem(CXX::Interface::makeCall(ClassAccess::pub, st, symbols::structures::iterable::begin));
                else
                    throw InvalidOperation("Structure not iterable");
            }
        } else
            throw InvalidOperation("ValueItem not iterable");
    }

    ValueItemIterator::ValueItemIterator(const ValueItemIterator& copy)
        : item(copy.item) {
        if (is_raw_array(item.meta.vtype))
            this->iterator_data = (void*)size_t(copy.iterator_data);
        else if (item.meta.vtype == VType::uarr)
            this->iterator_data = new list_array<ValueItem>::iterator(*(list_array<ValueItem>::iterator*)copy.iterator_data);
        else if (item.meta.vtype == VType::map)
            this->iterator_data = new std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>::iterator(*(std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>::iterator*)copy.iterator_data);
        else if (item.meta.vtype == VType::set)
            this->iterator_data = new std::unordered_set<ValueItem, art::hash<ValueItem>>::iterator(*(std::unordered_set<ValueItem, art::hash<ValueItem>>::iterator*)copy.iterator_data);
        else if (item.meta.vtype == VType::struct_)
            this->iterator_data = new ValueItem(*(ValueItem*)copy.iterator_data);
        else
            this->iterator_data = nullptr;
    }

    ValueItemIterator::~ValueItemIterator() {
        if (item.meta.vtype == VType::map)
            delete (std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>::iterator*)iterator_data;
        else if (item.meta.vtype == VType::set)
            delete (std::unordered_set<ValueItem, art::hash<ValueItem>>::iterator*)iterator_data;
        else if (item.meta.vtype == VType::struct_)
            delete (ValueItem*)iterator_data;
        else if (item.meta.vtype == VType::uarr)
            delete (list_array<ValueItem>::iterator*)iterator_data;
    }

    ValueItemIterator& ValueItemIterator::operator++() {
        if (is_raw_array(item.meta.vtype)) {
            (*(size_t*)&iterator_data)++;
        } else if (item.meta.vtype == VType::uarr) {
            list_array<ValueItem>::iterator& i = *(list_array<ValueItem>::iterator*)iterator_data;
            ++i;
        } else if (item.meta.vtype == VType::map) {
            std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>::iterator& i = *(std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>::iterator*)iterator_data;
            ++i;
        } else if (item.meta.vtype == VType::set) {
            std::unordered_set<ValueItem, art::hash<ValueItem>>::iterator& i = *(std::unordered_set<ValueItem, art::hash<ValueItem>>::iterator*)iterator_data;
            ++i;
        } else if (item.meta.vtype == VType::struct_) {
            Structure& st = (Structure&)(ValueItem&)iterator_data;
            if (st.has_method(symbols::structures::iterable::next, ClassAccess::pub))
                iterator_data = (void*)CXX::Interface::makeCall(ClassAccess::pub, st, symbols::structures::iterable::next);
            else
                throw InvalidOperation("Structure not iterable");
        } else
            throw InvalidOperation("ValueItem not iterable");
        return *this;
    }

    ValueItemIterator ValueItemIterator::operator++(int) {
        if (is_raw_array(item.meta.vtype)) {
            ValueItemIterator copy(item, iterator_data);
            (*(size_t*)&iterator_data)++;

            if ((size_t)iterator_data > item.meta.val_len)
                throw OutOfRange();
            return copy;
        } else if (item.meta.vtype == VType::uarr) {
            list_array<ValueItem>::iterator& i = *(list_array<ValueItem>::iterator*)iterator_data;
            ValueItemIterator copy(item, new list_array<ValueItem>::iterator(i));
            i++;
            return copy;
        } else if (item.meta.vtype == VType::map) {
            std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>::iterator& i = *(std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>::iterator*)iterator_data;
            ValueItemIterator copy(item, new std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>::iterator(i));
            i++;
            return copy;
        } else if (item.meta.vtype == VType::set) {
            std::unordered_set<ValueItem, art::hash<ValueItem>>::iterator& i = *(std::unordered_set<ValueItem, art::hash<ValueItem>>::iterator*)iterator_data;
            ValueItemIterator copy(item, new std::unordered_set<ValueItem, art::hash<ValueItem>>::iterator(i));
            i++;
            return copy;
        } else if (item.meta.vtype == VType::struct_) {
            Structure& st = (Structure&)(ValueItem&)iterator_data;
            if (st.has_method(symbols::structures::iterable::next, ClassAccess::pub)) {
                ValueItemIterator copy(item, iterator_data);
                iterator_data = (void*)CXX::Interface::makeCall(ClassAccess::pub, st, symbols::structures::iterable::next);
                return copy;
            } else
                throw InvalidOperation("Structure not iterable");
        } else
            throw InvalidOperation("ValueItem not iterable");
    }

    ValueItem& ValueItemIterator::operator*() {
        if (is_raw_array(item.meta.vtype))
            return ((ValueItem*)item.getSourcePtr())[(size_t)iterator_data];
        else if (item.meta.vtype == VType::uarr) {
            list_array<ValueItem>::iterator& i = *(list_array<ValueItem>::iterator*)iterator_data;
            return *i;
        } else if (item.meta.vtype == VType::map) {
            std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>::iterator& i = *(std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>::iterator*)iterator_data;
            return i->second;
        } else if (item.meta.vtype == VType::set)
            throw InvalidOperation("Set iterator not support getting item reference by iterator");
        else if (item.meta.vtype == VType::struct_)
            throw InvalidOperation("Structure not support getting item reference by iterator");
        else
            throw InvalidOperation("ValueItem not iterable");
    }

    ValueItem* ValueItemIterator::operator->() {
        if (is_raw_array(item.meta.vtype))
            return &((ValueItem*)item.getSourcePtr())[(size_t)iterator_data];
        else if (item.meta.vtype == VType::uarr) {
            list_array<ValueItem>::iterator& i = *(list_array<ValueItem>::iterator*)iterator_data;
            return &*i;
        } else if (item.meta.vtype == VType::map) {
            std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>::iterator& i = *(std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>::iterator*)iterator_data;
            return &i->second;
        } else if (item.meta.vtype == VType::set)
            throw InvalidOperation("Set iterator not support getting item reference by iterator");
        else if (item.meta.vtype == VType::struct_)
            throw InvalidOperation("Structure not support getting item reference by iterator");
        else
            throw InvalidOperation("ValueItem not iterable");
    }

    ValueItemIterator::operator ValueItem() const {
        if (is_raw_array(item.meta.vtype))
            return ((ValueItem*)item.getSourcePtr())[(size_t)iterator_data];
        else if (item.meta.vtype == VType::uarr) {
            list_array<ValueItem>::iterator& i = *(list_array<ValueItem>::iterator*)iterator_data;
            return *i;
        } else if (item.meta.vtype == VType::map) {
            std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>::iterator& i = *(std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>::iterator*)iterator_data;
            return i->second;
        } else if (item.meta.vtype == VType::set) {
            std::unordered_set<ValueItem, art::hash<ValueItem>>::iterator& i = *(std::unordered_set<ValueItem, art::hash<ValueItem>>::iterator*)iterator_data;
            return *i;
        } else if (item.meta.vtype == VType::struct_) {
            Structure& st = (Structure&)(ValueItem&)iterator_data;
            if (st.has_method(symbols::structures::iterable::get, ClassAccess::pub)) {
                return CXX::Interface::makeCall(ClassAccess::pub, st, symbols::structures::iterable::get);
            } else
                throw InvalidOperation("Structure not iterable");
        } else
            throw InvalidOperation("ValueItem not iterable");
    }

    ValueItem ValueItemIterator::get() const {
        return (ValueItem) * this;
    }

    ValueItemIterator& ValueItemIterator::operator=(const ValueItem& item) {
        if (is_raw_array(item.meta.vtype))
            ((ValueItem*)item.getSourcePtr())[(size_t)iterator_data] = item;

        else if (item.meta.vtype == VType::uarr) {
            list_array<ValueItem>::iterator& i = *(list_array<ValueItem>::iterator*)iterator_data;
            *i = item;
        } else if (item.meta.vtype == VType::map) {
            std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>::iterator& i = *(std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>::iterator*)iterator_data;
            i->second = item;
        } else if (item.meta.vtype == VType::set)
            throw InvalidOperation("Set iterator not support getting item reference by iterator");
        else if (item.meta.vtype == VType::struct_) {
            Structure& st = (Structure&)(ValueItem&)iterator_data;
            if (st.has_method(symbols::structures::iterable::set, ClassAccess::pub))
                CXX::Interface::makeCall(ClassAccess::pub, st, symbols::structures::iterable::set, item);
            else
                throw InvalidOperation("Structure not setable");
        } else
            throw InvalidOperation("ValueItem not iterable");
        return *this;
    }

    bool ValueItemIterator::operator==(const ValueItemIterator& compare) const {
        if (&item != &compare.item)
            return false;
        if (is_raw_array(item.meta.vtype)) {
            size_t& i = (size_t&)iterator_data;
            size_t& j = (size_t&)compare.iterator_data;
            return i == j;
        } else if (item.meta.vtype == VType::uarr) {
            list_array<ValueItem>::iterator& i = *(list_array<ValueItem>::iterator*)iterator_data;
            list_array<ValueItem>::iterator& j = *(list_array<ValueItem>::iterator*)compare.iterator_data;
            return i == j;
        } else if (item.meta.vtype == VType::map) {
            std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>::iterator& i = *(std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>::iterator*)iterator_data;
            std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>::iterator& j = *(std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>::iterator*)compare.iterator_data;
            return i == j;
        } else if (item.meta.vtype == VType::set) {
            std::unordered_set<ValueItem, art::hash<ValueItem>>::iterator& i = *(std::unordered_set<ValueItem, art::hash<ValueItem>>::iterator*)iterator_data;
            std::unordered_set<ValueItem, art::hash<ValueItem>>::iterator& j = *(std::unordered_set<ValueItem, art::hash<ValueItem>>::iterator*)compare.iterator_data;
            return i == j;
        } else if (item.meta.vtype == VType::struct_) {
            Structure& st = (Structure&)item;
            if (st.has_method(symbols::structures::iterable::next, ClassAccess::pub))
                return (bool)CXX::Interface::makeCall(ClassAccess::pub, st, symbols::structures::iterable::next);
            else
                throw InvalidOperation("Structure not iterable");
        } else
            throw InvalidOperation("ValueItem not iterable");
    }

    bool ValueItemIterator::operator!=(const ValueItemIterator& compare) const {
        if (&item != &compare.item)
            return false;
        return !(*this == compare);
    }

    ValueItemConstIterator::operator ValueItem() const {
        return iterator.operator art::ValueItem();
    }

    ValueItem ValueItemConstIterator::get() const {
        return iterator.get();
    }

#pragma endregion
}