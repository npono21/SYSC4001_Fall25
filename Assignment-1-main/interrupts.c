#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "interrupts.h"

void createVectorTable(VectorTable vectorTable[], FILE *vectorTableFile, int lineCount)
{
    rewind(vectorTableFile);
    int *vectors = malloc(lineCount * sizeof(int));
    if (vectors == NULL)
    {
        perror("Failed to allocate memory for vectors");
        exit(EXIT_FAILURE);
    }
    int readCount = 0;
    while (readCount < lineCount && fscanf(vectorTableFile, "%x", &vectors[readCount]) == 1)
    {
        readCount++;
    }

    int addresses[] = {
        0x0002, 0x0004, 0x0006, 0x0008, 0x000A,
        0x000C, 0x000E, 0x0010, 0x0012, 0x0014,
        0x0016, 0x0018, 0x001A, 0x001C, 0x001E,
        0x0020, 0x0022, 0x0024, 0x0026, 0x0028,
        0x002A, 0x002C, 0x002E, 0x0030, 0x0032};

    size_t addressCount = sizeof(addresses) / sizeof(addresses[0]);

    if (readCount != (int)addressCount)
    {
        fprintf(stderr,
                "Warning: Number of vectors read (%d) does not match number of addresses (%zu)\n",
                readCount, addressCount);
    }

    for (int i = 0; i < readCount && i < (int)addressCount; i++)
    {
        vectorTable[i].vector = vectors[i];
        vectorTable[i].address = addresses[i];
    }

    free(vectors);
}

void createDelayArray(int *delayArray, FILE *delayFile){
    int x = 1;
    while(fscanf(delayFile, "%d", delayArray + 1) != EOF){
        x++;
    }
}

void removeCommas(const char *input, char *output)
{
    int j = 0;
    for (int i = 0; input[i] != '\0'; i++)
    {
        if (input[i] != ',')
            output[j++] = input[i];
    }
    output[j] = '\0';
}

char *getExecutionTime(char *input)
{
    char *commaPosition = strchr(input, ',');
    if (commaPosition == NULL)
        return NULL;
    commaPosition++;
    char *output = malloc(strlen(commaPosition) + 1);
    if (output == NULL)
    {
        perror("Failed to allocate memory");
        return NULL;
    }
    strcpy(output, commaPosition);
    return output;
}

void handleCPU(FILE *file, int *currentTime, int executionTime)
{
    fprintf(file, "%d, %d, CPU execution\n", *currentTime, executionTime);
    *currentTime += executionTime;
}

void handleSysCall(FILE *file, int *currentTime, int *delayArray, char *filteredOperation, VectorTable vectorTable[])
{
    int number = 0;
    sscanf(filteredOperation, "SYSCALL %d", &number);
    fprintf(file, "%d, %d, switch to kernel mode\n", *currentTime, TIME_TO_SWITCH_KERNEL_MODE);
    *currentTime += TIME_TO_SWITCH_KERNEL_MODE;
    fprintf(file, "%d, %d, context saved\n", *currentTime, TIME_TO_SAVE_CONTEXT);
    *currentTime += TIME_TO_SAVE_CONTEXT;
    fprintf(file, "%d, %d, find vector %d in memory position 0x%04X\n", *currentTime, TIME_TO_FIND_VECTOR, number, vectorTable[number - 1].address);
    *currentTime += TIME_TO_FIND_VECTOR;
    fprintf(file, "%d, %d, load address 0x%04X into the PC\n", *currentTime, TIME_TO_LOAD_ADDRESS, vectorTable[number - 1].vector);
    *currentTime += TIME_TO_LOAD_ADDRESS;
    int executionTime = delayArray + number;
    int limit = executionTime / 2;
    int timeToSysCall = (rand() % (executionTime / 4 + 2)) + limit;
    int timeToTransferData = executionTime - timeToSysCall;

    fprintf(file, "%d, %d, SYSCALL: run the ISR\n", *currentTime, timeToSysCall);
    *currentTime += timeToSysCall;
    fprintf(file, "%d, %d, transfer data\n", *currentTime, timeToTransferData);
    *currentTime += timeToTransferData;
    fprintf(file, "%d, %d, context returned\n", *currentTime, TIME_TO_SAVE_CONTEXT);
    *currentTime += TIME_TO_SAVE_CONTEXT;
    fprintf(file, "%d, %d, switch to user mode\n", *currentTime, TIME_TO_SWITCH_KERNEL_MODE);
    *currentTime += TIME_TO_SWITCH_KERNEL_MODE;
    fprintf(file, "%d, 1, IRET\n", *currentTime);
    (*currentTime)++;
}

