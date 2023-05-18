// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../AttachA_CXX.hpp"
#include "../networking.hpp"
namespace AIs = AttachA::Interface::special;
namespace net{
    ProxyClassDefine define_TcpNetworkServer;
    ProxyClassDefine define_TcpClientSocket;
    ProxyClassDefine define_UdpSocket;
#pragma region TcpNetworkServer
    ValueItem* funs_TcpNetworkServer_start(ValueItem* args, uint32_t len){
        if(len >= 1){
            AIs::proxy_get_as_native<TcpNetworkServer>(args[0])->start();
            return nullptr;
        }else
            throw InvalidArguments("This function is proxy function");
    }
    ValueItem* funs_TcpNetworkServer_stop(ValueItem* args, uint32_t len){
        if(len >= 1){
            AIs::proxy_get_as_native<TcpNetworkServer>(args[0])->stop();
            return nullptr;
        }else
            throw InvalidArguments("This function is proxy function");
    }
    ValueItem* funs_TcpNetworkServer_is_running(ValueItem* args, uint32_t len){
        if(len >= 1){
            return new ValueItem(AIs::proxy_get_as_native<TcpNetworkServer>(args[0])->is_running());
        }else
            throw InvalidArguments("This function is proxy function");
    }
    ValueItem* funs_TcpNetworkServer_is_paused(ValueItem* args, uint32_t len){
        if(len >= 1){
            return new ValueItem(AIs::proxy_get_as_native<TcpNetworkServer>(args[0])->is_paused());
        }else
            throw InvalidArguments("This function is proxy function");
    }
    ValueItem* funs_TcpNetworkServer_is_corrupted(ValueItem* args, uint32_t len){
        if(len >= 1){
            return new ValueItem(AIs::proxy_get_as_native<TcpNetworkServer>(args[0])->is_corrupted());
        }else
            throw InvalidArguments("This function is proxy function");
    }
    ValueItem* funs_TcpNetworkServer_server_port(ValueItem* args, uint32_t len){
        if(len >= 1){
            return new ValueItem(AIs::proxy_get_as_native<TcpNetworkServer>(args[0])->server_port());
        }else
            throw InvalidArguments("This function is proxy function");
    }
    ValueItem* funs_TcpNetworkServer_server_ip(ValueItem* args, uint32_t len){
        if(len >= 1){
            return new ValueItem(AIs::proxy_get_as_native<TcpNetworkServer>(args[0])->server_ip());
        }else
            throw InvalidArguments("This function is proxy function");
    }
    ValueItem* funs_TcpNetworkServer_server_address(ValueItem* args, uint32_t len){
        if(len >= 1){
            return new ValueItem(AIs::proxy_get_as_native<TcpNetworkServer>(args[0])->server_address());
        }else
            throw InvalidArguments("This function is proxy function");
    }
    ValueItem* funs_TcpNetworkServer_await(ValueItem* args, uint32_t len){
        if(len >= 1){
            AIs::proxy_get_as_native<TcpNetworkServer>(args[0])->_await();
            return nullptr;
        }else
            throw InvalidArguments("This function is proxy function");
    }
    ValueItem* funs_TcpNetworkServer_pause(ValueItem* args, uint32_t len){
        if(len >= 1){
            AIs::proxy_get_as_native<TcpNetworkServer>(args[0])->pause();
            return nullptr;
        }else
            throw InvalidArguments("This function is proxy function");
    }
    ValueItem* funs_TcpNetworkServer_resume(ValueItem* args, uint32_t len){
        if(len >= 1){
            AIs::proxy_get_as_native<TcpNetworkServer>(args[0])->resume();
            return nullptr;
        }else
            throw InvalidArguments("This function is proxy function");
    }
    ValueItem* funs_TcpNetworkServer_set_default_buffer(ValueItem* args, uint32_t len){
        if(len >= 2){
            AIs::proxy_get_as_native<TcpNetworkServer>(args[0])->set_default_buffer_size((int32_t)args[1]);
            return nullptr;
        }else
            throw InvalidArguments("This function is proxy function");
    }
    ValueItem* funs_TcpNetWorkServer_set_accept_filter(ValueItem* args, uint32_t len){
        if(len >= 2){
            auto fun = args[1].funPtr();
            if(fun == nullptr) throw InvalidArguments("Expected function pointer");
            AIs::proxy_get_as_native<TcpNetworkServer>(args[0])->set_accept_filter(*fun);
            return nullptr;
        }else
            throw InvalidArguments("This function is proxy function");
    }
#pragma endregion
#pragma region TcpClientSocket
    ValueItem* funs_TcpClientSocket_recv(ValueItem* args, uint32_t len){
        if(len >= 2){
            return new ValueItem(AIs::proxy_get_as_native<TcpClientSocket>(args[0])->recv((uint8_t*)args[1].getSourcePtr(), (int32_t)args[2]));
        }else
            throw InvalidArguments("This function is proxy function");
    }
    ValueItem* funs_TcpClientSocket_send(ValueItem* args, uint32_t len){
        if(len >= 2){
            return new ValueItem(AIs::proxy_get_as_native<TcpClientSocket>(args[0])->send((uint8_t*)args[1].getSourcePtr(), (int32_t)args[2]));
        }else
            throw InvalidArguments("This function is proxy function");
    }
    ValueItem* funs_TcpClientSocket_send_file(ValueItem* args, uint32_t len){
        if(len >= 5){
            uint64_t data_len = 0;
            uint64_t offset = 0;
            uint32_t chunks_size = 0;
            if(args[1].meta.vtype != VType::proxy && args[1].meta.vtype != VType::string)
                throw InvalidArguments("The second argument must be a file handle or a file path.");
            if(len >= 3)
                data_len = (uint64_t)args[2];
            if(len >= 4)
                offset = (uint64_t)args[3];
            if(len >= 5)
                chunks_size = (uint32_t)args[4];

            if(args[1].meta.vtype == VType::string){
                std::string& str = *(std::string*)args[1].getSourcePtr();
                return new ValueItem(AIs::proxy_get_as_native<TcpClientSocket>(args[0])->send_file(str.data(), str.size(), data_len, offset, chunks_size));
            }else if(args[1].meta.vtype == VType::proxy){
                auto& proxy = *((ProxyClass*)args[1].val);
                if(proxy.declare_ty){
                    if(proxy.declare_ty->name == "file_handle")
                        return new ValueItem(((TcpClientSocket*)((ProxyClass*)args[0].val)->class_ptr)->send_file(**(typed_lgr<::files::FileHandle>*)proxy.class_ptr, data_len, offset, chunks_size));
                    else if(proxy.declare_ty->name == "blocking_file_handle")
                        return new ValueItem(((TcpClientSocket*)((ProxyClass*)args[0].val)->class_ptr)->send_file(**(typed_lgr<::files::BlockingFileHandle>*)proxy.class_ptr, data_len, offset, chunks_size));
                }
            }
            throw InvalidArguments("Excepted string, proxy<file_handle> or proxy<blocking_file_handle> as second argument");
        }else throw InvalidArguments("This function is proxy function");
    }
    ValueItem* funs_TcpClientSocket_close(ValueItem* args, uint32_t len){
        if(len >= 1){
            AIs::proxy_get_as_native<TcpClientSocket>(args[0])->close();
            return nullptr;
        }else
            throw InvalidArguments("This function is proxy function");
    }
    ValueItem* funs_TcpClientSocket_rebuffer(ValueItem* args, uint32_t len){
        if(len >= 2){
            AIs::proxy_get_as_native<TcpClientSocket>(args[0])->rebuffer((int32_t)args[1]);
            return nullptr;
        }else
            throw InvalidArguments("This function is proxy function");
    }
#pragma endregion
#pragma region UdpSocket
    ValueItem* funs_udp_socket_recv(ValueItem* args, uint32_t len){
        if(len >= 3){
            return new ValueItem(AIs::proxy_get_as_native<udp_socket>(args[0])->recv((uint8_t*)args[1].getSourcePtr(), (int32_t)args[2], args[3]));
        }else
            throw InvalidArguments("This function is proxy function");
    }
    ValueItem* funs_udp_socket_send(ValueItem* args, uint32_t len){
        if(len >= 3){
            return new ValueItem(AIs::proxy_get_as_native<udp_socket>(args[0])->send((uint8_t*)args[1].getSourcePtr(), (int32_t)args[2], args[3]));
        }else
            throw InvalidArguments("This function is proxy function");
    }
#pragma endregion


