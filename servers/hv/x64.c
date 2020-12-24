#include "x64.h"
#include <resea/printf.h>

static enum x64_reg decode_rm(uint8_t rm) {
    switch (rm) {
        case 0b000:
            return X64_REG_EAX;
        case 0b001:
            return X64_REG_ECX;
        case 0b010:
            return X64_REG_EDX;
        case 0b011:
            return X64_REG_EBX;
        case 0b110:
            return X64_REG_ESI;
        case 0b111:
            return X64_REG_EDI;
        default:
            NYI();
    }
}

/// Decodes a memory access instruction.
error_t decode_inst(uint8_t *inst, int inst_len, enum x64_op *op,
                    enum x64_reg *reg, bool *reg_to_mem, int *size) {
    // TRACE("decode_inst: inst=[%02x, %02x, %02x, %02x, %02x, ...], len=%d",
    //       inst[0], inst[1], inst[2], inst[3], inst[4], inst_len);

    if (inst_len < 2) {
        WARN_DBG("too short instruction");
        return ERR_INVALID_ARG;
    }

    uint8_t modrm = inst[1];
    switch (inst[0]) {
        // MOV r32,r/m32
        case 0x8b: {
            *op = X64_OP_MOV;
            *reg = decode_rm((modrm >> 3) & 0b111);
            *reg_to_mem = false;
            *size = 4;
            break;
        }
        // MOV r/m32,r32
        case 0x89: {
            *op = X64_OP_MOV;
            *reg = decode_rm((modrm >> 3) & 0b111);
            *reg_to_mem = true;
            *size = 4;
            break;
        }
        default:
            WARN_DBG("unsupported opcode: 0x%02x", inst[0]);
            return ERR_NOT_ACCEPTABLE;
    }

    return OK;
}
