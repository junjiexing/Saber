#pragma once

#include <cstdint>

struct x86dis_vex
{
	uint8_t mmmm;
	uint8_t vvvv;
	uint8_t l;
	uint8_t w;
	uint8_t pp;
};

enum X86OpSize
{
	X86_OPSIZEUNKNOWN = -1,
	X86_OPSIZE16 = 0,
	X86_OPSIZE32 = 1,
	X86_OPSIZE64 = 2,
};

enum X86AddrSize
{
	X86_ADDRSIZEUNKNOWN = -1,
	X86_ADDRSIZE16 = 0,
	X86_ADDRSIZE32 = 1,
	X86_ADDRSIZE64 = 2,
};

enum X86_Optype
{
	X86_OPTYPE_EMPTY = 0,
	X86_OPTYPE_IMM = 1,
	X86_OPTYPE_REG = 2,
	X86_OPTYPE_SEG = 3,
	X86_OPTYPE_MEM = 4,
	X86_OPTYPE_CRX = 5,
	X86_OPTYPE_DRX = 6,
	X86_OPTYPE_STX = 7,
	X86_OPTYPE_MMX = 8,
	X86_OPTYPE_XMM = 9,
	X86_OPTYPE_YMM = 10,
	X86_OPTYPE_FARPTR = 11,

	// user defined types start here
	X86_OPTYPE_USER = 32,
};

struct x86_insn_op
{
	X86_Optype type;
	int size;
	bool need_rex;
	bool forbid_rex;
	union
	{
		// 		struct
		// 		{
		// 			uint32_t seg;
		// 			uint32_t offset;
		// 		} farptr;
		uint64_t imm;
		int reg;
		int seg;
		struct
		{
			uint64_t disp;
			int base;
			int index;
			int scale;
			int addrsize;
			bool floatptr;
			bool addrptr;
			bool hasdisp;
		} mem;
		int crx;
		int drx;
		int trx;
		int stx;
		int mmx;
		int xmm;
		int ymm;
		union
		{
			int i;
			void *p;
		} user[4];
	};
};

struct x86dis_insn
{
	bool invalid;
	int8_t opsizeprefix;
	int8_t lockprefix;
	int8_t repprefix;
	int8_t segprefix;
	uint8_t rexprefix;
	x86dis_vex vexprefix;
	int size;
	int opcode;
	int opcodeclass;
	X86OpSize eopsize;
	X86AddrSize eaddrsize;
	bool ambiguous;
	const char *name;
	x86_insn_op op[5];
};

enum AsmSyntaxHighlightEnum
{
	e_cs_default = 0,
	e_cs_comment,
	e_cs_number,
	e_cs_symbol,
	e_cs_string
};

struct x86opc_insn
{
	const char *name;
	uint8_t op[4];
};

struct x86opc_vex_insn
{
	const char *name;
	uint8_t vex;
	uint8_t op[5];
};

struct x86opc_insn_op
{
	uint8_t type;
	uint8_t extra;
	uint8_t info;
	uint8_t size;
};

/* generic disassembler styles */
#define DIS_STYLE_HIGHLIGHT		0x80000000		/* create highlighting information in strf() */
#define DIS_STYLE_HEX_CSTYLE		0x40000000		/* IF SET: mov eax, 0x12345678 		ELSE: mov eax, 12345678 */
#define DIS_STYLE_HEX_ASMSTYLE		0x20000000		/* IF SET: mov eax, 12345678h 		ELSE: mov eax, 12345678 */
#define DIS_STYLE_HEX_UPPERCASE		0x10000000		/* IF SET: mov eax, 5678ABCD	 	ELSE: mov eax, 5678abcd */
#define DIS_STYLE_HEX_NOZEROPAD		0x08000000		/* IF SET: mov eax, 8002344	 	ELSE: mov eax, 008002344 */
#define DIS_STYLE_SIGNED		0x04000000		/* IF SET: mov eax, -1	 		ELSE: mov eax, 0ffffffffh */

#define DIS_STYLE_TABSIZE			1

/* x86-specific styles */
#define X86DIS_STYLE_EXPLICIT_MEMSIZE	0x00000001	/* IF SET: mov word ptr [0000], ax 	ELSE: mov [0000], ax */
#define X86DIS_STYLE_OPTIMIZE_ADDR	0x00000002	/* IF SET: mov [eax*3], ax 		ELSE: mov [eax+eax*2+00000000], ax */


class x64dis
{
public:
	x64dis();
	x86dis_insn* decode(uint8_t *code, int maxlen, uint64_t addr);
	const char *str(x86dis_insn* disasm_insn, int options);
	const char *strf(x86dis_insn* disasm_insn, int options, const char *format);
private:
	void checkInfo(x86opc_insn *xinsn);
	void decode_modrm(x86_insn_op *op, char size, bool allow_reg,
		bool allow_mem, bool mmx, bool xmm, bool ymm);
	void prefixes();
	void prepInsns();
	void decode_insn(x86opc_insn *xinsn);
	void decode_vex_insn(x86opc_vex_insn *xinsn);
	void decode_op(x86_insn_op *op, x86opc_insn_op *xop);
	void decode_sib(x86_insn_op *op, int mod);
	int esizeop(uint32_t c);
	int esizeop_ex(uint32_t c);
	uint8_t getbyte();
	uint16_t getword();
	uint32_t getdword();
	uint64_t getqword();
	int getmodrm();
	int getsib();
	int getdrex();
	uint32_t getdisp();
	int getspecialimm();
	void invalidate();
	bool isfloat(char c);
	bool isaddr(char c);
	void str_format(char **str, const char **format, char *p, char *n,
		char *op[5], int oplen[5], char stopchar, int print);
	void str_op(char *opstr, int *opstrlen, x86dis_insn *insn,
		x86_insn_op *op, bool explicit_params);
	uint32_t mkmod(uint32_t modrm);
	uint32_t mkreg(uint32_t modrm);
	uint32_t mkindex(uint32_t modrm);
	uint32_t mkrm(uint32_t modrm);

	void getOpcodeMetrics(int &min_length, int &max_length,
		int &min_look_ahead, int &avg_look_ahead, int &addr_align);
	uint8_t getSize(void *disasm_insn);
	bool validInsn(x86dis_insn* disasm_insn);

	const char *get_cs(AsmSyntaxHighlightEnum style);
	void hexd(char **s, int size, int options, uint32_t imm);
public:
	void hexq(char **s, int size, int options, uint64_t imm);
	void enable_highlighting();
	void disable_highlighting();


	char* (*addr_sym_func)(uint64_t addr, int *symstrlen, void *context) = nullptr;
	void* addr_sym_func_context = nullptr;
	static x86opc_insn(*x86_64_insns)[256];
	X86OpSize opsize;
	X86AddrSize addrsize;
	x86opc_insn(*x86_insns)[256];
	x86dis_insn insn;
	char insnstr[256];
	uint8_t *codep, *ocodep;
	uint64_t addr;
	uint8_t c;
	int modrm;
	int sib;
	int drex;
	int maxlen;
	int special_imm;
	uint32_t disp;
	bool have_disp;
	bool fixdisp;
	int options;
	bool highlight;
};

