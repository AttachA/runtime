// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "AttachA_CXX.hpp"
bool inited = false;
ProxyClassDefine define_UniversalAddress;
ProxyClassDefine define_TcpNetworkStream;
ProxyClassDefine define_TcpNetworkBlocking;
ProxyClassDefine define_UdpNetworkManager;

void init_define_TcpNetworkStream();
void init_define_TcpNetworkBlocking();
void init_define_UdpNetworkManager();
void init_define_UniversalAddress();

#if defined(_WIN32) || defined(_WIN64)
#define _WINSOCKAPI_
#define WIN32_LEAN_AND_MEAN
#include "tasks_util/windows_overlaped.hpp"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <stdio.h>
#pragma comment(lib, "Ws2_32.lib")

#include "networking.hpp"
#include "FuncEnviropment.hpp"
#include "agreement/symbols.hpp"
#include <condition_variable>

LPFN_ACCEPTEX _AcceptEx;
LPFN_GETACCEPTEXSOCKADDRS _GetAcceptExSockaddrs;
WSADATA wsaData;

void init_win_fns(SOCKET sock){
	static bool inited = false;
	if (inited)
		return;
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	GUID GuidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
	DWORD dwBytes = 0;

	if (SOCKET_ERROR == WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidAcceptEx, sizeof(GuidAcceptEx), &_AcceptEx, sizeof(_AcceptEx), &dwBytes, NULL, NULL))
		throw std::runtime_error("WSAIoctl failed get AcceptEx");
	if (SOCKET_ERROR == WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidGetAcceptExSockaddrs, sizeof(GuidGetAcceptExSockaddrs), &_GetAcceptExSockaddrs, sizeof(_GetAcceptExSockaddrs), &dwBytes, NULL, NULL))
		throw std::runtime_error("WSAIoctl failed get GetAcceptExSockaddrs");
	inited = true;
}



using universal_address = sockaddr_storage;
namespace UniversalAddress{
	enum class AddressType : uint32_t{
		IPv4,
		IPv6,
		UNDEFINED = (uint32_t)-1
	};
	ValueItem* _define_to_string(ValueItem* args, uint32_t len){
		if(len < 1)
			throw InvalidArguments("universal_address.to_string, expected 1 argument, got " + std::to_string(len));
		if(args[0].meta.vtype != VType::proxy)
			throw InvalidArguments("universal_address.to_string, excepted proxy, got " + enum_to_string(args[0].meta.vtype));
		ProxyClass& proxy = (ProxyClass&)args[0];
		if(proxy.declare_ty != &define_UniversalAddress){
			if(proxy.declare_ty->name != "universal_address")
				throw InvalidArguments("universal_address.to_string, excepted universal_address, got " + proxy.declare_ty->name);
			else
				throw InvalidArguments("universal_address.to_string, excepted universal_address, got non native universal_address");
		}
		auto* address = (universal_address*)proxy.class_ptr;
		std::string result;
		result.resize(INET6_ADDRSTRLEN);
		if(address->ss_family == AF_INET)
			inet_ntop(AF_INET, &((sockaddr_in*)address)->sin_addr, result.data(), INET6_ADDRSTRLEN);
		else if(address->ss_family == AF_INET6){
			inet_ntop(AF_INET6, &((sockaddr_in6*)address)->sin6_addr, result.data(), INET6_ADDRSTRLEN);
			if(result.starts_with("::ffff:"))
				result = result.substr(7);
		}
		else
			throw std::runtime_error("universal_address.to_string, unknown address family");
		result.resize(strlen(result.data()));
		
		return new ValueItem(result);
	}
	ValueItem* _define_type(ValueItem* args,uint32_t len){
		if(len < 1)
			throw InvalidArguments("universal_address.type, expected 1 argument, got " + std::to_string(len));
		if(args[0].meta.vtype != VType::proxy)
			throw InvalidArguments("universal_address.type, excepted proxy, got " + enum_to_string(args[0].meta.vtype));
		ProxyClass& proxy = (ProxyClass&)args[0];
		if(proxy.declare_ty != &define_UniversalAddress){
			if(proxy.declare_ty->name != "universal_address")
				throw InvalidArguments("universal_address.type, excepted universal_address, got " + proxy.declare_ty->name);
			else
				throw InvalidArguments("universal_address.type, excepted universal_address, got non native universal_address");
		}
		auto* address = (universal_address*)proxy.class_ptr;
		std::string result;
		result.resize(INET6_ADDRSTRLEN);
		if(address->ss_family == AF_INET)
			return new ValueItem((uint32_t)AddressType::IPv4);
		else if(address->ss_family == AF_INET6){
			inet_ntop(AF_INET6, &((sockaddr_in6*)address)->sin6_addr, result.data(), INET6_ADDRSTRLEN);
			if(result.starts_with("::ffff:"))
				return new ValueItem((uint32_t)AddressType::IPv4);
			else
				return new ValueItem((uint32_t)AddressType::IPv6);
		}
		else
			throw std::runtime_error("universal_address.type, unknown address family");
	}
	ValueItem* _define_actual_type(ValueItem* args,uint32_t len){
		if(len < 1)
			throw InvalidArguments("universal_address.actual_type, expected 1 argument, got " + std::to_string(len));
		if(args[0].meta.vtype != VType::proxy)
			throw InvalidArguments("universal_address.actual_type, excepted proxy, got " + enum_to_string(args[0].meta.vtype));
		ProxyClass& proxy = (ProxyClass&)args[0];
		if(proxy.declare_ty != &define_UniversalAddress){
			if(proxy.declare_ty->name != "universal_address")
				throw InvalidArguments("universal_address.actual_type, excepted universal_address, got " + proxy.declare_ty->name);
			else
				throw InvalidArguments("universal_address.actual_type, excepted universal_address, got non native universal_address");
		}
		auto* address = (universal_address*)proxy.class_ptr;
		if(address->ss_family == AF_INET)
			return new ValueItem((uint32_t)AddressType::IPv4);
		else if(address->ss_family == AF_INET6){
			return new ValueItem((uint32_t)AddressType::IPv6);
		}
		else
			throw std::runtime_error("universal_address.actual_type, unknown address family");
	}
	ValueItem* _define_port(ValueItem* args,uint32_t len){
		if(len < 1)
			throw InvalidArguments("universal_address.port, expected 1 argument, got " + std::to_string(len));
		if(args[0].meta.vtype != VType::proxy)
			throw InvalidArguments("universal_address.port, excepted proxy, got " + enum_to_string(args[0].meta.vtype));
		ProxyClass& proxy = (ProxyClass&)args[0];
		if(proxy.declare_ty != &define_UniversalAddress){
			if(proxy.declare_ty->name != "universal_address")
				throw InvalidArguments("universal_address.port, excepted universal_address, got " + proxy.declare_ty->name);
			else
				throw InvalidArguments("universal_address.port, excepted universal_address, got non native universal_address");
		}
		auto* address = (universal_address*)proxy.class_ptr;
		if(address->ss_family == AF_INET)
			return new ValueItem((uint32_t)((sockaddr_in*)address)->sin_port);
		else if(address->ss_family == AF_INET6){
			return new ValueItem((uint32_t)((sockaddr_in6*)address)->sin6_port);
		}
		else
			throw std::runtime_error("universal_address.port, unknown address family");
	}
	ValueItem* _define_full_address(ValueItem* args, uint32_t len){
		if(len < 1)
			throw InvalidArguments("universal_address.to_string, expected 1 argument, got " + std::to_string(len));
		if(args[0].meta.vtype != VType::proxy)
			throw InvalidArguments("universal_address.to_string, excepted proxy, got " + enum_to_string(args[0].meta.vtype));
		ProxyClass& proxy = (ProxyClass&)args[0];
		if(proxy.declare_ty != &define_UniversalAddress){
			if(proxy.declare_ty->name != "universal_address")
				throw InvalidArguments("universal_address.to_string, excepted universal_address, got " + proxy.declare_ty->name);
			else
				throw InvalidArguments("universal_address.to_string, excepted universal_address, got non native universal_address");
		}
		auto* address = (universal_address*)proxy.class_ptr;
		std::string result;
		result.resize(INET6_ADDRSTRLEN);

		auto actual_family = address->ss_family;
		if(address->ss_family == AF_INET)
			inet_ntop(AF_INET, &((sockaddr_in*)address)->sin_addr, result.data(), INET6_ADDRSTRLEN);
		else if(address->ss_family == AF_INET6){
			inet_ntop(AF_INET6, &((sockaddr_in6*)address)->sin6_addr, result.data(), INET6_ADDRSTRLEN);
			if(result.starts_with("::ffff:")){
				result = result.substr(7);
				actual_family = AF_INET;
			}
		}
		else
			throw std::runtime_error("universal_address.to_string, unknown address family");
		result.resize(strlen(result.data()));
		if(actual_family == AF_INET){
			result += ":" + std::to_string((uint32_t)htons(((sockaddr_in*)address)->sin_port));
		}else{
			result = '[' + result +  "]:" + std::to_string((uint32_t)htons(((sockaddr_in6*)address)->sin6_port));
		}
		return new ValueItem(result);
	}
}
void init_define_UniversalAddress(){
	if(!define_UniversalAddress.name.empty())
		return;
	define_UniversalAddress.name = "universal_address";
	define_UniversalAddress.copy = AttachA::Interface::special::proxyCopy<universal_address, false>;
	define_UniversalAddress.destructor = AttachA::Interface::special::proxyDestruct<universal_address, false>;
	define_UniversalAddress.funs[symbols::structures::convert::to_string] = 
		ClassFnDefine(
			new FuncEnviropment(UniversalAddress::_define_to_string, false), false, ClassAccess::pub
		);
	define_UniversalAddress.funs["type"] = 
		ClassFnDefine(
			new FuncEnviropment(UniversalAddress::_define_type, false), false, ClassAccess::pub
		);
	define_UniversalAddress.funs["actual_type"] = 
		ClassFnDefine(
			new FuncEnviropment(UniversalAddress::_define_actual_type, false), false, ClassAccess::pub
		);
	define_UniversalAddress.funs["port"] = 
		ClassFnDefine(
			new FuncEnviropment(UniversalAddress::_define_port, false), false, ClassAccess::pub
		);
	define_UniversalAddress.funs["full_address"] =
		ClassFnDefine(
			new FuncEnviropment(UniversalAddress::_define_full_address, false), false, ClassAccess::pub
		);
}

