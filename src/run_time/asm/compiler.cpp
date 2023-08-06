#if defined(_M_AMD64) || defined(__amd64__)
	#include "compiler/dynamic/amd64/_any_os.hpp"
	#include "compiler/static/amd64/_any_os.hpp"
	#if defined(_WIN32) || defined(_WIN64)
		#include "compiler/dynamic/amd64/windows.hpp"
		#include "compiler/dynamic/amd64/windows.hpp"
	#elif defined(__linux__)
		#include "compiler/dynamic/amd64/linux.hpp"
		#include "compiler/static/amd64/linux.hpp"
		#error "Temporary unsupported platform"
	#elif defined(__APPLE__)
		#include "compiler/dynamic/amd64/macos.hpp"
		#include "compiler/static/amd64/macos.hpp"
		#error "Temporary unsupported platform"
	#else
		#error "Unsupported platform"
	#endif
#elif defined(_M_ARM64) || defined(__aarch64__)
	#include "compiler/dynamic/arm64/_any_os.hpp"
	#include "compiler/static/arm64/_any_os.hpp"
	#if defined(_WIN32) || defined(_WIN64)
		#include "compiler/dynamic/arm64/windows.hpp"
		#include "compiler/static/arm64/windows.hpp"
		#error "Temporary unsupported platform"
	#elif defined(__linux__)
		#include "compiler/dynamic/arm64/linux.hpp"
		#include "compiler/static/arm64/linux.hpp"
		#error "Temporary unsupported platform"
	#elif defined(__APPLE__)
		#include "compiler/dynamic/arm64/macos.hpp"
		#include "compiler/static/arm64/macos.hpp"
		#error "Temporary unsupported platform"
	#else
		#error "Unsupported platform"
	#endif
#else
#error "Unsupported architecture"
#endif

#include "compiler/dynamic/crossplatform.hpp"
#include "compiler/static/crossplatform.hpp"