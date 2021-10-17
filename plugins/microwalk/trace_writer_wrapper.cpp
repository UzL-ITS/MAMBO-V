/**
 * C wrapper for trace_wrapper.cpp
 */

#include "trace_writer_wrapper.h"
#include <string>

extern "C" {
	TraceWriter* TraceWriter_new(char *filenamePrefix)
	{
		return new TraceWriter(std::string(filenamePrefix));
	}

	TraceEntry* TraceWriter_Begin(TraceWriter* self)
	{
		return self->Begin();
	}

	TraceEntry* TraceWriter_End(TraceWriter* self)
	{
		return self->End();
	}

	void TraceWriter_WriteBufferToFile(TraceWriter* self, TraceEntry* end)
	{
		self->WriteBufferToFile(end);
	}

	void TraceWriter_TestcaseStart(TraceWriter* self, int testcaseId, TraceEntry* nextEntry)
	{
		self->TestcaseStart(testcaseId, nextEntry);
	}

	void TraceWriter_TestcaseEnd(TraceWriter* self, TraceEntry* nextEntry)
	{
		self->TestcaseEnd(nextEntry);
	}

	void TraceWriter_destroy(TraceWriter* self)
	{
		delete self;
	}

	bool TraceWriter_CheckBufferFull(TraceEntry* nextEntry, TraceEntry* entryBufferEnd)
	{
		return TraceWriter::CheckBufferFull(nextEntry, entryBufferEnd);
	}

	TraceEntry* TraceWriter_InsertMemoryReadEntry(TraceEntry* nextEntry, uintptr_t instructionAddress, uintptr_t memoryAddress, uint32_t size)
	{
		return TraceWriter::InsertMemoryReadEntry(nextEntry, instructionAddress, memoryAddress, size);
	}

	TraceEntry* TraceWriter_InsertMemoryWriteEntry(TraceEntry* nextEntry, uintptr_t instructionAddress, uintptr_t memoryAddress, uint32_t size)
	{
		return TraceWriter::InsertMemoryWriteEntry(nextEntry, instructionAddress, memoryAddress, size);
	}

	TraceEntry* TraceWriter_InsertHeapAllocSizeParameterEntry(TraceEntry* nextEntry, uint64_t size)
	{
		return TraceWriter::InsertHeapAllocSizeParameterEntry(nextEntry, size);
	}
	TraceEntry* TraceWriter_InsertCallocSizeParameterEntry(TraceEntry* nextEntry, uint64_t count, uint64_t size)
	{
		return TraceWriter::InsertCallocSizeParameterEntry(nextEntry, count, size);
	}

	TraceEntry* TraceWriter_InsertHeapAllocAddressReturnEntry(TraceEntry* nextEntry, uintptr_t memoryAddress)
	{
		return TraceWriter::InsertHeapAllocAddressReturnEntry(nextEntry, memoryAddress);
	}

	TraceEntry* TraceWriter_InsertHeapFreeAddressParameterEntry(TraceEntry* nextEntry, uintptr_t memoryAddress)
	{
		return TraceWriter::InsertHeapFreeAddressParameterEntry(nextEntry, memoryAddress);
	}

	TraceEntry* TraceWriter_InsertStackPointerModificationEntry(TraceEntry* nextEntry, uintptr_t instructionAddress, uintptr_t newStackPointer, uint8_t flags)
	{
		return TraceWriter::InsertStackPointerModificationEntry(nextEntry, instructionAddress, newStackPointer, flags);
	}

	TraceEntry* TraceWriter_InsertBranchEntry(TraceEntry* nextEntry, uintptr_t sourceAddress, uintptr_t targetAddress, uint8_t taken, uint8_t type)
	{
		return TraceWriter::InsertBranchEntry(nextEntry, sourceAddress, targetAddress, taken, type);
	}

	TraceEntry* TraceWriter_InsertJumpEntry(TraceEntry* nextEntry, uintptr_t sourceAddress, uintptr_t targetAddress, uint8_t type)
	{
		return TraceWriter::InsertJumpEntry(nextEntry, sourceAddress, targetAddress, type);
	}

	TraceEntry* TraceWriter_InsertStackPointerInfoEntry(TraceEntry* nextEntry, uintptr_t stackPointerMin, uintptr_t stackPointerMax)
	{
		return TraceWriter::InsertStackPointerInfoEntry(nextEntry, stackPointerMin, stackPointerMax);
	}

	void TraceWriter_InitPrefixMode(const char *filenamePrefix)
	{
		TraceWriter::InitPrefixMode(std::string(filenamePrefix));
	}

	void TraceWriter_WriteImageLoadData(int interesting, uint64_t startAddress, uint64_t endAddress, char *name)
	{
		std::string s = std::string(name);
		TraceWriter::WriteImageLoadData(interesting, startAddress, endAddress, s);
	}
}