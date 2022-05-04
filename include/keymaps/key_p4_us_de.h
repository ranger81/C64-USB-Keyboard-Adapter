/*********************************************************************
 * key_dk_us.h - Keyboard mapping American C64 keyboard to American  *
 * keyboard setting on the PC side.                                  *
 *********************************************************************
 * Spaceman Spiff's Commodire 64 USB Keyboard (c64key for short) is  *
 * is free software; you can redistribute it and/or modify it under  *
 * the terms of the OBDEV license, as found in the licence.txt file. *
 *                                                                   *
 * c64key is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of    *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the     *
 * OBDEV license for further details.                                *
 *********************************************************************/
#ifndef KEYMAP_H
#define KEYMAP_H
#include <avr/pgmspace.h>

#define PLUS4

/* Number of rows in keyboard matrix */
#define NUMROWS 9

/* The USB keycodes are enumerated here - the first part is simply
   an enumeration of the allowed scan-codes used for USB HID devices */
enum keycodes {
  KEY__=0,
  KEY_errorRollOver,
  KEY_POSTfail,
  KEY_errorUndefined,
  KEY_A,        // 4
  KEY_B,
  KEY_C,
  KEY_D,
  KEY_E,
  KEY_F,
  KEY_G, 
  KEY_H,
  KEY_I,
  KEY_J,
  KEY_K,
  KEY_L,
  KEY_M,        // 0x10
  KEY_N,
  KEY_O,
  KEY_P,
  KEY_Q, 
  KEY_R,
  KEY_S,
  KEY_T,
  KEY_U,
  KEY_V,
  KEY_W,
  KEY_X,
  KEY_Y,
  KEY_Z,
  KEY_1,
  KEY_2,
  KEY_3,        // 0x20
  KEY_4,
  KEY_5,
  KEY_6,
  KEY_7,
  KEY_8,
  KEY_9,
  KEY_0,        // 0x27
  KEY_enter,
  KEY_esc,
  KEY_bckspc,   // backspace
  KEY_tab,
  KEY_spc,      // space
  KEY_minus,    // - (and _)
  KEY_equal,    // =
  KEY_lbr,      // [
  KEY_rbr,      // ]  -- 0x30
  KEY_bckslsh,  // \ (and |)
  KEY_hash,     // Non-US # and ~
  KEY_smcol,    // ; (and :)
  KEY_ping,     // ' and "
  KEY_grave,    // Grave accent and tilde
  KEY_comma,    // , (and <)
  KEY_dot,      // . (and >)
  KEY_slash,    // / (and ?)
  KEY_cpslck,   // capslock
  KEY_F1,
  KEY_F2,
  KEY_F3,
  KEY_F4,
  KEY_F5,
  KEY_F6, 
  KEY_F7,       // 0x40
  KEY_F8,
  KEY_F9,
  KEY_F10,
  KEY_F11,
  KEY_F12,
  KEY_PrtScr,
  KEY_scrlck,
  KEY_break,
  KEY_ins,
  KEY_home,
  KEY_pgup,
  KEY_del,
  KEY_end,
  KEY_pgdn,
  KEY_rarr, 
  KEY_larr,     // 0x50
  KEY_darr,
  KEY_uarr,
  KEY_numlock,
  KEY_KPslash,
  KEY_KPast,
  KEY_KPminus,
  KEY_KPplus,
  KEY_KPenter,
  KEY_KP1,
  KEY_KP2,
  KEY_KP3,
  KEY_KP4,
  KEY_KP5,
  KEY_KP6,
  KEY_KP7,
  KEY_KP8,      // 0x60
  KEY_KP9,
  KEY_KP0,
  KEY_KPcomma,
  KEY_Euro2,

  /* These are NOT standard USB HID - handled specially in decoding,
     so they will be mapped to the modifier byte in the USB report */
  KEY_Modifiers,
  MOD_LCTRL,    // 0x01
  MOD_LSHIFT,   // 0x02
  MOD_LALT,     // 0x04
  MOD_LGUI,     // 0x08
  MOD_RCTRL,    // 0x10
  MOD_RSHIFT,   // 0x20
  MOD_RALT,     // 0x40
  MOD_RGUI,     // 0x80
  
