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

int open_drive(void);
int list_files(void);
bool load_rom_file(Uxn *u, char* fname);
