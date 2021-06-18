#include "keyboard.h"
#include "stdio.h"

#include "console.h"
#include "queues.h"

void keyboard_data(char *str) {

    int buffer_count;
    // '\0' or NULL ??
    for (int i = 0; str[i] != '\0'; i++)
    {
        pcount(keyboard_buffer, &buffer_count);
        // Test if size if queue
        if (buffer_count != CONSOLE_COL*CONSOLE_LIG)
            psend(keyboard_buffer, (int)str[i]);
        else
            return;
    }
    printf("Coucou\n");
}

// void cons_echo(int on);
// unsigned long cons_read(char *string, unsigned long length);
// int cons_write(const char *str, long size);