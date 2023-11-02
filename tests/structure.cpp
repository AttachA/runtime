#include <attacha/run_time.hpp>
#include <gtest/gtest.h>


using namespace art;

ValueItem* _test_set_string(ValueItem* args, uint32_t argc) {
    EXPECT_EQ(argc, 2);
    EXPECT_EQ(args[0].meta.vtype, VType::struct_);
    Structure* str = (Structure*)args[0].getSourcePtr();
    if (str->vtable_mode == Structure::VTableMode::AttachAVirtualTable)
        EXPECT_EQ(CXX::Interface::typeVTable<art::ustring>(), str->vtable);
    EXPECT_NE(str->self, nullptr);
    art::ustring& class_string = *(art::ustring*)str->self;
    EXPECT_NO_THROW({
        class_string = (art::ustring)args[1];
    });
    return nullptr;
}

ValueItem* _test_get_string(ValueItem* args, uint32_t argc) {
    EXPECT_EQ(argc, 1);
    EXPECT_EQ(args[0].meta.vtype, VType::struct_);
    Structure* str = (Structure*)args[0].getSourcePtr();
    if (str->vtable_mode == Structure::VTableMode::AttachAVirtualTable)
        EXPECT_EQ(CXX::Interface::typeVTable<art::ustring>(), str->vtable);
    EXPECT_NE(str->self, nullptr);
    art::ustring& class_string = *(art::ustring*)str->self;
    return new ValueItem(class_string);
}

TEST(Structure, AttachA_methods) {
    auto table = CXX::Interface::createTable<art::ustring>(".",
                                                           CXX::Interface::direct_method("get_string", _test_get_string),
                                                           CXX::Interface::direct_method("set_string", _test_set_string));
    CXX::Interface::typeVTable<art::ustring>() = table;
    EXPECT_NE(table, nullptr);
    EXPECT_TRUE(table->hasMethod("get_string", ClassAccess::pub));
    EXPECT_TRUE(table->hasMethod("set_string", ClassAccess::pub));
    EXPECT_TRUE(table->hasMethod("get_string", ClassAccess::prot));
    EXPECT_TRUE(table->hasMethod("set_string", ClassAccess::prot));
    EXPECT_TRUE(table->hasMethod("get_string", ClassAccess::priv));
    EXPECT_TRUE(table->hasMethod("set_string", ClassAccess::priv));
    EXPECT_TRUE(table->hasMethod("get_string", ClassAccess::intern));
    EXPECT_TRUE(table->hasMethod("set_string", ClassAccess::intern));

    Structure* str = CXX::Interface::constructStructure<art::ustring>(table);
    EXPECT_NE(str, nullptr);
    EXPECT_TRUE(str->has_method("get_string", ClassAccess::pub));
    EXPECT_TRUE(str->has_method("set_string", ClassAccess::pub));
    EXPECT_TRUE(str->has_method("get_string", ClassAccess::prot));
    EXPECT_TRUE(str->has_method("set_string", ClassAccess::prot));
    EXPECT_TRUE(str->has_method("get_string", ClassAccess::priv));
    EXPECT_TRUE(str->has_method("set_string", ClassAccess::priv));
    EXPECT_TRUE(str->has_method("get_string", ClassAccess::intern));
    EXPECT_TRUE(str->has_method("set_string", ClassAccess::intern));


    Structure* str2 = CXX::Interface::constructStructure<art::ustring>(table, "Hello world!");
    EXPECT_NE(str2, nullptr);
    EXPECT_TRUE(str2->has_method("get_string", ClassAccess::pub));
    EXPECT_TRUE(str2->has_method("set_string", ClassAccess::pub));
    EXPECT_TRUE(str2->has_method("get_string", ClassAccess::prot));
    EXPECT_TRUE(str2->has_method("set_string", ClassAccess::prot));
    EXPECT_TRUE(str2->has_method("get_string", ClassAccess::priv));
    EXPECT_TRUE(str2->has_method("set_string", ClassAccess::priv));
    EXPECT_TRUE(str2->has_method("get_string", ClassAccess::intern));
    EXPECT_TRUE(str2->has_method("set_string", ClassAccess::intern));

    EXPECT_EQ(Structure::compare(str, str2), -1);
    EXPECT_TRUE(CXX::Interface::makeCall(ClassAccess::pub, *str2, "get_string") == ValueItem("Hello world!"));
    EXPECT_TRUE(CXX::Interface::makeCall(ClassAccess::pub, *str, "get_string") == ValueItem(""));
    CXX::Interface::makeCall(ClassAccess::pub, *str, "set_string", "Hello world!");
    EXPECT_TRUE(CXX::Interface::makeCall(ClassAccess::pub, *str, "get_string") == ValueItem("Hello world!"));
    EXPECT_EQ(Structure::compare(str, str2), 0);
}

