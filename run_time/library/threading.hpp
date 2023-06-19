#ifndef RUN_TIME_LIBRARY_THREADING
#define RUN_TIME_LIBRARY_THREADING
#include "../attacha_abi_structs.hpp"
namespace threading{
    namespace constructor{
        ValueItem* thread(ValueItem* args, uint32_t len);
        ValueItem* main_thread(ValueItem* args, uint32_t len);
        
    }
}

#endif /* RUN_TIME_LIBRARY_THREADING */
