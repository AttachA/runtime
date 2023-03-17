#if defined(_WIN32) || defined(_WIN64)
#define _WINSOCKAPI_
#define WIN32_LEAN_AND_MEAN
#include "tasks_util/windows_overlaped.hpp"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <stdio.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

#include "networking.hpp"
#include "FuncEnviropment.hpp"
#include "AttachA_CXX.hpp"
#include <condition_variable>




struct tcp_handle{
	OVERLAPPED overlapped;
	std::list<std::tuple<char*, size_t>> write_queue;
	typed_lgr<Task> notify_task;
	SOCKET socket;
	WSABUF buffer;
	char* data;
    int total_bytes;
    int sent_bytes;
	int readed_bytes;
	int data_len;

	enum class Opcode{
		ACCEPT,
		READ,
		WRITE
	} opcode = Opcode::ACCEPT;

	tcp_handle(SOCKET socket, size_t buffer_len) : socket(socket){
		SecureZeroMemory(&overlapped, sizeof(OVERLAPPED));
		data = new char[buffer_len];
		buffer.buf = data;
		buffer.len = buffer_len;
		data_len = buffer_len;
		total_bytes = 0;
		sent_bytes = 0;
		readed_bytes = 0;
	}
	~tcp_handle(){
		close();
	}
	bool data_available(){
		if(!data)
			return false;
		if(readed_bytes)
			return true;
		DWORD value = 0;
		int result = ::ioctlsocket(socket, FIONREAD, &value);
		if(result == SOCKET_ERROR)
			return false;
		else
			return value > 0;
	}
	void send_data(char* data, int len){
		if(!data)
			return;
		char* new_data = new char[len];
		memcpy(new_data, data, len);
		write_queue.push_back(std::make_tuple(new_data, len));
	}
	//async
	bool send_queue_item(){
		if(!data)
			return false;
		if(write_queue.empty())
			return false;
		auto item = write_queue.front();
		write_queue.pop_front();
		auto& send_data = std::get<0>(item);
		auto& val_len = std::get<1>(item);
		//set buffer
		buffer.len = data_len;
		buffer.buf = data;
		while(buffer.len < val_len) {
			memcpy(data, send_data, buffer.len);
			memcpy(send_data, send_data + buffer.len, val_len - buffer.len);
			if(!send_await())
				return false;
			val_len -= buffer.len;
		}
		if(val_len){
			memcpy(data, send_data, val_len);
			buffer.len = val_len;
			delete[] send_data;
			return send_await();
		}
		delete[] send_data;
		return true;
	}

	void read_data(){
		if(!data)
			return;
		buffer.len = data_len;
		buffer.buf = data;

		DWORD flags = 0;
		DWORD bytes_received = 0;
		typed_lgr<Task> task_await = notify_task = Task::dummy_task();
		opcode = Opcode::READ;
		int result = WSARecv(socket, &buffer, 1, &bytes_received, &flags, &overlapped, NULL);
		if ((SOCKET_ERROR == bytes_received) && (WSA_IO_PENDING != WSAGetLastError())){
			close();
			return;
		}
		Task::await_task(task_await, false);
		notify_task = nullptr;
	}
	void read_available(char* extern_buffer, int buffer_len, int& readed){
		if(!readed_bytes)
			read_data();
		if(readed_bytes < buffer_len){
			readed = readed_bytes;
			memcpy(extern_buffer, data, readed_bytes);
			readed_bytes = 0;
		}
		else{
			readed = buffer_len;
			memcpy(extern_buffer, buffer.buf, buffer_len);
			readed_bytes -= buffer_len;
			buffer.buf += buffer_len;
			buffer.len -= buffer_len;
		}
	}
	char* read_available_no_copy(int& readed){
		if(!readed_bytes)
			read_data();
		readed = readed_bytes;
		readed_bytes = 0;
		return data;
	}


