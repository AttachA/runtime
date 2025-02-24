// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

namespace art {
    //functions starting with symbol '#' is constructors
    //functions starting with symbol '>' default main function(name does not matter, but should be unique)
    //functions starting with symbol '\1' is special functions that used by compilers for special needs and should not called by user
    //L> full name recommended to be in this format: "\1 <language name> <version> <compiler> ..."
    //functions starting with symbol '\2' is special functions that used by compilers to initialize types and should not be initialized to runtime but called by compilers
    //L> and this functions should not call any user provided functions, but can call standard library, these functions intended to register vtables and maybe do any other actions to give il_compiler ability to optimize static calls to classes
    //functions starting with symbol '\3' is same special function which starts with '\2' but must be called once when readed and must be not called if appeared again
    //symbol '#' in names represent multiple constructors ex '# net ip#v6' is constructor for
    //L>   '# net ip' that receive only ip6 address in string, btw what contains after '#' is not important, that can be just numbers

    void initRuntime();
    void deinitRuntime();


    void initStandardLib();      //init all,except CMath and debug
    void initStandardLib_safe(); //init all,except CMath, internal(partially), debug and start_debug.  In internal will be initialized only limited vtable view without write access

    extern "C" void initCMathLib();
    void initStandardLib_exception();
    void initStandardLib_bytes();
    void initStandardLib_console();
    void initStandardLib_math();
    void initStandardLib_file();
    void initStandardLib_parallel();
    void initStandardLib_chanel();
    void initStandardLib_internal(bool vtable_full_mode = false, bool allow_self_build = false);
    void initStandardLib_internal_memory();
    void initStandardLib_internal_run_time();
    void initStandardLib_internal_run_time_native();
    void initStandardLib_internal_stack();
    void initStandardLib_net();
    void initStandardLib_localization();
    void initStandardLib_strings();
    void initStandardLib_times();
    void initStandardLib_debug();       //debug tools
    void initStandardLib_start_debug(); //allow enable debug tools

}
