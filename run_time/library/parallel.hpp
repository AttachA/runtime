#pragma once
#include "../attacha_abi_structs.hpp"

namespace parallel {
	void init();
	namespace constructor {
		ValueItem* createProxy_ConditionVariable(ValueItem*, uint32_t);
		ValueItem* createProxy_Mutex(ValueItem*, uint32_t);
		ValueItem* createProxy_Semaphore(ValueItem*, uint32_t);

		//1 arg [string]
		ValueItem* createProxy_ConcurentFile(ValueItem*, uint32_t);

		ValueItem* createProxy_EventSystem(ValueItem*, uint32_t);
		ValueItem* createProxy_TaskLimiter(ValueItem*, uint32_t);
		//ValueItem* createProxy_ValueMonitor(ValueItem*, uint32_t);
		//ValueItem* createProxy_ValueChangeMonitor(ValueItem*, uint32_t);
	}

	//typed_lgr<FuncEnviropment>*
	ValueItem* createThread(ValueItem*, uint32_t);
}