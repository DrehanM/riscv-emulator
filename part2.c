#include <stdio.h> // for stderr
#include <stdlib.h> // for exit()
#include "types.h"
#include "utils.h"
#include "riscv.h"

void execute_rtype(Instruction, Processor *);
void execute_itype_except_load(Instruction, Processor *);
void execute_branch(Instruction, Processor *);
void execute_jal(Instruction, Processor *);
void execute_load(Instruction, Processor *, Byte *);
void execute_store(Instruction, Processor *, Byte *);
void execute_ecall(Processor *, Byte *);
void execute_lui(Instruction, Processor *);

void execute_instruction(uint32_t instruction_bits, Processor *processor, Byte *memory) {    
    Instruction instruction = parse_instruction(instruction_bits);
    processor->PC += 4;
    switch(instruction.opcode) {
        case 0x33:
            execute_rtype(instruction, processor);
            break;
        case 0x13:
            execute_itype_except_load(instruction, processor);
            break;
        case 0x73:
            execute_ecall(processor, memory);
            break;
        case 0x63:
            execute_branch(instruction, processor);
            break;
        case 0x6F:
            execute_jal(instruction, processor);
            break;
        case 0x23:
            execute_store(instruction, processor, memory);
            break;
        case 0x03:
            execute_load(instruction, processor, memory);
            break;
        case 0x37:
            execute_lui(instruction, processor);
            break;
        default: // undefined opcode
            handle_invalid_instruction(instruction);
            exit(-1);
            break;
    }
}

void execute_rtype(Instruction instruction, Processor *processor) {
    int rd = instruction.rtype.rd;
    int rs1 = processor->R[instruction.rtype.rs1];
    int rs2 = processor->R[instruction.rtype.rs2];
    switch (instruction.rtype.funct3){
        case 0x0:
            switch (instruction.rtype.funct7) {
                case 0x0:
                    // ADD
                    processor->R[rd] = rs1 + rs2;
                    break;
                case 0x1:
                    // MUL
                    processor->R[rd] = rs1 * rs2;
                    break;
                case 0x20:
                    // SUB
                    processor->R[rd] = rs1 - rs2;
                    break;
                default:
                    handle_invalid_instruction(instruction);
                    exit(-1);
                    break;
            }
            break;
        case 0x1:
            switch (instruction.rtype.funct7) {
                case 0x0:
                    // SLL
                    processor->R[rd] = rs1 << rs2;
                    break;
                case 0x1:
                    processor->R[rd] = (Word) ((Double) ((rs1 * rs2) & 0xFFFFFFFF00000000) >> 32);
                    break;
            }
            break;
        case 0x2:
            // SLT
            processor->R[rd] = rs1 < rs2;
            break;
        case 0x4:
            switch (instruction.rtype.funct7) {
                case 0x0:
                    // XOR
                    processor->R[rd] = rs1 ^ rs2;
                    break;
                case 0x1:
                    // DIV
                    processor->R[rd] = rs1 / rs2;
                    break;
                default:
                    handle_invalid_instruction(instruction);
                    exit(-1);
                    break;
            }
            break;
        case 0x5:
            switch (instruction.rtype.funct7) {
                case 0x0:
                    // SRL      
                    processor->R[rd] = rs1 >> rs2;
                    break;
                case 0x20:
                    // SRA
                    processor->R[rd] = (Word) ((sWord) rs1 >> rs2); //possibly will change rd to signed word
                    break;
                default:
                    handle_invalid_instruction(instruction);
                    exit(-1);
                break;
            }
            break;
        case 0x6:
            switch (instruction.rtype.funct7) {
                case 0x0:
                    // OR
                     processor->R[rd] = rs1 | rs2;
                    break;
                case 0x1:
                    // REM
                    processor->R[rd] = rs1 % rs2;
                    break;
                default:
                    handle_invalid_instruction(instruction);
                    exit(-1);
                    break;
            }
            break;
        case 0x7:
            // AND
            processor->R[rd] = rs1 & rs2;
            break;
        default:
            handle_invalid_instruction(instruction);
            exit(-1);
            break;
    }
}

