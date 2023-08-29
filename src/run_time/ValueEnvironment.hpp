// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <unordered_map>

#include <run_time/attacha_abi_structs.hpp>

namespace art {
    class ValueEnvironment {
        std::unordered_map<art::ustring, ValueEnvironment*, art::hash<art::ustring>> environments;

    public:
        ValueItem value;

        ValueEnvironment*& joinEnvironment(const art::ustring& str) {
            return environments[str];
        }

        bool hasEnvironment(const art::ustring& str) {
            return environments.contains(str);
        }

        void removeEnvironment(const art::ustring& str) {
            delete environments[str];
            environments.erase(str);
        }

        void clear() {
            for (auto& [key, value] : environments)
                delete value;
            environments.clear();
            value = nullptr;
        }
    };

    extern ValueEnvironment environments;
}