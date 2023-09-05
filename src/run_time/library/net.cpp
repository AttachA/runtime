// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/AttachA_CXX.hpp>
#include <run_time/cxx_library/networking.hpp>

namespace art {
    namespace Ai = CXX::Interface;

    namespace net {
        AttachAVirtualTable* define_TcpNetworkServer;
        AttachAVirtualTable* define_TcpClientSocket;
        AttachAVirtualTable* define_UdpSocket;
#pragma region TcpNetworkServer
        AttachAFun(funs_TcpNetworkServer_start, 1, {
            Ai::getExtractAs<typed_lgr<TcpNetworkServer>>(args[0], define_TcpClientSocket)->start();
        });
        AttachAFun(funs_TcpNetworkServer_stop, 1, {
            Ai::getExtractAs<typed_lgr<TcpNetworkServer>>(args[0], define_TcpClientSocket)->stop();
        });
        AttachAFun(funs_TcpNetworkServer_is_running, 1, {
            return Ai::getExtractAs<typed_lgr<TcpNetworkServer>>(args[0], define_TcpClientSocket)->is_running();
        });
        AttachAFun(funs_TcpNetworkServer_is_paused, 1, {
            return Ai::getExtractAs<typed_lgr<TcpNetworkServer>>(args[0], define_TcpClientSocket)->is_paused();
        });
        AttachAFun(funs_TcpNetworkServer_is_corrupted, 1, {
            return Ai::getExtractAs<typed_lgr<TcpNetworkServer>>(args[0], define_TcpClientSocket)->is_corrupted();
        });
        AttachAFun(funs_TcpNetworkServer_server_port, 1, {
            return Ai::getExtractAs<typed_lgr<TcpNetworkServer>>(args[0], define_TcpClientSocket)->server_port();
        });
        AttachAFun(funs_TcpNetworkServer_server_ip, 1, {
            return Ai::getExtractAs<typed_lgr<TcpNetworkServer>>(args[0], define_TcpClientSocket)->server_ip();
        });
        AttachAFun(funs_TcpNetworkServer_server_address, 1, {
            return Ai::getExtractAs<typed_lgr<TcpNetworkServer>>(args[0], define_TcpClientSocket)->server_address();
        });
        AttachAFun(funs_TcpNetworkServer_accept, 1, {
            return Ai::getExtractAs<typed_lgr<TcpNetworkServer>>(args[0], define_TcpClientSocket)->accept(len == 2 ? (bool)args[1] : false);
        });
        AttachAFun(funs_TcpNetworkServer_await, 1, {
            Ai::getExtractAs<typed_lgr<TcpNetworkServer>>(args[0], define_TcpClientSocket)->_await();
        });
        AttachAFun(funs_TcpNetworkServer_pause, 1, {
            Ai::getExtractAs<typed_lgr<TcpNetworkServer>>(args[0], define_TcpClientSocket)->pause();
        });
        AttachAFun(funs_TcpNetworkServer_resume, 1, {
            Ai::getExtractAs<typed_lgr<TcpNetworkServer>>(args[0], define_TcpClientSocket)->resume();
        });
        AttachAFun(funs_TcpNetworkServer_set_configuration, 2, {
            Ai::getExtractAs<typed_lgr<TcpNetworkServer>>(args[0], define_TcpClientSocket)->set_configuration(Ai::getExtractAsStatic<TcpConfiguration>(args[1]));
        });
        AttachAFun(funs_TcpNetworkServer_set_accept_filter, 2, {
            CXX::excepted(args[1], VType::function);
            Ai::getExtractAs<typed_lgr<TcpNetworkServer>>(args[0], define_TcpClientSocket)->set_accept_filter(*args[1].funPtr());
        });
#pragma endregion
#pragma region TcpClientSocket
        AttachAFun(funs_TcpClientSocket_set_configuration, 2, {
            Ai::getExtractAs<typed_lgr<TcpClientSocket>>(args[0], define_TcpClientSocket)->set_configuration(Ai::getExtractAsStatic<TcpConfiguration>(args[1]));
        });
        AttachAFun(funs_TcpClientSocket_recv, 2, {
            if (len == 2) {
                uint32_t buf_len = (int32_t)args[1];
                uint8_t* buf = new uint8_t[buf_len];
                uint32_t readed = Ai::getExtractAs<typed_lgr<TcpClientSocket>>(args[0], define_TcpClientSocket)->recv(buf, buf_len);
                return ValueItem(buf, readed, no_copy);
            } else {
                CXX::excepted_basic_array(args[1]);
                if (!args[1].meta.as_ref && !args[1].meta.use_gc)
                    throw InvalidArguments("Array must be a reference or gc object to be used as buffer");
                int32_t to_recv = (int32_t)args[2];
                if (to_recv > args[1].meta.val_len)
                    throw InvalidArguments("Array length is less than requested bytes to recv");
                return Ai::getExtractAs<typed_lgr<TcpClientSocket>>(args[0], define_TcpClientSocket)->recv((uint8_t*)args[1].getSourcePtr(), to_recv);
            }
        });
        AttachAFun(funs_TcpClientSocket_send, 2, {
            if (len == 2) {
                if (args[1].meta.vtype == VType::raw_arr_i8 || args[1].meta.vtype == VType::raw_arr_ui8)
                    return Ai::getExtractAs<typed_lgr<TcpClientSocket>>(args[0], define_TcpClientSocket)->send((uint8_t*)args[1].getSourcePtr(), (int32_t)args[1].meta.val_len);
                else
                    CXX::excepted(args[1], VType::raw_arr_ui8);
            } else {
                CXX::excepted_basic_array(args[1]);
                int32_t to_send = (int32_t)args[2];
                if (to_send > args[1].meta.val_len)
                    throw InvalidArguments("Array length is less than requested bytes to send");
                return Ai::getExtractAs<typed_lgr<TcpClientSocket>>(args[0], define_TcpClientSocket)->send((uint8_t*)args[1].getSourcePtr(), to_send);
            }
        });
        AttachAFun(funs_TcpClientSocket_send_file, 5, {
            auto& class_ = Ai::getExtractAs<typed_lgr<TcpClientSocket>>(args[0], define_TcpClientSocket);
            uint64_t data_len = 0;
            uint64_t offset = 0;
            uint32_t chunks_size = 0;
            if (args[1].meta.vtype != VType::struct_ && args[1].meta.vtype != VType::string)
                throw InvalidArguments("The second argument must be a file handle or a file path.");
            if (len >= 3)
                data_len = (uint64_t)args[2];
            if (len >= 4)
                offset = (uint64_t)args[3];
            if (len >= 5)
                chunks_size = (uint32_t)args[4];

            if (args[1].meta.vtype == VType::string) {
                art::ustring& str = *(art::ustring*)args[1].getSourcePtr();
                return class_->send_file(str.data(), str.size(), data_len, offset, chunks_size);
            } else if (args[1].meta.vtype == VType::struct_) {
                auto& proxy = (Structure&)args[1];
                if (proxy.get_vtable()) {
                    if (proxy.get_vtable() == Ai::typeVTable<typed_lgr<art::files::FileHandle>>())
                        return class_->send_file(*Ai::getAs<typed_lgr<art::files::FileHandle>>(proxy), data_len, offset, chunks_size);
                    else if (proxy.get_vtable() == Ai::typeVTable<typed_lgr<art::files::BlockingFileHandle>>())
                        return class_->send_file(*Ai::getAs<typed_lgr<art::files::BlockingFileHandle>>(proxy), data_len, offset, chunks_size);
                }
            } else
                throw InvalidArguments("The second argument must be a file handle or a file path.");
        });
        AttachAFun(funs_TcpClientSocket_close, 1, {
            Ai::getExtractAs<typed_lgr<TcpClientSocket>>(args[0], define_TcpClientSocket)->close();
        });
        AttachAFun(funs_TcpClientSocket_rebuffer, 2, {
            Ai::getExtractAs<typed_lgr<TcpClientSocket>>(args[0], define_TcpClientSocket)->rebuffer((int32_t)args[1]);
        });
#pragma endregion
#pragma region UdpSocket
        AttachAFun(funs_udp_socket_recv, 2, {
            ValueItem sender;
            ValueItem ret;
            if (len >= 2) {
                uint32_t buf_len = (int32_t)args[1];
                uint8_t* buf = new uint8_t[buf_len];
                uint32_t readed = Ai::getExtractAs<typed_lgr<udp_socket>>(args[0], define_UdpSocket)->recv(buf, buf_len, sender);
                ret = ValueItem(buf, readed, no_copy);
            } else {
                CXX::excepted_basic_array(args[1]);
                if (!args[1].meta.as_ref && !args[2].meta.use_gc)
                    throw InvalidArguments("Array must be a reference or gc object to be used as buffer");
                uint32_t to_recv = (int32_t)args[2];
                if (to_recv > args[1].meta.val_len)
                    throw InvalidArguments("Array length is less than requested bytes to recv");
                ret = Ai::getExtractAs<typed_lgr<udp_socket>>(args[0], define_UdpSocket)->recv((uint8_t*)args[1].getSourcePtr(), to_recv, sender);
            }
            return ValueItem({std::move(ret), std::move(sender)});
        });
        AttachAFun(funs_udp_socket_send, 4, {
            if (args[1].meta.vtype == VType::raw_arr_i8 || args[1].meta.vtype == VType::raw_arr_ui8) {
                if (args[1].meta.val_len < (int32_t)args[2])
                    throw InvalidArguments("Array length is less than requested bytes to send");
                return Ai::getExtractAs<typed_lgr<udp_socket>>(args[0], define_UdpSocket)->send((uint8_t*)args[1].getSourcePtr(), (int32_t)args[2], args[3]);
            } else
                CXX::excepted(args[1], VType::raw_arr_ui8);
        });
#pragma endregion

