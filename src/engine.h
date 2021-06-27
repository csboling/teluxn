// ----------------------------------------------------------------------------
// defines engine actions available to control
// ----------------------------------------------------------------------------

#pragma once

#include <stdbool.h>
#include "uxn/uxn.h"

#define FILENAME_CT 8
#define FILENAME_LEN 32

extern int selected_rom;
extern int rom_filename_ct;
extern char rom_filenames[FILENAME_CT][FILENAME_LEN];

/* int open_drive(int *line); */
int list_files(int *line);
bool load_rom_file(int *line, Uxn *u, char* fname);
void print_status(int *line, char *msg);
void print_err(int *line, char *msg);

void drive_info(int *line);