void execute_itype_except_load(Instruction instruction, Processor *processor) {
    int rd = instruction.itype.rd;
    int rs1 = processor->R[instruction.itype.rs1];
    int imm = sign_extend_number(instruction.itype.imm, 12);
    int shift0p;
    switch (instruction.itype.funct3) {
        case 0x0:
            // ADDI
            processor->R[rd] = rs1 + imm;
            break;
        case 0x1:
            // SLLI
            processor->R[rd] = rs1 << imm;
            break;
        case 0x2:
            // SLTI
            processor->R[rd] = rs1 < imm;
            break;
        case 0x4:
            // XORI
            processor->R[rd] = rs1 ^ imm;
            break;
        case 0x5:
            shift0p = instruction.itype.imm >> 10;
            switch (shift0p) {
                case 0x0:
                    //SRLI
                    processor->R[rd] = rs1 >> (instruction.itype.imm & 0x1F);
                    break;
                case 0x1:
                    //SRAI
                    processor->R[rd] = (Word) ((sWord) rs1 >> (instruction.itype.imm & 0x1F));
                    break;
                default:
                handle_invalid_instruction(instruction);
                break;
            }
            break;
        case 0x6:
            // ORI
            processor->R[rd] = rs1 | imm;
            break;
        case 0x7:
            // ANDI
            processor->R[rd] = rs1 & imm;
            break;
        default:
            handle_invalid_instruction(instruction);
            break;
    }
}

void execute_ecall(Processor *p, Byte *memory) {
    Register i;
    
    // syscall number is given by a0 (x10)
    // argument is given by a1
    switch(p->R[10]) {
        case 1: // print an integer
            printf("%d",p->R[11]);
            break;
        case 4: // print a string
            for(i=p->R[11];i<MEMORY_SPACE && load(memory,i,LENGTH_BYTE);i++) {
                printf("%c",load(memory,i,LENGTH_BYTE));
            }
            break;
        case 10: // exit
            printf("exiting the simulator\n");
            exit(0);
            break;
        case 11: // print a character
            printf("%c",p->R[11]);
            break;
        default: // undefined ecall
            printf("Illegal ecall number %d\n", p->R[10]);
            exit(-1);
            break;
    }
}

void execute_branch(Instruction instruction, Processor *processor) {
    int rs1 = processor->R[instruction.sbtype.rs1];
    int rs2 = processor->R[instruction.sbtype.rs2];
    int offset = get_branch_offset(instruction);
    switch (instruction.sbtype.funct3) {
        case 0x0:
            // BEQ
            if (rs1 == rs2) {
                processor->PC += offset - 4;
            }
            break;
        case 0x1:
            // BNE
            if (rs1 != rs2) {
                processor->PC += offset - 4;
            }
            break;
        default:
            handle_invalid_instruction(instruction);
            exit(-1);
            break;
    }
}

void execute_load(Instruction instruction, Processor *processor, Byte *memory) {
    int rd = instruction.itype.rd;
    int rs1 = processor->R[instruction.itype.rs1];
    int offset = sign_extend_number(instruction.itype.imm, 12);
    switch (instruction.itype.funct3) {
        case 0x0:
            // LB
            processor->R[rd] = sign_extend_number(load(memory, rs1 + offset, LENGTH_BYTE), 8);
            break;
        case 0x1:
            // LH
            processor->R[rd] = sign_extend_number(load(memory, rs1 + offset, LENGTH_HALF_WORD), 16);
            break;
        case 0x2:
            // LW
            processor->R[rd] = load(memory, rs1 + offset, LENGTH_WORD);
            break;
        default:
            handle_invalid_instruction(instruction);
            break;
    }
}

void execute_store(Instruction instruction, Processor *processor, Byte *memory) {
    int rs1 = processor->R[instruction.stype.rs1];
    int rs2 = processor->R[instruction.stype.rs2];
    int offset = get_store_offset(instruction);
    switch (instruction.stype.funct3) {
        case 0x0:
            // SB
            store(memory, rs1 + offset, LENGTH_BYTE, rs2);
            break;
        case 0x1:
            // SH
            store(memory, rs1 + offset, LENGTH_HALF_WORD, rs2);
            break;
        case 0x2:
            // SW
            store(memory, rs1 + offset, LENGTH_WORD, rs2);
            break;
        default:
            handle_invalid_instruction(instruction);
            exit(-1);
            break;
    }
}

void execute_jal(Instruction instruction, Processor *processor) {
    processor->R[instruction.ujtype.rd] = processor->PC - 4;
    processor->PC += get_jump_offset(instruction) - 4;
}

void execute_lui(Instruction instruction, Processor *processor) {
    processor->R[instruction.utype.rd] = instruction.utype.imm << 12;
}

void store(Byte *memory, Address address, Alignment alignment, Word value) {
    for (int i = 0; i < alignment; i++) {
        memory[address + i] = (value >> (i * 8)) & 0xFF;
    }
}

Word load(Byte *memory, Address address, Alignment alignment) {
    Word result = 0;
    for (int i = alignment; i >= 0; i--) {
        result = (result << 8) + (Word) memory[address + i];
    }
    return result;
}
