// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <unordered_map>
#include "attacha_abi_structs.hpp"
namespace art{
	class ValueEnvironment {
		std::unordered_map<std::string, ValueEnvironment*> enviropments;
	public:
		ValueItem value;
		ValueEnvironment*& joinEnviropment(const std::string& str) {
			return enviropments[str];
		}
		bool hasEnviropment(const std::string& str) {
			return enviropments.contains(str);
		}
		void removeEnviropment(const std::string& str) {
			delete enviropments[str];
			enviropments.erase(str);
		}
	};
	extern ValueEnvironment enviropments;
	extern thread_local ValueEnvironment thread_local_enviropments;
}