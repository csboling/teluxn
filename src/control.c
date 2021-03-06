// -----------------------------------------------------------------------------
// controller - the glue between the engine and the hardware
//
// reacts to events (grid press, clock etc) and translates them into appropriate
// engine actions. reacts to engine updates and translates them into user
// interface and hardware updates (grid LEDs, CV outputs etc)
//
// should talk to hardware via what's defined in interface.h only
// should talk to the engine via what's defined in engine.h only
// ----------------------------------------------------------------------------

#include "compiler.h"
#include "string.h"

#include "kbd.h"
#include "conf_usb_host.h"  // needed in order to include "usb_protocol_hid.h"
#include "usb_protocol_hid.h"

#include "screen.h"
#include "util.h"

#include "control.h"
#include "interface.h"
#include "engine.h"

#include "uxn/uxn.h"
#include "uxn/ppu.h"

preset_meta_t meta;
preset_data_t preset;
shared_data_t shared;
int selected_preset;

// ----------------------------------------------------------------------------
// firmware dependent stuff starts here

enum page_t {
    PAGE_BOOT,
    PAGE_LOAD,
    PAGE_RUN,
};
enum page_t curr_page = PAGE_BOOT;
bool screen_dirty = false;
bool button_state;
bool msc_connected;
bool rom_loaded;

Uxn uxn;
Ppu ppu;
/* uint8_t bg_pixels[128 * 64]; */
uint8_t screen_pixels[128 * 64];
Device *devscreen, *devctrl, *devgpio;

// ----------------------------------------------------------------------------
// functions for main.c

void system_talk(Device *d, uint8_t b0, uint8_t w);
void screen_talk(Device *d, uint8_t b0, uint8_t w);
void gpio_talk(Device *d, uint8_t b0, uint8_t w);
void ii_talk(Device *d, uint8_t b0, uint8_t w);
void nil_talk(Device *d, uint8_t b0, uint8_t w);
void loadrom(Uxn *u, char *mem);

void init_presets(void) {
}

void system_talk(Device *d, uint8_t b0, uint8_t w) {
    if (!w) {
        d->dat[0x2] = d->u->wst.ptr;
        d->dat[0x3] = d->u->rst.ptr;
    } else {
        putcolors(&ppu, &d->dat[0x8]);
        /* reqdraw = 1; */
    }
}

void screen_talk(Device *d, uint8_t b0, uint8_t w) {
    if(w && b0 == 0xe) {
        uint16_t x = mempeek16(d->dat, 0x8) & 0x7F;
        uint16_t y = mempeek16(d->dat, 0xa) & 0x3F;
        uint8_t *addr = &d->mem[mempeek16(d->dat, 0xc)];
        Layer *layer = (d->dat[0xe] >> 4 & 0x1) ? &ppu.fg : &ppu.bg;
        uint8_t mode = d->dat[0xe] >> 5;

        if(!mode)
            putpixel(&ppu, layer, x, y, d->dat[0xe] & 0xf);
        else if(mode-- & 0x1)
            puticn(&ppu, layer, x, y, addr, d->dat[0xe] & 0xf, mode & 0x2, mode & 0x4);
        else
            putchr(&ppu, layer, x, y, addr, d->dat[0xe] & 0xf, mode & 0x2, mode & 0x4);
    }
}

void gpio_talk(Device *d, uint8_t b0, uint8_t w) {
    switch (b0) {
        case 0x2:
            if (w) {
                for (int i = 0; i < 4; i++) {
                    set_gate(i, d->dat[0x2] & (1 << (3 - i)));
                }
            }
            break;
    }
}

void ii_talk(Device *d, uint8_t b0, uint8_t w) {
    (void)d;
    (void)b0;
    (void)w;
}

void nil_talk(Device *d, uint8_t b0, uint8_t w) {
    (void)d;
    (void)b0;
    (void)w;
}

void loadrom(Uxn *u, char *mem) {
    memcpy(u->ram.dat + PAGE_PROGRAM, mem, sizeof(u->ram.dat) - PAGE_PROGRAM);
}

