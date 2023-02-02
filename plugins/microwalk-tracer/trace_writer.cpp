/* INCLUDES */
#include "trace_writer.h"
#include <iostream>
#include <fstream>
#include <sstream>


/* STATIC VARIABLES */

bool TraceWriter::_prefixMode;
std::ofstream TraceWriter::_prefixDataFileStream;


/* TYPES */

TraceWriter::TraceWriter(std::string filenamePrefix)
{
    // Remember prefix
    _outputFilenamePrefix = filenamePrefix;

    // Open prefix output file
	std::string filename = filenamePrefix + "prefix.trace";
    OpenOutputFile(filename);
}

TraceWriter::~TraceWriter()
{
    // Close file stream
    _outputFileStream.close();
}

void TraceWriter::InitPrefixMode(const std::string& filenamePrefix)
{
    // Start trace prefix mode
    _prefixMode = true;

    // Open prefix metadata output file
    _prefixDataFileStream.exceptions(std::ofstream::failbit | std::ofstream::badbit);
	std::string prefixDataFilename = filenamePrefix + "prefix_data.txt";
    _prefixDataFileStream.open(prefixDataFilename.c_str(), std::ofstream::out | std::ofstream::trunc);
    if(!_prefixDataFileStream)
    {
        std::cerr << "Error: Could not open prefix metadata output file '" << prefixDataFilename << "'." << std::endl;
        exit(1);
    }
    std::cerr << "Trace prefix mode started" << std::endl;
}

TraceEntry* TraceWriter::Begin()
{
    return _entries;
}

TraceEntry* TraceWriter::End()
{
    return &_entries[ENTRY_BUFFER_SIZE];
}

