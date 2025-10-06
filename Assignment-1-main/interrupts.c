#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "interrupts.h"

/**
 * This function creates a Vector Table struct with two columns: Address and Vector
 * These values are then used late on in the handling of the trace file
 */
void createVectorTable(VectorTable vectorTable[], FILE *vectorTableFile, int lineCount)
{
    // make an int array with the exact amount of space for every value in "vector_table.txt"
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

    // hardcoded values for vector addresses
    int addresses[] = {
        0x0002, 0x0004, 0x0006, 0x0008, 0x000A,
        0x000C, 0x000E, 0x0010, 0x0012, 0x0014,
        0x0016, 0x0018, 0x001A, 0x001C, 0x001E,
        0x0020, 0x0022, 0x0024, 0x0026, 0x0028,
        0x002A, 0x002C, 0x002E, 0x0030, 0x0032};

    // make sure that the amount of vector addresses and vectors is the same to not have a misamtch
    size_t addressCount = sizeof(addresses) / sizeof(addresses[0]);
    if (readCount != (int)addressCount)
    {
        fprintf(stderr,
                "Warning: Number of vectors read (%d) does not match number of addresses (%zu)\n",
                readCount, addressCount);
    }

    // write all the vector address and vectors into the vectorTable struct for later use
    for (int i = 0; i < readCount && i < (int)addressCount; i++)
    {
        vectorTable[i].vector = vectors[i];
        vectorTable[i].address = addresses[i];
    }

    // free the pointer vectors from memory
    free(vectors);
}

/**
 * Basic function to remove all commas from a line read from a .txt file
 * Ex. "CPU, 500" --> CPU 500
 */
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

/**
 * Basic function to get the numerical value afer the comma in the string assuming that is the execution time
 * Ex. "CPU, 500" will return 500.
 */
char *getExecutionTime(char *input)
{
    char *commaPosition = strchr(input, ','); // find comma in input string and return a ptr to it's position
    if (commaPosition == NULL)
        return NULL;

    // move the ptr to the next character after the comma to forget about the comma
    commaPosition++;

    // allocate memory to store a copy of whatever comes after the comma, plus 1 to make room for the null terminator
    char *output = malloc(strlen(commaPosition) + 1);
    if (output == NULL)
    {
        perror("Failed to allocate memory");
        return NULL;
    }

    // copy everything from where commaPosition points to in the string to output string
    strcpy(output, commaPosition);
    return output;
}

/**
 * Handle every line in the trace files that denotes CPU execution.
 * Very simple, just write's "current time, execution time, CPU execution" to output file
 * Ex. "CPU, 500" --> "0, 500, CPU execution"
 */
void handleCPU(FILE *file, int *currentTime, int executionTime)
{
    fprintf(file, "%d, %d, CPU execution\n", *currentTime, executionTime);
    *currentTime += executionTime;
}

/**
 * Handle every line in the trace files that denotes a SYSCALL execution
 * Steps to a SYSCALL execution:
 * 1. switch to kernel mode (fixed time --> TIME_TO_SWITCH_KERNEL_MODE)
 * 2. context saved (fixed time --> TIME_TO_SAVE_CONTEXT)
 * 3. find vector in memory position (fixed time --> TIME_TO_FIND_VECTOR)
 * 4. load address into PC (fixed time --> TIME_TO_LOAD_ADDRESS)
 * 5. run the ISR (time value retrieved from device_table.txt depending on what device call the SYSCALL and then a random value is calculated with it)
 * 6. transfer data (time value retrieved from device_table.txt depending on what device call the SYSCALL minus the time for running the ISR)
 * 7. context saved (fixed time --> TIME_TO_SAVE_CONTEXT)
 * 8. switch to user mode (fixed time --> TIME_TO_SWITCH_KERNEL_MODE)
 *
 * all fixed time values are stored in interrupts.h
 *
 */
void handleSysCall(FILE *file, int *currentTime, int executionTime, char *filteredOperation, VectorTable vectorTable[])
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
    int limit = executionTime / 2;

    // calculate time value dependings on device_table.txt and which device calls the SYSCALL
    int timeToSysCall = (rand() % (executionTime / 4 + 2)) + limit; // random value between 0 and value retrieved from device_table.txt
    int timeToTransferData = executionTime - timeToSysCall;         // difference between random value and device_table.txt,
    // i.e., timeToTransferData + timeToSysCall = value retrieved from device_table.txt

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

