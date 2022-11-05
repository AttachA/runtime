#include <list>
#include <queue>
#include "../tasks.hpp"
#include "../AttachA_CXX.hpp"
#include "chanel.hpp"
namespace chanel {
	template<class Class_>
	inline typed_lgr<Class_> getClass(ValueItem* vals) {
		vals->getAsync();
		if(vals->meta.vtype == VType::proxy)
			return (*(typed_lgr<Class_>*)(((ProxyClass*)vals->getSourcePtr()))->class_ptr);
		else
			throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem _lazy_load_singleton_chanelDeath() {
		static ValueItem val;
		if (val.meta.vtype == VType::noting) {
			static TaskMutex m;
			std::lock_guard guard(m);
			if (val.meta.vtype == VType::noting) {
				try {
					throw AException("TaskDeath", "Chanel died before all handlers unsubed");
				}
				catch (...) {
					val = std::current_exception();
				}
			}
		}
		return val;
	}
	ValueItem _lazy_load_singleton_handleDeath() {
		static ValueItem val;
		if (val.meta.vtype == VType::noting){
			static TaskMutex m;
			std::lock_guard guard(m);
			if (val.meta.vtype == VType::noting) {
				try {
					throw AException("TaskDeath", "Detached from chanel");
				}
				catch (...) {
					val = std::current_exception();
				}
			}
		}
		return val;
	}



	void ChanelHandler::put(const ValueItem& val) {
		std::lock_guard guard(res_lock);
		res_cache.push(val);
		res_await.notify_all();
	}
	ChanelHandler::ChanelHandler(){}
	ValueItem ChanelHandler::take() {
		if(res_cache.empty())
			if (!wait_item())
				throw AException("TaskDeath", "Detached from chanel");
		if (!allow_sub)
			if (res_cache.size() == 1)
				return res_cache.front();
		auto res = std::move(res_cache.front());
		res_cache.pop();
		return res;
	}
	bool ChanelHandler::can_take() {
		std::lock_guard guard(res_lock);
		if (res_cache.size() > 1)
			return true;
		return allow_sub;
	}
	bool ChanelHandler::end_of_chanel() {
		return allow_sub;
	}
	bool ChanelHandler::wait_item() {
		while (res_cache.empty()) {
			{
				std::lock_guard guard(res_lock);
				if (!allow_sub)
					return false;
			}
			MutexUnify mu(res_lock);
			std::unique_lock ul(mu);
			res_await.wait_for(ul, 500);
		}
		return true;
	}
	

	Chanel::Chanel() {}
	Chanel::~Chanel() {
		std::lock_guard guard(no_race);
		auto begin = suber.begin();
		auto end = suber.end();
		ValueItem val = _lazy_load_singleton_chanelDeath();
		while (begin != end) {
			if (begin->totalLinks() == 1)
				begin = suber.erase(begin);
			else {
				std::lock_guard item_guard((*begin)->res_lock);
				(*begin)->res_cache.push(val);
				(*begin)->allow_sub = false;
			}
		}
		notifier.notify_all();
	}
	void Chanel::notify(const ValueItem& val) {
		std::lock_guard guard(no_race);
		auto begin = suber.begin();
		auto end = suber.end();
		while (begin != end) {
			if (begin->totalLinks() == 1)
				begin = suber.erase(begin);
			else
				(*begin++)->put(val);
		}
	}
	void Chanel::notify(ValueItem* vals, uint32_t len) {
		std::lock_guard guard(no_race);
		auto begin = suber.begin();
		auto end = suber.end();
		while (begin != end) {
			std::lock_guard item_guard((*begin)->res_lock);
			if (begin->totalLinks() == 1)
				begin = suber.erase(begin);
			else {
				auto ibegin = vals;
				auto iend = vals + len;
				auto& cache = (*begin++)->res_cache;
				while (ibegin != iend)
					cache.push(*ibegin++);
			}
			(*begin)->res_await.notify_all();
		}
	}
	typed_lgr<ChanelHandler> Chanel::create_handle() {
		std::lock_guard guard(no_race);
		return suber.emplace_back(new ChanelHandler());
	}
	typed_lgr<ChanelHandler> Chanel::add_handle(typed_lgr<ChanelHandler> handler) {
		std::lock_guard guard(no_race);
		return suber.emplace_back(handler);
	}
	void Chanel::remove_handle(typed_lgr<ChanelHandler> handle) {
		std::lock_guard guard(no_race);
		auto end = suber.end();
		auto found = std::find(suber.begin(), end, handle);
		if (found == end)
			return;
		else {
			{
				std::lock_guard item_guard((*found)->res_lock);
				(*found)->allow_sub = false;
				(*found)->res_cache.push(_lazy_load_singleton_handleDeath());
			}
			suber.erase(found);
		}
	}




	ProxyClassDefine define_Chanel;
	ProxyClassDefine define_ChanelHandler;


	ValueItem* funs_Chanel_notify(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive only [class ptr] [any...]");
		else if (len == 2)
			getClass<Chanel>(vals)->notify((uint64_t)vals[1]);
		else
			getClass<Chanel>(vals)->notify(vals + 2, len - 2);
		return nullptr;
	}
	ValueItem* funs_Chanel_create_handle(ValueItem* vals, uint32_t len) {
		if (len < 1)
			throw InvalidArguments("That function recuive only [class ptr] [any...]");
		else
			return new ValueItem( new ProxyClass(new typed_lgr(getClass<Chanel>(vals)->create_handle()), &define_ChanelHandler));
	}
	ValueItem* funs_Chanel_remove_handle(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive only [class ptr] [any...]");
		else {
			getClass<Chanel>(vals)->remove_handle(getClass<ChanelHandler>(vals + 1));
			return nullptr;
		}
	}
	ValueItem* funs_Chanel_add_handle(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive only [class ptr] [any...]");
		else {
			getClass<Chanel>(vals)->add_handle(getClass<ChanelHandler>(vals + 1));
			return nullptr;
		}
	}


	ValueItem* funs_ChanelHandler_get(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive only [class ptr] [any...]");
		else
			return new ValueItem(getClass<ChanelHandler>(vals)->take());
	}
	ValueItem* funs_ChanelHandler_can_get(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive only [class ptr] [any...]");
		else
			return new ValueItem(getClass<ChanelHandler>(vals)->can_take());
	}
	ValueItem* funs_ChanelHandler_end_of_chanel(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive only [class ptr] [any...]");
		else
			return new ValueItem(getClass<ChanelHandler>(vals)->end_of_chanel());
	}
	ValueItem* funs_ChanelHandler_wait_item(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive only [class ptr] [any...]");
		else
			return new ValueItem(getClass<ChanelHandler>(vals)->wait_item());
	}

	void init() {
		define_Chanel.copy = AttachA::Interface::special::proxyCopy<Chanel, true>;
		define_Chanel.destructor = AttachA::Interface::special::proxyDestruct<Chanel, true>;
		define_Chanel.name = "chanel";
		define_Chanel.funs["notify"] = { new FuncEnviropment(funs_Chanel_notify,false),false,ClassAccess::pub };
		define_Chanel.funs["create_handle"] = { new FuncEnviropment(funs_Chanel_create_handle,false),false,ClassAccess::pub };
		define_Chanel.funs["remove_handle"] = { new FuncEnviropment(funs_Chanel_remove_handle,false),false,ClassAccess::pub };
		define_Chanel.funs["remove_handle"] = { new FuncEnviropment(funs_Chanel_remove_handle,false),false,ClassAccess::pub };
		define_Chanel.funs["add_handle"] = { new FuncEnviropment(funs_Chanel_add_handle,false),false,ClassAccess::pub };


		define_ChanelHandler.copy = AttachA::Interface::special::proxyCopy<ChanelHandler, true>;
		define_ChanelHandler.destructor = AttachA::Interface::special::proxyDestruct<ChanelHandler, true>;
		define_ChanelHandler.name = "chanel_handler";
		define_ChanelHandler.funs["get"] = { new FuncEnviropment(funs_ChanelHandler_get,false),false,ClassAccess::pub };
		define_ChanelHandler.funs["can_get"] = { new FuncEnviropment(funs_ChanelHandler_can_get,false),false,ClassAccess::pub };
		define_ChanelHandler.funs["end_of_chanel"] = { new FuncEnviropment(funs_ChanelHandler_end_of_chanel,false),false,ClassAccess::pub };
		define_ChanelHandler.funs["wait_item"] = { new FuncEnviropment(funs_ChanelHandler_wait_item,false),false,ClassAccess::pub };
	}
	namespace constructor {
		ValueItem* createProxy_Chanel(ValueItem*, uint32_t) {
			return new ValueItem(new ProxyClass(new typed_lgr(new Chanel()), &define_Chanel), VType::proxy);
		}
		ValueItem* createProxy_ChanelHandler(ValueItem*, uint32_t) {
			return new ValueItem(new ProxyClass(new typed_lgr(new ChanelHandler()), &define_ChanelHandler), VType::proxy);
		}
	}
}