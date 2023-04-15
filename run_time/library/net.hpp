#pragma once
#include "../attacha_abi_structs.hpp"

namespace net {
	namespace server{
		namespace constructor {
			ValueItem* createProxy_TcpServer(ValueItem*, uint32_t);//[function], [port], [ip = localhost]
			ValueItem* createProxy_UpdServer(ValueItem*, uint32_t);//[function], [port], [ip = localhost]

			ValueItem* createProxy_HttpServer(ValueItem*, uint32_t);//[function], [http version], [ip = localhost], [port = 443],  [encryption = ssl], [certificate = 0]
		}
	}
	namespace client{
		namespace constructor {
			ValueItem* createProxy_TcpClient(ValueItem*, uint32_t);//[function], [port], [ip = localhost]
			ValueItem* createProxy_UpdClient(ValueItem*, uint32_t);//[function], [port], [ip = localhost]

			ValueItem* createProxy_HttpServer(ValueItem*, uint32_t);//[function], [http version], [ip = localhost], [port = 443],  [encryption = ssl], [certificate = 0]
		}
		ValueItem* tcpRequest(ValueItem*, uint32_t);//[ui8[] or uarr<ui8> request body], [port], [ip = localhost], format] return ui8[] or uarr<ui8>
		ValueItem* updRequest(ValueItem*, uint32_t);//[ui8[] or uarr<ui8> request body], [port], [ip = localhost], format] return ui8[] or uarr<ui8>
	}
	namespace api {
		namespace constructor {
			ValueItem* createProxy_ApiInstance(ValueItem*, uint32_t);//function uarr[any, ui8 response format](map[string,string] query, string path)
			ValueItem* createProxy_HttpRequestManager(ValueItem*, uint32_t);//[http version], [ip = localhost], [port = 443],  [encryption = ssl], [certificate = 0]
		}
		ValueItem* httpRequest(ValueItem*, uint32_t);//[adress + query, format] return any
	}
}