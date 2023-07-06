// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef RUN_TIME_CXX_LIBRARY_NETWORKING
#define RUN_TIME_CXX_LIBRARY_NETWORKING
#include "../link_garbage_remover.hpp"
#include "../attacha_abi_structs.hpp"
namespace art{
	namespace files{
		class FileHandle;
		class BlockingFileHandle;
	}
	class TcpNetworkServer {
		struct TcpNetworkManager* handle;
	public:
		enum class ManageType{
			blocking,
			write_delayed
		};

		TcpNetworkServer(typed_lgr<class FuncEnvironment> on_connect, ValueItem& ip_port, ManageType manage_type, size_t acceptors = 10, int32_t timeout_ms = 0, int32_t default_buffer = 8192);
		~TcpNetworkServer();
		void start();
		void pause();
		void resume();
		void stop();
		void _await();

		bool is_running();
		bool is_paused();
		bool is_corrupted();

		uint16_t server_port();
		std::string server_ip();
		ValueItem server_address();
		void set_default_buffer_size(int32_t size);
		void set_accept_filter(typed_lgr<class FuncEnvironment> filter);
	};
	class TcpClientSocket{
		class TcpClientManager* handle;
		TcpClientSocket();
	public:
		~TcpClientSocket();
		static TcpClientSocket* connect(ValueItem& ip_port);
		static TcpClientSocket* connect(ValueItem& ip_port, int32_t timeout_ms);

		static TcpClientSocket* connect(ValueItem& ip_port, char* data, uint32_t size);
		static TcpClientSocket* connect(ValueItem& ip_port, char* data, uint32_t size, int32_t timeout_ms);

		int32_t recv(uint8_t* data, int32_t size);
		bool send(uint8_t* data, int32_t size);
		bool send_file(const char* file_path, size_t file_path_len, uint64_t data_len, uint64_t offset, uint32_t chunks_size);
		bool send_file(class art::files::FileHandle& file_path, uint64_t data_len, uint64_t offset, uint32_t chunks_size);
		bool send_file(class art::files::BlockingFileHandle& file, uint64_t data_len, uint64_t offset, uint32_t chunks_size);
		void close();
		void reset();
		void rebuffer(int32_t size);
	};
	struct udp_socket{
		class udp_handle* handle;
		udp_socket(ValueItem& ip_port, uint32_t timeout_ms);
		~udp_socket();

		uint32_t recv(uint8_t* data, uint32_t size, ValueItem& sender);
		uint32_t send(uint8_t* data, uint32_t size, ValueItem& to);
	};

	uint8_t init_networking();
	void deinit_networking();
	bool ipv6_supported();

	ValueItem makeIP4(const char* ip, uint16_t port = 0);
	ValueItem makeIP6(const char* ip, uint16_t port = 0);
	ValueItem makeIP(const char* ip, uint16_t port = 0);
	ValueItem makeIP4_port(const char* ip_port);//ip4:port
	ValueItem makeIP6_port(const char* ip_port);//ip6:port
	ValueItem makeIP_port(const char* ip_port);//ip:port
}
#endif /* RUN_TIME_CXX_LIBRARY_NETWORKING */
