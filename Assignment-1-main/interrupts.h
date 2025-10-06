#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TIME_TO_SWITCH_KERNEL_MODE 1
#define TIME_TO_SAVE_CONTEXT 10
#define TIME_TO_FIND_VECTOR 1
#define TIME_TO_LOAD_ADDRESS 1

// define VectorTable struct containing vectors and their addresses
typedef struct
{
    int address;
    int vector;
} VectorTable;

void createVectorTable(VectorTable vectorTable[], FILE *vectorTableFile, int lineCount);

void removeCommas(const char *input, char *output);

char *getExecutionTime(char *input);

void handleCPU(FILE *file, int *currentTime, int executionTime);

void handleSysCall(FILE *file, int *currentTime, int executionTime, char *filteredOperation, VectorTable vectorTable[]);

void handleEndIO(FILE *file, int *currentTime, int executionTime, char *filteredOperation, VectorTable vectorTable[]);

#endif