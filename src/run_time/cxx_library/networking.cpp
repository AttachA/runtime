// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <util/platform.hpp>
#if PLATFORM_WINDOWS
#define _WINSOCKAPI_
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <stdio.h>
#pragma comment(lib, "Ws2_32.lib")
#elif PLATFORM_LINUX
#include <arpa/inet.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>
#else
#error Unsupported platform
#endif

#include <condition_variable>
#include <utf8cpp/utf8.h>
#include <configuration/agreement/symbols.hpp>
#include <run_time/AttachA_CXX.hpp>
#include <run_time/cxx_library/files.hpp>
#include <run_time/cxx_library/networking.hpp>
#include <run_time/asm/FuncEnvironment.hpp>
#include <run_time/tasks/util/native_workers_singleton.hpp>

namespace art {
    AttachAVirtualTable* define_UniversalAddress = nullptr;
    AttachAVirtualTable* define_TcpConfiguration = nullptr;

    void init_define_UniversalAddress();
    using universal_address = ::sockaddr_storage;

    namespace UniversalAddress {
        enum class AddressType : uint32_t {
            IPv4,
            IPv6,
            UNDEFINED = (uint32_t)-1
        };

        ValueItem* _define_to_string(ValueItem* args, uint32_t len) {
            if (len < 1)
                throw InvalidArguments("universal_address, expected 1 argument, got " + std::to_string(len));
            auto& address = CXX::Interface::getExtractAs<universal_address>(args[0], define_UniversalAddress);
            art::ustring result;
            result.resize(INET6_ADDRSTRLEN);
            if (address.ss_family == AF_INET)
                inet_ntop(AF_INET, &((sockaddr_in&)address).sin_addr, result.data(), INET6_ADDRSTRLEN);
            else if (address.ss_family == AF_INET6) {
                inet_ntop(AF_INET6, &((sockaddr_in6&)address).sin6_addr, result.data(), INET6_ADDRSTRLEN);
                if (result.starts_with("::ffff:"))
                    result = result.substr(7);
            } else
                throw std::runtime_error("universal_address, unknown address family");
            result.resize(strlen(result.data()));

            return new ValueItem(result);
        }

        ValueItem* _define_type(ValueItem* args, uint32_t len) {
            if (len < 1)
                throw InvalidArguments("universal_address, expected 1 argument, got " + std::to_string(len));
            auto& address = CXX::Interface::getExtractAs<universal_address>(args[0], define_UniversalAddress);
            art::ustring result;
            result.resize(INET6_ADDRSTRLEN);
            if (address.ss_family == AF_INET)
                return new ValueItem((uint32_t)AddressType::IPv4);
            else if (address.ss_family == AF_INET6) {
                inet_ntop(AF_INET6, &((sockaddr_in6&)address).sin6_addr, result.data(), INET6_ADDRSTRLEN);
                if (result.starts_with("::ffff:"))
                    return new ValueItem((uint32_t)AddressType::IPv4);
                else
                    return new ValueItem((uint32_t)AddressType::IPv6);
            } else
                throw std::runtime_error("universal_address, unknown address family");
        }

        ValueItem* _define_actual_type(ValueItem* args, uint32_t len) {
            if (len < 1)
                throw InvalidArguments("universal_address, expected 1 argument, got " + std::to_string(len));
            auto& address = CXX::Interface::getExtractAs<universal_address>(args[0], define_UniversalAddress);
            if (address.ss_family == AF_INET)
                return new ValueItem((uint32_t)AddressType::IPv4);
            else if (address.ss_family == AF_INET6) {
                return new ValueItem((uint32_t)AddressType::IPv6);
            } else
                throw std::runtime_error("universal_address, unknown address family");
        }

        ValueItem* _define_port(ValueItem* args, uint32_t len) {
            if (len < 1)
                throw InvalidArguments("universal_address, expected 1 argument, got " + std::to_string(len));
            auto& address = CXX::Interface::getExtractAs<universal_address>(args[0], define_UniversalAddress);
            if (address.ss_family == AF_INET)
                return new ValueItem((uint32_t)((sockaddr_in&)address).sin_port);
            else if (address.ss_family == AF_INET6) {
                return new ValueItem((uint32_t)((sockaddr_in6&)address).sin6_port);
            } else
                throw std::runtime_error("universal_address, unknown address family");
        }

        ValueItem* _define_full_address(ValueItem* args, uint32_t len) {
            if (len < 1)
                throw InvalidArguments("universal_address, expected 1 argument, got " + std::to_string(len));
            auto& address = CXX::Interface::getExtractAs<universal_address>(args[0], define_UniversalAddress);
            art::ustring result;
            result.resize(INET6_ADDRSTRLEN);

            auto actual_family = address.ss_family;
            if (address.ss_family == AF_INET)
                inet_ntop(AF_INET, &((sockaddr_in&)address).sin_addr, result.data(), INET6_ADDRSTRLEN);
            else if (address.ss_family == AF_INET6) {
                inet_ntop(AF_INET6, &((sockaddr_in6&)address).sin6_addr, result.data(), INET6_ADDRSTRLEN);
                if (result.starts_with("::ffff:")) {
                    result = result.substr(7);
                    actual_family = AF_INET;
                }
            } else
                throw std::runtime_error("universal_address.to_string, unknown address family");
            result.resize(strlen(result.data()));
            if (actual_family == AF_INET) {
                result += ":" + std::to_string((uint32_t)htons(((sockaddr_in&)address).sin_port));
            } else {
                result = '[' + result + "]:" + std::to_string((uint32_t)htons(((sockaddr_in6&)address).sin6_port));
            }
            return new ValueItem(result);
        }
    }

    namespace TcpConfiguration_map {
        AttachAFun(_define_get_recv_timeout_ms, 1, {
            auto& conf = CXX::Interface::getExtractAs<TcpConfiguration>(args[0], define_TcpConfiguration);
            return conf.recv_timeout_ms;
        });
        AttachAFun(_define_get_send_timeout_ms, 1, {
            auto& conf = CXX::Interface::getExtractAs<TcpConfiguration>(args[0], define_TcpConfiguration);
            return conf.send_timeout_ms;
        });
        AttachAFun(_define_get_buffer_size, 1, {
            auto& conf = CXX::Interface::getExtractAs<TcpConfiguration>(args[0], define_TcpConfiguration);
            return conf.buffer_size;
        });
        AttachAFun(_define_get_fast_open_queue, 1, {
            auto& conf = CXX::Interface::getExtractAs<TcpConfiguration>(args[0], define_TcpConfiguration);
            return conf.fast_open_queue;
        });
        AttachAFun(_define_get_connection_timeout_ms, 1, {
            auto& conf = CXX::Interface::getExtractAs<TcpConfiguration>(args[0], define_TcpConfiguration);
            return conf.connection_timeout_ms;
        });
        AttachAFun(_define_get_allow_ip4, 1, {
            auto& conf = CXX::Interface::getExtractAs<TcpConfiguration>(args[0], define_TcpConfiguration);
            return conf.allow_ip4;
        });
        AttachAFun(_define_get_enable_delay, 1, {
            auto& conf = CXX::Interface::getExtractAs<TcpConfiguration>(args[0], define_TcpConfiguration);
            return conf.enable_delay;
        });
        AttachAFun(_define_get_enable_timestamps, 1, {
            auto& conf = CXX::Interface::getExtractAs<TcpConfiguration>(args[0], define_TcpConfiguration);
            return conf.enable_timestamps;
        });
        AttachAFun(_define_get_enable_keep_alive, 1, {
            auto& conf = CXX::Interface::getExtractAs<TcpConfiguration>(args[0], define_TcpConfiguration);
            return conf.enable_keep_alive;
        });
        AttachAFun(_define_get_keep_alive_idle_ms, 1, {
            auto& conf = CXX::Interface::getExtractAs<TcpConfiguration>(args[0], define_TcpConfiguration);
            return conf.keep_alive_settings.idle_ms;
        });
        AttachAFun(_define_get_keep_alive_interval_ms, 1, {
            auto& conf = CXX::Interface::getExtractAs<TcpConfiguration>(args[0], define_TcpConfiguration);
            return conf.keep_alive_settings.interval_ms;
        });
        AttachAFun(_define_get_keep_alive_retry_count, 1, {
            auto& conf = CXX::Interface::getExtractAs<TcpConfiguration>(args[0], define_TcpConfiguration);
            return conf.keep_alive_settings.retry_count;
        });
        AttachAFun(_define_get_keep_alive_user_timeout_ms, 1, {
            auto& conf = CXX::Interface::getExtractAs<TcpConfiguration>(args[0], define_TcpConfiguration);
            return conf.keep_alive_settings.user_timeout_ms;
        });


        AttachAFun(_define_set_recv_timeout_ms, 2, {
            auto& conf = CXX::Interface::getExtractAs<TcpConfiguration>(args[0], define_TcpConfiguration);
            conf.recv_timeout_ms = (uint32_t)args[1];
        });
        AttachAFun(_define_set_send_timeout_ms, 2, {
            auto& conf = CXX::Interface::getExtractAs<TcpConfiguration>(args[0], define_TcpConfiguration);
            conf.send_timeout_ms = (uint32_t)args[1];
        });
        AttachAFun(_define_set_buffer_size, 2, {
            auto& conf = CXX::Interface::getExtractAs<TcpConfiguration>(args[0], define_TcpConfiguration);
            conf.buffer_size = (uint32_t)args[1];
        });
        AttachAFun(_define_set_fast_open_queue, 2, {
            auto& conf = CXX::Interface::getExtractAs<TcpConfiguration>(args[0], define_TcpConfiguration);
            conf.fast_open_queue = (uint32_t)args[1];
        });
        AttachAFun(_define_set_connection_timeout_ms, 2, {
            auto& conf = CXX::Interface::getExtractAs<TcpConfiguration>(args[0], define_TcpConfiguration);
            conf.connection_timeout_ms = (uint32_t)args[1];
        });
        AttachAFun(_define_set_allow_ip4, 2, {
            auto& conf = CXX::Interface::getExtractAs<TcpConfiguration>(args[0], define_TcpConfiguration);
            conf.allow_ip4 = (bool)args[1];
        });
        AttachAFun(_define_set_enable_delay, 2, {
            auto& conf = CXX::Interface::getExtractAs<TcpConfiguration>(args[0], define_TcpConfiguration);
            conf.enable_delay = (bool)args[1];
        });
        AttachAFun(_define_set_enable_timestamps, 2, {
            auto& conf = CXX::Interface::getExtractAs<TcpConfiguration>(args[0], define_TcpConfiguration);
            conf.enable_timestamps = (bool)args[1];
        });
        AttachAFun(_define_set_enable_keep_alive, 2, {
            auto& conf = CXX::Interface::getExtractAs<TcpConfiguration>(args[0], define_TcpConfiguration);
            conf.enable_keep_alive = (bool)args[1];
        });
        AttachAFun(_define_set_keep_alive_idle_ms, 2, {
            auto& conf = CXX::Interface::getExtractAs<TcpConfiguration>(args[0], define_TcpConfiguration);
            conf.keep_alive_settings.idle_ms = (uint32_t)args[1];
        });
        AttachAFun(_define_set_keep_alive_interval_ms, 2, {
            auto& conf = CXX::Interface::getExtractAs<TcpConfiguration>(args[0], define_TcpConfiguration);
            conf.keep_alive_settings.interval_ms = (uint32_t)args[1];
        });
        AttachAFun(_define_set_keep_alive_retry_count, 2, {
            auto& conf = CXX::Interface::getExtractAs<TcpConfiguration>(args[0], define_TcpConfiguration);
            conf.keep_alive_settings.retry_count = (uint32_t)args[1];
        });
        AttachAFun(_define_set_keep_alive_user_timeout_ms, 2, {
            auto& conf = CXX::Interface::getExtractAs<TcpConfiguration>(args[0], define_TcpConfiguration);
            conf.keep_alive_settings.user_timeout_ms = (uint32_t)args[1];
        });
    }

    void init_define_UniversalAddress() {
        if (define_UniversalAddress != nullptr)
            return;
        define_UniversalAddress = CXX::Interface::createTable<universal_address>(
            "universal_address",
            CXX::Interface::direct_method("to_string", UniversalAddress::_define_to_string),
            CXX::Interface::direct_method("type", UniversalAddress::_define_type),
            CXX::Interface::direct_method("actual_type", UniversalAddress::_define_actual_type),
            CXX::Interface::direct_method("port", UniversalAddress::_define_port),
            CXX::Interface::direct_method("full_address", UniversalAddress::_define_full_address)
        );
    }

    void init_define_TcpConfiguration() {
        if (define_UniversalAddress != nullptr)
            return;
        define_UniversalAddress = CXX::Interface::createTable<TcpConfiguration>(
            "tcp_configuration",
            CXX::Interface::direct_method("get_recv_timeout_ms", TcpConfiguration_map::_define_get_recv_timeout_ms),
            CXX::Interface::direct_method("get_send_timeout_ms", TcpConfiguration_map::_define_get_send_timeout_ms),
            CXX::Interface::direct_method("get_buffer_size", TcpConfiguration_map::_define_get_buffer_size),
            CXX::Interface::direct_method("get_fast_open_queue", TcpConfiguration_map::_define_get_fast_open_queue),
            CXX::Interface::direct_method("get_connection_timeout_ms", TcpConfiguration_map::_define_get_connection_timeout_ms),
            CXX::Interface::direct_method("get_allow_ip4", TcpConfiguration_map::_define_get_allow_ip4),
            CXX::Interface::direct_method("get_enable_delay", TcpConfiguration_map::_define_get_enable_delay),
            CXX::Interface::direct_method("get_enable_timestamps", TcpConfiguration_map::_define_get_enable_timestamps),
            CXX::Interface::direct_method("get_enable_keep_alive", TcpConfiguration_map::_define_get_enable_keep_alive),
            CXX::Interface::direct_method("get_keep_alive_idle_ms", TcpConfiguration_map::_define_get_keep_alive_idle_ms),
            CXX::Interface::direct_method("get_keep_alive_interval_ms", TcpConfiguration_map::_define_get_keep_alive_interval_ms),
            CXX::Interface::direct_method("get_keep_alive_retry_count", TcpConfiguration_map::_define_get_keep_alive_retry_count),
            CXX::Interface::direct_method("get_keep_alive_user_timeout_ms", TcpConfiguration_map::_define_get_keep_alive_user_timeout_ms),
            CXX::Interface::direct_method("set_recv_timeout_ms", TcpConfiguration_map::_define_set_recv_timeout_ms),
            CXX::Interface::direct_method("set_send_timeout_ms", TcpConfiguration_map::_define_set_send_timeout_ms),
            CXX::Interface::direct_method("set_buffer_size", TcpConfiguration_map::_define_set_buffer_size),
            CXX::Interface::direct_method("set_fast_open_queue", TcpConfiguration_map::_define_set_fast_open_queue),
            CXX::Interface::direct_method("set_connection_timeout_ms", TcpConfiguration_map::_define_set_connection_timeout_ms),
            CXX::Interface::direct_method("set_allow_ip4", TcpConfiguration_map::_define_set_allow_ip4),
            CXX::Interface::direct_method("set_enable_delay", TcpConfiguration_map::_define_set_enable_delay),
            CXX::Interface::direct_method("set_enable_timestamps", TcpConfiguration_map::_define_set_enable_timestamps),
            CXX::Interface::direct_method("set_enable_keep_alive", TcpConfiguration_map::_define_set_enable_keep_alive),
            CXX::Interface::direct_method("set_keep_alive_idle_ms", TcpConfiguration_map::_define_set_keep_alive_idle_ms),
            CXX::Interface::direct_method("set_keep_alive_interval_ms", TcpConfiguration_map::_define_set_keep_alive_interval_ms),
            CXX::Interface::direct_method("set_keep_alive_retry_count", TcpConfiguration_map::_define_set_keep_alive_retry_count),
            CXX::Interface::direct_method("set_keep_alive_user_timeout_ms", TcpConfiguration_map::_define_set_keep_alive_user_timeout_ms)
        );
    }

    void internal_makeIP4(universal_address& addr_storage, const char* ip, uint16_t port) {
        memset(&addr_storage, 0, sizeof(addr_storage));
        sockaddr_in6 addr6;
        memset(&addr6, 0, sizeof(addr6));
        addr6.sin6_family = AF_INET6;
        addr6.sin6_port = htons(port);
        addr6.sin6_addr.s6_addr[10] = 0xFF;
        addr6.sin6_addr.s6_addr[11] = 0xFF;
        if (inet_pton(AF_INET, ip, &addr6.sin6_addr.s6_addr[12]) != 1)
            throw InvalidArguments("Invalid ip4 address");

        memcpy(&addr_storage, &addr6, sizeof(addr6));
    }

    void internal_makeIP6(universal_address& addr_storage, const char* ip, uint16_t port) {
        memset(&addr_storage, 0, sizeof(addr_storage));
        sockaddr_in6 addr6;
        memset(&addr6, 0, sizeof(addr6));
        addr6.sin6_family = AF_INET6;
        addr6.sin6_port = htons(port);
        if (inet_pton(AF_INET6, ip, &addr6.sin6_addr) != 1)
            throw InvalidArguments("Invalid ip6 address");

        memcpy(&addr_storage, &addr6, sizeof(addr6));
    }

    void internal_makeIP(universal_address& addr_storage, const char* ip, uint16_t port) {
        memset(&addr_storage, 0, sizeof(addr_storage));
        sockaddr_in6 addr6;
        memset(&addr6, 0, sizeof(addr6));
        addr6.sin6_family = AF_INET6;
        addr6.sin6_port = htons(port);
        addr6.sin6_addr.s6_addr[10] = 0xFF;
        addr6.sin6_addr.s6_addr[11] = 0xFF;
        if (inet_pton(AF_INET, ip, &addr6.sin6_addr + 12) == 1)
            ;
        else if (inet_pton(AF_INET6, ip, &addr6.sin6_addr) != 1)
            throw InvalidArguments("Invalid ip address");
        memcpy(&addr_storage, &addr6, sizeof(addr6));
    }

    void internal_makeIP4_port(universal_address& addr_storage, const char* ip_port) {
        const char* port = strchr(ip_port, ':');
        if (!port)
            throw InvalidArguments("Invalid ip4 address");
        uint16_t port_num = (uint16_t)std::stoi(port + 1);
        art::ustring ip(ip_port, port);
        internal_makeIP4(addr_storage, ip.c_str(), port_num);
    }

    void internal_makeIP6_port(universal_address& addr_storage, const char* ip_port) {
        if (ip_port[0] != '[')
            throw InvalidArguments("Invalid ip6:port address");
        const char* port = strchr(ip_port, ']');
        if (!port)
            throw InvalidArguments("Invalid ip6:port address");
        if (port[1] != ':')
            throw InvalidArguments("Invalid ip6:port address");
        if (port[2] == 0)
            throw InvalidArguments("Invalid ip6:port address");
        uint16_t port_num = (uint16_t)std::stoi(port + 2);


        if (ip_port == port - 1) {
            sockaddr_in6 addr6;
            memset(&addr6, 0, sizeof(addr6));
            addr6.sin6_family = AF_INET6;
            addr6.sin6_port = htons(port_num);
            memcpy(&addr_storage, &addr6, sizeof(addr6));
            return;
        }
        art::ustring ip(ip_port + 1, port);
        internal_makeIP6(addr_storage, ip.c_str(), port_num);
    }

    void internal_makeIP_port(universal_address& addr_storage, const char* ip_port) {
        if (ip_port[0] == '[')
            return internal_makeIP6_port(addr_storage, ip_port);
        else
            return internal_makeIP4_port(addr_storage, ip_port);
    }

