#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

/* Sign extends the given field to a 32-bit integer where field is
 * interpreted an n-bit integer. */ 
int sign_extend_number(unsigned int field, unsigned int n) {
    int sign_bit = field & (1 << (n - 1));
    int result;

    if (sign_bit) {
        result = (-1 ^ (sign_bit - 1)) | field;
    } else {
        result = field;
    }
    return result;
}

/* Unpacks the 32-bit machine code instruction given into the correct
 * type within the instruction struct */ 
Instruction parse_instruction(uint32_t instruction_bits) {
    Instruction instruction;
    instruction.bits = instruction_bits;
    return instruction;
}

/* Return the number of bytes (from the current PC) to the branch label using the given
 * branch instruction */
int get_branch_offset(Instruction instruction) {
    int imm5 = instruction.sbtype.imm5;
    int imm7 = instruction.sbtype.imm7;
    int offset = 0;
    offset |= ((imm7 & 0x40) << 5) | ((imm5 & 1) << 10) | ((imm7 & 0x3F) << 4) | ((imm5 & 0x1E) >> 1);
    offset = sign_extend_number(offset << 1, 13);
    return offset; 
}

/* Returns the number of bytes (from the current PC) to the jump label using the given
 * jump instruction */
int get_jump_offset(Instruction instruction) {
    int imm = instruction.ujtype.imm;
    int offset = 0;
    offset |= (imm & (1 << 19)) | ((imm & 0xFF) << 11) | ((imm & 0x100) << 2) | ((imm & 0x7FE00) >> 9);
    offset = sign_extend_number(offset << 1, 21);
    return offset;
}

int get_store_offset(Instruction instruction) {
    int imm5 = instruction.stype.imm5;
    int imm7 = instruction.stype.imm7;
    return sign_extend_number((imm7 << 5) | imm5, 12);
}

void handle_invalid_instruction(Instruction instruction) {
    printf("Invalid Instruction: 0x%08x\n", instruction.bits); 
}

void handle_invalid_read(Address address) {
    printf("Bad Read. Address: 0x%08x\n", address);
    exit(-1);
}

void handle_invalid_write(Address address) {
    printf("Bad Write. Address: 0x%08x\n", address);
    exit(-1);
}

