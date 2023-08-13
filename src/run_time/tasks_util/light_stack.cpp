// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include "light_stack.hpp"
#include "../../run_time.hpp"
#include "../library/console.hpp"
#include "../exceptions.hpp"
#include "../../../configuration/tasks.hpp"
#include <cassert>
#include <boost/lockfree/queue.hpp>
#include <atomic>
#include <vector>
namespace art{
    typedef boost::context::stack_context stack_context;

    boost::lockfree::queue<light_stack::stack_context> stack_allocations(configuration::tasks::light_stack::inital_buffer_size);
    std::atomic_size_t stack_allocations_buffer = configuration::tasks::light_stack::inital_buffer_size;
    bool light_stack::flush_used_stacks = configuration::tasks::light_stack::flush_used_stacks;
    size_t light_stack::max_buffer_size = configuration::tasks::light_stack::max_buffer_size;
}
#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#include <intrin.h>//_AddressOfReturnAddress
namespace art{
    stack_context create_stack(size_t size){
        // calculate how many pages are required
        const size_t guard_page_size = (size_t(fault_reserved_pages) + 1) * page_size;

        void* vp = ::VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
        if (!vp) 
            throw AllocationException("VirtualAlloc failed");

        // needs at least 3 pages to fully construct the coroutine and switch to it
        const auto init_commit_size = page_size * 3;
        auto pPtr = static_cast<PBYTE>(vp) + size;
        pPtr -= init_commit_size;
        if (!VirtualAlloc(pPtr, init_commit_size, MEM_COMMIT, PAGE_READWRITE)) 
            throw AllocationException("VirtualAlloc failed");

        // create guard page so the OS can catch page faults and grow our stack
        pPtr -= guard_page_size;
        if (!VirtualAlloc(pPtr, guard_page_size, MEM_COMMIT, PAGE_READWRITE | PAGE_GUARD))
            throw AllocationException("VirtualAlloc failed");
        stack_context sctx;
        sctx.size = size;
        sctx.sp = static_cast<char*>(vp) + sctx.size;
        return sctx;
    }


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

    size_t large_page_size = GetLargePageMinimum();

    light_stack::light_stack(size_t size) BOOST_NOEXCEPT_OR_NOTHROW : size(size) {}

    stack_context light_stack::allocate() {
        const size_t guard_page_size = (size_t(fault_reserved_pages) + 1) * page_size;
        const size_t pages = (size + guard_page_size + page_size - 1) / page_size;
        // add one page at bottom that will be used as guard-page
        const size_t size__ = (pages + 1) * page_size;

        stack_context result;
        if (stack_allocations.pop(result)) {
            stack_allocations_buffer--;
            if(!flush_used_stacks)
                return result;
            else {
                memset(static_cast<char*>(result.sp) - result.size, 0xCC, result.size);
                return result;
            }
        }
        else
            return create_stack(size__);
    }

    void unlimited_buffer(stack_context& sctx ){
        if (!stack_allocations.push(sctx)) 
            ::VirtualFree(static_cast<char*>(sctx.sp) - sctx.size, 0, MEM_RELEASE);
        else
            stack_allocations_buffer++;
    }
    void limited_buffer(stack_context& sctx ){
    if (++stack_allocations_buffer < light_stack::max_buffer_size) {
            if (!stack_allocations.push(sctx)) {
                ::VirtualFree(static_cast<char*>(sctx.sp) - sctx.size, 0, MEM_RELEASE);
                stack_allocations_buffer--;
            }
        }
        else {
            ::VirtualFree(static_cast<char*>(sctx.sp) - sctx.size, 0, MEM_RELEASE);
            stack_allocations_buffer--;
        }
    }

    void light_stack::deallocate(stack_context& sctx ) {
        assert(sctx.sp);
        if(!max_buffer_size)
            unlimited_buffer(sctx);
        else if(max_buffer_size != SIZE_MAX)
            limited_buffer(sctx);
        else
            ::VirtualFree(static_cast<char*>(sctx.sp) - sctx.size, 0, MEM_RELEASE);
    }


