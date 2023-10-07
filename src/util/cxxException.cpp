// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <util/cxxException.hpp>
#include <util/platform.hpp>
#include <util/ustring.hpp>
#ifdef PLATFORM_WINDOWS
#include <string>
#include <windows.h>

namespace art {
    namespace except_abi {
        namespace {
            struct ExThrowInfo {
                uint32_t attributes;
                int32_t clean_up;
                int32_t forward_compat;
                int32_t catchable_type_array;
            };

            struct ExCatchableTypeArray {
                uint32_t catchable_types;

                // It is variable size but we only need the first element of this array
                uint32_t array_of_catchable_types(size_t index) const {
                    return reinterpret_cast<uint32_t*>(reinterpret_cast<size_t>(this) + sizeof(uint32_t))[index];
                }
            };

            struct ExCatchableType {
                uint32_t properties; // bit field  IsScalar 1; RefrenceOnly: 2, HasVirtualBases 4; isWinRT: 8, IsStdBadAlloc 16
                int32_t type_info;
                uint32_t non_virtual_adjustment;
                uint32_t offset_to_virtual_base_ptr;
                uint32_t virtual_base_table_index;
                uint32_t size;
                int32_t copy_function;
            };

            struct internal_exception_ptr {
                EXCEPTION_RECORD* record;
                int* ref;
            };
        }

        CXXExInfo exceptCXXDetails(LPEXCEPTION_RECORD e) {
            CXXExInfo ex;
            if (e->ExceptionCode != 0xe06d7363) {
                //this is not a c++ exception
                ex.native_id = e->ExceptionCode;
                ex.ex_data_0 = e->ExceptionInformation[0];
                ex.ex_data_1 = e->ExceptionInformation[1];
                ex.ex_data_2 = e->ExceptionInformation[2];
                ex.ex_data_3 = e->ExceptionInformation[3];
                return ex;
            }
            const ExThrowInfo* throwInfo = (const ExThrowInfo*)e->ExceptionInformation[2];
            const ExCatchableTypeArray* cArray = (const ExCatchableTypeArray*)(e->ExceptionInformation[3] + throwInfo->catchable_type_array);
            ex.ex_ptr = (const void*)e->ExceptionInformation[1];
            uint32_t count = cArray->catchable_types;
            for (uint32_t i = 0; i < count; i++) {
                const ExCatchableType* ss = (const ExCatchableType*)(e->ExceptionInformation[3] + cArray->array_of_catchable_types(i));
                CXXExInfo::Tys tys{
                    (const std::type_info*)(e->ExceptionInformation[3] + ss->type_info),
                    (const void*)(e->ExceptionInformation[3] + ss->copy_function),
                    ss->size,
                    (bool)(ss->properties & 16),
                    (bool)(ss->properties & 2)};

                ex.ty_arr.push_back(tys);
            }
            ex.ty_arr.shrink_to_fit();
            ex.cleanup_fn = (const void*)(e->ExceptionInformation[3] + throwInfo->clean_up);
            return ex;
        }

        CXXExInfo exceptCXXDetails(LPEXCEPTION_POINTERS e) {
            return exceptCXXDetails(e->ExceptionRecord);
        }

        static void ex_rethrow(const std::exception_ptr& ex) {
            std::rethrow_exception(ex);
        }

        static int getCxxExInfoFromException(CXXExInfo& res, void* e) {
            res = except_abi::exceptCXXDetails((LPEXCEPTION_POINTERS)e);
            return EXCEPTION_EXECUTE_HANDLER;
        }

        static void getCxxExInfoFromException(CXXExInfo& res, const std::exception_ptr& ex) {
            res = exceptCXXDetails(reinterpret_cast<const internal_exception_ptr&>(ex).record);
        }
    }

    void getCxxExInfoFromException(CXXExInfo& res, const std::exception_ptr& ex) {
        except_abi::getCxxExInfoFromException(res, ex);
    }

    void getCxxExInfoFromNative(CXXExInfo& res, void* ex_ptr) {
        res = except_abi::exceptCXXDetails((LPEXCEPTION_POINTERS)ex_ptr);
    }

    void getCxxExInfoFromNative1(CXXExInfo& res, void* ex_ptr) {
        res = except_abi::exceptCXXDetails((LPEXCEPTION_RECORD)ex_ptr);
    }

