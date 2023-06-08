// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/context/stack_traits.hpp>
#include <boost/context/stack_context.hpp>
#include "../attacha_abi.hpp"
struct light_stack {
    typedef boost::context::stack_traits traits_type;
    typedef boost::context::stack_context stack_context;

    light_stack(std::size_t size = traits_type::default_size()) BOOST_NOEXCEPT_OR_NOTHROW;
    stack_context allocate();
    void deallocate(stack_context& sctx);

    //returns true if the stack is shrinked
    static bool shrink_current(size_t bytes_treeshold = (1 << 12) * 3);

    //make sure 'bytes_to_use' avaible in stack, and increase stack without slow STATUS_GUARD_PAGE_VIOLATION
    static bool prepare(size_t bytes_to_use);
    //alloc all stack without slow STATUS_GUARD_PAGE_VIOLATION
    static bool prepare();

    //returns 
    // farr[
    //     farr[undefined_ptr stack_begin_from, undefined_ptr stack_end_at, ui64 used_memory, str desk, bool fault_prot]
    //     ...
    // ]
    //
    static ValueItem* dump_current();
    //default formated string
    static std::string dump_current_str();
    //console
    static void dump_current_out();

    //returns 
    // farr[
    //     farr[undefined_ptr stack_begin_from, undefined_ptr stack_end_at, ui64 used_memory, str desk, bool fault_prot]
    //     ...
    // ]
    //or
    //
    // false if invalid ptr
    static ValueItem* dump(void*);
    //default formated string
    static std::string dump_str(void*);
    //console
    static void dump_out(void*);


    static size_t used_size();
    static size_t unused_size();
    static size_t allocated_size();
    static size_t free_size();


    static bool is_supported();


    //set light_stack buffer, can non`t reduce
    static void set_buffer(size_t buffer_len);

    //set in stack bytes from buffer to 0xCCCC, by default false
    static bool flush_stack;
private:
    std::size_t size;
};