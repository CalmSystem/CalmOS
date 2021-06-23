#include "mouse.h"
#include "interrupt.h"
#include "ps2.h"
#include "cpu.h"
#include "stdio.h"

int device;
// int current_byte = 0;
int bytes_per_packet = 3;
// char packet[4] = {0};

void init_mouse(int dev) {
    device = dev;

    mouse_enable_scroll_wheel();
    mouse_enable_five_buttons();

    mouse_set_sample_rate(80);
    mouse_set_resolution(0x00);
    mouse_set_scaling(false);

    ps2_write_device(device, PS2_DEV_ENABLE_SCAN);
    ps2_expect_ack();

    printf("[mouse] Mouse end init\n");
}

void mouse_set_sample_rate(unsigned char rate) {
    ps2_write_device(device, MOUSE_SET_SAMPLE);
    ps2_expect_ack();
    ps2_write_device(device, rate);
    ps2_expect_ack();
}

void mouse_set_scaling(bool enabled) {
    unsigned char cmd = enabled ? MOUSE_ENABLE_SCALING : MOUSE_DISABLE_SCALING;
    ps2_write_device(device, cmd);
    ps2_expect_ack();
}

void mouse_set_resolution(unsigned char level) {
    ps2_write_device(device, MOUSE_SET_RESOLUTION);
    ps2_expect_ack();
    ps2_write_device(device, level);
    ps2_expect_ack();
}

void mouse_enable_scroll_wheel() {
    mouse_set_sample_rate(200);
    mouse_set_sample_rate(100);
    mouse_set_sample_rate(80);

    int type = ps2_identity_device(device);

    if (type == PS2_MOUSE_SCROLL_WHEEL) {
        bytes_per_packet = 4;
        printf("[mouse] Enable scroll wheel.\n");
    } else {
        printf("[mouse] Unable to enable scroll wheel.\n");
    }
}

void mouse_enable_five_buttons() {
    if (bytes_per_packet != 4) {
        return;
    }

    mouse_set_sample_rate(200);
    mouse_set_sample_rate(200);
    mouse_set_sample_rate(80);

    int type = ps2_identity_device(device);

    if (type != PS2_MOUSE_FIVE_BUTTONS) {
        printf("[mouse] Mouse has fewer than five buttons\n");
    } else {
        printf("[mouse] Five buttons enabled\n");
    }
}

unsigned char mouse_cycle = 0;
signed char mouse_byte[3];
signed char mouse_x = 0;
signed char mouse_y = 0;

void mouse_IT()
{
    outb(0x20, 0x20);
    outb(0x20, 0xA0);
    switch (mouse_cycle)
    {
    case 0:
        mouse_byte[0] = inb(0x60);
        mouse_cycle++;
        break;
    case 1:
        mouse_byte[1] = inb(0x60);
        mouse_cycle++;
        break;
    case 2:
        mouse_byte[2] = inb(0x60);
        mouse_x = mouse_byte[1];
        mouse_y = mouse_byte[2];
        mouse_cycle = 0;
        break;
    }
    printf("mx: %d, my: %d\n", mouse_x, mouse_y);
}


/*
void mouse_wait(int rw) {
	int timeout = 100000;
	while ((timeout--) > 0) {
		// All output to port 0x60 or 0x64 must be preceded by waiting for bit 1 (value=2) of port 0x64 to become clear.
		// Similarly, bytes cannot be read from port 0x60 until bit 0 (value=1) of port 0x64 is set.
		if ((!rw && (inb(0x64) & 1) == 1) || (rw && (inb(0x64) & 2) == 0)) {
			timeout = 0;
		}
	}
}

void mouse_write(unsigned char val, unsigned int port) {
  mouse_wait(1);
  outb(val, port);
}

void mouse_write_command(unsigned char cmd) {
  mouse_write(0xD4, 0x64);
  mouse_write(cmd, 0x60);
}

unsigned char mouse_read() {
  mouse_wait(0);
  return inb(0x60);
}

void mouse_init() {
  unsigned char _status;
  mouse_write(0x20, 0x64);
  _status = mouse_read();
  _status = ((_status | (1 << 1)) & ~(1 << 5));
  mouse_write(0x60, 0x64);
  mouse_write(_status, 0x60);

  mouse_write_command(0x76); // default config
  mouse_read(); // Ack

  mouse_write_command(0xF4); // Start sending packets
  mouse_read(); // Ack
}
*/