/**
 * Handle every line in the trace files that denotes a END_IO execution
 * Simply based off hardcoded time values in interrupts.h and writing the steps of END_IO into the output txt file with those time values
 *
 * all fixed time values are stored in interrupts.h
 *
 */
void handleEndIO(FILE *file, int *currentTime, int executionTime, char *filteredOperation, VectorTable vectorTable[])
{
    int number = 0;
    sscanf(filteredOperation, "END_IO %d", &number);

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
    // initialze variables used in main
    int vectorTableCount = 0;
    int deviceTableCount = 0;
    char c;

    // make sure command line receives three arguments: 1. executable file, 2: intput trace file, 3: output file
    if (argc != 3)
    {
        printf("Usage: %s <input_trace_file> <output_file>\n", argv[0]);
        return 1;
    }

    // open all files for reading and make sure they open properly
    FILE *inputFile = fopen(argv[1], "r");
    if (inputFile == NULL)
    {
        perror("Failed to open input file");
        return EXIT_FAILURE;
    }
    FILE *outputFile = fopen(argv[2], "w");
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
    FILE *deviceTableFile = fopen("../device_table.txt", "r");
    if (deviceTableFile == NULL)
    {
        perror("Failed to open device table file");
        fclose(inputFile);
        fclose(outputFile);
        fclose(vectorTableFile);
        return 1; // exit here
    }

    // count number of lines in device_table.txt
    rewind(deviceTableFile);
    while ((c = getc(deviceTableFile)) != EOF)
        if (c == '\n')
            deviceTableCount++;

    // count number of lines in vector_table.txt
    while ((c = getc(vectorTableFile)) != EOF)
        if (c == '\n')
            vectorTableCount++;

    // create a vector table based on calculated number of values in device_table.txt and vector_table.txt
    VectorTable vectorTable[100];
    createVectorTable(vectorTable, vectorTableFile, vectorTableCount);

    char line[50];
    int currentTime = 0;
    srand(time(NULL));

    while (fgets(line, sizeof(line), inputFile))
    {
        char filteredOperation[100];
        removeCommas(line, filteredOperation); // remove comma from individual line in trace file (CPU, 500 --> CPU 500)

        char *executionTimeStr = getExecutionTime(line);                   // get the numerical value from individual line in trace file (CPU 500 --> 500)
        int executionTime = executionTimeStr ? atoi(executionTimeStr) : 0; // convert executionTime to integer for later use
        free(executionTimeStr);

        // if the individual line in trace file contains "CPU", execute handleCPU() function
        if (strstr(filteredOperation, "CPU") != NULL)
            handleCPU(outputFile, &currentTime, executionTime);
        // if the individual line in trace file contains "SYSCALL", execute handleSysCall() function
        else if (strstr(filteredOperation, "SYSCALL") != NULL)
        {
            rewind(deviceTableFile);
            char buffer[100];
            int currentLine = 1;
            int value = 0;

            // get corresponding value from device_table.txt that denotes how long it takes to run the ISR and trasfer the data
            while (fgets(buffer, sizeof(buffer), deviceTableFile))
            {
                if (currentLine == executionTime) // executionTime used as line number
                {
                    if (sscanf(buffer, "%d", &value) != 1)
                    {
                        fprintf(stderr, "Failed to read value from line %d\n", executionTime);
                        return -1;
                    }
                    break;
                }
                currentLine++;
            }

            handleSysCall(outputFile, &currentTime, value, filteredOperation, vectorTable);
        }

        // if the individual line in trace file contains "END_IO", execute handleEndIO() function
        else if (strstr(filteredOperation, "END_IO") != NULL)
            handleEndIO(outputFile, &currentTime, executionTime, filteredOperation, vectorTable);
    }

    // close all txt files
    fclose(inputFile);
    fclose(outputFile);
    fclose(vectorTableFile);
    fclose(deviceTableFile);

    printf("Simulation Over!\n");
    return 0;
}