        namespace constructor {
            ValueItem* createProxy_IP4(ValueItem* args, uint32_t len) {
                if (len < 1)
                    return nullptr;
                if (args[0].meta.vtype != VType::string)
                    throw InvalidArguments("Excepted string as first argument");
                if (len == 1)
                    return new ValueItem(makeIP4(((art::ustring*)args[0].getSourcePtr())->data()));
                else
                    return new ValueItem(makeIP4(((art::ustring*)args[0].getSourcePtr())->data(), (uint16_t)args[1]));
            }

            ValueItem* createProxy_IP6(ValueItem* args, uint32_t len) {
                if (len < 1)
                    return nullptr;
                if (args[0].meta.vtype != VType::string)
                    throw InvalidArguments("Excepted string as first argument");
                if (len == 1)
                    return new ValueItem(makeIP6(((art::ustring*)args[0].getSourcePtr())->data()));
                else
                    return new ValueItem(makeIP6(((art::ustring*)args[0].getSourcePtr())->data(), (uint16_t)args[1]));
            }

            ValueItem* createProxy_IP(ValueItem* args, uint32_t len) {
                if (len < 1)
                    return nullptr;
                if (args[0].meta.vtype != VType::string)
                    throw InvalidArguments("Excepted string as first argument");
                if (len == 1)
                    return new ValueItem(makeIP(((art::ustring*)args[0].getSourcePtr())->data()));
                else
                    return new ValueItem(makeIP(((art::ustring*)args[0].getSourcePtr())->data(), (uint16_t)args[1]));
            }

