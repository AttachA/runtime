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
        ValueItem* grow(ValueItem*, uint32_t);
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
        }
    }
    //asm
    namespace _asm_ {
        namespace construct{


            ValueItem* createProxy_FunctionHolder(ValueItem*, uint32_t);//{ uarr<LabelPoint> points; PrologBuilder pbuild; ScopeMangaer sm; }
            
            ValueItem* createProxy_CallBuilder(ValueItem*, uint32_t);//typed_lgr<FunctionHolder> fh;
            ValueItem* createProxy_AbstractedBuilder(ValueItem*, uint32_t);//typed_lgr<FunctionHolder> fh;

            ValueItem* createProxy_Register(ValueItem*, uint32_t);
            // names different for different calling conventions and different platforms:
            // x64 windows: https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention
            //      resr(rax), argr0(rcx), argr1(rdx), argr2(r8), argr3(r9), mut0(r10),
            //      mut1(r11), nmut0(rbx), src(rsi), dst(rdi), base(rbp), stack(rsp),
            //      resr_32(eax), argr0_32(ecx), argr1_32(edx), argr2_32(r8d), argr3_32(r9d), mut0_32(r10d),
            //      mut1_32(r11d), nmut0_32(ebx), src_32(esi), dst_32(edi), base_32(ebp), stack_32(esp),
            //      resr_16(ax), argr0_16(cx), argr1_16(dx), argr2_16(r8w), argr3_16(r9w), mut0_16(r10w),
            //      mut1_16(r11w), nmut0_16(bx), src_16(si), dst_16(di), base_16(bp), stack_16(sp),
            //      resr_8l(al), argr0_8l(cl), argr1_8l(dl), src_8l(sil), dst_8l(dil), base_8l(bpl), stack_8l(spl),
            //      resr_8h(ah), argr0_8h(ch), argr1_8h(dh), argr2_8h(r8b), argr3_8h(r9b), mut0_8h(r10b),
            //      mut1_8h(r11b), nmut0_8h(bl), src_8h(sih), dst_8h(dih), base_8h(bph), stack_8h(sph),
            //      argr0_x(xmm0), argr1_x(xmm1), argr2_x(xmm2), argr3_x(xmm3), xmt4(xmm4), xmt5(xmm5),
            //      xmm6(xmm6), xmm7(xmm7), xmm8(xmm8), xmm9(xmm9), xmm10(xmm10), xmm11(xmm11),
            //      xmm12(xmm12), xmm13(xmm13), xmm14(xmm14), xmm15(xmm15)
            //
            // x64 linux System V: https://refspecs.linuxfoundation.org/elf/x86_64-abi-0.99.pdf
            //      resr(rax), argr0(rdi), argr1(rsi), argr2(rdx), argr3(rcx), argr4(r8),
            //      argr5(r9), mut0(r10), mut1(r11), nmut0(rdx), nmut1(rsi), base0(rbp), base1(rbx), stack(rsp),
            //      resr_32(eax), argr0_32(edi), argr1_32(esi), argr2_32(edx), argr3_32(ecx), argr4_32(r8d), argr5_32(r9d),
            //      mut0_32(r10d), mut1_32(r11d), nmut0_32(edx), nmut1_32(esi), base0_32(ebp), base1_32(ebx), stack_32(esp),
            //      resr_16(ax), argr0_16(di), argr1_16(si), argr2_16(dx), argr3_16(cx), argr4_16(r8w), argr5_16(r9w),
            //      mut0_16(r10w), mut1_16(r11w), nmut0_16(dx), nmut1_16(si), base0_16(bp), base1_16(bx), stack_16(sp),
            //      resr_8l(al), argr0_8l(dil), argr1_8l(sil), argr2_8l(dl), argr3_8l(cl), argr4_8l(r8b), argr5_8l(r9b),
            //      mut0_8l(r10b), mut1_8l(r11b), nmut0_8l(dl), nmut1_8l(sil), base0_8l(bpl), base1_8l(bl), stack_8l(spl),
            //      resr_8h(ah), argr0_8h(dih), argr1_8h(sih), argr2_8h(dh), argr3_8h(ch), argr4_8h(r8b), argr5_8h(r9b),
            //      base0_8h(bph), base1_8h(bh), stack_8h(sph),
            //      argr0_x/resr0_x(xmm0), argr1_x/resr0_x(xmm1), argr2_x(xmm2), argr3_x(xmm3), argr4_x(xmm4), argr5_x(xmm5),
            //      argr6_x(xmm6), argr7_x(xmm7), xmm8(xmm8), xmm9(xmm9), xmm10(xmm10), xmm11(xmm11),
            //      xmm12(xmm12), xmm13(xmm13), xmm14(xmm14), xmm15(xmm15)
            //
            // or regular names:
            //     rax, rbx, rcx, rdx, rbx, rsp, rbp, rsi, rdi, r8, r9, r10, r11, r12, r13, r14, r15
            //     eax, ebx, ecx, edx, ebx, esp, ebp, esi, edi, r8d, r9d, r10d, r11d, r12d, r13d, r14d, r15d
            //     ax, bx, cx, dx, bx, sp, bp, si, di, r8w, r9w, r10w, r11w, r12w, r13w, r14w, r15w
            //     al, bl, cl, dl, bl, ah, bh, ch, dh, r8b, r9b, r10b, r11b, r12b, r13b, r14b, r15b
            //     xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15
            //
            // if used invalid name then will be thrown exception InvalidArchitectureException
        }
    }
    void init();
}

#endif /* RUN_TIME_LIBRARY_INTERNAL */
