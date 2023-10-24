#ifndef SRC_RUN_TIME_ATTACHA_CXX_STRUCT
#define SRC_RUN_TIME_ATTACHA_CXX_STRUCT
#include <run_time/attacha_abi_structs.hpp>
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
            void*& typeVTable() {
                static void* hold = nullptr;
                return hold;
            }

            template <class Class_>
            Class_& getExtractAsDynamic(Structure& proxy) {
                return art::CXX::Interface::getExtractAs<Class_>(proxy, (AttachADynamicVirtualTable*)typeVTable<Class_>());
            }

            template <class Class_>
            Class_& getExtractAsDynamic(ValueItem& str) {
                return art::CXX::Interface::getExtractAs<Class_>(str, (AttachADynamicVirtualTable*)typeVTable<Class_>());
            }

            template <class Class_>
            Class_& getExtractAsStatic(Structure& proxy) {
                return art::CXX::Interface::getExtractAs<Class_>(proxy, (AttachAVirtualTable*)typeVTable<Class_>());
            }

            template <class Class_>
            Class_& getExtractAsStatic(ValueItem& str) {
                return art::CXX::Interface::getExtractAs<Class_>(str, (AttachAVirtualTable*)typeVTable<Class_>());
            }
        }
    }
}

#endif /* SRC_RUN_TIME_ATTACHA_CXX_STRUCT */
