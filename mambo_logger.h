#ifndef MAMBO_LOGGER_H
#define MAMBO_LOGGER_H

#ifndef DEBUG
	// Macros for external use
	#define log_create(...)
	#define log_open(...)
	#define log_close(...)
	#define log_open_raw_fp(...)
	#define log_set_secondary_fp(...)
	#define log(...)

#else

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>

// Macros for external use
#define log_create(name, init_flags) _log_create(name, init_flags)
#define log_open(name, mode) _log_open(name, mode)
#define log_close(name) _log_close(name)
#define log_open_raw_fp(name, modes) _log_open_raw_fp(name, modes)
#define log_set_secondary_fp(name, fp) _log_set_secondary_fp(name, fp)
#define log(name, ...) _log(name, __VA_ARGS__)

#define LOG_OPEN	0b1			// Filepointer was opened
#define LOG_STDOUT	0b10		// Activate STDOUT output
#define LOG_STDERR	0b100		// Activate STDERR output (recommended)

typedef struct {
	char name[60];
	FILE *log_fp;
	FILE *log_sec_fp;
	int flags;
} mambo_logger_t;

mambo_logger_t* _log_search(char *name);
void _log_create(char *name, int init_flags);
void _log_open(char *name, char *modes);
void _log_close(char *name);
FILE* _log_open_raw_fp(char *name, char *modes);
void _log_set_secondary_fp(char *name, FILE *fp);
void _log(char *name, const char *format, ...);

#endif
#endif