// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef SRC_RUN_TIME_ATTACHA_CXX_STRUCT
#define SRC_RUN_TIME_ATTACHA_CXX_STRUCT
#include <run_time/attacha_abi_structs.hpp>
#include <run_time/tasks.hpp>
#include <unordered_map>
#include <util/ustring.hpp>

namespace art {
    namespace CXX {
        namespace Interface {
            template <class Class_>
            Class_& getAs(Structure& str) {
                return *(Class_*)str.self;
            }

            template <class Class_>
            Class_& getAs(ValueItem& str) {
                if (str.meta.vtype == VType::struct_) {
                    return *(Class_*)((Structure&)str).self;
                } else
                    throw InvalidArguments("getAs: ValueItem is not a struct");
            }

            template <class Class_>
            const Class_& getAs(const Structure& str) {
                return *(const Class_*)str.self;
            }

            template <class Class_>
            const Class_& getAs(const ValueItem& str) {
                if (str.meta.vtype == VType::struct_) {
                    return *(const Class_*)((const Structure&)str).self;
                } else
                    throw InvalidArguments("getAs: ValueItem is not a struct");
            }

            template <class Class_>
            Class_& getExtractAs(Structure& proxy, AttachADynamicVirtualTable* vtable) {
                if (proxy.vtable != vtable) {
                    if (proxy.get_name() != vtable->name)
                        throw InvalidArguments(vtable->name + ", excepted " + vtable->name + ", got " + proxy.get_name());
                    else
                        throw InvalidArguments(vtable->name + ", excepted " + vtable->name + ", got non native" + vtable->name);
                }
                return art::CXX::Interface::getAs<Class_>(proxy);
            }

            template <class Class_>
            Class_& getExtractAs(ValueItem& str, AttachADynamicVirtualTable* vtable) {
                if (str.meta.vtype == VType::struct_) {
                    Structure& proxy = (Structure&)str;
                    if (proxy.vtable != vtable) {
                        if (proxy.get_name() != vtable->name)
                            throw InvalidArguments(vtable->name + ", excepted " + vtable->name + ", got " + proxy.get_name());
                        else
                            throw InvalidArguments(vtable->name + ", excepted " + vtable->name + ", got non native" + vtable->name);
                    }
                    return art::CXX::Interface::getAs<Class_>(proxy);
                } else
                    throw InvalidArguments(vtable->name + ", type mismatch, excepted struct_, got " + enum_to_string(str.meta.vtype));
            }

            template <class Class_>
            Class_& getExtractAs(Structure& proxy, AttachAVirtualTable* vtable) {
                if (proxy.vtable != vtable) {
                    if (proxy.get_name() != vtable->getName())
                        throw InvalidArguments(vtable->getName() + ", excepted " + vtable->getName() + ", got " + proxy.get_name());
                    else
                        throw InvalidArguments(vtable->getName() + ", excepted " + vtable->getName() + ", got non native" + vtable->getName());
                }
                return art::CXX::Interface::getAs<Class_>(proxy);
            }


            template <class Class_>
            Class_& getExtractAs(ValueItem& str, AttachAVirtualTable* vtable) {
                if (str.meta.vtype == VType::struct_) {
                    Structure& proxy = (Structure&)str;
                    if (proxy.vtable != vtable) {
                        if (proxy.get_name() != vtable->getName())
                            throw InvalidArguments(vtable->getName() + ", excepted " + vtable->getName() + ", got " + proxy.get_name());
                        else
                            throw InvalidArguments(vtable->getName() + ", excepted " + vtable->getName() + ", got non native" + vtable->getName());
                    }
                    return art::CXX::Interface::getAs<Class_>(proxy);
                } else
                    throw InvalidArguments(vtable->getName() + ", type mismatch, excepted struct_, got " + enum_to_string(str.meta.vtype));
            }

            template <class Class_>
            const Class_& getExtractAs(const Structure& proxy, AttachADynamicVirtualTable* vtable) {
                if (proxy.vtable != vtable) {
                    if (proxy.get_name() != vtable->name)
                        throw InvalidArguments(vtable->name + ", excepted " + vtable->name + ", got " + proxy.get_name());
                    else
                        throw InvalidArguments(vtable->name + ", excepted " + vtable->name + ", got non native" + vtable->name);
                }
                return art::CXX::Interface::getAs<Class_>(proxy);
            }

            template <class Class_>
            const Class_& getExtractAs(const ValueItem& str, AttachADynamicVirtualTable* vtable) {
                if (str.meta.vtype == VType::struct_) {
                    const Structure& proxy = (const Structure&)str;
                    if (proxy.vtable != vtable) {
                        if (proxy.get_name() != vtable->name)
                            throw InvalidArguments(vtable->name + ", excepted " + vtable->name + ", got " + proxy.get_name());
                        else
                            throw InvalidArguments(vtable->name + ", excepted " + vtable->name + ", got non native" + vtable->name);
                    }
                    return art::CXX::Interface::getAs<Class_>(proxy);
                } else
                    throw InvalidArguments(vtable->name + ", type mismatch, excepted struct_, got " + enum_to_string(str.meta.vtype));
            }

            template <class Class_>
            const Class_& getExtractAs(const Structure& proxy, AttachAVirtualTable* vtable) {
                if (proxy.vtable != vtable) {
                    if (proxy.get_name() != vtable->getName())
                        throw InvalidArguments(vtable->getName() + ", excepted " + vtable->getName() + ", got " + proxy.get_name());
                    else
                        throw InvalidArguments(vtable->getName() + ", excepted " + vtable->getName() + ", got non native" + vtable->getName());
                }
                return art::CXX::Interface::getAs<Class_>(proxy);
            }

