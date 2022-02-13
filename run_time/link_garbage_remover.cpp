#include "link_garbage_remover.hpp"
thread_local std::unordered_set<const void*> __lgr_safe_deph;

