#pragma once
#include "../attacha_abi_structs.hpp"

namespace net {
	namespace server{
		namespace constructor {
			ValueItem* createProxy_TcpServer(ValueItem*, uint32_t);//[port], [ip = localhost]
			ValueItem* createProxy_UpdServer(ValueItem*, uint32_t);//[port], [ip = localhost]

			ValueItem* createProxy_HttpServer(ValueItem*, uint32_t);//[http version], [ip = localhost], [port = 443],  [encryption = ssl], [certificate = 0]
		}
	}
	namespace api {
		namespace constructor {
			ValueItem* createProxy_ApiInstance(ValueItem*, uint32_t);//function uarr[any, ui8 response format](map[string,string] query, string path)
			ValueItem* createProxy_HttpRequestManager(ValueItem*, uint32_t);//[http version], [ip = localhost], [port = 443],  [encryption = ssl], [certificate = 0]
		}
		ValueItem* createHttpRequest(ValueItem*, uint32_t);//[adress + query, format] return any
	}
}