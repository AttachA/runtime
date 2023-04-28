// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

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


	bool ChanelHandler__wait_item(std::unique_lock<MutexUnify>& ul, std::queue<ValueItem>& res_cache, bool& allow_sub, TaskConditionVariable& res_await) {
		while (res_cache.empty()) {
			if (!allow_sub)
				return false;
			res_await.wait(ul);
		}
		return true;
	}
	void ChanelHandler::put(const ValueItem& val) {
		std::lock_guard guard(res_mut);
		res_cache.push(val);
		res_await.notify_all();
	}
	ChanelHandler::ChanelHandler(){allow_sub = true;}
	ValueItem ChanelHandler::get() {
		ValueItem res;
		MutexUnify mu(res_mut);
		std::unique_lock ul(mu);
		while(res_cache.empty()) {
			if(!ChanelHandler__wait_item(ul, res_cache, allow_sub, res_await));
				throw AException("TaskDeath", "Detached from chanel");
		}
		res = std::move(res_cache.front());
		res_cache.pop();
		return res;
	}	
	ValueItem ChanelHandler::try_get() {
		ValueItem res;
		std::lock_guard lg(res_mut);
		if (res_cache.empty())
			return ValueItem{ false, res };
		else {
			res = std::move(res_cache.front());
			res_cache.pop();
			return ValueItem{ true, res };
		}
	}
	bool ChanelHandler::can_get() {
		if (!res_cache.empty())
			return true;
		return allow_sub;
	}
	bool ChanelHandler::end_of_chanel() {
		return allow_sub;
	}
	bool ChanelHandler::wait_item() {
		MutexUnify mu(res_mut);
		std::unique_lock ul(mu);
		return ChanelHandler__wait_item(ul, res_cache, allow_sub, res_await);
	}
	

	Chanel::Chanel() {}
	Chanel::~Chanel() {
		for(auto& it : auto_notifyer) {
			std::lock_guard guard(it->no_race);
			it->end_of_life = true;
		}
		std::lock_guard guard(no_race);
		auto_notifyer.clear();
		auto begin = suber.begin();
		auto end = suber.end();
		ValueItem val = _lazy_load_singleton_chanelDeath();
		while (begin != end) {
			(*begin)->allow_sub = false;
			(*begin)->res_cache.push(val);
			(*begin++)->res_await.notify_all();
		}
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

	ValueItem* _auto_notify_task(ValueItem* val, uint32_t len){
		AutoNotifyChanel* info = (AutoNotifyChanel*)val->getSourcePtr();		
		while(true){
			if(info->end_of_life)
				break;
			ValueItem* tmp = Task::get_result(info->handle_task, info->handle_from);
			std::lock_guard guard(info->no_race);
			if(info->end_of_life)
				break;
			if(tmp == nullptr)
				continue;
			info->chanel->notify(*tmp);
			delete tmp;
		}
		return nullptr;
	}
	typed_lgr<FuncEnviropment> auto_notify_task = new FuncEnviropment(_auto_notify_task, false);

	typed_lgr<AutoNotifyChanel> Chanel::auto_notify(typed_lgr<Task>& val){
		AutoNotifyChanel* res = new AutoNotifyChanel();
		{
			std::lock_guard guard(val->no_race);
			res->handle_from = val->fres.results.size();
		}
		res->chanel = this;
		res->notifier_task = new Task(auto_notify_task,ValueItem(res, VType::undefined_ptr));
		res->handle_task = val;
		Task::start(res->notifier_task);

		std::lock_guard guard(no_race);
		auto_notifyer.emplace_back(res);
		return res;
	}
	
	typed_lgr<AutoNotifyChanel> Chanel::auto_notify_continue(typed_lgr<Task>& val){
		AutoNotifyChanel* res = new AutoNotifyChanel();
		res->chanel = this;
		res->notifier_task = new Task(auto_notify_task,ValueItem(res, VType::undefined_ptr));
		res->handle_task = val;
		Task::start(res->notifier_task);

		std::lock_guard guard(no_race);
		auto_notifyer.emplace_back(res);
		return res;
	}
	
	typed_lgr<AutoNotifyChanel> Chanel::auto_notify_skip(typed_lgr<Task>& val, size_t start_from){
		AutoNotifyChanel* res = new AutoNotifyChanel();
		res->chanel = this;
		res->notifier_task = new Task(auto_notify_task,ValueItem(res, VType::undefined_ptr));
		res->handle_task = val;
		res->handle_from = start_from;
		Task::start(res->notifier_task);

		std::lock_guard guard(no_race);
		auto_notifyer.emplace_back(res);
		return res;
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
			(*found)->allow_sub = false;
			(*found)->res_cache.push(_lazy_load_singleton_handleDeath());
			suber.erase(found);
		}
	}
	void Chanel::remove_auto_notify(typed_lgr<AutoNotifyChanel> notifyer){
		{
			std::lock_guard guard(notifyer->no_race);
			notifyer->end_of_life = true;
		}
		std::lock_guard guard(no_race);
		auto end = auto_notifyer.end();
		auto found = std::find(auto_notifyer.begin(), end, notifyer);
		if (found == end)
			return;
		auto_notifyer.erase(found);
	}




	ProxyClassDefine define_Chanel;
	ProxyClassDefine define_ChanelHandler;
	ProxyClassDefine define_AutoNotifyChanel;


	ValueItem* funs_Chanel_notify(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive only [class ptr] [any...]");
		else if (len == 2)
			getClass<Chanel>(vals)->notify(vals[1]);
		else
			getClass<Chanel>(vals)->notify(vals + 2, len - 2);
		return nullptr;
	}
	ValueItem* funs_Chanel_auto_notify(ValueItem* vals, uint32_t len){
		if (len != 2)
			throw InvalidArguments("That function recuive only [class ptr] [async res / function]");
		switch (vals[1].meta.vtype) {
		case VType::async_res:{
			auto task = *(typed_lgr<Task>*)vals[1].getSourcePtr();
			return new ValueItem( new ProxyClass(new typed_lgr(getClass<Chanel>(vals)->auto_notify(task)), &define_AutoNotifyChanel));
		}
		case VType::function: {
			auto func = *(typed_lgr<FuncEnviropment>*)vals[1].getSourcePtr();
			ValueItem noting;
			typed_lgr task = new Task(func, noting);
			return new ValueItem( new ProxyClass(new typed_lgr(getClass<Chanel>(vals)->auto_notify(task)), &define_AutoNotifyChanel));
		}
		default:
			throw InvalidArguments("That function recuive only [class ptr] [any...]");
		}
	}
	ValueItem* funs_Chanel_auto_notify_continue(ValueItem* vals, uint32_t len){
		if (len != 2)
			throw InvalidArguments("That function recuive only [class ptr] [async res / function]");
		switch (vals[1].meta.vtype) {
		case VType::async_res:{
			auto task = *(typed_lgr<Task>*)vals[1].getSourcePtr();
			return new ValueItem( new ProxyClass(new typed_lgr(getClass<Chanel>(vals)->auto_notify_continue(task)), &define_AutoNotifyChanel));
		}
		case VType::function: {
			auto& func = *vals[1].funPtr();
			ValueItem noting;
			typed_lgr task = new Task(func, noting);
			return new ValueItem( new ProxyClass(new typed_lgr(getClass<Chanel>(vals)->auto_notify_continue(task)), &define_AutoNotifyChanel));
		}
		default:
			throw InvalidArguments("That function recuive only [class ptr] [any...]");
		}
	}
	ValueItem* funs_Chanel_auto_notify_skip(ValueItem* vals, uint32_t len){
		if (len != 3)
			throw InvalidArguments("That function recuive only [class ptr] [async res / function] [start from]");
		size_t start_from = (size_t)vals[2];
		switch (vals[1].meta.vtype) {
		case VType::async_res:{
			auto task = *(typed_lgr<Task>*)vals[1].getSourcePtr();
			return new ValueItem(new ProxyClass(new typed_lgr(getClass<Chanel>(vals)->auto_notify_skip(task,start_from)), &define_AutoNotifyChanel));
		}
		case VType::function: {
			auto& func = *vals[1].funPtr();
			ValueItem noting;
			typed_lgr task = new Task(func, noting);
			return new ValueItem(new ProxyClass(new typed_lgr(getClass<Chanel>(vals)->auto_notify_skip(task,start_from)), &define_AutoNotifyChanel));
		}
		default:
			throw InvalidArguments("That function recuive only [class ptr] [any...]");
		}
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
	ValueItem* funs_Chanel_remove_auto_notify(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive only [class ptr] [any...]");
		else {
			getClass<Chanel>(vals)->remove_auto_notify(getClass<AutoNotifyChanel>(vals + 1));
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
			return new ValueItem(getClass<ChanelHandler>(vals)->get());
	}
	ValueItem* funs_ChanelHandler_try_get(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive only [class ptr] [any...]");
		else
			return new ValueItem(getClass<ChanelHandler>(vals)->try_get());
	}
	ValueItem* funs_ChanelHandler_can_get(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive only [class ptr] [any...]");
		else
			return new ValueItem(getClass<ChanelHandler>(vals)->can_get());
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
		define_Chanel.funs["add_handle"] = { new FuncEnviropment(funs_Chanel_add_handle,false),false,ClassAccess::pub };
		define_Chanel.funs["auto_notify"] = { new FuncEnviropment(funs_Chanel_auto_notify,false),false,ClassAccess::pub };
		define_Chanel.funs["auto_notify_continue"] = { new FuncEnviropment(funs_Chanel_auto_notify_continue,false),false,ClassAccess::pub };
		define_Chanel.funs["auto_notify_skip"] = { new FuncEnviropment(funs_Chanel_auto_notify_skip,false),false,ClassAccess::pub };
		define_Chanel.funs["remove_auto_notify"] = { new FuncEnviropment(funs_Chanel_remove_auto_notify,false),false,ClassAccess::pub };


		define_ChanelHandler.copy = AttachA::Interface::special::proxyCopy<ChanelHandler, true>;
		define_ChanelHandler.destructor = AttachA::Interface::special::proxyDestruct<ChanelHandler, true>;
		define_ChanelHandler.name = "chanel_handler";
		define_ChanelHandler.funs["get"] = { new FuncEnviropment(funs_ChanelHandler_get,false),false,ClassAccess::pub };
		define_ChanelHandler.funs["try_get"] = { new FuncEnviropment(funs_ChanelHandler_get,false),false,ClassAccess::pub };
		define_ChanelHandler.funs["can_get"] = { new FuncEnviropment(funs_ChanelHandler_can_get,false),false,ClassAccess::pub };
		define_ChanelHandler.funs["end_of_chanel"] = { new FuncEnviropment(funs_ChanelHandler_end_of_chanel,false),false,ClassAccess::pub };
		define_ChanelHandler.funs["wait_item"] = { new FuncEnviropment(funs_ChanelHandler_wait_item,false),false,ClassAccess::pub };

		define_AutoNotifyChanel.copy = AttachA::Interface::special::proxyCopy<AutoNotifyChanel, true>;
		define_AutoNotifyChanel.destructor = AttachA::Interface::special::proxyDestruct<AutoNotifyChanel, true>;
		define_AutoNotifyChanel.name = "auto_notify_chanel";
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