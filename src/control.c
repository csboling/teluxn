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
#include "conf_usb_host.h"  // needed in order to include "usb_protocol_hid.h"
#include "usb_protocol_hid.h"

#include "control.h"
#include "interface.h"
#include "engine.h"

#include "rom.h"
#include "uxn/uxn.h"

preset_meta_t meta;
preset_data_t preset;
shared_data_t shared;
int selected_preset;

// ----------------------------------------------------------------------------
// firmware dependent stuff starts here

Uxn uxn;
Device *devscreen, *devctrl, *devgpio;

// ----------------------------------------------------------------------------
// functions for main.c

void screen_talk(Device *d, Uint8 b0, Uint8 w);
void gpio_talk(Device *d, Uint8 b0, Uint8 w);
void ii_talk(Device *d, Uint8 b0, Uint8 w);
void nil_talk(Device *d, Uint8 b0, Uint8 w);
void loadrom(Uxn *u, char *mem);
void doctrl(Uxn *u, u8 mod, u8 key);

void init_presets(void) {
    // called by main.c if there are no presets saved to flash yet
    // initialize meta - some meta data to be associated with a preset, like a glyph
    // initialize shared (any data that should be shared by all presets) with default values
    // initialize preset with default values
    // store them to flash

    /* for (u8 i = 0; i < get_preset_count(); i++) { */
    /*     store_preset_to_flash(i, &meta, &preset); */
    /* } */

    /* store_shared_data_to_flash(&shared); */
    /* store_preset_index(0); */
}

void screen_talk(Device *d, Uint8 b0, Uint8 w) {
    (void)d;
    (void)b0;
    (void)w;
}

void gpio_talk(Device *d, Uint8 b0, Uint8 w) {
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

void ii_talk(Device *d, Uint8 b0, Uint8 w) {
    (void)d;
    (void)b0;
    (void)w;
}

void nil_talk(Device *d, Uint8 b0, Uint8 w) {
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
    draw_str("teluxn", line++, 15, 0);
    refresh_screen();
    if (!bootuxn(&uxn)) {
        draw_str("uxn boot error", line++, 15, 0);
        refresh_screen();
        return;
    }
    portuxn(&uxn, 0x0, "---", nil_talk);
    portuxn(&uxn, 0x1, "console", nil_talk);
    devscreen = portuxn(&uxn, 0x2, "screen", screen_talk);
    devgpio = portuxn(&uxn, 0x3, "gpio", gpio_talk);
    portuxn(&uxn, 0x4, "analog", nil_talk);
    portuxn(&uxn, 0x5, "ii", ii_talk);
    portuxn(&uxn, 0x6, "---", nil_talk);
    portuxn(&uxn, 0x7, "midi", nil_talk);
    devctrl = portuxn(&uxn, 0x8, "controller", nil_talk);
    portuxn(&uxn, 0x9, "---", nil_talk);
    portuxn(&uxn, 0xa, "file", nil_talk);
    portuxn(&uxn, 0xb, "---", nil_talk);
    portuxn(&uxn, 0xc, "---", nil_talk);
    portuxn(&uxn, 0xd, "---", nil_talk);
    portuxn(&uxn, 0xe, "---", nil_talk);
    portuxn(&uxn, 0xf, "---", nil_talk);

    /* write screen dimensions */
    mempoke16(devscreen->dat, 0x2, 128);
    mempoke16(devscreen->dat, 0x4, 64);

    loadrom(&uxn, uxn_rom);
    draw_str("load", line++, 15, 0);
    if (!evaluxn(&uxn, PAGE_PROGRAM)) {
        draw_str("prog error", line++, 15, 0);
        refresh_screen();
        return;
    }
    else {
        draw_str("prog done", line++, 15, 0);
        refresh_screen();
        return;
    }
}

void doctrl(Uxn *u, u8 mod, u8 z) {
    u8 flag = 0x00;
    switch (mod) {
    case HID_MODIFIER_LEFT_CTRL: flag = 0x01; break;
    case HID_MODIFIER_LEFT_ALT: flag = 0x02; break;
    case HID_MODIFIER_LEFT_SHIFT: flag = 0x04; break;
    case HID_ESCAPE: flag = 0x08; break;
    case HID_UP: flag = 0x10; break;
    case HID_DOWN: flag = 0x20; break;
    case HID_LEFT: flag = 0x40; break;
    case HID_RIGHT: flag = 0x80; break;
    }
    if (flag && z)
        devctrl->dat[2] |= flag;
    else if (flag)
        devctrl->dat[2] &= (~flag);
}

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
            if (length == 3) {
                devctrl->dat[3] = data[1];
                doctrl(&uxn, data[0], data[2]);
                evaluxn(&uxn, mempeek16(devctrl->dat, 0));
            }
            break;

        case ARC_ENCODER_COARSE:
            break;

        case FRONT_BUTTON_PRESSED:
            break;

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