  /* Other keys that need special handling -
     These are looked up in the table spec_keys because they do not
     generate the same scan-code in the shifted and unshifted state,
     and some may need to alter the shift-state to generate the
     correct character code on the PC */
  KEY_Special,
  SPC_del,
  SPC_F1,
  SPC_F2,
  SPC_F3,
  SPC_HELP,
  SPC_crsru,
  SPC_crsrd,
  SPC_crsrl,
  SPC_crsrr
};

/* The keymap for the Commodore 64 keyboard with Danish keys mapping
   to a PC with danish keyboard mapping */
const unsigned char keymap[NUMROWS][8] PROGMEM = { // German keymap (Plus4)
    {SPC_del, 		KEY_3, 		KEY_5, 		KEY_7, 		KEY_9, 		SPC_crsrd, 		SPC_crsrl, 		KEY_1}, 		// row0
    {KEY_enter, 	KEY_W, 		KEY_R, 		KEY_Y, 		KEY_I, 		KEY_P, 			KEY_bckslsh, 	KEY_tab},		// row1
    {KEY_rbr/*pound*/, 		KEY_A, 		KEY_D, 		KEY_G, 		KEY_J, 		KEY_L, 			KEY_ping, 		MOD_LCTRL}, 	// row2
    {SPC_HELP, 		KEY_4, 		KEY_6, 		KEY_8, 		KEY_0, 		SPC_crsru, 		SPC_crsrr, 		KEY_2}, 		// row3
    {SPC_F1, 		KEY_Z, 		KEY_C, 		KEY_B, 		KEY_M, 		KEY_dot, 		KEY_esc, 		KEY_spc}, 		// row4
    {SPC_F2, 		KEY_S, 		KEY_F, 		KEY_H, 		KEY_K, 		KEY_smcol, 		KEY_grave/*=*/, 			MOD_LGUI}, 		// row5
    {SPC_F3, 		KEY_E, 		KEY_T, 		KEY_U, 		KEY_O, 		KEY_equal/*-*/, 	KEY_minus, 		KEY_Q}, 		// row6
    {KEY_lbr, 		MOD_LSHIFT, KEY_X, 		KEY_V, 		KEY_N, 		KEY_comma, 		KEY_slash, 		MOD_RALT}, 		// row7
    {0, 0, 0, KEY_3/*x*/, 0, 0, 0, 0} // Imaginary row8 is for restore
  };

/* Special keys that need to generate different scan-codes for unshifted
   and shifted states, or that need to alter the modifier keys. 
   Since the LGUI and RGUI bits are not used, these signify that the
   left and right shift states should be deleted from report, so
     0x88 means clear both shift flags
     0x00 means do not alter shift states
     0xC8 means clear both shifts and set R_ALT */
const unsigned char spec_keys[9][4] PROGMEM = {
  { KEY_bckspc,  0x00, KEY_del,     0x88}, // SPC_del - backspace and delete
  { KEY_F1,      0x00, KEY_F5,      0x88}, // SPC_F1 - F1 and F2
  { KEY_F2,      0x00, KEY_F6,      0x88}, // SPC_F2 - F3 and F4
  { KEY_F3,      0x00, KEY_F7,      0x88}, // SPC_F3 - F5 and F6
  { KEY_F4,      0x00, KEY_F8,      0x88}, // SPC_HELP - F7 and F8
  { KEY_uarr,    0x00, KEY_pgup,    0x88}, // SPC_crsru - up
  { KEY_darr,    0x00, KEY_pgdn,    0x88}, // SPC_crsrd - down
  { KEY_larr,    0x00, KEY_home,    0x88}, // SPC_crsrl - left
  { KEY_rarr,    0x00, KEY_end,     0x88} // SPC_crsrr - right
};
#endif
