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

/* void drive_info(int *line) { */
/*     nav_reset(); */
/*     if (!nav_drive_set(nav_drive_nb() - 1)) { */
/* 	print_err(line, "drive_set "); */
/* 	return; */
/*     } */
/*     if (!nav_partition_mount()) { */
/* 	print_err(line, "mount "); */
/* 	return; */
/*     } */

/*     if (!nav_dir_name((FS_STRING)str_buff, 35)) { */
/* 	print_err(line, "dir_name "); */
/* 	return; */
/*     } */
/*     print_status(line, str_buff); */

/*     if (!nav_filelist_first(FS_DIR)) { */
/* 	// Sort items by files */
/* 	nav_filelist_first(FS_FILE); */
/*     } */
/*     nav_filelist_reset(); */
/*     // While an item can be found */
/*     while (nav_filelist_set(0, FS_FIND_NEXT)) */
/*     { */
/* 	nav_file_name((FS_STRING)str_buff, 35, FS_NAME_GET, true); */
/* 	print_status(line, str_buff); */
/*     } */
/* } */

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
	print_status(line, rom_filenames[rom_filename_ct]);
	if (str_ends_with(rom_filenames[rom_filename_ct], ".rom")) {
	    rom_filename_ct++;
	    if (rom_filename_ct == FILENAME_CT) {
		break;
	    }
	}
    }

    return rom_filename_ct;
}

/* int list_files(int *line) { */
/*     int rom_index = 0; */

/*     nav_reset(); */

/*     for (uint8_t i = 0; i < FS_NB_NAVIGATOR; i++) { */
/*         if (nav_select(i)) { */
/*             int lun_count = uhi_msc_get_lun(); */
/*             for (int lun = 0; lun < lun_count; lun++) { */
/* 	        print_status(line, "try drive"); */
/* 	        if (!nav_drive_set(lun)) { */
/* 		    print_err(line, "drive set err "); */
/* 		    continue; */
/* 	        } */
/* 	        if (!nav_partition_mount()) { */
/* 		    print_err(line, "mount err "); */
/* 		    continue; */
/* 	        } */

/* 	        print_status(line, "mounted drive"); */

/* 	        nav_filelist_reset(); */
/*                     /\* if (!nav_filelist_single_enable(FS_FILE)) { *\/ */
/* 	    	/\* 	print_err(line, "filelist en err "); *\/ */
/* 	    	/\* 	break; *\/ */
/*                     /\* } *\/ */
/* 	    	/\* if (!nav_dir_root()) { *\/ */
/* 	    	/\*     print_err(line, "dir root err "); *\/ */
/* 	    	/\* } *\/ */

/* 	        /\* if (!nav_filelist_set(0, FS_FIND_NEXT)) { *\/ */
/* 	        /\* 	print_err(line, "filelist set err "); *\/ */
/* 	        /\* } *\/ */
/* 	        while (!nav_filelist_eol()) { */
/* 	    	    if (nav_file_name(rom_filenames[rom_index], FILENAME_LEN - 1, FS_NAME_GET, false)) { */
/* 	    	        print_status(line, rom_filenames[rom_index]); */
/* 	    	        if (str_ends_with(rom_filenames[rom_index], ".rom")) { */
/* 	    	    	rom_index++; */

/* 	    	    	/\* nav_exit(); *\/ */
/* 	    	    	rom_filename_ct = rom_index; */
/* 	    	    	return rom_index; */


/* 	    	    	if (rom_index == FILENAME_CT) { */
/* 	    	    	    goto done; */
/* 	    	    	} */
/* 	    	        } */
/* 	    	        else { */
/* 	    	    	print_status(line, rom_filenames[rom_index]); */
/* 	    	    	/\* rom_filenames[rom_index][0] = '\0'; *\/ */
/* 	    	        } */
/* 	    	    } */
/* 	    	    else { */
/* 	    	        print_err(line, "filename err "); */
/* 	    	        print_status(line, rom_filenames[rom_index]); */
/*                                 /\* continue; *\/ */
/* 	    	    } */
/* 	    	    if (!nav_filelist_set(0, FS_FIND_NEXT)) { */
/* 	    	        print_err(line, "filelist next err "); */
/* 		    } */
/* 		} */
/* 	        /\* if (!nav_filelist_single_disable()) { *\/ */
/* 	        /\*     print_err(line, "filelist dis err "); *\/ */
/* 	        /\* } *\/ */
/* 	    } */
/*         } */
/*     } */

/* done: */
/*     /\* nav_exit(); *\/ */
/*     rom_filename_ct = rom_index; */
/*     return rom_index; */
/* err: */
/*     nav_exit(); */
/*     return -fs_g_status; */
/* } */


/* static bool mount_drive(int *line) { */
/*     if (uhi_msc_is_available()) { */
/*         for (int i = 0; i < uhi_msc_get_lun(); i++) { */
/* 	    if (nav_drive_set(i)) { */
/* 	        if (nav_partition_mount()) { */
/* 		    return true; */
/* 		} */
/* 		else { */
/* 		    print_err(line, "partition err"); */
/* 		} */
/* 	    } */
/* 	    else { */
/* 		print_err(line, "set err"); */
/* 	    } */
/* 	} */
/*     } */
/*     else { */
/* 	print_err(line, "uhi msc err"); */
/*     } */
/*     return false; */
/* } */

/* static bool open_drive(int *line) { */
/*     nav_reset(); */
/*     for (uint8_t i = 0; i < FS_NB_NAVIGATOR; i++) { */
/*         if (nav_select(i)) { */
/* 	    if (mount_drive(line)) { */
/* 	        return true; */
/* 	    } */
/* 	    else { */
/* 	        print_err(line, "mount err"); */
/* 	    } */
/* 	} */
/* 	else { */
/* 	    print_err(line, "nav sel err"); */
/* 	} */
/*     } */
/*     nav_filelist_reset(); */
/*     nav_exit(); */
/*     return false; */
/* } */

bool load_rom_file(int *line, Uxn *u, char* fname) {
    /* if (!open_drive(line)) { */
    /* 	print_err(line, "open drive err"); */
    /* } */
    /* nav_reset(); */
    /* nav_select(0); */

    /* for (uint8_t i = 0; i < FS_NB_NAVIGATOR; i++) { */
    /*     if (nav_select(i)) { */
        /* int lun_count = uhi_msc_get_lun(); */
        /* for (int lun = 0; lun < lun_count; lun++) { */
	/*     print_status(line, "try drive"); */
	/*     if (!nav_drive_set(lun)) { */
	/* 	print_err(line, "drive set err "); */
	/* 	continue; */
	/*     } */
	/*     if (!nav_partition_mount()) { */
	/* 	print_err(line, "mount err "); */
	/* 	continue; */
	/*     } */

	    /* nav_filelist_reset(); */
		/* if (!nav_dir_root()) { */
		/*     print_err(line, "dir root err "); */
		/* } */

    print_status(line, "try to read file");
    print_status(line, fname);

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
    /* file_close(); */
    /* while (bytes_read < length) { */
    /* 	uint16_t chunk = file_read_buf(u->ram.dat + PAGE_PROGRAM + bytes_read, length - bytes_read); */
    /* 	if (chunk == 0) { */
    /* 	    print_err(line, "EOF"); */
    /* 	    break; */
    /* 	} */
    /* 	bytes_read += chunk; */
    /* } */
    file_close();
    /* nav_exit(); */
    return true;
	/* } */

	/* nav_exit(); */
	/* return false; */
}
