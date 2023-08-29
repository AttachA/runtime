// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/AttachA_CXX.hpp>
#include <run_time/library/chanel.hpp>
#include <run_time/tasks.hpp>

namespace art {
    namespace chanel {
        ValueItem _lazy_load_singleton_chanelDeath() {
            static ValueItem val;
            if (val.meta.vtype == VType::noting) {
                static TaskMutex m;
                std::lock_guard guard(m);
                if (val.meta.vtype == VType::noting) {
                    try {
                        throw AException("TaskDeath", "Chanel died before all handlers unsubbed");
                    } catch (...) {
                        val = std::current_exception();
                    }
                }
            }
            return val;
        }

        ValueItem _lazy_load_singleton_handleDeath() {
            static ValueItem val;
            if (val.meta.vtype == VType::noting) {
                static TaskMutex m;
                std::lock_guard guard(m);
                if (val.meta.vtype == VType::noting) {
                    try {
                        throw AException("TaskDeath", "Detached from chanel");
                    } catch (...) {
                        val = std::current_exception();
                    }
                }
            }
            return val;
        }

        bool ChanelHandler__wait_item(art::unique_lock<MutexUnify>& ul, std::queue<ValueItem>& res_cache, bool& allow_sub, TaskConditionVariable& res_await) {
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

        ChanelHandler::ChanelHandler() {
            allow_sub = true;
        }

        ValueItem ChanelHandler::get() {
            ValueItem res;
            MutexUnify mu(res_mut);
            art::unique_lock ul(mu);
            while (res_cache.empty()) {
                if (!ChanelHandler__wait_item(ul, res_cache, allow_sub, res_await))
                    ;
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
                return ValueItem{false, res};
            else {
                res = std::move(res_cache.front());
                res_cache.pop();
                return ValueItem{true, res};
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
            art::unique_lock ul(mu);
            return ChanelHandler__wait_item(ul, res_cache, allow_sub, res_await);
        }

        bool AutoEventChanel::notify(const ValueItem& val) {
            if (!notifier_event.is_deleted()) {
                switch (ntype) {
                case NotifyType::default_:
                    return notifier_event->notify(const_cast<ValueItem&>(val));
                case NotifyType::sync:
                    return notifier_event->sync_notify(const_cast<ValueItem&>(val));
                case NotifyType::async:
                    return Task::get_result(notifier_event->async_notify(const_cast<ValueItem&>(val)));
                case NotifyType::await:
                    return notifier_event->await_notify(const_cast<ValueItem&>(val));
                default:
                    return false;
                }
            } else
                return false;
        }

        Chanel::Chanel() {}

        Chanel::~Chanel() {
            for (auto& it : auto_notifier) {
                std::lock_guard guard(it->no_race);
                it->end_of_life = true;
            }
            std::lock_guard guard(no_race);
            auto_notifier.clear();
            auto begin = subscribers.begin();
            auto end = subscribers.end();
            ValueItem val = _lazy_load_singleton_chanelDeath();
            while (begin != end) {
                (*begin)->allow_sub = false;
                (*begin)->res_cache.push(val);
                (*begin++)->res_await.notify_all();
            }
        }

        void Chanel::notify(const ValueItem& val) {
            std::lock_guard guard(no_race);
            {
                auto begin = subscribers.begin();
                auto end = subscribers.end();
                while (begin != end) {
                    if (begin->totalLinks() == 1)
                        begin = subscribers.erase(begin);
                    else
                        (*begin++)->put(val);
                }
            }
            {
                auto begin = auto_events.begin();
                auto end = auto_events.end();
                while (begin != end) {
                    if ((*begin)->notifier_event.is_deleted())
                        begin = auto_events.erase(begin);
                    else
                        (*begin++)->notify(val);
                }
            }
        }

        void Chanel::notify(ValueItem&& val) {
            std::lock_guard guard(no_race);
            {
                auto begin = subscribers.begin();
                auto end = subscribers.end();
                while (begin != end) {
                    if (begin->totalLinks() == 1)
                        begin = subscribers.erase(begin);
                    else
                        (*begin++)->put(std::move(val));
                }
            }
            {
                auto begin = auto_events.begin();
                auto end = auto_events.end();
                while (begin != end) {
                    if ((*begin)->notifier_event.is_deleted())
                        begin = auto_events.erase(begin);
                    else
                        (*begin++)->notify(val);
                }
            }
        }

        void Chanel::notify(ValueItem* vals, uint32_t len) {
            std::lock_guard guard(no_race);
            {
                auto begin = subscribers.begin();
                auto end = subscribers.end();
                while (begin != end) {
                    if (begin->totalLinks() == 1)
                        begin = subscribers.erase(begin);
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
            {
                auto begin = auto_events.begin();
                auto end = auto_events.end();
                while (begin != end) {
                    if ((*begin)->notifier_event.is_deleted())
                        begin = auto_events.erase(begin);
                    else {
                        auto ibegin = vals;
                        auto iend = vals + len;
                        while (ibegin != iend)
                            (*begin++)->notify(*ibegin++);
                    }
                }
            }
        }

        ValueItem* _auto_notify_task(ValueItem* val, uint32_t len) {
            AutoNotifyChanel* info = (AutoNotifyChanel*)val->getSourcePtr();
            while (true) {
                if (info->end_of_life)
                    break;
                ValueItem* tmp = Task::get_result(info->handle_task, info->handle_from);
                std::lock_guard guard(info->no_race);
                if (info->end_of_life)
                    break;
                if (tmp == nullptr)
                    continue;
                info->chanel->notify(*tmp);
                delete tmp;
            }
            return nullptr;
        }

        art::shared_ptr<FuncEnvironment> auto_notify_task = new FuncEnvironment(_auto_notify_task, false);

        typed_lgr<AutoNotifyChanel> Chanel::auto_notify(art::shared_ptr<Task>& val) {
            AutoNotifyChanel* res = new AutoNotifyChanel();
            {
                std::lock_guard guard(val->no_race);
                res->handle_from = val->fres.results.size();
            }
            res->chanel = this;
            res->notifier_task = new Task(auto_notify_task, ValueItem(res, VType::undefined_ptr));
            res->handle_task = val;
            Task::start(res->notifier_task);

            std::lock_guard guard(no_race);
            auto_notifier.emplace_back(res);
            return res;
        }

        typed_lgr<AutoNotifyChanel> Chanel::auto_notify_continue(art::shared_ptr<Task>& val) {
            AutoNotifyChanel* res = new AutoNotifyChanel();
            res->chanel = this;
            res->notifier_task = new Task(auto_notify_task, ValueItem(res, VType::undefined_ptr));
            res->handle_task = val;
            Task::start(res->notifier_task);

            std::lock_guard guard(no_race);
            auto_notifier.emplace_back(res);
            return res;
        }

        typed_lgr<AutoNotifyChanel> Chanel::auto_notify_skip(art::shared_ptr<Task>& val, size_t start_from) {
            AutoNotifyChanel* res = new AutoNotifyChanel();
            res->chanel = this;
            res->notifier_task = new Task(auto_notify_task, ValueItem(res, VType::undefined_ptr));
            res->handle_task = val;
            res->handle_from = start_from;
            Task::start(res->notifier_task);

            std::lock_guard guard(no_race);
            auto_notifier.emplace_back(res);
            return res;
        }

        typed_lgr<AutoEventChanel> Chanel::auto_event(typed_lgr<EventSystem>& event, AutoEventChanel::NotifyType type) {
            AutoEventChanel* res = new AutoEventChanel();
            res->notifier_event = event;
            res->ntype = type;
            std::lock_guard guard(no_race);
            auto_events.emplace_back(res);
            return res;
        }

        typed_lgr<ChanelHandler> Chanel::create_handle() {
            std::lock_guard guard(no_race);
            return subscribers.emplace_back(new ChanelHandler());
        }

        typed_lgr<ChanelHandler> Chanel::add_handle(typed_lgr<ChanelHandler> handler) {
            std::lock_guard guard(no_race);
            return subscribers.emplace_back(handler);
        }

        void Chanel::remove_handle(typed_lgr<ChanelHandler> handle) {
            std::lock_guard guard(no_race);
            auto end = subscribers.end();
            auto found = std::find(subscribers.begin(), end, handle);
            if (found == end)
                return;
            else {
                (*found)->allow_sub = false;
                (*found)->res_cache.push(_lazy_load_singleton_handleDeath());
                subscribers.erase(found);
            }
        }

        void Chanel::remove_auto_notify(typed_lgr<AutoNotifyChanel> notifier) {
            {
                std::lock_guard guard(notifier->no_race);
                notifier->end_of_life = true;
            }
            std::lock_guard guard(no_race);
            auto end = auto_notifier.end();
            auto found = std::find(auto_notifier.begin(), end, notifier);
            if (found == end)
                return;
            auto_notifier.erase(found);
        }

        void Chanel::remove_auto_event(typed_lgr<AutoEventChanel> notifier) {
            std::lock_guard guard(no_race);
            auto end = auto_events.end();
            auto found = std::find(auto_events.begin(), end, notifier);
            if (found == end)
                return;
            auto_events.erase(found);
        }

        AttachAVirtualTable* define_Chanel;
        AttachAVirtualTable* define_ChanelHandler;
        AttachAVirtualTable* define_AutoNotifyChanel;
        AttachAVirtualTable* define_AutoEventChanel;

        AttachAFun(funs_Chanel_notify, 2, {
            if (len == 2)
                CXX::Interface::getExtractAs<typed_lgr<Chanel>>(args[0], define_Chanel)->notify(args[0]);
            else
                CXX::Interface::getExtractAs<typed_lgr<Chanel>>(args[0], define_Chanel)->notify(args + 2, len - 2);
        }) AttachAFun(funs_Chanel_auto_notify, 2, {
            art::shared_ptr<Task> task;
            switch (args[1].meta.vtype) {
            case VType::async_res:
                task = *(art::shared_ptr<Task>*)args[1].getSourcePtr();
                break;
            case VType::function:
                task = new Task(*args[1].funPtr(), {});
                break;
            default:
                throw InvalidArguments("That function receives [class ptr] [any...]");
            }
            return ValueItem(art::CXX::Interface::constructStructure<typed_lgr<AutoNotifyChanel>>(define_AutoNotifyChanel,
                                                                                                  art::CXX::Interface::getExtractAs<typed_lgr<Chanel>>(args[0], define_Chanel)->auto_notify(task)),
                             no_copy);
        }) AttachAFun(funs_Chanel_auto_notify_continue, 2, {
            art::shared_ptr<Task> task;
            switch (args[1].meta.vtype) {
            case VType::async_res:
                task = *(art::shared_ptr<Task>*)args[1].getSourcePtr();
                break;
            case VType::function:
                task = new Task(*args[1].funPtr(), {});
                break;
            default:
                throw InvalidArguments("That function receives [class ptr] [async res / function]");
            }
            return ValueItem(art::CXX::Interface::constructStructure<typed_lgr<AutoNotifyChanel>>(define_AutoNotifyChanel,
                                                                                                  art::CXX::Interface::getExtractAs<typed_lgr<Chanel>>(args[0], define_Chanel)->auto_notify_continue(task)),
                             no_copy);
        }) AttachAFun(funs_Chanel_auto_notify_skip, 3, {
            art::shared_ptr<Task> task;
            switch (args[1].meta.vtype) {
            case VType::async_res:
                task = *(art::shared_ptr<Task>*)args[1].getSourcePtr();
                break;
            case VType::function:
                task = new Task(*args[1].funPtr(), {});
                break;
            default:
                throw InvalidArguments("That function receives [class ptr] [async res / function] [start from]");
            }
            return ValueItem(art::CXX::Interface::constructStructure<typed_lgr<AutoNotifyChanel>>(define_AutoNotifyChanel,
                                                                                                  art::CXX::Interface::getExtractAs<typed_lgr<Chanel>>(args[0], define_Chanel)->auto_notify_skip(task, (size_t)args[2])),
                             no_copy);
        }) AttachAFun(funs_Chanel_auto_event, 3, {
            art::CXX::Interface::getExtractAs<typed_lgr<Chanel>>(args[0], define_Chanel)->auto_event(art::CXX::Interface::getExtractAs<typed_lgr<EventSystem>>(args[1], (AttachAVirtualTable*)art::CXX::Interface::typeVTable<EventSystem>()), (AutoEventChanel::NotifyType)(uint8_t)args[2]);
        }) AttachAFun(funs_Chanel_create_handle, 1, {
            return ValueItem(art::CXX::Interface::constructStructure<typed_lgr<ChanelHandler>>(define_ChanelHandler,
                                                                                               art::CXX::Interface::getExtractAs<typed_lgr<Chanel>>(args[0], define_Chanel)->create_handle()),
                             no_copy);
        }) AttachAFun(funs_Chanel_remove_handle, 2, {
            art::CXX::Interface::getExtractAs<typed_lgr<Chanel>>(args[0], define_Chanel)->remove_handle(art::CXX::Interface::getExtractAs<typed_lgr<ChanelHandler>>(args[1], define_ChanelHandler));
        }) AttachAFun(funs_Chanel_remove_auto_notify, 2, {
            art::CXX::Interface::getExtractAs<typed_lgr<Chanel>>(args[0], define_Chanel)->remove_auto_notify(art::CXX::Interface::getExtractAs<typed_lgr<AutoNotifyChanel>>(args[1], define_AutoNotifyChanel));
        }) AttachAFun(funs_Chanel_remove_auto_event, 2, {
            art::CXX::Interface::getExtractAs<typed_lgr<Chanel>>(args[0], define_Chanel)->remove_auto_event(art::CXX::Interface::getExtractAs<typed_lgr<AutoEventChanel>>(args[1], define_AutoEventChanel));
        }) AttachAFun(funs_Chanel_add_handle, 2, {
            art::CXX::Interface::getExtractAs<typed_lgr<Chanel>>(args[0], define_Chanel)->add_handle(art::CXX::Interface::getExtractAs<typed_lgr<ChanelHandler>>(args[1], define_ChanelHandler));
        })

            AttachAFun(funs_ChanelHandler_get, 2, {
                return art::CXX::Interface::getExtractAs<typed_lgr<ChanelHandler>>(args[0], define_ChanelHandler)->get();
            }) AttachAFun(funs_ChanelHandler_try_get, 2, {
                return art::CXX::Interface::getExtractAs<typed_lgr<ChanelHandler>>(args[0], define_ChanelHandler)->try_get();
            }) AttachAFun(funs_ChanelHandler_can_get, 2, {
                return art::CXX::Interface::getExtractAs<typed_lgr<ChanelHandler>>(args[0], define_ChanelHandler)->can_get();
            }) AttachAFun(funs_ChanelHandler_end_of_chanel, 2, {
                return art::CXX::Interface::getExtractAs<typed_lgr<ChanelHandler>>(args[0], define_ChanelHandler)->end_of_chanel();
            }) AttachAFun(funs_ChanelHandler_wait_item, 2, {
                art::CXX::Interface::getExtractAs<typed_lgr<ChanelHandler>>(args[0], define_ChanelHandler)->wait_item();
            })

                void init() {
            if (define_Chanel != nullptr)
                return;
            define_Chanel = art::CXX::Interface::createTable<typed_lgr<Chanel>>("chanel",
                                                                                art::CXX::Interface::direct_method("notify", funs_Chanel_notify),
                                                                                art::CXX::Interface::direct_method("create_handle", funs_Chanel_create_handle),
                                                                                art::CXX::Interface::direct_method("remove_handle", funs_Chanel_remove_handle),
                                                                                art::CXX::Interface::direct_method("add_handle", funs_Chanel_add_handle),
                                                                                art::CXX::Interface::direct_method("auto_notify", funs_Chanel_auto_notify),
                                                                                art::CXX::Interface::direct_method("auto_event", funs_Chanel_auto_event),
                                                                                art::CXX::Interface::direct_method("auto_notify_continue", funs_Chanel_auto_notify_continue),
                                                                                art::CXX::Interface::direct_method("auto_notify_skip", funs_Chanel_auto_notify_skip),
                                                                                art::CXX::Interface::direct_method("remove_auto_notify", funs_Chanel_remove_auto_notify),
                                                                                art::CXX::Interface::direct_method("remove_auto_event", funs_Chanel_remove_auto_event));

            define_ChanelHandler = art::CXX::Interface::createTable<typed_lgr<ChanelHandler>>("chanel_handler",
                                                                                              art::CXX::Interface::direct_method("get", funs_ChanelHandler_get),
                                                                                              art::CXX::Interface::direct_method("try_get", funs_ChanelHandler_try_get),
                                                                                              art::CXX::Interface::direct_method("can_get", funs_ChanelHandler_can_get),
                                                                                              art::CXX::Interface::direct_method("end_of_chanel", funs_ChanelHandler_end_of_chanel),
                                                                                              art::CXX::Interface::direct_method("wait_item", funs_ChanelHandler_wait_item));

            define_AutoNotifyChanel = art::CXX::Interface::createTable<typed_lgr<AutoNotifyChanel>>("auto_notify_chanel");
            define_AutoEventChanel = art::CXX::Interface::createTable<typed_lgr<AutoEventChanel>>("auto_event_chanel");
            art::CXX::Interface::typeVTable<typed_lgr<Chanel>>() = define_Chanel;
            art::CXX::Interface::typeVTable<typed_lgr<ChanelHandler>>() = define_ChanelHandler;
            art::CXX::Interface::typeVTable<typed_lgr<AutoEventChanel>>() = define_AutoEventChanel;
        }

        namespace constructor {
            ValueItem* createProxy_Chanel(ValueItem*, uint32_t) {
                return new ValueItem(art::CXX::Interface::constructStructure<typed_lgr<Chanel>>(define_Chanel, new Chanel()), no_copy);
            }

            ValueItem* createProxy_ChanelHandler(ValueItem*, uint32_t) {
                return new ValueItem(art::CXX::Interface::constructStructure<typed_lgr<ChanelHandler>>(define_ChanelHandler, new ChanelHandler()), no_copy);
            }
        }
    }
}