    namespace constructor{
        ValueItem* createProxy_IP4(ValueItem* args, uint32_t len){
            if (len < 1) return nullptr;
            if (args[0].meta.vtype != VType::string) throw InvalidArguments("Excepted string as first argument");
            if (len == 1) return new ValueItem(makeIP4(((std::string*)args[0].getSourcePtr())->data()));
            else return new ValueItem(makeIP4(((std::string*)args[0].getSourcePtr())->data(), (uint16_t)args[1]));
        }
        ValueItem* createProxy_IP6(ValueItem* args, uint32_t len){
            if (len < 1) return nullptr;
            if (args[0].meta.vtype != VType::string) throw InvalidArguments("Excepted string as first argument");
            if (len == 1) return new ValueItem(makeIP6(((std::string*)args[0].getSourcePtr())->data()));
            else return new ValueItem(makeIP6(((std::string*)args[0].getSourcePtr())->data(), (uint16_t)args[1]));
        }
        ValueItem* createProxy_IP(ValueItem* args, uint32_t len){
            if (len < 1) return nullptr;
            if (args[0].meta.vtype != VType::string) throw InvalidArguments("Excepted string as first argument");
            if (len == 1) return new ValueItem(makeIP(((std::string*)args[0].getSourcePtr())->data()));
            else return new ValueItem(makeIP(((std::string*)args[0].getSourcePtr())->data(), (uint16_t)args[1]));
        }
        ValueItem* createProxy_Address(ValueItem* args, uint32_t len){
            if (len < 1) return nullptr;
            if (args[0].meta.vtype != VType::string) throw InvalidArguments("Excepted string as first argument");
            return new ValueItem(makeIP_port(((std::string*)args[0].getSourcePtr())->data()));
        }

