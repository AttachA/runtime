#include "Windows.h"
#include <intrin.h>
#include "light_stack.hpp"
#include <boost/assert.hpp>
#include <cmath>
#include <exception>
#include "../../run_time.hpp"
#include <iostream>
#include <assert.h>
#include <exception>
typedef boost::context::stack_context stack_context;


BOOST_NOINLINE PBYTE get_current_stack_pointer() {
    return (PBYTE)_AddressOfReturnAddress() + 8;
}
bool set_lock_pages_priv( HANDLE hProcess, BOOL bEnable ) {
    struct {
        DWORD Count;
        LUID_AND_ATTRIBUTES Privilege[1];
    } Info;
    struct TokenCloser {
        HANDLE Token;
        TokenCloser(HANDLE Token) : Token(Token) {}
        ~TokenCloser() { CloseHandle(Token); }
        operator HANDLE&() { return Token; }
    } Token(nullptr);

    if (!OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, &(HANDLE&)Token))
        return false;


    Info.Count = 1;
    if (bEnable) 
        Info.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;
    else 
        Info.Privilege[0].Attributes = 0;
    

    if(!LookupPrivilegeValue( NULL, SE_LOCK_MEMORY_NAME, &(Info.Privilege[0].Luid)))
        return false;

    if(!AdjustTokenPrivileges(Token, false, (PTOKEN_PRIVILEGES)&Info, 0, nullptr, nullptr))
        return false;
    else if( GetLastError() != ERROR_SUCCESS )
            return false;
    return true;
}


light_stack::light_stack(std::size_t size) BOOST_NOEXCEPT_OR_NOTHROW : size(size) {}

stack_context light_stack::allocate() {
    // calculate how many pages are required
    const size_t guard_page_size = (size_t(fault_reserved_pages) + 1) * traits_type::page_size();
    const std::size_t pages = (size + guard_page_size + traits_type::page_size() - 1) / traits_type::page_size();
    // add one page at bottom that will be used as guard-page
    const std::size_t size__ = (pages + 1) * traits_type::page_size();

    void* vp = ::VirtualAlloc(0, size__, MEM_RESERVE, PAGE_READWRITE);
    if (!vp) 
        throw std::bad_alloc();

    // needs at least 3 pages to fully construct the coroutine and switch to it
    const auto init_commit_size = traits_type::page_size() * 3;
    auto pPtr = static_cast<PBYTE>(vp) + size__;
    pPtr -= init_commit_size;
    if (!VirtualAlloc(pPtr, init_commit_size, MEM_COMMIT, PAGE_READWRITE)) 
        throw std::bad_alloc();

    // create guard page so the OS can catch page faults and grow our stack
    pPtr -= guard_page_size;
    if (!VirtualAlloc(pPtr, guard_page_size, MEM_COMMIT, PAGE_READWRITE | PAGE_GUARD))
        throw std::bad_alloc();
    stack_context sctx;
    sctx.size = size__;
    sctx.sp = static_cast<char*>(vp) + sctx.size;
    return sctx;
}



void light_stack::deallocate(stack_context& sctx ) {
    BOOST_ASSERT(sctx.sp);
    ::VirtualFree(static_cast<char*>(sctx.sp) - sctx.size, 0, MEM_RELEASE);
}



template< typename T >
struct hex_fmt_t {
    hex_fmt_t(T x) : val(x) {}
    friend std::ostream& operator<<(std::ostream& os, const hex_fmt_t& v) {
        return os << std::hex
            << std::internal
            << std::showbase
            << std::setw(8)
            << std::setfill('0')
            << v.val;
    }
    T val;
};

template< typename T >
hex_fmt_t<T> fmt_hex(T x) {
    return hex_fmt_t<T>{ x };
}

const char* fmt_state(DWORD state) {
    switch (state) {
    case MEM_COMMIT: return "COMMIT ";
    case MEM_FREE: return "FREE   ";
    case MEM_RESERVE: return "RESERVE";
    default: return "unknown";
    }
}

