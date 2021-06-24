#include "ps2.h"
#include "mouse.h"
#include "cpu.h"
#include "stdio.h"

/**
 * This code is based on SnowFlakesOS: https://github.com/29jm/SnowflakeOS
 * Made by Johan Manuel, under MIT Licence.
 * There is only some small changes in this code so we can say it's kind of copy
 * & paste.
 */


bool all_controllers[] = {true, true};

/**
 * In the above functions, most of the comments are configuration for keyboard.
 * We've commented it because keyboard init is made elsewhere in the code but we
 * wanted to keep it in case we want to use it later...
 */

void init_ps2() {
    //printf("[ps2] Initializing\n");

    bool dual_channel = true;

    // ps2_write(PS2_DISABLE_FIRST, PS2_CMD_PORT);
    ps2_write(PS2_DISABLE_SECOND, PS2_CMD_PORT);

    inb(PS2_DATA_PORT);

    ps2_write(PS2_READ_CONFIG, PS2_CMD_PORT);
    unsigned char ps2_config = ps2_read(PS2_DATA_PORT);

    // ps2_config |= PS2_CFG_SYSTEM_FLAG;
    ps2_config |= 2;

    // if (ps2_config & PS2_CFG_MUST_BE_ZERO) {
    //     //printf("[ps2] Invalid bit set in configuration byte.\n");
    // }

    // ps2_config &= ~(PS2_CFG_FIRST_PORT | PS2_CFG_SECOND_PORT | PS2_CFG_TRANSLATION);
    // ps2_config &= ~(PS2_CFG_SECOND_PORT);

    ps2_write(PS2_WRITE_CONFIG, PS2_CMD_PORT);
    ps2_write(ps2_config, PS2_DATA_PORT);

    ps2_write(PS2_SELF_TEST, PS2_CMD_PORT);

    if(ps2_read(PS2_DATA_PORT) != PS2_SELF_TEST_OK) {
        //printf("[ps2] Controller failed self-test\n");
        // all_controllers[0] = false;
        all_controllers[1] = false;
        return;
    }

    ps2_write(PS2_WRITE_CONFIG, PS2_CMD_PORT);
    ps2_write(ps2_config, PS2_DATA_PORT);

    ps2_write(PS2_ENABLE_SECOND, PS2_CMD_PORT);
    ps2_write(PS2_READ_CONFIG, PS2_CMD_PORT);
    ps2_config = ps2_read(PS2_DATA_PORT);

    if (ps2_config & PS2_CFG_SECOND_CLOCK) {
        //printf("[ps2] Only one PS/2 controller\n");
        dual_channel = false;
        all_controllers[1] = false;
    } else {
        ps2_write(PS2_DISABLE_SECOND, PS2_CMD_PORT);
    }

    // ps2_write(PS2_TEST_FIRST, PS2_CMD_PORT);

    // if (ps2_read(PS2_DATA_PORT) != PS2_TEST_OK) {
    //     //printf("[ps2] First PS/2 port failed testing\n");
    //     all_controllers[0] = false;
    // }

    if(dual_channel) {
        ps2_write(PS2_TEST_SECOND, PS2_CMD_PORT);

        if (ps2_read(PS2_DATA_PORT) != PS2_TEST_OK) {
            //printf("[ps2] Second PS/2 port failed testing\n");
            all_controllers[1] = false;
        }
    }

    // for (int i = 0 ; i < 2; i++) {
    for (int i = 1 ; i < 2; i++) {
        if (!all_controllers[i]) {
            continue;
        }

        ps2_enable_port(i, true);

        //printf("[ps2] Resetting device %d\n", i);
        ps2_write_device(i, PS2_DEV_RESET);
        ps2_wait_ms(500);
        ps2_expect_ack();
        ps2_wait_ms(100);

        unsigned char ret = ps2_read(PS2_DATA_PORT);

        // if(i == 0 && ret != PS2_DEV_RESET_ACK) {
        //     //printf("[ps2] Keyboard failed to ack rest, sent %x\n", ret);
        //     goto error;
        // } else
        if (i == 1) {
            unsigned char ret2 = ps2_read(PS2_DATA_PORT);

            if ((ret == PS2_DEV_RESET_ACK && (ret2 == PS2_DEV_ACK || ret2 == 0x00)) ||
               ((ret == PS2_DEV_ACK || ret == 0x00) && ret2 == PS2_DEV_RESET_ACK)) {
                   // empty
            } else {
                //printf("[ps2] Mice failed to ack reset, sent %x, %x\n", ret, ret2);
                goto error;
            }
        }

        // if (i == 0) {
        //     ps2_read(PS2_DATA_PORT);
        //     ps2_enable_port(i, false);
        // }
        continue;

    error:
        ps2_enable_port(i, false);
        all_controllers[i] = false;
    }

    // if (all_controllers[0]) {
    //     ps2_enable_port(0, true);
    // }

    // for (int i = 0; i < 2; i++) {
    for (int i = 1; i < 2; i++) {
        if (all_controllers[i]) {
            int type = ps2_identity_device(i);
            bool driver_called = true;

            switch (type)
            {
                case PS2_KEYBOARD:
                case PS2_KEYBOARD_TRANSLATED:
                    //printf("[ps2] Keyboard\n");
                    // init_kbd();
                    break;
                case PS2_MOUSE:
                case PS2_MOUSE_SCROLL_WHEEL:
                case PS2_MOUSE_FIVE_BUTTONS:
                    //printf("[ps2] Device %d is: Mouse\n", i);
                    init_mouse(i);
                    break;
                default:
                    driver_called = false;
                    break;
            }

            if (driver_called) {
                ps2_config |= i == 0 ? PS2_CFG_FIRST_PORT : PS2_CFG_SECOND_PORT;
                ps2_config &= ~(i == 0 ? PS2_CFG_FIRST_CLOCK : PS2_CFG_SECOND_CLOCK);
            }
        }
    }

    ps2_write(PS2_WRITE_CONFIG, PS2_CMD_PORT);
    ps2_write(ps2_config, PS2_DATA_PORT);

    //printf("[ps2] Init ok\n");

}