void handleEndIO(FILE *file, int *currentTime, int *delayArray, char *filteredOperation, VectorTable vectorTable[])
{
    int number = 0;
    sscanf(filteredOperation, "END_IO %d", &number);

    int executionTime = delayArray + number;
    fprintf(file, "%d, 1, check priority of interrupt\n", *currentTime);
    (*currentTime)++;
    fprintf(file, "%d, 1, check if masked\n", *currentTime);
    (*currentTime)++;
    fprintf(file, "%d, %d, switch to kernel mode\n", *currentTime, TIME_TO_SWITCH_KERNEL_MODE);
    *currentTime += TIME_TO_SWITCH_KERNEL_MODE;
    fprintf(file, "%d, %d, context saved\n", *currentTime, TIME_TO_SAVE_CONTEXT);
    *currentTime += TIME_TO_SAVE_CONTEXT;
    fprintf(file, "%d, %d, find vector %d in memory position 0x%04X\n", *currentTime, TIME_TO_FIND_VECTOR, number, vectorTable[number - 1].address);
    *currentTime += TIME_TO_FIND_VECTOR;
    fprintf(file, "%d, %d, load address 0x%04X into the PC\n", *currentTime, TIME_TO_LOAD_ADDRESS, vectorTable[number - 1].vector);
    *currentTime += TIME_TO_LOAD_ADDRESS;
    fprintf(file, "%d, %d, END_IO\n", *currentTime, executionTime);
    *currentTime += executionTime;
    fprintf(file, "%d, %d, context returned\n", *currentTime, TIME_TO_SAVE_CONTEXT);
    *currentTime += TIME_TO_SAVE_CONTEXT;
    fprintf(file, "%d, %d, switch to user mode\n", *currentTime, TIME_TO_SWITCH_KERNEL_MODE);
    *currentTime += TIME_TO_SWITCH_KERNEL_MODE;
    fprintf(file, "%d, 1, IRET\n", *currentTime);
    (*currentTime)++;
}

int main(int argc, char *argv[])
{
    int count = 0;
    char c;

    if (argc != 2)
    {
        printf("Usage: %s <input_trace_file>\n", argv[0]);
        return 1;
    }

    FILE *inputFile = fopen(argv[1], "r");
    if (inputFile == NULL)
    {
        perror("Failed to open input file");
        return EXIT_FAILURE;
    }

    FILE *outputFile = fopen("../output_files/execution.txt", "w");
    if (outputFile == NULL)
    {
        perror("Failed to create execution.txt");
        fclose(inputFile);
        return 1;
    }

    FILE *vectorTableFile = fopen("../vector_table.txt", "r");
    if (vectorTableFile == NULL)
    {
        perror("Failed to open vector table file");
        fclose(inputFile);
        fclose(outputFile);
        return 1;
    }
    FILE *delayTableFile = fopen("../device_table.txt", "r");
    if (delayTableFile == NULL)
    {
        perror("Failed to open device table file");
        fclose(inputFile);
        fclose(outputFile);
        fclose(vectorTableFile);
        return 1;
    }

    while ((c = getc(vectorTableFile)) != EOF)
        if (c == '\n')
            count++;

    VectorTable vectorTable[100];
    createVectorTable(vectorTable, vectorTableFile, count);

    int delayArray[50];
    createDelayArray(delayArray, delayTableFile);

    char line[50];
    int currentTime = 0;
    srand(time(NULL));

    while (fgets(line, sizeof(line), inputFile))
    {
        char filteredOperation[100];
        removeCommas(line, filteredOperation);

        char *executionTimeStr = getExecutionTime(line);
        int executionTime = executionTimeStr ? atoi(executionTimeStr) : 0;
        free(executionTimeStr);

        if (strstr(filteredOperation, "CPU") != NULL)
            handleCPU(outputFile, &currentTime, executionTime);
        else if (strstr(filteredOperation, "SYSCALL") != NULL)
            handleSysCall(outputFile, &currentTime, delayArray, filteredOperation, vectorTable);
        else if (strstr(filteredOperation, "END_IO") != NULL)
            handleEndIO(outputFile, &currentTime, delayArray, filteredOperation, vectorTable);
    }

    fclose(inputFile);
    fclose(outputFile);
    fclose(vectorTableFile);
    fclose(delayTableFile);

    printf("Simulation Over!\n");
    return 0;
}
