// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef RUN_TIME_NETWORKING
#define RUN_TIME_NETWORKING
#include "link_garbage_remover.hpp"
#include "attacha_abi_structs.hpp"
//clients serve async, but reading and writing operations is blocking per client
// accept spawn new task/thread for each new connection
class TcpNetworkServer {
	struct TcpNetworkManager* handle;
public:
	enum class ManageType{
		//like regular blocking socket read/write(not block other clients)
		//proxy TcpNetworkBlocking{
		//	ui8[] read(ui32 size)//return empty array if connection is closed
		//	uint32 available()//return available bytes count
		//	i64 write(ui8[]& data)//return -1 if connection is closed, or sended bytes count
		//	void close()//close connection
		//	bool is_closed()//return true if connection is closed
		//}
		blocking,

		//automaticaly send all data from buffer if no read data available
		//proxy TcpNetworkStream{
		//	ui8[] read_available()//read all available data, request new data if buffer is empty, return empty array if connection is closed
		//	ui8[]& read_available_ref()//read all available data, request new data if buffer is empty, return empty array if connection is closed, return refrerence to internal buffer
		//	void write(ui8[]& data)//put data to buffer
		// 	void force_write()//write all data from buffer, and ignore read requests
		//	void force_write_and_close(ui8[]& data)//write all data from buffer, and close connection
		//	void close()//close connection
		//  bool is_closed()//return true if connection is closed
		//}
		write_delayed
	};
	//arguments:
	//	TcpNetworkBlocking/TcpNetworkStream& client
	//	IP client_ip
	//	IP local_ip
	//return:
	//	nothing
	typed_lgr<class FuncEnviropment> on_connect;

	TcpNetworkServer(typed_lgr<class FuncEnviropment> on_connect, ValueItem& ip_port, ManageType manage_type, size_t acceptors = 10);
	~TcpNetworkServer();
	void start(size_t pool_size = 0);
	void pause();
	void resume();
	void stop();
	void mainline();
	void set_pool_size(size_t pool_size);

	bool is_running();
	bool is_paused();
	bool is_corrupted();

	uint16_t server_port();
	std::string server_ip();
	ValueItem server_address();

};


class UdpNetworkServer {
	struct UdpNetworkManager* handle;
public:
	//arguments:
	//	ui8[]& data
	//	IP client_ip
	//	IP local_ip
	//	UdpNetworkServer& server
	//return:
	//	nothing
	UdpNetworkServer(typed_lgr<class FuncEnviropment> packet_handler, uint32_t buffer_len, ValueItem& ip_port, size_t acceptors = 10);
	~UdpNetworkServer();
	//start handling
	void start(size_t pool_size = 0);
	//stop handling
	void stop();
	//check if the server is running
	bool is_running();
	//start handling, and use the current thread to handle connections, also can be used afer start to wait server to stop
	void mainline();
	//return status of the server, will be checked after constructor
	bool is_corrupted();


	//send data to client
	//return true if data was sent
	bool send(ValueItem client_ip, uint8_t* data, uint32_t size);
	
	//change pool size, if server is running
	//if pool_size == 0, then pool size will be set to number of cores
	void set_pool_size(size_t pool_size);
	
	uint16_t server_port();
	std::string server_ip();
	ValueItem server_address();

	bool is_disabled();
	bool is_paused();
};


uint8_t init_networking();
void deinit_networking();
bool ipv6_supported();

namespace client {
	
}

ValueItem makeIP4(const char* ip, uint16_t port = 0);
ValueItem makeIP6(const char* ip, uint16_t port = 0);
ValueItem makeIP(const char* ip, uint16_t port = 0);
ValueItem makeIP4_port(const char* ip_port);//ip4:port
ValueItem makeIP6_port(const char* ip_port);//ip6:port
ValueItem makeIP_port(const char* ip_port);//ip:port

#endif /* RUN_TIME_NETWORKING */