    void get_address_from_valueItem(const ValueItem& ip_port, universal_address& addr_storage) {
        if (ip_port.meta.vtype == VType::struct_) {
            auto& address = CXX::Interface::getExtractAs<universal_address>((Structure&)ip_port, define_UniversalAddress);
            memcpy(&addr_storage, &address, sizeof(addr_storage));
        } else if (ip_port.meta.vtype == VType::string)
            internal_makeIP_port(addr_storage, ((art::ustring)ip_port).c_str());
        else
            throw InvalidArguments("excepted universal_address or string, got " + enum_to_string(ip_port.meta.vtype));
    }

    ValueItem construct_TcpConfiguration() {
        return CXX::Interface::constructStructure<TcpConfiguration>(define_TcpConfiguration);
    }
}

namespace art {
#if PLATFORM_WINDOWS
    bool inited = false;
    AttachAVirtualTable* define_TcpNetworkStream = nullptr;
    AttachAVirtualTable* define_TcpNetworkBlocking = nullptr;
    void init_define_TcpNetworkStream();
    void init_define_TcpNetworkBlocking();

    ::LPFN_ACCEPTEX _AcceptEx;
    ::LPFN_GETACCEPTEXSOCKADDRS _GetAcceptExSockaddrs;
    ::LPFN_CONNECTEX _ConnectEx;
    ::LPFN_TRANSMITFILE _TransmitFile;
    ::LPFN_DISCONNECTEX _DisconnectEx;
    ::WSADATA wsaData;

    void init_win_fns(SOCKET sock) {
        static bool inited = false;
        if (inited)
            return;
        ::GUID GuidAcceptEx = WSAID_ACCEPTEX;
        ::GUID GuidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
        ::GUID GuidConnectEx = WSAID_CONNECTEX;
        ::GUID GuidTransmitFile = WSAID_TRANSMITFILE;
        ::GUID GuidDisconnectEx = WSAID_DISCONNECTEX;
        ::DWORD dwBytes = 0;

        if (SOCKET_ERROR == ::WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidAcceptEx, sizeof(GuidAcceptEx), &_AcceptEx, sizeof(_AcceptEx), &dwBytes, NULL, NULL))
            throw std::runtime_error("WSAIoctl failed get AcceptEx");
        if (SOCKET_ERROR == ::WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidGetAcceptExSockaddrs, sizeof(GuidGetAcceptExSockaddrs), &_GetAcceptExSockaddrs, sizeof(_GetAcceptExSockaddrs), &dwBytes, NULL, NULL))
            throw std::runtime_error("WSAIoctl failed get GetAcceptExSockaddrs");
        if (SOCKET_ERROR == ::WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidConnectEx, sizeof(GuidConnectEx), &_ConnectEx, sizeof(_ConnectEx), &dwBytes, NULL, NULL))
            throw std::runtime_error("WSAIoctl failed get ConnectEx");
        if (SOCKET_ERROR == ::WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidTransmitFile, sizeof(GuidTransmitFile), &_TransmitFile, sizeof(_TransmitFile), &dwBytes, NULL, NULL))
            throw std::runtime_error("WSAIoctl failed get TransmitFile");
        if (SOCKET_ERROR == ::WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidDisconnectEx, sizeof(GuidDisconnectEx), &_DisconnectEx, sizeof(_DisconnectEx), &dwBytes, NULL, NULL))
            throw std::runtime_error("WSAIoctl failed get DisconnectEx");


        inited = true;
    }

#pragma region TCP

    struct tcp_handle : public NativeWorkerHandle {
        std::list<std::tuple<char*, size_t>> write_queue;
        std::list<std::tuple<char*, size_t>> read_queue;
        TaskConditionVariable cv;
        TaskMutex cv_mutex;
        ::SOCKET socket;
        ::WSABUF buffer;
        char* data;
        int total_bytes;
        int sent_bytes;
        int readed_bytes;
        int data_len;
        bool force_mode;
        bool is_bound = false;
        uint32_t max_read_queue_size;
        TcpError invalid_reason = TcpError::none;
        enum class Opcode : uint8_t {
            ACCEPT,
            READ,
            WRITE,
            TRANSMIT_FILE,
            INTERNAL_READ,
            INTERNAL_CLOSE
        } opcode = Opcode::ACCEPT;

        tcp_handle(SOCKET socket, int32_t buffer_len, NativeWorkerManager* manager, uint32_t read_queue_size = 10)
            : socket(socket), NativeWorkerHandle(manager), max_read_queue_size(read_queue_size) {
            if (buffer_len < 0)
                throw InvalidArguments("buffer_len must be positive");
            SecureZeroMemory(&overlapped, sizeof(OVERLAPPED));
            data = new char[buffer_len];
            buffer.buf = data;
            buffer.len = buffer_len;
            data_len = buffer_len;
            total_bytes = 0;
            sent_bytes = 0;
            readed_bytes = 0;
            force_mode = false;
        }

        ~tcp_handle() {
            close();
        }

        uint32_t available_bytes() {
            if (!data)
                return 0;
            if (readed_bytes)
                return true;
            DWORD value = 0;
            int result = ::ioctlsocket(socket, FIONREAD, &value);
            if (result == SOCKET_ERROR)
                return 0;
            else
                return value;
        }

        bool data_available() {
            return available_bytes() > 0;
        }

        void send_data(const char* data, int len) {
            if (!data)
                return;
            char* new_data = new char[len];
            memcpy(new_data, data, len);
            write_queue.push_back(std::make_tuple(new_data, len));
        }

        //async
        bool send_queue_item() {
            if (!data)
                return false;
            if (write_queue.empty())
                return false;
            auto item = write_queue.front();
            write_queue.pop_front();
            auto& send_data = std::get<0>(item);
            auto& val_len = std::get<1>(item);
            std::unique_ptr<char[]> send_data_ptr(send_data);
            //set buffer
            buffer.len = data_len;
            buffer.buf = data;
            while (val_len) {
                size_t to_sent_bytes = val_len > data_len ? data_len : val_len;
                memcpy(data, send_data, to_sent_bytes);
                buffer.len = to_sent_bytes;
                buffer.buf = data;
                if (!send_await()) {
                    return false;
                }
                if (val_len < sent_bytes)
                    return true;
                val_len -= sent_bytes;
                send_data += sent_bytes;
            }
            return true;
        }

        void read_force(uint32_t buffer_len, char* buffer) {
            if (!data)
                return;
            if (!buffer_len)
                return;
            if (!buffer)
                return;
            while (buffer_len) {
                int readed = 0;
                read_available(buffer, buffer_len, readed);
                buffer += readed;
                if (readed > buffer_len)
                    return;
                buffer_len -= readed;
            }
        }

        int64_t write_force(const char* to_write, uint32_t to_write_len) {
            if (!data)
                return -1;
            if (!to_write_len)
                return -1;
            if (!to_write)
                return -1;

            force_mode = true;
            if (data_len < to_write_len) {
                buffer.len = data_len;
                buffer.buf = this->data;
                if (!send_await())
                    return -1;
                force_mode = false;
                return sent_bytes;
            } else {
                buffer.len = to_write_len;
                buffer.buf = this->data;
                memcpy(this->data, to_write, to_write_len);
                if (!send_await())
                    return -1;
                force_mode = false;
                return sent_bytes;
            }
        }

        void read_data() {
            if (!data)
                return;
            MutexUnify mutex(cv_mutex);
            art::unique_lock<MutexUnify> lock(mutex);
            opcode = Opcode::READ;
            read();
            cv.wait(lock);
        }

        void read_available_no_block(char* extern_buffer, int buffer_len, int& readed) {
            if (!readed_bytes)
                readed = 0;
            else if (readed_bytes < buffer_len) {
                readed = readed_bytes;
                memcpy(extern_buffer, data, readed_bytes);
                readed_bytes = 0;
            } else {
                readed = buffer_len;
                memcpy(extern_buffer, buffer.buf, buffer_len);
                readed_bytes -= buffer_len;
                buffer.buf += buffer_len;
                buffer.len -= buffer_len;
            }
        }

        void read_available(char* extern_buffer, int buffer_len, int& readed) {
            if (!readed_bytes) {
                if (read_queue.empty())
                    read_data();
                else {
                    auto item = read_queue.front();
                    read_queue.pop_front();
                    auto& read_data = std::get<0>(item);
                    auto& val_len = std::get<1>(item);
                    std::unique_ptr<char[]> read_data_ptr(read_data);
                    buffer.buf = data;
                    buffer.len = data_len;
                    readed_bytes = val_len;
                    memcpy(data, read_data, val_len);
                }
            }
            if (readed_bytes < buffer_len) {
                readed = readed_bytes;
                memcpy(extern_buffer, data, readed_bytes);
                readed_bytes = 0;
            } else {
                readed = buffer_len;
                memcpy(extern_buffer, buffer.buf, buffer_len);
                readed_bytes -= buffer_len;
                buffer.buf += buffer_len;
                buffer.len -= buffer_len;
            }
        }

        char* read_available_no_copy(int& readed) {
            if (!readed_bytes)
                read_data();
            readed = readed_bytes;
            readed_bytes = 0;
            return data;
        }

        void close(TcpError err = TcpError::local_close) {
            if (!data)
                return;
            pre_close(err);
            internal_close();
        }

        void handle(unsigned long dwBytesTransferred) {
            DWORD flags = 0, bytes = 0;
            if (!data) {
                if (opcode != Opcode::INTERNAL_CLOSE)
                    return;
            }
            MutexUnify mutex(cv_mutex);
            art::unique_lock<MutexUnify> lock(mutex);
            switch (opcode) {
            case Opcode::READ: {
                readed_bytes = dwBytesTransferred;
                cv.notify_all();
                break;
            }
            case Opcode::WRITE:
                sent_bytes += dwBytesTransferred;
                if (sent_bytes < total_bytes) {
                    buffer.buf = data + sent_bytes;
                    buffer.len = total_bytes - sent_bytes;
                    if (!data_available()) {
                        if (!send())
                            cv.notify_all();
                    } else {
                        char* data = new char[buffer.len];
                        memcpy(data, buffer.buf, buffer.len);
                        write_queue.push_front(std::make_tuple(data, buffer.len));
                        if (force_mode) {
                            opcode = Opcode::INTERNAL_READ;
                            read();
                        } else
                            cv.notify_all();
                    }
                } else
                    cv.notify_all();
                break;
            case Opcode::TRANSMIT_FILE:
                cv.notify_all();
                break;
            case Opcode::INTERNAL_READ:
                if (dwBytesTransferred) {
                    char* buffer = new char[dwBytesTransferred];
                    memcpy(buffer, data, dwBytesTransferred);
                    read_queue.push_back(std::make_tuple(buffer, dwBytesTransferred));
                }
                if (!data_available()) {
                    if (read_queue.size() > max_read_queue_size)
                        close(TcpError::read_queue_overflow);
                    else
                        read();
                } else {
                    if (write_queue.empty())
                        close(TcpError::invalid_state);
                    else {
                        auto item = write_queue.front();
                        write_queue.pop_front();
                        auto& write_data = std::get<0>(item);
                        auto& val_len = std::get<1>(item);
                        memcpy(data, write_data, val_len);
                        delete[] write_data;
                        buffer.buf = data;
                        buffer.len = val_len;
                        if (!send())
                            cv.notify_all();
                    }
                }
                break;
            case Opcode::INTERNAL_CLOSE:
                closesocket(socket);
                socket = INVALID_SOCKET;
                cv.notify_all();
                break;
            default:
                break;
            }
        }

        void send_and_close(const char* data, int len) {
            if (!data)
                return;
            buffer.len = data_len;
            buffer.buf = this->data;
            write_queue = {};
            force_mode = true;
            while (data_len < len) {
                memcpy(buffer.buf, data, buffer.len);
                if (!send_await())
                    return;
                data += buffer.len;
                len -= buffer.len;
            }
            if (len) {
                //send last part of data and close
                memcpy(buffer.buf, data, len);
                buffer.len = len;
                send_await();
            }
            force_mode = false;
            close();
        }

        bool send_file(void* file, uint64_t data_len, uint64_t offset, uint32_t chunks_size) {
            if (!data)
                return false;
            if (chunks_size == 0)
                chunks_size = 0x1000;
            if (data_len == 0) {
                LARGE_INTEGER file_size;
                if (!GetFileSizeEx(file, &file_size))
                    return false;
                data_len = file_size.QuadPart;
                if (offset > data_len)
                    return false;
                data_len -= offset;
            }

            if (data_len > 0x7FFFFFFE) {
                //send file in chunks using TransmitFile
                uint64_t sended = 0;
                uint64_t blocks = data_len / 0x7FFFFFFE;
                uint64_t last_block = data_len % blocks;

                while (blocks--)
                    if (!transfer_file(socket, file, 0x7FFFFFFE, chunks_size, sended + offset))
                        return false;
                    else
                        sended += 0x7FFFFFFE;


                if (last_block)
                    if (!transfer_file(socket, file, last_block, chunks_size, sended + offset))
                        return false;
            } else {
                if (!transfer_file(socket, file, data_len, chunks_size, offset))
                    return false;
            }
            return true;
        }

        bool send_file(const char* path, size_t path_len, uint64_t data_len, uint64_t offset, uint32_t chunks_size) {
            if (!data)
                return false;
            std::u16string wpath;
            utf8::utf8to16(path, path + path_len, std::back_inserter(wpath));
            HANDLE file = CreateFileW((wchar_t*)wpath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
            if (!file)
                return false;
            bool result;
            try {
                result = send_file(file, data_len, offset, chunks_size);
            } catch (...) {
                CloseHandle(file);
                throw;
            }
            CloseHandle(file);
            return result;
        }

        bool valid() {
            return data != nullptr;
        }

        void reset() {
            if (!data)
                return;
            pre_close(TcpError::local_reset);
            closesocket(socket); //with iocp socket not send everything and cancel all operations
        }

        void connection_reset() {
            MutexUnify mutex(cv_mutex);
            art::unique_lock<MutexUnify> lock(mutex);
            data = nullptr;
            invalid_reason = TcpError::remote_close;
            readed_bytes = 0;
            cv.notify_all();
            internal_close();
        }

        void rebuffer(int32_t buffer_len) {
            if (!data)
                return;
            if (buffer_len < 0)
                throw InvalidArguments("buffer_len must be positive");
            if (buffer_len == 0)
                buffer_len = 0x1000;
            if (buffer_len == data_len)
                return;
            char* new_data = new char[buffer_len];
            delete[] data;
            data = new_data;
            data_len = buffer_len;
        }

    private:
        void pre_close(TcpError err) {
            MutexUnify mutex(cv_mutex);
            art::unique_lock<MutexUnify> lock(mutex);
            std::list<std::tuple<char*, size_t>> clear_write_queue;
            std::list<std::tuple<char*, size_t>> clear_read_queue;
            readed_bytes = 0;
            sent_bytes = 0;
            delete[] data;
            data = nullptr;
            invalid_reason = err;
            write_queue.swap(clear_write_queue);
            read_queue.swap(clear_read_queue);
            cv.notify_all();

            lock.unlock();
            for (auto& item : clear_write_queue)
                delete[] std::get<0>(item);
            for (auto& item : clear_read_queue)
                delete[] std::get<0>(item);
        }

        void internal_close() {
            MutexUnify mutex(cv_mutex);
            art::unique_lock<MutexUnify> lock(mutex);
            opcode = Opcode::INTERNAL_CLOSE;
            if (!_DisconnectEx(socket, nullptr, TF_REUSE_SOCKET, 0)) {
                if (WSAGetLastError() != ERROR_IO_PENDING)
                    invalid_reason = TcpError::local_close;
                cv.wait(lock);
            } else
                closesocket(socket);
        }

        bool handle_error() {
            auto error = WSAGetLastError();
            if (WSA_IO_PENDING == error)
                return true;
            else {
                switch (error) {
                case WSAECONNRESET:
                    invalid_reason = TcpError::remote_close;
                    break;
                case WSAECONNABORTED:
                case WSA_OPERATION_ABORTED:
                case WSAENETRESET:
                    invalid_reason = TcpError::local_close;
                    break;
                case WSAEWOULDBLOCK:
                    return false; //try later
                default:
                    invalid_reason = TcpError::undefined_error;
                    break;
                }
                close();
                return false;
            }
        }

        void read() {
            DWORD flags = 0;
            buffer.buf = this->data;
            buffer.len = data_len;
            int result = WSARecv(socket, &buffer, 1, NULL, &flags, &overlapped, NULL);
            if ((SOCKET_ERROR == result)) {
                handle_error();
                return;
            }
        }

        bool send() {
            DWORD flags = 0;
            opcode = Opcode::WRITE;
            int result = WSASend(socket, &buffer, 1, NULL, flags, &overlapped, NULL);
            if ((SOCKET_ERROR == result))
                return handle_error();
            return true;
        }

        bool send_await() {
            MutexUnify mutex(cv_mutex);
            art::unique_lock<MutexUnify> lock(mutex);
            if (!send())
                return false;
            cv.wait(lock);
            return data; //if data is null, then socket is closed
        }

        bool transfer_file(SOCKET sock, HANDLE FILE, uint32_t block, uint32_t chunks_size, uint64_t offset) {
            MutexUnify mutex(cv_mutex);
            art::unique_lock<MutexUnify> lock(mutex);
            overlapped.Offset = offset & 0xFFFFFFFF;
            overlapped.OffsetHigh = offset >> 32;
            opcode = Opcode::TRANSMIT_FILE;
            bool res = _TransmitFile(sock, FILE, block, chunks_size, &overlapped, NULL, TF_USE_KERNEL_APC);
            if (!res && WSAGetLastError() != WSA_IO_PENDING)
                res = false;
            cv.wait(lock);
            return res;
        }
    };

#pragma region TcpNetworkStream

    class TcpNetworkStreamImpl : public TcpNetworkStream {
        friend class TcpNetworkManager;
        struct tcp_handle* handle;
        TaskMutex mutex;
        TcpError last_error;

        bool checkup() {
            if (!handle)
                return false;
            if (!handle->valid()) {
                last_error = handle->invalid_reason;
                delete handle;
                handle = nullptr;
                return false;
            }
            return true;
        }

    public:
        TcpNetworkStreamImpl(tcp_handle* handle)
            : handle(handle), last_error(TcpError::none) {}

        ~TcpNetworkStreamImpl() {
            if (handle) {
                std::lock_guard lg(mutex);
                handle->close();
                delete handle;
            }
            handle = nullptr;
        }

        ValueItem read_available_ref() override {
            std::lock_guard lg(mutex);
            if (!handle)
                return nullptr;
            while (!handle->data_available()) {
                if (!handle->send_queue_item())
                    break;
            }
            if (!checkup())
                return ValueItem(nullptr, ValueMeta(VType::raw_arr_ui8, false, false, 0), as_reference);
            int readed = 0;
            char* data = handle->read_available_no_copy(readed);
            return ValueItem(data, ValueMeta(VType::raw_arr_ui8, false, false, readed), as_reference);
        }

        ValueItem read_available(char* buffer, int buffer_len) override {
            std::lock_guard lg(mutex);
            if (!handle)
                return nullptr;
            while (!handle->data_available()) {
                if (!handle->send_queue_item())
                    break;
            }

            if (!checkup())
                return (uint32_t)0;
            int readed = 0;
            handle->read_available(buffer, buffer_len, readed);
            return ValueItem((uint32_t)readed);
        }

        bool data_available() override {
            std::lock_guard lg(mutex);
            if (handle)
                return handle->data_available();
            return false;
        }

        void write(const char* data, size_t size) override {
            std::lock_guard lg(mutex);
            if (handle) {
                handle->send_data(data, size);
                while (!handle->data_available()) {
                    if (!handle->send_queue_item())
                        break;
                }
                checkup();
            }
        }

        bool write_file(char* path, size_t path_len, uint64_t data_len, uint64_t offset, uint32_t chunks_size) override{
            std::lock_guard lg(mutex);
            if (handle) {
                while (handle->valid())
                    if (!handle->send_queue_item())
                        break;

                if (!checkup())
                    return false;

                return handle->send_file(path, path_len, data_len, offset, chunks_size);
            }
            return false;
        }

        bool write_file(void* fhandle, uint64_t data_len, uint64_t offset, uint32_t chunks_size) override {
            std::lock_guard lg(mutex);
            if (handle) {
                while (handle->valid())
                    if (!handle->send_queue_item())
                        break;
                if (!checkup())
                    return false;
                return handle->send_file(fhandle, data_len, offset, chunks_size);
            }
            return false;
        }

        //write all data from write_queue
        void force_write()override {
            std::lock_guard lg(mutex);
            if (handle) {
                while (handle->valid())
                    if (!handle->send_queue_item())
                        break;
                checkup();
            }
        }

        void force_write_and_close(const char* data, size_t size) override {
            std::lock_guard lg(mutex);
            if (handle) {
                handle->send_and_close(data, size);
                last_error = handle->invalid_reason;
                delete handle;
            }
            handle = nullptr;
        }

        void close() override {
            std::lock_guard lg(mutex);
            if (handle) {
                handle->close();
                last_error = handle->invalid_reason;
                delete handle;
            }
            handle = nullptr;
        }

        void reset() override {
            std::lock_guard lg(mutex);
            if (handle) {
                handle->reset();
                last_error = handle->invalid_reason;
                delete handle;
            }
            handle = nullptr;
        }

        void rebuffer(int32_t new_size) override {
            std::lock_guard lg(mutex);
            if (handle)
                handle->rebuffer(new_size);
        }

        bool is_closed() override {
            std::lock_guard lg(mutex);
            if (handle) {
                bool res = handle->valid();
                if (!res) {
                    delete handle;
                    handle = nullptr;
                }
                return !res;
            }
            return true;
        }

        TcpError error() override {
            std::lock_guard lg(mutex);
            if (handle)
                return handle->invalid_reason;
            return last_error;
        }

        ValueItem local_address() override {
            std::lock_guard lg(mutex);
            if (!handle)
                return nullptr;
            universal_address addr;
            int socklen = sizeof(universal_address);
            if (getsockname(handle->socket, (sockaddr*)&addr, &socklen) == -1)
                return nullptr;
            return CXX::Interface::constructStructure<universal_address>(define_UniversalAddress, addr);
        }

        ValueItem remote_address() override {
            std::lock_guard lg(mutex);
            if (!handle)
                return nullptr;
            universal_address addr;
            int socklen = sizeof(universal_address);
            if (getpeername(handle->socket, (sockaddr*)&addr, &socklen) == -1)
                return nullptr;
            return CXX::Interface::constructStructure<universal_address>(define_UniversalAddress, addr);
        }
    };

    AttachAFun(funs_TcpNetworkStream_read_available_ref, 1, {
        return CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream).read_available_ref();
    });
    AttachAFun(funs_TcpNetworkStream_read_available, 2, {
        auto& stream = CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream);
        if (args[1].meta.vtype != VType::raw_arr_ui8 && args[1].meta.vtype != VType::raw_arr_i8)
            throw InvalidArguments("The second argument must be a raw_arr_ui8.");
        return stream.read_available((char*)args[1].val, args[1].meta.val_len);
    });
    AttachAFun(funs_TcpNetworkStream_data_available, 1, {
        return CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream).data_available();
    });
    AttachAFun(funs_TcpNetworkStream_write, 2, {
        auto& stream = CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream);
        if (args[1].meta.vtype != VType::raw_arr_ui8 && args[1].meta.vtype != VType::raw_arr_i8)
            throw InvalidArguments("The second argument must be a raw_arr_ui8.");
        stream.write((char*)args[1].val, args[1].meta.val_len);
    });
    AttachAFun(funs_TcpNetworkStream_write_file, 2, {
        auto& stream = CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream);
        ValueItem& arg1 = args[1];
        uint64_t data_len = 0;
        uint64_t offset = 0;
        uint32_t chunks_size = 0;
        if (arg1.meta.vtype != VType::struct_ && arg1.meta.vtype != VType::string)
            throw InvalidArguments("The second argument must be a file handle or a file path.");
        if (len >= 3)
            data_len = (uint64_t)args[2];
        if (len >= 4)
            offset = (uint64_t)args[3];
        if (len >= 5)
            chunks_size = (uint32_t)args[4];
        if (arg1.meta.vtype == VType::struct_) {
            auto& proxy = (Structure&)args[1];
            if (proxy.get_vtable()) {
                if (proxy.get_name() == "file_handle") {
                    return stream.write_file((*(typed_lgr<art::files::FileHandle>*)proxy.get_data_no_vtable())->internal_get_handle(), data_len, offset, chunks_size);
                } else if (proxy.get_name() == "blocking_file_handle")
                    return stream.write_file((*(typed_lgr<art::files::BlockingFileHandle>*)proxy.get_data_no_vtable())->internal_get_handle(), data_len, offset, chunks_size);
            }
            throw InvalidArguments("The second argument must be a file handle or a file path.");
        } else {
            art::ustring& path = *(art::ustring*)arg1.getSourcePtr();
            stream.write_file(path.data(), path.size(), data_len, offset, chunks_size);
        }
    });
    AttachAFun(funs_TcpNetworkStream_force_write, 1, {
        CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream).force_write();
    });
    AttachAFun(funs_TcpNetworkStream_force_write_and_close, 2, {
        auto& stream = CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream);
        if (args[1].meta.vtype != VType::raw_arr_ui8 && args[1].meta.vtype != VType::raw_arr_i8)
            throw InvalidArguments("The second argument must be a raw_arr_ui8.");
        stream.force_write_and_close((char*)args[1].getSourcePtr(), args[1].meta.val_len);
    });
    AttachAFun(funs_TcpNetworkStream_close, 1, {
        CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream).close();
    });
    AttachAFun(funs_TcpNetworkStream_reset, 1, {
        CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream).reset();
    });
    AttachAFun(funs_TcpNetworkStream_rebuffer, 2, {
        CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream).rebuffer((int32_t)args[1]);
    });
    AttachAFun(funs_TcpNetworkStream_is_closed, 1, {
        return CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream).is_closed();
    });
    AttachAFun(funs_TcpNetworkStream_error, 1, {
        return (uint8_t)CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream).error();
    });
    AttachAFun(funs_TcpNetworkStream_local_address, 1, {
        return CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream).local_address();
    });
    AttachAFun(funs_TcpNetworkStream_remote_address, 1, {
        return CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream).remote_address();
    });

    void init_define_TcpNetworkStream() {
        if (define_TcpNetworkStream != nullptr)
            return;
        define_TcpNetworkStream = CXX::Interface::createTable<TcpNetworkStreamImpl>(
            "tcp_network_stream",
            CXX::Interface::direct_method("read_available_ref", funs_TcpNetworkStream_read_available_ref),
            CXX::Interface::direct_method("read_available", funs_TcpNetworkStream_read_available),
            CXX::Interface::direct_method("data_available", funs_TcpNetworkStream_data_available),
            CXX::Interface::direct_method("write", funs_TcpNetworkStream_write),
            CXX::Interface::direct_method("write_file", funs_TcpNetworkStream_write_file),
            CXX::Interface::direct_method("force_write", funs_TcpNetworkStream_force_write),
            CXX::Interface::direct_method("force_write_and_close", funs_TcpNetworkStream_force_write_and_close),
            CXX::Interface::direct_method("close", funs_TcpNetworkStream_close),
            CXX::Interface::direct_method("reset", funs_TcpNetworkStream_reset),
            CXX::Interface::direct_method("rebuffer", funs_TcpNetworkStream_rebuffer),
            CXX::Interface::direct_method("is_closed", funs_TcpNetworkStream_is_closed),
            CXX::Interface::direct_method("error", funs_TcpNetworkStream_error),
            CXX::Interface::direct_method("local_address", funs_TcpNetworkStream_local_address),
            CXX::Interface::direct_method("remote_address", funs_TcpNetworkStream_remote_address)
        );
        CXX::Interface::typeVTable<TcpNetworkStream>() = define_TcpNetworkStream;
    }

