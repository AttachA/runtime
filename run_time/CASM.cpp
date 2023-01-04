// Copyright Danyil Melnytskyi 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "CASM.hpp"
#include <Windows.h>
#include <DbgHelp.h>
#include <dbgeng.h>
#include <string>
#include <unordered_map>
#include <mutex>
#include <cassert>
#include "tasks.hpp"
void _______dbgOut(const char* str) {
	OutputDebugStringA(str);
}

#define CONV_ASMJIT(to_conv) { if(auto tmp = (to_conv)) throw CompileTimeException("Fail create func asmjit err code: " + std::to_string(tmp)); }
//return offset from allocated to additional size or code size
size_t alocate_and_prepare_code(uint8_t*& res, CodeHolder* code, asmjit::JitAllocator* alloc, size_t additional_size) {
	res = 0;
	CONV_ASMJIT(code->flatten());
	CONV_ASMJIT(code->resolveUnresolvedLinks());

	size_t estimatedCodeSize = code->codeSize();
	if (ASMJIT_UNLIKELY(estimatedCodeSize == 0))
		throw CompileTimeException("Code is empty");

	uint8_t* rx;
	uint8_t* rw;
	CONV_ASMJIT(alloc->alloc((void**)&rx, (void**)&rw, estimatedCodeSize + additional_size));

	// Relocate the code.
	Error err = code->relocateToBase(uintptr_t((void*)rx));
	if (ASMJIT_UNLIKELY(err)) {
		alloc->release(rx);
		return err;
	}

	// Recalculate the final code size and shrink the memory we allocated for it
	// in case that some relocations didn't require records in an address table.
	size_t codeSize = code->codeSize();

	if (codeSize < estimatedCodeSize)
		alloc->shrink(rx, codeSize + additional_size);

	{
		//asmjit deprecated
		//asmjit::VirtMem::ProtectJitReadWriteScope rwScope(rx, codeSize);

		for (asmjit::Section* section : code->_sections) {
			size_t offset = size_t(section->offset());
			size_t bufferSize = size_t(section->bufferSize());
			size_t virtualSize = size_t(section->virtualSize());

			assert(offset + bufferSize <= codeSize);
			memcpy(rw + offset, section->data(), bufferSize);

			if (virtualSize > bufferSize) {
				assert(offset + virtualSize <= codeSize);
				memset(rw + offset + bufferSize, 0, virtualSize - bufferSize);
			}
		}
	}

	res = rx;
	return codeSize;
    
}


#ifdef _WIN64
#pragma comment(lib,"Dbghelp.lib")
struct frame_info {
	std::string name;
	std::string file;
	size_t fun_size = 0;
};
std::unordered_map<uint8_t*, frame_info> frame_symbols;
TaskMutex DbgHelp_lock;





struct NativeSymbolResolver {
	NativeSymbolResolver() {
		SymInitialize(GetCurrentProcess(), nullptr, true);
	}
	~NativeSymbolResolver() {
		SymCleanup(GetCurrentProcess());
	}

	StackTraceItem GetName(void* frame) {
		unsigned char buffer[sizeof(SYMBOL_INFO) + 128];
		PSYMBOL_INFO symbol64 = reinterpret_cast<SYMBOL_INFO*>(buffer);
		memset(symbol64, 0, sizeof(SYMBOL_INFO) + 128);
		symbol64->SizeOfStruct = sizeof(SYMBOL_INFO);
		symbol64->MaxNameLen = 128;

		DWORD64 displacement = 0;
		BOOL result = SymFromAddr(GetCurrentProcess(), (DWORD64)frame, &displacement, symbol64);
		if (result) {
			IMAGEHLP_LINE64 line64;
			DWORD displacement = 0;
			memset(&line64, 0, sizeof(IMAGEHLP_LINE64));
			line64.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
			result = SymGetLineFromAddr64(GetCurrentProcess(), (DWORD64)frame, &displacement, &line64);
			if (result)
				return { symbol64->Name, line64.FileName, line64.LineNumber };
			else
				return { symbol64->Name, line64.FileName ? line64.FileName: "UNDEFINED" , SIZE_MAX};
		}
		return { "UNDEFINED","UNDEFINED", SIZE_MAX };
	}
};