void internal_makeIP4(universal_address& addr_storage, const char* ip, uint16_t port){
	sockaddr_in6 addr6;
	memset(&addr6, 0, sizeof(addr6));
	addr6.sin6_family = AF_INET6;
	addr6.sin6_port = htons(port);
	addr6.sin6_addr.s6_addr[10] = 0xFF;
	addr6.sin6_addr.s6_addr[11] = 0xFF;
	if(inet_pton(AF_INET, ip, &addr6.sin6_addr.s6_addr[12]) != 1)
		throw InvalidArguments("Invalid ip4 address");

	memcpy(&addr_storage, &addr6, sizeof(addr6));
}
void internal_makeIP6(universal_address& addr_storage, const char* ip, uint16_t port){
	sockaddr_in6 addr6;
	memset(&addr6, 0, sizeof(addr6));
	addr6.sin6_family = AF_INET6;
	addr6.sin6_port = htons(port);
	if(inet_pton(AF_INET6, ip, &addr6.sin6_addr) != 1)
		throw InvalidArguments("Invalid ip6 address");

	memcpy(&addr_storage, &addr6, sizeof(addr6));
}
void internal_makeIP(universal_address& addr_storage, const char* ip, uint16_t port){
	sockaddr_in6 addr6;
	memset(&addr6, 0, sizeof(addr6));
	addr6.sin6_family = AF_INET6;
	addr6.sin6_port = htons(port);
	addr6.sin6_addr.s6_addr[10] = 0xFF;
	addr6.sin6_addr.s6_addr[11] = 0xFF;
	if(inet_pton(AF_INET, ip, &addr6.sin6_addr+12) == 1);
	else if(inet_pton(AF_INET6, ip, &addr6.sin6_addr) != 1)
		throw InvalidArguments("Invalid ip4 address");
	memcpy(&addr_storage, &addr6, sizeof(addr6));
}

void internal_makeIP4_port(universal_address& addr_storage, const char* ip_port){
	const char* port = strchr(ip_port, ':');
	if(!port)
		throw InvalidArguments("Invalid ip4 address");
	uint16_t port_num = (uint16_t)std::stoi(port+1);
	std::string ip(ip_port, port);
	internal_makeIP4(addr_storage, ip.c_str(), port_num);
}
void internal_makeIP6_port(universal_address& addr_storage, const char* ip_port){
	if(ip_port[0] != '[')
		throw InvalidArguments("Invalid ip6:port address");
	const char* port = strchr(ip_port, ']');
	if(!port)
		throw InvalidArguments("Invalid ip6:port address");
	if(port[1] != ':')
		throw InvalidArguments("Invalid ip6:port address");
	if(port[2] == 0 )
		throw InvalidArguments("Invalid ip6:port address");
	uint16_t port_num = (uint16_t)std::stoi(port+2);


	if(ip_port == port - 1){
		sockaddr_in6 addr6;
		memset(&addr6, 0, sizeof(addr6));
		addr6.sin6_family = AF_INET6;
		addr6.sin6_port = htons(port_num);
		memcpy(&addr_storage, &addr6, sizeof(addr6));
		return;
	}
	std::string ip(ip_port+1, port);
	internal_makeIP6(addr_storage, ip.c_str(), port_num);
}
void internal_makeIP_port(universal_address& addr_storage, const char* ip_port){
	if(ip_port[0] == '[')
		return internal_makeIP6_port(addr_storage, ip_port);
	else
		return internal_makeIP4_port(addr_storage, ip_port);
}