std::string fmt_protect(DWORD prot) {
    std::string res;
    if (prot & PAGE_NOACCESS)
        res += "NOACCESS";
    if (prot & PAGE_READONLY)
        res += res.size() ? " & READONLY" : "READONLY";
    if (prot & PAGE_READWRITE)
        res += res.size() ? " & READWRITE" : "READWRITE";
    if (prot & PAGE_WRITECOPY)
        res += res.size() ? " & WRITECOPY" : "WRITECOPY";
    if (prot & PAGE_EXECUTE)
        res += res.size() ? " & EXECUTE" : "EXECUTE";
    if (prot & PAGE_EXECUTE_READ)
        res += res.size() ? " & EXECUTE_READ" : "EXECUTE_READ";
    if (prot & PAGE_EXECUTE_READWRITE)
        res += res.size() ? " & EXECUTE_READWRITE" : "EXECUTE_READWRITE";
    if (prot & PAGE_EXECUTE_WRITECOPY)
        res += res.size() ? " & EXECUTE_WRITECOPY" : "EXECUTE_WRITECOPY";
    if (prot & PAGE_GUARD)
        res += res.size() ? " & GUARD" : "GUARD";
    if (prot & PAGE_NOCACHE)
        res += res.size() ? " & NOCACHE" : "NOCACHE";
    if (prot & PAGE_WRITECOMBINE)
        res += res.size() ? " & WRITECOMBINE" : "WRITECOMBINE";

    if (!res.size())
        return "ZERO";
    return res;
}


void dump_stack(bool* pPtr) {
    std::cout << "####### Stack Dump Start #######\n";

    const auto page_size = boost::context::stack_traits::page_size();

    // Get the stack last page.
    MEMORY_BASIC_INFORMATION stMemBasicInfo;
    BOOST_VERIFY(VirtualQuery(pPtr, &stMemBasicInfo, sizeof(stMemBasicInfo)));
    bool* pPos = (bool*)stMemBasicInfo.AllocationBase;
    do {
        BOOST_VERIFY(VirtualQuery(pPos, &stMemBasicInfo, sizeof(stMemBasicInfo)));
        BOOST_VERIFY(stMemBasicInfo.RegionSize);

        std::cout << "Range: " << fmt_hex((SIZE_T)pPos)
            << " - " << fmt_hex((SIZE_T)pPos + stMemBasicInfo.RegionSize)
            << " Protect: " << fmt_protect(stMemBasicInfo.Protect)
            << " State: " << fmt_state(stMemBasicInfo.State)
            //<< " Type: " << fmt_type( stMemBasicInfo.Type )
            << std::dec
            << " Pages: " << stMemBasicInfo.RegionSize / page_size
            << std::endl;

        pPos += stMemBasicInfo.RegionSize;
    } while (pPos < pPtr);
    std::cout << "####### Stack Dump Finish #######" << std::endl;
}


BOOST_NOINLINE bool* GetStackPointer() {
    return (bool*)_AddressOfReturnAddress() + 8;
}
void dump_stack() {
    bool* pPtr = GetStackPointer();
    dump_stack(pPtr);
}

size_t light_stack::shrink_current(size_t bytes_treeshold){return 0;}
    //try increase stack to use them without slow page_faults
    size_t light_stack::prepare(size_t bytes_to_use){return 0;}

    //returns 
    // farr[
    //     farr[undefined_ptr stack_begin_from, undefined_ptr stack_end_at, ui64 used_memory]
    //     farr[undefined_ptr from, undefined_ptr to, str desk, bool fault_prot]
    //     ...
    // ]
    ValueItem* light_stack::dump_current(){return 0;}
    //default formated string
    std::string light_stack::dump_current_str(){return "";}
    //console
    void light_stack::dump_current_out(){
        dump_stack();
    }

    //returns 
    // farr[
    //     farr[undefined_ptr stack_begin_from, undefined_ptr stack_end_at, ui64 used_memory]
    //     farr[undefined_ptr from, undefined_ptr to, str desk, bool fault_prot]
    //     ...
    // ]
    ValueItem* light_stack::dump(void*){return 0;}
    //default formated string
    std::string light_stack::dump_str(void*){return "";}
    //console
    void light_stack::dump_out(void* ptr){
        dump_stack((bool*)ptr);
    }


    size_t light_stack::current_can_be_used(){return 0;}
    size_t light_stack::current_size(){return 0;}
    size_t light_stack::full_current_size(){return 0;}
    bool light_stack::is_supported(){return 0;}