#pragma endregion

#pragma region TcpNetworkBlocking

    class TcpNetworkBlockingImpl : public TcpNetworkBlocking {
        friend class TcpNetworkManager;
        tcp_handle* handle;
        TaskMutex mutex;
        TcpError last_error;

        bool checkup() {
            if (!handle)
                return false;
            if (!handle->valid()) {
                last_error = handle->invalid_reason;
                delete handle;
                handle = nullptr;
                return false;
            }
            return true;
        }

    public:
        TcpNetworkBlockingImpl(tcp_handle* handle)
            : handle(handle), last_error(TcpError::none) {}

        ~TcpNetworkBlockingImpl() {
            std::lock_guard lg(mutex);
            if (handle)
                delete handle;
            handle = nullptr;
        }

        ValueItem read(uint32_t len) override {
            std::lock_guard lg(mutex);
            if (handle) {
                if (!checkup())
                    return nullptr;
                char* buf = new char[len];
                handle->read_force(len, buf);
                if (len == 0) {
                    delete[] buf;
                    return nullptr;
                }
                return ValueItem((uint8_t*)buf, len, no_copy);
            }
            return nullptr;
        }

        ValueItem available_bytes() override {
            std::lock_guard lg(mutex);
            if (handle)
                return handle->available_bytes();
            return 0ui32;
        }

        ValueItem write(const char* data, uint32_t len) override {
            std::lock_guard lg(mutex);
            if (handle) {
                if (!checkup())
                    return nullptr;
                return handle->write_force(data, len);
            }
            return nullptr;
        }

        ValueItem write_file(char* path, size_t len, uint64_t data_len, uint64_t offset, uint32_t block_size) override {
            std::lock_guard lg(mutex);
            if (handle) {
                if (!checkup())
                    return nullptr;
                return handle->send_file(path, len, data_len, offset, block_size);
            }
            return nullptr;
        }

        ValueItem write_file(void* fhandle, uint64_t data_len, uint64_t offset, uint32_t block_size) override {
            std::lock_guard lg(mutex);
            if (handle) {
                if (!checkup())
                    return nullptr;
                return handle->send_file(fhandle, data_len, offset, block_size);
            }
            return nullptr;
        }

        void close() override {
            std::lock_guard lg(mutex);
            if (handle) {
                handle->close();
                last_error = handle->invalid_reason;
                delete handle;
                handle = nullptr;
            }
        }

        void reset() override {
            std::lock_guard lg(mutex);
            if (handle) {
                handle->reset();
                last_error = handle->invalid_reason;
                delete handle;
                handle = nullptr;
            }
        }

        void rebuffer(size_t new_size) override {
            std::lock_guard lg(mutex);
            if (handle)
                handle->rebuffer(new_size);
        }

        bool is_closed() override {
            std::lock_guard lg(mutex);
            if (handle) {
                bool res = handle->valid();
                if (!res) {
                    last_error = handle->invalid_reason;
                    delete handle;
                    handle = nullptr;
                }
                return !res;
            }
            return true;
        }

        TcpError error() override{
            std::lock_guard lg(mutex);
            if (handle)
                return handle->invalid_reason;
            return last_error;
        }

        ValueItem local_address()override {
            std::lock_guard lg(mutex);
            if (!handle)
                return nullptr;
            universal_address addr;
            int socklen = sizeof(universal_address);
            if (getsockname(handle->socket, (sockaddr*)&addr, &socklen) == -1)
                return nullptr;
            return CXX::Interface::constructStructure<universal_address>(define_UniversalAddress, addr);
        }

        ValueItem remote_address() override {
            std::lock_guard lg(mutex);
            if (!handle)
                return nullptr;
            universal_address addr;
            int socklen = sizeof(universal_address);
            if (getpeername(handle->socket, (sockaddr*)&addr, &socklen) == -1)
                return nullptr;
            return CXX::Interface::constructStructure<universal_address>(define_UniversalAddress, addr);
        }
    };

    AttachAFun(funs_TcpNetworkBlocking_read, 2, {
        return CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).read((uint32_t)args[1]);
    });
    AttachAFun(funs_TcpNetworkBlocking_available_bytes, 1, {
        return CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).available_bytes();
    });
    AttachAFun(funs_TcpNetworkBlocking_write, 2, {
        if (args[1].meta.vtype != VType::raw_arr_ui8)
            throw InvalidArguments("The third argument must be a raw_arr_ui8.");
        if (len == 2)
            return CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).write((char*)args[1].getSourcePtr(), args[1].meta.val_len);
        else {
            uint32_t len = (uint32_t)args[2];
            if (len > args[1].meta.val_len)
                throw OutOfRange("The length of the data to be sent is greater than the length of array.");
            return CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).write((char*)args[1].getSourcePtr(), len);
        }
    });
    AttachAFun(funs_TcpNetworkBlocking_write_file, 2, {
        ValueItem& arg1 = args[1];
        uint64_t data_len = 0;
        uint64_t offset = 0;
        uint32_t chunks_size = 0;
        if (arg1.meta.vtype != VType::struct_ && arg1.meta.vtype != VType::string)
            throw InvalidArguments("The second argument must be a file handle or a file path.");
        if (len >= 3)
            data_len = (uint64_t)args[2];
        if (len >= 4)
            offset = (uint64_t)args[3];
        if (len >= 5)
            chunks_size = (uint32_t)args[4];

        if (arg1.meta.vtype == VType::struct_) {
            auto& proxy = (Structure&)args[1];
            if (proxy.get_vtable()) {
                if (proxy.get_vtable() == CXX::Interface::typeVTable<typed_lgr<art::files::FileHandle>>())
                    return CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).write_file((*(typed_lgr<art::files::FileHandle>*)proxy.get_data_no_vtable())->internal_get_handle(), data_len, offset, chunks_size);
                else if (proxy.get_vtable() == CXX::Interface::typeVTable<typed_lgr<art::files::BlockingFileHandle>>())
                    return CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).write_file((*(typed_lgr<art::files::BlockingFileHandle>*)proxy.get_data_no_vtable())->internal_get_handle(), data_len, offset, chunks_size);
            }
            throw InvalidArguments("The second argument must be a file handle or a file path.");
        } else {
            art::ustring& path = *((art::ustring*)arg1.getSourcePtr());
            return CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).write_file(path.data(), path.size(), data_len, offset, chunks_size);
        }
    });
    AttachAFun(funs_TcpNetworkBlocking_close, 1, {
        CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).close();
    });
    AttachAFun(funs_TcpNetworkBlocking_reset, 1, {
        CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).reset();
    });
    AttachAFun(funs_TcpNetworkBlocking_rebuffer, 2, {
        CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).rebuffer((int32_t)args[1]);
    });
    AttachAFun(funs_TcpNetworkBlocking_is_closed, 1, {
        return CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).is_closed();
    });
    AttachAFun(funs_TcpNetworkBlocking_error, 1, {
        return (uint8_t)CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).error();
    });

    AttachAFun(funs_TcpNetworkBlocking_local_address, 1, {
        return CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).local_address();
    });
    AttachAFun(funs_TcpNetworkBlocking_remote_address, 1, {
        return CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).remote_address();
    });

    void init_define_TcpNetworkBlocking() {
        if (define_TcpNetworkBlocking != nullptr)
            return;
        define_TcpNetworkBlocking = CXX::Interface::createTable<TcpNetworkBlockingImpl>(
            "tcp_network_blocking",
            CXX::Interface::direct_method("read", funs_TcpNetworkBlocking_read),
            CXX::Interface::direct_method("available_bytes", funs_TcpNetworkBlocking_available_bytes),
            CXX::Interface::direct_method("write", funs_TcpNetworkBlocking_write),
            CXX::Interface::direct_method("write_file", funs_TcpNetworkBlocking_write_file),
            CXX::Interface::direct_method("close", funs_TcpNetworkBlocking_close),
            CXX::Interface::direct_method("reset", funs_TcpNetworkBlocking_reset),
            CXX::Interface::direct_method("rebuffer", funs_TcpNetworkBlocking_rebuffer),
            CXX::Interface::direct_method("is_closed", funs_TcpNetworkBlocking_is_closed),
            CXX::Interface::direct_method("error", funs_TcpNetworkBlocking_error),
            CXX::Interface::direct_method("local_address", funs_TcpNetworkBlocking_local_address),
            CXX::Interface::direct_method("remote_address", funs_TcpNetworkBlocking_remote_address)
        );
        CXX::Interface::typeVTable<TcpNetworkBlocking>() = define_TcpNetworkBlocking;
    }

