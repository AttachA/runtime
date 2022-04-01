/// this file external, used as manual for linux DWARF, not licensed
#pragma once
#include <vector>
#include <memory>
using namespace asmjit;
#ifdef _WIN32
#include <Windows.h>
#include <DbgHelp.h>
#else
#include <execinfo.h>
#include <cxxabi.h>
#include <cstring>
#include <cstdlib>
#endif

//#define ASMJIT_PROPAGATE(cmd) do{ if(auto tmp = (cmd)) throw CompileTimeException("Fail compile func, asmjit err code: " + std::to_string(tmp));}while(0)



#undef ASMJIT_PROPAGATE















//struct JitLineInfo
//{
//	ptrdiff_t InstructionIndex = 0;
//	int32_t LineNumber = -1;
//	asmjit::Label Label;
//};
//struct JitFuncInfo
//{
//	String name;
//	String filename;
//	std::vector<JitLineInfo> LineInfo;
//	void* start;
//	void* end;
//};




//static void* AllocJitMemory(size_t size)
//{
//	using namespace asmjit;
//
//	if (JitBlockPos + size <= JitBlockSize)
//	{
//		uint8_t* p = JitBlocks[JitBlocks.size() - 1];
//		p += JitBlockPos;
//		JitBlockPos += size;
//		return p;
//	}
//	else
//	{
//		void* p;
//		VirtMem::alloc(&p, 1024 * 1024, VirtMem::MemoryFlags::kAccessWrite | VirtMem::MemoryFlags::kAccessExecute);
//		if (!p)
//			return nullptr;
//		JitBlocks.push_back((uint8_t*)p);
//		JitBlockSize = 1024 * 1024;
//		JitBlockPos = size;
//		return p;
//	}
//}

#if false

extern "C"
{
	void __register_frame(const void*);
	void __deregister_frame(const void*);
}

static void WriteLength(std::vector<uint8_t>& stream, unsigned int pos, unsigned int v)
{
	*(uint32_t*)(&stream[pos]) = v;
}

static void WriteUInt64(std::vector<uint8_t>& stream, uint64_t v)
{
	for (int i = 0; i < 8; i++)
		stream.push_back((v >> (i * 8)) & 0xff);
}

static void WriteUInt32(std::vector<uint8_t>& stream, uint32_t v)
{
	for (int i = 0; i < 4; i++)
		stream.push_back((v >> (i * 8)) & 0xff);
}

static void WriteUInt16(std::vector<uint8_t>& stream, uint16_t v)
{
	for (int i = 0; i < 2; i++)
		stream.push_back((v >> (i * 8)) & 0xff);
}

static void WriteUInt8(std::vector<uint8_t>& stream, uint8_t v)
{
	stream.push_back(v);
}

static void WriteULEB128(std::vector<uint8_t>& stream, uint32_t v)
{
	while (true)
	{
		if (v < 128)
		{
			WriteUInt8(stream, v);
			break;
		}
		else
		{
			WriteUInt8(stream, (v & 0x7f) | 0x80);
			v >>= 7;
		}
	}
}

static void WriteSLEB128(std::vector<uint8_t>& stream, int32_t v)
{
	if (v >= 0)
	{
		WriteULEB128(stream, v);
	}
	else
	{
		while (true)
		{
			if (v > -128)
			{
				WriteUInt8(stream, v & 0x7f);
				break;
			}
			else
			{
				WriteUInt8(stream, v);
				v >>= 7;
			}
		}
	}
}

static void WritePadding(std::vector<uint8_t>& stream)
{
	int padding = stream.size() % 8;
	if (padding != 0)
	{
		padding = 8 - padding;
		for (int i = 0; i < padding; i++) WriteUInt8(stream, 0);
	}
}

static void WriteCIE(std::vector<uint8_t>& stream, const std::vector<uint8_t>& cieInstructions, uint8_t returnAddressReg)
{
	unsigned int lengthPos = stream.size();
	WriteUInt32(stream, 0); // Length
	WriteUInt32(stream, 0); // CIE ID

	WriteUInt8(stream, 1); // CIE Version
	WriteUInt8(stream, 'z');
	WriteUInt8(stream, 'R'); // fde encoding
	WriteUInt8(stream, 0);
	WriteULEB128(stream, 1);
	WriteSLEB128(stream, -1);
	WriteULEB128(stream, returnAddressReg);

	WriteULEB128(stream, 1); // LEB128 augmentation size
	WriteUInt8(stream, 0); // DW_EH_PE_absptr (FDE uses absolute pointers)

	for (unsigned int i = 0; i < cieInstructions.size(); i++)
		stream.push_back(cieInstructions[i]);

	WritePadding(stream);
	WriteLength(stream, lengthPos, stream.size() - lengthPos - 4);
}

