/*
 * apex_cpu.c
 * Contains APEX cpu pipeline implementation
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apex_cpu.h"
#include "apex_macros.h"

/* Converts the PC(4000 series) into array index for code memory
 *
 * Note: You are not supposed to edit this function
 */
static int
get_code_memory_index_from_pc(const int pc)
{
    return (pc - 4000) / 4;
}

static void
print_instruction(const CPU_Stage *stage)
{
    switch (stage->opcode)
    {
    case OPCODE_STR:
    case OPCODE_ADD:
    case OPCODE_SUB:
    case OPCODE_MUL:
    case OPCODE_DIV:
    case OPCODE_AND:
    case OPCODE_OR:
    case OPCODE_XOR:
    case OPCODE_LDR:
    {
        printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
               stage->rs2);
        break;
    }

    case OPCODE_MOVC:
    {
        printf("%s,R%d,#%d ", stage->opcode_str, stage->rd, stage->imm);
        break;
    }

    case OPCODE_LOAD:
    case OPCODE_ADDL:
    case OPCODE_SUBL:
    {
        printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
               stage->imm);
        break;
    }

    case OPCODE_STORE:
    {
        printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rs1, stage->rs2,
               stage->imm);
        break;
    }

    case OPCODE_BZ:
    case OPCODE_BNZ:
    {
        printf("%s,#%d ", stage->opcode_str, stage->imm);
        break;
    }

    case OPCODE_CMP:
    {
        printf("%s R%d,R%d ", stage->opcode_str, stage->rs1, stage->rs2);
        break;
    }

    case OPCODE_HALT:
    {
        printf("%s", stage->opcode_str);
        break;
    }

    case OPCODE_NULL:
    {
        printf(" ");
        break;
    }
    case OPCODE_NOP:
        printf("%s",stage->opcode_str);
    }
}

/* Debug function which prints the CPU stage content
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_stage_content(const char *name, const CPU_Stage *stage)
{
    printf("%-15s: pc(%d) ", name, stage->pc);
    print_instruction(stage);
    printf("\n");
}

/* Debug function which prints the register file
 *
 * Note: You are not supposed to edit this function
 */
static void
print_reg_file(const APEX_CPU *cpu)
{
    int i;

    printf("----------\n%s\n----------\n", "Registers:");

    for (int i = 0; i < REG_FILE_SIZE / 2; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }

    printf("\n");

    for (i = (REG_FILE_SIZE / 2); i < REG_FILE_SIZE; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }

    printf("\n");
}

/*
 * Fetch Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_fetch(APEX_CPU *cpu)
{
    APEX_Instruction *current_ins;

    if (cpu->decode.stalled == 1)
    {
        cpu->fetch.stalled = 1;
    }
    else
    {
        cpu->fetch.stalled = 0;
    }

    if (cpu->fetch.flush == 1)
    {
        strcpy(cpu->fetch.opcode_str, " ");
        cpu->fetch.opcode = 0x0;
        cpu->fetch.pc = '\0';
        cpu->fetch.flush = 0;
        return;
    }

    /* This fetches new branch target instruction from next cycle */
    if (cpu->fetch_from_next_cycle == TRUE)
    {
        cpu->fetch_from_next_cycle = FALSE;

        /* Skip this cycle*/
        return;
    }

    /* Store current PC in fetch latch */
    cpu->fetch.pc = cpu->pc;

    /* Index into code memory using this pc and copy all instruction fields
         * into fetch latch  */
    current_ins = &cpu->code_memory[get_code_memory_index_from_pc(cpu->pc)];
    strcpy(cpu->fetch.opcode_str, current_ins->opcode_str);
    cpu->fetch.opcode = current_ins->opcode;
    cpu->fetch.rd = current_ins->rd;
    cpu->fetch.rs1 = current_ins->rs1;
    cpu->fetch.rs2 = current_ins->rs2;
    cpu->fetch.imm = current_ins->imm;
    switch(cpu->fetch.opcode) {
        case OPCODE_STR:
        case OPCODE_LDR:{
            cpu->fetch.op_controller =1;
            break;
        }
        case OPCODE_STORE:
        case OPCODE_LOAD:{
            cpu->fetch.op_controller =2;
            break;
        }
        case OPCODE_CMP:
        {
            cpu->fetch.op_controller =3;
            break;
        }
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_MUL:
        case OPCODE_DIV:
        case OPCODE_AND:
        case OPCODE_OR: 
        case OPCODE_XOR:
        {
            cpu->fetch.op_controller =4;
            break;
        } 
        case OPCODE_MOVC:{
            cpu->fetch.op_controller =5;
            break;
        }

        case OPCODE_SUBL:
        case OPCODE_ADDL:
        {
            cpu->fetch.op_controller =6;
            break;
        }
        
        default:
        {
            cpu->fetch.op_controller =7;
            break;
        }
    }


    
    if (cpu->fetch.opcode == 0)
    {
        strcpy(cpu->fetch.opcode_str, " ");
        cpu->fetch.pc = '\0';
    
    }

    if (cpu->fetch.has_insn && (!cpu->fetch.stalled))
    {
        /* Update PC for next instruction */
        cpu->pc += 4;

        /* Copy data from fetch latch to decode latch*/
        if (cpu->decode.stalled == 0)
        {
            cpu->decode = cpu->fetch;
        }
    }

    if (ENABLE_DEBUG_MESSAGES && cpu->fetch.opcode != OPCODE_NULL)
    {
        print_stage_content("Fetch", &cpu->fetch);
    }
}