uint32_t CaptureStackTrace___(uint32_t max_frames, void** out_frames, CONTEXT& context) {
	memset(out_frames, 0, sizeof(void*) * max_frames);

	// RtlCaptureStackBackTrace doesn't support RtlAddFunctionTable..
	//return RtlCaptureStackBackTrace(0, max_frames, out_frames, nullptr);
	
	//if you wanna port to x32 use RtlCaptureStackBackTrace
	UNWIND_HISTORY_TABLE history;
	memset(&history, 0, sizeof(UNWIND_HISTORY_TABLE));
	
	ULONG64 establisherframe = 0;
	PVOID handlerdata = nullptr;
	
	uint32_t frame;
	for (frame = 0; frame < max_frames; frame++) {
		ULONG64 imagebase;
		PRUNTIME_FUNCTION rtfunc = RtlLookupFunctionEntry(context.Rip, &imagebase, &history);
	
		KNONVOLATILE_CONTEXT_POINTERS nvcontext;
		memset(&nvcontext, 0, sizeof(KNONVOLATILE_CONTEXT_POINTERS));
		if (!rtfunc) {
			context.Rip = (ULONG64)(*(PULONG64)context.Rsp);
			context.Rsp += 8;
		}
		else
			RtlVirtualUnwind(UNW_FLAG_NHANDLER, imagebase, context.Rip, rtfunc, &context, &handlerdata, &establisherframe, &nvcontext);
		
	
		if (!context.Rip)
			break;
	
		out_frames[frame] = (void*)context.Rip;
	}
	return frame;
}

uint32_t CaptureStackTrace(uint32_t max_frames, void** out_frames) {
	CONTEXT context;
	RtlCaptureContext(&context);
	return CaptureStackTrace___(max_frames, out_frames, context);
}
uint32_t CaptureStackTrace(uint32_t max_frames, void** out_frames, void* rip_frame) {
	CONTEXT context{0};
	context.Rip = (DWORD64)rip_frame;
	return CaptureStackTrace___(max_frames, out_frames, context);
}


size_t JITPCToLine(uint8_t* pc, const frame_info* info) {
	//int PCIndex = int(pc - ((uint8_t*)(info->start)));
	//if (info->LineInfo.Size() == 1) return info->LineInfo[0].LineNumber;
	//for (unsigned i = 1; i < info->LineInfo.Size(); i++)
	//{
	//	if (info->LineInfo[i].InstructionIndex >= PCIndex)
	//	{
	//		return info->LineInfo[i - 1].LineNumber;
	//	}
	//}
	return SIZE_MAX;
}






//convert FrameResult struct to native unwindInfo
std::vector<uint16_t> convert(FrameResult& frame) {
	auto& codes = frame.prolog;

	std::vector<uint16_t> info((sizeof(UWINFO_head) >> 1));
	info.reserve(codes.size() + codes.size() & 1);

	frame.head.Flags = frame.use_handle ? UNW_FLAG_EHANDLER : 0;
	*(UWINFO_head*)(info.data()) = frame.head;

	for (size_t i = codes.size(); i > 0; i--)
		info.push_back(codes[i - 1]);

	if (codes.size() & 1)
		info.push_back(0);

	if (frame.use_handle) {
		info.resize(info.size() + 4);
		*(DWORD*)(info.data() + info.size() - 4) = frame.exHandleOff;
	}
	info.resize(info.size() + 8);
	*(uint64_t*)(info.data() + info.size() - 8) = 0xFFFFFFFFFFFFFFFF;
	return info;
}


EXCEPTION_DISPOSITION handle_s(
	IN PEXCEPTION_RECORD ExceptionRecord,
	IN ULONG64 EstablisherFrame,
	IN OUT PCONTEXT ContextRecord,
	IN OUT PDISPATCHER_CONTEXT DispatcherContext
) {
	printf("hello!");
	return ExceptionContinueSearch;
}

