#include "generate.h"

#include "../chunk.h"

#include <assert.h>

#define TO_STACK_SLOT(n) ((n) << 4)

// Instruction bit masks with parameters set to zero

#define LDR_UNSIGNED_OFFSET 0xF9400000

#define LDP_SIGNED_OFFSET 0xA9400000

#define STP_SIGNED_OFFSET 0xA9000000
#define STP_PRE_INDEX 0xA9800000

#define MOV_IMMEDIATE 0xD2800000

#define ADD_IMMEDIATE 0x91000000
#define SUB_IMMEDIATE 0xD1000000

#define BRANCH 0x14000000
#define BRANCH_CONDITION 0x54000000
#define BRANCH_REGISTER 0xD61F0000
#define BRANCH_LINK_REGISTER 0xD63F0000

#define CLOSURE_REGISTER 19
#define CONSTANT_REGISTER 20
#define OP_TABLE_REGISTER 21

#define FP 29
#define SP 31
#define XZR 31

// Condition codes for conditional branching
enum Condition {
    COND_EQUAL = 0x0,
    COND_NOT_EQUAL = 0x1,
    COND_CARRY_SET = 0x2,
    COND_CARRY_CLEAR = 0x3,

    COND_GREATER_EQUAL = 0xa,
    COND_GREATER = 0xc,

    COND_LESS_EQUAL = 0xd,
    COND_LESS = 0xb,
};

// Operation construction helpers

static inline
uint32_t ldr_signed_offset(uint32_t rt, uint32_t rn, uint32_t offset)
{
    return LDR_UNSIGNED_OFFSET | rt | (rn << 5) | ((offset & 0xFFF) << 10);
}

static inline
uint32_t ldp_signed_offset(uint32_t rt1, uint32_t rt2, uint32_t rn, uint32_t offset)
{
    return LDP_SIGNED_OFFSET | rt1 | (rt2 << 10) | (rn << 5) | ((offset & 0x7F) << 15);
}

static inline
uint32_t stp_signed_offset(uint32_t rt1, uint32_t rt2, uint32_t rn, uint32_t offset)
{
    return STP_SIGNED_OFFSET | rt1 | (rt2 << 10) | (rn << 5) | ((offset & 0x7F) << 15);
}

static inline
uint32_t stp_pre_index(uint32_t rt1, uint32_t rt2, uint32_t rn, uint32_t offset)
{
    return STP_PRE_INDEX | rt1 | (rt2 << 10) | (rn << 5) | ((offset & 0x7F) << 15);
}

static inline
uint32_t mov_immediate(uint32_t rd, uint32_t immediate)
{
    return MOV_IMMEDIATE | rd | ((immediate & 0xFFFF) << 5);
}

static inline
uint32_t add_immediate(uint32_t rn, uint32_t rd, uint32_t immediate)
{
    return ADD_IMMEDIATE | (rn << 5) | rd | ((immediate & 0xFFF) << 10);
}

static inline
uint32_t sub_immediate(uint32_t rn, uint32_t rd, uint32_t immediate)
{
    return SUB_IMMEDIATE | (rn << 5) | rd | ((immediate & 0xFFF) << 10);
}

static inline
uint32_t branch(uint32_t offset)
{
    return BRANCH | (offset & 0x3FFFFFF);
}

static inline
uint32_t branch_conditional(uint32_t condition, uint32_t immediate)
{
    return BRANCH_CONDITION | condition | ((immediate & 0x7FFF) << 5);
}

static inline 
uint32_t branch_register(uint32_t rn)
{
    return BRANCH_REGISTER | (rn << 5);
}

static inline
uint32_t branch_link_register(uint32_t rn)
{
    return BRANCH_LINK_REGISTER | (rn << 5);
}

#define WRITE(op) writeChunk(chunk, op, line);

GenResult emitGetConstant(Chunk *chunk, int line, uint32_t index)
{
    if (index < 32) {
        WRITE(ldp_signed_offset(0, 1, CONSTANT_REGISTER, index << 1));
        WRITE(stp_pre_index(0, 1, SP, -2));
    } else { // Assumes index is below 256
        WRITE(add_immediate(CONSTANT_REGISTER, 0, TO_STACK_SLOT(index)));
        WRITE(ldp_signed_offset(0, 1, 0, 0));
        WRITE(stp_pre_index(0, 1, SP, -2));
    }
    return GEN_OK;
}