    const char* fmt_state(DWORD state) {
        switch (state) {
        case MEM_COMMIT: return "COMMIT";
        case MEM_FREE: return "FREE";
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


    std::string dump_stack(bool* pPtr) {
        std::string res;
        // Get the stack last page.
        MEMORY_BASIC_INFORMATION stMemBasicInfo;
        BOOST_VERIFY(VirtualQuery(pPtr, &stMemBasicInfo, sizeof(stMemBasicInfo)));
        bool* pPos = (bool*)stMemBasicInfo.AllocationBase;
        do {
            BOOST_VERIFY(VirtualQuery(pPos, &stMemBasicInfo, sizeof(stMemBasicInfo)));
            BOOST_VERIFY(stMemBasicInfo.RegionSize);
            res += "Range: " + string_help::hexstr((SIZE_T)pPos);
            res += " - " + string_help::hexstr((SIZE_T)pPos + stMemBasicInfo.RegionSize);
            res += " Protect: " + fmt_protect(stMemBasicInfo.Protect);
            res += " State: "; res += fmt_state(stMemBasicInfo.State);
            //res += " Type: " + fmt_type(stMemBasicInfo.Type);
            res += " Pages: " + std::to_string(stMemBasicInfo.RegionSize / page_size);
            res += "\n";
            pPos += stMemBasicInfo.RegionSize;
        } while (pPos < pPtr);
        return res;
    }



    struct allocation_details{
        DWORD prot;
        DWORD state;
        size_t length;
        bool* base;
        bool* page;
        allocation_details(size_t page_ptr, size_t base_ptr, size_t len, DWORD prot, DWORD state) : page((bool*)page_ptr),base((bool*)page_ptr), length(len), state(state), prot(prot)  {}
        allocation_details(const MEMORY_BASIC_INFORMATION& mbi) : allocation_details((size_t)mbi.BaseAddress,(size_t)mbi.AllocationBase, mbi.RegionSize,mbi.Protect, mbi.State)  {}
    };

    BOOST_NOINLINE bool* GetStackPointer() {
        return (bool*)_AddressOfReturnAddress() + 8;
    }

    struct stack_snapshot {
        stack_snapshot(bool* pPtr) {
            // Get the stack last page.
            MEMORY_BASIC_INFORMATION stMemBasicInfo;
            if(!(VirtualQuery(pPtr, &stMemBasicInfo, sizeof(stMemBasicInfo)))){
                valid = false;
                return;
            }
            bool* pPos = (bool*)stMemBasicInfo.AllocationBase;
            allocations.reserve(3);
            do {
                BOOST_VERIFY(VirtualQuery(pPos, &stMemBasicInfo, sizeof(stMemBasicInfo)));
                BOOST_VERIFY(stMemBasicInfo.RegionSize);
                allocations.push_back(allocation_details(stMemBasicInfo));
                pPos += stMemBasicInfo.RegionSize;
            } while (pPos < pPtr);
            allocations.shrink_to_fit();
            for (auto& a : allocations) {
                if (a.state == MEM_RESERVE) {
                    reserved = &a;
                }
                else if (a.prot & PAGE_GUARD) {
                    guard = &a;
                }
                else if (a.state == MEM_COMMIT) {
                    committed = &a;
                }
            }

        }
        stack_snapshot() : stack_snapshot(GetStackPointer()) {}
        std::vector<allocation_details> allocations;
        allocation_details* reserved = nullptr;
        allocation_details* guard = nullptr;
        allocation_details* committed = nullptr;
        bool valid = true;
    };

    enum stack_item{
        reserved,
        guard,
        committed
    };
    template<stack_item item>
    allocation_details get_stack_detail(bool* pPtr){
        MEMORY_BASIC_INFORMATION stMemBasicInfo;
        BOOST_VERIFY(VirtualQuery(GetStackPointer(), &stMemBasicInfo, sizeof(stMemBasicInfo)));
        bool* start = (bool*)stMemBasicInfo.AllocationBase;
        BOOST_VERIFY(VirtualQuery(start, &stMemBasicInfo, sizeof( stMemBasicInfo )));//begin of allocation// usually MEM reserve
        start += stMemBasicInfo.RegionSize;
        if constexpr(item == stack_item::reserved){
            if(stMemBasicInfo.State == MEM_RESERVE)
                return stMemBasicInfo;
        } else {
            if(stMemBasicInfo.State == MEM_RESERVE){
                BOOST_VERIFY(VirtualQuery((bool*)start, &stMemBasicInfo, sizeof(stMemBasicInfo)));//after reserve usually guard page
                start += stMemBasicInfo.RegionSize;
            }

            if constexpr(item == stack_item::guard){
                if(stMemBasicInfo.Protect & PAGE_GUARD)
                    return stMemBasicInfo;
            } else {
                if(stMemBasicInfo.Protect & PAGE_GUARD){
                    BOOST_VERIFY(VirtualQuery((bool*)start, &stMemBasicInfo, sizeof(stMemBasicInfo)));//after guard page usually MEM commit
                    start += stMemBasicInfo.RegionSize;
                }
                
                if(stMemBasicInfo.State == MEM_COMMIT)
                    return stMemBasicInfo;
            }
        }
        return allocation_details(0,0,0,0,0);
    }
    std::string dump_stack() {
        return dump_stack(GetStackPointer());
    }

    //shit code, TO-DO fix it
    bool light_stack::shrink_current(size_t bytes_threshold){
        bool* pPtr = GetStackPointer();
        stack_snapshot snapshot(pPtr);
        
        size_t unused = snapshot.committed->length - size_t((snapshot.committed->base + snapshot.committed->length) - pPtr);
        if(unused <= bytes_threshold)
            return false;
        size_t to_free = (unused - bytes_threshold) / page_size * page_size;
        if(!to_free)
            return false;

        size_t guard_page_size = snapshot.guard ? snapshot.guard->length : 0;
        if(to_free < guard_page_size) {
            DWORD old_options;
            BOOST_VERIFY(VirtualProtect(snapshot.guard->base - to_free, guard_page_size, PAGE_GUARD, &old_options));
            BOOST_VERIFY(VirtualFree(snapshot.guard->base, to_free, MEM_DECOMMIT));
        }else if(guard_page_size){
            if(!VirtualFree(snapshot.guard->base, to_free + guard_page_size, MEM_DECOMMIT))
                return false;
            VirtualAlloc(snapshot.guard->base + to_free, guard_page_size, MEM_COMMIT, PAGE_READWRITE | PAGE_GUARD);
        }else{
            if(!VirtualFree(snapshot.committed->base + snapshot.committed->length, to_free, MEM_DECOMMIT))
                return false;
        }
        return true;
    }

    bool light_stack::prepare(){
        bool* pPtr = GetStackPointer(); 
        MEMORY_BASIC_INFORMATION stMemBasicInfo;
        BOOST_VERIFY(VirtualQuery(pPtr, &stMemBasicInfo, sizeof(stMemBasicInfo)));
        bool* start = (bool*)stMemBasicInfo.AllocationBase;
        bool* guard = nullptr;
        size_t guard_page_size = 0;

        
        BOOST_VERIFY(VirtualQuery(start, &stMemBasicInfo, sizeof(stMemBasicInfo)));
        BOOST_VERIFY(stMemBasicInfo.RegionSize);
        
        if(stMemBasicInfo.State != MEM_RESERVE || stMemBasicInfo.Protect & PAGE_GUARD)
            return false;

        size_t free_space = stMemBasicInfo.RegionSize;
        
        BOOST_VERIFY(VirtualQuery(start + free_space, &stMemBasicInfo, sizeof(stMemBasicInfo)));
        BOOST_VERIFY(stMemBasicInfo.RegionSize);

        if(stMemBasicInfo.Protect & PAGE_GUARD){
            guard = start + free_space;
            guard_page_size = stMemBasicInfo.RegionSize;
        }

        if(guard){
            //BOOST_VERIFY(VirtualFree(guard, guard_page_size, MEM_DECOMMIT ));
            DWORD old_options;
            BOOST_VERIFY(VirtualProtect(guard, guard_page_size, PAGE_READWRITE, &old_options));
            size_t to_alloc = free_space - guard_page_size;
            while(to_alloc){
                if(!VirtualAlloc(guard - to_alloc, page_size , MEM_COMMIT, PAGE_READWRITE))
                    return false;
                to_alloc -= page_size;
            }
            BOOST_VERIFY(VirtualAlloc(start, guard_page_size, MEM_COMMIT, PAGE_READWRITE | PAGE_GUARD));
        }
        else{
            VirtualAlloc(start, free_space, MEM_COMMIT, PAGE_READWRITE);
        }
        return true;
    }

    bool light_stack::prepare(size_t bytes_to_use){
        bool* pPtr = GetStackPointer(); 
        MEMORY_BASIC_INFORMATION stMemBasicInfo;
        BOOST_VERIFY(VirtualQuery(pPtr, &stMemBasicInfo, sizeof(stMemBasicInfo)));
        bool* start = (bool*)stMemBasicInfo.AllocationBase;
        bool* guard = nullptr;
        size_t guard_page_size = 0;

        
        BOOST_VERIFY(VirtualQuery(start, &stMemBasicInfo, sizeof(stMemBasicInfo)));
        BOOST_VERIFY(stMemBasicInfo.RegionSize);

        size_t free_space;

        if(stMemBasicInfo.State != MEM_RESERVE || stMemBasicInfo.Protect & PAGE_GUARD)
            free_space = 0;
        else
            free_space = stMemBasicInfo.RegionSize;


        BOOST_VERIFY(VirtualQuery(start + free_space, &stMemBasicInfo, sizeof(stMemBasicInfo)));
        BOOST_VERIFY(stMemBasicInfo.RegionSize);


        size_t used = (size_t)pPtr;
        if(stMemBasicInfo.Protect & PAGE_GUARD){
            guard = start + free_space;
            guard_page_size = stMemBasicInfo.RegionSize;
            used -= (size_t)guard + guard_page_size;
        }
        else
            used -= (size_t)start + free_space;
        
        if(used > bytes_to_use)
            return true;
        else
            bytes_to_use -= used;

        if(free_space < bytes_to_use)
            return false;

        bytes_to_use = (bytes_to_use + page_size - 1) & ~(page_size - 1);


        if(guard){
            //this will work, but no, im not know why
            //BOOST_VERIFY(VirtualFree(guard, guard_page_size, MEM_DECOMMIT ));
            //BOOST_VERIFY(VirtualAlloc(guard, bytes_to_use, MEM_COMMIT, PAGE_READWRITE));
            //BOOST_VERIFY(VirtualAlloc(guard - bytes_to_use, guard_page_size, MEM_COMMIT, PAGE_READWRITE | PAGE_GUARD));

            //then im made this shit:
            DWORD old_options;
            BOOST_VERIFY(VirtualProtect(guard, guard_page_size, PAGE_READWRITE, &old_options));
            
            size_t to_alloc = bytes_to_use - guard_page_size;
            while(to_alloc){
                if(!VirtualAlloc(guard - to_alloc, page_size , MEM_COMMIT, PAGE_READWRITE))
                    return false;
                to_alloc -= page_size;
            }
            BOOST_VERIFY(VirtualAlloc(guard - bytes_to_use, guard_page_size, MEM_COMMIT, PAGE_READWRITE | PAGE_GUARD));
        }
        else
            VirtualAlloc(start + free_space, bytes_to_use, MEM_COMMIT, PAGE_READWRITE);
        return true;
    }


    ValueItem* light_stack::dump_current(){
        return dump(GetStackPointer());
    }
    std::string light_stack::dump_current_str(){
        return dump_stack();
    }
    void light_stack::dump_current_out(){
        ValueItem item("####### Stack Dump Start #######\n" + dump_stack() + "####### Stack Dump End #######\n");
        console::print(&item, 1);
    }

    ValueItem* light_stack::dump(void* ptr){
        list_array<ValueItem> stack;
        stack_snapshot snap((bool*)ptr);
        for(auto& a : snap.allocations){
            list_array<ValueItem> item;
            item.push_back(new ValueItem(a.base));
            item.push_back(new ValueItem(a.base + a.length));
            item.push_back(new ValueItem(a.length));
            item.push_back(new ValueItem(fmt_protect(a.prot) + " " + fmt_state(a.state)));
            item.push_back(new ValueItem(bool(a.prot & PAGE_GUARD)));
            stack.push_back(new ValueItem(std::move(item)));
        }
        return new ValueItem(std::move(stack));
    }
    //default formatted string
    std::string light_stack::dump_str(void*){
        return "";
    }
    //console
    void light_stack::dump_out(void* ptr){
        ValueItem item("####### Stack Dump Start #######\n" + dump_stack((bool*)ptr) + "####### Stack Dump End #######\n");
        console::print(&item, 1);
    }
    size_t light_stack::allocated_size(){
        return get_stack_detail<stack_item::committed>(GetStackPointer()).length;
    }
    size_t light_stack::free_size(){
        return get_stack_detail<stack_item::reserved>(GetStackPointer()).length;
    }
    size_t light_stack::used_size(){
        bool* pPtr = GetStackPointer();
        auto detail = get_stack_detail<stack_item::committed>(pPtr);
        return size_t((detail.base + detail.length) - pPtr);
    }
    size_t light_stack::unused_size(){
        bool* pPtr = GetStackPointer();
        auto detail = get_stack_detail<stack_item::committed>(pPtr);
        return detail.length - size_t((detail.base + detail.length) - pPtr);
    }

    bool light_stack::is_supported(){return true;}
}
#else if defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
namespace art{
    stack_context create_stack(size_t size){
        // calculate how many pages are required
        const size_t guard_page_size = (size_t(fault_reserved_pages) + 1) * page_size;

        void* vp = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (!vp) 
            throw AllocationException("VirtualAlloc failed");

        // needs at least 3 pages to fully construct the coroutine and switch to it
        const auto init_commit_size = page_size * 3;
        auto pPtr = static_cast<uint8_t*>(vp) + size;
        pPtr -= init_commit_size;
        if (mprotect(pPtr, init_commit_size, PROT_READ | PROT_WRITE) == -1) 
            throw AllocationException("VirtualAlloc failed");

        //PROT_NONE already used for guard page
        stack_context sctx;
        sctx.size = size;
        sctx.sp = static_cast<char*>(vp) + sctx.size;
        return sctx;
    }


    BOOST_NOINLINE uint8_t* get_current_stack_pointer() {
        return (uint8_t*)__builtin_return_address(0) + 8;
    }
   
    size_t large_page_size = 0;//GetLargePageMinimum();

    light_stack::light_stack(size_t size) BOOST_NOEXCEPT_OR_NOTHROW : size(size) {}

    stack_context light_stack::allocate() {
        const size_t guard_page_size = (size_t(fault_reserved_pages) + 1) * page_size;
        const size_t pages = (size + guard_page_size + page_size - 1) / page_size;
        // add one page at bottom that will be used as guard-page
        const size_t size__ = (pages + 1) * page_size;

        stack_context result;
        if (stack_allocations.pop(result)) {
            stack_allocations_buffer--;
            if(!flush_used_stacks)
                return result;
            else {
                memset(static_cast<char*>(result.sp) - result.size, 0xCC, result.size);
                return result;
            }
        }
        else
            return create_stack(size__);
    }

    void unlimited_buffer(stack_context& sctx ){
        if (!stack_allocations.push(sctx)) 
            munmap(static_cast<char*>(sctx.sp) - sctx.size, sctx.size);
        else
            stack_allocations_buffer++;
    }
    void limited_buffer(stack_context& sctx ){
    if (++stack_allocations_buffer < light_stack::max_buffer_size) {
            if (!stack_allocations.push(sctx)) {
                munmap(static_cast<char*>(sctx.sp) - sctx.size, sctx.size);
                stack_allocations_buffer--;
            }
        }
        else {
            munmap(static_cast<char*>(sctx.sp) - sctx.size, sctx.size);
            stack_allocations_buffer--;
        }
    }

    void light_stack::deallocate(stack_context& sctx ) {
        assert(sctx.sp);
        if(!max_buffer_size)
            unlimited_buffer(sctx);
        else if(max_buffer_size != SIZE_MAX)
            limited_buffer(sctx);
        else
            munmap(static_cast<char*>(sctx.sp) - sctx.size, sctx.size);
    }


    std::string fmt_protect(int prot) {
        std::string res;
        if (prot & PROT_NONE)
            res += "NOACCESS";
        if (prot & PROT_READ)
            res += res.size() ? " & READONLY" : "READONLY";
        if (prot & PROT_WRITE)
            res += res.size() ? " & READWRITE" : "READWRITE";
        if (prot & PROT_EXEC)
            res += res.size() ? " & EXECUTE" : "EXECUTE";
        if (prot & PROT_GROWSDOWN)
            res += res.size() ? " & GROWSDOWN" : "GROWSDOWN";
        if (prot & PROT_GROWSUP)
            res += res.size() ? " & GROWSUP" : "GROWSUP";
        #ifdef PROT_SAO
        if (prot & PROT_SAO)
            res += res.size() ? " & SAO" : "SAO";
        #endif
        #ifdef PROT_SEM
        if (prot & PROT_SEM)
            res += res.size() ? " & SEM" : "SEM";
        #endif
        if (prot & PROT_NONE)
            res += res.size() ? " & NONE" : "NONE";
        if (!res.size())
            return "ZERO";
        return res;
    }

    std::string dump_stack(bool* pPtr) {return "";}



    struct allocation_details{
        bool* base;
        size_t size;
        int prot;
        int flags;
    };

    BOOST_NOINLINE bool* GetStackPointer() {
        return (bool*)__builtin_return_address(0) + 8;
    }

    struct stack_snapshot {
        stack_snapshot(bool* pPtr) {}
        stack_snapshot() : stack_snapshot(GetStackPointer()) {}
        std::vector<allocation_details> allocations;
        allocation_details* reserved = nullptr;
        allocation_details* guard = nullptr;
        allocation_details* committed = nullptr;
        bool valid = true;
    };

    enum stack_item{
        reserved,
        guard,
        committed
    };
    template<stack_item item>
    allocation_details get_stack_detail(bool* pPtr){return allocation_details();}
    std::string dump_stack() {return "";}

    //shit code, TO-DO fix it
    bool light_stack::shrink_current(size_t bytes_threshold){return false;}

    bool light_stack::prepare(){return false;}

    bool light_stack::prepare(size_t bytes_to_use){return false;}


    ValueItem* light_stack::dump_current(){return nullptr;}
    std::string light_stack::dump_current_str(){return "";}
    void light_stack::dump_current_out(){}

    ValueItem* light_stack::dump(void* ptr){return nullptr;}
    //default formatted string
    std::string light_stack::dump_str(void*){return "";}
    //console
    void light_stack::dump_out(void* ptr){}
    size_t light_stack::allocated_size(){return (size_t)-1;}
    size_t light_stack::free_size(){return (size_t)-1;}
    size_t light_stack::used_size(){return (size_t)-1;}
    size_t light_stack::unused_size(){return (size_t)-1;}

    bool light_stack::is_supported(){return false;}
}
#endif