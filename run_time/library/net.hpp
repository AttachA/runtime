#pragma once
#include "../attacha_abi_structs.hpp"

namespace net {
	namespace constructor {
		ValueItem* createProxy_IP4(ValueItem*, uint32_t);//[string ipv4], [port=0]
		ValueItem* createProxy_IP6(ValueItem*, uint32_t);//[string ipv6], [port=0]
		ValueItem* createProxy_IP(ValueItem*, uint32_t);//[string ip], [port=0]
		ValueItem* createProxy_Address(ValueItem*, uint32_t);//[string ip:port]


		ValueItem* createProxy_TcpServer(ValueItem*, uint32_t);//[function], [ip:port], [manage_type = write_delayed(0)], [acceptors = 10], [accept_mode = task(0)]
		ValueItem* createProxy_UdpServer(ValueItem*, uint32_t);//[function], [port], [ip = localhost]
		ValueItem* createProxy_HttpServer(ValueItem*, uint32_t);//[function], [http version], [ip = localhost], [port = 443],  [encryption = ssl], [certificate = 0]

		ValueItem* createProxy_TcpClient(ValueItem*, uint32_t);//[function], [port], [ip = localhost]
		ValueItem* createProxy_UdpClient(ValueItem*, uint32_t);//[function], [port], [ip = localhost]
		ValueItem* createProxy_HttpClient(ValueItem*, uint32_t);//[function], [http version], [ip = localhost], [port = 443],  [encryption = ssl], [certificate = 0]
	}
	namespace api {
		namespace constructor {
			ValueItem* createProxy_ApiInstance(ValueItem*, uint32_t);//function uarr[any, ui8 response format](map[string,string] query, string path)
			ValueItem* createProxy_HttpRequestManager(ValueItem*, uint32_t);//[http version], [ip = localhost], [port = 443],  [encryption = ssl], [certificate = 0]
		}
		ValueItem* httpRequest(ValueItem*, uint32_t);//[adress + query, format] return any
	}

	void init();
}