void* __casm_test_handle = (void*)handle_s;
void* FrameResult::init(uint8_t*& frame,CodeHolder* code, asmjit::JitRuntime& runtime, const char* symbol_name, const char* file_path) {
	std::vector<uint16_t> unwindInfo = convert(*this);
	size_t unwindInfoSize = unwindInfo.size() * sizeof(uint16_t);

	uint8_t* baseaddr;
	size_t fun_size = alocate_and_prepare_code(baseaddr, code, runtime.allocator(), unwindInfoSize + sizeof(RUNTIME_FUNCTION));
	uint8_t* startaddr = baseaddr;
	uint8_t* unwindptr = baseaddr + (((fun_size + 15) >> 4) << 4);
	memcpy(unwindptr, unwindInfo.data(), unwindInfoSize);

	RUNTIME_FUNCTION* table = (RUNTIME_FUNCTION*)(unwindptr + unwindInfoSize);
	frame = (uint8_t*)table;
	table[0].BeginAddress = (DWORD)(ptrdiff_t)(startaddr - baseaddr);
	table[0].EndAddress = (DWORD)(ptrdiff_t)(use_handle ? exHandleOff : fun_size);
	table[0].UnwindData = (DWORD)(ptrdiff_t)(unwindptr - baseaddr);
	BOOLEAN result = RtlAddFunctionTable(table, 1, (DWORD64)baseaddr);

	if (result == 0) {
		runtime.allocator()->release(baseaddr);
		throw CompileTimeException("RtlAddFunctionTable failed");
	}
	auto& tmp = frame_symbols[frame];
	tmp.fun_size = fun_size;
	tmp.name = symbol_name;
	tmp.file = file_path;
	return baseaddr;
}
bool FrameResult::deinit(uint8_t* frame, void* funct, asmjit::JitRuntime& runtime){
	if (frame) {
		BOOLEAN result = RtlDeleteFunctionTable((RUNTIME_FUNCTION*)frame);
		auto tmp = runtime.allocator()->release(funct);
		frame_symbols.erase(frame);
		return !(result == 0 || tmp);
	}
	return false;
};
#else
struct NativeSymbolResolver {
	StackTraceItem GetName(void* frame) {
		char** strings;
		void* frames[1] = { frame };
		strings = backtrace_symbols(frames, 1);

		char* ptr = strings[0];
		char* filename = ptr;
		const char* function = "";

		while (*ptr) {
			if (*ptr == '(') {
				*(ptr++) = 0;
				function = ptr;
				break;
			}
			ptr++;
		}

		if (function[0]) {
			while (*ptr) {
				if (*ptr == '+' || *ptr == ')') {
					*(ptr++) = 0;
					break;
				}
				ptr++;
			}
		}

		int status;
		char* new_function = abi::__cxa_demangle(function, nullptr, nullptr, &status);
		if (new_function)
			function = new_function;
		

		std::string s = "Called from " + std::string(function) + " at " + filename + '\n';
		if (new_function)
			free(new_function);
		free(strings);
		return { s,"UNDEFINED",SIZE_MAX };
	}
};


size_t JITPCToLine(uint8_t* pc, const frame_info* info) {
	//int PCIndex = int(pc - ((uint8_t*)(info->start)));
	//if (info->LineInfo.Size() == 1) return info->LineInfo[0].LineNumber;
	//for (unsigned i = 1; i < info->LineInfo.Size(); i++)
	//{
	//	if (info->LineInfo[i].InstructionIndex >= PCIndex)
	//	{
	//		return info->LineInfo[i - 1].LineNumber;
	//	}
	//}
	return SIZE_MAX;
}
uint32_t CaptureStackTrace(uint32_t max_frames, void** out_frames) {
	if(max_frames != (int32_t)max_frames)
		return 0;
	return (uint32_t)backtrace(out_frames, (int32_t)max_frames);
}
uint32_t CaptureStackTrace(uint32_t max_frames, void** out_frames, void* rip) {
	return 0;
}


void* PrologResult::init(uint8_t*& frame, CodeHolder* code, asmjit::JitRuntime& runtime, const char* symbol_name) {
	void* res = 0;
	frame = nullptr;
	CONV_ASMJIT(runtime._add(&res, code));
	return res;
}
bool PrologResult::deinit(uint8_t* frame, void* funct, asmjit::JitRuntime& runtime) {
	return !(runtime._release(funct));
};
#endif



