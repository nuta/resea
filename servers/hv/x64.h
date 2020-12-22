#ifndef __X64_H__
#define __X64_H__

#include <types.h>

enum x64_op {
    X64_OP_MOV,
};

enum x64_reg {
    X64_REG_EAX,
    X64_REG_ECX,
    X64_REG_EDX,
    X64_REG_EBX,
    X64_REG_ESI,
    X64_REG_EDI,
};

error_t decode_inst(uint8_t *inst, int inst_len, enum x64_op *op,
                    enum x64_reg *reg, bool *reg_to_mem, int *size);

#endif