#pragma endregion

    class TcpNetworkManager : public NativeWorkerManager {
        TaskMutex safety;
        art::shared_ptr<FuncEnvironment> handler_fn;
        art::shared_ptr<FuncEnvironment> accept_filter;
        sockaddr_in6 connectionAddress;
        SOCKET main_socket;
        int timeout_ms;

    public:
        TcpConfiguration config;

    private:
        bool allow_new_connections = false;
        bool disabled = true;
        bool corrupted = false;
        size_t acceptors;
        TcpNetworkServer::ManageType manage_type;
        TaskConditionVariable state_changed_cv;

        void make_acceptEx(tcp_handle* pClientContext) {
        re_try:
            static const auto address_len = sizeof(sockaddr_storage) + 16;
            auto new_sock = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
            pClientContext->socket = new_sock;
            pClientContext->opcode = tcp_handle::Opcode::ACCEPT;
            BOOL success = _AcceptEx(
                main_socket,
                new_sock,
                pClientContext->buffer.buf,
                0,
                address_len,
                address_len,
                nullptr,
                &pClientContext->overlapped);
            if (success != TRUE) {
                auto err = WSAGetLastError();
                if (err == WSA_IO_PENDING)
                    return;
                else if (err == WSAECONNRESET) {
                    closesocket(new_sock);
                    goto re_try;
                } else {
                    closesocket(new_sock);
                    return;
                }
            }
        }

        void make_acceptEx(void) {
            tcp_handle* pClientContext = new tcp_handle(0, config.buffer_size, this);
            make_acceptEx(pClientContext);
        }

        ValueItem accept_manager_construct(tcp_handle* self) {
            switch (manage_type) {
            case TcpNetworkServer::ManageType::blocking:
                return ValueItem(CXX::Interface::constructStructure<TcpNetworkBlockingImpl>(define_TcpNetworkBlocking, self), no_copy);
            case TcpNetworkServer::ManageType::write_delayed:
                return ValueItem(CXX::Interface::constructStructure<TcpNetworkStreamImpl>(define_TcpNetworkStream, self), no_copy);
            default:
                return nullptr;
            }
        }

        void accepted(tcp_handle* self, ValueItem clientAddr, ValueItem localAddr) {
            if (!allow_new_connections) {
                delete self;
                return;
            }
            std::lock_guard guard(safety);
            Task::start(new Task(handler_fn, ValueItem{accept_manager_construct(self), std::move(clientAddr), std::move(localAddr)}));
        }

        void new_connection(tcp_handle& data, bool good) {
            if (!data.is_bound)
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
            ValueItem clientAddress(CXX::Interface::constructStructure<universal_address>(define_UniversalAddress, *pClientAddr), no_copy);
            ValueItem localAddress(CXX::Interface::constructStructure<universal_address>(define_UniversalAddress, *pLocalAddr), no_copy);
            if (accept_filter) {
                if (CXX::cxxCall(accept_filter, clientAddress, localAddress)) {
                    closesocket(data.socket);
#ifndef DISABLE_RUNTIME_INFO
                    auto tmp = UniversalAddress::_define_to_string(&clientAddress, 1);
                    ValueItem notify{"Client: " + (art::ustring)*tmp + " not accepted due filter"};
                    delete tmp;
                    info.async_notify(notify);
#endif
                    if (!data.is_bound)
                        delete &data;
                    else
                        make_acceptEx(&data);
                    return;
                }
            }

#ifndef DISABLE_RUNTIME_INFO
            {
                auto tmp = UniversalAddress::_define_to_string(&clientAddress, 1);
                ValueItem notify{"Client connected from: " + (art::ustring)*tmp};
                delete tmp;
                info.async_notify(notify);
            }
#endif
            setsockopt(data.socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&main_socket, sizeof(main_socket));
            {
                std::lock_guard lock(safety);
                if (!NativeWorkersSingleton::register_handle((HANDLE)data.socket, &data)) {
                    closesocket(data.socket);
#ifndef DISABLE_RUNTIME_INFO
                    auto tmp = UniversalAddress::_define_to_string(&clientAddress, 1);
                    ValueItem notify{"Client: " + (art::ustring)*tmp + " not accepted because register handle failed " + std::to_string(data.socket)};
                    delete tmp;
                    info.sync_notify(notify);
#endif
                    if (!data.is_bound)
                        delete &data;
                    else
                        make_acceptEx(&data);
                    return;
                }
            }
            if (data.is_bound) {
                lock_guard guard(data.cv_mutex);
                data.cv.notify_all();
            } else
                accepted(&data, std::move(clientAddress), std::move(localAddress));
            return;
        }

        void make_socket() {
            main_socket = WSASocketW(AF_INET6, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
            if (main_socket == INVALID_SOCKET) {
                ValueItem error = art::ustring("Failed create socket: ") + std::to_string(WSAGetLastError());
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            DWORD argp = 1; //non blocking
            int result = setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&argp, sizeof(argp));
            if (result == SOCKET_ERROR) {
                ValueItem error = art::ustring("Failed set reuse addr: ") + std::to_string(WSAGetLastError());
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            if (ioctlsocket(main_socket, FIONBIO, &argp) == SOCKET_ERROR) {
                ValueItem error = art::ustring("Failed set no block mode: ") + std::to_string(WSAGetLastError());
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            int cfg = !config.allow_ip4;
            if (setsockopt(main_socket, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&cfg, sizeof(cfg)) == -1) {
                ValueItem error = art::ustring("Failed set dual mode: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            cfg = !config.enable_timestamps;
            if (setsockopt(main_socket, IPPROTO_TCP, TCP_TIMESTAMPS, (char*)&cfg, sizeof(cfg)) == -1) {
                ValueItem error = art::ustring("Failed set timestamps mode: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            cfg = !config.enable_delay;
            if (setsockopt(main_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&cfg, sizeof(cfg)) == -1) {
                ValueItem error = art::ustring("Failed set delay mode: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            cfg = config.fast_open_queue;
            if (setsockopt(main_socket, IPPROTO_TCP, TCP_FASTOPEN, (char*)&cfg, sizeof(cfg))) {
                ValueItem warn = art::ustring("Failed set fast open settings for server (") + std::to_string(errno) + "), continue regular mode";
                warning.async_notify(warn);
            }
            cfg = config.recv_timeout_ms;
            if (setsockopt(main_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&cfg, sizeof(cfg)) == -1) {
                ValueItem error = art::ustring("Failed set recv timeout: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            cfg = config.send_timeout_ms;
            if (setsockopt(main_socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&cfg, sizeof(cfg)) == -1) {
                ValueItem error = art::ustring("Failed set recv timeout: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            cfg = config.enable_keep_alive;
            if (setsockopt(main_socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&cfg, sizeof(cfg)) == -1) {
                ValueItem error = art::ustring("Failed to enable keep alive: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            if (config.enable_keep_alive) {
                int cfg = config.keep_alive_settings.idle_ms;
                if (setsockopt(main_socket, IPPROTO_TCP, TCP_KEEPIDLE, (char*)&cfg, sizeof(cfg)) == -1) {
                    ValueItem error = art::ustring("Failed set keep idle: ") + std::to_string(errno);
                    errors.sync_notify(error);
                    corrupted = true;
                    return;
                }
                cfg = config.keep_alive_settings.interval_ms;
                if (setsockopt(main_socket, IPPROTO_TCP, TCP_KEEPINTVL, (char*)&cfg, sizeof(cfg)) == -1) {
                    ValueItem error = art::ustring("Failed set keep alive interval: ") + std::to_string(errno);
                    errors.sync_notify(error);
                    corrupted = true;
                    return;
                }
                cfg = config.keep_alive_settings.retry_count;
                if (setsockopt(main_socket, IPPROTO_TCP, TCP_KEEPCNT, (char*)&cfg, sizeof(cfg)) == -1) {
                    ValueItem error = art::ustring("Failed set keep alive retry count: ") + std::to_string(errno);
                    errors.sync_notify(error);
                    corrupted = true;
                    return;
                }
#ifdef TCP_MAXRTMS
                cfg = config.keep_alive_settings.user_timeout_ms;
                if (setsockopt(main_socket, IPPROTO_TCP, TCP_MAXRTMS, (char*)&cfg, sizeof(cfg)) == -1) {
                    ValueItem error = art::ustring("Failed set user timeout: ") + std::to_string(errno);
                    errors.sync_notify(error);
                    corrupted = true;
                    return;
                }
#else
                cfg = config.keep_alive_settings.user_timeout_ms / 1000;
                if (setsockopt(main_socket, IPPROTO_TCP, TCP_MAXRT, (char*)&cfg, sizeof(cfg)) == -1) {
                    ValueItem error = art::ustring("Failed set user timeout: ") + std::to_string(errno);
                    errors.sync_notify(error);
                    corrupted = true;
                    return;
                }
#endif
            }

            init_win_fns(main_socket);
            if (bind(main_socket, (sockaddr*)&connectionAddress, sizeof(sockaddr_in6)) == SOCKET_ERROR) {
                ValueItem error = art::ustring("Failed bind: ") + std::to_string(WSAGetLastError());
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            if (!NativeWorkersSingleton::register_handle((HANDLE)main_socket, this)) {
                ValueItem error = art::ustring("Failed register handle: ") + std::to_string(GetLastError());
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            if (listen(main_socket, SOMAXCONN) == SOCKET_ERROR) {
                WSACleanup();
                ValueItem error = art::ustring("Failed start handle: ") + std::to_string(GetLastError());
                errors.sync_notify(error);
                corrupted = true;
            }
        }

    public:
        TcpNetworkManager(universal_address& ip_port, size_t acceptors, TcpNetworkServer::ManageType manage_type, const TcpConfiguration& config)
            : acceptors(acceptors), manage_type(manage_type), config(config) {
            memcpy(&connectionAddress, &ip_port, sizeof(sockaddr_in6));
        }

        ~TcpNetworkManager() {
            shutdown();
        }

        void handle(void* _data, NativeWorkerHandle* overlapped, unsigned long dwBytesTransferred, bool status) override {
            auto& data = *(tcp_handle*)overlapped;
            if (data.opcode == tcp_handle::Opcode::ACCEPT)
                new_connection(data, !((FALSE == status) || ((true == status) && (0 == dwBytesTransferred))));
            else if (!((FALSE == status) || ((true == status) && (0 == dwBytesTransferred))))
                data.handle(dwBytesTransferred);
            else {
#ifndef DISABLE_RUNTIME_INFO
                {
                    ValueItem notify{"Client disconnected (client hash: " + std::to_string(art::hash<void*>()(overlapped)) + ')'};
                    info.async_notify(notify);
                }
#endif
                data.connection_reset();
            }
        }

        void set_configuration(const TcpConfiguration& tcp) {
            if (corrupted)
                throw AttachARuntimeException("TcpNetworkManager is corrupted");
            std::lock_guard lock(safety);
            config = tcp;
        }

        void set_on_connect(art::shared_ptr<FuncEnvironment> handler_fn, TcpNetworkServer::ManageType manage_type) {
            if (corrupted)
                throw AttachARuntimeException("TcpNetworkManager is corrupted");
            std::lock_guard lock(safety);
            this->handler_fn = handler_fn;
            this->manage_type = manage_type;
        }

        void shutdown() {
            if (corrupted)
                throw AttachARuntimeException("TcpNetworkManager is corrupted");
            std::lock_guard lock(safety);
            if (disabled)
                return;
            if (closesocket(main_socket) == SOCKET_ERROR)
                WSACleanup();
            allow_new_connections = false;
            disabled = true;
            state_changed_cv.notify_all();
        }

        void pause() {
            if (corrupted)
                throw AttachARuntimeException("TcpNetworkManager is corrupted");
            allow_new_connections = false;
        }

        void resume() {
            if (corrupted)
                throw AttachARuntimeException("TcpNetworkManager is corrupted");
            allow_new_connections = true;
        }

        void start() {
            if (corrupted)
                throw AttachARuntimeException("TcpNetworkManager is corrupted");
            std::lock_guard lock(safety);
            allow_new_connections = true;
            if (!disabled)
                return;
            make_socket();
            if (corrupted)
                return;
            for (size_t i = 0; i < acceptors; i++)
                make_acceptEx();
            disabled = false;
            state_changed_cv.notify_all();
        }

        ValueItem accept(bool ignore_acceptors = false) {
            if (!ignore_acceptors && acceptors)
                throw AttachARuntimeException("Tried to accept connection with enabled acceptors and ignore_acceptors = false");
            if (corrupted)
                throw AttachARuntimeException("TcpNetworkManager is corrupted");
            if (disabled)
                throw AttachARuntimeException("TcpNetworkManager is disabled");
            if (!allow_new_connections)
                throw AttachARuntimeException("TcpNetworkManager is paused");
            tcp_handle* data = new tcp_handle(0, config.buffer_size, this);
            MutexUnify um(data->cv_mutex);
            unique_lock lock(um);
            data->opcode = tcp_handle::Opcode::ACCEPT;
            data->is_bound = true;
            make_acceptEx(data);
            data->cv.wait(lock);
            return accept_manager_construct(data);
        }

        void _await() {
            MutexUnify um(safety);
            art::unique_lock lock(um);
            if (corrupted)
                throw AttachARuntimeException("TcpNetworkManager is corrupted");
            while (!disabled)
                state_changed_cv.wait(lock);
        }

        void set_accept_filter(art::shared_ptr<FuncEnvironment> filter) {
            if (corrupted)
                throw AttachARuntimeException("TcpNetworkManager is corrupted");
            std::lock_guard lock(safety);
            this->accept_filter = filter;
        }

        bool is_corrupted() {
            return corrupted;
        }

        uint16_t port() {
            if (corrupted)
                throw AttachARuntimeException("TcpNetworkManager is corrupted");
            return htons(connectionAddress.sin6_port);
        }

        art::ustring ip() {
            if (corrupted)
                throw AttachARuntimeException("TcpNetworkManager is corrupted");
            Structure* tmp = CXX::Interface::constructStructure<universal_address>(define_UniversalAddress);
            memcpy(tmp->get_data_no_vtable(), &connectionAddress, sizeof(sockaddr_in6));
            tmp->fully_constructed = true;
            ValueItem args(tmp, as_reference);
            ValueItem* res;
            try {
                res = UniversalAddress::_define_to_string(&args, 1);
            } catch (...) {
                Structure::destruct(tmp);
                throw;
            }
            Structure::destruct(tmp);
            art::ustring ret = (art::ustring)*res;
            delete res;
            return ret;
        }

        ValueItem address() {
            if (corrupted)
                throw AttachARuntimeException("TcpNetworkManager is corrupted");

            sockaddr_storage addr;
            memcpy(&addr, &connectionAddress, sizeof(sockaddr_in6));
            memset(((char*)&addr) + sizeof(sockaddr_in6), 0, sizeof(sockaddr_storage) - sizeof(sockaddr_in6));
            return ValueItem(CXX::Interface::constructStructure<universal_address>(define_UniversalAddress, addr), no_copy);
        }

        bool is_paused() {
            return !disabled && !allow_new_connections;
        }

        bool in_run() {
            return !disabled;
        }
    };

    class TcpClientManager : public NativeWorkerManager {
        TaskMutex mutex;
        sockaddr_in6 connectionAddress;
        tcp_handle* _handle;
        bool corrupted = false;

        void set_configuration(SOCKET sock, const TcpConfiguration& config) {
            int cfg = !config.enable_timestamps;
            if (setsockopt(sock, IPPROTO_TCP, TCP_TIMESTAMPS, (char*)&cfg, sizeof(cfg)) == -1) {
                ValueItem error = art::ustring("Failed set timestamps mode: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            cfg = !config.enable_delay;
            if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&cfg, sizeof(cfg)) == -1) {
                ValueItem error = art::ustring("Failed set delay mode: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            cfg = config.recv_timeout_ms;
            if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&cfg, sizeof(cfg)) == -1) {
                ValueItem error = art::ustring("Failed set recv timeout: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            cfg = config.send_timeout_ms;
            if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&cfg, sizeof(cfg)) == -1) {
                ValueItem error = art::ustring("Failed set recv timeout: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            cfg = config.enable_keep_alive;
            if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&cfg, sizeof(cfg)) == -1) {
                ValueItem error = art::ustring("Failed to enable keep alive: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            if (config.enable_keep_alive) {
                int cfg = config.keep_alive_settings.idle_ms;
                if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, (char*)&cfg, sizeof(cfg)) == -1) {
                    ValueItem error = art::ustring("Failed set keep idle: ") + std::to_string(errno);
                    errors.sync_notify(error);
                    corrupted = true;
                    return;
                }
                cfg = config.keep_alive_settings.interval_ms;
                if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, (char*)&cfg, sizeof(cfg)) == -1) {
                    ValueItem error = art::ustring("Failed set keep alive interval: ") + std::to_string(errno);
                    errors.sync_notify(error);
                    corrupted = true;
                    return;
                }
                cfg = config.keep_alive_settings.retry_count;
                if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, (char*)&cfg, sizeof(cfg)) == -1) {
                    ValueItem error = art::ustring("Failed set keep alive count: ") + std::to_string(errno);
                    errors.sync_notify(error);
                    corrupted = true;
                    return;
                }
#ifdef TCP_MAXRTMS
                cfg = config.keep_alive_settings.user_timeout_ms;
                if (setsockopt(sock, IPPROTO_TCP, TCP_MAXRTMS, (char*)&cfg, sizeof(cfg)) == -1) {
                    ValueItem error = art::ustring("Failed set user timeout: ") + std::to_string(errno);
                    errors.sync_notify(error);
                    corrupted = true;
                    return;
                }
#else
                cfg = config.keep_alive_settings.user_timeout_ms / 1000;
                if (setsockopt(sock, IPPROTO_TCP, TCP_MAXRT, (char*)&cfg, sizeof(cfg)) == -1) {
                    ValueItem error = art::ustring("Failed set user timeout: ") + std::to_string(errno);
                    errors.sync_notify(error);
                    corrupted = true;
                    return;
                }
#endif
            }
            DWORD argp = 1;
            if (ioctlsocket(sock, FIONBIO, &argp) == SOCKET_ERROR) {
                ValueItem error = art::ustring("Failed set no block mode: ") + std::to_string(WSAGetLastError());
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
        }

    public:
        void handle(void* _data, NativeWorkerHandle* overlapped, unsigned long dwBytesTransferred, bool status) override {
            tcp_handle& handle = *(tcp_handle*)overlapped;
            if ((FALSE == status) || ((true == status) && (0 == dwBytesTransferred)))
                handle.connection_reset();
            else if (handle.opcode == tcp_handle::Opcode::ACCEPT)
                handle.cv.notify_all();
            else
                handle.handle(dwBytesTransferred);
        }

        TcpClientManager(sockaddr_in6& _connectionAddress, const TcpConfiguration& config)
            : connectionAddress(_connectionAddress) {
            SOCKET clientSocket = WSASocketW(AF_INET6, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
            if (clientSocket == INVALID_SOCKET) {
                corrupted = true;
                return;
            }
            set_configuration(clientSocket, config);
            if (corrupted) {
                closesocket(clientSocket);
                return;
            }

            _handle = new tcp_handle(clientSocket, 4096, this);
            MutexUnify umutex(_handle->cv_mutex);
            art::unique_lock<MutexUnify> lock(umutex);
            if (!_ConnectEx(clientSocket, (sockaddr*)&connectionAddress, sizeof(connectionAddress), NULL, 0, nullptr, (OVERLAPPED*)_handle)) {
                auto err = WSAGetLastError();
                if (err != ERROR_IO_PENDING) {
                    corrupted = true;
                    _handle->reset();
                    return;
                }
            }
            if (config.connection_timeout_ms > 0) {
                if (!_handle->cv.wait_for(lock, config.connection_timeout_ms)) {
                    corrupted = true;
                    _handle->reset();
                    return;
                }
            } else
                _handle->cv.wait(lock);
        }

        TcpClientManager(sockaddr_in6& _connectionAddress, char* data, uint32_t len, const TcpConfiguration& config)
            : connectionAddress(_connectionAddress), _handle(nullptr) {
            SOCKET clientSocket = WSASocketW(AF_INET6, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
            if (clientSocket == INVALID_SOCKET) {
                corrupted = true;
                return;
            }
            set_configuration(clientSocket, config);
            if (corrupted) {
                closesocket(clientSocket);
                return;
            }
            int cfg = !config.enable_delay;
            if (setsockopt(clientSocket, IPPROTO_TCP, TCP_FASTOPEN, (char*)&cfg, sizeof(cfg)) == -1) {
                ValueItem error = art::ustring("Failed set delay mode: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                closesocket(clientSocket);
                return;
            }

            _handle = new tcp_handle(clientSocket, 4096, this);
            char* old_buffer = _handle->data;
            _handle->data = data;
            _handle->buffer.buf = data;
            _handle->buffer.len = len;
            _handle->total_bytes = len;
            _handle->opcode = tcp_handle::Opcode::WRITE;
            MutexUnify umutex(_handle->cv_mutex);
            art::unique_lock<MutexUnify> lock(umutex);
            if (!_ConnectEx(clientSocket, (sockaddr*)&connectionAddress, sizeof(connectionAddress), data, len, nullptr, (OVERLAPPED*)_handle)) {
                auto err = WSAGetLastError();
                if (err != ERROR_IO_PENDING) {
                    corrupted = true;
                    return;
                }
            }
            if (config.connection_timeout_ms > 0) {
                if (!_handle->cv.wait_for(lock, config.connection_timeout_ms)) {
                    corrupted = true;
                    _handle->data = old_buffer;
                    _handle->reset();
                    return;
                }
            } else
                _handle->cv.wait(lock);
            _handle->data = old_buffer;
        }

        ~TcpClientManager() override {
            if (corrupted)
                return;
            delete _handle;
        }

        void set_configuration(const TcpConfiguration& config) {
            if (corrupted)
                throw std::runtime_error("TcpClientManager::read, corrupted");
            if (_handle) {
                set_configuration(_handle->socket, config);
                if (!corrupted)
                    _handle->rebuffer(config.buffer_size);
            }
        }

        int32_t read(char* data, int32_t len) {
            if (corrupted)
                throw std::runtime_error("TcpClientManager::read, corrupted");
            std::lock_guard<TaskMutex> lock(mutex);
            int32_t readed = 0;
            while (!_handle->available_bytes())
                if (!_handle->send_queue_item())
                    break;
            _handle->read_available(data, len, readed);
            return readed;
        }

        bool write(const char* data, int32_t len) {
            if (corrupted)
                throw std::runtime_error("TcpClientManager::write, corrupted");
            std::lock_guard<TaskMutex> lock(mutex);
            _handle->send_data(data, len);
            while (!_handle->available_bytes())
                if (!_handle->send_queue_item())
                    break;
            return _handle->valid();
        }

        bool write_file(const char* path, size_t len, uint64_t data_len, uint64_t offset, uint32_t chunks_size) {
            if (corrupted)
                throw std::runtime_error("TcpClientManager::write_file, corrupted");
            std::lock_guard<TaskMutex> lock(mutex);
            while (!_handle->available_bytes())
                if (!_handle->send_queue_item())
                    break;
            return _handle->send_file(path, len, data_len, offset, chunks_size);
        }

        bool write_file(void* handle, uint64_t data_len, uint64_t offset, uint32_t chunks_size) {
            if (corrupted)
                throw std::runtime_error("TcpClientManager::write_file, corrupted");
            std::lock_guard<TaskMutex> lock(mutex);
            while (!_handle->available_bytes())
                if (!_handle->send_queue_item())
                    break;
            return _handle->send_file(handle, data_len, offset, chunks_size);
        }

        void close() {
            if (corrupted)
                throw std::runtime_error("TcpClientManager::close, corrupted");
            std::lock_guard<TaskMutex> lock(mutex);
            _handle->close();
        }

        void reset() {
            if (corrupted)
                throw std::runtime_error("TcpClientManager::close, corrupted");
            std::lock_guard<TaskMutex> lock(mutex);
            _handle->reset();
        }

        bool is_corrupted() {
            return corrupted;
        }

        void rebuffer(uint32_t size) {
            if (corrupted)
                throw std::runtime_error("TcpClientManager::rebuffer, corrupted");
            std::lock_guard<TaskMutex> lock(mutex);
            _handle->rebuffer(size);
        }
    };

#pragma endregion

    class udp_handle : public NativeWorkerHandle, public NativeWorkerManager {
        art::typed_lgr<Task> notify_task;
        SOCKET socket;
        sockaddr_in6 server_address;

    public:
        DWORD fullifed_bytes;
        bool status;
        DWORD last_error;

        udp_handle(sockaddr_in6& address, uint32_t timeout_ms)
            : NativeWorkerHandle(this), last_error(0), fullifed_bytes(0), status(false) {
            socket = WSASocketW(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
            if (socket == INVALID_SOCKET)
                return;
            if (bind(socket, (sockaddr*)&address, sizeof(sockaddr_in6)) == SOCKET_ERROR) {
                closesocket(socket);
                socket = INVALID_SOCKET;
                return;
            }
            server_address = address;
        }

        void handle(void* data, NativeWorkerHandle* overlapped, unsigned long fullifed_bytes, bool status) override {
            this->fullifed_bytes = fullifed_bytes;
            this->status = status;
            last_error = GetLastError();
            Task::start(notify_task);
        }

        void recv(uint8_t* data, uint32_t size, sockaddr_storage& sender, int& sender_len) {
            if (socket == INVALID_SOCKET)
                throw InvalidOperation("Socket not connected");
            WSABUF buf;
            buf.buf = (char*)data;
            buf.len = size;
            notify_task = Task::dummy_task();
            DWORD flags = 0;
            if (WSARecvFrom(socket, &buf, 1, nullptr, &flags, (sockaddr*)&sender, &sender_len, (OVERLAPPED*)this, nullptr)) {
                if (WSAGetLastError() != WSA_IO_PENDING) {
                    last_error = WSAGetLastError();
                    status = false;
                    fullifed_bytes = 0;
                    notify_task = nullptr;
                    return;
                }
            }
            Task::await_task(notify_task);
            notify_task = nullptr;
        }

        void send(uint8_t* data, uint32_t size, sockaddr_storage& to) {
            sockaddr_in6 sender;
            WSABUF buf;
            buf.buf = (char*)data;
            buf.len = size;
            notify_task = Task::dummy_task();
            if (WSASendTo(socket, &buf, 1, nullptr, 0, (sockaddr*)&to, sizeof(to), (OVERLAPPED*)this, nullptr)) {
                if (WSAGetLastError() != WSA_IO_PENDING) {
                    last_error = WSAGetLastError();
                    status = false;
                    fullifed_bytes = 0;
                    notify_task = nullptr;
                    return;
                }
            }
            Task::await_task(notify_task);
            notify_task = nullptr;
        }

        ValueItem local_address() {
            universal_address addr;
            int socklen = sizeof(universal_address);
            if (getsockname(socket, (sockaddr*)&addr, &socklen) == -1)
                return nullptr;
            return CXX::Interface::constructStructure<universal_address>(define_UniversalAddress, addr);
        }

        ValueItem remote_address() {
            universal_address addr;
            int socklen = sizeof(universal_address);
            if (getpeername(socket, (sockaddr*)&addr, &socklen) == -1)
                return nullptr;
            return CXX::Interface::constructStructure<universal_address>(define_UniversalAddress, addr);
        }
    };

    uint8_t init_networking() {
        init_define_UniversalAddress();
        init_define_TcpConfiguration();
        init_define_TcpNetworkStream();
        init_define_TcpNetworkBlocking();

        if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {
            auto err = WSAGetLastError();
            switch (err) {
            case WSASYSNOTREADY:
                return 1;
            case WSAVERNOTSUPPORTED:
                return 2;
                return 3;
            case WSAEPROCLIM:
                return 4;
            case WSAEFAULT:
                return 5;
            default:
                return 0xFF;
            }
        };
        inited = true;
        return 0;
    }

    void deinit_networking() {
        if (inited)
            WSACleanup();
        inited = false;
    }

#elif PLATFORM_LINUX
    bool inited = false;
    AttachAVirtualTable* define_TcpNetworkStream = nullptr;
    AttachAVirtualTable* define_TcpNetworkBlocking = nullptr;
    using SOCKET = int;
#define INVALID_SOCKET -1

    struct tcp_handle : public NativeWorkerHandle {
        std::list<std::tuple<char*, size_t>> write_queue;
        std::list<std::tuple<char*, size_t>> read_queue;
        TaskConditionVariable cv;
        TaskMutex cv_mutex;
        SOCKET socket;

        struct {
            char* buf;
            int len;
        } buffer;

        char* data;
        int total_bytes;
        int sent_bytes;
        int readed_bytes;
        int data_len;
        int aerrno = 0;
        bool force_mode;
        bool is_bound = false;
        uint32_t max_read_queue_size;
        TcpError invalid_reason = TcpError::none;
        sockaddr_storage clientAddress;
        socklen_t clientAddressLen = sizeof(sockaddr_storage);

        enum class Opcode : uint8_t {
            ACCEPT,
            READ,
            WRITE,
            INTERNAL_READ,
            INTERNAL_CLOSE
        } opcode = Opcode::ACCEPT;

        tcp_handle(SOCKET socket, int32_t buffer_len, NativeWorkerManager* manager, uint32_t read_queue_size = 10)
            : socket(socket), NativeWorkerHandle(manager), max_read_queue_size(read_queue_size) {
            if (buffer_len < 0)
                throw InvalidArguments("buffer_len must be positive");
            if (buffer_len) {
                data = new char[buffer_len];
                buffer.buf = data;
                buffer.len = buffer_len;
                data_len = buffer_len;
            } else
                data = nullptr;
            total_bytes = 0;
            sent_bytes = 0;
            readed_bytes = 0;
            force_mode = false;
        }

        ~tcp_handle() {
            close();
        }

        uint32_t available_bytes() {
            if (!data)
                return 0;
            if (readed_bytes)
                return true;
            int value = 0;
            int result = ioctl(socket, FIONREAD, &value);
            if (result != 0)
                return 0;
            else
                return value;
        }

        bool data_available() {
            return available_bytes() > 0;
        }

        void send_data(const char* data, int len) {
            if (!data)
                return;
            char* new_data = new char[len];
            memcpy(new_data, data, len);
            write_queue.push_back(std::make_tuple(new_data, len));
        }

        //async
        bool send_queue_item() {
            if (!data)
                return false;
            if (write_queue.empty())
                return false;
            auto item = write_queue.front();
            write_queue.pop_front();
            auto& send_data = std::get<0>(item);
            auto& val_len = std::get<1>(item);
            std::unique_ptr<char[]> send_data_ptr(send_data);
            //set buffer
            buffer.len = data_len;
            buffer.buf = data;
            while (val_len) {
                size_t to_sent_bytes = val_len > data_len ? data_len : val_len;
                memcpy(data, send_data, to_sent_bytes);
                buffer.len = to_sent_bytes;
                buffer.buf = data;
                if (!send_await()) {
                    return false;
                }
                if (val_len < sent_bytes)
                    return true;
                val_len -= sent_bytes;
                send_data += sent_bytes;
            }
            return true;
        }

        void read_force(uint32_t buffer_len, char* buffer) {
            if (!data)
                return;
            if (!buffer_len)
                return;
            if (!buffer)
                return;
            while (buffer_len) {
                int readed = 0;
                read_available(buffer, buffer_len, readed);
                buffer += readed;
                if (readed > buffer_len)
                    return;
                buffer_len -= readed;
            }
        }

        int64_t write_force(const char* to_write, uint32_t to_write_len) {
            if (!data)
                return -1;
            if (!to_write_len)
                return -1;
            if (!to_write)
                return -1;

            force_mode = true;
            if (data_len < to_write_len) {
                buffer.len = data_len;
                buffer.buf = this->data;
                if (!send_await())
                    return -1;
                force_mode = false;
                return sent_bytes;
            } else {
                buffer.len = to_write_len;
                buffer.buf = this->data;
                memcpy(this->data, to_write, to_write_len);
                if (!send_await())
                    return -1;
                force_mode = false;
                return sent_bytes;
            }
        }

        void read_data() {
            if (!data)
                return;
            MutexUnify mutex(cv_mutex);
            art::unique_lock<MutexUnify> lock(mutex);
            opcode = Opcode::READ;
            read();
            cv.wait(lock);
        }

        void read_available_no_block(char* extern_buffer, int buffer_len, int& readed) {
            if (!readed_bytes)
                readed = 0;
            else if (readed_bytes < buffer_len) {
                readed = readed_bytes;
                memcpy(extern_buffer, data, readed_bytes);
                readed_bytes = 0;
            } else {
                readed = buffer_len;
                memcpy(extern_buffer, buffer.buf, buffer_len);
                readed_bytes -= buffer_len;
                buffer.buf += buffer_len;
                buffer.len -= buffer_len;
            }
        }

        void read_available(char* extern_buffer, int buffer_len, int& readed) {
            if (!readed_bytes) {
                if (read_queue.empty())
                    read_data();
                else {
                    auto item = read_queue.front();
                    read_queue.pop_front();
                    auto& read_data = std::get<0>(item);
                    auto& val_len = std::get<1>(item);
                    std::unique_ptr<char[]> read_data_ptr(read_data);
                    buffer.buf = data;
                    buffer.len = data_len;
                    readed_bytes = val_len;
                    memcpy(data, read_data, val_len);
                }
            }
            if (readed_bytes < buffer_len) {
                readed = readed_bytes;
                memcpy(extern_buffer, data, readed_bytes);
                readed_bytes = 0;
            } else {
                readed = buffer_len;
                memcpy(extern_buffer, buffer.buf, buffer_len);
                readed_bytes -= buffer_len;
                buffer.buf += buffer_len;
                buffer.len -= buffer_len;
            }
        }

        char* read_available_no_copy(int& readed) {
            if (!readed_bytes)
                read_data();
            readed = readed_bytes;
            readed_bytes = 0;
            return data;
        }

        void close(TcpError err = TcpError::local_close) {
            if (!data)
                return;
            pre_close(err);
            internal_close();
        }

        void handle(unsigned long dwBytesTransferred, int sock_error) {
            int flags = 0, bytes = 0;
            if (!data) {
                if (opcode != Opcode::INTERNAL_CLOSE)
                    return;
            }
            if (sock_error) {
                switch (sock_error) {
                case EFAULT:
                case EINVAL:
                case EAGAIN:
#if EAGAIN != EWOULDBLOCK
                case EWOULDBLOCK:
#endif
                    pre_close(TcpError::invalid_state);
                    return;
                case ECONNRESET:
                    pre_close(TcpError::remote_close);
                    return;
                default:
                    pre_close(TcpError::undefined_error);
                    return;
                }
            }
            MutexUnify mutex(cv_mutex);
            art::unique_lock<MutexUnify> lock(mutex);
            switch (opcode) {
            case Opcode::READ: {
                readed_bytes = dwBytesTransferred;
                cv.notify_all();
                break;
            }
            case Opcode::WRITE:
                sent_bytes += dwBytesTransferred;
                if (sent_bytes < total_bytes) {
                    buffer.buf = data + sent_bytes;
                    buffer.len = total_bytes - sent_bytes;
                    if (!data_available())
                        send();
                    else {
                        char* data = new char[buffer.len];
                        memcpy(data, buffer.buf, buffer.len);
                        write_queue.push_front(std::make_tuple(data, buffer.len));
                        if (force_mode) {
                            opcode = Opcode::INTERNAL_READ;
                            read();
                        } else
                            cv.notify_all();
                    }
                } else
                    cv.notify_all();
                break;
            case Opcode::INTERNAL_READ:
                if (dwBytesTransferred) {
                    char* buffer = new char[dwBytesTransferred];
                    memcpy(buffer, data, dwBytesTransferred);
                    read_queue.push_back(std::make_tuple(buffer, dwBytesTransferred));
                }
                if (!data_available()) {
                    if (read_queue.size() > max_read_queue_size)
                        close(TcpError::read_queue_overflow);
                    else
                        read();
                } else {
                    if (write_queue.empty())
                        close(TcpError::invalid_state);
                    else {
                        auto item = write_queue.front();
                        write_queue.pop_front();
                        auto& write_data = std::get<0>(item);
                        auto& val_len = std::get<1>(item);
                        memcpy(data, write_data, val_len);
                        delete[] write_data;
                        buffer.buf = data;
                        buffer.len = val_len;
                        send();
                    }
                }
                break;
            case Opcode::INTERNAL_CLOSE:
                cv.notify_all();
                break;
            default:
                break;
            }
        }

        void send_and_close(const char* data, int len) {
            if (!data)
                return;
            buffer.len = data_len;
            buffer.buf = this->data;
            write_queue = {};
            force_mode = true;
            while (data_len < len) {
                memcpy(buffer.buf, data, buffer.len);
                if (!send_await())
                    return;
                data += buffer.len;
                len -= buffer.len;
            }
            if (len) {
                //send last part of data and close
                memcpy(buffer.buf, data, len);
                buffer.len = len;
                send_await();
            }
            force_mode = false;
            close();
        }

        bool send_file(int file, uint64_t data_len, uint64_t offset, uint32_t chunks_size) {
            if (!data)
                return false;
            if (chunks_size == 0)
                chunks_size = 0x1000;
            if (data_len == 0) {
                struct stat file_stat;
                if (fstat(file, &file_stat) == -1)
                    return false;
                data_len = file_stat.st_size;
                if (data_len < offset)
                    return false;
                data_len -= offset;
            }

            if (data_len > UINT_MAX) {
                uint64_t sended = 0;
                uint64_t blocks = data_len / UINT_MAX;
                uint64_t last_block = data_len % blocks;

                while (blocks--)
                    if (!transfer_file(socket, file, UINT_MAX, chunks_size, sended + offset))
                        return false;
                    else
                        sended += UINT_MAX;


                if (last_block)
                    if (!transfer_file(socket, file, last_block, chunks_size, sended + offset))
                        return false;
            } else {
                if (!transfer_file(socket, file, data_len, chunks_size, offset))
                    return false;
            }
            return true;
        }

        bool send_file(const char* path, size_t path_len, uint64_t data_len, uint64_t offset, uint32_t chunks_size) {
            if (!data)
                return false;
            if (chunks_size == 0)
                chunks_size = 0x1000;
            int file = ::open(path, O_RDONLY | O_NONBLOCK);
            if (file == -1)
                return false;
            bool result;
            try {
                result = send_file(file, data_len, offset, chunks_size);
            } catch (...) {
                ::close(file);
                throw;
            }
            ::close(file);
            return result;
        }

        bool valid() {
            return data != nullptr;
        }

        void reset() {
            if (!data)
                return;
            struct linger sl;
            sl.l_onoff = 1;
            sl.l_linger = 0;
            setsockopt(socket, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl));
            pre_close(TcpError::local_reset);
            internal_close();
        }

        void connection_reset() {
            MutexUnify mutex(cv_mutex);
            art::unique_lock<MutexUnify> lock(mutex);
            char* old_data = data;
            data = nullptr;
            invalid_reason = TcpError::remote_close;
            readed_bytes = 0;
            cv.notify_all();
            delete[] data;
        }

        void rebuffer(int32_t buffer_len) {
            if (!data)
                return;
            if (buffer_len < 0)
                throw InvalidArguments("buffer_len must be positive");
            char* new_data = new char[buffer_len];
            delete[] data;
            data = new_data;
            data_len = buffer_len;
        }

    private:
        void pre_close(TcpError err) {
            MutexUnify mutex(cv_mutex);
            art::unique_lock<MutexUnify> lock(mutex);
            opcode = Opcode::INTERNAL_CLOSE;
            NativeWorkersSingleton::post_shutdown(this, socket, SHUT_RDWR);
            cv.wait(lock);
            std::list<std::tuple<char*, size_t>> clear_write_queue;
            std::list<std::tuple<char*, size_t>> clear_read_queue;
            readed_bytes = 0;
            sent_bytes = 0;
            delete[] data;
            data = nullptr;
            invalid_reason = err;
            write_queue.swap(clear_write_queue);
            read_queue.swap(clear_read_queue);
            cv.notify_all();

            lock.unlock();
            for (auto& item : clear_write_queue)
                delete[] std::get<0>(item);
            for (auto& item : clear_read_queue)
                delete[] std::get<0>(item);
        }

        void internal_close() {
            MutexUnify mutex(cv_mutex);
            art::unique_lock<MutexUnify> lock(mutex);
            opcode = Opcode::INTERNAL_CLOSE;
            NativeWorkersSingleton::post_close(this, socket);
            cv.wait(lock);
            socket = INVALID_SOCKET;
        }

        void read() {
            buffer.buf = this->data;
            buffer.len = data_len;
            NativeWorkersSingleton::post_recv(this, socket, buffer.buf, buffer.len, 0);
        }

        void send() {
            opcode = Opcode::WRITE;
            NativeWorkersSingleton::post_send(this, socket, buffer.buf, buffer.len, 0);
        }

        bool send_await() {
            MutexUnify mutex(cv_mutex);
            art::unique_lock<MutexUnify> lock(mutex);
            send();
            cv.wait(lock);
            return data; //if data is null, then socket is closed
        }

        bool transfer_file(SOCKET sock, int file, uint32_t total_size, uint32_t chunks_size, uint64_t offset) {
            uint64_t sent_bytes = 0;
            struct stat file_stat;
            bool result = true;
            if (fstat(file, &file_stat) == -1)
                return false;
            if (file_stat.st_size < offset + total_size)
                return false;

            char* file_data = (char*)mmap(NULL, file_stat.st_size, PROT_READ, MAP_PRIVATE, file, 0);
            if (file_data == MAP_FAILED)
                return false;

            if (!total_size) {
                while (file_stat.st_size > offset) {
                    int chunk_size = std::min((uint64_t)chunks_size, file_stat.st_size - offset);
                    int sent = write_force(file_data + offset, chunk_size);
                    if (sent == -1) {
                        result = false;
                        break;
                    }
                    if (sent >= file_stat.st_size + offset)
                        break;
                    offset += sent;
                }
            } else {
                while (total_size) {
                    int chunk_size = std::min((uint64_t)std::min(chunks_size, total_size), file_stat.st_size - offset);
                    int sent = write_force(file_data + offset, chunk_size);
                    if (sent == -1) {
                        result = false;
                        break;
                    }
                    offset += sent;
                    if (sent >= total_size)
                        break;
                    total_size -= sent;
                }
            }
            munmap(file_data, file_stat.st_size);
            return result;
        }
    };

#pragma region TcpNetworkStream

    class TcpNetworkStreamImpl : public TcpNetworkStream {
        friend class TcpNetworkManager;
        struct tcp_handle* handle;
        TaskMutex mutex;
        TcpError last_error;

        bool checkup() {
            if (!handle)
                return false;
            if (!handle->valid()) {
                last_error = handle->invalid_reason;
                delete handle;
                handle = nullptr;
                return false;
            }
            return true;
        }

    public:
        TcpNetworkStreamImpl(tcp_handle* handle)
            : handle(handle), last_error(TcpError::none) {}

        ~TcpNetworkStreamImpl() {
            if (handle) {
                std::lock_guard lg(mutex);
                handle->close();
                delete handle;
            }
            handle = nullptr;
        }

        ValueItem read_available_ref() override {
            std::lock_guard lg(mutex);
            if (!handle)
                return nullptr;
            while (!handle->data_available()) {
                if (!handle->send_queue_item())
                    break;
            }
            if (!checkup())
                return ValueItem(nullptr, ValueMeta(VType::raw_arr_ui8, false, false, 0), as_reference);
            int readed = 0;
            char* data = handle->read_available_no_copy(readed);
            return ValueItem(data, ValueMeta(VType::raw_arr_ui8, false, false, readed), as_reference);
        }

        ValueItem read_available(char* buffer, int buffer_len) override {
            std::lock_guard lg(mutex);
            if (!handle)
                return nullptr;
            while (!handle->data_available()) {
                if (!handle->send_queue_item())
                    break;
            }

            if (!checkup())
                return (uint32_t)0;
            int readed = 0;
            handle->read_available(buffer, buffer_len, readed);
            return ValueItem((uint32_t)readed);
        }

        bool data_available() override {
            std::lock_guard lg(mutex);
            if (handle)
                return handle->data_available();
            return false;
        }

        void write(const char* data, size_t size) override {
            std::lock_guard lg(mutex);
            if (handle) {
                handle->send_data(data, size);
                while (!handle->data_available()) {
                    if (!handle->send_queue_item())
                        break;
                }
                checkup();
            }
        }

        bool write_file(char* path, size_t path_len, uint64_t data_len, uint64_t offset, uint32_t chunks_size) override {
            std::lock_guard lg(mutex);
            if (handle) {
                while (handle->valid())
                    if (!handle->send_queue_item())
                        break;

                if (!checkup())
                    return false;

                return handle->send_file(path, path_len, data_len, offset, chunks_size);
            }
            return false;
        }

        bool write_file(int fhandle, uint64_t data_len, uint64_t offset, uint32_t chunks_size) override {
            std::lock_guard lg(mutex);
            if (handle) {
                while (handle->valid())
                    if (!handle->send_queue_item())
                        break;
                if (!checkup())
                    return false;
                return handle->send_file(fhandle, data_len, offset, chunks_size);
            }
            return false;
        }

        //write all data from write_queue
        void force_write() override {
            std::lock_guard lg(mutex);
            if (handle) {
                while (handle->valid())
                    if (!handle->send_queue_item())
                        break;
                checkup();
            }
        }

        void force_write_and_close(const char* data, size_t size) override {
            std::lock_guard lg(mutex);
            if (handle) {
                handle->send_and_close(data, size);
                last_error = handle->invalid_reason;
                delete handle;
            }
            handle = nullptr;
        }

        void close() override {
            std::lock_guard lg(mutex);
            if (handle) {
                handle->close();
                last_error = handle->invalid_reason;
                delete handle;
            }
            handle = nullptr;
        }

        void reset() override {
            std::lock_guard lg(mutex);
            if (handle) {
                handle->reset();
                last_error = handle->invalid_reason;
                delete handle;
            }
            handle = nullptr;
        }

        void rebuffer(int32_t new_size) override {
            std::lock_guard lg(mutex);
            if (handle)
                handle->rebuffer(new_size);
        }

        bool is_closed() override {
            std::lock_guard lg(mutex);
            if (handle) {
                bool res = handle->valid();
                if (!res) {
                    delete handle;
                    handle = nullptr;
                }
                return !res;
            }
            return true;
        }

        TcpError error() override {
            std::lock_guard lg(mutex);
            if (handle)
                return handle->invalid_reason;
            return last_error;
        }

        ValueItem local_address() override {
            std::lock_guard lg(mutex);
            if (!handle)
                return nullptr;
            universal_address addr;
            socklen_t socklen = sizeof(universal_address);
            if (getsockname(handle->socket, (sockaddr*)&addr, &socklen) == -1)
                return nullptr;
            return CXX::Interface::constructStructure<universal_address>(define_UniversalAddress, addr);
        }

        ValueItem remote_address() override {
            std::lock_guard lg(mutex);
            if (!handle)
                return nullptr;
            universal_address addr;
            socklen_t socklen = sizeof(universal_address);
            if (getpeername(handle->socket, (sockaddr*)&addr, &socklen) == -1)
                return nullptr;
            return CXX::Interface::constructStructure<universal_address>(define_UniversalAddress, addr);
        }
    };

    AttachAFun(funs_TcpNetworkStream_read_available_ref, 1, {
        return CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream).read_available_ref();
    });
    AttachAFun(funs_TcpNetworkStream_read_available, 2, {
        auto& stream = CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream);
        if (args[1].meta.vtype != VType::raw_arr_ui8 && args[1].meta.vtype != VType::raw_arr_i8)
            throw InvalidArguments("The second argument must be a raw_arr_ui8.");
        return stream.read_available((char*)args[1].val, args[1].meta.val_len);
    });
    AttachAFun(funs_TcpNetworkStream_data_available, 1, {
        return CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream).data_available();
    });
    AttachAFun(funs_TcpNetworkStream_write, 2, {
        auto& stream = CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream);
        if (args[1].meta.vtype != VType::raw_arr_ui8 && args[1].meta.vtype != VType::raw_arr_i8)
            throw InvalidArguments("The second argument must be a raw_arr_ui8.");
        stream.write((char*)args[1].val, args[1].meta.val_len);
    });
    AttachAFun(funs_TcpNetworkStream_write_file, 2, {
        auto& stream = CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream);
        ValueItem& arg1 = args[1];
        uint64_t data_len = 0;
        uint64_t offset = 0;
        uint32_t chunks_size = 0;
        if (arg1.meta.vtype != VType::struct_ && arg1.meta.vtype != VType::string)
            throw InvalidArguments("The second argument must be a file handle or a file path.");
        if (len >= 3)
            data_len = (uint64_t)args[2];
        if (len >= 4)
            offset = (uint64_t)args[3];
        if (len >= 5)
            chunks_size = (uint32_t)args[4];
        if (arg1.meta.vtype == VType::struct_) {
            auto& proxy = (Structure&)args[1];
            if (proxy.get_vtable()) {
                if (proxy.get_name() == "file_handle") {
                    return stream.write_file((*(typed_lgr<art::files::FileHandle>*)proxy.get_data_no_vtable())->internal_get_handle(), data_len, offset, chunks_size);
                } else if (proxy.get_name() == "blocking_file_handle")
                    return stream.write_file((*(typed_lgr<art::files::BlockingFileHandle>*)proxy.get_data_no_vtable())->internal_get_handle(), data_len, offset, chunks_size);
            }
            throw InvalidArguments("The second argument must be a file handle or a file path.");
        } else {
            art::ustring& path = *(art::ustring*)arg1.getSourcePtr();
            stream.write_file(path.data(), path.size(), data_len, offset, chunks_size);
        }
    });
    AttachAFun(funs_TcpNetworkStream_force_write, 1, {
        CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream).force_write();
    });
    AttachAFun(funs_TcpNetworkStream_force_write_and_close, 2, {
        auto& stream = CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream);
        if (args[1].meta.vtype != VType::raw_arr_ui8 && args[1].meta.vtype != VType::raw_arr_i8)
            throw InvalidArguments("The second argument must be a raw_arr_ui8.");
        stream.force_write_and_close((char*)args[1].getSourcePtr(), args[1].meta.val_len);
    });
    AttachAFun(funs_TcpNetworkStream_close, 1, {
        CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream).close();
    });
    AttachAFun(funs_TcpNetworkStream_reset, 1, {
        CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream).reset();
    });
    AttachAFun(funs_TcpNetworkStream_rebuffer, 2, {
        CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream).rebuffer((int32_t)args[1]);
    });
    AttachAFun(funs_TcpNetworkStream_is_closed, 1, {
        return CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream).is_closed();
    });
    AttachAFun(funs_TcpNetworkStream_error, 1, {
        return (uint8_t)CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream).error();
    });
    AttachAFun(funs_TcpNetworkStream_local_address, 1, {
        return CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream).local_address();
    });
    AttachAFun(funs_TcpNetworkStream_remote_address, 1, {
        return CXX::Interface::getExtractAs<TcpNetworkStreamImpl>(args[0], define_TcpNetworkStream).remote_address();
    });

    void init_define_TcpNetworkStream() {
        if (define_TcpNetworkStream != nullptr)
            return;
        define_TcpNetworkStream = CXX::Interface::createTable<TcpNetworkStreamImpl>(
            "tcp_network_stream",
            CXX::Interface::direct_method("read_available_ref", funs_TcpNetworkStream_read_available_ref),
            CXX::Interface::direct_method("read_available", funs_TcpNetworkStream_read_available),
            CXX::Interface::direct_method("data_available", funs_TcpNetworkStream_data_available),
            CXX::Interface::direct_method("write", funs_TcpNetworkStream_write),
            CXX::Interface::direct_method("write_file", funs_TcpNetworkStream_write_file),
            CXX::Interface::direct_method("force_write", funs_TcpNetworkStream_force_write),
            CXX::Interface::direct_method("force_write_and_close", funs_TcpNetworkStream_force_write_and_close),
            CXX::Interface::direct_method("close", funs_TcpNetworkStream_close),
            CXX::Interface::direct_method("reset", funs_TcpNetworkStream_reset),
            CXX::Interface::direct_method("rebuffer", funs_TcpNetworkStream_rebuffer),
            CXX::Interface::direct_method("is_closed", funs_TcpNetworkStream_is_closed),
            CXX::Interface::direct_method("error", funs_TcpNetworkStream_error),
            CXX::Interface::direct_method("local_address", funs_TcpNetworkStream_local_address),
            CXX::Interface::direct_method("remote_address", funs_TcpNetworkStream_remote_address)
        );
        CXX::Interface::typeVTable<TcpNetworkStream>() = define_TcpNetworkStream;
    }

#pragma endregion

#pragma region TcpNetworkBlocking

    class TcpNetworkBlockingImpl : public TcpNetworkBlocking {
        friend class TcpNetworkManager;
        tcp_handle* handle;
        TaskMutex mutex;
        TcpError last_error;

        bool checkup() {
            if (!handle)
                return false;
            if (!handle->valid()) {
                last_error = handle->invalid_reason;
                delete handle;
                handle = nullptr;
                return false;
            }
            return true;
        }

    public:
        TcpNetworkBlockingImpl(tcp_handle* handle)
            : handle(handle), last_error(TcpError::none) {}

        ~TcpNetworkBlockingImpl() {
            std::lock_guard lg(mutex);
            if (handle)
                delete handle;
            handle = nullptr;
        }

        ValueItem read(uint32_t len) override {
            std::lock_guard lg(mutex);
            if (handle) {
                if (!checkup())
                    return nullptr;
                char* buf = new char[len];
                handle->read_force(len, buf);
                if (len == 0) {
                    delete[] buf;
                    return nullptr;
                }
                return ValueItem((uint8_t*)buf, len, no_copy);
            }
            return nullptr;
        }

        ValueItem available_bytes() override {
            std::lock_guard lg(mutex);
            if (handle)
                return handle->available_bytes();
            return (uint32_t)0;
        }

        ValueItem write(const char* data, uint32_t len) override {
            std::lock_guard lg(mutex);
            if (handle) {
                if (!checkup())
                    return nullptr;
                return handle->write_force(data, len);
            }
            return nullptr;
        }

        ValueItem write_file(char* path, size_t len, uint64_t data_len, uint64_t offset, uint32_t block_size) override {
            std::lock_guard lg(mutex);
            if (handle) {
                if (!checkup())
                    return nullptr;
                return handle->send_file(path, len, data_len, offset, block_size);
            }
            return nullptr;
        }

        ValueItem write_file(int fhandle, uint64_t data_len, uint64_t offset, uint32_t block_size) override {
            std::lock_guard lg(mutex);
            if (handle) {
                if (!checkup())
                    return nullptr;
                return handle->send_file(fhandle, data_len, offset, block_size);
            }
            return nullptr;
        }

        void close() override {
            std::lock_guard lg(mutex);
            if (handle) {
                handle->close();
                last_error = handle->invalid_reason;
                delete handle;
                handle = nullptr;
            }
        }

        void reset() override {
            std::lock_guard lg(mutex);
            if (handle) {
                handle->reset();
                last_error = handle->invalid_reason;
                delete handle;
                handle = nullptr;
            }
        }

        void rebuffer(size_t new_size) override {
            std::lock_guard lg(mutex);
            if (handle)
                handle->rebuffer(new_size);
        }

        bool is_closed() override {
            std::lock_guard lg(mutex);
            if (handle) {
                bool res = handle->valid();
                if (!res) {
                    last_error = handle->invalid_reason;
                    delete handle;
                    handle = nullptr;
                }
                return !res;
            }
            return true;
        }

        TcpError error() override {
            std::lock_guard lg(mutex);
            if (handle)
                return handle->invalid_reason;
            return last_error;
        }

        ValueItem local_address() override {
            std::lock_guard lg(mutex);
            if (!handle)
                return nullptr;
            universal_address addr;
            socklen_t socklen = sizeof(universal_address);
            if (getsockname(handle->socket, (sockaddr*)&addr, &socklen) == -1)
                return nullptr;
            return CXX::Interface::constructStructure<universal_address>(define_UniversalAddress, addr);
        }

        ValueItem remote_address() override {
            std::lock_guard lg(mutex);
            if (!handle)
                return nullptr;
            universal_address addr;
            socklen_t socklen = sizeof(universal_address);
            if (getpeername(handle->socket, (sockaddr*)&addr, &socklen) == -1)
                return nullptr;
            return CXX::Interface::constructStructure<universal_address>(define_UniversalAddress, addr);
        }
    };

    AttachAFun(funs_TcpNetworkBlocking_read, 2, {
        return CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).read((uint32_t)args[1]);
    });
    AttachAFun(funs_TcpNetworkBlocking_available_bytes, 1, {
        return CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).available_bytes();
    });
    AttachAFun(funs_TcpNetworkBlocking_write, 2, {
        if (args[1].meta.vtype != VType::raw_arr_ui8)
            throw InvalidArguments("The third argument must be a raw_arr_ui8.");
        if (len == 2)
            return CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).write((char*)args[1].getSourcePtr(), args[1].meta.val_len);
        else {
            uint32_t len = (uint32_t)args[2];
            if (len > args[1].meta.val_len)
                throw OutOfRange("The length of the data to be sent is greater than the length of array.");
            return CXX::Interface::getExtractAs<TcpNetworkBlocking>(args[0], define_TcpNetworkBlocking).write((char*)args[1].getSourcePtr(), len);
        }
    });
    AttachAFun(funs_TcpNetworkBlocking_write_file, 2, {
        ValueItem& arg1 = args[1];
        uint64_t data_len = 0;
        uint64_t offset = 0;
        uint32_t chunks_size = 0;
        if (arg1.meta.vtype != VType::struct_ && arg1.meta.vtype != VType::string)
            throw InvalidArguments("The second argument must be a file handle or a file path.");
        if (len >= 3)
            data_len = (uint64_t)args[2];
        if (len >= 4)
            offset = (uint64_t)args[3];
        if (len >= 5)
            chunks_size = (uint32_t)args[4];

        if (arg1.meta.vtype == VType::struct_) {
            auto& proxy = (Structure&)args[1];
            if (proxy.get_vtable()) {
                if (proxy.get_vtable() == CXX::Interface::typeVTable<typed_lgr<art::files::FileHandle>>())
                    return CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).write_file((*(typed_lgr<art::files::FileHandle>*)proxy.get_data_no_vtable())->internal_get_handle(), data_len, offset, chunks_size);
                else if (proxy.get_vtable() == CXX::Interface::typeVTable<typed_lgr<art::files::BlockingFileHandle>>())
                    return CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).write_file((*(typed_lgr<art::files::BlockingFileHandle>*)proxy.get_data_no_vtable())->internal_get_handle(), data_len, offset, chunks_size);
            }
            throw InvalidArguments("The second argument must be a file handle or a file path.");
        } else {
            art::ustring& path = *((art::ustring*)arg1.getSourcePtr());
            return CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).write_file(path.data(), path.size(), data_len, offset, chunks_size);
        }
    });
    AttachAFun(funs_TcpNetworkBlocking_close, 1, {
        CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).close();
    });
    AttachAFun(funs_TcpNetworkBlocking_reset, 1, {
        CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).reset();
    });
    AttachAFun(funs_TcpNetworkBlocking_rebuffer, 2, {
        CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).rebuffer((int32_t)args[1]);
    });
    AttachAFun(funs_TcpNetworkBlocking_is_closed, 1, {
        return CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).is_closed();
    });
    AttachAFun(funs_TcpNetworkBlocking_error, 1, {
        return (uint8_t)CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).error();
    });

    AttachAFun(funs_TcpNetworkBlocking_local_address, 1, {
        return CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).local_address();
    });
    AttachAFun(funs_TcpNetworkBlocking_remote_address, 1, {
        return CXX::Interface::getExtractAs<TcpNetworkBlockingImpl>(args[0], define_TcpNetworkBlocking).remote_address();
    });

    void init_define_TcpNetworkBlocking() {
        if (define_TcpNetworkBlocking != nullptr)
            return;
        define_TcpNetworkBlocking = CXX::Interface::createTable<TcpNetworkBlockingImpl>(
            "tcp_network_blocking",
            CXX::Interface::direct_method("read", funs_TcpNetworkBlocking_read),
            CXX::Interface::direct_method("available_bytes", funs_TcpNetworkBlocking_available_bytes),
            CXX::Interface::direct_method("write", funs_TcpNetworkBlocking_write),
            CXX::Interface::direct_method("write_file", funs_TcpNetworkBlocking_write_file),
            CXX::Interface::direct_method("close", funs_TcpNetworkBlocking_close),
            CXX::Interface::direct_method("reset", funs_TcpNetworkBlocking_reset),
            CXX::Interface::direct_method("rebuffer", funs_TcpNetworkBlocking_rebuffer),
            CXX::Interface::direct_method("is_closed", funs_TcpNetworkBlocking_is_closed),
            CXX::Interface::direct_method("error", funs_TcpNetworkBlocking_error),
            CXX::Interface::direct_method("local_address", funs_TcpNetworkBlocking_local_address),
            CXX::Interface::direct_method("remote_address", funs_TcpNetworkBlocking_remote_address)
        );
        CXX::Interface::typeVTable<TcpNetworkBlocking>() = define_TcpNetworkBlocking;
    }

