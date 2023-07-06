// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "../attacha_abi_structs.hpp"
namespace art{
	namespace file {
		namespace constructor {
			ValueItem* createProxy_FileHandle(ValueItem*, uint32_t);
			ValueItem* createProxy_BlockingFileHandle(ValueItem*, uint32_t);

			ValueItem* createProxy_TextFile(ValueItem*, uint32_t);

			ValueItem* createProxy_FolderBrowser(ValueItem*, uint32_t);
			ValueItem* createProxy_FolderChangesMonitor(ValueItem*, uint32_t);
		}
		ValueItem* remove(ValueItem*, uint32_t);
		ValueItem* rename(ValueItem*, uint32_t);
		ValueItem* copy(ValueItem*, uint32_t);
		void init();
	}
}