/*
 * Decode Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_decode(APEX_CPU *cpu)
{

    if (cpu->decode.flush == 1)
    {
        strcpy(cpu->decode.opcode_str, " ");
        cpu->decode.opcode = 0x0;
        cpu->decode.pc = 0000;
        cpu->decode.flush = 0;
    }
    if (cpu->decode.stalled == 1)
    {
        cpu->execute.flush = 1;
    }
    else
    {
        cpu->execute.flush = 0;
    }

    if (cpu->decode.has_insn && (!cpu->decode.stalled))
    {
        
        int rs1 = FALSE;
        int rs2 = FALSE;
        int rd = FALSE;
        int mem_op_1 = FALSE;
        int mem_op_2 = FALSE;
        if(cpu->regs_valid[cpu->decode.rs1]) {rs1 = TRUE; cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];}
        if(cpu->regs_valid[cpu->decode.rs2]) {rs2 = TRUE; cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];}
        if(cpu->regs_valid[cpu->decode.rd]) {rd = TRUE; cpu->decode.rd_value = cpu->regs[cpu->decode.rd];}
        if(rs1&&rs2&rd){
            if (cpu->mem_valid[cpu->decode.rs1_value + cpu->decode.rs2_value])
                {
                    mem_op_1 = TRUE;
                }
        }
        if(rs1&&rs2){
            if (cpu->mem_valid[cpu->decode.rs1_value + cpu->decode.imm])
                {
                    mem_op_2 = TRUE;
                }
        }
        switch(cpu->decode.op_controller){
            case 1:{
                if(!mem_op_1){
                    cpu->decode.stalled = 1;
                }
                break;
            }
            case 2:{
                if(!mem_op_2){
                    cpu->decode.stalled = 1;
                }
                break;
            }
            case 3:{
                if(!(rs1&&rs2)){
                    cpu->decode.stalled = 1;
                }
                break;
            }
            case 4:{
                if(!(rs1&&rs2&&rd)){
                    cpu->decode.stalled = 1;
                }
                break;
            }
            case 5:{
                if(!rd){
                    cpu->decode.stalled = 1;
                }
                break;
            }
            case 6:{
                if(!(rd&&rs1)){
                    cpu->decode.stalled = 1;
                }
                break;
            }
            case 7:{
                break;
            }


        }
        if((cpu->decode.opcode == OPCODE_HALT)){
           
        }    
    }
    if (cpu->decode.stalled == 0)
    {
        cpu->execute = cpu->decode;
        cpu->decode.has_insn = FALSE;
    }

    if (ENABLE_DEBUG_MESSAGES && cpu->decode.opcode != OPCODE_NULL)
    {
        print_stage_content("Decode/RF", &cpu->decode);
    }
}

/*
 * Execute Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_execute(APEX_CPU *cpu)
{
    if (cpu->execute.flush == 1)
    {
        strcpy(cpu->execute.opcode_str, " ");
        cpu->execute.opcode = 0x0;
        cpu->execute.pc = 0000;
    }
    if (cpu->execute.has_insn)
    {
        /* Execute logic based on instruction type */
        switch (cpu->execute.opcode)
        {

        case OPCODE_STR:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value + cpu->execute.rs2_value;
            cpu->mem_valid[cpu->execute.result_buffer] = 0;
            break;
        }

        case OPCODE_STORE:
        {

            cpu->execute.result_buffer = (cpu->execute.rs2_value) + (cpu->execute.imm);
            cpu->mem_valid[cpu->execute.result_buffer] = 0;
            break;
        }

        case OPCODE_LDR:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value + cpu->execute.rs2_value;
            cpu->regs_valid[cpu->execute.rd] = 0;

            break;
        }
        case OPCODE_LOAD:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value + cpu->execute.imm;
            cpu->regs_valid[cpu->execute.rd] = 0;
            break;
        }

        case OPCODE_CMP:
        {

            cpu->execute.result_buffer = cpu->execute.rs1_value - cpu->execute.rs2_value;
            cpu->zero_flag = cpu->execute.result_buffer;
            break;
        }

        case OPCODE_ADD:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value + cpu->execute.rs2_value;
            cpu->regs_valid[cpu->execute.rd] = 0;

            break;
        }
        case OPCODE_SUB:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value - cpu->execute.rs2_value;
            cpu->regs_valid[cpu->execute.rd] = 0;

            break;
        }
        case OPCODE_MUL:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value * cpu->execute.rs2_value;
            cpu->regs_valid[cpu->execute.rd] = 0;

            break;
        }
        case OPCODE_DIV:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value / cpu->execute.rs2_value;
            cpu->regs_valid[cpu->execute.rd] = 0;

            break;
        }
        case OPCODE_AND:
        {
            cpu->execute.result_buffer = (cpu->execute.rs1_value) & (cpu->execute.rs2_value);
            cpu->regs_valid[cpu->execute.rd] = 0;

            break;
        }
        case OPCODE_OR:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value | cpu->execute.rs2_value;
            cpu->regs_valid[cpu->execute.rd] = 0;

            break;
        }
        case OPCODE_XOR:
        {
            cpu->execute.result_buffer = (cpu->execute.rs1_value) ^ (cpu->execute.rs2_value);
            cpu->regs_valid[cpu->execute.rd] = 0;

            break;
        }
        case OPCODE_ADDL:
        {
            cpu->execute.result_buffer = (cpu->execute.rs1_value) + (cpu->execute.imm);
            cpu->regs_valid[cpu->execute.rd] = 0;

            break;
        }
        case OPCODE_SUBL:
        {
            cpu->execute.result_buffer = (cpu->execute.rs1_value) - (cpu->execute.imm);
            cpu->regs_valid[cpu->execute.rd] = 0;

            break;
        }

        case OPCODE_BZ:
        {
            if (cpu->zero_flag == 0)
            {
                cpu->branch_taken = 1;
            }
            else
            {
                cpu->branch_taken = 0;
            }
            break;
        }

        case OPCODE_BNZ:
        {
            if (cpu->zero_flag != 0)
            {
                cpu->branch_taken = 1;
            }
            else
            {
                cpu->branch_taken = 0;
            }
            break;
        }

        case OPCODE_MOVC:
        {
            cpu->execute.result_buffer = cpu->execute.imm;

            cpu->regs_valid[cpu->execute.rd] = 0;
            break;
        }

        case OPCODE_HALT:
        {
            cpu->decode.flush = 1;
             cpu->fetch.flush = 1;
            break;
        }
        }

        /* Copy data from execute latch to memory latch*/
        cpu->memory = cpu->execute;
        cpu->execute.has_insn = FALSE;
        if (ENABLE_DEBUG_MESSAGES && cpu->execute.opcode != OPCODE_NULL)
        {
            print_stage_content("Execute", &cpu->execute);
        }
    }
}