void ps2_wait_ms(int ms) {
    int i = 0;
    while( i++ < 1000 * ms) {
        __asm__ __volatile__ ("pause");
    }
}

bool ps2_expect_ack() {
    unsigned char ret = ps2_read(PS2_DATA_PORT);
    if (ret != PS2_DEV_ACK) {
        //printf("[ps2] Device failed to acknowledge command, sent %x\n", ret);
        return false;
    }
    return true;
}

void ps2_enable_port(int i, bool enable) {
    if (!all_controllers[i])
        return;
    if (!enable)
        ps2_write(i == 0 ? PS2_DISABLE_FIRST : PS2_DISABLE_SECOND, PS2_CMD_PORT);
    else
        ps2_write(i == 0 ? PS2_ENABLE_FIRST : PS2_ENABLE_SECOND, PS2_CMD_PORT);
    // Flush data port
    ps2_read(PS2_DATA_PORT);
}

bool ps2_write_device(int device, unsigned char value) {
    if (device != 0) {
        if (!ps2_write(PS2_WRITE_SECOND, PS2_CMD_PORT)) {
            return false;
        }
    }
    return ps2_write(value, PS2_DATA_PORT);
}

bool ps2_wait_and_write() {
    int timer = PS2_TIMEOUT;
    while(!(inb(PS2_CMD_PORT) & 2) && timer-- > 0) {
        __asm__ __volatile__ ("pause");
    }
    return timer != 0;
}

bool ps2_wait_and_read() {
    int timer = PS2_TIMEOUT;
    while (!(inb(PS2_CMD_PORT) & 1) && timer-- >= 0)
    {
        __asm__ __volatile__("pause");
    }
    return timer != 0;
}

unsigned char ps2_read(unsigned short port) {
    if(ps2_wait_and_read()) {
        return inb(port);
    }
    //printf("[ps2] Error reading\n");
    return -1;
}

bool ps2_write(unsigned char value, unsigned short port) {
    if(ps2_wait_and_write()) {
        outb(value, port);
        return true;
    }
    //printf("[ps2] Error writing\n");
    return false;
}

int ps2_identity_bytes_to_type(int first, int second) {
    if (first == 0x00 || first == 0x03 || first == 0x04) {
        return first; // Ps2 mouse
    } else if (first == 0xAB) {
        if (second == 0x41 || second == 0xC1) {
            return PS2_KEYBOARD_TRANSLATED;
        } else if (second == 0x83) {
            return PS2_KEYBOARD;
        }
    }
    return PS2_DEVICE_UNKNOWN;
}

int ps2_identity_device(int i) {
    //printf("[ps2] Identifying device %d\n", i);
    ps2_write_device(i, PS2_DEV_DISABLE_SCAN);
    ps2_expect_ack();
    ps2_write_device(i, PS2_DEV_IDENTIFY);
    ps2_expect_ack();

    int first_id_byte = ps2_read(PS2_DATA_PORT);
    int second_id_byte = ps2_read(PS2_DATA_PORT);

    return ps2_identity_bytes_to_type(first_id_byte, second_id_byte);
}