	void close(){
		if(!data)
			return;
		if(!HasOverlappedIoCompleted(&overlapped)){
			CancelIoEx((HANDLE)socket, &overlapped);
			WaitForSingleObjectEx((HANDLE)socket, INFINITE, TRUE);
		}
		readed_bytes = 0;
		closesocket(socket);
		delete[] data;
		data = nullptr;
		if(notify_task)
			Task::start(notify_task);
		notify_task = nullptr;
	}

	void handle(DWORD dwBytesTransferred){
		DWORD flags = 0, bytes = 0;
		if(!data)
			return;
		switch (opcode) {
		case Opcode::READ:
			readed_bytes = dwBytesTransferred;
			Task::start(notify_task);
			break;
		case Opcode::WRITE:
			sent_bytes+=dwBytesTransferred;
			if(sent_bytes< total_bytes){
				buffer.buf = data + sent_bytes;
				buffer.len = total_bytes - sent_bytes;
				send();
			}
			else Task::start(notify_task);
		default:
			break;
		}
	}

	void send_and_close(char* data, int len){
		if(!data)
			return;
		buffer.len = data_len;
		buffer.buf = this->data;
		while(data_len < len) {
			memcpy(buffer.buf, data, buffer.len);
			if(!send_await())
				return;
			data += buffer.len;
			len -= buffer.len;
		}
		if(len){
			//send last part of data and close
			memcpy(buffer.buf, data, len);
			buffer.len = len;
			send_await();
		}
		close();
	}




	bool valid(){
		return data != nullptr;
	}


	void connection_reset(){
		data = nullptr;
		if(notify_task)
			Task::start(notify_task);
		notify_task = nullptr;
		if(!HasOverlappedIoCompleted(&overlapped)){
			CancelIoEx((HANDLE)socket, &overlapped);
			WaitForSingleObjectEx((HANDLE)socket, INFINITE, TRUE);
		}
		readed_bytes = 0;
		closesocket(socket);
	}
private:
	bool send(){
		DWORD bytes_sent = 0;
		DWORD flags = 0;
		opcode = Opcode::WRITE;
		int result = WSASend(socket, &buffer, 1, &bytes_sent, flags, &overlapped, NULL);
		if ((SOCKET_ERROR == bytes_sent) && (WSA_IO_PENDING != WSAGetLastError())){
			close();
			return false;
		}
		return true;
	}
	bool send_await(){
		typed_lgr<Task> task_await = notify_task = Task::dummy_task();
		if(!send())
			return false;
		Task::await_task(task_await);
		notify_task = nullptr;
		return data;//if data is null, then socket is closed
	}
};



ProxyClassDefine define_TcpNetworkStream;
void init_define_TcpNetworkStream();
class TcpNetworkStream{
	friend class TcpNetworkManager;
	friend class tcp_handle;
	struct tcp_handle* handle;
	typed_lgr<Task> self;
	TcpNetworkStream(tcp_handle* handle):handle(handle){}
public:
	~TcpNetworkStream(){
		if(handle)
			handle->close();
		handle = nullptr;
	}

	ValueItem read_available_ref(){
		if(!handle)
			return nullptr;
		while(!handle->data_available()){
			if(!handle->send_queue_item());
				break;
		}
		int readed = 0; 
		char* data = handle->read_available_no_copy(readed);
		return ValueItem(data, ValueMeta(VType::raw_arr_ui8, false, false, readed) , as_refrence);
	}
	ValueItem read_available(char* buffer, int buffer_len){
		if(!handle)
			return nullptr;
		while(!handle->data_available()){
			if(!handle->send_queue_item());
				break;
		}
		int readed = 0; 
		handle->read_available(buffer, buffer_len, readed);
		return ValueItem((uint32_t)readed);
	}
	bool data_available(){
		if(handle)
			return handle->data_available();
		return false;
	}
	//put data to queue
	void write(char* data, size_t size){
		if(handle)
			handle->send_data(data, size);
	}
	//write all data from write_queue
	void force_write(){
		if(handle)
			while(handle->send_queue_item());
	}
	void force_write_and_close(char* data, size_t size){
		if(handle)
			handle->send_and_close(data, size);
	}
	void close(){
		if(handle)
			handle->close();
		handle = nullptr;
	}
	bool is_closed(){
		if(handle){
			bool res = handle->valid();
			if(!res){
				delete handle;
				handle = nullptr;
			}
			return !res;
		}
		return true;
	}
};

