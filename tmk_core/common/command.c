/*
Copyright 2011 Jun Wako <wakojun@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdint.h>
#include <stdbool.h>
#include "wait.h"
#include "keycode.h"
#include "host.h"
#include "keymap.h"
#include "print.h"
#include "debug.h"
#include "util.h"
#include "timer.h"
#include "keyboard.h"
#include "bootloader.h"
#include "action_layer.h"
#include "action_util.h"
#include "eeconfig.h"
#include "sleep_led.h"
#include "led.h"
#include "command.h"
#include "backlight.h"
#include "hook.h"

#ifdef MOUSEKEY_ENABLE
#include "mousekey.h"
#endif

#ifdef PROTOCOL_PJRC
#   include "usb_keyboard.h"
#   ifdef EXTRAKEY_ENABLE
#       include "usb_extra.h"
#   endif
#endif

#ifdef PROTOCOL_VUSB
#   include "usbdrv.h"
#endif

#ifdef PS2_MOUSE_ENABLE
extern uint8_t ps2_mouse_enabled;
#endif


static bool command_common(uint8_t code);
static void command_common_help(void);
static bool command_console(uint8_t code);
static void command_console_help(void);
#ifdef MOUSEKEY_ENABLE
static bool mousekey_console(uint8_t code);
static void mousekey_console_help(void);
#endif

static void switch_default_layer(uint8_t layer);


command_state_t command_state = ONESHOT;


bool command_proc(uint8_t code)
{
    if (has_mods_key) return false;
    switch (command_state) {
        case ONESHOT:
            if (!IS_COMMAND())
                return false;
            return (command_extra(code) || command_common(code));
            break;
        case CONSOLE:
            if (IS_COMMAND())
                return (command_extra(code) || command_common(code));
            else
                return (command_console_extra(code) || command_console(code));
            break;
#if defined(MOUSEKEY_ENABLE) && defined(DEBUG)  //all save 563B
        case MOUSEKEY:
            mousekey_console(code);
            break;
#endif
        default:
            command_state = ONESHOT;
            return false;
    }
    return true;
}

/* TODO: Refactoring is needed. */
/* This allows to define extra commands. return false when not processed. */
bool command_extra(uint8_t code) __attribute__ ((weak));
bool command_extra(uint8_t code)
{
    clear_keyboard(); 
    (void)code;
    return false;
}

bool command_console_extra(uint8_t code) __attribute__ ((weak));
bool command_console_extra(uint8_t code)
{
    (void)code;
    return false;
}


/***********************************************************
 * Command common
 ***********************************************************/
static void command_common_help(void)
{
    print("\n\t- Magic -\n"
          "d: debug\n"
          "x: debug matrix\n"
          "k: debug keyboard\n"
          "m: debug mouse\n"
          "v: version\n"
          "s: status\n"
          "c: console mode\n"
          "0-7: layer0-7(F10-F7)\n"
          "Paus: bootloader\n"

#ifdef KEYBOARD_LOCK_ENABLE
          "Caps: Lock\n"
#endif

#ifdef BOOTMAGIC_ENABLE
          "e: eeprom\n"
#endif

#ifdef NKRO_ENABLE
          "n: NKRO\n"
#endif

#ifdef SLEEP_LED_ENABLE
          "z: sleep LED test\n"
#endif
    );
}