void TraceWriter::OpenOutputFile(std::string& filename)
{
    // Open file for writing
    _outputFileStream.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    _currentOutputFilename = filename;
    _outputFileStream.open(_currentOutputFilename.c_str(), std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
    if(!_outputFileStream)
    {
        std::cerr << "Error: Could not open output file '" << _currentOutputFilename << "'." << std::endl;
        exit(1);
    }
}

void TraceWriter::WriteBufferToFile(TraceEntry* end)
{
    // Write buffer contents
    if(_testcaseId != -1 || _prefixMode)
        _outputFileStream.write(reinterpret_cast<char*>(_entries), reinterpret_cast<uintptr_t>(end) - reinterpret_cast<uintptr_t>(_entries));
}

void TraceWriter::TestcaseStart(int testcaseId, TraceEntry* nextEntry)
{
    // Exit prefix mode if necessary
    if(_prefixMode)
        TestcaseEnd(nextEntry);

    // Remember new testcase ID
    _testcaseId = testcaseId;

    // Open file for writing
    std::stringstream filenameStream;
    filenameStream << _outputFilenamePrefix << "t" << std::dec << _testcaseId << ".trace";
	std::string filename = filenameStream.str();
    OpenOutputFile(filename);
    std::cerr << "Switched to testcase #" << std::dec << _testcaseId << std::endl;
}

void TraceWriter::TestcaseEnd(TraceEntry* nextEntry)
{
    // Save remaining trace data
    if(nextEntry != _entries)
        WriteBufferToFile(nextEntry);

    // Close file handle and reset flags
    _outputFileStream.close();
    _outputFileStream.clear();

    // Exit prefix mode if necessary
    if(_prefixMode)
    {
        _prefixDataFileStream.close();
        _prefixMode = false;
        std::cerr << "Trace prefix mode ended" << std::endl;
    }
    else
    {
        // Notify caller that the trace file is complete
		std::cout << "t\t" << _currentOutputFilename << std::endl;
    }

    // Disable tracing until next test case starts
    _testcaseId = -1;
}

void TraceWriter::WriteImageLoadData(int interesting, uint64_t startAddress, uint64_t endAddress, std::string& name)
{
    // Prefix mode active?
    if(!_prefixMode)
    {
        std::cerr << "Image load ignored: " << name << std::endl;
        return;
    }

    // Write image data
    _prefixDataFileStream << "i\t" << interesting << "\t" << std::hex << startAddress << "\t" << std::hex << endAddress << "\t" << name << std::endl;
}

bool TraceWriter::CheckBufferFull(TraceEntry* nextEntry, TraceEntry* entryBufferEnd)
{
    return nextEntry != NULL && nextEntry == entryBufferEnd;
}

TraceEntry* TraceWriter::InsertMemoryReadEntry(TraceEntry* nextEntry, uintptr_t instructionAddress, uintptr_t memoryAddress, uint32_t size)
{
    // Create entry
    nextEntry->Type = TraceEntryTypes::MemoryRead;
    nextEntry->Param0 = size;
    nextEntry->Param1 = instructionAddress;
    nextEntry->Param2 = memoryAddress;
    return ++nextEntry;
}

TraceEntry* TraceWriter::InsertMemoryWriteEntry(TraceEntry* nextEntry, uintptr_t instructionAddress, uintptr_t memoryAddress, uint32_t size)
{
    // Create entry
    nextEntry->Type = TraceEntryTypes::MemoryWrite;
    nextEntry->Param0 = size;
    nextEntry->Param1 = instructionAddress;
    nextEntry->Param2 = memoryAddress;
    return ++nextEntry;
}

TraceEntry* TraceWriter::InsertHeapAllocSizeParameterEntry(TraceEntry* nextEntry, uint64_t size)
{
    // Check whether given entry pointer is valid (we might be in a non-instrumented thread)
    if(nextEntry == NULL)
        return nextEntry;

    // Create entry
    nextEntry->Type = TraceEntryTypes::HeapAllocSizeParameter;
    nextEntry->Param1 = size;
    return ++nextEntry;
}

TraceEntry* TraceWriter::InsertCallocSizeParameterEntry(TraceEntry* nextEntry, uint64_t count, uint64_t size)
{
    return InsertHeapAllocSizeParameterEntry(nextEntry, count * size);
}

TraceEntry* TraceWriter::InsertHeapAllocAddressReturnEntry(TraceEntry* nextEntry, uintptr_t memoryAddress)
{
    // Check whether given entry pointer is valid (we might be in a non-instrumented thread)
    if(nextEntry == NULL)
        return nextEntry;

    // Create entry
    nextEntry->Type = TraceEntryTypes::HeapAllocAddressReturn;
    nextEntry->Param2 = memoryAddress;
    return ++nextEntry;
}

TraceEntry* TraceWriter::InsertHeapFreeAddressParameterEntry(TraceEntry* nextEntry, uintptr_t memoryAddress)
{
    // Check whether given entry pointer is valid (we might be in a non-instrumented thread)
    if(nextEntry == NULL)
        return nextEntry;

    // Create entry
    nextEntry->Type = TraceEntryTypes::HeapFreeAddressParameter;
    nextEntry->Param2 = memoryAddress;
    return ++nextEntry;
}

TraceEntry* TraceWriter::InsertStackPointerModificationEntry(TraceEntry* nextEntry, uintptr_t instructionAddress, uintptr_t newStackPointer, uint8_t flags)
{
    // Check whether given entry pointer is valid (we might be in a non-instrumented thread)
    if(nextEntry == NULL)
        return nextEntry;

    // Create entry
    nextEntry->Type = TraceEntryTypes::StackPointerModification;
    nextEntry->Flag = flags;
    nextEntry->Param1 = instructionAddress;
    nextEntry->Param2 = newStackPointer;
    return ++nextEntry;
}

TraceEntry* TraceWriter::InsertBranchEntry(TraceEntry* nextEntry, uintptr_t sourceAddress, uintptr_t targetAddress, uint8_t taken, uint8_t type)
{
    // Create entry
    nextEntry->Type = TraceEntryTypes::Branch;
    nextEntry->Param1 = sourceAddress;
    nextEntry->Param2 = targetAddress;
    nextEntry->Flag = static_cast<uint8_t>(type) | static_cast<uint8_t>(taken == 0 ? TraceEntryFlags::BranchNotTaken : TraceEntryFlags::BranchTaken);
    return ++nextEntry;
}

TraceEntry* TraceWriter::InsertJumpEntry(TraceEntry* nextEntry, uintptr_t sourceAddress, uintptr_t targetAddress, uint8_t type)
{
    // Create entry
    nextEntry->Type = TraceEntryTypes::Branch;
    nextEntry->Param1 = sourceAddress;
    nextEntry->Param2 = targetAddress;
    nextEntry->Flag = static_cast<uint8_t>(type) | static_cast<uint8_t>(TraceEntryFlags::BranchTaken);
    return ++nextEntry;
}

TraceEntry* TraceWriter::InsertStackPointerInfoEntry(TraceEntry* nextEntry, uintptr_t stackPointerMin, uintptr_t stackPointerMax)
{
    // Create entry
    nextEntry->Type = TraceEntryTypes::StackPointerInfo;
    nextEntry->Param1 = stackPointerMin;
	nextEntry->Param2 = stackPointerMax;
    return ++nextEntry;
}

ImageData::ImageData(bool interesting, std::string name, uintptr_t startAddress, uintptr_t endAddress)
{
    _interesting = interesting;
    _name = name;
    _startAddress = startAddress;
    _endAddress = endAddress;
}

bool ImageData::ContainsBasicBlock(uintptr_t bb)
{
    // Check basic block start address only. It's expected, that start and end address are in the same image (otherwise something else went wrong).
    return _startAddress <= bb && bb <= _endAddress;
}

bool ImageData::IsInteresting()
{
    return _interesting;
}