// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <library/list_array.hpp>
#include <run_time/AttachA_CXX.hpp>
#include <run_time/attacha_abi.hpp>
#include <util/exceptions.hpp>


namespace art {
    namespace helper_functions {
        inline void setSize(void** value, size_t res) {
            void*& set = getValue(*value, *(ValueMeta*)(value + 1));
            ValueMeta& meta = *(ValueMeta*)(value + 1);
            switch (meta.vtype) {
            case VType::i8:
                if (((int8_t&)set = (int8_t)res) != res)
                    throw NumericUndererflowException();
                break;
            case VType::i16:
                if (((int16_t&)set = (int16_t)res) != res)
                    throw NumericUndererflowException();
                break;
            case VType::i32:
                if (((int32_t&)set = (int32_t)res) != res)
                    throw NumericUndererflowException();
                break;
            case VType::i64:
                if (((int64_t&)set = (int64_t)res) != res)
                    throw NumericUndererflowException();
                break;
            case VType::ui8:
                if (((uint8_t&)set = (uint8_t)res) != res)
                    throw NumericOverflowException();
                break;
            case VType::ui16:
                if (((uint16_t&)set = (uint16_t)res) != res)
                    throw NumericOverflowException();
                break;
            case VType::ui32:
                if (((uint32_t&)set = (uint32_t)res) != res)
                    throw NumericOverflowException();
                break;
            case VType::ui64:
                (uint64_t&)set = res;
                break;
            case VType::flo:
                if (size_t((float&)set = (float)res) != res)
                    throw NumericOverflowException();
                break;
            case VType::doub:
                if (size_t((double&)set = (double)res) != res)
                    throw NumericOverflowException();
                break;
            default:
                throw InvalidType("Need sizable type");
            }
        }


        template <char typ>
        inline void IndexArrayCopyStatic(void** value, list_array<ValueItem>** arr_ref, uint64_t pos) {
            universalRemove(value);
            ValueMeta& meta = ((ValueMeta*)arr_ref)[1];
            list_array<ValueItem>* arr = (list_array<ValueItem>*)getValue(*(void**)arr_ref, meta);
            if constexpr (typ == 2) {
                if (arr->size() > pos) {
                    ValueItem temp = ((list_array<ValueItem>*)arr)->atDefault(pos);
                    *value = temp.val;
                    *((size_t*)(value + 1)) = temp.meta.encoded;
                    temp.val = nullptr;
                } else {
                    *value = nullptr;
                    *((size_t*)(value + 1)) = 0;
                }
            } else {
                ValueItem* res;
                if constexpr (typ == 1)
                    res = &((list_array<ValueItem>*)arr)->at(pos);
                else
                    res = &((list_array<ValueItem>*)arr)->operator[](pos);
                *value = copyValue(res->val, res->meta);
                *((size_t*)(value + 1)) = res->meta.encoded;
            }
        }
        template <char typ>
        inline void IndexArrayMoveStatic(void** value, list_array<ValueItem>** arr_ref, uint64_t pos) {
            universalRemove(value);
            ValueMeta& meta = ((ValueMeta*)arr_ref)[1];
            list_array<ValueItem>* arr = (list_array<ValueItem>*)getValue(*(void**)arr_ref, meta);
            if constexpr (typ == 2) {
                if (arr->size() > pos) {
                    ValueItem& res = ((list_array<ValueItem>*)arr)->operator[](pos);
                    *value = res.val;
                    *((size_t*)(value + 1)) = res.meta.encoded;
                    res.val = nullptr;
                    res.meta.encoded = 0;
                } else {
                    *value = nullptr;
                    *((size_t*)(value + 1)) = 0;
                }
            } else {
                ValueItem* res;
                if constexpr (typ == 1)
                    res = &((list_array<ValueItem>*)arr)->at(pos);
                else
                    res = &((list_array<ValueItem>*)arr)->operator[](pos);
                *value = res->val;
                *((size_t*)(value + 1)) = res->meta.encoded;
                res->val = nullptr;
                res->meta.encoded = 0;
            }
        }