static void WriteFDE(std::vector<uint8_t>& stream, const std::vector<uint8_t>& fdeInstructions, uint32_t cieLocation, unsigned int& functionStart)
{
	unsigned int lengthPos = stream.size();
	WriteUInt32(stream, 0); // Length
	uint32_t offsetToCIE = stream.size() - cieLocation;
	WriteUInt32(stream, offsetToCIE);

	functionStart = stream.size();
	WriteUInt64(stream, 0); // func start
	WriteUInt64(stream, 0); // func size

	WriteULEB128(stream, 0); // LEB128 augmentation size

	for (unsigned int i = 0; i < fdeInstructions.size(); i++)
		stream.push_back(fdeInstructions[i]);

	WritePadding(stream);
	WriteLength(stream, lengthPos, stream.size() - lengthPos - 4);
}

static void WriteAdvanceLoc(std::vector<uint8_t>& fdeInstructions, uint64_t offset, uint64_t& lastOffset)
{
	uint64_t delta = offset - lastOffset;
	if (delta < (1 << 6))
	{
		WriteUInt8(fdeInstructions, (1 << 6) | delta); // DW_CFA_advance_loc
	}
	else if (delta < (1 << 8))
	{
		WriteUInt8(fdeInstructions, 2); // DW_CFA_advance_loc1
		WriteUInt8(fdeInstructions, delta);
	}
	else if (delta < (1 << 16))
	{
		WriteUInt8(fdeInstructions, 3); // DW_CFA_advance_loc2
		WriteUInt16(fdeInstructions, delta);
	}
	else
	{
		WriteUInt8(fdeInstructions, 4); // DW_CFA_advance_loc3
		WriteUInt32(fdeInstructions, delta);
	}
	lastOffset = offset;
}

static void WriteDefineCFA(std::vector<uint8_t>& cieInstructions, int dwarfRegId, int stackOffset)
{
	WriteUInt8(cieInstructions, 0x0c); // DW_CFA_def_cfa
	WriteULEB128(cieInstructions, dwarfRegId);
	WriteULEB128(cieInstructions, stackOffset);
}

static void WriteDefineStackOffset(std::vector<uint8_t>& fdeInstructions, int stackOffset)
{
	WriteUInt8(fdeInstructions, 0x0e); // DW_CFA_def_cfa_offset
	WriteULEB128(fdeInstructions, stackOffset);
}

static void WriteRegisterStackLocation(std::vector<uint8_t>& instructions, int dwarfRegId, int stackLocation)
{
	WriteUInt8(instructions, (2 << 6) | dwarfRegId); // DW_CFA_offset
	WriteULEB128(instructions, stackLocation);
}