/*
 * Memory Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_memory(APEX_CPU *cpu)
{
    if (cpu->memory.has_insn)
    {

        switch (cpu->memory.opcode)
        {
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_MUL:
        case OPCODE_DIV:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        case OPCODE_CMP:
        case OPCODE_MOVC:
        case OPCODE_ADDL:
        case OPCODE_SUBL:
        {

            /* No work for ADD */
            break;
        }

        case OPCODE_LOAD:
        case OPCODE_LDR:
        {
            cpu->memory.result_buffer = cpu->data_memory[cpu->memory.result_buffer];
            break;
        }

        case OPCODE_STORE:
        {
            cpu->data_memory[cpu->memory.result_buffer] = cpu->memory.rs1_value;
            cpu->mem_valid[cpu->memory.result_buffer] = 1;
            cpu->decode.stalled = 0;
            break;
        }
        case OPCODE_STR:
        {
            cpu->data_memory[cpu->memory.result_buffer] = cpu->memory.rd_value;
            cpu->mem_valid[cpu->memory.result_buffer] = 1;
            cpu->decode.stalled = 0;
            break;
        }
        case OPCODE_BZ:
        case OPCODE_BNZ:
        {
            if (cpu->branch_taken == 1)
            {

                /* Calculate new PC, and send it to fetch unit */
                cpu->pc = cpu->memory.pc + cpu->memory.imm;
                cpu->decode.has_insn = FALSE;
                cpu->fetch.has_insn = TRUE;
                cpu->execute.flush = 1;
                cpu->decode.flush = 1;
                cpu->branch_taken = 0;
                
            }
            break;
        }

        case OPCODE_HALT:
        {
            cpu->execute.flush = 1;
             cpu->fetch.flush = 1;
            break;
        }
        }

        /* Copy data from memory latch to writeback latch*/
        cpu->writeback = cpu->memory;
        cpu->memory.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES && cpu->memory.opcode != OPCODE_NULL)
        {
            print_stage_content("Memory", &cpu->memory);
        }
    }
}