ValueItem* funs_TcpNetworkStream_read_available_ref(ValueItem* args, uint32_t len){
	if(len == 1){
		if(args[0].meta.vtype != VType::proxy)
			throw InvalidArguments("The first argument must be a client_context.");
		return new ValueItem(((TcpNetworkStream*)((ProxyClass*)args[0].val)->class_ptr)->read_available_ref());
	}else
		throw InvalidArguments("The number of arguments must be 1.");
}
ValueItem* funs_TcpNetworkStream_read_available(ValueItem* args, uint32_t len){
	if(len == 2){
		if(args[0].meta.vtype != VType::proxy)
			throw InvalidArguments("The first argument must be a client_context.");
		if(args[1].meta.vtype != VType::raw_arr_ui8)
			throw InvalidArguments("The second argument must be a raw_arr_ui8.");
		return new ValueItem(((TcpNetworkStream*)((ProxyClass*)args[0].val)->class_ptr)->read_available((char*)args[1].val, args[1].meta.val_len));
	}else
		throw InvalidArguments("The number of arguments must be 2.");
}
ValueItem* funs_TcpNetworkStream_data_available(ValueItem* args, uint32_t len){
	if(len == 1){
		if(args[0].meta.vtype != VType::proxy)
			throw InvalidArguments("The first argument must be a client_context.");
		return new ValueItem(((TcpNetworkStream*)((ProxyClass*)args[0].val)->class_ptr)->data_available());
	}else
		throw InvalidArguments("The number of arguments must be 1.");
}
ValueItem* funs_TcpNetworkStream_write(ValueItem* args, uint32_t len){
	if(len == 2){
		if(args[0].meta.vtype != VType::proxy)
			throw InvalidArguments("The first argument must be a client_context.");
		if(args[1].meta.vtype != VType::raw_arr_ui8)
			throw InvalidArguments("The second argument must be a raw_arr_ui8.");
		((TcpNetworkStream*)((ProxyClass*)args[0].val)->class_ptr)->write((char*)args[1].val, args[1].meta.val_len);
		return nullptr;
	}else
		throw InvalidArguments("The number of arguments must be 2.");
}
ValueItem* funs_TcpNetworkStream_force_write(ValueItem* args, uint32_t len){
	if(len == 1){
		if(args[0].meta.vtype != VType::proxy)
			throw InvalidArguments("The first argument must be a client_context.");
		((TcpNetworkStream*)((ProxyClass*)args[0].val)->class_ptr)->force_write();
		return nullptr;
	}else
		throw InvalidArguments("The number of arguments must be 1.");
}
ValueItem* funs_TcpNetworkStream_force_write_and_close(ValueItem* args, uint32_t len){
	if(len == 2){
		if(args[0].meta.vtype != VType::proxy)
			throw InvalidArguments("The first argument must be a client_context.");
		if(args[1].meta.vtype != VType::raw_arr_ui8)
			throw InvalidArguments("The second argument must be a raw_arr_ui8.");
		((TcpNetworkStream*)((ProxyClass*)args[0].val)->class_ptr)->force_write_and_close((char*)args[1].val, args[1].meta.val_len);
		return nullptr;
	}else
		throw InvalidArguments("The number of arguments must be 2.");
}
ValueItem* funs_TcpNetworkStream_close(ValueItem* args, uint32_t len){
	if(len == 1){
		if(args[0].meta.vtype != VType::proxy)
			throw InvalidArguments("The first argument must be a client_context.");
		((TcpNetworkStream*)((ProxyClass*)args[0].val)->class_ptr)->close();
		return nullptr;
	}else
		throw InvalidArguments("The number of arguments must be 1.");
}
ValueItem* funs_TcpNetworkStream_is_closed(ValueItem* args, uint32_t len){
	if(len == 1){
		if(args[0].meta.vtype != VType::proxy)
			throw InvalidArguments("The first argument must be a client_context.");
		return new ValueItem(((TcpNetworkStream*)((ProxyClass*)args[0].val)->class_ptr)->is_closed());
	}else
		throw InvalidArguments("The number of arguments must be 1.");
}