        template <char typ>
        void IndexArraySetCopyStatic(void** value, list_array<ValueItem>** arr_ref, uint64_t pos) {
            ValueMeta& meta = ((ValueMeta*)arr_ref)[1];
            list_array<ValueItem>* arr = (list_array<ValueItem>*)getValue(*(void**)arr_ref, meta);
            if constexpr (typ == 2) {
                if (((list_array<ValueItem>*)arr)->size() <= pos)
                    ((list_array<ValueItem>*)arr)->resize(pos + 1);
                ((list_array<ValueItem>*)arr)->operator[](pos) = reinterpret_cast<ValueItem&>(value);
            } else {
                if constexpr (typ == 1)
                    ((list_array<ValueItem>*)arr)->at(pos) = reinterpret_cast<ValueItem&>(value);
                else
                    ((list_array<ValueItem>*)arr)->operator[](pos) = reinterpret_cast<ValueItem&>(value);
            }
        }
        template <char typ>
        void IndexArraySetMoveStatic(void** value, list_array<ValueItem>** arr_ref, uint64_t pos) {
            ValueMeta& meta = ((ValueMeta*)arr_ref)[1];
            list_array<ValueItem>* arr = (list_array<ValueItem>*)getValue(*(void**)arr_ref, meta);
            if constexpr (typ == 2) {
                if (((list_array<ValueItem>*)arr)->size() <= pos)
                    ((list_array<ValueItem>*)arr)->resize(pos + 1);
                ((list_array<ValueItem>*)arr)->operator[](pos) = reinterpret_cast<ValueItem&&>(value);
            } else {
                if constexpr (typ == 1)
                    ((list_array<ValueItem>*)arr)->at(pos) = reinterpret_cast<ValueItem&&>(value);
                else
                    ((list_array<ValueItem>*)arr)->operator[](pos) = reinterpret_cast<ValueItem&&>(value);
            }
        }


        template <char typ, class T>
        void IndexArrayCopyStatic(void** value, void** arr_ref, uint64_t pos) {
            universalRemove(value);
            ValueMeta& meta = ((ValueMeta*)arr_ref)[1];
            T* arr = (T*)getValue(*arr_ref, meta);
            if constexpr (typ == 2) {
                if (meta.val_len <= pos) {
                    *value = nullptr;
                    *((size_t*)(value + 1)) = 0;
                    return;
                }
            } else if constexpr (typ == 1)
                if (meta.val_len > pos)
                    throw OutOfRange();

            ValueItem temp = arr[pos];
            *value = temp.val;
            *((size_t*)(value + 1)) = temp.meta.encoded;
            temp.val = nullptr;
        }
        template <char typ, class T>
        void IndexArrayMoveStatic(void** value, void** arr_ref, uint64_t pos) {
            universalRemove(value);
            ValueMeta& meta = ((ValueMeta*)arr_ref)[1];
            T* arr = (T*)getValue(*arr_ref, meta);
            if constexpr (typ == 2) {
                if (meta.val_len <= pos) {
                    *value = nullptr;
                    *((size_t*)(value + 1)) = 0;
                    return;
                }
            } else if constexpr (typ == 1)
                if (meta.val_len > pos)
                    throw OutOfRange();

            ValueItem temp = std::move(arr[pos]);
            if (!std::is_same_v<T, ValueItem>)
                arr[pos] = T();
            *value = temp.val;
            *((size_t*)(value + 1)) = temp.meta.encoded;
            temp.val = nullptr;
        }


