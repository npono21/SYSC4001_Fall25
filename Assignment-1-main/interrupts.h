#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TIME_TO_SWITCH_KERNEL_MODE 1
#define TIME_TO_SAVE_CONTEXT 10
#define TIME_TO_FIND_VECTOR 25
#define TIME_TO_LOAD_ADDRESS 1

// Defining our VectorTable struct which will contain all Vector Table addresses we need
typedef struct
{
    int address;
    int vector;
} VectorTable;

// Reads our defined Vector Table address and write them into the VectorTable struct
void createVectorTable(VectorTable vectorTable[], FILE *vectorTableFile, int lineCount);

// This function will take the operation from trace.txt, ex. 'CPU,', and remove the commma for better processing later on
void removeCommas(const char *input, char *output);

char *getExecutionTime(char *input);

// Function to handle CPU operations
void handleCPU(FILE *file, int *currentTime, int executionTime);

// Function to handle SYSCALL operations
void handleSysCall(FILE *file, int *currentTime, int executionTime, char *filteredOperation, VectorTable vectorTable[]);

// Function to handle END_IO operations
void handleEndIO(FILE *file, int *currentTime, int executionTime, char *filteredOperation, VectorTable vectorTable[]);

#endif