            ValueItem* createProxy_Address(ValueItem* args, uint32_t len) {
                if (len < 1)
                    return nullptr;
                if (args[0].meta.vtype != VType::string)
                    throw InvalidArguments("Excepted string as first argument");
                return new ValueItem(makeIP_port(((art::ustring*)args[0].getSourcePtr())->data()));
            }

            ValueItem* createProxy_TcpServer(ValueItem* args, uint32_t len) {
                if (len < 2)
                    throw InvalidArguments("Required arguments: [function], [ip:port], [manage_type = write_delayed(1)], [acceptors = 10], [timeout_ms = 0]");
                auto fun = args[0].funPtr();
                auto ip_port = args[1];
                TcpNetworkServer::ManageType manage_type = TcpNetworkServer::ManageType::write_delayed;
                size_t acceptors = 10;
                TcpConfiguration config = {};
                if (len >= 3)
                    if (args[2].meta.vtype != VType::noting)
                        manage_type = (TcpNetworkServer::ManageType)(uint8_t)args[2];
                if (len >= 4)
                    if (args[3].meta.vtype != VType::noting)
                        acceptors = (size_t)args[3];
                if (len >= 5)
                    if (args[4].meta.vtype != VType::noting)
                        config = CXX::Interface::getExtractAsStatic<TcpConfiguration>(args[4]);
                return new ValueItem(Ai::constructStructure<typed_lgr<TcpNetworkServer>>(define_TcpNetworkServer, new TcpNetworkServer(*fun, ip_port, manage_type, acceptors, config)));
            }

            ValueItem* createProxy_HttpServer(ValueItem*, uint32_t) {
                throw NotImplementedException();
            }