#pragma endregion

    class TcpNetworkManager : public NativeWorkerManager {
        TaskMutex safety;
        art::shared_ptr<FuncEnvironment> handler_fn;
        art::shared_ptr<FuncEnvironment> accept_filter;
        sockaddr_in6 connectionAddress;
        SOCKET main_socket;

    public:
        TcpConfiguration config;

    private:
        bool allow_new_connections = false;
        bool disabled = true;
        bool corrupted = false;
        size_t acceptors;
        TcpNetworkServer::ManageType manage_type;
        TaskConditionVariable state_changed_cv;

        void make_acceptEx(void) {
            tcp_handle* pClientContext = new tcp_handle(0, config.buffer_size, this);
            NativeWorkersSingleton::post_accept(pClientContext, main_socket, nullptr, nullptr, 0);
        }

        ValueItem accept_manager_construct(tcp_handle* self) {
            switch (manage_type) {
            case TcpNetworkServer::ManageType::blocking:
                return ValueItem(CXX::Interface::constructStructure<TcpNetworkBlockingImpl>(define_TcpNetworkBlocking, self), no_copy);
            case TcpNetworkServer::ManageType::write_delayed:
                return ValueItem(CXX::Interface::constructStructure<TcpNetworkStreamImpl>(define_TcpNetworkStream, self), no_copy);
            default:
                return nullptr;
            }
        }

        void accepted(tcp_handle* self, ValueItem clientAddr, ValueItem localAddr) {
            if (!allow_new_connections) {
                delete self;
                return;
            }
            if (self->aerrno) {
                self->connection_reset();
                delete self;
                return;
            }
            std::lock_guard guard(safety);
            Task::start(
                new Task(handler_fn, ValueItem{accept_manager_construct(self), std::move(clientAddr), std::move(localAddr)}));
        }

        void accept_bounded(tcp_handle& data, SOCKET client_socket) {
            std::lock_guard guard(data.cv_mutex);
            data.socket = client_socket;
            data.cv.notify_all();
        }

        void new_connection(tcp_handle& data, SOCKET client_socket) {
            if (!allow_new_connections) {
                NativeWorkersSingleton::post_accept(&data, main_socket, nullptr, nullptr, 0);
                close(client_socket);
            }
            if (data.is_bound && !accept_filter)
                return accept_bounded(data, client_socket);
            if (!accept_filter)
                make_acceptEx();
            universal_address pClientAddr;
            universal_address pLocalAddr;
            socklen_t remoteLen = sizeof(universal_address);
            socklen_t localLen = sizeof(universal_address);
            getsockname(client_socket, (sockaddr*)&pLocalAddr, &localLen);
            getpeername(client_socket, (sockaddr*)&pClientAddr, &remoteLen);

            ValueItem clientAddress(CXX::Interface::constructStructure<universal_address>(define_UniversalAddress, pClientAddr), no_copy);
            ValueItem localAddress(CXX::Interface::constructStructure<universal_address>(define_UniversalAddress, pLocalAddr), no_copy);
            if (accept_filter) {
                if (CXX::cxxCall(accept_filter, clientAddress, localAddress)) {
                    NativeWorkersSingleton::post_accept(&data, main_socket, nullptr, nullptr, 0);
                    close(client_socket);
#ifndef DISABLE_RUNTIME_INFO
                    auto tmp = UniversalAddress::_define_to_string(&clientAddress, 1);
                    ValueItem notify{"Client: " + (art::ustring)*tmp + " not accepted due filter"};
                    delete tmp;
                    info.async_notify(notify);
#endif
                    return;
                }
                make_acceptEx();
            }

#ifndef DISABLE_RUNTIME_INFO
            {
                auto tmp = UniversalAddress::_define_to_string(&clientAddress, 1);
                ValueItem notify{"Client connected from: " + (art::ustring)*tmp};
                delete tmp;
                info.async_notify(notify);
            }
#endif
            if (data.is_bound)
                accept_bounded(data, client_socket);
            else {
                data.socket = client_socket;
                accepted(&data, std::move(clientAddress), std::move(localAddress));
            }
            return;
        }

        void make_socket() {
            main_socket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
            if (main_socket == INVALID_SOCKET) {
                ValueItem error = art::ustring("Failed create socket: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            int argp = 1;
            if (setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, &argp, sizeof(argp)) == -1) {
                ValueItem error = art::ustring("Failed set reuse addr: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            if (setsockopt(main_socket, SOL_SOCKET, SO_REUSEPORT, &argp, sizeof(argp)) == -1) {
                ValueItem error = art::ustring("Failed set reuse port: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }


            int cfg = !config.allow_ip4;
            if (setsockopt(main_socket, IPPROTO_IPV6, IPV6_V6ONLY, &cfg, sizeof(cfg)) == -1) {
                ValueItem error = art::ustring("Failed set dual mode: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            cfg = !config.enable_timestamps;
            if (setsockopt(main_socket, IPPROTO_TCP, TCP_TIMESTAMP, &cfg, sizeof(cfg)) == -1) {
                if (errno != 1) {
                    ValueItem error = art::ustring("Failed set timestamps mode: ") + std::to_string(errno);
                    errors.sync_notify(error);
                    corrupted = true;
                    return;
                }
            }
            cfg = !config.enable_delay;
            if (setsockopt(main_socket, IPPROTO_TCP, TCP_NODELAY, &cfg, sizeof(cfg)) == -1) {
                ValueItem error = art::ustring("Failed set delay mode: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            cfg = config.fast_open_queue;
            if (setsockopt(main_socket, IPPROTO_TCP, TCP_FASTOPEN, &cfg, sizeof(cfg))) {
                ValueItem warn = art::ustring("Failed set fast open settings for server (") + std::to_string(errno) + "), continue regular mode";
                warning.async_notify(warn);
            }
            struct timeval cfgt = {};
            cfgt.tv_sec = config.recv_timeout_ms / 1000;
            cfgt.tv_usec = (config.recv_timeout_ms % 1000) * 1000;
            if (setsockopt(main_socket, SOL_SOCKET, SO_RCVTIMEO, &cfgt, sizeof(cfgt)) == -1) {
                ValueItem error = art::ustring("Failed set recv timeout: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            cfgt.tv_sec = config.send_timeout_ms / 1000;
            cfgt.tv_usec = (config.send_timeout_ms % 1000) * 1000;
            if (setsockopt(main_socket, SOL_SOCKET, SO_SNDTIMEO, &cfgt, sizeof(cfgt)) == -1) {
                ValueItem error = art::ustring("Failed set recv timeout: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            cfg = config.enable_keep_alive;
            if (setsockopt(main_socket, SOL_SOCKET, SO_KEEPALIVE, &cfg, sizeof(cfg)) == -1) {
                ValueItem error = art::ustring("Failed to enable keep alive: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            if (config.enable_keep_alive) {
                int cfg = config.keep_alive_settings.idle_ms;
                if (setsockopt(main_socket, IPPROTO_TCP, TCP_KEEPIDLE, &cfg, sizeof(cfg)) == -1) {
                    ValueItem error = art::ustring("Failed set keep idle: ") + std::to_string(errno);
                    errors.sync_notify(error);
                    corrupted = true;
                    return;
                }
                cfg = config.keep_alive_settings.interval_ms;
                if (setsockopt(main_socket, IPPROTO_TCP, TCP_KEEPINTVL, &cfg, sizeof(cfg)) == -1) {
                    ValueItem error = art::ustring("Failed set keep alive interval: ") + std::to_string(errno);
                    errors.sync_notify(error);
                    corrupted = true;
                    return;
                }
                cfg = config.keep_alive_settings.retry_count;
                if (setsockopt(main_socket, IPPROTO_TCP, TCP_KEEPCNT, &cfg, sizeof(cfg)) == -1) {
                    ValueItem error = art::ustring("Failed set keep alive retry count: ") + std::to_string(errno);
                    errors.sync_notify(error);
                    corrupted = true;
                    return;
                }
                cfg = config.keep_alive_settings.user_timeout_ms;
                if (setsockopt(main_socket, IPPROTO_TCP, TCP_USER_TIMEOUT, &cfg, sizeof(cfg)) == -1) {
                    ValueItem error = art::ustring("Failed set user timeout: ") + std::to_string(errno);
                    errors.sync_notify(error);
                    corrupted = true;
                    return;
                }
            }
            if (bind(main_socket, (sockaddr*)&connectionAddress, sizeof(sockaddr_in6)) == -1) {
                ValueItem error = art::ustring("Failed bind: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            if (listen(main_socket, SOMAXCONN) == -1) {
                ValueItem error = art::ustring("Failed bind: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
        }

    public:
        TcpNetworkManager(universal_address& ip_port, size_t acceptors, TcpNetworkServer::ManageType manage_type, TcpConfiguration config)
            : acceptors(acceptors), manage_type(manage_type), config(config) {
            memcpy(&connectionAddress, &ip_port, sizeof(sockaddr_in6));
        }

        ~TcpNetworkManager() {
            shutdown();
        }

        void handle(NativeWorkerHandle* completion, io_uring_cqe* cqe) override {
            auto& data = *(tcp_handle*)completion;
            if (cqe->res < 0)
                data.aerrno = -cqe->res;
            if (data.opcode == tcp_handle::Opcode::ACCEPT)
                new_connection(data, cqe->res);
            else
                data.handle(cqe->res < 0 ? 0 : cqe->res, cqe->res < 0 ? -cqe->res : 0);
        }

        void set_on_connect(art::shared_ptr<FuncEnvironment> handler_fn, TcpNetworkServer::ManageType manage_type) {
            if (corrupted)
                throw AttachARuntimeException("TcpNetworkManager is corrupted");
            std::lock_guard lock(safety);
            this->handler_fn = handler_fn;
            this->manage_type = manage_type;
        }

        void shutdown() {
            if (corrupted)
                throw AttachARuntimeException("TcpNetworkManager is corrupted");
            std::lock_guard lock(safety);
            if (disabled)
                return;
            tcp_handle* data = new tcp_handle(main_socket, 0, this, 0);
            MutexUnify mutex(data->cv_mutex);
            art::unique_lock lock2(mutex);
            NativeWorkersSingleton::post_shutdown(data, main_socket, SHUT_RDWR);
            data->cv.wait(lock2);
            allow_new_connections = false;
            disabled = true;
            state_changed_cv.notify_all();
        }

        void pause() {
            if (corrupted)
                throw AttachARuntimeException("TcpNetworkManager is corrupted");
            allow_new_connections = false;
        }

        void resume() {
            if (corrupted)
                throw AttachARuntimeException("TcpNetworkManager is corrupted");
            allow_new_connections = true;
        }

        void start() {
            if (corrupted)
                throw AttachARuntimeException("TcpNetworkManager is corrupted");
            std::lock_guard lock(safety);
            allow_new_connections = true;
            if (!disabled)
                return;
            make_socket();
            if (corrupted)
                return;
            for (size_t i = 0; i < acceptors; i++)
                make_acceptEx();
            disabled = false;
            state_changed_cv.notify_all();
        }

        ValueItem accept(bool ignore_acceptors = false) {
            if (!ignore_acceptors && acceptors)
                throw AttachARuntimeException("Tried to accept connection with enabled acceptors and ignore_acceptors = false");
            if (corrupted)
                throw AttachARuntimeException("TcpNetworkManager is corrupted");
            if (disabled)
                throw AttachARuntimeException("TcpNetworkManager is disabled");
            if (!allow_new_connections)
                throw AttachARuntimeException("TcpNetworkManager is paused");

            tcp_handle* data = new tcp_handle(0, config.buffer_size, this, 0);
            MutexUnify mutex(data->cv_mutex);
            art::unique_lock lock(mutex);
            data->is_bound = true;
            data->opcode = tcp_handle::Opcode::ACCEPT;
            NativeWorkersSingleton::post_accept(data, main_socket, nullptr, nullptr, 0);
            data->cv.wait(lock);
            return accept_manager_construct(data);
        }

        void _await() {
            MutexUnify um(safety);
            art::unique_lock lock(um);
            if (corrupted)
                throw AttachARuntimeException("TcpNetworkManager is corrupted");
            while (!disabled)
                state_changed_cv.wait(lock);
        }

        void set_configuration(TcpConfiguration config) {
            if (corrupted)
                throw AttachARuntimeException("TcpNetworkManager is corrupted");
            this->config = config;
        }

        void set_accept_filter(art::shared_ptr<FuncEnvironment> filter) {
            if (corrupted)
                throw AttachARuntimeException("TcpNetworkManager is corrupted");
            std::lock_guard lock(safety);
            this->accept_filter = filter;
        }

        bool is_corrupted() {
            return corrupted;
        }

        uint16_t port() {
            if (corrupted)
                throw AttachARuntimeException("TcpNetworkManager is corrupted");
            return htons(connectionAddress.sin6_port);
        }

        art::ustring ip() {
            if (corrupted)
                throw AttachARuntimeException("TcpNetworkManager is corrupted");
            Structure* tmp = CXX::Interface::constructStructure<universal_address>(define_UniversalAddress);
            memcpy(tmp->get_data_no_vtable(), &connectionAddress, sizeof(sockaddr_in6));
            tmp->fully_constructed = true;
            ValueItem args(tmp, as_reference);
            ValueItem* res;
            try {
                res = UniversalAddress::_define_to_string(&args, 1);
            } catch (...) {
                Structure::destruct(tmp);
                throw;
            }
            Structure::destruct(tmp);
            art::ustring ret = (art::ustring)*res;
            delete res;
            return ret;
        }

        ValueItem address() {
            if (corrupted)
                throw AttachARuntimeException("TcpNetworkManager is corrupted");

            sockaddr_storage* addr = new sockaddr_storage;
            memcpy(addr, &connectionAddress, sizeof(sockaddr_in6));
            memset(((char*)addr) + sizeof(sockaddr_in6), 0, sizeof(sockaddr_storage) - sizeof(sockaddr_in6));
            return ValueItem(CXX::Interface::constructStructure<universal_address>(define_UniversalAddress, *addr), no_copy);
        }

        bool is_paused() {
            return !disabled && !allow_new_connections;
        }

        bool in_run() {
            return !disabled;
        }
    };

    class TcpClientManager : public NativeWorkerManager {
        TaskMutex mutex;
        sockaddr_in6 connectionAddress;
        tcp_handle* _handle;
        bool corrupted = false;

    public:
        void handle(NativeWorkerHandle* overlapped, io_uring_cqe* cqe) override {
            tcp_handle& handle = *(tcp_handle*)overlapped;
            if (cqe->res < 0)
                handle.aerrno = -cqe->res;

            if (handle.opcode == tcp_handle::Opcode::ACCEPT)
                handle.cv.notify_all();
            else
                handle.handle(cqe->res, cqe->res < 0 ? -cqe->res : 0);
        }

        TcpClientManager(sockaddr_in6& _connectionAddress, TcpConfiguration config)
            : connectionAddress(_connectionAddress) {
            SOCKET clientSocket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
            if (clientSocket == INVALID_SOCKET) {
                corrupted = true;
                return;
            }
            int cfg = config.recv_timeout_ms;
            if (setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &cfg, sizeof(cfg)) == -1) {
                ValueItem error = art::ustring("Failed set recv timeout: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            cfg = config.connection_timeout_ms ? config.connection_timeout_ms : config.recv_timeout_ms;
            if (setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, &cfg, sizeof(cfg)) == -1) {
                ValueItem error = art::ustring("Failed set recv timeout: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            cfg = config.enable_keep_alive;
            if (setsockopt(clientSocket, SOL_SOCKET, SO_KEEPALIVE, &cfg, sizeof(cfg)) == -1) {
                ValueItem error = art::ustring("Failed to enable keep alive: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            if (config.enable_keep_alive) {
                int cfg = config.keep_alive_settings.idle_ms;
                if (setsockopt(clientSocket, IPPROTO_TCP, TCP_KEEPIDLE, &cfg, sizeof(cfg)) == -1) {
                    ValueItem error = art::ustring("Failed set keep idle: ") + std::to_string(errno);
                    errors.sync_notify(error);
                    corrupted = true;
                    return;
                }
                cfg = config.keep_alive_settings.interval_ms;
                if (setsockopt(clientSocket, IPPROTO_TCP, TCP_KEEPINTVL, &cfg, sizeof(cfg)) == -1) {
                    ValueItem error = art::ustring("Failed set keep alive interval: ") + std::to_string(errno);
                    errors.sync_notify(error);
                    corrupted = true;
                    return;
                }
                cfg = config.keep_alive_settings.retry_count;
                if (setsockopt(clientSocket, IPPROTO_TCP, TCP_KEEPCNT, &cfg, sizeof(cfg)) == -1) {
                    ValueItem error = art::ustring("Failed set keep alive count: ") + std::to_string(errno);
                    errors.sync_notify(error);
                    corrupted = true;
                    return;
                }
                cfg = config.keep_alive_settings.user_timeout_ms;
                if (setsockopt(clientSocket, IPPROTO_TCP, TCP_USER_TIMEOUT, &cfg, sizeof(cfg)) == -1) {
                    ValueItem error = art::ustring("Failed set user timeout: ") + std::to_string(errno);
                    errors.sync_notify(error);
                    corrupted = true;
                    return;
                }
            }
            int argp = 1;
            if (ioctl(clientSocket, FIONBIO, &argp) == -1) {
                ValueItem error = art::ustring("Failed set no block mode: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            _handle = new tcp_handle(clientSocket, config.buffer_size, this);
            MutexUnify umutex(_handle->cv_mutex);
            art::unique_lock<MutexUnify> lock(umutex);
            NativeWorkersSingleton::post_connect(_handle, clientSocket, (sockaddr*)&connectionAddress, sizeof(connectionAddress));
            if (config.connection_timeout_ms > 0) {
                if (!_handle->cv.wait_for(lock, config.connection_timeout_ms)) {
                    corrupted = true;
                    _handle->reset();
                    return;
                }
            } else
                _handle->cv.wait(lock);
            cfg = config.send_timeout_ms;
            if (setsockopt(clientSocket, IPPROTO_TCP, SO_SNDTIMEO, &cfg, sizeof(cfg)) == -1) {
                ValueItem error = art::ustring("Failed set user timeout: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
        }

        TcpClientManager(sockaddr_in6& _connectionAddress, char* data, uint32_t len, TcpConfiguration config)
            : connectionAddress(_connectionAddress), _handle(nullptr) {
            SOCKET clientSocket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
            if (clientSocket == INVALID_SOCKET) {
                corrupted = true;
                return;
            }
            int argp = 1;
            int cfg = config.recv_timeout_ms;
            if (setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &cfg, sizeof(cfg)) == -1) {
                ValueItem error = art::ustring("Failed set recv timeout: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            cfg = config.send_timeout_ms;
            if (setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, &cfg, sizeof(cfg)) == -1) {
                ValueItem error = art::ustring("Failed set recv timeout: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            cfg = config.enable_keep_alive;
            if (setsockopt(clientSocket, SOL_SOCKET, SO_KEEPALIVE, &cfg, sizeof(cfg)) == -1) {
                ValueItem error = art::ustring("Failed to enable keep alive: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            if (config.enable_keep_alive) {
                int cfg = config.keep_alive_settings.idle_ms;
                if (setsockopt(clientSocket, IPPROTO_TCP, TCP_KEEPIDLE, &cfg, sizeof(cfg)) == -1) {
                    ValueItem error = art::ustring("Failed set keep idle: ") + std::to_string(errno);
                    errors.sync_notify(error);
                    corrupted = true;
                    return;
                }
                cfg = config.keep_alive_settings.interval_ms;
                if (setsockopt(clientSocket, IPPROTO_TCP, TCP_KEEPINTVL, &cfg, sizeof(cfg)) == -1) {
                    ValueItem error = art::ustring("Failed set keep alive interval: ") + std::to_string(errno);
                    errors.sync_notify(error);
                    corrupted = true;
                    return;
                }
                cfg = config.keep_alive_settings.retry_count;
                if (setsockopt(clientSocket, IPPROTO_TCP, TCP_KEEPCNT, &cfg, sizeof(cfg)) == -1) {
                    ValueItem error = art::ustring("Failed set keep alive count: ") + std::to_string(errno);
                    errors.sync_notify(error);
                    corrupted = true;
                    return;
                }
                cfg = config.keep_alive_settings.user_timeout_ms;
                if (setsockopt(clientSocket, IPPROTO_TCP, TCP_USER_TIMEOUT, &cfg, sizeof(cfg)) == -1) {
                    ValueItem error = art::ustring("Failed set user timeout: ") + std::to_string(errno);
                    errors.sync_notify(error);
                    corrupted = true;
                    return;
                }
            }
            if (ioctl(clientSocket, FIONBIO, &argp) == -1) {
                ValueItem error = art::ustring("Failed set no block mode: ") + std::to_string(errno);
                errors.sync_notify(error);
                corrupted = true;
                return;
            }
            _handle = new tcp_handle(clientSocket, 4096, this);
            char* old_buffer = _handle->data;
            _handle->data = data;
            _handle->buffer.buf = data;
            _handle->buffer.len = len;
            _handle->total_bytes = len;
            _handle->opcode = tcp_handle::Opcode::WRITE;
            MutexUnify umutex(_handle->cv_mutex);
            art::unique_lock<MutexUnify> lock(umutex);
            NativeWorkersSingleton::post_sendto(_handle, clientSocket, _handle->buffer.buf, _handle->buffer.len, MSG_FASTOPEN, (sockaddr*)&connectionAddress, sizeof(connectionAddress));
            if (config.connection_timeout_ms > 0) {
                if (!_handle->cv.wait_for(lock, config.connection_timeout_ms)) {
                    corrupted = true;
                    _handle->data = old_buffer;
                    _handle->reset();
                    return;
                }
            } else
                _handle->cv.wait(lock);
            _handle->data = old_buffer;
        }

        ~TcpClientManager() override {
            if (corrupted)
                return;
            delete _handle;
        }

        void set_configuration(TcpConfiguration config) {
            if (corrupted)
                throw std::runtime_error("TcpClientManager::set_configuration, corrupted");
            if (_handle) {
                SOCKET clientSocket = _handle->socket;
                int cfg = config.recv_timeout_ms;
                if (setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &cfg, sizeof(cfg)) == -1) {
                    ValueItem error = art::ustring("Failed set recv timeout: ") + std::to_string(errno);
                    errors.sync_notify(error);
                    corrupted = true;
                    return;
                }
                cfg = config.send_timeout_ms;
                if (setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, &cfg, sizeof(cfg)) == -1) {
                    ValueItem error = art::ustring("Failed set recv timeout: ") + std::to_string(errno);
                    errors.sync_notify(error);
                    corrupted = true;
                    return;
                }
                cfg = config.enable_keep_alive;
                if (setsockopt(clientSocket, SOL_SOCKET, SO_KEEPALIVE, &cfg, sizeof(cfg)) == -1) {
                    ValueItem error = art::ustring("Failed to enable keep alive: ") + std::to_string(errno);
                    errors.sync_notify(error);
                    corrupted = true;
                    return;
                }
                if (config.enable_keep_alive) {
                    int cfg = config.keep_alive_settings.idle_ms;
                    if (setsockopt(clientSocket, IPPROTO_TCP, TCP_KEEPIDLE, &cfg, sizeof(cfg)) == -1) {
                        ValueItem error = art::ustring("Failed set keep idle: ") + std::to_string(errno);
                        errors.sync_notify(error);
                        corrupted = true;
                        return;
                    }
                    cfg = config.keep_alive_settings.interval_ms;
                    if (setsockopt(clientSocket, IPPROTO_TCP, TCP_KEEPINTVL, &cfg, sizeof(cfg)) == -1) {
                        ValueItem error = art::ustring("Failed set keep alive interval: ") + std::to_string(errno);
                        errors.sync_notify(error);
                        corrupted = true;
                        return;
                    }
                    cfg = config.keep_alive_settings.retry_count;
                    if (setsockopt(clientSocket, IPPROTO_TCP, TCP_KEEPCNT, &cfg, sizeof(cfg)) == -1) {
                        ValueItem error = art::ustring("Failed set keep alive count: ") + std::to_string(errno);
                        errors.sync_notify(error);
                        corrupted = true;
                        return;
                    }
                    cfg = config.keep_alive_settings.user_timeout_ms;
                    if (setsockopt(clientSocket, IPPROTO_TCP, TCP_USER_TIMEOUT, &cfg, sizeof(cfg)) == -1) {
                        ValueItem error = art::ustring("Failed set user timeout: ") + std::to_string(errno);
                        errors.sync_notify(error);
                        corrupted = true;
                        return;
                    }
                }
                if (config.buffer_size > 0 && config.buffer_size != _handle->data_len) {
                    if (config.buffer_size != (int)config.buffer_size)
                        config.buffer_size = INT_MAX;
                    _handle->data_len = config.buffer_size;
                    delete[] _handle->data;
                    _handle->data = new char[_handle->data_len];
                    _handle->buffer.buf = _handle->data;
                    _handle->buffer.len = _handle->data_len;
                }
            }
        }

        int32_t read(char* data, int32_t len) {
            if (corrupted)
                throw std::runtime_error("TcpClientManager::read, corrupted");
            std::lock_guard<TaskMutex> lock(mutex);
            int32_t readed = 0;
            while (!_handle->available_bytes())
                if (!_handle->send_queue_item())
                    break;
            _handle->read_available(data, len, readed);
            return readed;
        }

        bool write(const char* data, int32_t len) {
            if (corrupted)
                throw std::runtime_error("TcpClientManager::write, corrupted");
            std::lock_guard<TaskMutex> lock(mutex);
            _handle->send_data(data, len);
            while (!_handle->available_bytes())
                if (!_handle->send_queue_item())
                    break;
            return _handle->valid();
        }

        bool write_file(const char* path, size_t len, uint64_t data_len, uint64_t offset, uint32_t chunks_size) {
            if (corrupted)
                throw std::runtime_error("TcpClientManager::write_file, corrupted");
            std::lock_guard<TaskMutex> lock(mutex);
            while (!_handle->available_bytes())
                if (!_handle->send_queue_item())
                    break;
            return _handle->send_file(path, len, data_len, offset, chunks_size);
        }

        bool write_file(int handle, uint64_t data_len, uint64_t offset, uint32_t chunks_size) {
            if (corrupted)
                throw std::runtime_error("TcpClientManager::write_file, corrupted");
            std::lock_guard<TaskMutex> lock(mutex);
            while (!_handle->available_bytes())
                if (!_handle->send_queue_item())
                    break;
            return _handle->send_file(handle, data_len, offset, chunks_size);
        }

        void close() {
            if (corrupted)
                throw std::runtime_error("TcpClientManager::close, corrupted");
            std::lock_guard<TaskMutex> lock(mutex);
            _handle->close();
        }

        void reset() {
            if (corrupted)
                throw std::runtime_error("TcpClientManager::close, corrupted");
            std::lock_guard<TaskMutex> lock(mutex);
            _handle->reset();
        }

        bool is_corrupted() {
            return corrupted;
        }

        void rebuffer(uint32_t size) {
            if (corrupted)
                throw std::runtime_error("TcpClientManager::rebuffer, corrupted");
            std::lock_guard<TaskMutex> lock(mutex);
            _handle->rebuffer(size);
        }
    };

    class udp_handle : public NativeWorkerHandle, public NativeWorkerManager {
        art::typed_lgr<Task> notify_task;
        SOCKET socket;
        sockaddr_in6 server_address;

    public:
        uint32_t fullifed_bytes;
        uint32_t last_error;

        udp_handle(sockaddr_in6& address, uint32_t timeout_ms)
            : NativeWorkerHandle(this), last_error(0), fullifed_bytes(0) {
            socket = ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
            if (socket == INVALID_SOCKET)
                return;
            if (bind(socket, (sockaddr*)&address, sizeof(sockaddr_in6)) == -1) {
                close(socket);
                socket = INVALID_SOCKET;
                return;
            }
            server_address = address;
        }

        void handle(NativeWorkerHandle* overlapped, io_uring_cqe* cqe) override {
            this->fullifed_bytes = cqe->res > -1 ? cqe->res : 0;
            this->last_error = cqe->res > -1 ? 0 : errno;
            Task::start(notify_task);
        }

        void recv(uint8_t* data, uint32_t size, sockaddr_storage& sender, int& sender_len) {
            if (socket == INVALID_SOCKET)
                throw InvalidOperation("Socket not connected");
            notify_task = Task::dummy_task();
            socklen_t sender_len_ = 0;
            NativeWorkersSingleton::post_recvfrom(this, socket, data, size, 0, (sockaddr*)&sender, &sender_len_);
            Task::await_task(notify_task);
            sender_len = sender_len_;
            notify_task = nullptr;
        }

        void send(uint8_t* data, uint32_t size, sockaddr_storage& to) {
            if (socket == INVALID_SOCKET)
                throw InvalidOperation("Socket not connected");
            notify_task = Task::dummy_task();
            NativeWorkersSingleton::post_sendto(this, socket, data, size, 0, (sockaddr*)&to, sizeof(sockaddr_in6));
            Task::await_task(notify_task);
            notify_task = nullptr;
        }

        ValueItem local_address() {
            universal_address addr;
            socklen_t socklen = sizeof(universal_address);
            if (getsockname(socket, (sockaddr*)&addr, &socklen) == -1)
                return nullptr;
            return CXX::Interface::constructStructure<universal_address>(define_UniversalAddress, addr);
        }

        ValueItem remote_address() {
            universal_address addr;
            socklen_t socklen = sizeof(universal_address);
            if (getpeername(socket, (sockaddr*)&addr, &socklen) == -1)
                return nullptr;
            return CXX::Interface::constructStructure<universal_address>(define_UniversalAddress, addr);
        }
    };

#pragma endregion

    uint8_t init_networking() {
        init_define_UniversalAddress();
        init_define_TcpConfiguration();
        init_define_TcpNetworkStream();
        init_define_TcpNetworkBlocking();
        inited = true;
        return 0;
    }

    void deinit_networking() {
        inited = false;
    }
#else
#error Unsupported platform
#endif

    TcpNetworkServer::TcpNetworkServer(art::shared_ptr<FuncEnvironment> on_connect, const ValueItem& ip_port, ManageType mt, size_t acceptors, TcpConfiguration config) {
        if (!inited)
            throw InternalException("Network module not initialized");
        sockaddr_storage address;
        get_address_from_valueItem(ip_port, address);
        handle = new TcpNetworkManager(address, acceptors, mt, config);
        handle->set_on_connect(on_connect, mt);
    }

    TcpNetworkServer::~TcpNetworkServer() {
        if (handle)
            delete handle;
        handle = nullptr;
    }

    void TcpNetworkServer::start() {
        handle->start();
    }

    void TcpNetworkServer::pause() {
        handle->pause();
    }

    void TcpNetworkServer::resume() {
        handle->resume();
    }

    void TcpNetworkServer::stop() {
        handle->shutdown();
    }

    bool TcpNetworkServer::is_running() {
        return handle->in_run();
    }

    ValueItem TcpNetworkServer::accept(bool ignore_acceptors) {
        return handle->accept(ignore_acceptors);
    }

    void TcpNetworkServer::_await() {
        handle->_await();
    }

    bool TcpNetworkServer::is_corrupted() {
        return handle->is_corrupted();
    }

    uint16_t TcpNetworkServer::server_port() {
        return handle->port();
    }

    art::ustring TcpNetworkServer::server_ip() {
        return handle->ip();
    }

    ValueItem TcpNetworkServer::server_address() {
        return handle->address();
    }

    bool TcpNetworkServer::is_paused() {
        return handle->is_paused();
    }

    void TcpNetworkServer::set_configuration(TcpConfiguration config) {
        if (handle)
            handle->set_configuration(config);
    }

    void TcpNetworkServer::set_accept_filter(art::shared_ptr<FuncEnvironment> filter) {
        if (handle)
            handle->set_accept_filter(filter);
    }

    TcpClientSocket::TcpClientSocket()
        : handle(nullptr) {}

    TcpClientSocket::~TcpClientSocket() {
        if (handle)
            delete handle;
        handle = nullptr;
    }

    TcpClientSocket* TcpClientSocket::connect(const ValueItem& ip_port, TcpConfiguration configuration) {
        if (!inited)
            throw InternalException("Network module not initialized");
        sockaddr_storage address;
        get_address_from_valueItem(ip_port, address);
        std::unique_ptr<TcpClientSocket> result;
        result.reset(new TcpClientSocket());
        result->handle = new TcpClientManager((sockaddr_in6&)address, configuration);
        return result.release();
    }

    TcpClientSocket* TcpClientSocket::connect(const ValueItem& ip_port, char* data, uint32_t size, TcpConfiguration configuration) {
        if (!inited)
            throw InternalException("Network module not initialized");
        sockaddr_storage address;
        get_address_from_valueItem(ip_port, address);
        std::unique_ptr<TcpClientSocket> result;
        result.reset(new TcpClientSocket());
        result->handle = new TcpClientManager((sockaddr_in6&)address, data, size, configuration);
        return result.release();
    }

    void TcpClientSocket::set_configuration(TcpConfiguration config) {
        if (handle)
            handle->set_configuration(config);
    }

    int32_t TcpClientSocket::recv(uint8_t* data, int32_t size) {
        if (!inited)
            throw InternalException("Network module not initialized");

        if (handle)
            return handle->read((char*)data, size);
        return 0;
    }

    bool TcpClientSocket::send(uint8_t* data, int32_t size) {
        if (!inited)
            throw InternalException("Network module not initialized");
        if (handle)
            return handle->write((char*)data, size);
        return false;
    }

    bool TcpClientSocket::send_file(const char* file_path, size_t file_path_len, uint64_t data_len, uint64_t offset, uint32_t chunks_size) {
        if (!handle)
            throw InvalidOperation("Socket not connected");
        return handle->write_file(file_path, file_path_len, data_len, offset, chunks_size);
    }

    bool TcpClientSocket::send_file(class art::files::FileHandle& file, uint64_t data_len, uint64_t offset, uint32_t chunks_size) {
        if (!handle)
            throw InvalidOperation("Socket not connected");
        return handle->write_file(file.internal_get_handle(), data_len, offset, chunks_size);
    }

    bool TcpClientSocket::send_file(class art::files::BlockingFileHandle& file, uint64_t data_len, uint64_t offset, uint32_t chunks_size) {
        if (!handle)
            throw InvalidOperation("Socket not connected");
        return handle->write_file(file.internal_get_handle(), data_len, offset, chunks_size);
    }

    void TcpClientSocket::close() {
        if (handle) {
            handle->close();
            delete handle;
            handle = nullptr;
        }
    }

    void TcpClientSocket::reset() {
        if (handle) {
            handle->reset();
            delete handle;
            handle = nullptr;
        }
    }

    void TcpClientSocket::rebuffer(int32_t size) {
        if (handle)
            handle->rebuffer(size);
    }

    udp_socket::udp_socket(const ValueItem& ip_port, uint32_t timeout_ms) {
        sockaddr_storage address;
        get_address_from_valueItem(ip_port, address);
        handle = new udp_handle((sockaddr_in6&)address, timeout_ms);
    }

    udp_socket::~udp_socket() {
        if (handle)
            delete handle;
    }

    uint32_t udp_socket::recv(uint8_t* data, uint32_t size, ValueItem& sender) {
        sockaddr_storage sender_address;
        int sender_len = sizeof(sender_address);
        handle->recv(data, size, sender_address, sender_len);
        if (handle->fullifed_bytes == 0 && handle->last_error != 0)
            throw AttachARuntimeException("Error while receiving data from udp socket with error code: " + std::to_string(handle->last_error));
        sender = ValueItem(CXX::Interface::constructStructure<universal_address>(define_UniversalAddress, sender_address), no_copy);
        return handle->fullifed_bytes;
    }

    uint32_t udp_socket::send(uint8_t* data, uint32_t size, ValueItem& to) {
        sockaddr_storage to_ip_port;
        get_address_from_valueItem(to, to_ip_port);
        handle->send(data, size, to_ip_port);
        if (handle->fullifed_bytes == 0 && handle->last_error != 0)
            throw AttachARuntimeException("Error while receiving data from udp socket with error code: " + std::to_string(handle->last_error));
        return handle->fullifed_bytes;
    }

    ValueItem udp_socket::local_address() {
        if (!handle)
            return nullptr;
        return handle->local_address();
    }

    ValueItem udp_socket::remote_address() {
        if (!handle)
            return nullptr;
        return handle->remote_address();
    }

    bool ipv6_supported() {
        if (!inited)
            throw InternalException("Network module not initialized");
        static int ipv6_supported = -1;
        if (ipv6_supported == -1) {
            ipv6_supported = 0;
            SOCKET sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
            if (sock != INVALID_SOCKET) {
                ipv6_supported = 1;
#ifdef _WIN32
                closesocket(sock);
#else
                close(sock);
#endif
            }
        }
        return ipv6_supported == 1;
    }

    ValueItem makeIP4(const char* ip, uint16_t port) {
        if (!inited)
            throw InternalException("Network module not initialized");
        ValueItem clientAddress(CXX::Interface::constructStructure<universal_address>(define_UniversalAddress), no_copy);
        internal_makeIP4(CXX::Interface::getAs<universal_address>(clientAddress), ip, port);
        return clientAddress;
    }

    ValueItem makeIP6(const char* ip, uint16_t port) {
        if (!inited)
            throw InternalException("Network module not initialized");
        ValueItem clientAddress(CXX::Interface::constructStructure<universal_address>(define_UniversalAddress), no_copy);
        internal_makeIP6(CXX::Interface::getAs<universal_address>(clientAddress), ip, port);
        return clientAddress;
    }

    ValueItem makeIP(const char* ip, uint16_t port) {
        if (!inited)
            throw InternalException("Network module not initialized");
        ValueItem clientAddress(CXX::Interface::constructStructure<universal_address>(define_UniversalAddress), no_copy);
        internal_makeIP(CXX::Interface::getAs<universal_address>(clientAddress), ip, port);
        return clientAddress;
    }

    ValueItem makeIP4_port(const char* ip_port) {
        if (!inited)
            throw InternalException("Network module not initialized");
        ValueItem clientAddress(CXX::Interface::constructStructure<universal_address>(define_UniversalAddress), no_copy);
        internal_makeIP4_port(CXX::Interface::getAs<universal_address>(clientAddress), ip_port);
        return clientAddress;
    }

    ValueItem makeIP6_port(const char* ip_port) {
        if (!inited)
            throw InternalException("Network module not initialized");
        ValueItem clientAddress(CXX::Interface::constructStructure<universal_address>(define_UniversalAddress), no_copy);
        internal_makeIP6_port(CXX::Interface::getAs<universal_address>(clientAddress), ip_port);
        return clientAddress;
    }

    ValueItem makeIP_port(const char* ip_port) {
        if (ip_port[0] == '[')
            return makeIP6_port(ip_port);
        else
            return makeIP4_port(ip_port);
    }
}