        template <char typ, class T>
        void IndexArraySetCopyStatic(void** value, void** arr_ref, uint32_t pos) {
            ValueMeta& meta = ((ValueMeta*)arr_ref)[1];
            T* arr = (T*)getValue(*arr_ref, meta);
            if constexpr (typ != 0) {
                if (meta.val_len <= pos)
                    throw OutOfRange();
            }
            if constexpr (std::is_same_v<T, ValueItem>)
                arr[pos] = *reinterpret_cast<ValueItem*>(value);
            else
                arr[pos] = (T) * reinterpret_cast<ValueItem*>(value);
        }
        template <char typ, class T>
        void IndexArraySetMoveStatic(void** value, void** arr_ref, uint32_t pos) {
            ValueMeta& meta = ((ValueMeta*)arr_ref)[1];
            T* arr = (T*)getValue(*arr_ref, meta);
            if constexpr (typ != 0) {
                if (meta.val_len <= pos)
                    throw OutOfRange();
            }
            arr[pos] = (T) reinterpret_cast<ValueItem&&>(value);
            if (!std::is_same_v<T, ValueItem>)
                reinterpret_cast<ValueItem&>(value) = ValueItem();
        }


        template <char typ>
        void IndexArrayStaticInterface(void** value, ValueItem* arr_ref, uint64_t pos) noexcept(false) {
            universalRemove(value);
            size_t length = (size_t)CXX::Interface::makeCall(ClassAccess::pub, *arr_ref, symbols::structures::size);
            if constexpr (typ == 2) {
                if (length <= pos) {
                    *value = nullptr;
                    *((size_t*)(value + 1)) = 0;
                    return;
                }
            } else if constexpr (typ == 1)
                if (length > pos)
                    throw OutOfRange();
            ValueItem temp =
                CXX::Interface::makeCall(ClassAccess::pub, *arr_ref, symbols::structures::index_operator, pos);
            *value = temp.val;
            *((size_t*)(value + 1)) = temp.meta.encoded;
            temp.val = nullptr;
        }
        template <char typ>
        void IndexArraySetStaticInterface(void** value, ValueItem* arr_ref, uint64_t pos) {
            size_t length = (size_t)CXX::Interface::makeCall(ClassAccess::pub, *arr_ref, symbols::structures::size);
            if constexpr (typ != 0) {
                if (length <= pos)
                    throw OutOfRange();
            }
            CXX::Interface::makeCall(ClassAccess::pub,
                                     *arr_ref,
                                     symbols::structures::index_set_operator,
                                     pos,
                                     reinterpret_cast<ValueItem&>(value));
        }