    bool hasClassInEx(CXXExInfo& cxx, const char* class_nam) {
        art::ustring str = art::ustring("class ") + class_nam;
        return cxx.ty_arr.contains_one([&str, class_nam](const CXXExInfo::Tys& ty) {
            if (ty.is_bad_alloc)
                if (strcmp(class_nam, "bad_alloc") == 0)
                    return true;
            return ty.ty_info->name() == str;
        });
    }

    bool isBadAlloc(CXXExInfo& cxx) {
        return cxx.ty_arr.contains_one([](const CXXExInfo::Tys& ty) { return ty.is_bad_alloc; });
    }

    void* getExPtrFromException(const std::exception_ptr& ex) {
        return (void*)reinterpret_cast<const except_abi::internal_exception_ptr&>(ex).record->ExceptionInformation[1];
    }

    void CXXExInfo::make_cleanup() {
        if (cleanup_fn) {
            if (ty_arr.size())
                if (ty_arr[0].is_refrence_only)
                    return;
            ((void (*)(const void*))cleanup_fn)(ex_ptr);
        }
    }
}
#else
namespace art {
    void getCxxExInfoFromException(CXXExInfo& res, const std::exception_ptr& ex) {}

    void getCxxExInfoFromNative(CXXExInfo& res, void*) {}

    void getCxxExInfoFromNative1(CXXExInfo& res, void*) {}

    bool hasClassInEx(CXXExInfo& cxx, const char* class_nam) {
        return false;
    }

    bool isBadAlloc(CXXExInfo& cxx) {
        return false;
    }

    void CXXExInfo::make_cleanup() {
        if (cleanup_fn)
            ((void (*)(const void*))cleanup_fn)(ex_ptr);
    }

    void* getExPtrFromException(const std::exception_ptr& ex) {
        return nullptr;
    }
}
#endif
namespace art {

    CXXExInfo::Tys::Tys(const Tys& copy) {
        *this = copy;
    }

    CXXExInfo::Tys::Tys(Tys&& move) {
        *this = std::move(move);
    }

    CXXExInfo::Tys& CXXExInfo::Tys::operator=(const Tys& copy) {
        ty_info = copy.ty_info;
        copy_fn = copy.copy_fn;
        is_bad_alloc = copy.is_bad_alloc;
        is_refrence_only = copy.is_refrence_only;
        return *this;
    }

    CXXExInfo::Tys& CXXExInfo::Tys::operator=(Tys&& move) {
        ty_info = move.ty_info;
        copy_fn = move.copy_fn;
        is_bad_alloc = move.is_bad_alloc;
        is_refrence_only = move.is_refrence_only;

        move.ty_info = nullptr;
        move.copy_fn = nullptr;
        move.is_bad_alloc = false;
        move.is_refrence_only = false;
        return *this;
    }

    CXXExInfo::CXXExInfo(const CXXExInfo& copy) {
        *this = copy;
    }

    CXXExInfo::CXXExInfo(CXXExInfo&& move) {
        *this = std::move(move);
    }

    CXXExInfo& CXXExInfo::operator=(const CXXExInfo& copy) {
        ty_arr = copy.ty_arr;
        cleanup_fn = copy.cleanup_fn;
        ex_ptr = copy.ex_ptr;
        native_id = copy.native_id;
        ex_data_0 = copy.ex_data_0;
        ex_data_1 = copy.ex_data_1;
        ex_data_2 = copy.ex_data_2;
        ex_data_3 = copy.ex_data_3;
        return *this;
    }

    CXXExInfo& CXXExInfo::operator=(CXXExInfo&& move) {
        ty_arr = std::move(move.ty_arr);
        cleanup_fn = move.cleanup_fn;
        ex_ptr = move.ex_ptr;
        native_id = move.native_id;
        ex_data_0 = move.ex_data_0;
        ex_data_1 = move.ex_data_1;
        ex_data_2 = move.ex_data_2;
        ex_data_3 = move.ex_data_3;

        move.cleanup_fn = nullptr;
        move.ex_ptr = nullptr;
        move.native_id = 0;
        move.ex_data_0 = 0;
        move.ex_data_1 = 0;
        move.ex_data_2 = 0;
        move.ex_data_3 = 0;
        return *this;
    }
}