class TcpNetworkManager : public OverlappedController {
	TaskMutex safety;
	typed_lgr<class FuncEnviropment> handler_fn;
	typed_lgr<class FuncEnviropment> accept_filter;
	WSADATA wsaData;
    sockaddr_in connectionAddress;
	SOCKET main_socket;
public:
	int32_t default_len = 4096;
private:
	bool allow_new_connections = false;
	bool disabled = true;
	bool corrupted = false;
	size_t acceptors;
	TcpNetworkServer::AcceptMode accept_mode;

	void make_acceptEx(void){
		static const auto adress_len = sizeof(sockaddr_in) + 16;
		auto new_sock = WSASocketA(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		DWORD argp = 1;//non blocking
		tcp_handle *pClientContext = new tcp_handle(new_sock, default_len);
		pClientContext->opcode = tcp_handle::Opcode::ACCEPT;
		int nBytesRecv = AcceptEx(main_socket, new_sock, pClientContext->buffer.buf, 0, adress_len, adress_len, NULL, &pClientContext->overlapped);
		if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError())){
			delete pClientContext;
			return;
		}
	}
	void accepted(tcp_handle* self){
		if(!allow_new_connections){
			delete self;
			return;
		}
		switch (accept_mode) {
		case TcpNetworkServer::AcceptMode::task:
			Task::start(new Task(handler_fn, ValueItem(new ProxyClass(new TcpNetworkStream(self), &define_TcpNetworkStream), VType::proxy, no_copy)));
			break;
		case TcpNetworkServer::AcceptMode::thread: {
			std::thread([this, self](){
				ValueItem* args = new ValueItem[1];
				args[0] = ValueItem(new ProxyClass(new TcpNetworkStream(self), &define_TcpNetworkStream), VType::proxy, no_copy);
				FuncEnviropment::sync_call(handler_fn, args, 1);
				delete[] args;
			}).detach();
			break;
		}
		default:
			break;
		}
	}

