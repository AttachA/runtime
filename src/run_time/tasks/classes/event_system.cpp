// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/AttachA_CXX.hpp>
#include <run_time/asm/FuncEnvironment.hpp>
#include <run_time/asm/attacha_environment.hpp>
#include <run_time/tasks.hpp>

namespace art {
    bool EventSystem::removeOne(std::list<art::shared_ptr<FuncEnvironment>>& list, const art::shared_ptr<FuncEnvironment>& func) {
        auto iter = list.begin();
        auto end = list.begin();
        while (iter != end) {
            if (*iter == func) {
                list.erase(iter);
                return true;
            }
            ++iter;
        }
        return false;
    }

    void EventSystem::async_call(std::list<art::shared_ptr<FuncEnvironment>>& list, ValueItem& args) {
        std::list<art::shared_ptr<FuncEnvironment>> copy;
        {
            art::lock_guard guard(no_race);
            copy = list;
        }
        for (auto& it : copy)
            Task::start(new Task(it, args));
    }

    bool EventSystem::awaitCall(std::list<art::shared_ptr<FuncEnvironment>>& list, ValueItem& args) {
        std::vector<art::typed_lgr<Task>> wait_tasks;
        {
            std::list<art::shared_ptr<FuncEnvironment>> copy;
            {
                art::lock_guard guard(no_race);
                copy = list;
            }
            wait_tasks.reserve(list.size());
            for (auto& it : copy) {
                art::typed_lgr<Task> tsk(new Task(it, args));
                wait_tasks.emplace_back(tsk);
                Task::start(tsk);
            }
        }
        bool need_cancel = false;
        for (auto& it : wait_tasks) {
            if (need_cancel) {
                it->make_cancel = true;
                continue;
            }
            auto res = Task::get_result(it);
            if (res) {
                if ((bool)*res)
                    need_cancel = true;
                delete res;
            }
        }
        return need_cancel;
    }

    bool EventSystem::sync_call(std::list<art::shared_ptr<FuncEnvironment>>& list, ValueItem& args) {
        std::list<art::shared_ptr<FuncEnvironment>> copy;
        {
            art::lock_guard guard(no_race);
            copy = list;
        }
        if (args.meta.vtype == VType::async_res)
            args.getAsync();
        if (args.meta.vtype == VType::noting) {
            for (art::shared_ptr<FuncEnvironment>& it : copy)
                if ((bool)CXX::cxxCall(it))
                    return true;
        } else
            for (art::shared_ptr<FuncEnvironment>& it : copy)
                if ((bool)CXX::aCall(it, args))
                    return true;
        return false;
    }

    void EventSystem::operator+=(const art::shared_ptr<FuncEnvironment>& func) {
        art::lock_guard guard(no_race);
        avg_priority.push_back(func);
    }

    void EventSystem::join(const art::shared_ptr<FuncEnvironment>& func, bool async_mode, Priority priority) {
        art::lock_guard guard(no_race);
        if (async_mode) {
            switch (priority) {
            case Priority::heigh:
                async_heigh_priority.push_back(func);
                break;
            case Priority::upper_avg:
                async_upper_avg_priority.push_back(func);
                break;
            case Priority::avg:
                async_avg_priority.push_back(func);
                break;
            case Priority::lower_avg:
                async_lower_avg_priority.push_back(func);
                break;
            case Priority::low:
                async_low_priority.push_back(func);
                break;
            default:
                break;
            }
        } else {
            switch (priority) {
            case Priority::heigh:
                heigh_priority.push_back(func);
                break;
            case Priority::upper_avg:
                upper_avg_priority.push_back(func);
                break;
            case Priority::avg:
                avg_priority.push_back(func);
                break;
            case Priority::lower_avg:
                lower_avg_priority.push_back(func);
                break;
            case Priority::low:
                low_priority.push_back(func);
                break;
            default:
                break;
            }
        }
    }

    bool EventSystem::leave(const art::shared_ptr<FuncEnvironment>& func, bool async_mode, Priority priority) {
        art::lock_guard guard(no_race);
        if (async_mode) {
            switch (priority) {
            case Priority::heigh:
                return removeOne(async_heigh_priority, func);
            case Priority::upper_avg:
                return removeOne(async_upper_avg_priority, func);
            case Priority::avg:
                return removeOne(async_avg_priority, func);
            case Priority::lower_avg:
                return removeOne(async_lower_avg_priority, func);
            case Priority::low:
                return removeOne(async_low_priority, func);
            default:
                return false;
            }
        } else {
            switch (priority) {
            case Priority::heigh:
                return removeOne(heigh_priority, func);
            case Priority::upper_avg:
                return removeOne(upper_avg_priority, func);
            case Priority::avg:
                return removeOne(avg_priority, func);
            case Priority::lower_avg:
                return removeOne(lower_avg_priority, func);
            case Priority::low:
                return removeOne(low_priority, func);
            default:
                return false;
            }
        }
    }

    bool EventSystem::await_notify(const ValueItem& it) {
        return await_notify(ValueItem(it));
    }

    bool EventSystem::notify(const ValueItem& it) {
        return notify(ValueItem(it));
    }