        template <char typ>
        void inlineIndexArraySetCopyStatic(BuildCall& b, VType type) {
            switch (type) {
            case VType::raw_arr_i8:
                b.finalize(IndexArraySetCopyStatic<typ, int8_t>);
                break;
            case VType::raw_arr_i16:
                b.finalize(IndexArraySetCopyStatic<typ, int16_t>);
                break;
            case VType::raw_arr_i32:
                b.finalize(IndexArraySetCopyStatic<typ, int32_t>);
                break;
            case VType::raw_arr_i64:
                b.finalize(IndexArraySetCopyStatic<typ, int64_t>);
                break;
            case VType::raw_arr_ui8:
                b.finalize(IndexArraySetCopyStatic<typ, uint8_t>);
                break;
            case VType::raw_arr_ui16:
                b.finalize(IndexArraySetCopyStatic<typ, uint16_t>);
                break;
            case VType::raw_arr_ui32:
                b.finalize(IndexArraySetCopyStatic<typ, uint32_t>);
                break;
            case VType::raw_arr_ui64:
                b.finalize(IndexArraySetCopyStatic<typ, uint64_t>);
                break;
            case VType::raw_arr_flo:
                b.finalize(IndexArraySetCopyStatic<typ, float>);
                break;
            case VType::raw_arr_doub:
                b.finalize(IndexArraySetCopyStatic<typ, double>);
                break;
            case VType::uarr:
                b.finalize(IndexArraySetCopyStatic<typ>);
                break;
            case VType::faarr:
            case VType::saarr:
                b.finalize(IndexArraySetCopyStatic<typ, ValueItem>);
                break;
            case VType::struct_:
                b.finalize(IndexArraySetStaticInterface<0>);
                break;
            default:
                throw InvalidIL("Invalid opcode, unsupported static type for this operation");
            }
        }
        template <char typ>
        void inlineIndexArraySetMoveStatic(BuildCall& b, VType type) {
            switch (type) {
            case VType::raw_arr_i8:
                b.finalize(IndexArraySetMoveStatic<typ, int8_t>);
                break;
            case VType::raw_arr_i16:
                b.finalize(IndexArraySetMoveStatic<typ, int16_t>);
                break;
            case VType::raw_arr_i32:
                b.finalize(IndexArraySetMoveStatic<typ, int32_t>);
                break;
            case VType::raw_arr_i64:
                b.finalize(IndexArraySetMoveStatic<typ, int64_t>);
                break;
            case VType::raw_arr_ui8:
                b.finalize(IndexArraySetMoveStatic<typ, uint8_t>);
                break;
            case VType::raw_arr_ui16:
                b.finalize(IndexArraySetMoveStatic<typ, uint16_t>);
                break;
            case VType::raw_arr_ui32:
                b.finalize(IndexArraySetMoveStatic<typ, uint32_t>);
                break;
            case VType::raw_arr_ui64:
                b.finalize(IndexArraySetMoveStatic<typ, uint64_t>);
                break;
            case VType::raw_arr_flo:
                b.finalize(IndexArraySetMoveStatic<typ, float>);
                break;
            case VType::raw_arr_doub:
                b.finalize(IndexArraySetMoveStatic<typ, double>);
                break;
            case VType::uarr:
                b.finalize(IndexArraySetMoveStatic<typ>);
                break;
            case VType::faarr:
            case VType::saarr:
                b.finalize(IndexArraySetMoveStatic<typ, ValueItem>);
                break;
            default:
                throw InvalidIL("Invalid opcode, unsupported static type for this operation");
            }
        }

        template <char typ>
        void inlineIndexArrayCopyStatic(BuildCall& b, VType type) {
            switch (type) {
            case VType::raw_arr_i8:
                b.finalize(IndexArrayCopyStatic<typ, int8_t>);
                break;
            case VType::raw_arr_i16:
                b.finalize(IndexArrayCopyStatic<typ, int16_t>);
                break;
            case VType::raw_arr_i32:
                b.finalize(IndexArrayCopyStatic<typ, int32_t>);
                break;
            case VType::raw_arr_i64:
                b.finalize(IndexArrayCopyStatic<typ, int64_t>);
                break;
            case VType::raw_arr_ui8:
                b.finalize(IndexArrayCopyStatic<typ, uint8_t>);
                break;
            case VType::raw_arr_ui16:
                b.finalize(IndexArrayCopyStatic<typ, uint16_t>);
                break;
            case VType::raw_arr_ui32:
                b.finalize(IndexArrayCopyStatic<typ, uint32_t>);
                break;
            case VType::raw_arr_ui64:
                b.finalize(IndexArrayCopyStatic<typ, uint64_t>);
                break;
            case VType::raw_arr_flo:
                b.finalize(IndexArrayCopyStatic<typ, float>);
                break;
            case VType::raw_arr_doub:
                b.finalize(IndexArrayCopyStatic<typ, double>);
                break;
            case VType::uarr:
                b.finalize(IndexArrayCopyStatic<typ>);
                break;
            case VType::faarr:
            case VType::saarr:
                b.finalize(IndexArrayCopyStatic<typ, ValueItem>);
                break;
            case VType::struct_:
                b.finalize(IndexArrayStaticInterface<0>);
                break;
            default:
                throw InvalidIL("Invalid opcode, unsupported static type for this operation");
            }
        }
        template <char typ>
        void inlineIndexArrayMoveStatic(BuildCall& b, VType type) {
            switch (type) {
            case VType::raw_arr_i8:
                b.finalize(IndexArrayMoveStatic<typ, int8_t>);
                break;
            case VType::raw_arr_i16:
                b.finalize(IndexArrayMoveStatic<typ, int16_t>);
                break;
            case VType::raw_arr_i32:
                b.finalize(IndexArrayMoveStatic<typ, int32_t>);
                break;
            case VType::raw_arr_i64:
                b.finalize(IndexArrayMoveStatic<typ, int64_t>);
                break;
            case VType::raw_arr_ui8:
                b.finalize(IndexArrayMoveStatic<typ, uint8_t>);
                break;
            case VType::raw_arr_ui16:
                b.finalize(IndexArrayMoveStatic<typ, uint16_t>);
                break;
            case VType::raw_arr_ui32:
                b.finalize(IndexArrayMoveStatic<typ, uint32_t>);
                break;
            case VType::raw_arr_ui64:
                b.finalize(IndexArrayMoveStatic<typ, uint64_t>);
                break;
            case VType::raw_arr_flo:
                b.finalize(IndexArrayMoveStatic<typ, float>);
                break;
            case VType::raw_arr_doub:
                b.finalize(IndexArrayMoveStatic<typ, double>);
                break;
            case VType::uarr:
                b.finalize(IndexArrayMoveStatic<typ>);
                break;
            case VType::faarr:
            case VType::saarr:
                b.finalize(IndexArrayMoveStatic<typ, ValueItem>);
                break;
            default:
                throw InvalidIL("Invalid opcode, unsupported static type for this operation");
            }
        }

