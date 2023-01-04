#pragma once
#include "../attacha_abi_structs.hpp"

namespace file {
	namespace constructor {
		ValueItem* createProxy_RawFile(ValueItem*, uint32_t);
		ValueItem* createProxy_TextFile(ValueItem*, uint32_t);
		ValueItem* createProxy_EnbtFile(ValueItem*, uint32_t);
		ValueItem* createProxy_JsonFile(ValueItem*, uint32_t);
		ValueItem* createProxy_YamlFile(ValueItem*, uint32_t);
	}
	namespace log {
		namespace constructor {
			ValueItem* createProxy_TextLog(ValueItem*, uint32_t);
			ValueItem* createProxy_EnbtLog(ValueItem*, uint32_t);
			ValueItem* createProxy_JsonLog(ValueItem*, uint32_t);
		}
	}

	ValueItem* readBytes(ValueItem*, uint32_t); //ui8[] / uarr<ui8>
	ValueItem* readLines(ValueItem*, uint32_t); //uarr<string>
	ValueItem* readEnbts(ValueItem*, uint32_t); //uarr<any>
	ValueItem* readJson(ValueItem*, uint32_t); //any
	ValueItem* readYaml(ValueItem*, uint32_t); //any

	ValueItem* readEnbtToken(ValueItem*, uint32_t); //any, args: (string path / RawFile), string enbt token adress
	ValueItem* readEnbtToken(ValueItem*, uint32_t); //any

	void init();
}