GenResult emitGetArgument(Chunk *chunk, int line, uint32_t argCount, uint32_t argument)
{
    assert(argument <= 255);
    assert(argument <= argCount);
    uint32_t index = argCount - argument - 1;
    // Since the frame pointer points to the frame that stores old fp and lr, 
    // we have to add an extra slot offset to the address.
    if (index <= 30) {
        WRITE(ldp_signed_offset(0, 1, FP, (index + 1) * 2));
        WRITE(stp_pre_index(0, 1, SP, -2));
    } else {
        WRITE(add_immediate(FP, 0, TO_STACK_SLOT(index)));
        WRITE(ldp_signed_offset(0, 1, 0, 2));
        WRITE(stp_pre_index(0, 1, SP, -2));
    }
    return GEN_OK;
}

GenResult emitSetArgument(Chunk *chunk, int line, uint32_t argCount, uint32_t argument)
{
    assert(argument <= 255);
    assert(argument <= argCount);
    uint32_t index = argCount - argument - 1;
    // Since the frame pointer points to the frame that stores old fp and lr, 
    // we have to add an extra slot offset to the address.
    WRITE(ldp_signed_offset(0, 1, SP, 0));
    if (index <= 30) {
        WRITE(stp_signed_offset(0, 1, FP, (index + 1) * 2));
    } else {
        WRITE(add_immediate(FP, 0, TO_STACK_SLOT(index)));
        WRITE(stp_signed_offset(0, 1, 0, 2));
    }
    return GEN_OK;
}

GenResult emitGetLocal(Chunk *chunk, int line, uint32_t localCount, uint32_t local)
{
    assert(local <= 255);
    assert(local <= localCount);
    uint32_t index = localCount - local - 1;

    if (index <= 29) {
        WRITE(ldp_signed_offset(0, 1, FP, (-index - 3) * 2));
        WRITE(stp_pre_index(0, 1, SP, -2));
    } else {
        WRITE(sub_immediate(FP, 0, TO_STACK_SLOT(index)));
        WRITE(ldp_signed_offset(0, 1, 0, -3 * 2));
        WRITE(stp_pre_index(0, 1, SP, -2));
    }
    return GEN_OK;
}

GenResult emitSetLocal(Chunk *chunk, int line, uint32_t localCount, uint32_t local)
{
    assert(local <= 255);
    assert(local <= localCount);
    uint32_t index = localCount - local - 1;

    WRITE(ldp_signed_offset(0, 1, SP, 0))
    if (index <= 29) {
        WRITE(stp_signed_offset(0, 1, FP, (-index - 3) * 2));
    } else {
        WRITE(sub_immediate(FP, 0, TO_STACK_SLOT(index)));
        WRITE(stp_signed_offset(0, 1, FP, -3 * 2));
    }
    return GEN_OK;
}

GenResult emitDefineGlobal(Chunk *chunk, int line, uint32_t op, uint32_t nameIndex)
{
    WRITE(ldp_signed_offset(0, 1, SP, 0));
    WRITE(ldr_signed_offset(2, CONSTANT_REGISTER, (nameIndex << 1) + 1));
    WRITE(ldr_signed_offset(16, OP_TABLE_REGISTER, op));
    WRITE(branch_link_register(16));
    return GEN_OK;
}

GenResult emitGetGlobal(Chunk *chunk, int line, uint32_t op, uint32_t nameIndex)
{
    WRITE(ldr_signed_offset(0, CONSTANT_REGISTER, (nameIndex << 1) + 1));
    WRITE(ldr_signed_offset(16, OP_TABLE_REGISTER, op));
    WRITE(branch_link_register(16));
    return GEN_OK;
}

GenResult emitSetGlobal(Chunk *chunk, int line, uint32_t op, uint32_t nameIndex)
{
    WRITE(ldp_signed_offset(0, 1, SP, 0));
    WRITE(ldr_signed_offset(2, CONSTANT_REGISTER, (nameIndex << 1) + 1));
    WRITE(ldr_signed_offset(16, OP_TABLE_REGISTER, op));
    WRITE(branch_link_register(16));
    return GEN_OK;
}

