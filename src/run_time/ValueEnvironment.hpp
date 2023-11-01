// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <unordered_map>

#include <run_time/attacha_abi_structs.hpp>
#include <util/link_garbage_remover.hpp>

namespace art {
    class ValueEnvironment {
        std::unordered_map<art::ustring, typed_lgr<ValueEnvironment>, art::hash<art::ustring>> environments;
        typed_lgr<ValueEnvironment> parent;

    public:
        ValueItem value;

        ValueEnvironment(ValueEnvironment* parent = nullptr);
        ~ValueEnvironment();

        typed_lgr<ValueEnvironment> joinEnvironment(const art::ustring& str);
        typed_lgr<ValueEnvironment> joinEnvironment(const std::initializer_list<art::ustring>& strs);
        bool hasEnvironment(const art::ustring& str);
        bool hasEnvironment(const std::initializer_list<art::ustring>& strs);
        void removeEnvironment(const art::ustring& str);
        void removeEnvironment(const std::initializer_list<art::ustring>& strs);
        void clear();
        //finds value in current env and if not found, try find in parent
        ValueItem* findValue(const art::ustring& str);

        bool depth_safety();
    };
}