public:
	TcpNetworkManager(short port, size_t acceptors, TcpNetworkServer::AcceptMode accept) : acceptors(acceptors), accept_mode(accept) {
		init_define_TcpNetworkStream();
		
    	memset(&connectionAddress, 0, sizeof(sockaddr_in));
		connectionAddress.sin_family = AF_INET;
		connectionAddress.sin_port = htons(port);
		WSAStartup(MAKEWORD(2, 2), &wsaData);
		main_socket = WSASocketA(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (main_socket == INVALID_SOCKET){
			WSACleanup();
			corrupted = true;
			return;
		}
		DWORD argp = 1;//non blocking
		int result = setsockopt(main_socket,SOL_SOCKET,SO_REUSEADDR,(char*)&argp, sizeof(argp));
		if (result == SOCKET_ERROR){
			WSACleanup();
			corrupted = true;
			return;
		}
		if (ioctlsocket(main_socket, FIONBIO, &argp) == SOCKET_ERROR){
			WSACleanup();
			corrupted = true;
			return;
		}
		if (bind(main_socket, (sockaddr*)&connectionAddress, sizeof(sockaddr_in)) == SOCKET_ERROR){
			WSACleanup();
			corrupted = true;
			return;
		}
		register_handle((HANDLE)main_socket, this);
	}
	~TcpNetworkManager(){
		shutdown();
		WSACleanup();
	}
	
	void handle(void* _data, void* overlapped, DWORD dwBytesTransferred, bool status) {
		auto& data = *(tcp_handle*)overlapped;

		if(data.opcode == tcp_handle::Opcode::ACCEPT){
			make_acceptEx();
			
			SOCKADDR_IN* pClientAddr = NULL;
            SOCKADDR_IN* pLocalAddr = NULL;
            int remoteLen = sizeof(SOCKADDR_IN);
            int localLen = sizeof(SOCKADDR_IN);
            GetAcceptExSockaddrs(data.buffer.buf,
                                         data.buffer.len - ((sizeof(SOCKADDR_IN) + 16) * 2),
                                         sizeof(SOCKADDR_IN) + 16,
                                         sizeof(SOCKADDR_IN) + 16,
                                         (LPSOCKADDR*)&pLocalAddr,
                                         &localLen,
                                         (LPSOCKADDR*)&pClientAddr,
                                         &remoteLen);

            //clientIp = inet_ntoa(pClientAddr->sin_addr);
            //clientPort = ntohs(pClientAddr->sin_port);
			if(accept_filter){
				if(AttachA::cxxCall(accept_filter,(void*)pLocalAddr)){
					closesocket(data.socket);
					#ifndef DISABLE_RUNTIME_INFO
					ValueItem notify{ std::string("Client: ") + inet_ntoa(pClientAddr->sin_addr) + std::string(" not accepted due filter") };
					info.async_notify(notify);
					#endif
					return;
				}
			}
			
			#ifndef DISABLE_RUNTIME_INFO
			{
				ValueItem notify{ std::string("Client connected from: ") + inet_ntoa(pClientAddr->sin_addr) };
				info.async_notify(notify);
			}
			#endif

			setsockopt(data.socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&main_socket, sizeof(main_socket) );
			{
				std::lock_guard lock(safety);
				register_handle((HANDLE)data.socket, &data);
			}
			
			accepted(&data);
			return;
		}
		else if ((FALSE == status) || ((true == status) && (0 == dwBytesTransferred)))
		{
			#ifndef DISABLE_RUNTIME_INFO
			{
				ValueItem notify{ std::string("Client disconnected") };
				info.async_notify(notify);
			}
			#endif
			data.connection_reset();
			return;
		}
		data.handle(dwBytesTransferred);
	}
	void set_on_connect(typed_lgr<class FuncEnviropment> handler_fn){
		if(corrupted)
			throw std::runtime_error("TcpNetworkManager is corrupted");
		std::lock_guard lock(safety);
		this->handler_fn = handler_fn;
	}
	void mainline(){
		if(corrupted)
			throw std::runtime_error("TcpNetworkManager is corrupted");
		
		if(disabled){
			std::lock_guard lock(safety);
			if(disabled){
				if (listen(main_socket, SOMAXCONN) == SOCKET_ERROR){
					WSACleanup();
					return;
				}
				allow_new_connections = true; 
				disabled = false;
				
				for(size_t i = 0; i < acceptors; i++)
					make_acceptEx();
			}
		}
		if(Task::is_task()){
			TaskConditionVariable cv;
			TaskMutex mtx;
			MutexUnify unif(mtx);
			std::unique_lock ul(unif);
			
			int status = 0; /*0 -no result, 1-normal shutdown, 2 - exception */
			std::exception_ptr exception;
			std::thread([&status, &mtx, &cv,&exception, this]() {
				try{
					dispatch();
					std::unique_lock ul(mtx);
					status = 1;
					cv.notify_all();
				}catch(...){
					std::unique_lock ul(mtx);
					exception = std::current_exception();
					status = 2;
					cv.notify_all();
				}
			}).detach();
			while(!status)
				cv.wait(ul);
			if(status == 2)
				std::rethrow_exception(exception);
		}
		else
			dispatch();
	}
	void shutdown(){
		if(corrupted)
			throw std::runtime_error("TcpNetworkManager is corrupted");
		std::lock_guard lock(safety);
		if(disabled)
			return;
		stop();
		if(closesocket(main_socket) == SOCKET_ERROR)
			WSACleanup();
		allow_new_connections = false;
		disabled = true;
	}
	void pause(){
		if(corrupted)
			throw std::runtime_error("TcpNetworkManager is corrupted");
		allow_new_connections = false;
	}
	void resume(){
		if(corrupted)
			throw std::runtime_error("TcpNetworkManager is corrupted");
		allow_new_connections = true;
	}
	void start(){
		if(corrupted)
			throw std::runtime_error("TcpNetworkManager is corrupted");
		std::lock_guard lock(safety);
		allow_new_connections = true; 
		if(!disabled)
			return;
		if (listen(main_socket, SOMAXCONN) == SOCKET_ERROR){
			WSACleanup();
			return;
		}
		OverlappedController::run();
		for(size_t i = 0; i < acceptors; i++)
			make_acceptEx();
		disabled = false;
	}


	void set_accept_filter(typed_lgr<class FuncEnviropment> filter){
		if(corrupted)
			throw std::runtime_error("TcpNetworkManager is corrupted");
		std::lock_guard lock(safety);
		this->accept_filter = filter;
	}
	bool is_corrupted(){
		return corrupted;
	}
};




