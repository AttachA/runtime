#include <boost/context/stack_traits.hpp>
#include <boost/context/stack_context.hpp>
#include "../attacha_abi.hpp"
struct light_stack {
    typedef boost::context::stack_traits traits_type;
    typedef boost::context::stack_context stack_context;

    light_stack(std::size_t size = traits_type::default_size()) BOOST_NOEXCEPT_OR_NOTHROW;
    stack_context allocate();
    void deallocate(stack_context& sctx);


    static size_t shrink_current(size_t bytes_treeshold = (1 << 12) * 3);
    //try increase stack to use them without slow page_faults
    static size_t prepare(size_t bytes_to_use);

    //returns 
    // farr[
    //     farr[undefined_ptr stack_begin_from, undefined_ptr stack_end_at, ui64 used_memory]
    //     farr[undefined_ptr from, undefined_ptr to, str desk, bool fault_prot]
    //     ...
    // ]
    static ValueItem* dump_current();
    //default formated string
    static std::string dump_current_str();
    //console
    static void dump_current_out();

    //returns 
    // farr[
    //     farr[undefined_ptr stack_begin_from, undefined_ptr stack_end_at, ui64 used_memory]
    //     farr[undefined_ptr from, undefined_ptr to, str desk, bool fault_prot]
    //     ...
    // ]
    static ValueItem* dump(void*);
    //default formated string
    static std::string dump_str(void*);
    //console
    static void dump_out(void*);


    static size_t current_can_be_used();
    static size_t current_size();
    static size_t full_current_size();
    static bool is_supported();
private:
    std::size_t size;
};