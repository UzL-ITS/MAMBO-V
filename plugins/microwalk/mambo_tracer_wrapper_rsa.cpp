/* INCLUDES */

// OS-specific imports and helper macros
#if defined(_WIN32)
    #define _CRT_SECURE_NO_DEPRECATE
    
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
    
    #define _EXPORT __declspec(dllexport)
    #define _NOINLINE __declspec(noinline)
#else
    #ifdef _GNU_SOURCE
        #undef _GNU_SOURCE
    #endif

    #define _EXPORT __attribute__((visibility("default")))
    #define _NOINLINE __attribute__((noinline))
    
    #include <sys/resource.h>
#endif

// OpenSSL includes
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

// Standard includes
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>


/* TYPES */

#if defined(_WIN32)
    // Helper struct to read Thread Information Block.
    struct _TEB
    {
        NT_TIB tib;
        // Ignore remainder
    };
#else
    // Empty
#endif

/* FUNCTIONS */

// Performs target initialization steps.
// This function is called once in the very beginning, to make sure that the target is entirely loaded, and incorporated into the trace prefix.
_EXPORT _NOINLINE void InitTarget()
{
	// Nothing to do here
}

// Executes the target function.
// This function should only call the investigated library functions, this executable will not analyzed by the fuzzer.
// Do not use global variables, since the fuzzer might reuse the instrumented version of this executable for several different inputs.
_EXPORT _NOINLINE void RunTarget(FILE* input)
{
    RSA *rsa = PEM_read_RSAPrivateKey(input, nullptr, nullptr, nullptr);
	if(rsa == nullptr)
		ERR_print_errors_fp(stderr);

	// ...computations ...
	
	RSA_free(rsa);
}

// Pin notification functions.
// These functions (and their names) must not be optimized away by the compiler, so Pin can find and instrument them.
// The return values reduce the probability that the compiler uses these function in other places as no-ops (Visual C++ did do this in some experiments).
#pragma optimize("", off)
extern "C" _EXPORT _NOINLINE int PinNotifyTestcaseStart(int t) { return t + 42; }
extern "C" _EXPORT _NOINLINE int PinNotifyTestcaseEnd() { return 42; }
extern "C" _EXPORT _NOINLINE int PinNotifyStackPointer(uint64_t spMin, uint64_t spMax) { return static_cast<int>(spMin + spMax + 42); }
#pragma optimize("", on)

// Reads the stack pointer base value and transmits it to Pin.
_EXPORT void ReadAndSendStackPointer()
{
#if defined(_WIN32)
    // Read stack pointer
    _TEB* threadEnvironmentBlock = NtCurrentTeb();
    PinNotifyStackPointer(reinterpret_cast<uint64_t>(threadEnvironmentBlock->tib.StackLimit), reinterpret_cast<uint64_t>(threadEnvironmentBlock->tib.StackBase));
#else
    // There does not seem to be a reliable way to get the stack size, so we use an estimation
    // Compiling with -fno-split-stack may be desired, to avoid surprises during analysis

    // Take the current stack pointer as base value
    uintptr_t stackBase;
#if defined(__x86_64__)
    asm("mov %%rsp, %0" : "=r"(stackBase));
#elif defined(__riscv)
    asm("C.MV %0, sp" : "=r"(stackBase) ::);
#else
    #error Unsupported architecture!
#endif

    // Get full stack size
    struct rlimit stackLimit;
    if(getrlimit(RLIMIT_STACK, &stackLimit) != 0)
    {
        char errBuffer[128];
        strerror_r(errno, errBuffer, sizeof(errBuffer));
        fprintf(stderr, "Error reading stack limit: [%d] %s\n", errno, errBuffer);
    }

    uint64_t stackMin = reinterpret_cast<uint64_t>(stackBase) - reinterpret_cast<uint64_t>(stackLimit.rlim_cur);
    uint64_t stackMax = (reinterpret_cast<uint64_t>(stackBase) + 0x10000) & ~0x10000ull; // Round to next higher multiple of 64 kB (should be safe on x86 systems)
    PinNotifyStackPointer(stackMin, stackMax);
#endif
}

// Main trace target function. The following actions are performed:
//     The current action is read from stdin.
//     A line with "t" followed by a numeric ID, and another line with a file path determining a new testcase, that is subsequently loaded and fed into the target function, while calling PinNotifyNextFile() beforehand.
//     A line with "e 0" terminates the program.
extern "C" _EXPORT void TraceFunc()
{
	// First transmit stack pointer information
	ReadAndSendStackPointer();

	// Initialize target library
	InitTarget();

    // Run until exit is requested
    char inputBuffer[512];
    char errBuffer[128];
    while(true)
    {
        // Read command and testcase ID (0 for exit command)
        char command;
        int testcaseId;
        fgets(inputBuffer, sizeof(inputBuffer), stdin);
        sscanf(inputBuffer, "%c %d", &command, &testcaseId);

        // Exit or process given testcase
        if(command == 'e')
            break;
        if(command == 't')
        {
            // Read testcase file name
            fgets(inputBuffer, sizeof(inputBuffer), stdin);
            int inputFileNameLength = strlen(inputBuffer);
            if(inputFileNameLength > 0 && inputBuffer[inputFileNameLength - 1] == '\n')
                inputBuffer[inputFileNameLength - 1] = '\0';

            // Load testcase file and run target function
            FILE* inputFile = fopen(inputBuffer, "rb");
            if(inputFile == nullptr)
            {
#if defined(_WIN32)
                strerror_s(errBuffer, sizeof(errBuffer), errno);
#else
                strerror_r(errno, errBuffer, sizeof(errBuffer));
#endif
                fprintf(stderr, "Error opening input file '%s': [%d] %s\n", inputBuffer, errno, errBuffer);
                continue;
            }
            PinNotifyTestcaseStart(testcaseId);
            RunTarget(inputFile);
            PinNotifyTestcaseEnd();
            fclose(inputFile);
        }
    }
}

// Wrapper entry point.
int main(int argc, const char** argv)
{
    // Run target function
    TraceFunc();
    return 0;
}