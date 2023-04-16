# Attach A
![](https://tokei.rs/b1/github/GParcade/AttachA)

This project implements a small virtual machine compared to the known JVM/NET, and also implements a huge number of features that are not available to these virtual machines


# Example:
- mutexes, condition_variables, e.t.c. which can be used both in normal threads and in tasks,(parallel ...)
- Awaitable threads,(parallel create_async_thread)
- Full stack size control,(internal stack ...)
- Correct stack unwinding after stack overflow,(optional spare space for unwinding in guard page)


# For what?
Basically, this project was made for another project, because no C++ implementation allows hot reloading, and the one that does not work with C++, so I decided to make my own VM.

Since this is a small VM implementation and unlike .NET/JVM does not do any runtime optimizations and delegates this optimization responsibility to compilers in exchange for more freedom providing a reimagined implementation of asynchrony, file operations, networking, hot reloading, and the ability to combine static and dynamic typing.

# Notes for compiling project
- This project uses [Boost.context](www.boost.org) and [asmjit](https://github.com/asmjit/asmjit) libs, compile these projects and link before compiling
- curenly supported only x64 Windows