/*
 * Writeback Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static int
APEX_writeback(APEX_CPU *cpu)
{
    if (cpu->writeback.has_insn)
    {
        /* Write result to register file based on instruction type */
        switch (cpu->writeback.opcode)
        {
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_MUL:
        case OPCODE_DIV:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        case OPCODE_ADDL:
        case OPCODE_SUBL:
        case OPCODE_LDR:
        case OPCODE_LOAD:
         case OPCODE_MOVC:
        {
            cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
            cpu->regs_valid[cpu->writeback.rd] = 1;

            break;
        }
        case OPCODE_HALT:
        {
            cpu->memory.flush = 1;
            break;
        }
        }
        cpu->decode.stalled = 0;
        cpu->insn_completed++;
        cpu->writeback.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES && cpu->writeback.opcode != OPCODE_NULL)
        {
            print_stage_content("Writeback", &cpu->writeback);
        }
        if (cpu->writeback.opcode == OPCODE_HALT)
        {
            /* Stop the APEX simulator */
            return TRUE;
        }
    }

    /* Default */
    return 0;
}

/*
 * This function creates and initializes APEX cpu.
 *
 * Note: You are free to edit this function according to your implementation
 */
APEX_CPU *
APEX_cpu_init(const char *filename)
{
    int i;
    APEX_CPU *cpu;

    if (!filename)
    {
        return NULL;
    }

    cpu = calloc(1, sizeof(APEX_CPU));

    if (!cpu)
    {
        return NULL;
    }

    /* Initialize PC, Registers and all pipeline stages */
    cpu->pc = 4000;
    memset(cpu->regs, 0, sizeof(int) * REG_FILE_SIZE);

    memset(cpu->data_memory, 0, sizeof(int) * DATA_MEMORY_SIZE);
    for (i = 0; i < 4096; i++)
    {
        cpu->mem_valid[i] = 1;
    }
    cpu->zero_flag = -9999;
    cpu->branch_taken = 0;
    
    for (i = 0; i < 16; i++)
    {
        cpu->regs_valid[i] = 1;
    }

    cpu->single_step = ENABLE_SINGLE_STEP;

    /* Parse input file and create code memory */
    cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);
    if (!cpu->code_memory)
    {
        free(cpu);
        return NULL;
    }

    if (ENABLE_DEBUG_MESSAGES)
    {
        fprintf(stderr,
                "APEX_CPU: Initialized APEX CPU, loaded %d instructions\n",
                cpu->code_memory_size);
        fprintf(stderr, "APEX_CPU: PC initialized to %d\n", cpu->pc);
        fprintf(stderr, "APEX_CPU: Printing Code Memory\n");
        printf("%-9s %-9s %-9s %-9s %-9s\n", "opcode_str", "rd", "rs1", "rs2",
               "imm");

        for (i = 0; i < cpu->code_memory_size; ++i)
        {
            printf("%-9s %-9d %-9d %-9d %-9d\n", cpu->code_memory[i].opcode_str,
                   cpu->code_memory[i].rd, cpu->code_memory[i].rs1,
                   cpu->code_memory[i].rs2, cpu->code_memory[i].imm);
        }
    }

    /* To start fetch stage */
    cpu->fetch.has_insn = TRUE;

    return cpu;
}

