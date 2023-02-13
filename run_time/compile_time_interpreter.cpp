#include "run_time_compiler.hpp"
#include "tasks.hpp"
#include <asmjit/core/cpuinfo.h>
#include "tools.hpp"
#include "attacha_abi.hpp"
using namespace run_time;

std::tuple<std::vector<uint8_t>,uint16_t,bool,bool> build(list_array<ValueItem>& its, const std::vector<uint8_t>& code) {
	throw DeprecatedException();
}