GenResult emitNil(Chunk *chunk, int line)
{
    WRITE(stp_pre_index(XZR, XZR, SP, -2));
    return GEN_OK;
}

GenResult emitFalse(Chunk *chunk, int line)
{
    WRITE(mov_immediate(15, 1));
    WRITE(stp_pre_index(15, XZR, SP, -2));
    return GEN_OK;
}

GenResult emitTrue(Chunk *chunk, int line)
{
    WRITE(mov_immediate(15, 1));
    WRITE(stp_pre_index(15, 15, SP, -2));
    return GEN_OK;
}

GenResult emitPop(Chunk *chunk, int line, uint32_t n)
{
    WRITE(add_immediate(SP, SP, TO_STACK_SLOT(n)));
    return GEN_OK;
}

// This will use a lookup table for ops, so that functions are movable.
// TODO: Compare with fixed calls to binary handlers directly.
// In a hot loop, a fixed call should have better pipelining, and thus be faster.
GenResult emitBinary(Chunk *chunk, int line, uint32_t op)
{
    WRITE(ldr_signed_offset(16, OP_TABLE_REGISTER, op));
    WRITE(branch_link_register(16));
    return GEN_OK;
}

GenResult emitPrint(Chunk *chunk, int line, uint32_t op)
{
    // Since this value was just calculated, it may be in x0 and x1 already.
    // We may need to have the constant operations load from the stack to rely on this assertion.
    WRITE(ldp_signed_offset(0, 1, SP, 0));
    WRITE(ldr_signed_offset(16, OP_TABLE_REGISTER, op));
    WRITE(branch_link_register(16));
    return GEN_OK;
}

GenResult emitLoop(Chunk *chunk, int line, int start)
{
    int32_t offset = start - chunk->count;
    WRITE(branch(offset));
    return GEN_OK;
}

/// This must emit only a single jump instruction, to be patched later.
int emitConditionalJump(Chunk *chunk, int line)
{
    WRITE(ldp_signed_offset(0, 1, SP, 0));
    WRITE(0x7100001F); // cmp w0, #0
    WRITE(0x54000060); // b.eq <condition>
    WRITE(0x7100041F); // cmp w0, #1
    WRITE(0x7A400820); // ccmp w1, #0, #0, eq
    // condition:
    int offset = chunk->count;
    WRITE(branch_conditional(COND_EQUAL, 0));
    return offset;
}

GenResult patchConditionalJump(Chunk *chunk, int line, int jump, int current)
{
    int32_t offset = current - jump;
    if (offset >= 1 << 26) {
        return GEN_BRANCH_OVERFLOW;
    }
    chunk->code[jump] = branch_conditional(COND_EQUAL, offset);
    return GEN_OK;
}

// TODO: If changing value type to single bits, optimize this to use AND before a ccmp.
GenResult emitCondition(Chunk *chunk, int line)
{
    WRITE(0x7100001F); // cmp  w0, #0
    WRITE(0x54000060); // b.eq end
    WRITE(0x7100041f); // cmp  w0, #1
    WRITE(0x7a400820); // ccmp w1, #0, #0, eq
    return GEN_OK;
}

GenResult emitConditionFromBool(Chunk *chunk, int line)
{
    WRITE(0x7100003F); // cmp  w1, #0
    return GEN_OK;
}

GenResult emitReturn(Chunk *chunk, int line, uint32_t op)
{
    WRITE(ldr_signed_offset(16, OP_TABLE_REGISTER, op));
    WRITE(branch_register(16));
    return GEN_OK;
}

// TODO: emitReturnFromInitializer
GenResult emitReturnFromInitializer(Chunk *chunk, int line, uint32_t op)
{
    
    return GEN_IMMEDIATE_OVERFLOW;
}

GenResult emitReturnFromScript(Chunk *chunk, int line)
{
    WRITE(0xA97E53B3); // ldp x19, x20, [x29, #-32]
    WRITE(0xA97F5BB5); // ldp x21, x22, [x29, #-16]
    WRITE(0xd2800000); // mov x0, #0 ; return code 0
    WRITE(0x910043bf); // add sp, x29, #16
    WRITE(0xA9407bbd); // ldp x29, x30, [x29] ; reload frame pointer
    WRITE(0xd65f03c0); // ret
    return GEN_OK;
}