void init_control(void) {
    clear_screen();
    int line = 0;
    draw_str("teluxn - uxn vm on teletype", line++, 15, 0);
    refresh_screen();
    if (!bootuxn(&uxn)) {
        draw_str("uxn boot error", line++, 15, 0);
        refresh_screen();
        return;
    }
    if (!initppu(&ppu, 16, 8, screen_pixels, screen_pixels)) {
        draw_str("ppu error", line++, 15, 0);
        refresh_screen();
        return;
    }
    portuxn(&uxn, 0x0, "system", system_talk);
    portuxn(&uxn, 0x1, "console", nil_talk);
    devscreen = portuxn(&uxn, 0x2, "screen", screen_talk);
    portuxn(&uxn, 0x3, "---", nil_talk);
    portuxn(&uxn, 0x4, "---", nil_talk);
    portuxn(&uxn, 0x5, "---", nil_talk);
    portuxn(&uxn, 0x6, "---", nil_talk);
    portuxn(&uxn, 0x7, "midi", nil_talk);
    devctrl = portuxn(&uxn, 0x8, "controller", nil_talk);
    portuxn(&uxn, 0x9, "---", nil_talk);
    portuxn(&uxn, 0xa, "file", nil_talk);
    portuxn(&uxn, 0xb, "---", nil_talk);
    devgpio = portuxn(&uxn, 0xc, "gpio", gpio_talk);
    portuxn(&uxn, 0xd, "ii", ii_talk);
    portuxn(&uxn, 0xe, "---", nil_talk);
    portuxn(&uxn, 0xf, "---", nil_talk);

    /* write screen dimensions */
    mempoke16(devscreen->dat, 0x2, 128);
    mempoke16(devscreen->dat, 0x4, 64);

    draw_str("ok. load roms from usb", line++, 15, 0);
    refresh_screen();
}

static void doctrl(Uxn *u, u8 mod, u8 key, u8 is_release) {
    u8 flag = 0x00;
    if (mod & HID_MODIFIER_LEFT_CTRL) flag |= 0x01;
    if (mod & HID_MODIFIER_RIGHT_CTRL) flag |= 0x01;
    if (mod & HID_MODIFIER_LEFT_ALT) flag |= 0x02;
    if (mod & HID_MODIFIER_RIGHT_ALT) flag |= 0x02;
    if (mod & HID_MODIFIER_LEFT_SHIFT) flag |= 0x04;
    if (mod & HID_MODIFIER_RIGHT_SHIFT) flag |= 0x04;
    if (key == HID_ESCAPE) flag |= 0x08;
    if (key == HID_UP) flag |= 0x10;
    if (key == HID_DOWN) flag |= 0x20;
    if (key == HID_LEFT) flag |= 0x40;
    if (key == HID_RIGHT) flag |= 0x80;

    if (flag) {
        if (is_release) {
            devctrl->dat[2] &= (~flag);
        }
        else {
            devctrl->dat[2] |= flag;
        }
    }
}

static uint8_t translate_keycode(u8 key, u8 mod) {
    switch (key) {
        case HID_ENTER: return 0x0a;
        case HID_BACKSPACE: return 0x08;
        case HID_TAB: return 0x09;
        case HID_ESCAPE: return 0x1B;
        case HID_DELETE: return 0x7F;
    }

    // latin alphabet
    if (key >= HID_A && key <= HID_Z) {
        if ((mod & HID_MODIFIER_LEFT_SHIFT) || (mod & HID_MODIFIER_RIGHT_SHIFT)) {
            return key + 0x3D;
        }
        else {
            return key + 0x5D;
        }
    }

    return hid_to_ascii(key, mod);
}

int release_count = 0;

