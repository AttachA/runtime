// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef RUN_TIME_CXX_LIBRARY_NETWORKING
#define RUN_TIME_CXX_LIBRARY_NETWORKING
#include <run_time/attacha_abi_structs.hpp>
#include <util/link_garbage_remover.hpp>
#include <util/ustring.hpp>
namespace art{
	namespace files{
		class FileHandle;
		class BlockingFileHandle;
	}
	struct TcpConfiguration {
		uint32_t recv_timeout_ms = 2000;
		uint32_t send_timeout_ms = 2000;
		uint32_t buffer_size = 8192;
		uint32_t fast_open_queue = 5;//0 - disable fast open

		uint32_t connection_timeout_ms = 2000;//set send_timeout_ms to this value when connecting to server, rollback to send_timeout_ms after connection, also start user space timeout when connecting
		//int32_t max_retransmit_count; is not portable across platforms

		bool allow_ip4 : 1 = true;
		bool enable_delay : 1 = true;//TCP_NODELAY
		bool enable_timestamps : 1 = true;//TCP_TIMESTAMP, some websites report that enabling this option can cause performance spikes, turn off if you have problems
		bool enable_keep_alive : 1 = true;
		struct {
			uint32_t idle_ms = 5000;
			uint32_t interval_ms = 3000;
			uint8_t retry_count = 3;//255 is max,0 - invalid value and will be replaced by 3
			uint32_t user_timeout_ms = idle_ms + interval_ms * retry_count;//not recommended to decrease this value
		} keep_alive_settings;
	};
	class TcpNetworkServer {
		struct TcpNetworkManager* handle;
	public:
		enum class ManageType{
			blocking,
			write_delayed
		};

		TcpNetworkServer(art::shared_ptr<FuncEnvironment> on_connect, ValueItem& ip_port, ManageType manage_type, size_t acceptors = 10, TcpConfiguration config = {});
		~TcpNetworkServer();
		void start();
		void pause();
		void resume();
		void stop();
		ValueItem accept(bool ignore_acceptors = false);
		void _await();

		bool is_running();
		bool is_paused();
		bool is_corrupted();

		uint16_t server_port();
		art::ustring server_ip();
		ValueItem server_address();
		//apply to new connections
		void set_configuration(TcpConfiguration config);
		void set_accept_filter(art::shared_ptr<FuncEnvironment> filter);
	};
	class TcpClientSocket{
		class TcpClientManager* handle;
		TcpClientSocket();
	public:
		~TcpClientSocket();
		static TcpClientSocket* connect(ValueItem& ip_port, TcpConfiguration config = {});
		static TcpClientSocket* connect(ValueItem& ip_port, char* data, uint32_t size, TcpConfiguration config = {});
		//apply to current connection
		void set_configuration(TcpConfiguration config);
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
		
		ValueItem local_address();
		ValueItem remote_address();
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
	ValueItem construct_TcpConfiguration();
}
#endif /* RUN_TIME_CXX_LIBRARY_NETWORKING */