#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "networking.hpp"
#include "FuncEnviropment.hpp"
#include "AttachA_CXX.hpp"
#endif

TcpNetworkServer::TcpNetworkServer(typed_lgr<class FuncEnviropment> on_connect, short port, size_t acceptors, AcceptMode mode){
	handle = new TcpNetworkManager(port, acceptors, mode);
	handle->set_on_connect(on_connect);
}
TcpNetworkServer::~TcpNetworkServer(){
	if(handle)
		delete handle;
	handle = nullptr;
}
void TcpNetworkServer::start(){
	handle->start();
}
void TcpNetworkServer::pause(){
	handle->pause();
}
void TcpNetworkServer::resume(){
	handle->resume();
}
void TcpNetworkServer::stop(){
	handle->shutdown();
}
bool TcpNetworkServer::is_running(){
	return handle->in_run();
}
void TcpNetworkServer::mainline(){
	handle->mainline();
}
bool TcpNetworkServer::is_corrupted(){
	return handle->is_corrupted();
}




void init_define_TcpNetworkStream(){
	if(define_TcpNetworkStream.name.empty())
		return;
	define_TcpNetworkStream.name = "tcp_network_stream";
	define_TcpNetworkStream.copy = nullptr;
	define_TcpNetworkStream.destructor = AttachA::Interface::special::proxyDestruct<TcpNetworkStream, false>;
	define_TcpNetworkStream.funs["read_available_ref"] = ClassFnDefine{new FuncEnviropment(funs_TcpNetworkStream_read_available_ref, false), false, ClassAccess::pub};
	define_TcpNetworkStream.funs["read_available"] = ClassFnDefine{new FuncEnviropment(funs_TcpNetworkStream_read_available, false), false, ClassAccess::pub};
	define_TcpNetworkStream.funs["data_available"] = ClassFnDefine{new FuncEnviropment(funs_TcpNetworkStream_data_available, false), false, ClassAccess::pub};
	define_TcpNetworkStream.funs["write"] = ClassFnDefine{new FuncEnviropment(funs_TcpNetworkStream_write, false), false, ClassAccess::pub};
	define_TcpNetworkStream.funs["force_write"] = ClassFnDefine{new FuncEnviropment(funs_TcpNetworkStream_force_write, false), false, ClassAccess::pub};
	define_TcpNetworkStream.funs["force_write_and_close"] = ClassFnDefine{new FuncEnviropment(funs_TcpNetworkStream_force_write_and_close, false), false, ClassAccess::pub};
	define_TcpNetworkStream.funs["close"] = ClassFnDefine{new FuncEnviropment(funs_TcpNetworkStream_close, false), false, ClassAccess::pub};
	define_TcpNetworkStream.funs["is_closed"] = ClassFnDefine{new FuncEnviropment(funs_TcpNetworkStream_is_closed, false), false, ClassAccess::pub};
}