        template <char typ>
        void IndexArrayCopyDynamic(void** value, ValueItem* arr, uint64_t pos) {
            switch (arr->meta.vtype) {
            case VType::uarr:
                IndexArrayCopyStatic<typ>(value, (list_array<ValueItem>**)arr, pos);
                break;
            case VType::faarr:
            case VType::saarr:
                IndexArrayCopyStatic<typ, ValueItem>(value, (void**)arr, pos);
                break;
            case VType::raw_arr_i8:
                IndexArrayCopyStatic<typ, int8_t>(value, (void**)arr, pos);
                break;
            case VType::raw_arr_i16:
                IndexArrayCopyStatic<typ, int16_t>(value, (void**)arr, pos);
                break;
            case VType::raw_arr_i32:
                IndexArrayCopyStatic<typ, int32_t>(value, (void**)arr, pos);
                break;
            case VType::raw_arr_i64:
                IndexArrayCopyStatic<typ, int64_t>(value, (void**)arr, pos);
                break;
            case VType::raw_arr_ui8:
                IndexArrayCopyStatic<typ, uint8_t>(value, (void**)arr, pos);
                break;
            case VType::raw_arr_ui16:
                IndexArrayCopyStatic<typ, uint16_t>(value, (void**)arr, pos);
                break;
            case VType::raw_arr_ui32:
                IndexArrayCopyStatic<typ, uint32_t>(value, (void**)arr, pos);
                break;
            case VType::raw_arr_ui64:
                IndexArrayCopyStatic<typ, uint64_t>(value, (void**)arr, pos);
                break;
            case VType::raw_arr_flo:
                IndexArrayCopyStatic<typ, uint64_t>(value, (void**)arr, pos);
                break;
            case VType::raw_arr_doub:
                IndexArrayCopyStatic<typ, uint64_t>(value, (void**)arr, pos);
                break;
            case VType::struct_:
                IndexArrayStaticInterface<typ>(value, arr, pos);
                break;
            default:
                throw NotImplementedException();
            }
        }
        template <char typ>
        void IndexArrayMoveDynamic(void** value, ValueItem* arr, uint64_t pos) {
            switch (arr->meta.vtype) {
            case VType::uarr:
                IndexArrayMoveStatic<typ>(value, (list_array<ValueItem>**)arr, pos);
                break;
            case VType::faarr:
            case VType::saarr:
                IndexArrayMoveStatic<typ, ValueItem>(value, (void**)arr, pos);
                break;
            case VType::raw_arr_i8:
                IndexArrayMoveStatic<typ, int8_t>(value, (void**)arr, pos);
                break;
            case VType::raw_arr_i16:
                IndexArrayMoveStatic<typ, int16_t>(value, (void**)arr, pos);
                break;
            case VType::raw_arr_i32:
                IndexArrayMoveStatic<typ, int32_t>(value, (void**)arr, pos);
                break;
            case VType::raw_arr_i64:
                IndexArrayMoveStatic<typ, int64_t>(value, (void**)arr, pos);
                break;
            case VType::raw_arr_ui8:
                IndexArrayMoveStatic<typ, uint8_t>(value, (void**)arr, pos);
                break;
            case VType::raw_arr_ui16:
                IndexArrayMoveStatic<typ, uint16_t>(value, (void**)arr, pos);
                break;
            case VType::raw_arr_ui32:
                IndexArrayMoveStatic<typ, uint32_t>(value, (void**)arr, pos);
                break;
            case VType::raw_arr_ui64:
                IndexArrayMoveStatic<typ, uint64_t>(value, (void**)arr, pos);
                break;
            case VType::raw_arr_flo:
                IndexArrayMoveStatic<typ, uint64_t>(value, (void**)arr, pos);
                break;
            case VType::raw_arr_doub:
                IndexArrayMoveStatic<typ, uint64_t>(value, (void**)arr, pos);
                break;
            default:
                throw NotImplementedException();
            }
        }


