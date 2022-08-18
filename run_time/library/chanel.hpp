#pragma once
#include <list>
#include <queue>
#include "../tasks.hpp"
#include "../attacha_abi_structs.hpp"
namespace chanel {
	class ChanelHandler {
		TaskConditionVariable res_await;
		TaskMutex res_lock;
		std::queue<ValueItem> res_cache;
		bool allow_sub = true;
		inline void put(const ValueItem& val);
		friend class Chanel;
	public:
		ChanelHandler();
		ValueItem take();
		bool can_take();
		bool end_of_chanel();
		bool wait_item();
	};
	class Chanel {
		TaskMutex no_race;
		TaskConditionVariable notifier;
		std::list<typed_lgr<ChanelHandler>> suber;

	public:
		Chanel();
		~Chanel();
		void notify(const ValueItem& val);
		void notify(ValueItem* vals, uint32_t len);
		typed_lgr<ChanelHandler> create_handle();
		typed_lgr<ChanelHandler> add_handle(typed_lgr<ChanelHandler> handler);
		void remove_handle(typed_lgr<ChanelHandler> handle);
	};


	void init();
	namespace constructor {
		ValueItem* createProxy_Chanel(ValueItem*, uint32_t);
		ValueItem* createProxy_ChanelHandler(ValueItem*, uint32_t);
	}
}