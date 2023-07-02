// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef RUN_TIME_LIBRARY_INTERNAL
#define RUN_TIME_LIBRARY_INTERNAL
#include "../attacha_abi_structs.hpp"



namespace internal {

    //not thread safe!
    namespace memory{
        //returns farr[farr[ptr from, ptr to, len, str desk, bool is_fault]...], args: array/value ptr
        ValueItem* dump(ValueItem*, uint32_t);
    }

    //not thread safe!
    namespace stack {
        //reduce stack size, returns bool, args: shrink treeshold(optional)
        ValueItem* shrink(ValueItem*, uint32_t);
        //grow stack size, returns bool, args: grow count
        ValueItem* prepare(ValueItem*, uint32_t);
        //make sure stack size is enough and increase if too small, returns bool, args: grow count
        ValueItem* reserve(ValueItem*, uint32_t);


        //returns farr[farr[ptr from, ptr to, str desk, bool is_fault]...], args: none
        ValueItem* dump(ValueItem*, uint32_t);

        //better stack is supported?
        ValueItem* bs_supported(ValueItem*, uint32_t);
        //better stack is os depended implementation, for example, on windows supported because that use guard pages for auto grow stack, linix may be support that too but not sure

        //in windows we can deallocate unused stacks and set guard page to let windows auto increase stack size
        //also we can manually increase stack size by manually allocating pages and set guard page to another position
        // that allow reduce memory usage and increase application performance

        //in linux stack can be increased automatically by mmap(null, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_GROWSDOWN, -1, 0)
        //but we may be cannont deallocate unused stacks

        ValueItem* used_size(ValueItem*, uint32_t);
        ValueItem* unused_size(ValueItem*, uint32_t);
        ValueItem* allocated_size(ValueItem*, uint32_t);
        ValueItem* free_size(ValueItem*, uint32_t);



        //returns [{file_path, fun_name, line},...], args: framesToSkip, include_native, max_frames
        ValueItem* trace(ValueItem*, uint32_t);
        //returns [rip,...], args: framesToSkip, include_native, max_frames
        ValueItem* trace_frames(ValueItem*, uint32_t);
        //returns {file_path, fun_name, line}, args: frame,(optional include_native)
        ValueItem* resolve_frame(ValueItem*, uint32_t);
    }

    namespace run_time{
        //not recomended to use, use only for debug
        ValueItem* gc_pause(ValueItem*, uint32_t);
        ValueItem* gc_resume(ValueItem*, uint32_t);

        //gc can ignore this hint
        ValueItem* gc_hinit_collect(ValueItem*, uint32_t);

        namespace native{
            namespace construct{
                ValueItem* createProxy_NativeValue(ValueItem*, uint32_t);// used in NativeTemplate
                ValueItem* createProxy_NativeTemplate(ValueItem*, uint32_t);// used in NativeLib
                ValueItem* createProxy_NativeLib(ValueItem*, uint32_t);// args: str lib path(resolved by os), do not use functions from this instance when destructor called
            }
            void init();
        }
    }
    namespace construct{
        ValueItem* createProxy_function_builder(ValueItem*, uint32_t);
        ValueItem* createProxy_index_pos(ValueItem*, uint32_t);
        ValueItem* createProxy_line_info(ValueItem*, uint32_t);
    }
    void init();
}

#endif /* RUN_TIME_LIBRARY_INTERNAL */
