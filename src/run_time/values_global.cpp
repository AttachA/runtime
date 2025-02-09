// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/values_global.hpp>

namespace art {
    values_global::values_global(values_global* parent)
        : parent(parent) {}

    values_global::~values_global() {
        clear();
    }

    typed_lgr<values_global> values_global::join_namespace(const art::ustring& str) {
        auto it = namespaces.find(str);
        if (it == namespaces.end())
            return namespaces[str] = new values_global(this);
        return it->second;
    }

    typed_lgr<values_global> values_global::join_namespace(typed_lgr<values_global> current_namespace, std::initializer_list<art::ustring> strs) {
        for (auto& str : strs) {
            auto it = current_namespace->namespaces.find(str);
            if (it == current_namespace->namespaces.end()) {
                current_namespace = current_namespace->namespaces[str] = new values_global(*current_namespace);
            } else
                current_namespace = it->second;
        }
        return current_namespace;
    }

    typed_lgr<values_global> values_global::join_namespace_strict(typed_lgr<values_global> current_namespace, std::initializer_list<art::ustring> strs) {
        for (auto& str : strs) {
            auto it = current_namespace->namespaces.find(str);
            if (it == current_namespace->namespaces.end()) {
                throw UndefinedValue("This global variable is not defined: " + str);
            } else
                current_namespace = it->second;
        }
        return current_namespace;
    }

    bool values_global::has_namespace(const art::ustring& str) {
        return namespaces.contains(str);
    }

    bool values_global::has_namespace(typed_lgr<values_global> current_namespace, std::initializer_list<art::ustring> strs) {
        for (auto& str : strs) {
            auto it = current_namespace->namespaces.find(str);
            if (it == current_namespace->namespaces.end())
                return false;
            current_namespace = it->second;
        }
        return true;
    }

    void values_global::remove_namespace(const art::ustring& str) {
        auto it = namespaces.find(str);
        if (it != namespaces.end())
            namespaces.erase(it);
    }

    void values_global::remove_namespace(typed_lgr<values_global> current_namespace, std::initializer_list<art::ustring> strs) {
        typed_lgr<values_global> prev_namespace = current_namespace;
        decltype(namespaces)::iterator it = current_namespace->namespaces.end();
        for (auto& str : strs) {
            auto it = current_namespace->namespaces.find(str);
            if (it == current_namespace->namespaces.end())
                return;
            prev_namespace = current_namespace;
            current_namespace = it->second;
        }
        prev_namespace->namespaces.erase(it);
    }

    void values_global::clear() {
        namespaces.clear();
        value = nullptr;
    }

    ValueItem* values_global::find_value(const art::ustring& str) {
        typed_lgr<values_global> current_namespace(this, true);
        while (current_namespace) {
            auto it = current_namespace->namespaces.find(str);
            if (it == current_namespace->namespaces.end())
                current_namespace = current_namespace->parent;
            else
                return &it->second->value;
        }
        return nullptr;
    }

    ValueItem* values_global::find_value_local(const art::ustring& str) {
        auto it = namespaces.find(str);
        if (it == namespaces.end())
            return nullptr;
        return &it->second->value;
    }

    ValueItem* values_global::find_auto_join(typed_lgr<values_global> current_namespace, const art::ustring& str, const art::ustring& separator) {
        list_array<ustring> separated = str.split(separator);
        auto last = separated.take_back();
        for (auto& it : separated)
            current_namespace = current_namespace->join_namespace(it);
        return current_namespace->find_value(last);
    }

    ValueItem* values_global::find_value_local_auto_join(typed_lgr<values_global> current_namespace, const art::ustring& str, const art::ustring& separator) {
        list_array<ustring> separated = str.split(separator);
        auto last = separated.take_back();
        for (auto& it : separated)
            current_namespace = current_namespace->join_namespace(it);
        return current_namespace->find_value_local(last);
    }

    bool values_global::depth_safety() {
        for (auto& [name, env] : namespaces)
            if (!env.depth_safety())
                return false;
        return true;
    }
}