StackTraceItem JitGetStackFrameName(NativeSymbolResolver* nativeSymbols, void* pc) {
	for (auto& it : frame_symbols) {
		auto& info = it.second;
		if (pc >= it.first && pc < (it.first + info.fun_size))
			return { info.name,  info.file, JITPCToLine((uint8_t*)pc, &info) };
	}
	return nativeSymbols ? nativeSymbols->GetName(pc) : StackTraceItem("UNDEFINED", "UNDEFINED", SIZE_MAX);
}



std::vector<StackTraceItem> FrameResult::JitCaptureStackTrace(uint32_t framesToSkip, bool includeNativeFrames, uint32_t max_frames) {
#ifdef _WIN64
	std::lock_guard lg(DbgHelp_lock);//in windiws NativeSymbolResolver class and CaptureStackTrace function use single thread DbgHelp functions
#endif
	if (max_frames + 1 == 0)
		throw std::bad_array_new_length();
	max_frames += 1;
	std::unique_ptr<void*,std::default_delete<void*[]>> frames_buffer(new void*[max_frames]);
	void** frame = frames_buffer.get();
	uint32_t numframes = CaptureStackTrace(max_frames, frame);

	std::unique_ptr<NativeSymbolResolver> nativeSymbols;
	if (includeNativeFrames)
		nativeSymbols.reset(new NativeSymbolResolver());

	std::vector<StackTraceItem> stack_trace;
	for (uint32_t i = framesToSkip + 1; i < numframes; i++)
		stack_trace.push_back(JitGetStackFrameName(nativeSymbols.get(), frame[i]));
	return stack_trace;
}

std::vector<void*>* FrameResult::JitCaptureStackChainTrace(uint32_t framesToSkip, bool includeNativeFrames, uint32_t max_frames) {
#ifdef _WIN64
	std::lock_guard lg(DbgHelp_lock);//in windiws NativeSymbolResolver class and CaptureStackTrace function use single thread DbgHelp functions
#endif
	if (max_frames + 1 == 0)
		throw std::bad_array_new_length();
	max_frames += 1;
	std::unique_ptr<void*,std::default_delete<void*[]>> frames_buffer(new void*[max_frames]);
	void** frame = frames_buffer.get();
	uint32_t numframes = CaptureStackTrace(max_frames, frame);
	if (framesToSkip >= numframes)
		return nullptr;
	else 
		return new std::vector<void*>( frame + framesToSkip, frame + numframes);
}

std::vector<StackTraceItem> FrameResult::JitCaptureExternStackTrace(void* rip, uint32_t framesToSkip, bool includeNativeFrames, uint32_t max_frames) {
#ifdef _WIN64
	std::lock_guard lg(DbgHelp_lock);//in windiws NativeSymbolResolver class and CaptureStackTrace function use single thread DbgHelp functions
#endif
	if (max_frames + 1 == 0)
		throw std::bad_array_new_length();
	max_frames += 1;
	std::unique_ptr<void*,std::default_delete<void*[]>> frames_buffer(new void*[max_frames]);
	void** frame = frames_buffer.get();
	uint32_t numframes = CaptureStackTrace(max_frames, frame, rip);

	std::unique_ptr<NativeSymbolResolver> nativeSymbols;
	if (includeNativeFrames)
		nativeSymbols.reset(new NativeSymbolResolver());

	std::vector<StackTraceItem> stack_trace;
	for (uint32_t i = framesToSkip + 1; i < numframes; i++)
		stack_trace.push_back(JitGetStackFrameName(nativeSymbols.get(), frame[i]));
	return stack_trace;
}

std::vector<void*>* FrameResult::JitCaptureExternStackChainTrace(void* rip, uint32_t framesToSkip, bool includeNativeFrames, uint32_t max_frames) {
#ifdef _WIN64
	std::lock_guard lg(DbgHelp_lock);//in windiws NativeSymbolResolver class and CaptureStackTrace function use single thread DbgHelp functions
#endif
	if (max_frames + 1 == 0)
		throw std::bad_array_new_length();
	max_frames += 1;
	std::unique_ptr<void*,std::default_delete<void*[]>> frames_buffer(new void*[max_frames]);
	void** frame = frames_buffer.get();
	uint32_t numframes = CaptureStackTrace(max_frames, frame, rip);
	if (framesToSkip >= numframes)
		return nullptr;
	else 
		return new std::vector<void*>( frame + framesToSkip, frame + numframes);
}