TEST(Structure, AttachA_methods_dynamic) {
    auto table = CXX::Interface::createDTable<art::ustring>(".",
                                                            CXX::Interface::direct_method("get_string", _test_get_string),
                                                            CXX::Interface::direct_method("set_string", _test_set_string));
    EXPECT_NE(table, nullptr);
    EXPECT_TRUE(table->hasMethod("get_string", ClassAccess::pub));
    EXPECT_TRUE(table->hasMethod("set_string", ClassAccess::pub));
    EXPECT_TRUE(table->hasMethod("get_string", ClassAccess::prot));
    EXPECT_TRUE(table->hasMethod("set_string", ClassAccess::prot));
    EXPECT_TRUE(table->hasMethod("get_string", ClassAccess::priv));
    EXPECT_TRUE(table->hasMethod("set_string", ClassAccess::priv));
    EXPECT_TRUE(table->hasMethod("get_string", ClassAccess::intern));
    EXPECT_TRUE(table->hasMethod("set_string", ClassAccess::intern));

    Structure* str = CXX::Interface::constructStructure<art::ustring>(table);
    EXPECT_NE(str, nullptr);
    EXPECT_TRUE(str->has_method("get_string", ClassAccess::pub));
    EXPECT_TRUE(str->has_method("set_string", ClassAccess::pub));
    EXPECT_TRUE(str->has_method("get_string", ClassAccess::prot));
    EXPECT_TRUE(str->has_method("set_string", ClassAccess::prot));
    EXPECT_TRUE(str->has_method("get_string", ClassAccess::priv));
    EXPECT_TRUE(str->has_method("set_string", ClassAccess::priv));
    EXPECT_TRUE(str->has_method("get_string", ClassAccess::intern));
    EXPECT_TRUE(str->has_method("set_string", ClassAccess::intern));

    table = CXX::Interface::createDTable<art::ustring>(".",
                                                       CXX::Interface::direct_method("get_string", _test_get_string),
                                                       CXX::Interface::direct_method("set_string", _test_set_string));
    Structure* str2 = CXX::Interface::constructStructure<art::ustring>(table, "Hello world!");
    EXPECT_NE(str2, nullptr);
    EXPECT_TRUE(str2->has_method("get_string", ClassAccess::pub));
    EXPECT_TRUE(str2->has_method("set_string", ClassAccess::pub));
    EXPECT_TRUE(str2->has_method("get_string", ClassAccess::prot));
    EXPECT_TRUE(str2->has_method("set_string", ClassAccess::prot));
    EXPECT_TRUE(str2->has_method("get_string", ClassAccess::priv));
    EXPECT_TRUE(str2->has_method("set_string", ClassAccess::priv));
    EXPECT_TRUE(str2->has_method("get_string", ClassAccess::intern));
    EXPECT_TRUE(str2->has_method("set_string", ClassAccess::intern));

    EXPECT_EQ(Structure::compare(str, str2), -1);
    EXPECT_TRUE(CXX::Interface::makeCall(ClassAccess::pub, *str2, "get_string") == ValueItem("Hello world!"));
    EXPECT_TRUE(CXX::Interface::makeCall(ClassAccess::pub, *str, "get_string") == ValueItem(""));
    CXX::Interface::makeCall(ClassAccess::pub, *str, "set_string", "Hello world!");
    EXPECT_TRUE(CXX::Interface::makeCall(ClassAccess::pub, *str, "get_string") == ValueItem("Hello world!"));
    EXPECT_EQ(Structure::compare(str, str2), 0);
}