#ifdef BOOTMAGIC_ENABLE
static void print_eeconfig(void)
{
    print("default_layer: "); print_dec(eeconfig_read_default_layer()); print("\n");

    debug_config_t dc;
    dc.raw = eeconfig_read_debug();
    print("debug_config.raw: "); print_hex8(dc.raw); print("\n");
    print(".enable: "); print_dec(dc.enable); print("\n");
    print(".matrix: "); print_dec(dc.matrix); print("\n");
    print(".keyboard: "); print_dec(dc.keyboard); print("\n");
    print(".mouse: "); print_dec(dc.mouse); print("\n");

    keymap_config_t kc;
    kc.raw = eeconfig_read_keymap();
    print("keymap_config.raw: "); print_hex8(kc.raw); print("\n");
    print(".swap_control_capslock: "); print_dec(kc.swap_control_capslock); print("\n");
    print(".capslock_to_control: "); print_dec(kc.capslock_to_control); print("\n");
    print(".swap_lalt_lgui: "); print_dec(kc.swap_lalt_lgui); print("\n");
    print(".swap_ralt_rgui: "); print_dec(kc.swap_ralt_rgui); print("\n");
    print(".no_gui: "); print_dec(kc.no_gui); print("\n");
    print(".swap_grave_esc: "); print_dec(kc.swap_grave_esc); print("\n");
    print(".swap_backslash_backspace: "); print_dec(kc.swap_backslash_backspace); print("\n");
    print(".nkro: "); print_dec(kc.nkro); print("\n");

#ifdef BACKLIGHT_ENABLE
    backlight_config_t bc;
    bc.raw = eeconfig_read_backlight();
    print("backlight_config.raw: "); print_hex8(bc.raw); print("\n");
    print(".enable: "); print_dec(bc.enable); print("\n");
    print(".level: "); print_dec(bc.level); print("\n");
#endif

#ifdef PS2_MOUSE_ENABLE
    print("ps2_mouse_enabled: "); print_hex8(ps2_mouse_enabled); print("\n");
#endif
}
#endif