    bool EventSystem::sync_notify(const ValueItem& it) {
        return sync_notify(ValueItem(it));
    }

    bool EventSystem::await_notify(ValueItem&& it) {
        if (sync_call(heigh_priority, it))
            return true;
        if (sync_call(upper_avg_priority, it))
            return true;
        if (sync_call(avg_priority, it))
            return true;
        if (sync_call(lower_avg_priority, it))
            return true;
        if (sync_call(low_priority, it))
            return true;

        if (awaitCall(async_heigh_priority, it))
            return true;
        if (awaitCall(async_upper_avg_priority, it))
            return true;
        if (awaitCall(async_avg_priority, it))
            return true;
        if (awaitCall(async_lower_avg_priority, it))
            return true;
        if (awaitCall(async_low_priority, it))
            return true;
        return false;
    }

    bool EventSystem::notify(ValueItem&& it) {
        if (sync_call(heigh_priority, it))
            return true;
        if (sync_call(upper_avg_priority, it))
            return true;
        if (sync_call(avg_priority, it))
            return true;
        if (sync_call(lower_avg_priority, it))
            return true;
        if (sync_call(low_priority, it))
            return true;

        async_call(async_heigh_priority, it);
        async_call(async_upper_avg_priority, it);
        async_call(async_avg_priority, it);
        async_call(async_lower_avg_priority, it);
        async_call(async_low_priority, it);
        return false;
    }

    bool EventSystem::sync_notify(ValueItem&& it) {
        if (sync_call(heigh_priority, it))
            return true;
        if (sync_call(upper_avg_priority, it))
            return true;
        if (sync_call(avg_priority, it))
            return true;
        if (sync_call(lower_avg_priority, it))
            return true;
        if (sync_call(low_priority, it))
            return true;
        if (sync_call(async_heigh_priority, it))
            return true;
        if (sync_call(async_upper_avg_priority, it))
            return true;
        if (sync_call(async_avg_priority, it))
            return true;
        if (sync_call(async_lower_avg_priority, it))
            return true;
        if (sync_call(async_low_priority, it))
            return true;
        return false;
    }

    struct event_ref_holder {
        typed_lgr<EventSystem>* event;

        event_ref_holder(typed_lgr<EventSystem>& ref)
            : event(new typed_lgr<EventSystem>(ref)) {}

        event_ref_holder(const event_ref_holder& ref)
            : event(new typed_lgr<EventSystem>(ref.event ? *ref.event : nullptr)) {}

        event_ref_holder(event_ref_holder&& ref)
            : event(ref.event) {
            ref.event = nullptr;
        }

        ~event_ref_holder() {
            if (event)
                delete event;
            event = nullptr;
        }
    };

    ValueItem* __async_notify(ValueItem* vals, uint32_t) {
        typed_lgr<EventSystem> es = *CXX::Interface::getExtractAsStatic<event_ref_holder>(*vals).event;
        ValueItem& args = vals[1];
        if (es->sync_call(es->heigh_priority, args))
            return new ValueItem(true);
        if (es->sync_call(es->upper_avg_priority, args))
            return new ValueItem(true);
        if (es->sync_call(es->avg_priority, args))
            return new ValueItem(true);
        if (es->sync_call(es->lower_avg_priority, args))
            return new ValueItem(true);
        if (es->sync_call(es->low_priority, args))
            return new ValueItem(true);
        if (es->awaitCall(es->async_heigh_priority, args))
            return new ValueItem(true);
        if (es->awaitCall(es->async_upper_avg_priority, args))
            return new ValueItem(true);
        if (es->awaitCall(es->async_avg_priority, args))
            return new ValueItem(true);
        if (es->awaitCall(es->async_lower_avg_priority, args))
            return new ValueItem(true);
        if (es->awaitCall(es->async_low_priority, args))
            return new ValueItem(true);
        return new ValueItem(false);
    }

    art::shared_ptr<FuncEnvironment>& _async_notify = attacha_environment::create_fun_env(new FuncEnvironment(__async_notify, false, false));

    void init_event_ref_holder() {
        static std::atomic_bool inited = false;
        if (!inited.exchange(true)) {
            CXX::Interface::typeVTable<event_ref_holder>() = CXX::Interface::createProxyTable<event_ref_holder>("event_ref_holder");
        }
    }

    art::typed_lgr<Task> EventSystem::async_notify(typed_lgr<EventSystem>& self, const ValueItem& args) {
        return async_notify(self, ValueItem(args));
    }

    art::typed_lgr<Task> EventSystem::async_notify(typed_lgr<EventSystem>& self, ValueItem&& args) {
        init_event_ref_holder();
        art::typed_lgr<Task> res = new Task(
            _async_notify,
            ValueItem{
                ValueItem(
                    CXX::Interface::constructStructure<event_ref_holder>(
                        (AttachAVirtualTable*)CXX::Interface::typeVTableReadOnly<event_ref_holder>(),
                        self
                    ),
                    no_copy
                ),
                std::move(args)
            }
        );
        Task::start(res);
        return res;
    }
}