static std::vector<uint8_t> CreateUnwindInfoUnix(asmjit::FuncNode* func, unsigned int& functionStart)
{
	using namespace asmjit;

	// Build .eh_frame:
	//
	// The documentation for this can be found in the DWARF standard
	// The x64 specific details are described in "System V Application Binary Interface AMD64 Architecture Processor Supplement"
	//
	// See appendix D.6 "Call Frame Information Example" in the DWARF 5 spec.
	//
	// The CFI_Parser<A>::decodeFDE parser on the other side..
	// https://github.com/llvm-mirror/libunwind/blob/master/src/DwarfParser.hpp

	// Asmjit -> DWARF register id
	int dwarfRegId[16];
	dwarfRegId[x86::Gp::kIdAx] = 0;
	dwarfRegId[x86::Gp::kIdDx] = 1;
	dwarfRegId[x86::Gp::kIdCx] = 2;
	dwarfRegId[x86::Gp::kIdBx] = 3;
	dwarfRegId[x86::Gp::kIdSi] = 4;
	dwarfRegId[x86::Gp::kIdDi] = 5;
	dwarfRegId[x86::Gp::kIdBp] = 6;
	dwarfRegId[x86::Gp::kIdSp] = 7;
	dwarfRegId[x86::Gp::kIdR8] = 8;
	dwarfRegId[x86::Gp::kIdR9] = 9;
	dwarfRegId[x86::Gp::kIdR10] = 10;
	dwarfRegId[x86::Gp::kIdR11] = 11;
	dwarfRegId[x86::Gp::kIdR12] = 12;
	dwarfRegId[x86::Gp::kIdR13] = 13;
	dwarfRegId[x86::Gp::kIdR14] = 14;
	dwarfRegId[x86::Gp::kIdR15] = 15;
	int dwarfRegRAId = 16;
	int dwarfRegXmmId = 17;

	std::vector<uint8_t> cieInstructions;
	std::vector<uint8_t> fdeInstructions;

	uint8_t returnAddressReg = dwarfRegRAId;
	int stackOffset = 8; // Offset from RSP to the Canonical Frame Address (CFA) - stack position where the CALL return address is stored

	WriteDefineCFA(cieInstructions, dwarfRegId[x86::Gp::kIdSp], stackOffset);
	WriteRegisterStackLocation(cieInstructions, returnAddressReg, stackOffset);

	FuncFrameLayout layout;
	func->detail().
	Error error = layout.init(func->detail(), func->frame());
	if (error != kErrorOk)x
		I_Error("FuncFrameLayout.init failed");

	// We need a dummy emitter for instruction size calculations
	CodeHolder code;
	code.init(GetHostCodeInfo());
	x86::Assembler assembler(&code);
	x86::Emitter* emitter = &assembler;
	uint64_t lastOffset = 0;

	// Note: the following code must match exactly what x86::Internal::emitProlog does

	x86::Gp zsp = emitter->zsp();   // ESP|RSP register.
	x86::Gp zbp = emitter->zsp();   // EBP|RBP register.
	zbp.setId(x86::Gp::kIdBp);
	x86::Gp gpReg = emitter->zsp(); // General purpose register (temporary).
	x86::Gp saReg = emitter->zsp(); // Stack-arguments base register.
	uint32_t gpSaved = layout.getSavedRegs(x86::Reg::kKindGp);

	if (layout.hasPreservedFP())
	{
		// Emit: 'push zbp'
		//       'mov  zbp, zsp'.
		gpSaved &= ~Utils::mask(x86::Gp::kIdBp);
		emitter->push(zbp);

		stackOffset += 8;
		WriteAdvanceLoc(fdeInstructions, assembler.offset(), lastOffset);
		WriteDefineStackOffset(fdeInstructions, stackOffset);
		WriteRegisterStackLocation(fdeInstructions, dwarfRegId[x86::Gp::kIdBp], stackOffset);

		emitter->mov(zbp, zsp);
	}

	if (gpSaved)
	{
		for (uint32_t i = gpSaved, regId = 0; i; i >>= 1, regId++)
		{
			if (!(i & 0x1)) continue;
			// Emit: 'push gp' sequence.
			gpReg.setId(regId);
			emitter->push(gpReg);

			stackOffset += 8;
			WriteAdvanceLoc(fdeInstructions, assembler.offset(), lastOffset);
			WriteDefineStackOffset(fdeInstructions, stackOffset);
			WriteRegisterStackLocation(fdeInstructions, dwarfRegId[regId], stackOffset);
		}
	}

	uint32_t stackArgsRegId = layout.getStackArgsRegId();
	if (stackArgsRegId != Globals::kInvalidRegId && stackArgsRegId != x86::Gp::kIdSp)
	{
		saReg.setId(stackArgsRegId);
		if (!(layout.hasPreservedFP() && stackArgsRegId == x86::Gp::kIdBp))
		{
			// Emit: 'mov saReg, zsp'.
			emitter->mov(saReg, zsp);
		}
	}

	if (layout.hasDynamicAlignment())
	{
		// Emit: 'and zsp, StackAlignment'.
		emitter->and_(zsp, -static_cast<int32_t>(layout.getStackAlignment()));
	}

	if (layout.hasStackAdjustment())
	{
		// Emit: 'sub zsp, StackAdjustment'.
		emitter->sub(zsp, layout.getStackAdjustment());

		stackOffset += layout.getStackAdjustment();
		WriteAdvanceLoc(fdeInstructions, assembler.offset(), lastOffset);
		WriteDefineStackOffset(fdeInstructions, stackOffset);
	}

	if (layout.hasDynamicAlignment() && layout.hasDsaSlotUsed())
	{
		// Emit: 'mov [zsp + dsaSlot], saReg'.
		x86::Mem saMem = x86::ptr(zsp, layout._dsaSlot);
		emitter->mov(saMem, saReg);
	}

	uint32_t xmmSaved = layout.getSavedRegs(x86::Reg::kKindVec);
	if (xmmSaved)
	{
		int vecOffset = layout.getVecStackOffset();
		x86::Mem vecBase = x86::ptr(zsp, layout.getVecStackOffset());
		x86::Reg vecReg = x86::xmm(0);
		bool avx = layout.isAvxEnabled();
		bool aligned = layout.hasAlignedVecSR();
		uint32_t vecInst = aligned ? (avx ? x86::Inst::kIdVmovaps : x86::Inst::kIdMovaps) : (avx ? x86::Inst::kIdVmovups : x86::Inst::kIdMovups);
		uint32_t vecSize = 16;
		for (uint32_t i = xmmSaved, regId = 0; i; i >>= 1, regId++)
		{
			if (!(i & 0x1)) continue;

			// Emit 'movaps|movups [zsp + X], xmm0..15'.
			vecReg.setId(regId);
			emitter->emit(vecInst, vecBase, vecReg);
			vecBase.addOffsetLo32(static_cast<int32_t>(vecSize));

			WriteAdvanceLoc(fdeInstructions, assembler.offset(), lastOffset);
			WriteRegisterStackLocation(fdeInstructions, dwarfRegXmmId + regId, stackOffset - vecOffset);
			vecOffset += static_cast<int32_t>(vecSize);
		}
	}

	std::vector<uint8_t> stream;
	WriteCIE(stream, cieInstructions, returnAddressReg);
	WriteFDE(stream, fdeInstructions, 0, functionStart);
	WriteUInt32(stream, 0);
	return stream;
}