        template <char typ>
        void IndexArraySetCopyDynamic(void** value, ValueItem* arr, uint64_t pos) {
            switch (arr->meta.vtype) {
            case VType::uarr:
                IndexArraySetCopyStatic<typ>(value, (list_array<ValueItem>**)arr, pos);
                break;
            case VType::faarr:
            case VType::saarr:
                IndexArraySetCopyStatic<typ, ValueItem>(value, (void**)arr, (uint32_t)pos);
                break;
            case VType::raw_arr_i8:
                IndexArraySetCopyStatic<typ, int8_t>(value, (void**)arr, (uint32_t)pos);
                break;
            case VType::raw_arr_i16:
                IndexArraySetCopyStatic<typ, int16_t>(value, (void**)arr, (uint32_t)pos);
                break;
            case VType::raw_arr_i32:
                IndexArraySetCopyStatic<typ, int32_t>(value, (void**)arr, (uint32_t)pos);
                break;
            case VType::raw_arr_i64:
                IndexArraySetCopyStatic<typ, int64_t>(value, (void**)arr, (uint32_t)pos);
                break;
            case VType::raw_arr_ui8:
                IndexArraySetCopyStatic<typ, uint8_t>(value, (void**)arr, (uint32_t)pos);
                break;
            case VType::raw_arr_ui16:
                IndexArraySetCopyStatic<typ, uint16_t>(value, (void**)arr, (uint32_t)pos);
                break;
            case VType::raw_arr_ui32:
                IndexArraySetCopyStatic<typ, uint32_t>(value, (void**)arr, (uint32_t)pos);
                break;
            case VType::raw_arr_ui64:
                IndexArraySetCopyStatic<typ, uint64_t>(value, (void**)arr, (uint32_t)pos);
                break;
            case VType::raw_arr_flo:
                IndexArraySetCopyStatic<typ, uint64_t>(value, (void**)arr, (uint32_t)pos);
                break;
            case VType::raw_arr_doub:
                IndexArraySetCopyStatic<typ, uint64_t>(value, (void**)arr, (uint32_t)pos);
                break;
            case VType::struct_:
                IndexArraySetStaticInterface<typ>(value, arr, pos);
                break;
            default:
                throw NotImplementedException();
            }
        }
        template <char typ>
        void IndexArraySetMoveDynamic(void** value, ValueItem* arr, uint64_t pos) {
            switch (arr->meta.vtype) {
            case VType::uarr:
                IndexArraySetMoveStatic<typ>(value, (list_array<ValueItem>**)arr, pos);
                break;
            case VType::faarr:
            case VType::saarr:
                IndexArraySetMoveStatic<typ, ValueItem>(value, (void**)arr, (uint32_t)pos);
                break;
            case VType::raw_arr_i8:
                IndexArraySetMoveStatic<typ, int8_t>(value, (void**)arr, (uint32_t)pos);
                break;
            case VType::raw_arr_i16:
                IndexArraySetMoveStatic<typ, int16_t>(value, (void**)arr, (uint32_t)pos);
                break;
            case VType::raw_arr_i32:
                IndexArraySetMoveStatic<typ, int32_t>(value, (void**)arr, (uint32_t)pos);
                break;
            case VType::raw_arr_i64:
                IndexArraySetMoveStatic<typ, int64_t>(value, (void**)arr, (uint32_t)pos);
                break;
            case VType::raw_arr_ui8:
                IndexArraySetMoveStatic<typ, uint8_t>(value, (void**)arr, (uint32_t)pos);
                break;
            case VType::raw_arr_ui16:
                IndexArraySetMoveStatic<typ, uint16_t>(value, (void**)arr, (uint32_t)pos);
                break;
            case VType::raw_arr_ui32:
                IndexArraySetMoveStatic<typ, uint32_t>(value, (void**)arr, (uint32_t)pos);
                break;
            case VType::raw_arr_ui64:
                IndexArraySetMoveStatic<typ, uint64_t>(value, (void**)arr, (uint32_t)pos);
                break;
            case VType::raw_arr_flo:
                IndexArraySetMoveStatic<typ, uint64_t>(value, (void**)arr, (uint32_t)pos);
                break;
            case VType::raw_arr_doub:
                IndexArraySetMoveStatic<typ, uint64_t>(value, (void**)arr, (uint32_t)pos);
                break;
            default:
                throw NotImplementedException();
            }
        }
        inline void takeStart(list_array<ValueItem>* dest, void** insert) {
            reinterpret_cast<ValueItem&>(insert) = dest->take_front();
        }
        inline void takeEnd(list_array<ValueItem>* dest, void** insert) {
            reinterpret_cast<ValueItem&>(insert) = dest->take_back();
        }
        inline void take(size_t pos, list_array<ValueItem>* dest, void** insert) {
            reinterpret_cast<ValueItem&>(insert) = dest->take(pos);
        }
        inline void getRange(list_array<ValueItem>* dest, void** insert, size_t start, size_t end) {
            auto tmp = dest->range(start, end);
            reinterpret_cast<ValueItem&>(insert) = list_array<ValueItem>(tmp.begin(), tmp.end());
        }
        inline void takeRange(list_array<ValueItem>* dest, void** insert, size_t start, size_t end) {
            reinterpret_cast<ValueItem&>(insert) = dest->take(start, end);
        }


