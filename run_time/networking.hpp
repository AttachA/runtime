#ifndef RUN_TIME_NETWORKING
#define RUN_TIME_NETWORKING
#include "link_garbage_remover.hpp"

//clients serve async, but reading and writing operations is blocking per client
// accept spawn new task/thread for each new connection
class TcpNetworkServer {
	struct TcpNetworkManager* handle;
public:
	enum class AcceptMode{
		task,
		thread
	};
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

	TcpNetworkServer(typed_lgr<class FuncEnviropment> on_connect, short port, ManageType manage_type, size_t acceptors = 10, AcceptMode accept_mode = AcceptMode::task);
	~TcpNetworkServer();
	//start handling
	void start();
	//stop getting new connections, and continue handling connected
	void pause();
	//resume getting new connections
	void resume();
	//stop and close all connections
	void stop();
	//check if the server is running
	bool is_running();
	//start handling, and use the current thread to handle connections, also can be used afer start to wait server to stop
	void mainline();
	//return status of the server, will be checked after constructor
	bool is_corrupted();
};


class UdpNetworkServer {
	struct UdpNetworkManager* handle;
public:
	enum class AcceptMode{
		task,
		thread
	};
	//arguments:
	//	ui8[]& data
	//	IP client_ip
	//	IP local_ip
	//	UdpNetworkServer& server
	//return:
	//	nothing
	UdpNetworkServer(typed_lgr<class FuncEnviropment> packet_handler,  short port, size_t acceptors = 10,AcceptMode accept_mode = AcceptMode::task);
	~UdpNetworkServer();
	//start handling
	void start();
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
};

#endif /* RUN_TIME_NETWORKING */