void* AddJitFunction(asmjit::CodeHolder* code, JitCompiler* compiler)
{
	using namespace asmjit;

	CCFunc* func = compiler->Codegen();

	size_t codeSize = code->getCodeSize();
	if (codeSize == 0)
		return nullptr;

	unsigned int fdeFunctionStart = 0;
	std::vector<uint8_t> unwindInfo = CreateUnwindInfoUnix(func, fdeFunctionStart);
	size_t unwindInfoSize = unwindInfo.size();

	codeSize = (codeSize + 15) / 16 * 16;

	uint8_t* p = (uint8_t*)AllocJitMemory(codeSize + unwindInfoSize);
	if (!p)
		return nullptr;

	size_t relocSize = code->relocate(p);
	if (relocSize == 0)
		return nullptr;

	size_t unwindStart = relocSize;
	unwindStart = (unwindStart + 15) / 16 * 16;
	JitBlockPos -= codeSize - unwindStart;

	uint8_t* baseaddr = JitBlocks.back();
	uint8_t* startaddr = p;
	uint8_t* endaddr = p + relocSize;
	uint8_t* unwindptr = p + unwindStart;
	memcpy(unwindptr, &unwindInfo[0], unwindInfoSize);

	if (unwindInfo.size() > 0)
	{
		uint64_t* unwindfuncaddr = (uint64_t*)(unwindptr + fdeFunctionStart);
		unwindfuncaddr[0] = (ptrdiff_t)startaddr;
		unwindfuncaddr[1] = (ptrdiff_t)(endaddr - startaddr);

#ifdef __APPLE__
		// On macOS __register_frame takes a single FDE as an argument
		uint8_t* entry = unwindptr;
		while (true)
		{
			uint32_t length = *((uint32_t*)entry);
			if (length == 0)
				break;

			if (length == 0xffffffff)
			{
				uint64_t length64 = *((uint64_t*)(entry + 4));
				if (length64 == 0)
					break;

				uint64_t offset = *((uint64_t*)(entry + 12));
				if (offset != 0)
				{
					__register_frame(entry);
					JitFrames.push_back(entry);
				}
				entry += length64 + 12;
			}
			else
			{
				uint32_t offset = *((uint32_t*)(entry + 4));
				if (offset != 0)
				{
					__register_frame(entry);
					JitFrames.push_back(entry);
				}
				entry += length + 4;
			}
		}
#else
		// On Linux it takes a pointer to the entire .eh_frame
		__register_frame(unwindptr);
		JitFrames.push_back(unwindptr);
#endif
	}

	JitDebugInfo.push_back({ compiler->GetScriptFunction()->PrintableName, compiler->GetScriptFunction()->SourceFileName, compiler->LineInfo, startaddr, endaddr });

	return p;
}
#endif
