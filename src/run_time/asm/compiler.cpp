// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <util/platform.hpp>
#if ARCHITECTURE_AMD64
	#include <run_time/asm/compiler/dynamic/amd64/_any_os.hpp>
	#include <run_time/asm/compiler/static/amd64/_any_os.hpp>
	#if PLATFORM_WINDOWS
		#include <run_time/asm/compiler/dynamic/amd64/windows.hpp>
		#include <run_time/asm/compiler/dynamic/amd64/windows.hpp>
	#elif PLATFORM_LINUX
		#include <run_time/asm/compiler/dynamic/amd64/linux.hpp>
		#include <run_time/asm/compiler/static/amd64/linux.hpp>
	#elif PLATFORM_MACOS
		#include <run_time/asm/compiler/dynamic/amd64/macos.hpp>
		#include <run_time/asm/compiler/static/amd64/macos.hpp>
		#error "Temporary unsupported platform"
	#else
		#error "Unsupported platform"
	#endif
#elif ARCHITECTURE_AARCH64
	#include <run_time/asm/compiler/dynamic/arm64/_any_os.hpp>
	#include <run_time/asm/compiler/static/arm64/_any_os.hpp>
	#if PLATFORM_WINDOWS
		#include <run_time/asm/compiler/dynamic/arm64/windows.hpp>
		#include <run_time/asm/compiler/static/arm64/windows.hpp>
		#error "Temporary unsupported platform"
	#elif PLATFORM_LINUX
		#include <run_time/asm/compiler/dynamic/arm64/linux.hpp>
		#include <run_time/asm/compiler/static/arm64/linux.hpp>
		#error "Temporary unsupported platform"
	#elif PLATFORM_MACOS
		#include <run_time/asm/compiler/dynamic/arm64/macos.hpp>
		#include <run_time/asm/compiler/static/arm64/macos.hpp>
		#error "Temporary unsupported platform"
	#else
		#error "Unsupported platform"
	#endif
#else
#error "Unsupported architecture"
#endif

#include <run_time/asm/compiler/dynamic/crossplatform.hpp>
#include <run_time/asm/compiler/static/crossplatform.hpp>