#pragma once
#include <capstone.h>
#include <utility>
#ifdef TARGET_ARCH_ARM64
#include "Disassembler_arm64.h"
#else
// x64 / no-analysis builds: minimal stand-in types so the shared headers compile.
// All real disassembly/analysis is excluded via NO_CODE_ANALYSIS, and the only
// A64 type referenced outside arm64 files is A64::Register. Disassembler_arm64.cpp
// (still in the build) defines RegisterNames / the capstone handle against this stub.
namespace A64 {
        class Register {
        public:
                enum Value : int { kInvalidRegister = -1 };
                static constexpr int kNumberOfRegisters = 67; // r0-r31 + d0-d31 + rCSP/rZR/NZCV (see RegisterNames in Disassembler_arm64.cpp)
                static const char* RegisterNames[kNumberOfRegisters]; // defined in Disassembler_arm64.cpp
                constexpr Register() : reg(kInvalidRegister) {}
                constexpr Register(Value v) : reg(v) {}
                constexpr bool operator==(Register a) const { return reg == a.reg; }
                constexpr bool operator!=(Register a) const { return reg != a.reg; }
                constexpr bool operator==(Value v) const { return reg == v; }
                constexpr bool operator!=(Value v) const { return reg != v; }
                bool IsSet() const { return reg >= 0; }
                const char* Name() const { return (reg >= 0 && reg < kNumberOfRegisters) ? RegisterNames[reg] : "invalid"; }
                constexpr operator int() const { return reg; }
        private:
                Value reg;
        };
}
class AsmInstruction {
public:
        explicit AsmInstruction(cs_insn* insn = nullptr) : insn(insn) {}
        uint64_t address() const { return insn->address; }
        uint32_t size() const { return insn->size; }
        // no-analysis / x64: real operand inspection is never meaningful here
        uint8_t op_count() const { return 0; }
        struct Operands {
                const cs_arm64_op& operator[](size_t) const { static cs_arm64_op dummy{}; return dummy; }
        } ops;
        cs_insn* insn;
};
// placeholders so unguarded IDA-script helpers compile; never match at runtime
constexpr arm64_reg CSREG_DART_PP = ARM64_REG_INVALID;
constexpr arm64_reg CSREG_DART_THR = ARM64_REG_INVALID;
// verbatim from Disassembler_arm64.h (arch-independent helper used by il.h etc.)
struct AddrRange {
        uint64_t start{ 0 };
        uint64_t end{ 0 };

        AddrRange() = default;
        AddrRange(uint64_t start, uint64_t end) : start{ start }, end{ end } {}

        bool Has(uint64_t addr) const { return addr >= start && addr < end; }
};
#endif


// master of disassmbled instructions from capstone
// do not allow copy because this class must free the instructions
class AsmInstructions {
	cs_insn* insns;
	size_t count;

	AsmInstructions(cs_insn* insns, size_t count) : insns(insns), count(count) {}
public:
	AsmInstructions() = delete;
	AsmInstructions(const AsmInstructions&) = delete;
	AsmInstructions(AsmInstructions&& rhs) noexcept : insns(std::exchange(rhs.insns, nullptr)), count(std::exchange(rhs.count, 0)) {}
	AsmInstructions& operator=(const AsmInstructions&) = delete;
	~AsmInstructions() { if (insns) cs_free(insns, count); }

	cs_insn* Insns() { return insns; }
	size_t Count() const { return count; }
	AsmInstruction First() { return AsmInstruction(insns); }
	AsmInstruction Last() { return AsmInstruction(&insns[count - 1]); }
	cs_insn* FirstPtr() { return insns; }
	cs_insn* LastPtr() { return &insns[count - 1]; }
	bool IsFirst(AsmInstruction& insn) { return insn.address() == insns->address; }

	AsmInstruction At(size_t i) { return AsmInstruction(insns + i); }
	cs_insn* Ptr(size_t i) { return &insns[i]; }
	size_t AtIndex(uint64_t addr) {
		ASSERT(addr > insns->address);
		// estimate index (normally 4 bytes per instruction for arm64)
		auto idx = (addr - insns->address) / 4;
		ASSERT(idx < count);
		while (idx < count && insns[idx].address < addr)
			++idx;
		return idx;
	}
	AsmInstruction AtAddr(uint64_t addr) {
		ASSERT(addr > insns->address);
		// estimate index (normally 4 bytes per instruction for arm64)
		auto idx = (addr - insns->address) / 4;
		ASSERT(idx < count);
		auto insn = &insns[idx];
		while (insn->address < addr)
			++insn;
		ASSERT(insn->address == addr);
		return AsmInstruction(insn);
	}

	friend class Disassembler;
	friend class Instruction;
};

// partial instructions from Instructions object.
// this class object are safe to copy/move because there is no freeing when destructor is called
class AsmBlock {
	cs_insn* insns;
	cs_insn* last_insn;

public:
	explicit AsmBlock() : insns(nullptr), last_insn(nullptr) {}
	explicit AsmBlock(cs_insn* insns, cs_insn* last_insn) : insns(insns), last_insn(last_insn) {}
	bool isValid() { return insns != nullptr; }
	AsmInstruction first() { return AsmInstruction(insns); }
	AsmInstruction last() { return AsmInstruction(last_insn); }
	cs_insn* first_ptr() { return insns; }
	cs_insn* last_ptr() { return last_insn; }
	AsmInstruction at(size_t i) { return AsmInstruction(insns + i); }
	cs_insn* ptr(size_t i) { return &insns[i]; }
	bool isLast(cs_insn* insn) { return insn == last_insn; }
	bool isAfter(cs_insn* insn) { return insn->address > last_insn->address; }
	uint64_t Address() { return insns->address; }
	uint64_t AddressEnd() { return last_insn->address + last_insn->size; }

	friend class Instruction;
};

class Disassembler
{
public:
	Disassembler(bool hasDetail = true);
	~Disassembler() { cs_close(&cshandle); }
	Disassembler(const Disassembler&) = delete;
	Disassembler(Disassembler&&) = delete;
	Disassembler& operator=(const Disassembler&) = delete;

	AsmInstructions Disasm(const uint8_t* code, size_t code_size, uint64_t address, size_t max_count = 0);
	const char* GetRegName(arm64_reg reg) { return cs_reg_name(cshandle, reg); }

private:
	csh cshandle;
};

