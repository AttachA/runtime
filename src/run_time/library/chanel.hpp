// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <list>
#include <queue>
#include "../tasks.hpp"
#include "../attacha_abi_structs.hpp"
namespace chanel {
	class ChanelHandler {
		TaskMutex res_mut;
		TaskConditionVariable res_await;
		std::queue<ValueItem> res_cache;
		bool allow_sub;
		inline void put(const ValueItem& val);
		friend class Chanel;
	public:
		ChanelHandler();
		ValueItem get();
		ValueItem try_get();
		bool can_get();
		bool end_of_chanel();
		bool wait_item();
	};
	class Chanel;
	struct AutoNotifyChanel  {
		typed_lgr<Task> notifier_task;
		typed_lgr<Task> handle_task;
		Chanel* chanel;
		bool end_of_life;
		TaskMutex no_race;
		size_t handle_from = 0;
	};
	struct AutoEventChanel{
		typed_lgr<EventSystem> notifier_event;
		enum class NotifyType {
			default_,
			sync,
			async,
			await
		} ntype;
		bool notify(const ValueItem&);
	};
	class Chanel {
		TaskMutex no_race;
		std::list<typed_lgr<ChanelHandler>> suber;
		std::list<typed_lgr<AutoNotifyChanel>> auto_notifyer;
		std::list<typed_lgr<AutoEventChanel>> auto_events;
	public:
		Chanel();
		~Chanel();
		void notify(ValueItem&& val);
		void notify(const ValueItem& val);
		void notify(ValueItem* vals, uint32_t len);
		typed_lgr<AutoNotifyChanel> auto_notify(typed_lgr<Task>& val);
		typed_lgr<AutoNotifyChanel> auto_notify_continue(typed_lgr<Task>& val);
		typed_lgr<AutoNotifyChanel> auto_notify_skip(typed_lgr<Task>& val, size_t start_from);
		
		typed_lgr<AutoEventChanel> auto_event(typed_lgr<EventSystem>& val, AutoEventChanel::NotifyType type);

		typed_lgr<ChanelHandler> create_handle();
		typed_lgr<ChanelHandler> add_handle(typed_lgr<ChanelHandler> handler);
		void remove_handle(typed_lgr<ChanelHandler> handle);
		void remove_auto_notify(typed_lgr<AutoNotifyChanel> notifyer);
		void remove_auto_event(typed_lgr<AutoEventChanel> notifyer);
	};


	void init();
	namespace constructor {
		ValueItem* createProxy_Chanel(ValueItem*, uint32_t);
		ValueItem* createProxy_ChanelHandler(ValueItem*, uint32_t);
	}
}