static bool command_common(uint8_t code)
{
#ifdef KEYBOARD_LOCK_ENABLE
    static host_driver_t *host_driver = 0;
#endif
#ifdef SLEEP_LED_ENABLE
    static bool sleep_led_test = false;
#endif
    switch (code) {
#ifndef NO_DEBUG // only need 1 ... 0 and F1 ... F10
#ifdef SLEEP_LED_ENABLE
        case KC_Z:
            // test breathing sleep LED
            print("Sleep LED test\n");
            if (sleep_led_test) {
                sleep_led_disable();
                led_set(host_keyboard_leds());
            } else {
                sleep_led_enable();
            }
            sleep_led_test = !sleep_led_test;
            break;
#endif
#ifdef BOOTMAGIC_ENABLE
        case KC_E:
            print("eeconfig:\n");
            print_eeconfig();
            break;
#endif
#ifdef KEYBOARD_LOCK_ENABLE
        case KC_CAPSLOCK:
            if (host_get_driver()) {
                host_driver = host_get_driver();
                clear_keyboard();
                host_set_driver(0);
                print("Locked.\n");
            } else {
                host_set_driver(host_driver);
                print("Unlocked.\n");
            }
            break;
#endif
        case KC_H:
        case KC_SLASH: /* ? */
            command_common_help();
            break;
        case KC_C:
            debug_matrix   = false;
            debug_keyboard = false;
            debug_mouse    = false;
            debug_enable   = false;
            command_console_help();
            print("C> ");
            command_state = CONSOLE;
            break;
        case KC_PAUSE:
            clear_keyboard();
            print("\n\nbootloader... ");
            wait_ms(1000);
            bootloader_jump(); // not return
            break;
        case KC_D:
            if (debug_enable) {
                print("\ndebug: off\n");
                debug_matrix   = false;
                debug_keyboard = false;
                debug_mouse    = false;
                debug_enable   = false;
            } else {
                print("\ndebug: on\n");
                debug_enable   = true;
            }
            break;
        case KC_X: // debug matrix toggle
            debug_matrix = !debug_matrix;
            if (debug_matrix) {
                print("\nmatrix: on\n");
                debug_enable = true;
            } else {
                print("\nmatrix: off\n");
            }
            break;
        case KC_K: // debug keyboard toggle
            debug_keyboard = !debug_keyboard;
            if (debug_keyboard) {
                print("\nkeyboard: on\n");
                debug_enable = true;
            } else {
                print("\nkeyboard: off\n");
            }
            break;
        case KC_M: // debug mouse toggle
            debug_mouse = !debug_mouse;
            if (debug_mouse) {
                print("\nmouse: on\n");
                debug_enable = true;
            } else {
                print("\nmouse: off\n");
            }
            break;
        case KC_V: // print version & information
            print("\n\t- Version -\n");
            print("DESC: " STR(DESCRIPTION) "\n");
            print("VID: " STR(VENDOR_ID) "(" STR(MANUFACTURER) ") "
                  "PID: " STR(PRODUCT_ID) "(" STR(PRODUCT) ") "
                  "VER: " STR(DEVICE_VER) "\n");
            print("BUILD: " STR(VERSION) " (" __TIME__ " " __DATE__ ")\n");
            /* build options */
            print("OPTIONS:"
#ifdef PROTOCOL_PJRC
            " PJRC"
#endif
#ifdef PROTOCOL_LUFA
            " LUFA"
#endif
#ifdef PROTOCOL_VUSB
            " VUSB"
#endif
#ifdef PROTOCOL_CHIBIOS
            " CHIBIOS"
#endif
#ifdef BOOTMAGIC_ENABLE
            " BOOTMAGIC"
#endif
#ifdef MOUSEKEY_ENABLE
            " MOUSEKEY"
#endif
#ifdef EXTRAKEY_ENABLE
            " EXTRAKEY"
#endif
#ifdef CONSOLE_ENABLE
            " CONSOLE"
#endif
#ifdef COMMAND_ENABLE
            " COMMAND"
#endif
#ifdef NKRO_ENABLE
            " NKRO"
#endif
#ifdef KEYMAP_SECTION_ENABLE
            " KEYMAP_SECTION"
#endif
#ifdef KEYMAP_IN_EEPROM_ENABLE
            " KEYMAP_IN_EEPROM"
#endif
#ifdef LEDMAP_ENABLE
            " LEDMAP"
#endif
#ifdef LEDMAP_IN_EEPROM_ENABLE
            " LEDMAP_IN_EEPROM"
#endif
#ifdef BACKLIGHT_ENABLE
            " BACKLIGHT"
#endif 
#ifdef SOFTPWM_LED_ENABLE
            " SOFTPWM_LED"
#endif
#ifdef BREATHING_LED_ENABLE
            " BREATHING_LED"
#endif
            " " STR(BOOTLOADER_SIZE) "\n");

            print("GCC: " STR(__GNUC__) "." STR(__GNUC_MINOR__) "." STR(__GNUC_PATCHLEVEL__)
#if defined(__AVR__)
                  " AVR-LIBC: " __AVR_LIBC_VERSION_STRING__
                  " AVR_ARCH: avr" STR(__AVR_ARCH__) "\n");
#elif defined(__arm__)
            // TODO
            );
#endif
            break;
        case KC_S:
            print("\n\t- Status -\n");
            print_val_hex8(host_keyboard_leds());
            print_val_hex8(keyboard_protocol);
            print_val_hex8(keyboard_idle);
#ifdef NKRO_ENABLE
    #ifdef __AVR__
            print_val_hex8(keyboard_protocol & (1<<4));
    #else
            print_val_hex8(keyboard_nkro);
    #endif
#endif
            print_val_hex32(timer_read32());

#ifdef PROTOCOL_PJRC
            print_val_hex8(UDCON);
            print_val_hex8(UDIEN);
            print_val_hex8(UDINT);
            print_val_hex8(usb_keyboard_leds);
            print_val_hex8(usb_keyboard_idle_count);
#endif

#ifdef PROTOCOL_PJRC
#   if USB_COUNT_SOF
            print_val_hex8(usbSofCount);
#   endif
#endif
            break;
#endif //no debug
#ifdef NKRO_ENABLE
        case KC_N:
            hook_nkro_change();
            //clear_keyboard(); //Prevents stuck keys.

#ifdef __AVR__
            keyboard_protocol ^= (1<<4);
            if (keyboard_protocol & (1<<4)) {
#else
            keyboard_nkro = !keyboard_nkro;
            if (keyboard_nkro) {
#endif
                    print("NKRO: on\n");
                } else {
                    print("NKRO: off\n");
                }
            //}
            break;
#endif
        //case KC_ESC:
        //case KC_GRV:
//#ifndef COMMAND_NO_DEFAULT_LAYER
#if 0
        case KC_1 ... KC_0:
        case KC_F1 ... KC_F10:
            ;
            uint8_t target_layer = 0;
            if (code < KC_8) target_layer = code - KC_1 + 1;
            else if (code >= KC_F1 && code < KC_F8) target_layer = code - KC_F1 + 1;
            switch_default_layer(target_layer);
            break;
#endif
        default:
            print("?");
            return false;
    }
    return true;
}