        ValueItem* createProxy_TcpServer(ValueItem* args, uint32_t len) {
            if(len < 2)
                throw InvalidArguments("Required arguments: [function], [ip:port], [manage_type = write_delayed(1)], [acceptors = 10], [timeout_ms = 0]");
            auto fun = args[0].funPtr();
            auto ip_port = args[1];
            TcpNetworkServer::ManageType manage_type = TcpNetworkServer::ManageType::write_delayed;
            size_t acceptors = 10;
            int32_t timeout_ms = 0;
            int32_t default_buffer = 8192;
            if(len >= 3)
                if(args[2].meta.vtype != VType::noting) manage_type = (TcpNetworkServer::ManageType)(uint8_t)args[2];
            if(len >= 4)
                if(args[3].meta.vtype != VType::noting) acceptors = (size_t)args[3];
            if(len >= 5)
                if(args[4].meta.vtype != VType::noting) timeout_ms = (int32_t)args[4];
            if(len >= 6)
                if(args[5].meta.vtype != VType::noting) default_buffer = (int32_t)args[5];
            if(default_buffer < 1) default_buffer = 8192;
            return new ValueItem(new ProxyClass(new typed_lgr(new TcpNetworkServer(*fun, ip_port,manage_type, acceptors, timeout_ms, default_buffer)), &define_TcpNetworkServer));
        }
		ValueItem* createProxy_HttpServer(ValueItem*, uint32_t){
            throw NotImplementedException();
        }
		ValueItem* createProxy_UdpSocket(ValueItem* args, uint32_t len){
            if(len < 1)
                throw InvalidArguments("Required arguments: [ip:port]");
            auto& ip_port = args[0];
            int32_t timeout_ms = 0;
            if(len >= 2)
                timeout_ms = (int32_t)args[1];
            return new ValueItem(new ProxyClass(new typed_lgr(new udp_socket(ip_port, timeout_ms)), &define_UdpSocket));
        }
    }
    ValueItem* ipv6_supported(ValueItem* , uint32_t ){
        return new ValueItem(::ipv6_supported());
    }
    
