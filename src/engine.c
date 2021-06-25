// ----------------------------------------------------------------------------
// app engine
//
// hardware agnostic application logic
// sends updates to control and provides actions for control to call
// ----------------------------------------------------------------------------

#include <string.h>

#include "engine.h"
#include "control.h"

#include "fat.h"
#include "file.h"
#include "fs_com.h"
#include "navigation.h"
#include "uhi_msc.h"
#include "uhi_msc_mem.h"

int selected_rom;
int rom_filename_ct;
char rom_filenames[FILENAME_CT][FILENAME_LEN];

#define INCWRAP(l) (l = (l + 1) % 8)

static bool mount_drive(void) {
    if (uhi_msc_is_available()) {
        for (int i = 0; i < uhi_msc_get_lun(); i++) {
	    if (nav_drive_set(i)) {
	        if (nav_partition_mount()) {
		    return true;
		}
	    }
	}
    }
    return false;
}

int open_drive(void) {
    nav_reset();
    for (uint8_t i = 0; i < FS_NB_NAVIGATOR; i++) {
        if (nav_select(i)) {
	    if (mount_drive()) {
	        return 0;
	    }
	}
    }
    nav_filelist_reset();
    nav_exit();
    return -fs_g_status;
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

int list_files(void) {
    if (!nav_filelist_single_enable(FS_FILE)) {
        return -fs_g_status;
    }
    if (!nav_filelist_set(0, FS_FIND_NEXT)) {
        return -fs_g_status;
    }
    int rom_index = 0;
    while (!nav_filelist_eol()) {
        if (nav_file_getname(rom_filenames[rom_index], FILENAME_LEN - 1)) {
	    if (str_ends_with(rom_filenames[rom_index], ".rom")) {
		rom_index++;
	        if (rom_index == FILENAME_CT)
		    break;
	    }
	    else {
	        rom_filenames[rom_index][0] = '\0';
	    }
	}
        if (!nav_filelist_set(0, FS_FIND_NEXT)) {
	    return -fs_g_status;
	}
    }
    nav_filelist_single_disable();
    rom_filename_ct = rom_index;
    return rom_index;
}

bool load_rom_file(Uxn *u, char* fname) {
    if (!nav_setcwd(fname, true, false)) {
        return false;
    }
    if (!file_open(FOPEN_MODE_R)) {
        return false;
    }
    file_read_buf(u->ram.dat + PAGE_PROGRAM, sizeof(u->ram.dat) - PAGE_PROGRAM);
    file_close();
    return true;
}