/***********************************************************
 * Command console
 ***********************************************************/
static void command_console_help(void)
{
    print("\n\t- Console -\n"
          "ESC/q: quit\n"
#ifdef MOUSEKEY_ENABLE
          "m: mousekey\n"
#endif
    );
}

static bool command_console(uint8_t code)
{
#ifdef DEBUG
    switch (code) {
        case KC_H:
        case KC_SLASH: /* ? */
            command_console_help();
            break;
        case KC_Q:
        case KC_ESC:
            command_state = ONESHOT;
            return false;
#if defined(MOUSEKEY_ENABLE) && defined(DEBUG)
        case KC_M:
            mousekey_console_help();
            print("M> ");
            command_state = MOUSEKEY;
            return true;
#endif
        default:
            print("?");
            return false;
    }
    print("C> ");
    return true;
#endif
}


#if defined(MOUSEKEY_ENABLE) && defined(DEBUG)
/***********************************************************
 * Mousekey console
 ***********************************************************/
static uint8_t mousekey_param = 0;

static void mousekey_param_print(void)
{
    print("\n\t- Values -\n");
    print("1: delay(*10ms): "); pdec(mk_delay); print("\n");
    print("2: interval(ms): "); pdec(mk_interval); print("\n");
    print("3: max_speed: "); pdec(mk_max_speed); print("\n");
    print("4: time_to_max: "); pdec(mk_time_to_max); print("\n");
    print("5: wheel_max_speed: "); pdec(mk_wheel_max_speed); print("\n");
    print("6: wheel_time_to_max: "); pdec(mk_wheel_time_to_max); print("\n");
}

//#define PRINT_SET_VAL(v)  print(#v " = "); print_dec(v); print("\n");
#define PRINT_SET_VAL(v)  xprintf(#v " = %d\n", (v))
static void mousekey_param_inc(uint8_t param, uint8_t inc)
{
    switch (param) {
        case 1:
            if (mk_delay + inc < UINT8_MAX)
                mk_delay += inc;
            else
                mk_delay = UINT8_MAX;
            PRINT_SET_VAL(mk_delay);
            break;
        case 2:
            if (mk_interval + inc < UINT8_MAX)
                mk_interval += inc;
            else
                mk_interval = UINT8_MAX;
            PRINT_SET_VAL(mk_interval);
            break;
        case 3:
            if (mk_max_speed + inc < UINT8_MAX)
                mk_max_speed += inc;
            else
                mk_max_speed = UINT8_MAX;
            PRINT_SET_VAL(mk_max_speed);
            break;
        case 4:
            if (mk_time_to_max + inc < UINT8_MAX)
                mk_time_to_max += inc;
            else
                mk_time_to_max = UINT8_MAX;
            PRINT_SET_VAL(mk_time_to_max);
            break;
        case 5:
            if (mk_wheel_max_speed + inc < UINT8_MAX)
                mk_wheel_max_speed += inc;
            else
                mk_wheel_max_speed = UINT8_MAX;
            PRINT_SET_VAL(mk_wheel_max_speed);
            break;
        case 6:
            if (mk_wheel_time_to_max + inc < UINT8_MAX)
                mk_wheel_time_to_max += inc;
            else
                mk_wheel_time_to_max = UINT8_MAX;
            PRINT_SET_VAL(mk_wheel_time_to_max);
            break;
    }
}

