#include <stdlib.h>
#include <stdio.h>
#include <libelf.h>

#include "../elf/elf_loader.h"

#define DEBUG 1
//#undef DEBUG
#ifdef DEBUG
  #define debug(...) fprintf(stderr, __VA_ARGS__)
#else
  #define debug(...)
#endif

int main(int argc, char **argv)
{
	Elf *elf = NULL;

	if (argc < 2) {
		printf("Syntax: dbm elf_file arguments\n");
		exit(EXIT_FAILURE);
	}

	struct elf_loader_auxv auxv;
	uintptr_t entry_address;
	load_elf("/usr/bin/lscpu", &elf, &auxv, &entry_address, false);
	debug("entry address: 0x%x\n", entry_address);
}