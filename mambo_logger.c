#ifdef DEBUG

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include "mambo_logger.h"

#define LOG_NAME_LENGTH 60
#define LOG_MAX_LOGGERS 20

static mambo_logger_t global_loggers[LOG_MAX_LOGGERS] = {0};
static int log_nr = 0;

mambo_logger_t* _log_search(char *name) {
	if (name == NULL)
		name = "";
	for (int i = 0; i < LOG_MAX_LOGGERS; i++) {
		if (strcmp(global_loggers[i].name, name) == 0)
			return &global_loggers[i];
	}
	return NULL;
}

void _log_create(char *name, int init_flags)
{
	mambo_logger_t *logger = _log_search(NULL);
	strncpy(logger->name, name, LOG_NAME_LENGTH);
	logger->flags = init_flags;
}

void _log_open(char *name, char *modes)
{
	mambo_logger_t *logger = _log_search(name);

	FILE *logfp = _log_open_raw_fp(name, modes);
	if (logfp != NULL) {
		fprintf(logfp, "==== START NEW LOGGING ====\n");
		logger->log_fp = logfp;
		logger->flags |= LOG_OPEN;
	} else
		logger->flags &= ~LOG_OPEN;
}

void _log_close(char *name)
{
	mambo_logger_t *logger = _log_search(name);

	FILE *logfp = logger->log_fp;
	if (logfp != NULL && logger->flags & LOG_OPEN) {
		fclose(logfp);
		logger->flags &= ~LOG_OPEN;
	}
}

FILE* _log_open_raw_fp(char *name, char* modes)
{
	char filename[LOG_NAME_LENGTH + 4];
	snprintf(filename, LOG_NAME_LENGTH + 4, "%s.log", name);

	FILE *fp;
	fp = fopen(filename, modes);
	return fp;
}

void _log_set_secondary_fp(char *name, FILE *fp)
{
	mambo_logger_t *logger = _log_search(name);
	if (logger == NULL) {
		fprintf(stderr, "logger %s not avaluable!\n", name);
		return;
	}

	logger->log_sec_fp = fp;
}

void _log(char *name, const char *format, ...) 
{
	mambo_logger_t *logger = _log_search(name);
	if (logger == NULL) {
		fprintf(stderr, "logger %s not avaluable!\n", name);
		return;
	}

	va_list arglist;

	if (logger->log_fp != NULL && logger->flags & LOG_OPEN) {
		fprintf(logger->log_fp, "%-5d [ %20s ]  ", log_nr, logger->name);
		if (logger->log_sec_fp != NULL)
			fprintf(logger->log_sec_fp, "%-5d [ %20s ]  ", log_nr, logger->name);

		if (logger->flags & LOG_STDOUT) {
			fprintf(stdout, "%-5d [ %20s ]  ", log_nr, logger->name);
		}
		if (logger->flags & LOG_STDERR) {
			fprintf(stderr, "%-5d [ %20s ]  ", log_nr, logger->name);
		}

		va_start(arglist, format);
		vfprintf(logger->log_fp, format, arglist);
		if (logger->log_sec_fp != NULL)
			vfprintf(logger->log_sec_fp, format, arglist);

		if (logger->flags & LOG_STDOUT) {
			vfprintf(stdout, format, arglist);
		}
		if (logger->flags & LOG_STDERR) {
			vfprintf(stderr, format, arglist);
		}
		va_end(arglist);

		log_nr++;
	}
}

#endif