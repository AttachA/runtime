// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef SRC_RUN_TIME_TASKS_UTIL_LIGHT_STACK
#define SRC_RUN_TIME_TASKS_UTIL_LIGHT_STACK

#include <boost/context/fiber.hpp>
#include <boost/context/stack_context.hpp>
#include <boost/context/stack_traits.hpp>
#include <run_time/attacha_abi.hpp>

namespace art {
    struct light_stack {
        typedef boost::context::stack_traits traits_type;
        typedef boost::context::stack_context stack_context;

        light_stack(std::size_t size = traits_type::default_size()) BOOST_NOEXCEPT_OR_NOTHROW;
        stack_context allocate();
        void deallocate(stack_context& sctx);

        //returns true if the stack is shrunk
        static bool shrink_current(size_t bytes_threshold = (1 << 12) * 3);

        //make sure 'bytes_to_use' available in stack, and increase stack without slow STATUS_GUARD_PAGE_VIOLATION
        static bool prepare(size_t bytes_to_use);
        //alloc all stack without slow STATUS_GUARD_PAGE_VIOLATION
        static bool prepare();

        //returns
        // faarr[
        //     faarr[undefined_ptr stack_begin_from, undefined_ptr stack_end_at, ui64 used_memory, str desk, bool fault_prot]
        //     ...
        // ]
        //
        static ValueItem* dump_current();
        //default formatted string
        static art::ustring dump_current_str();
        //console
        static void dump_current_out();

        //returns
        // faarr[
        //     faarr[undefined_ptr stack_begin_from, undefined_ptr stack_end_at, ui64 used_memory, str desk, bool fault_prot]
        //     ...
        // ]
        //or
        //
        // false if invalid ptr
        static ValueItem* dump(void*);
        //default formatted string
        static art::ustring dump_str(void*);
        //console
        static void dump_out(void*);


        static size_t used_size();
        static size_t unused_size();
        static size_t allocated_size();
        static size_t free_size();


        static bool is_supported();

        //set in stack bytes from buffer to 0xCCCC, by default false
        static bool flush_used_stacks;
        static size_t max_buffer_size;

    private:
        std::size_t size;
    };
}


#endif /* SRC_RUN_TIME_TASKS_UTIL_LIGHT_STACK */