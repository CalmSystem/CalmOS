#include "keyboard.h"
#include "stdio.h"
#include "string.h"

#include "console.h"
#include "queues.h"

int keyboard_buffer;
int echo_on = 1;

int get_buffer() {
    return keyboard_buffer;
}

void keyboard_data(char *str) {

    int buffer_count;
    int str_len = strlen(str);
    if(echo_on)
        cons_write(str, str_len);
    for (int i = 0; i < str_len; i++)
    {
        char c = str[i];
        pcount(keyboard_buffer, &buffer_count);
        // printf("-> %d '%c'='%d'| ", buffer_count, c, (int)c);
        // Test if size if queue
        if (buffer_count != CONSOLE_COL * CONSOLE_LIG) {
            psend(keyboard_buffer, (int)c);
        } else {
            printf("\n=== Bip! ===\n");
            return;
        }
    }
    // printf("Coucou\n");
}

void init_keyboard_buffer() {
    keyboard_buffer = pcreate(BUFFER_SIZE);
}

int cons_read(void) {
    int msg;
    preceive(keyboard_buffer, &msg);
    return msg;
}

// TODO: doc
unsigned long cons_readline(char *string, unsigned long length) {
    if(length == 0) return 0;
    int msg;
    int buf_save = pcreate(BUFFER_SIZE);
    unsigned long msg_len = 0;
    preceive(keyboard_buffer, &msg);
    while (msg != 13) {
        if(msg == 127) {
            if (msg_len > length) {
                psend(buf_save, msg);
            } else if (msg_len != 0) {
                string--;
                *string = '\0';
            }
            msg_len--;
        } else if (msg_len >= length) {
            psend(buf_save, msg);
            msg_len++;
        } else {
            *string = (char)msg;
            string++;
            msg_len++;
        }
        preceive(keyboard_buffer, &msg);
    }
    if(msg_len >= length) {
        preset(keyboard_buffer);
        int count;
        pcount(buf_save, &count);
        for(int i = 0; i < count; i++){
            preceive(buf_save, &msg);
            psend(keyboard_buffer, msg);
        }
        // If we want auto relaunch...
        // if(msg_len == length) {
            psend(keyboard_buffer, 13);
        // }
    }

    return (msg_len < length) ? msg_len : length;
}

int cons_write(const char *str, long size) {
    for(int i = 0; i < size ; i++){
        char c = str[i];
        // printf("(%d)", (int)c);
        int char_code = (int)c;
        if(char_code == 9 || (char_code >= 32 && char_code <= 126)) {
            console_putbytes(&c, 1);
        }
        else if (char_code == 13) {
            c = 10; // \n
            console_putbytes(&c, 1);
        }
        else if (char_code < 32) {
            printf("^%c", char_code + 64);
        }
        else if (char_code == 127) {
            console_putbytes("\b \b", 3);
        }
    }
    return 0;
}

void cons_echo(int on) {
    if(!on)
        echo_on = 0;
    else
        echo_on = on;
}
