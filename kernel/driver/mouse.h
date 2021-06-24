#ifndef MOUSE_H_
#define MOUSE_H_

#include "stdbool.h"

// Specific commands
#define MOUSE_SET_SAMPLE 0xF3
#define MOUSE_SET_RESOLUTION 0xE8
#define MOUSE_ENABLE_SCALING 0xE7
#define MOUSE_DISABLE_SCALING 0xE7

typedef struct {
    int x, y;
    bool left_button_pressed;
    bool right_button_pressed;
    bool middle_button_pressed;
} ps2_mouse_t;

ps2_mouse_t mouse_previous;

typedef void (*ps2_mouse_callback_t)(ps2_mouse_t);

// Bits of the first mouse data packet
#define MOUSE_Y_OVERFLOW (1 << 7)
#define MOUSE_X_OVERFLOW (1 << 6)
#define MOUSE_Y_NEG (1 << 5)
#define MOUSE_X_NEG (1 << 4)
#define MOUSE_ALWAYS_SET (1 << 3)
#define MOUSE_MIDDLE (1 << 2)
#define MOUSE_RIGHT (1 << 1)
#define MOUSE_LEFT (1 << 0)

// Bits of the fourth mouse data packet
#define MOUSE_UNUSED_A (1 << 7)
#define MOUSE_UNUSED_B (1 << 6)

/* Initialise the PS2 second device (mouse) */
void init_mouse(int dev);

/* Set different parameters for the mouse */
void mouse_set_sample_rate(unsigned char rate);
void mouse_set_scaling(bool enabled);
void mouse_set_resolution(unsigned char level);

/* Initialize different type of option for the mouse */
void mouse_enable_scroll_wheel();
void mouse_enable_five_buttons();

/* Another initializing function, simplier but without tests */
// void mouse_init();

/* mouse interruption handler */
void mouse_IT();

/* Sets the mouse callback function */
void mouse_set_callback(ps2_mouse_callback_t cb);

/* Allows to 'decode' mouse packets from the PS/2 data port */
void mouse_handle_packet();

#endif /*MOUSE_H_*/