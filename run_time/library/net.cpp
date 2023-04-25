#include "../AttachA_CXX.hpp"
#include "../networking.hpp"
namespace AIs = AttachA::Interface::special;
namespace net{
    ProxyClassDefine define_TcpNetworkServer;
#pragma region TcpNetworkServer
    ValueItem* funs_TcpNetworkServer_start(ValueItem* args, uint32_t len){
        if(len >= 1){
            AIs::proxy_get_as_native<TcpNetworkServer>(args[0])->start(len == 2 ? (size_t)args[1] : 0);
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
    ValueItem* funs_TcpNetworkServer_mainline(ValueItem* args, uint32_t len){
        if(len >= 1){
            AIs::proxy_get_as_native<TcpNetworkServer>(args[0])->mainline();
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
    ValueItem* funs_TcpNetworkServer_set_pool_size(ValueItem* args, uint32_t len){
        if(len == 1){
            AIs::proxy_get_as_native<TcpNetworkServer>(args[0])->set_pool_size(0);
            return nullptr;
        }else if(len >= 2){
            AIs::proxy_get_as_native<TcpNetworkServer>(args[0])->set_pool_size((size_t)args[1]);
            return nullptr;
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
                throw InvalidArguments("Required arguments: [function], [ip:port], [manage_type = write_delayed(1)], [acceptors = 10], [accept_mode = task(0)]");
            auto fun = args[0].funPtr();
            auto ip_port = args[1];
            TcpNetworkServer::ManageType manage_type = TcpNetworkServer::ManageType::write_delayed;
            size_t acceptors = 10;
            if(len >= 3)
                if(args[2].meta.vtype != VType::noting) manage_type = (TcpNetworkServer::ManageType)(uint8_t)args[2];
            if(len >= 4)
                if(args[3].meta.vtype != VType::noting) acceptors = (size_t)args[3];

            return new ValueItem(new ProxyClass(new TcpNetworkServer(*fun, ip_port,manage_type, acceptors), &define_TcpNetworkServer));
        }
		ValueItem* createProxy_UdpServer(ValueItem*, uint32_t){
            throw NotImplementedException();
        }
		ValueItem* createProxy_HttpServer(ValueItem*, uint32_t){
            throw NotImplementedException();
        }
		ValueItem* createProxy_TcpClient(ValueItem*, uint32_t){
            throw NotImplementedException();
        }
		ValueItem* createProxy_UdpClient(ValueItem*, uint32_t){
            throw NotImplementedException();
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
        define_TcpNetworkServer.funs["mainline"] = ClassFnDefine(new FuncEnviropment(funs_TcpNetworkServer_mainline, false), false, ClassAccess::pub);
        define_TcpNetworkServer.funs["pause"] = ClassFnDefine(new FuncEnviropment(funs_TcpNetworkServer_pause, false), false, ClassAccess::pub);
        define_TcpNetworkServer.funs["resume"] = ClassFnDefine(new FuncEnviropment(funs_TcpNetworkServer_resume, false), false, ClassAccess::pub);
        define_TcpNetworkServer.funs["set_pool_size"] = ClassFnDefine(new FuncEnviropment(funs_TcpNetworkServer_set_pool_size, false), false, ClassAccess::pub);
    }
}