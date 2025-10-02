#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "interrupts.h"

// Reads our defined Vector Table address and write them into the VectorTable struct
void createVectorTable(VectorTable vectorTable[])
{
    // Define the vector table using Example 1 Values
    int vectors[] = {0x01E3, // Predefined vectors
                     0x029C,
                     0x0695,
                     0x042B,
                     0x0292,
                     0x048B,
                     0x0639,
                     0x00BD,
                     0x06EF,
                     0x036C,
                     0x07B0,
                     0x01F8,
                     0x03B9,
                     0x06C7,
                     0x0165,
                     0x0584,
                     0x02DF,
                     0x05B3,
                     0x060A,
                     0x0765,
                     0x07B7,
                     0x0523,
                     0x03B7,
                     0x028C,
                     0x05E8,
                     0x05D3};

    int addresses[] = {0x0002, // Predefine Addresses
                       0x0004,
                       0x0006,
                       0x0008,
                       0x000A,
                       0x000C,
                       0x000E,
                       0x0010,
                       0x0012,
                       0x0014,
                       0x0016,
                       0x0018,
                       0x001A,
                       0x001C,
                       0x001E,
                       0x0020,
                       0x0022,
                       0x0024,
                       0x0026,
                       0x0028,
                       0x002A,
                       0x002C,
                       0x002E,
                       0x0030,
                       0x0032,
                       0x0034};

    size_t size = (sizeof(addresses) / sizeof(addresses[0])) + 1;

    for (int i = 0; i < size; i++)
    {
        vectorTable[i].vector = vectors[i];    // Store vector
        vectorTable[i].address = addresses[i]; // Store address
    }
}

// This function will take the operation from trace.txt, ex. 'CPU,', and remove the commma for better processing later on
void removeCommas(const char *input, char *output)
{
    int j = 0; // Index for the output string

    for (int i = 0; input[i] != '\0'; i++)
    {
        if (input[i] == ',')
        {
            continue; // Skip comma
        }
        output[j++] = input[i]; // Copy non-comma character to output
    }
    output[j] = '\0'; // Null-terminate the output string
}

char *getExecutionTime(char *input)
{
    // Find the position of the first comma
    char *commaPosition = strchr(input, ','); // Returns pointer to the first occurrence of ','

    // If no comma found, return NULL or an empty string
    if (commaPosition == NULL)
    {
        return NULL; // Or return strdup("") for an empty string
    }

    // Move the pointer to the character after the comma
    commaPosition++; // Increment to skip the comma

    // Allocate memory for the output string
    char *output = (char *)malloc(strlen(commaPosition) + 1); // +1 for null terminator
    if (output == NULL)
    {
        perror("Failed to allocate memory");
        return NULL;
    }

    // Copy everything after the comma into the output string
    strcpy(output, commaPosition);

    // Return the new string
    return output;
}

// Function to handle CPU operations
void handleCPU(FILE *file, int *currentTime, int executionTime)
{
    fprintf(file, "%d, %d, CPU execution\n", *currentTime, executionTime);
    *currentTime += executionTime;
}

// Function to handle SYSCALL operations
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

    float limit = executionTime * 0.40;
    int timeToSysCall = (rand() % 6) + limit;
    int timeToTransferData = (rand() % 6) + limit;
    int timeToCheckForErrors = executionTime - (timeToSysCall + timeToTransferData);

    fprintf(file, "%d, %d, SYSCALL: run the ISR\n", *currentTime, timeToSysCall);
    *currentTime += timeToSysCall;

    fprintf(file, "%d, %d, transfer data\n", *currentTime, timeToTransferData);
    *currentTime += timeToTransferData;

    fprintf(file, "%d, %d, check for errors\n", *currentTime, timeToCheckForErrors);
    *currentTime += timeToCheckForErrors;

    fprintf(file, "%d, 1, IRET\n", *currentTime);
    (*currentTime)++;
}

// Function to handle END_IO operations
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

    fprintf(file, "%d, %d, find vector %d in memory position 0x%04X\n", *currentTime, TIME_TO_FIND_VECTOR, number, vectorTable[number - 1].address);
    *currentTime += TIME_TO_FIND_VECTOR;

    fprintf(file, "%d, %d, load address 0x%04X into the PC\n", *currentTime, TIME_TO_LOAD_ADDRESS, vectorTable[number - 1].vector);
    *currentTime += TIME_TO_LOAD_ADDRESS;

    fprintf(file, "%d, %d, END_IO\n", *currentTime, executionTime);
    *currentTime += executionTime;

    fprintf(file, "%d, 1, IRET\n", *currentTime);
    (*currentTime)++;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <input_trace_file> <output_execution_file>\n", argv[0]);
        return 1;
    }

    // Open the input file provided as the first argument
    FILE *inputFile = fopen(argv[1], "r");
    if (inputFile == NULL)
    {
        perror("Failed to open input file");
        return EXIT_FAILURE;
    }

    // Open the output file provided as the second argument
    FILE *file = fopen(argv[2], "w");
    if (file == NULL)
    {
        perror("Failed to open output file");
        fclose(inputFile);
        return 1;
    }

    
    
    char line[50]; // Store up to 50 events from the trace file
    int currentTime = 0;
    VectorTable vectorTable[100];

    createVectorTable(vectorTable); // created a vectorTable struct with our above specified vector table address's and vectors in the createVectorTable function

    while (fgets(line, sizeof(line), inputFile))
    {
        char filteredOperation[100];
        char *executionTimeStr = getExecutionTime(line);
        int executionTime = atoi(executionTimeStr);
        free(executionTimeStr);

        removeCommas(line, filteredOperation);

        // Handle CPU, SYSCALL, and END_IO operations
        if (strstr(filteredOperation, "CPU") != NULL)
        {
            handleCPU(file, &currentTime, executionTime);
        }
        else if (strstr(filteredOperation, "SYSCALL") != NULL)
        {
            handleSysCall(file, &currentTime, executionTime, filteredOperation, vectorTable);
        }
        else if (strstr(filteredOperation, "END_IO") != NULL)
        {
            handleEndIO(file, &currentTime, executionTime, filteredOperation, vectorTable);
        }
    }
    
    fclose(inputFile);
    fclose(file);

    printf("Simulation Over!\n");
    return 0;
}