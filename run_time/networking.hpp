#ifndef RUN_TIME_NETWORKING
#define RUN_TIME_NETWORKING
#include "link_garbage_remover.hpp"
class TcpNetworkServer {
	struct TcpNetworkManager* handle;
public:
	enum class AcceptMode{
		task,
		thread
	};
	typed_lgr<class FuncEnviropment> on_connect;
	TcpNetworkServer(typed_lgr<class FuncEnviropment> on_connect, short port, size_t acceptors = 10, AcceptMode accept_mode = AcceptMode::task);
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
#endif /* RUN_TIME_NETWORKING */
