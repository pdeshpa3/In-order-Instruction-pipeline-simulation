/*
 * main.c
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include "apex_cpu.h"

int main(int argc, char const *argv[])
{
    APEX_CPU *cpu;

    fprintf(stderr, "APEX CPU Pipeline Simulator v%0.1lf\n", VERSION);

    if (argc != 2)
    {
        fprintf(stderr, "APEX_Help: Usage %s <input_file>\n", argv[0]);
        exit(1);
    }

    cpu = APEX_cpu_init(argv[1]);
    if (!cpu)
    {
        fprintf(stderr, "APEX_Error: Unable to initialize CPU\n");
        exit(1);
    }
    int x, y;
    while (1)
    {
        printf("\nSelect an option:");
        printf("\nInitialize: 1");
        printf("\nSimulate <n>: 2");
        printf("\nSingle Step: 3");
        printf("\nDisplay: 4");
        printf("\nShow Mem : 5");
        printf("\nBreak : 6\n");
        scanf("%d", &x);
        switch (x)
        {
        case 1:
        {
            printf("\nInitialized:");
            APEX_cpu_run(cpu, x, 0);
            break;
        }
        case 2:
        {
            printf("\nEnter number of steps to simulate:");
            scanf("%d", &y);
            APEX_cpu_run(cpu, x, y);
            break;
        }
        case 3:
        case 4:
        {
            APEX_cpu_run(cpu, x, y);
            break;
        }
        case 5:
        {
            printf("\nEnter location to get memory:\n");
            scanf("%d", &y);
            APEX_cpu_run(cpu, x, y);
            break;
        }

        default:
        {
            printf("\nExit : ");
            return 0;
        }
        }
    }
    APEX_cpu_stop(cpu);
    return 0;
}