        inline void throwEx(ValueItem* typ, ValueItem* desc) {
            static ValueItem noting;
            if (desc == nullptr)
                desc = &noting;
            if (typ == nullptr)
                typ = &noting;
            if (typ->meta.vtype == VType::string && desc->meta.vtype == VType::string)
                throw AException(*(art::ustring*)typ->getSourcePtr(), *(art::ustring*)desc->getSourcePtr());
            else if (typ->meta.vtype == VType::string)
                throw AException(*(art::ustring*)typ->getSourcePtr(), (art::ustring)*desc);
            else if (desc->meta.vtype == VType::string)
                throw AException((art::ustring)*typ, *(art::ustring*)desc->getSourcePtr());
            else
                throw AException((art::ustring)*typ, (art::ustring)*desc);
        }
        namespace throwEx_static {
            inline void throwEx_s_s(ValueItem* typ, ValueItem* desc) {
                throw AException(*(art::ustring*)typ->val, *(art::ustring*)desc->val);
            }
            inline void throwEx_gs_gs(ValueItem* typ, ValueItem* desc) {
                throw AException(*(art::ustring*)typ->getSourcePtr(), *(art::ustring*)desc->getSourcePtr());
            }
            inline void throwEx_gs_s(ValueItem* typ, ValueItem* desc) {
                throw AException(*(art::ustring*)typ->getSourcePtr(), (art::ustring)*desc);
            }
            inline void throwEx_s_gs(ValueItem* typ, ValueItem* desc) {
                throw AException((art::ustring)*typ, *(art::ustring*)desc->getSourcePtr());
            }
            inline void throwEx_s_0(ValueItem* typ) {
                throw AException(*(art::ustring*)typ->val, "");
            }
            inline void throwEx_0_s(ValueItem* desc) {
                throw AException("", *(art::ustring*)desc->val);
            }
            inline void throwEx_gs_0(ValueItem* typ) {
                throw AException(*(art::ustring*)typ->getSourcePtr(), "");
            }
            inline void throwEx_0_gs(ValueItem* desc) {
                throw AException("", *(art::ustring*)desc->getSourcePtr());
            }
            inline void throwEx_a_a(ValueItem* typ, ValueItem* desc) {
                throw AException((art::ustring)*typ, (art::ustring)*desc);
            }
            inline void throwEx_0_a(ValueItem* desc) {
                throw AException("", (art::ustring)*desc);
            }
            inline void throwEx_a_0(ValueItem* typ) {
                throw AException((art::ustring)*typ, "");
            }
        }