static void mousekey_param_dec(uint8_t param, uint8_t dec)
{
    switch (param) {
        case 1:
            if (mk_delay > dec)
                mk_delay -= dec;
            else
                mk_delay = 0;
            PRINT_SET_VAL(mk_delay);
            break;
        case 2:
            if (mk_interval > dec)
                mk_interval -= dec;
            else
                mk_interval = 0;
            PRINT_SET_VAL(mk_interval);
            break;
        case 3:
            if (mk_max_speed > dec)
                mk_max_speed -= dec;
            else
                mk_max_speed = 0;
            PRINT_SET_VAL(mk_max_speed);
            break;
        case 4:
            if (mk_time_to_max > dec)
                mk_time_to_max -= dec;
            else
                mk_time_to_max = 0;
            PRINT_SET_VAL(mk_time_to_max);
            break;
        case 5:
            if (mk_wheel_max_speed > dec)
                mk_wheel_max_speed -= dec;
            else
                mk_wheel_max_speed = 0;
            PRINT_SET_VAL(mk_wheel_max_speed);
            break;
        case 6:
            if (mk_wheel_time_to_max > dec)
                mk_wheel_time_to_max -= dec;
            else
                mk_wheel_time_to_max = 0;
            PRINT_SET_VAL(mk_wheel_time_to_max);
            break;
    }
}

static void mousekey_console_help(void)
{
    print("\n\t- Mousekey -\n"
          "ESC/q: quit\n"
          "1: delay(*10ms)\n"
          "2: interval(ms)\n"
          "3: max_speed\n"
          "4: time_to_max\n"
          "5: wheel_max_speed\n"
          "6: wheel_time_to_max\n"
          "\n"
          "p: print values\n"
          "d: set defaults\n"
          "up: +1\n"
          "down: -1\n"
          "pgup: +10\n"
          "pgdown: -10\n"
          "\n"
          "speed = delta * max_speed * (repeat / time_to_max)\n");
    xprintf("where delta: cursor=%d, wheel=%d\n" 
            "See http://en.wikipedia.org/wiki/Mouse_keys\n", MOUSEKEY_MOVE_DELTA,  MOUSEKEY_WHEEL_DELTA);
}

static bool mousekey_console(uint8_t code)
{
    switch (code) {
        case KC_H:
        case KC_SLASH: /* ? */
            mousekey_console_help();
            break;
        case KC_Q:
        case KC_ESC:
            if (mousekey_param) {
                mousekey_param = 0;
            } else {
                print("C> ");
                command_state = CONSOLE;
                return false;
            }
            break;
        case KC_P:
            mousekey_param_print();
            break;
        case KC_1 ... KC_6:
            mousekey_param = code - KC_1 + 1;
            break;
        case KC_UP:
            mousekey_param_inc(mousekey_param, 1);
            break;
        case KC_DOWN:
            mousekey_param_dec(mousekey_param, 1);
            break;
        case KC_PGUP:
            mousekey_param_inc(mousekey_param, 10);
            break;
        case KC_PGDN:
            mousekey_param_dec(mousekey_param, 10);
            break;
        case KC_D:
            mk_delay = MOUSEKEY_DELAY/10;
            mk_interval = MOUSEKEY_INTERVAL;
            mk_max_speed = MOUSEKEY_MAX_SPEED;
            mk_time_to_max = MOUSEKEY_TIME_TO_MAX;
            mk_wheel_max_speed = MOUSEKEY_WHEEL_MAX_SPEED;
            mk_wheel_time_to_max = MOUSEKEY_WHEEL_TIME_TO_MAX;
            print("set default\n");
            break;
        default:
            print("?");
            return false;
    }
    if (mousekey_param) {
        xprintf("M%d> ", mousekey_param);
    } else {
        print("M>" );
    }
    return true;
}
#endif


/***********************************************************
 * Utilities
 ***********************************************************/

static void switch_default_layer(uint8_t layer)
{
    xprintf("L%d\n", layer);
#ifdef UNIMAP_ENABLE
    default_layer_set(1<<layer);
#else
    default_layer_set(1UL<<layer);
#endif
    clear_keyboard();
}
