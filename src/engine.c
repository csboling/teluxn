// ----------------------------------------------------------------------------
// app engine
//
// hardware agnostic application logic
// sends updates to control and provides actions for control to call
// ----------------------------------------------------------------------------

#include <string.h>

#include "engine.h"
#include "control.h"
#include "interface.h"

#include "fat.h"
#include "file.h"
#include "fs_com.h"
#include "navigation.h"
#include "uhi_msc.h"
#include "uhi_msc_mem.h"
#include "util.h"

int selected_rom;
int rom_filename_ct;
char rom_filenames[FILENAME_CT][FILENAME_LEN];

void print_status(int *line, char *msg) {
    draw_str(msg, *line, 15, 0);
    refresh_screen();
    if (*line == 7) {
	*line = 0;
    }
    else {
	*line = *line + 1;
    }
}

void print_err(int *line, char *msg) {
    char s[36];
    s[0] = '\0';
    int len = strlen(msg);
    strcpy(s, msg);
    itoa(fs_g_status, s + len, 10);
    if (*line == 0) {
	clear_screen();
    }
    draw_str(s, *line, 15, 0);
    refresh_screen();
    if (*line == 7) {
	*line = 0;
    }
    else {
	*line = *line + 1;
    }
}

static bool str_ends_with(const char *str, const char *suffix)
{
    if (!str || !suffix)
        return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr)
        return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

int list_files(int *line) {
    nav_reset();
    if (!nav_drive_set(nav_drive_nb() - 1)) {
	print_err(line, "drive_set ");
	return -fs_g_status;
    }
    if (!nav_partition_mount()) {
	print_err(line, "mount ");
	return -fs_g_status;
    }

    strcpy(rom_filenames[0], "scan drive ");
    if (!nav_dir_name((FS_STRING)(rom_filenames[0] + 11), 35)) {
	print_err(line, "dir_name ");
	return -fs_g_status;
    }
    print_status(line, rom_filenames[0]);

    if (!nav_filelist_first(FS_FILE)) {
	print_err(line, "filelist_first ");
	return -fs_g_status;
    }
    if (!nav_filelist_single_enable(FS_FILE)) {
	print_err(line, "filelist_single ");
	return -fs_g_status;
    }
    nav_filelist_reset();

    rom_filename_ct = 0;
    while (nav_filelist_set(0, FS_FIND_NEXT)) {
	nav_file_name((FS_STRING)rom_filenames[rom_filename_ct], 35, FS_NAME_GET, true);
	if (str_ends_with(rom_filenames[rom_filename_ct], ".rom")) {
	    rom_filename_ct++;
	    if (rom_filename_ct == FILENAME_CT) {
		break;
	    }
	}
    }

    return rom_filename_ct;
}

bool load_rom_file(int *line, Uxn *u, char* fname) {
    if (!nav_setcwd((FS_STRING)fname, true, false)) {
	print_err(line, "couldn't find file");
	return false;
    }
    if (!file_open(FOPEN_MODE_R)) {
	print_err(line, "couldn't read file");
	return false;
    }

    uint16_t bytes_read = 0;
    const size_t length = sizeof(u->ram.dat) - PAGE_PROGRAM;
    while (!file_eof())
    {
    	uint16_t chunk = file_read_buf(u->ram.dat + PAGE_PROGRAM + bytes_read, length - bytes_read);
	if (chunk == 0) {
	    print_err(line, "no bytes read");
	    file_close();
	    return false;
	}
	bytes_read += chunk;
    }
    file_close();
    return true;
}