            ValueItem* createProxy_UdpSocket(ValueItem* args, uint32_t len) {
                if (len < 1)
                    throw InvalidArguments("Required arguments: [ip:port]");
                auto& ip_port = args[0];
                int32_t timeout_ms = 0;
                if (len >= 2)
                    timeout_ms = (int32_t)args[1];
                return new ValueItem(Ai::constructStructure<typed_lgr<udp_socket>>(define_UdpSocket, new udp_socket(ip_port, timeout_ms)));
            }
        }

        ValueItem* ipv6_supported(ValueItem*, uint32_t) {
            return new ValueItem(art::ipv6_supported());
        }

        ValueItem* tcp_client_connect(ValueItem* args, uint32_t len) {
            //[ip:port], [timeout_ms = 0] or [ip:port], [data], [timeout_ms = 0]
            if (len < 1)
                throw InvalidArguments("Required arguments: [ip:port], [timeout_ms = 0] or [ip:port], [data], [timeout_ms = 0]");
            auto& ip_port = args[0];
            if (len == 1) {
                return new ValueItem(Ai::constructStructure<typed_lgr<TcpClientSocket>>(define_TcpClientSocket, TcpClientSocket::connect(ip_port)), no_copy);
            } else if (len == 2) {
                auto& data = args[1];
                if (args[1].meta.vtype == VType::raw_arr_ui8 || args[1].meta.vtype == VType::raw_arr_i8) {
                    return new ValueItem(Ai::constructStructure<typed_lgr<TcpClientSocket>>(define_TcpClientSocket, TcpClientSocket::connect(ip_port, (char*)data.getSourcePtr(), data.meta.val_len)), no_copy);
                } else
                    return new ValueItem(Ai::constructStructure<typed_lgr<TcpClientSocket>>(define_TcpClientSocket, TcpClientSocket::connect(ip_port, CXX::Interface::getExtractAsStatic<TcpConfiguration>(data))));
            } else {
                auto& data = args[1];
                return new ValueItem(Ai::constructStructure<typed_lgr<TcpClientSocket>>(define_TcpClientSocket, TcpClientSocket::connect(ip_port, (char*)data.getSourcePtr(), data.meta.val_len, CXX::Interface::getExtractAsStatic<TcpConfiguration>(args[2]))), no_copy);
            }
        }

        void init() {
            define_TcpNetworkServer = Ai::createTable<typed_lgr<TcpNetworkServer>>(
                "tcp_server",
                Ai::direct_method("start", funs_TcpNetworkServer_start),
                Ai::direct_method("stop", funs_TcpNetworkServer_stop),
                Ai::direct_method("is_running", funs_TcpNetworkServer_is_running),
                Ai::direct_method("is_corrupted", funs_TcpNetworkServer_is_corrupted),
                Ai::direct_method("is_paused", funs_TcpNetworkServer_is_paused),
                Ai::direct_method("server_port", funs_TcpNetworkServer_server_port),
                Ai::direct_method("server_ip", funs_TcpNetworkServer_server_ip),
                Ai::direct_method("server_address", funs_TcpNetworkServer_server_address),
                Ai::direct_method("accept", funs_TcpNetworkServer_accept),
                Ai::direct_method("await", funs_TcpNetworkServer_await),
                Ai::direct_method("pause", funs_TcpNetworkServer_pause),
                Ai::direct_method("resume", funs_TcpNetworkServer_resume),
                Ai::direct_method("set_configuration", funs_TcpNetworkServer_set_configuration),
                Ai::direct_method("set_accept_filter", funs_TcpNetworkServer_set_accept_filter)
            );
            define_TcpClientSocket = Ai::createTable<typed_lgr<TcpClientSocket>>(
                "tcp_client",
                Ai::direct_method("set_configuration", funs_TcpClientSocket_set_configuration),
                Ai::direct_method("recv", funs_TcpClientSocket_recv),
                Ai::direct_method("send", funs_TcpClientSocket_send),
                Ai::direct_method("send_file", funs_TcpClientSocket_send_file),
                Ai::direct_method("close", funs_TcpClientSocket_close)
            );

            define_UdpSocket = Ai::createTable<typed_lgr<udp_socket>>(
                "udp_socket",
                Ai::direct_method("recv", funs_udp_socket_recv),
                Ai::direct_method("send", funs_udp_socket_send)
            );

            CXX::Interface::typeVTable<typed_lgr<TcpNetworkServer>>() = define_TcpNetworkServer;
            CXX::Interface::typeVTable<typed_lgr<TcpClientSocket>>() = define_TcpClientSocket;
            CXX::Interface::typeVTable<typed_lgr<files::BlockingFileHandle>>() = define_UdpSocket;
        }
    }
}