#pragma region TCP
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
	enum class error : uint8_t{
		none = 0,
		remote_close = 1,
		local_close = 2,
		undefined_error = 0xFF
	} invalid_reason = error::none;
	enum class Opcode : uint8_t{
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
	uint32_t available_bytes(){
		if(!data)
			return 0;
		if(readed_bytes)
			return true;
		DWORD value = 0;
		int result = ::ioctlsocket(socket, FIONREAD, &value);
		if(result == SOCKET_ERROR)
			return 0;
		else
			return value;
	}
	bool data_available(){
		return available_bytes() > 0;
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
		std::unique_ptr<char[]> send_data_ptr(send_data);
		//set buffer
		buffer.len = data_len;
		buffer.buf = data;
		while(val_len) {
			size_t to_sent_bytes = val_len > data_len ? data_len : val_len;
			memcpy(data, send_data, to_sent_bytes);
			buffer.len = to_sent_bytes;
			buffer.buf = data;
			if(!send_await()){
				return false;
			}
			if(val_len < sent_bytes)
				return true;
			val_len -= sent_bytes;
			send_data += sent_bytes;
		}
		return true;
	}


	void read_force(uint32_t buffer_len, char* buffer){
		if(!data)
			return;
		if(!buffer_len)
			return;
		if(!buffer)
			return;
		while(buffer_len){
			int readed = 0;
			read_available(buffer, buffer_len, readed);
			buffer += readed;
			buffer_len -= readed;
		}
	}
	int64_t write_force(char* to_write, uint32_t to_write_len){
		if(!data)
			return -1;
		if(!to_write_len)
			return -1;
		if(!to_write)
			return -1;


		if(data_len < to_write_len){
			buffer.len = data_len;
			buffer.buf = this->data;
			if(!send_await())
				return -1;
			return sent_bytes;
		}
		else{
			buffer.len = to_write_len;
			buffer.buf = this->data;
			memcpy(this->data, to_write, to_write_len);
			if(!send_await())
				return -1;
			return sent_bytes;
		}
	}


	void read_data(){
		if(!data)
			return;
		buffer.len = data_len;
		buffer.buf = data;

		DWORD flags = 0;
		typed_lgr<Task> task_await = notify_task = Task::dummy_task();
		opcode = Opcode::READ;
		int result = WSARecv(socket, &buffer, 1, NULL, &flags, &overlapped, NULL);
		if ((SOCKET_ERROR == result)){
			auto error = WSAGetLastError();
			if(WSA_IO_PENDING != error){
				switch (error){
				case WSAECONNRESET:
					invalid_reason = error::remote_close;
					break;
				case WSAECONNABORTED:
				case WSA_OPERATION_ABORTED:
					invalid_reason = error::local_close;
					break;
				default:
					invalid_reason = error::undefined_error;
					break;
				}
				close();
				return;
			}
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
		std::list<std::tuple<char*, size_t>> clear_write_queue;
		write_queue.swap(clear_write_queue);
		for(auto& item : clear_write_queue)
			delete[] std::get<0>(item);
		readed_bytes = 0;
		sent_bytes = 0;
		closesocket(socket);
		delete[] data;
		data = nullptr;
		invalid_reason = error::local_close;
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
			if(sent_bytes < total_bytes){
				buffer.buf = data + sent_bytes;
				buffer.len = total_bytes - sent_bytes;
				if(!data_available())
					send();
				else{
					char* data = new char[buffer.len];
					memcpy(data, buffer.buf, buffer.len);
					write_queue.push_front(std::make_tuple(data, buffer.len));
					Task::start(notify_task);
				}
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
		invalid_reason = error::remote_close;
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
		DWORD flags = 0;
		opcode = Opcode::WRITE;
		int result = WSASend(socket, &buffer, 1, NULL, flags, &overlapped, NULL);
		if ((SOCKET_ERROR == result)){
			auto error = WSAGetLastError();
			if(WSA_IO_PENDING != error){
				switch (error){
				case WSAECONNRESET:
					invalid_reason = error::remote_close;
					break;
				case WSAECONNABORTED:
				case WSA_OPERATION_ABORTED:
					invalid_reason = error::local_close;
					break;
				default:
					invalid_reason = error::undefined_error;
					break;
				}
				close();
				return false;
			}
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




#pragma region TcpNetworkStream
class TcpNetworkStream{
	friend class TcpNetworkManager;
	friend struct tcp_handle;
	struct tcp_handle* handle;
	TcpNetworkStream(tcp_handle* handle):handle(handle){}
	TaskMutex mutex;
public:
	~TcpNetworkStream(){
		if(handle){
			std::lock_guard lg(mutex);
			handle->close();
		}
		handle = nullptr;
	}

	ValueItem read_available_ref(){
		std::lock_guard lg(mutex);
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
		std::lock_guard lg(mutex);
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
		std::lock_guard lg(mutex);
		if(handle)
			return handle->data_available();
		return false;
	}
	//put data to queue
	void write(char* data, size_t size){
		std::lock_guard lg(mutex);
		if(handle){
			handle->send_data(data, size);
			while(!handle->data_available()){
				if(!handle->send_queue_item());
					break;
			}
		}
	}
	//write all data from write_queue
	void force_write(){
		std::lock_guard lg(mutex);
		if(handle)
			while(handle->valid())if(!handle->send_queue_item())break;
	}
	void force_write_and_close(char* data, size_t size){
		std::lock_guard lg(mutex);
		if(handle)
			handle->send_and_close(data, size);
	}
	void close(){
		std::lock_guard lg(mutex);
		if(handle)
			handle->close();
		handle = nullptr;
	}
	bool is_closed(){
		std::lock_guard lg(mutex);
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
	tcp_handle::error error(){
		std::lock_guard lg(mutex);
		if(handle)
			return handle->invalid_reason;
		return tcp_handle::error::local_close;
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
ValueItem* funs_TcpNetworkStream_error(ValueItem* args, uint32_t len){
	if(len == 1){
		if(args[0].meta.vtype != VType::proxy)
			throw InvalidArguments("The first argument must be a client_context.");
		return new ValueItem((uint8_t)((TcpNetworkStream*)((ProxyClass*)args[0].val)->class_ptr)->error());
	}else
		throw InvalidArguments("The number of arguments must be 1.");
}



void init_define_TcpNetworkStream(){
	if(!define_TcpNetworkStream.name.empty())
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
	define_TcpNetworkStream.funs["error"] = ClassFnDefine{new FuncEnviropment(funs_TcpNetworkStream_error, false), false, ClassAccess::pub};
}

#pragma endregion

#pragma region TcpNetworkBlocing
class TcpNetworkBlocing{
	friend class TcpNetworkManager;
	friend struct tcp_handle;
	tcp_handle* handle;
	TaskMutex mutex;
	TcpNetworkBlocing(tcp_handle* handle):handle(handle){}
public:
	~TcpNetworkBlocing(){
		std::lock_guard lg(mutex);
		if(handle)
			delete handle;
		handle = nullptr;
	}

	ValueItem read(uint32_t len){
		std::lock_guard lg(mutex);
		if(handle){
			char* buf = new char[len];
			handle->read_force(len, buf);
			if(len == 0){
				delete[] buf;
				return nullptr;
			}
			return ValueItem((uint8_t*)buf, len, no_copy);
		}
		return nullptr;
	}
	ValueItem available_bytes(){
		std::lock_guard lg(mutex);
		if(handle)
			return handle->available_bytes();
		return 0ui32;
	}
	ValueItem write(char* data, uint32_t len){
		std::lock_guard lg(mutex);
		if(handle)
			return handle->write_force(data, len);
		return nullptr;
	}
	void close(){
		std::lock_guard lg(mutex);
		if(handle){
			delete handle;
			handle = nullptr;
		}
	}
	bool is_closed(){
		std::lock_guard lg(mutex);
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
	tcp_handle::error error(){
		std::lock_guard lg(mutex);
		if(handle)
			return handle->invalid_reason;
		return tcp_handle::error::local_close;
	}
};

ValueItem* funs_TcpNetworkBlocing_read(ValueItem* args, uint32_t len){
	if(len == 2){
		if(args[0].meta.vtype != VType::proxy)
			throw InvalidArguments("The first argument must be a client_context.");
		return new ValueItem(((TcpNetworkBlocing*)((ProxyClass*)args[0].val)->class_ptr)->read((uint32_t)args[1]));
	}else
		throw InvalidArguments("The number of arguments must be 2.");
}
ValueItem* funs_TcpNetworkBlocing_available_bytes(ValueItem* args, uint32_t len){
	if(len == 1){
		if(args[0].meta.vtype != VType::proxy)
			throw InvalidArguments("The first argument must be a client_context.");
		return new ValueItem(((TcpNetworkBlocing*)((ProxyClass*)args[0].val)->class_ptr)->available_bytes());
	}else
		throw InvalidArguments("The number of arguments must be 1.");
}
ValueItem* funs_TcpNetworkBlocing_write(ValueItem* args, uint32_t len){
	if(len == 3){
		if(args[0].meta.vtype != VType::proxy)
			throw InvalidArguments("The first argument must be a client_context.");
		if(args[2].meta.vtype != VType::raw_arr_ui8)
			throw InvalidArguments("The third argument must be a raw_arr_ui8.");
		return new ValueItem(((TcpNetworkBlocing*)((ProxyClass*)args[0].val)->class_ptr)->write((char*)args[2].val, (uint32_t)args[1]));
	}else
		throw InvalidArguments("The number of arguments must be 3.");
}
ValueItem* funs_TcpNetworkBlocing_close(ValueItem* args, uint32_t len){
	if(len == 1){
		if(args[0].meta.vtype != VType::proxy)
			throw InvalidArguments("The first argument must be a client_context.");
		((TcpNetworkBlocing*)((ProxyClass*)args[0].val)->class_ptr)->close();
		return nullptr;
	}else
		throw InvalidArguments("The number of arguments must be 1.");
}
ValueItem* funs_TcpNetworkBlocing_is_closed(ValueItem* args, uint32_t len){
	if(len == 1){
		if(args[0].meta.vtype != VType::proxy)
			throw InvalidArguments("The first argument must be a client_context.");
		return new ValueItem(((TcpNetworkBlocing*)((ProxyClass*)args[0].val)->class_ptr)->is_closed());
	}else
		throw InvalidArguments("The number of arguments must be 1.");
}
ValueItem* funs_TcpNetworkBlocing_error(ValueItem* args, uint32_t len){
	if(len == 1){
		if(args[0].meta.vtype != VType::proxy)
			throw InvalidArguments("The first argument must be a client_context.");
		return new ValueItem((uint8_t)((TcpNetworkBlocing*)((ProxyClass*)args[0].val)->class_ptr)->error());
	}else
		throw InvalidArguments("The number of arguments must be 1.");
}

void init_define_TcpNetworkBlocking(){
	if(!define_TcpNetworkBlocking.name.empty())
		return;
	define_TcpNetworkBlocking.name = "tcp_network_blocking";
	define_TcpNetworkBlocking.destructor = AttachA::Interface::special::proxyDestruct<TcpNetworkBlocing, false>;
	define_TcpNetworkBlocking.copy = nullptr;
	define_TcpNetworkBlocking.funs["read"] = ClassFnDefine{new FuncEnviropment(funs_TcpNetworkBlocing_read, false), false, ClassAccess::pub};
	define_TcpNetworkBlocking.funs["available_bytes"] = ClassFnDefine{new FuncEnviropment(funs_TcpNetworkBlocing_available_bytes, false), false, ClassAccess::pub};
	define_TcpNetworkBlocking.funs["write"] = ClassFnDefine{new FuncEnviropment(funs_TcpNetworkBlocing_write, false), false, ClassAccess::pub};
	define_TcpNetworkBlocking.funs["close"] = ClassFnDefine{new FuncEnviropment(funs_TcpNetworkBlocing_close, false), false, ClassAccess::pub};
	define_TcpNetworkBlocking.funs["is_closed"] = ClassFnDefine{new FuncEnviropment(funs_TcpNetworkBlocing_is_closed, false), false, ClassAccess::pub};
	define_TcpNetworkBlocking.funs["error"] = ClassFnDefine{new FuncEnviropment(funs_TcpNetworkBlocing_error, false), false, ClassAccess::pub};
}

#pragma endregion

class TcpNetworkManager : public OverlappedController {
	TaskMutex safety;
	typed_lgr<class FuncEnviropment> handler_fn;
	typed_lgr<class FuncEnviropment> accept_filter;
    sockaddr_in6 connectionAddress;
	SOCKET main_socket;
public:
	int32_t default_len = 4096;
private:
	bool allow_new_connections = false;
	bool disabled = true;
	bool corrupted = false;
	size_t acceptors;
	TcpNetworkServer::ManageType manage_type;

	void make_acceptEx(void){
	re_try:
		static const auto adress_len = sizeof(sockaddr_storage) + 16;
		auto new_sock = WSASocketA(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		tcp_handle *pClientContext = new tcp_handle(new_sock, default_len);
		pClientContext->opcode = tcp_handle::Opcode::ACCEPT;
		BOOL success = _AcceptEx(
			main_socket,
			new_sock,
			pClientContext->buffer.buf,
			0,
			adress_len,
			adress_len,
			NULL,
			&pClientContext->overlapped
		);
		if(success != TRUE) {
			auto err = WSAGetLastError();
			if (err == WSA_IO_PENDING) 
				return;
			else if (err == WSAECONNRESET) {
				delete pClientContext;
				goto re_try;
			}
			else {
				delete pClientContext;
				return;
			}
		}
	}
	ValueItem accept_manager_construct(tcp_handle* self){
		switch (manage_type) {
		case TcpNetworkServer::ManageType::blocking:
			return ValueItem(new ProxyClass(new TcpNetworkBlocing(self), &define_TcpNetworkBlocking), VType::proxy, no_copy);
		case TcpNetworkServer::ManageType::write_delayed:
			return ValueItem(new ProxyClass(new TcpNetworkStream(self), &define_TcpNetworkStream), VType::proxy, no_copy);
		default:
			return nullptr;
		}
	}
	void accepted(tcp_handle* self,ValueItem clientAddr, ValueItem localAddr){
		if(!allow_new_connections){
			delete self;
			return;
		}
		std::lock_guard guard(safety);
		Task::start(new Task(handler_fn, ValueItem{
			accept_manager_construct(self), 
			std::move(clientAddr),
			std::move(localAddr)
		}));
	}

public:
	TcpNetworkManager(universal_address& ip_port, size_t acceptors,TcpNetworkServer::ManageType manage_type) : acceptors(acceptors),manage_type(manage_type) {
    	memcpy(&connectionAddress, &ip_port, sizeof(sockaddr_in6));
		main_socket = WSASocketA(AF_INET6, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (main_socket == INVALID_SOCKET){
			corrupted = true;
			return;
		}
		DWORD argp = 1;//non blocking
		int result = setsockopt(main_socket,SOL_SOCKET,SO_REUSEADDR,(char*)&argp, sizeof(argp));
		if (result == SOCKET_ERROR){
			corrupted = true;
			return;
		}
		argp = 0;
		result = setsockopt(main_socket,IPPROTO_IPV6,IPV6_V6ONLY,(char*)&argp, sizeof(argp));
		if (result == SOCKET_ERROR){
			corrupted = true;
			return;
		}
		if (ioctlsocket(main_socket, FIONBIO, &argp) == SOCKET_ERROR){
			corrupted = true;
			return;
		}
		init_win_fns(main_socket);
		if (bind(main_socket, (sockaddr*)&connectionAddress, sizeof(sockaddr_in6)) == SOCKET_ERROR){
			ValueItem error = std::string("Failed bind: ") + std::to_string(WSAGetLastError());
			errors.sync_notify(error);
			corrupted = true;
			return;
		}
		
		register_handle((HANDLE)main_socket, this);
	}
	~TcpNetworkManager(){
		shutdown();
	}
	
	void handle(void* _data, void* overlapped, DWORD dwBytesTransferred, bool status) {
		auto& data = *(tcp_handle*)overlapped;

		if(data.opcode == tcp_handle::Opcode::ACCEPT){
			make_acceptEx();
			
			universal_address* pClientAddr = NULL;
            universal_address* pLocalAddr = NULL;
            int remoteLen = sizeof(universal_address);
            int localLen = sizeof(universal_address);
            _GetAcceptExSockaddrs(data.buffer.buf,
                                         0,
                                         sizeof(universal_address) + 16,
                                         sizeof(universal_address) + 16,
                                         (LPSOCKADDR*)&pLocalAddr,
                                         &localLen,
                                         (LPSOCKADDR*)&pClientAddr,
                                         &remoteLen);
			ValueItem clientAddress(new ProxyClass(new universal_address(*pClientAddr),&define_UniversalAddress), VType::proxy, no_copy);
			ValueItem localAddress(new ProxyClass(new universal_address(*pLocalAddr),&define_UniversalAddress), VType::proxy, no_copy);
			if(accept_filter){
				if(AttachA::cxxCall(accept_filter,clientAddress,localAddress)){
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
				auto tmp = UniversalAddress::_define_to_string(&clientAddress,1);
				ValueItem notify{ "Client connected from: " + (std::string)*tmp };
				delete tmp;
				info.async_notify(notify);
			}
			#endif

			setsockopt(data.socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&main_socket, sizeof(main_socket) );
			{
				std::lock_guard lock(safety);
				register_handle((HANDLE)data.socket, &data);
			}
			
			accepted(&data, std::move(clientAddress), std::move(localAddress));
			return;
		}
		else if ((FALSE == status) || ((true == status) && (0 == dwBytesTransferred)))
		{
			#ifndef DISABLE_RUNTIME_INFO
			{
				auto tmp = UniversalAddress::_define_to_string(&clientAddress,1);
				ValueItem notify{ "Client disconnected" + (std::string)*tmp };
				delete tmp;
				info.async_notify(notify);
			}
			#endif
			data.connection_reset();
			return;
		}
		data.handle(dwBytesTransferred);
	}
	void set_on_connect(typed_lgr<class FuncEnviropment> handler_fn, TcpNetworkServer::ManageType manage_type){
		if(corrupted)
			throw AttachARuntimeException("TcpNetworkManager is corrupted");
		std::lock_guard lock(safety);
		this->handler_fn = handler_fn;
		this->manage_type = manage_type;
	}
	void mainline(){
		if(corrupted)
			throw AttachARuntimeException("TcpNetworkManager is corrupted");
		
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
			throw AttachARuntimeException("TcpNetworkManager is corrupted");
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
			throw AttachARuntimeException("TcpNetworkManager is corrupted");
		allow_new_connections = false;
	}
	void resume(){
		if(corrupted)
			throw AttachARuntimeException("TcpNetworkManager is corrupted");
		allow_new_connections = true;
	}
	void start(size_t pool_size){
		if(corrupted)
			throw AttachARuntimeException("TcpNetworkManager is corrupted");
		std::lock_guard lock(safety);
		allow_new_connections = true; 
		if(!disabled)
			return;
		if (listen(main_socket, SOMAXCONN) == SOCKET_ERROR){
			WSACleanup();
			return;
		}
		if(pool_size)
			OverlappedController::run(pool_size);
		else
			OverlappedController::run();
		for(size_t i = 0; i < acceptors; i++)
			make_acceptEx();
		disabled = false;
	}


	void set_accept_filter(typed_lgr<class FuncEnviropment> filter){
		if(corrupted)
			throw AttachARuntimeException("TcpNetworkManager is corrupted");
		std::lock_guard lock(safety);
		this->accept_filter = filter;
	}
	bool is_corrupted(){
		return corrupted;
	}
	void set_pool_size(size_t pool_size){
		if(corrupted)
			throw AttachARuntimeException("TcpNetworkManager is corrupted");
		std::lock_guard lock(safety);
		if(disabled)
			return;
		if(!pool_size)
			pool_size = std::thread::hardware_concurrency();
		OverlappedController::set_dispatchers(pool_size);
	}

	uint16_t port(){
		if(corrupted)
			throw AttachARuntimeException("TcpNetworkManager is corrupted");
		return htons(connectionAddress.sin6_port);
	}
	std::string ip(){
		if(corrupted)
			throw AttachARuntimeException("TcpNetworkManager is corrupted");

		ProxyClass tmp(&connectionAddress, &define_UniversalAddress);
		ValueItem args(&tmp, VType::proxy, as_refrence);
		ValueItem* res;
		try{
			res = UniversalAddress::_define_to_string(&args, 1);
		}catch(...){
			tmp.declare_ty = nullptr;//prevent delete
			throw;
		}
		tmp.declare_ty = nullptr;//prevent delete
		std::string ret = (std::string)*res;
		delete res;
		return ret;
	}
	ValueItem address(){
		if(corrupted)
			throw AttachARuntimeException("TcpNetworkManager is corrupted");

		sockaddr_storage* addr = new sockaddr_storage;
		memcpy(addr, &connectionAddress, sizeof(sockaddr_in6));
		memset(addr + sizeof(sockaddr_in6), 0, sizeof(sockaddr_storage) - sizeof(sockaddr_in6));
		return ValueItem(new ProxyClass(addr, &define_UniversalAddress), VType::proxy, no_copy);
	}

	bool is_paused(){
		return !disabled && !allow_new_connections;
	}
};
#pragma endregion
#pragma region UDP
struct udp_handle{
	static constexpr auto adress_len = sizeof(universal_address) + 16;
	static constexpr auto meta_length = adress_len * 2;

	WSAOVERLAPPED overlapped;
	SOCKET socket;
	WSABUF buffer;
	universal_address* clientAddress = nullptr;
	enum class Opcode{
		READ,
		WRITE
	} opcode = Opcode::WRITE;
	udp_handle(SOCKET socket, size_t buffer_len) : socket(socket){
		ZeroMemory(&overlapped, sizeof(overlapped));
		buffer.len = buffer_len;
		buffer.buf = new char[buffer_len];
	}
	~udp_handle(){
		delete[] buffer.buf;
		if(clientAddress)
			delete clientAddress;
	}
};
class UdpNetworkManager : public OverlappedController{
	TaskMutex safety;
	typed_lgr<class FuncEnviropment> income_fn;
	typed_lgr<class FuncEnviropment> outcome_fault_fn;
    sockaddr_in6 connectionAddress;
	SOCKET main_socket;
public:
	int32_t buffer_len = 4096;
private:
	bool allow_new_connections = false;
	bool disabled = true;
	bool corrupted = false;
	size_t acceptors;


	void make_acceptEx(void){
		auto new_sock = WSASocketA(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
		udp_handle *pClientContext = new udp_handle(new_sock, buffer_len);
		pClientContext->opcode = udp_handle::Opcode::READ;
		int nBytesRecv = _AcceptEx(main_socket, new_sock, pClientContext->buffer.buf, buffer_len - udp_handle::adress_len * 2, udp_handle::adress_len, udp_handle::adress_len, NULL, &pClientContext->overlapped);
		if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError())){
			delete pClientContext;
			return;
		}
	}
	static typed_lgr<class FuncEnviropment> accept_handle;
	static ValueItem* _accept_handle(ValueItem* args, uint32_t len){
		udp_handle* handle = (udp_handle*)(void*)args[0];
		typed_lgr<class FuncEnviropment> accept_handle = *args[1].funPtr();
		ValueItem fn_args((uint8_t*)handle->buffer.buf + udp_handle::meta_length, handle->buffer.len,as_refrence);
		ValueItem client_ip = ValueItem(new ProxyClass((universal_address*)args[2].val, &define_UniversalAddress), VType::proxy, no_copy);
		ValueItem manager = args[3];
		AttachA::cxxCall(accept_handle, fn_args, client_ip, manager);
		delete handle;
		return nullptr;
	}

	
	void accepted(udp_handle* self, universal_address* clientAddress){
		if(!allow_new_connections){
			delete self;
			return;
		}
		std::lock_guard guard(safety);
		Task::start(new Task(accept_handle,ValueItem{ self, income_fn, clientAddress, ValueItem(new ProxyClass(this, &define_UdpNetworkManager), VType::proxy, no_copy)}));
	}
	
	
	
	static typed_lgr<class FuncEnviropment> sended_fault_handle;
	static ValueItem* _sended_fault_handle(ValueItem* args, uint32_t len){
		typed_lgr<class FuncEnviropment> fault_handle = *args[0].funPtr();

		udp_handle* handle = (udp_handle*)(void*)args[1];
		ValueItem fail_send_array((uint8_t*)handle->buffer.buf, handle->buffer.len,as_refrence);
		ValueItem client_ip = ValueItem(new ProxyClass(new universal_address(*handle->clientAddress), &define_UniversalAddress), VType::proxy, no_copy);
		ValueItem sended_len = args[2];
		ValueItem manager = args[3];

		AttachA::cxxCall(fault_handle, fail_send_array, client_ip, sended_len, manager);
		delete handle;
		return nullptr;
	}
	void sended(udp_handle* self, DWORD dwBytesTransferred){
		if(self->buffer.len < dwBytesTransferred && outcome_fault_fn) {
			std::lock_guard guard(safety);
			ValueItem args{ outcome_fault_fn, self, (uint32_t)dwBytesTransferred, ValueItem(new ProxyClass(this, &define_UdpNetworkManager), VType::proxy, no_copy)};
			Task::start(new Task(sended_fault_handle, std::move(args)));
			return;
		}
		delete self;
	}
public:
	UdpNetworkManager(universal_address& ip_port, size_t acceptors) : acceptors(acceptors) {
		memcpy(&connectionAddress, &ip_port, sizeof(sockaddr_in6));
		main_socket = WSASocketA(AF_INET6, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (main_socket == INVALID_SOCKET){
			corrupted = true;
			return;
		}
		DWORD argp = 1;//non blocking
		int result = setsockopt(main_socket,SOL_SOCKET,SO_REUSEADDR,(char*)&argp, sizeof(argp));
		if (result == SOCKET_ERROR){
			corrupted = true;
			return;
		}
		argp = 0;//dual stack
		result = setsockopt(main_socket,IPPROTO_IPV6,IPV6_V6ONLY,(char*)&argp, sizeof(argp));
		if (result == SOCKET_ERROR){
			corrupted = true;
			return;
		}
		if (ioctlsocket(main_socket, FIONBIO, &argp) == SOCKET_ERROR){
			corrupted = true;
			return;
		}
		init_win_fns(main_socket);
		if (bind(main_socket, (sockaddr*)&connectionAddress, sizeof(sockaddr_in6)) == SOCKET_ERROR){
			corrupted = true;
			return;
		}
		register_handle((HANDLE)main_socket, this);
	}

	void handle(void* _data, void* overlapped, DWORD dwBytesTransferred, bool status) {
		if(!overlapped)
			return;
		auto& data = *(udp_handle*)overlapped;
		if ((FALSE == status) || ((true == status) && (0 == dwBytesTransferred))) {
			if(data.opcode == udp_handle::Opcode::READ)
				make_acceptEx();
			delete &data;
			return;
		}
		switch (data.opcode) {
			case udp_handle::Opcode::READ: {
				universal_address* pClientAddr = NULL;
				universal_address* pLocalAddr = NULL;
				int remoteLen = sizeof(universal_address);
				int localLen = sizeof(universal_address);
				_GetAcceptExSockaddrs(data.buffer.buf,
											0,
											sizeof(universal_address) + 16,
											sizeof(universal_address) + 16,
											(LPSOCKADDR*)&pLocalAddr,
											&localLen,
											(LPSOCKADDR*)&pClientAddr,
											&remoteLen);

				

				setsockopt(data.socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&main_socket, sizeof(main_socket) );
				{
					std::lock_guard lock(safety);
					register_handle((HANDLE)data.socket, &data);
				}
				
				accepted(&data, new universal_address(*pClientAddr));
				return;
			}
			case udp_handle::Opcode::WRITE: sended(&data, dwBytesTransferred);return;
		}
	}
	void set_on_connect(typed_lgr<class FuncEnviropment> handler_fn){
		if(corrupted)
			throw AttachARuntimeException("UdpNetworkManager is corrupted");
		std::lock_guard lock(safety);
		this->income_fn = handler_fn;
	}
	void set_on_send_fault(typed_lgr<class FuncEnviropment> handler_fn){
		if(corrupted)
			throw AttachARuntimeException("UdpNetworkManager is corrupted");
		std::lock_guard lock(safety);
		this->outcome_fault_fn = handler_fn;
	}
	void mainline(){
		if(corrupted)
			throw AttachARuntimeException("UdpNetworkManager is corrupted");
		
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
			throw AttachARuntimeException("UdpNetworkManager is corrupted");
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
			throw AttachARuntimeException("UdpNetworkManager is corrupted");
		allow_new_connections = false;
	}
	void resume(){
		if(corrupted)
			throw AttachARuntimeException("UdpNetworkManager is corrupted");
		allow_new_connections = true;
	}
	void start(size_t pool_size){
		if(corrupted)
			throw AttachARuntimeException("UdpNetworkManager is corrupted");
		std::lock_guard lock(safety);
		allow_new_connections = true; 
		if(!disabled)
			return;
		if (listen(main_socket, SOMAXCONN) == SOCKET_ERROR){
			WSACleanup();
			return;
		}
		if(pool_size)
			OverlappedController::run(pool_size);
		else
			OverlappedController::run();
		
		for(size_t i = 0; i < acceptors; i++)
			make_acceptEx();
		disabled = false;
	}

	bool is_corrupted(){
		return corrupted;
	}
	void set_pool_size(size_t pool_size){
		if(corrupted)
			throw AttachARuntimeException("UdpNetworkManager is corrupted");
		std::lock_guard lock(safety);
		if(disabled)
			return;
		if(!pool_size)
			pool_size = std::thread::hardware_concurrency();
		OverlappedController::set_dispatchers(pool_size);
	}

	bool send(const void* data, size_t size, const universal_address& addr){
		if(corrupted)
			throw AttachARuntimeException("UdpNetworkManager is corrupted");
		std::lock_guard lock(safety);
		if(disabled)
			return false;
		udp_handle* handle = new udp_handle(main_socket, size);
		handle->buffer.buf = new char[size];
		memcpy(handle->buffer.buf, data, size);
		handle->clientAddress = new universal_address(addr);
		handle->socket = main_socket;
		handle->opcode = udp_handle::Opcode::WRITE;
		if(WSASendTo(main_socket, &handle->buffer, 1, NULL, 0, (sockaddr*)&addr, sizeof(universal_address), (LPWSAOVERLAPPED)handle, NULL) == SOCKET_ERROR){
			if(WSAGetLastError() != WSA_IO_PENDING){
				delete handle;
				return false;
			}
		}
		return true;
	}

	
	uint16_t port(){
		if(corrupted)
			throw AttachARuntimeException("TcpNetworkManager is corrupted");
		return htons(connectionAddress.sin6_port);
	}
	std::string ip(){
		if(corrupted)
			throw AttachARuntimeException("TcpNetworkManager is corrupted");

		ProxyClass tmp(&connectionAddress, &define_UniversalAddress);
		ValueItem args(&tmp, VType::proxy, as_refrence);
		ValueItem* res;
		try{
			res = UniversalAddress::_define_to_string(&args, 1);
		}catch(...){
			tmp.declare_ty = nullptr;//prevent delete
			throw;
		}
		tmp.declare_ty = nullptr;//prevent delete
		std::string ret = (std::string)*res;
		delete res;
		return ret;
	}
	ValueItem address(){
		if(corrupted)
			throw AttachARuntimeException("TcpNetworkManager is corrupted");

		sockaddr_storage* addr = new sockaddr_storage;
		memcpy(addr, &connectionAddress, sizeof(sockaddr_in6));
		memset(addr + sizeof(sockaddr_in6), 0, sizeof(sockaddr_storage) - sizeof(sockaddr_in6));
		return ValueItem(new ProxyClass(addr, &define_UniversalAddress), VType::proxy, no_copy);
	}
	bool is_disabled(){
		return disabled;
	}
	bool is_paused(){
		return !disabled && !allow_new_connections;
	}
};
typed_lgr<class FuncEnviropment> UdpNetworkManager::accept_handle = new FuncEnviropment(UdpNetworkManager::_accept_handle, false);
typed_lgr<class FuncEnviropment> UdpNetworkManager::sended_fault_handle = new FuncEnviropment(UdpNetworkManager::_sended_fault_handle, false);


namespace proxy_UdpNetworkManager{
	ValueItem* _define_sendto(ValueItem* args,uint32_t len){
		if(len < 3)
			throw InvalidArguments("udp_manager &.sendto, expected 1 argument, got " + std::to_string(len));
		if(args[0].meta.vtype != VType::proxy)
			throw InvalidArguments("udp_manager &.sendto, excepted proxy, got " + enum_to_string(args[0].meta.vtype));
		ProxyClass& manager_proxy = (ProxyClass&)args[0];
		if(manager_proxy.declare_ty != &define_UdpNetworkManager){
			if(manager_proxy.declare_ty->name != "udp_manager &")
				throw InvalidArguments("udp_manager &.sendto, excepted udp_manager &, got " + manager_proxy.declare_ty->name);
			else
				throw InvalidArguments("udp_manager &.sendto, excepted udp_manager &, got non native udp_manager &");
		}
		if(args[1].meta.vtype != VType::proxy)
			throw InvalidArguments("udp_manager &.sendto, excepted proxy, got " + enum_to_string(args[0].meta.vtype));
		ProxyClass& address_proxy = (ProxyClass&)args[0];
		if(address_proxy.declare_ty != &define_UniversalAddress){
			if(address_proxy.declare_ty->name != "universal_address")
				throw InvalidArguments("udp_manager &.sendto, excepted universal_address, got " + address_proxy.declare_ty->name);
			else
				throw InvalidArguments("udp_manager &.sendto, excepted universal_address, got non native universal_address");
		}




		auto* manager = (UdpNetworkManager*)manager_proxy.class_ptr;
		switch (args[1].meta.vtype ) {
		case VType::raw_arr_ui8:
		case VType::raw_arr_i8:
			return new ValueItem(manager->send(args[1].getSourcePtr(), args[1].meta.val_len, *((universal_address*)address_proxy.class_ptr)));
		case VType::raw_arr_ui16:
		case VType::raw_arr_i16:
			return new ValueItem(manager->send(args[1].getSourcePtr(), args[1].meta.val_len * 2, *((universal_address*)address_proxy.class_ptr)));
		case VType::raw_arr_ui32:
		case VType::raw_arr_i32:
		case VType::raw_arr_flo:
			return new ValueItem(manager->send(args[1].getSourcePtr(), args[1].meta.val_len * 4, *((universal_address*)address_proxy.class_ptr)));
		case VType::raw_arr_ui64:
		case VType::raw_arr_i64:
		case VType::raw_arr_doub:
			return new ValueItem(manager->send(args[1].getSourcePtr(), args[1].meta.val_len * 8, *((universal_address*)address_proxy.class_ptr)));
		default:
			throw InvalidArguments("udp_manager &.sendto, excepted raw array, got " + enum_to_string(args[1].meta.vtype));
			break;
		}
	}
	typed_lgr<class FuncEnviropment> define_sendto = new FuncEnviropment(_define_sendto, false);
}
#pragma endregion
void init_define_UdpNetworkManager(){
	if(!define_UdpNetworkManager.name.empty())
		return;
	define_UdpNetworkManager.name = "udp_manager &";
	define_UdpNetworkManager.funs["sendto"] = ClassFnDefine(proxy_UdpNetworkManager::define_sendto, false, ClassAccess::pub);
}



uint8_t init_networking(){
	init_define_UniversalAddress();
	init_define_UdpNetworkManager();
	init_define_TcpNetworkStream();
	init_define_TcpNetworkBlocking();
	
	if(WSAStartup(MAKEWORD(2, 2), &wsaData)){
		auto err = WSAGetLastError();
		switch (err) {
		case WSASYSNOTREADY:return 1;
		case WSAVERNOTSUPPORTED:return 2;return 3;
		case WSAEPROCLIM:return 4;
		case WSAEFAULT:return 5;
		default:return 0xFF;
		}
	};
	inited = true;
	return 0;
}
void deinit_networking(){
	if(inited)
		WSACleanup();
	inited = false;
}

#else
//TO_DO: LINUX implementation using io_uring

uint8_t init_networking(){
	init_define_UniversalAddress();
	init_define_UdpNetworkManager();
	init_define_TcpNetworkStream();
	init_define_TcpNetworkBlocking();
	
	inited = true;
}
void deinit_networking(){
	//inited = false;
}
#endif

TcpNetworkServer::TcpNetworkServer(typed_lgr<class FuncEnviropment> on_connect, ValueItem& ip_port, ManageType mt, size_t acceptors){
	if(!inited)
		throw InternalException("Network module not initialized");
	if(ip_port.meta.vtype == VType::proxy){
		ProxyClass& proxy = (ProxyClass&)ip_port;
		if(proxy.declare_ty != &define_UniversalAddress){
			if(proxy.declare_ty->name != "universal_address")
				throw InvalidArguments("excepted universal_address, got " + proxy.declare_ty->name);
			else
				throw InvalidArguments("excepted universal_address, got non native universal_address");
		}
		handle = new TcpNetworkManager((universal_address&)proxy.class_ptr, acceptors, mt);
	}else if(ip_port.meta.vtype == VType::string){
		universal_address addr;
		internal_makeIP_port(addr, ((std::string)ip_port).c_str());
		handle = new TcpNetworkManager(addr, acceptors, mt);
	}else
		throw InvalidArguments("excepted universal_address or string, got " + enum_to_string(ip_port.meta.vtype));
		
	handle->set_on_connect(on_connect, mt);
}
TcpNetworkServer::~TcpNetworkServer(){
	if(handle)
		delete handle;
	handle = nullptr;
}
void TcpNetworkServer::start(size_t pool_size){
	handle->start(pool_size);
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
void TcpNetworkServer::set_pool_size(size_t pool_size){
	handle->set_pool_size(pool_size);
}

uint16_t TcpNetworkServer::server_port(){
	return handle->port();
}
std::string TcpNetworkServer::server_ip(){
	return handle->ip();
}
ValueItem TcpNetworkServer::server_address(){
	return handle->address();
}
bool TcpNetworkServer::is_paused(){
	return handle->is_paused();
}

UdpNetworkServer::UdpNetworkServer(typed_lgr<class FuncEnviropment> packet_handler, uint32_t buffer_len, ValueItem& ip_port, size_t acceptors){
	if(!inited)
		throw InternalException("Network module not initialized");

	if(ip_port.meta.vtype == VType::proxy){
		ProxyClass& proxy = (ProxyClass&)ip_port;
		if(proxy.declare_ty != &define_UniversalAddress){
			if(proxy.declare_ty->name != "universal_address")
				throw InvalidArguments("excepted universal_address, got " + proxy.declare_ty->name);
			else
				throw InvalidArguments("excepted universal_address, got non native universal_address");
		}
		handle = new UdpNetworkManager((universal_address&)proxy.class_ptr, acceptors);
	}else if(ip_port.meta.vtype == VType::string){
		universal_address addr;
		internal_makeIP_port(addr, ((std::string)ip_port).c_str());
		handle = new UdpNetworkManager(addr, acceptors);
	}else
		throw InvalidArguments("excepted universal_address or string, got " + enum_to_string(ip_port.meta.vtype));
	handle->buffer_len = buffer_len;
	if(handle->buffer_len != buffer_len)
		throw InternalException("Invalid buffer length, expected " + std::to_string(buffer_len) + ", got " + std::to_string(handle->buffer_len));
	handle->set_on_connect(packet_handler);
}
UdpNetworkServer::~UdpNetworkServer(){
	if(handle)
		delete handle;
	handle = nullptr;
}
void UdpNetworkServer::start(size_t pool_size){
	handle->start(pool_size);
}
void UdpNetworkServer::stop(){
	handle->shutdown();
}
bool UdpNetworkServer::is_running(){
	return handle->in_run();
}
void UdpNetworkServer::mainline(){
	handle->mainline();
}
bool UdpNetworkServer::is_corrupted(){
	return handle->is_corrupted();
}
void UdpNetworkServer::set_pool_size(size_t pool_size){
	handle->set_dispatchers(pool_size);
}


uint16_t UdpNetworkServer::server_port(){
	return handle->port();
}
std::string UdpNetworkServer::server_ip(){
	return handle->ip();
}
ValueItem UdpNetworkServer::server_address(){
	return handle->address();
}

bool UdpNetworkServer::is_disabled(){
	return handle->is_disabled();
}
bool UdpNetworkServer::is_paused(){
	return handle->is_paused();
}

bool ipv6_supported(){
	if(!inited)
		throw InternalException("Network module not initialized");
	static int ipv6_supported = -1;
	if(ipv6_supported == -1){
		ipv6_supported = 0;
		SOCKET sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
		if(sock != INVALID_SOCKET){
			ipv6_supported = 1;
			closesocket(sock);
		}
	}
	return ipv6_supported == 1;
}


ValueItem makeIP4(const char* ip, uint16_t port){
	if(!inited)
		throw InternalException("Network module not initialized");
	universal_address* addr_storage = new universal_address;
	internal_makeIP4(*addr_storage, ip, port);
	return ValueItem(new ProxyClass(addr_storage, &define_UniversalAddress), VType::proxy);
}
ValueItem makeIP6(const char* ip, uint16_t port){
	if(!inited)
		throw InternalException("Network module not initialized");
	universal_address* addr_storage = new universal_address;
	internal_makeIP6(*addr_storage, ip, port);
	return ValueItem(new ProxyClass(addr_storage, &define_UniversalAddress), VType::proxy);
}
ValueItem makeIP(const char* ip, uint16_t port){
	if(!inited)
		throw InternalException("Network module not initialized");
	universal_address* addr_storage = new universal_address;
	internal_makeIP(*addr_storage, ip, port);
	return ValueItem(new ProxyClass(addr_storage, &define_UniversalAddress), VType::proxy);
}


ValueItem makeIP4_port(const char* ip_port){
	if(!inited)
		throw InternalException("Network module not initialized");
	universal_address* addr_storage = new universal_address;
	internal_makeIP4_port(*addr_storage, ip_port);
	return ValueItem(new ProxyClass(addr_storage, &define_UniversalAddress), VType::proxy);
}
ValueItem makeIP6_port(const char* ip_port){
	if(!inited)
		throw InternalException("Network module not initialized");
	universal_address* addr_storage = new universal_address;
	internal_makeIP6_port(*addr_storage, ip_port);
	return ValueItem(new ProxyClass(addr_storage, &define_UniversalAddress), VType::proxy);
}
ValueItem makeIP_port(const char* ip_port){
	if(ip_port[0] == '[')
		return makeIP6_port(ip_port);
	else
		return makeIP4_port(ip_port);
}
