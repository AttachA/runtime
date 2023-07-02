// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "../attacha_abi_structs.hpp"

namespace net {
	namespace constructor {
		ValueItem* createProxy_IP4(ValueItem*, uint32_t);//[string ipv4], [port=0]
		ValueItem* createProxy_IP6(ValueItem*, uint32_t);//[string ipv6], [port=0]
		ValueItem* createProxy_IP(ValueItem*, uint32_t);//[string ip], [port=0]
		ValueItem* createProxy_Address(ValueItem*, uint32_t);//[string ip:port]

		ValueItem* createProxy_TcpServer(ValueItem*, uint32_t);//[function], [ip:port], [manage_type = write_delayed(0)], [acceptors = 10], [accept_mode = task(0)]
		ValueItem* createProxy_HttpServer(ValueItem*, uint32_t);//[function], [http version], [ip = localhost], [port = 443],  [encryption = ssl], [certificate = 0]

		ValueItem* createProxy_UdpSocket(ValueItem*, uint32_t);//[ip:port], [timeout_ms = 0]
	}
	ValueItem* ipv6_supported(ValueItem*, uint32_t);

	ValueItem* tcp_client_connect(ValueItem*, uint32_t);//[ip:port], [timeout_ms = 0] or [ip:port], [data], [timeout_ms = 0]
	void init();
}