        inline void setValue(void*& val, void* set, ValueMeta meta) {
            val = copyValue(set, meta);
        }
        inline void getInterfaceValue(ClassAccess access,
                                      ValueItem* val,
                                      const art::ustring* val_name,
                                      ValueItem* res) {
            *res = art::CXX::Interface::getValue(access, *val, *val_name);
        }
        inline void* prepareStack(void** stack, size_t size) {
            while (size)
                stack[--size] = nullptr;
            return stack;
        }

        template <typename T>
        void valueDestruct(void*& val) {
            if (val != nullptr) {
                ((T*)val)->~T();
                val = nullptr;
            }
        }
        inline void valueDestructDyn(void** val) {
            if (val != nullptr) {
                ((ValueItem*)val)->~ValueItem();
                val = nullptr;
            }
        }
        inline ValueItem* ValueItem_is_gc_proxy(ValueItem* val) {
            return new ValueItem(val->meta.use_gc);
        }
        inline void ValueItem_xmake_slice00(ValueItem* result, ValueItem* val, uint32_t start, uint32_t end) {
            *result = val->make_slice(start, end);
        }
        inline void ValueItem_xmake_slice10(ValueItem* result, ValueItem* val, ValueItem* start, uint32_t end) {
            *result = val->make_slice((uint32_t)*start, end);
        }
        inline void ValueItem_xmake_slice01(ValueItem* result, ValueItem* val, uint32_t start, ValueItem* end) {
            *result = val->make_slice(start, (uint32_t)*end);
        }
        inline void ValueItem_xmake_slice11(ValueItem* result, ValueItem* val, ValueItem* start, ValueItem* end) {
            *result = val->make_slice((uint32_t)*start, (uint32_t)*end);
        }
        inline void Unchecked_make_slice00(ValueItem* result, ValueItem* arr, uint32_t start, uint32_t end) {
            *result = ValueItem(arr + start, ValueMeta(VType::faarr, false, true, end - start), as_reference);
        }

        inline void ValueItem_make_ref(ValueItem* result, ValueItem* source) {
            *result = ValueItem(*source, as_reference);
        }
        inline void ValueItem_copy_unref(ValueItem* result, ValueItem* source) {
            ValueMeta meta = source->meta;
            meta.as_ref = false;
            *result = ValueItem(source->unRef(), meta);
        }
        inline void ValueItem_move_unref(ValueItem* result, ValueItem* source) {
            ValueMeta meta = source->meta;
            meta.as_ref = false;
            *result = ValueItem(source->unRef(), meta, no_copy);
            *source = nullptr;
        }
    }
}