/*
 * APEX CPU simulation loop
 *
 * Note: You are free to edit this function according to your implementation
 */
void APEX_cpu_run(APEX_CPU *cpu, int x, int y)
{

    switch (x)
    {
    case 1:
    {
        APEX_run_at_choice(cpu, 0);
        break;
    }
    case 2:
    {
        int p = 0;
        while (p < y)
        {
            APEX_run_at_choice(cpu, 1);
            p++;
        }
        printf("\n-----------------ARCHITECTURAL REGISTER FILE-------------- \n");
        for (int i = 0; i < 16; i++)
        {
            printf("| R[%d] | Value=%d | \n", i, cpu->regs[i]);
        }
        printf("\n-----------------ARCHITECTURAL REGISTER FILE-------------- \n");

        printf("\n-----------------DATA MEMORY-------------- \n");
        for (int i = 0; i < 4096; i++)
        {
            if (cpu->data_memory[i] != 0)
            {

                printf("| MEM[%d] | Value=%d | \n", i, cpu->data_memory[i]);
            }
        }
        printf("-----------------DATA MEMORY-------------- \n");
        break;
    }
    case 3:
    {
        int breaker = 0;
        while (breaker != 1)
        {
            breaker = APEX_run_at_choice(cpu, 0);
        }
        printf("\n-----------------ARCHITECTURAL REGISTER FILE-------------- \n");
        for (int i = 0; i < 16; i++)
        {
            printf("| R[%d] | Value=%d | \n", i, cpu->regs[i]);
        }
        printf("\n-----------------ARCHITECTURAL REGISTER FILE-------------- \n");

        printf("\n-----------------DATA MEMORY-------------- \n");
        for (int i = 0; i < 4096; i++)
        {
            if (cpu->data_memory[i] != 0)
            {

                printf("| MEM[%d] | Value=%d | \n", i, cpu->data_memory[i]);
            }
        }
        printf("-----------------DATA MEMORY-------------- \n");
        break;
    }
    case 4:
    {
        int breaker = 0;
        while (breaker != 1)
        {
            breaker = APEX_run_at_choice(cpu, 1);
        }
        printf("\n-----------------ARCHITECTURAL REGISTER FILE-------------- \n");
        for (int i = 0; i < 16; i++)
        {
            printf("| R[%d] | Value=%d | \n", i, cpu->regs[i]);
        }
        printf("\n-----------------ARCHITECTURAL REGISTER FILE-------------- \n");

        printf("\n-----------------DATA MEMORY-------------- \n");
        for (int i = 0; i < 4096; i++)
        {
            if (cpu->data_memory[i] != 0)
            {

                printf("| MEM[%d] | Value=%d | \n", i, cpu->data_memory[i]);
            }
        }
        printf("-----------------DATA MEMORY-------------- \n");
        break;
    }
    case 5:
    {
        printf("MEM[%d] | Value=%d", y, cpu->data_memory[y]);
        break;
    }
    default:
    {
        return;
    }
    }
}

int APEX_run_at_choice(APEX_CPU *cpu, int z)
{
    int breaker = 0;
    char user_prompt_val;
    if (ENABLE_DEBUG_MESSAGES)
    {
        printf("--------------------------------------------\n");
        printf("Clock Cycle #: %d\n", cpu->clock + 1);
        printf("--------------------------------------------\n");
    }

    if (APEX_writeback(cpu))
    {
        /* Halt in writeback stage */
        printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock + 1, cpu->insn_completed);
        return 1;
    }

    APEX_memory(cpu);
    APEX_execute(cpu);
    APEX_decode(cpu);
    APEX_fetch(cpu);

    print_reg_file(cpu);

    if (cpu->single_step && z == 0)
    {
        printf("Press any key to advance CPU Clock or <q> to quit:\n");
        scanf("%c", &user_prompt_val);

        if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
        {
            printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
            breaker = 1;
        }
    }

    cpu->clock++;
    return breaker;
}

/*
 * This function deallocates APEX CPU.
 *
 * Note: You are free to edit this function according to your implementation
 */
void APEX_cpu_stop(APEX_CPU *cpu)
{
    free(cpu->code_memory);
    free(cpu);
}