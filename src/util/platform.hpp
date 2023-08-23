#ifndef SRC_UTIL_PLATFORM
#define SRC_UTIL_PLATFORM


//platforms: windows, linux, macos, ios, android, unknown
#if defined(_WIN32) || defined(_WIN64)
#define PLATFORM_WINDOWS 1
#elif defined(__linux__) || defined(__unix__) || defined(__posix__) || defined(__LINUX__) || defined(__linux) || defined(__gnu_linux__)
#define PLATFORM_LINUX 1
#elif defined(__APPLE__) || defined(__MACH__)
#define PLATFORM_MACOS 1
#elif defined(__ANDROID__) || defined(__ANDROID_API__) || defined(ANDROID)
#define PLATFORM_ANDROID 1
#elif defined(__IPHONE_OS_VERSION_MIN_REQUIRED) || defined(__IPHONE_OS_VERSION_MAX_ALLOWED) || defined(__IPHONE_OS_VERSION_MAX_REQUIRED) || defined(__IPHONE_OS_VERSION_MAX_ALLOWED)
#define PLATFORM_IOS 1
#else
#define PLATFORM_UNKNOWN
#endif


//architectures: amd64, aarch64, mips64
#if defined(_M_AMD64) || defined(__amd64__)
#define ARCHITECTURE_AMD64 1
#elif defined(_M_ARM64) || defined(__aarch64__)
#define ARCHITECTURE_AARCH64 1
#elif defined(_M_MIPS64) || defined(__mips64__)
#define ARCHITECTURE_MIPS64 1
#else
#define ARCHITECTURE_UNKNOWN 1
#endif


//compilers: msvc, gcc, unknown
#if defined(_MSC_VER)
#define COMPILER_MSVC 1
#elif defined(__GNUC__)
#define COMPILER_GCC 1
#else
#define COMPILER_UNKNOWN 1
#endif


//bit: 64, 32, unknown
#if defined(_WIN64) || defined(__LP64__) || defined(_LP64) || defined(__64BIT__) || defined(__x86_64__) || defined(__ppc64__) || defined(__powerpc64__) || defined(__mips64__) || defined(__alpha__) || defined(__ia64__) || defined(__s390__) || defined(__s390x__) || defined(__sparc64__) || defined(__arch64__) || defined(__amd64__) || defined(__aarch64__) || defined(__64__)
#define BIT_64 1
#elif defined(_WIN32) || defined(__LP32__) || defined(_LP32) || defined(__32BIT__) || defined(__x86__) || defined(__ppc__) || defined(__powerpc__) || defined(__mips__) || defined(__alpha__) || defined(__ia32__) || defined(__s390__) || defined(__sparc__) || defined(__arch32__) || defined(__amd32__) || defined(__aarch32__) || defined(__32__)
#define BIT_32 1
#else
#define BIT_UNKNOWN 1
#endif



#endif /* SRC_UTIL_PLATFORM */
