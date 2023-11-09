# Attach A

![](https://tokei.rs/b1/github/AttachA/runtime?category=code&type=Cpp,Assembly)

This project implements a small virtual machine compared to the known JVM/NET, and also implements a huge number of features that are not available to these virtual machines


## Example:
- mutexes, condition_variables, e.t.c. which can be used both in normal threads and in tasks,(parallel ...)
- Awaitable threads,(parallel create_async_thread)
- Full stack size control,(internal stack ...)
- Correct stack unwinding after stack overflow,(optional spare space for unwinding in guard page)


## For what?
Basically, this project was made for another project, because no C++ implementation allows hot reloading, and the one that does not work with C++, so I decided to make my own VM.

Since this is a small VM implementation and unlike .NET/JVM does not do any runtime optimizations and delegates this optimization responsibility to compilers in exchange for more freedom providing a reimagined implementation of asynchronous, file operations, networking, hot reloading, and the ability to combine static and dynamic typing.

## Notes for compiling project
- Use cmake for compiling, in visual studio use vcpkg toolchain for cmake, in visual studio code everything is already configured, just run
- If you wanna port to another platform use this libraries for compilation: [boost-context](https://www.boost.org), [boost-preprocessor](https://www.boost.org), [boost-lockfree](https://www.boost.org), [boost-uuid](https://www.boost.org), [asmjit](https://github.com/asmjit/asmjit) and [utf-cpp](https://github.com/nemtrif/utfcpp)
- currently supported only x64 Windows and partially x64 Linux

## SAST Tools
[PVS-Studio](https://pvs-studio.com/pvs-studio/?utm_source=website&utm_medium=github&utm_campaign=open_source) - static analyzer for C, C++, C#, and Java code.

[![PVS-Studio](https://cdn.pvs-studio.com/static/favicon.ico)](https://pvs-studio.com/pvs-studio/?utm_source=website&utm_medium=github&utm_campaign=open_source)

