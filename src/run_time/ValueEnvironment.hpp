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
		std::unordered_map<std::string, ValueEnvironment*> environments;
	public:
		ValueItem value;
		ValueEnvironment*& joinEnvironment(const std::string& str) {
			return environments[str];
		}
		bool hasEnvironment(const std::string& str) {
			return environments.contains(str);
		}
		void removeEnvironment(const std::string& str) {
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