            template <class Class_>
            const Class_& getExtractAs(const ValueItem& str, AttachAVirtualTable* vtable) {
                if (str.meta.vtype == VType::struct_) {
                    const Structure& proxy = (const Structure&)str;
                    if (proxy.vtable != vtable) {
                        if (proxy.get_name() != vtable->getName())
                            throw InvalidArguments(vtable->getName() + ", excepted " + vtable->getName() + ", got " + proxy.get_name());
                        else
                            throw InvalidArguments(vtable->getName() + ", excepted " + vtable->getName() + ", got non native" + vtable->getName());
                    }
                    return art::CXX::Interface::getAs<Class_>(proxy);
                } else
                    throw InvalidArguments(vtable->getName() + ", type mismatch, excepted struct_, got " + enum_to_string(str.meta.vtype));
            }

            struct VTableData {
                void* vtable;
                Structure::VTableMode mode;

                VTableData() {
                    vtable = nullptr;
                    mode = Structure::VTableMode::undefined;
                }

                operator void*() const {
                    return vtable;
                }

                operator Structure::VTableMode() const {
                    return mode;
                }

                VTableData& operator=(void* vtable) {
                    this->vtable = vtable;
                    mode = Structure::VTableMode::undefined;
                    return *this;
                }

                VTableData& operator=(nullptr_t) {
                    this->vtable = nullptr;
                    mode = Structure::VTableMode::undefined;
                    return *this;
                }

                VTableData& operator=(AttachAVirtualTable* vtable) {
                    this->vtable = vtable;
                    mode = Structure::VTableMode::AttachAVirtualTable;
                    return *this;
                }

                VTableData& operator=(AttachADynamicVirtualTable* vtable) {
                    this->vtable = vtable;
                    mode = Structure::VTableMode::AttachADynamicVirtualTable;
                    return *this;
                }

                bool operator==(nullptr_t) const {
                    return vtable == nullptr;
                }

                bool operator!=(nullptr_t) const {
                    return vtable != nullptr;
                }

                operator bool() const {
                    return vtable != nullptr;
                }

                bool operator==(AttachAVirtualTable* vtable) const {
                    return this->vtable == vtable && mode == Structure::VTableMode::AttachAVirtualTable;
                }

                bool operator!=(AttachAVirtualTable* vtable) const {
                    return this->vtable != vtable || mode != Structure::VTableMode::AttachAVirtualTable;
                }

                bool operator==(AttachADynamicVirtualTable* vtable) const {
                    return this->vtable == vtable && mode == Structure::VTableMode::AttachADynamicVirtualTable;
                }

                bool operator!=(AttachADynamicVirtualTable* vtable) const {
                    return this->vtable != vtable || mode != Structure::VTableMode::AttachADynamicVirtualTable;
                }

                bool operator==(void* vtable) const {
                    return this->vtable == vtable;
                }

                bool operator!=(void* vtable) const {
                    return this->vtable != vtable;
                }

                operator AttachAVirtualTable*() const {
                    if (mode == Structure::VTableMode::AttachAVirtualTable)
                        return (AttachAVirtualTable*)vtable;
                    else
                        return nullptr;
                }

                operator AttachADynamicVirtualTable*() const {
                    if (mode == Structure::VTableMode::AttachADynamicVirtualTable)
                        return (AttachADynamicVirtualTable*)vtable;
                    else
                        return nullptr;
                }

                void unregister() {
                    if (mode == Structure::VTableMode::AttachAVirtualTable)
                        AttachAVirtualTable::destroy((AttachAVirtualTable*)vtable);
                    else if (mode == Structure::VTableMode::AttachADynamicVirtualTable)
                        delete (AttachADynamicVirtualTable*)vtable;
                    *this = nullptr;
                }
            };

            struct __vtable_memory {

                static protected_value<std::unordered_map<size_t, VTableData>>& map() {
                    static protected_value<std::unordered_map<size_t, VTableData>> vv;
                    return vv;
                }
            };

            inline void clear_vtable_memory() {
                __vtable_memory::map().set([](std::unordered_map<size_t, VTableData>& it) { it.clear(); });
            }

            template <class Class_>
            VTableData& typeVTable() {
                VTableData* res;
                __vtable_memory::map().set([&res](auto& it) {
                    res = &it[typeid(Class_).hash_code()];
                });
                return *res;
            }

            template <class Class_>
            const VTableData& typeVTableReadOnly() {
                const VTableData* res = nullptr;
                VTableData empty;
                __vtable_memory::map().get([&res](const std::unordered_map<size_t, VTableData>& it) {
                    auto item = it.find(typeid(Class_).hash_code());
                    if (item != it.end())
                        res = &item->second;
                });
                return res ? *res : empty;
            }


            template <class Class_>
            Class_& getExtractAsDynamic(Structure& proxy) {
                return art::CXX::Interface::getExtractAs<Class_>(proxy, (AttachADynamicVirtualTable*)typeVTableReadOnly<Class_>());
            }

            template <class Class_>
            Class_& getExtractAsDynamic(ValueItem& str) {
                return art::CXX::Interface::getExtractAs<Class_>(str, (AttachADynamicVirtualTable*)typeVTableReadOnly<Class_>());
            }

            template <class Class_>
            Class_& getExtractAsStatic(Structure& proxy) {
                return art::CXX::Interface::getExtractAs<Class_>(proxy, (AttachAVirtualTable*)typeVTableReadOnly<Class_>());
            }

            template <class Class_>
            Class_& getExtractAsStatic(ValueItem& str) {
                return art::CXX::Interface::getExtractAs<Class_>(str, (AttachAVirtualTable*)typeVTableReadOnly<Class_>());
            }
        }
    }
}

#endif /* SRC_RUN_TIME_ATTACHA_CXX_STRUCT */