	ValueItem* tcp_client_connect(ValueItem* args, uint32_t len){
        //[ip:port], [timeout_ms = 0] or [ip:port], [data], [timeout_ms = 0]
        if(len < 1)
            throw InvalidArguments("Required arguments: [ip:port], [timeout_ms = 0] or [ip:port], [data], [timeout_ms = 0]");
        auto& ip_port = args[0];
        if(len == 1)
            return new ValueItem(new ProxyClass(new typed_lgr(TcpClientSocket::connect(ip_port)), &define_TcpClientSocket));
        else if(len == 2){
            auto& data = args[1];
            if(args[1].meta.vtype == VType::raw_arr_ui8 || args[1].meta.vtype == VType::raw_arr_i8)
                return new ValueItem(new ProxyClass(new typed_lgr(TcpClientSocket::connect(ip_port,(char*)data.getSourcePtr(),data.meta.val_len)), &define_TcpClientSocket));
            else
                return new ValueItem(new ProxyClass(new typed_lgr(TcpClientSocket::connect(ip_port,(int32_t)data)), &define_TcpClientSocket));
        }
        else{
            auto& data = args[1];
            return new ValueItem(new ProxyClass(new typed_lgr(TcpClientSocket::connect(ip_port,(char*)data.getSourcePtr(),data.meta.val_len, (int32_t)args[2])), &define_TcpClientSocket));
        }
    }
    void init(){
        define_TcpNetworkServer.name = "tcp_server";
        define_TcpNetworkServer.destructor = AIs::proxyDestruct<TcpNetworkServer, true>;
        define_TcpNetworkServer.copy = AIs::proxyCopy<TcpNetworkServer, true>;
        define_TcpNetworkServer.funs["start"] = ClassFnDefine(new FuncEnviropment(funs_TcpNetworkServer_start, false), false, ClassAccess::pub);
        define_TcpNetworkServer.funs["stop"] = ClassFnDefine(new FuncEnviropment(funs_TcpNetworkServer_stop, false), false, ClassAccess::pub);
        define_TcpNetworkServer.funs["is_running"] = ClassFnDefine(new FuncEnviropment(funs_TcpNetworkServer_is_running, false), false, ClassAccess::pub);
        define_TcpNetworkServer.funs["is_corrupted"] = ClassFnDefine(new FuncEnviropment(funs_TcpNetworkServer_is_corrupted, false), false, ClassAccess::pub);
        define_TcpNetworkServer.funs["is_paused"] = ClassFnDefine(new FuncEnviropment(funs_TcpNetworkServer_is_paused, false), false, ClassAccess::pub);
        define_TcpNetworkServer.funs["server_port"] = ClassFnDefine(new FuncEnviropment(funs_TcpNetworkServer_server_port, false), false, ClassAccess::pub);
        define_TcpNetworkServer.funs["server_ip"] = ClassFnDefine(new FuncEnviropment(funs_TcpNetworkServer_server_ip, false), false, ClassAccess::pub);
        define_TcpNetworkServer.funs["server_address"] = ClassFnDefine(new FuncEnviropment(funs_TcpNetworkServer_server_address, false), false, ClassAccess::pub);
        define_TcpNetworkServer.funs["await"] = ClassFnDefine(new FuncEnviropment(funs_TcpNetworkServer_await, false), false, ClassAccess::pub);
        define_TcpNetworkServer.funs["pause"] = ClassFnDefine(new FuncEnviropment(funs_TcpNetworkServer_pause, false), false, ClassAccess::pub);
        define_TcpNetworkServer.funs["resume"] = ClassFnDefine(new FuncEnviropment(funs_TcpNetworkServer_resume, false), false, ClassAccess::pub);
        define_TcpNetworkServer.funs["set_default_buffer"] = ClassFnDefine(new FuncEnviropment(funs_TcpNetworkServer_set_default_buffer, false), false, ClassAccess::pub);
        define_TcpNetworkServer.funs["set_accept_filter"] = ClassFnDefine(new FuncEnviropment(funs_TcpNetWorkServer_set_accept_filter, false), false, ClassAccess::pub);

        define_TcpClientSocket.name = "tcp_client";
        define_TcpClientSocket.destructor = AIs::proxyDestruct<TcpClientSocket, true>;
        define_TcpClientSocket.copy = AIs::proxyCopy<TcpClientSocket, true>;
        define_TcpClientSocket.funs["recv"] = ClassFnDefine(new FuncEnviropment(funs_TcpClientSocket_recv, false), false, ClassAccess::pub);
        define_TcpClientSocket.funs["send"] = ClassFnDefine(new FuncEnviropment(funs_TcpClientSocket_send, false), false, ClassAccess::pub);
        define_TcpClientSocket.funs["send_file"] = ClassFnDefine(new FuncEnviropment(funs_TcpClientSocket_send_file, false), false, ClassAccess::pub);
        define_TcpClientSocket.funs["close"] = ClassFnDefine(new FuncEnviropment(funs_TcpClientSocket_close, false), false, ClassAccess::pub);
        define_TcpClientSocket.funs["rebuffer"] = ClassFnDefine(new FuncEnviropment(funs_TcpClientSocket_rebuffer, false), false, ClassAccess::pub);    

        define_UdpSocket.name = "udp_socket";
        define_UdpSocket.destructor = AIs::proxyDestruct<udp_socket, true>;
        define_UdpSocket.copy = AIs::proxyCopy<udp_socket, true>;
        define_UdpSocket.funs["recv"] = ClassFnDefine(new FuncEnviropment(funs_udp_socket_recv, false), false, ClassAccess::pub);
        define_UdpSocket.funs["send"] = ClassFnDefine(new FuncEnviropment(funs_udp_socket_send, false), false, ClassAccess::pub);
    }
}