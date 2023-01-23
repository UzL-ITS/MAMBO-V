/**
 * C wrapper for trace_wrapper.cpp
 */

#ifndef __TRACE_WRITER_WRAPPER_H
#define __TRACE_WRITER_WRAPPER_H

#include "trace_writer.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TraceWriter TraceWriter;
typedef struct TraceEntry TraceEntry;
typedef enum {
	// Branch taken: 1 Bit
    TraceEntryFlags_BranchNotTaken = 0 << 0,
    TraceEntryFlags_BranchTaken = 1 << 0,

    // Branch type: 2 Bits
    TraceEntryFlags_BranchTypeJump = 1 << 1,
    TraceEntryFlags_BranchTypeCall = 2 << 1,
    TraceEntryFlags_BranchTypeReturn = 3 << 1,

    // Stack (de)allocations
    TraceEntryFlags_StackIsCall = 1 << 0,
    TraceEntryFlags_StackIsReturn = 2 << 0,
    TraceEntryFlags_StackIsOther = 3 << 0
} TraceWriter_TraceEntryFlags;

// Constructor wrapper
TraceWriter* TraceWriter_new(char *filenamePrefix);

// Returns the address of the first buffer entry.
TraceEntry* TraceWriter_Begin(TraceWriter* self);

// Returns the address AFTER the last buffer entry.
TraceEntry* TraceWriter_End(TraceWriter* self);

// Writes the contents of the trace buffer into the output file.
// -> end: A pointer to the address *after* the last entry to be written.
void TraceWriter_WriteBufferToFile(TraceWriter* self, TraceEntry* end);

// Sets the next testcase ID and opens a suitable trace file.
void TraceWriter_TestcaseStart(TraceWriter* self, int testcaseId, TraceEntry* nextEntry);

// Closes the current trace file and notifies the caller that the testcase has completed.
void TraceWriter_TestcaseEnd(TraceWriter* self, TraceEntry* nextEntry);

// Destructor wrapper
void TraceWriter_destroy(TraceWriter* self);

///
/// Static functions (don't need an object passed)
///
// Determines whether the given buffer pointers are identical.
bool TraceWriter_CheckBufferFull(TraceEntry* nextEntry, TraceEntry* entryBufferEnd);

// Creates a new MemoryRead entry.
TraceEntry* TraceWriter_InsertMemoryReadEntry(TraceEntry* nextEntry, uintptr_t instructionAddress, uintptr_t memoryAddress, uint32_t size);

// Creates a new MemoryWrite entry.
TraceEntry* TraceWriter_InsertMemoryWriteEntry(TraceEntry* nextEntry, uintptr_t instructionAddress, uintptr_t memoryAddress, uint32_t size);

// Creates a new HeapAllocSizeParameter entry.
TraceEntry* TraceWriter_InsertHeapAllocSizeParameterEntry(TraceEntry* nextEntry, uint64_t size);
TraceEntry* TraceWriter_InsertCallocSizeParameterEntry(TraceEntry* nextEntry, uint64_t count, uint64_t size);

// Creates a new HeapAllocAddressReturn entry.
TraceEntry* TraceWriter_InsertHeapAllocAddressReturnEntry(TraceEntry* nextEntry, uintptr_t memoryAddress);

// Creates a new HeapFreeAddressParameter entry.
TraceEntry* TraceWriter_InsertHeapFreeAddressParameterEntry(TraceEntry* nextEntry, uintptr_t memoryAddress);

// Creates a new StackPointerModification entry.
TraceEntry* TraceWriter_InsertStackPointerModificationEntry(TraceEntry* nextEntry, uintptr_t instructionAddress, uintptr_t newStackPointer, uint8_t flags);

// Creates a new Branch entry.
TraceEntry* TraceWriter_InsertBranchEntry(TraceEntry* nextEntry, uintptr_t sourceAddress, uintptr_t targetAddress, uint8_t taken, uint8_t type);

// Creates a new Branch entry for Jumps.
TraceEntry* TraceWriter_InsertJumpEntry(TraceEntry* nextEntry, uintptr_t sourceAddress, uintptr_t targetAddress, uint8_t type);

// Creates a new StackPointerInfo entry.
TraceEntry* TraceWriter_InsertStackPointerInfoEntry(TraceEntry* nextEntry, uintptr_t stackPointerMin, uintptr_t stackPointerMax);

// Initializes the static part of the prefix mode (record image loads, even when the thread's TraceWriter object is not yet initialized).
// -> filenamePrefix: The path prefix of the output file. Existing files are overwritten.
void TraceWriter_InitPrefixMode(const char *filenamePrefix);

// Writes information about the given loaded image into the trace metadata file.
void TraceWriter_WriteImageLoadData(int interesting, uint64_t startAddress, uint64_t endAddress, char *name);


#ifdef __cplusplus
}
#endif
#endif