void process_event(u8 event, u8 *data, u8 length) {
    switch (event) {
        case MAIN_CLOCK_RECEIVED:
            break;

        case MAIN_CLOCK_SWITCHED:
            break;

        case GATE_RECEIVED:
            if (length == 2) {
                devgpio->dat[3] ^= (data[1] << data[0]);
                evaluxn(&uxn, mempeek16(devgpio->dat, 0));
            }
            break;

        case GRID_CONNECTED:
            break;

        case GRID_KEY_PRESSED:
            break;

        case GRID_KEY_HELD:
            break;

        case KEYBOARD_KEY:
            if (length == 4) {

                if (data[3]) {
                    release_count++;
                }

                /* char s[36]; */
                /* itoa(data[0], s, 10); */
                /* draw_str(s, 0, 15, 0); */
                /* itoa(data[1], s, 10); */
                /* draw_str(s, 1, 15, 0); */
                /* itoa(data[2], s, 10); */
                /* draw_str(s, 6, 15, 0); */
                /* memset(s, 0, 36); */
                /* itoa(data[3], s, 10); */
                /* strcat(s, " "); */
                /* itoa(release_count, s + 2, 10); */
                /* draw_str(s, 7, 15, 0); */
                /* refresh_screen(); */

                devctrl->dat[3] = translate_keycode(data[1], data[0]);
                doctrl(&uxn, data[0], data[1], data[3]);
                evaluxn(&uxn, mempeek16(devctrl->dat, 0));
                devctrl->dat[3] = 0;
            }
            break;

        case ARC_ENCODER_COARSE:
            break;

        case FRONT_BUTTON_PRESSED: {
            if (length != 1) return;
            button_state = data[0];

            if (curr_page == PAGE_LOAD && !button_state) {
                int line = 0;
                if (rom_filename_ct > 0 && selected_rom < rom_filename_ct) {
                    screen_dirty = false;
                    if (!load_rom_file(&line, &uxn, rom_filenames[selected_rom])) {
                        print_err(&line, "couldn't load rom");
                        rom_loaded = false;
                        return;
                    }
                    clear_screen();
                    if (!evaluxn(&uxn, PAGE_PROGRAM)) {
                        draw_str("error running rom", line++, 15, 0);
                        refresh_screen();
                        return;
                    }
                    rom_loaded = true;
                    curr_page = PAGE_RUN;
                    return;
                }
                else {
                    draw_str("no rom chosen", line++, 15, 0);
                    refresh_screen();
                    return;
                }
            }
            break;
        }

        case FRONT_BUTTON_HELD:
            break;

        case BUTTON_PRESSED:
            break;

        case I2C_RECEIVED:
            break;

        case TIMED_EVENT:
            break;

        case MIDI_CONNECTED:
            break;

        case MIDI_NOTE:
            break;

        case MIDI_CC:
            break;

        case MIDI_AFTERTOUCH:
            break;

        case SHNTH_BAR:
            break;

        case SHNTH_ANTENNA:
            break;

        case SHNTH_BUTTON:
            break;

        case MASS_STORAGE_CONNECTED: {
            int line = 0;
            if (length != 1) {
                draw_str("missing event data", line++, 15, 0);
                refresh_screen();
                return;
            }
            msc_connected = data[0] != 0;

            if (msc_connected) {
                clear_screen();

                int files = list_files(&line);
                if (files > 0) {
                    selected_rom = 0;
                    curr_page = PAGE_LOAD;
                    screen_dirty = true;
                    return;
                }
                else if (files < 0) {
                    print_err(&line, "file err ");
                }
                else {
                    print_status(&line, "no .rom files");
                }
            }
            else {
                if (rom_loaded) {
                    curr_page = PAGE_RUN;
                }
            }
            break;
        }

        default:
            break;
    }
}

void render_grid(void) {
    // render grid LEDs here or leave blank if not used
}

void render_arc(void) {
    // render arc LEDs here or leave blank if not used
}

uint8_t knob_scale(uint8_t s) {
    uint32_t t = get_knob_value(0) >> 4;
    t *= s;
    t >>= 12;
    return (uint8_t)t;
}

void render_screen(void) {
    if (curr_page == PAGE_RUN) {
        evaluxn(&uxn, mempeek16(devctrl->dat, 0));
        screen_draw_region(0, 0, 128, 64, screen_pixels);
        return;
    }

    if (!screen_dirty) return;
    clear_screen();
    switch (curr_page) {
    case PAGE_LOAD:
        selected_rom = knob_scale(rom_filename_ct);
        for (int i = 0; i < rom_filename_ct; i++) {
            draw_str(rom_filenames[i], i, 15, selected_rom == i ? 7 : 0);
        }
        break;
    }
    refresh_screen();
}
