# Attach A
![](https://tokei.rs/b1/github/GParcade/AttachA)
This project implement tiny VM like .Net or JVM.

# For what?
Basically, this project was made for another project, because no C++ implementation allows hot reloading, and the one that does not work with C++, so I decided to make my own VM.

Since this is a small VM implementation and unlike .NET/JVM does not do any runtime optimizations and delegates this optimization responsibility to compilers in exchange for more freedom providing a reimagined implementation of asynchrony, file operations, networking, hot reloading, and the ability to combine static and dynamic typing.

# Notes for compiling project
- This project uses [Boost.context](www.boost.org) and [asmjit](https://github.com/asmjit/asmjit) libs, compile these projects and link before compiling
- curenly supported only x64 Windows