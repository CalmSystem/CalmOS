#include "console.h"
#include "queues.h"
#include "stdio.h"
#include "string.h"
#include "beep.h"
#include "keyboard.h"

int keyboard_buffer;
int echo_on = 1;

int get_buffer() { return keyboard_buffer; }

void keyboard_data(char *str) {
  int buffer_count;
  int str_len = strlen(str);
  if (echo_on || *str == '\r') echo(str, str_len);
  for (int i = 0; i < str_len; i++) {
    char c = str[i];
    pcount(keyboard_buffer, &buffer_count);
    if (buffer_count != BUFFER_SIZE) {
      psend(keyboard_buffer, (int)c);
    } else {
      async_beep(1000, .1);
      return;
    }
  }
}

void init_keyboard_buffer() { keyboard_buffer = pcreate(BUFFER_SIZE); }

int cons_read(void) {
  int msg;
  preceive(keyboard_buffer, &msg);
  return msg;
}

unsigned long cons_readline(char *string, unsigned long length) {
  if (length == 0) return 0;
  int msg;
  int buf_save[BUFFER_SIZE];
  int buf_idx = 0;
  unsigned long msg_len = 0;
  preceive(keyboard_buffer, &msg);
  while (msg != 13) {
    if (msg == 127) {
      if (msg_len > length) {
        buf_save[buf_idx] = msg;
        buf_idx++;
      } else if (msg_len > 0) {
        string--;
        *string = '\0';
      }
      if(msg_len > 0){
        msg_len--;
      }
    } else if (msg_len >= length) {
      buf_save[buf_idx] = msg;
      buf_idx++;
      msg_len++;
    } else {
      *string = (char)msg;
      string++;
      msg_len++;
    }
    preceive(keyboard_buffer, &msg);
  }
  if (msg_len >= length) {
    preset(keyboard_buffer);
    for (int i = 0; i < buf_idx; i++) {
      psend(keyboard_buffer, buf_save[i]);
    }
    psend(keyboard_buffer, 13);
  }

  return (msg_len < length) ? msg_len : length;
}

int random_beep(void* arg){
  console_putbytes("\a", 1);
  return (int)arg;
}

// Piano mode
int piano = 0;

int echo(const char *str, long size) {
  for (int i = 0; i < size; i++) {
    char c = str[i];
    int char_code = (int)c;
    if(piano){ // Piano mode
      switch (char_code)
      {
      case 16: // CTRL + P
        piano = 0;
        break;
      case 113:
        async_beep(262, .1);
        break;
      case 122:
        async_beep(277, .1);
        break;
      case 115:
        async_beep(294, .1);
        break;
      case 101:
        async_beep(311, .1);
        break;
      case 100:
        async_beep(330, .1);
        break;
      case 102:
        async_beep(349, .1);
        break;
      case 116:
        async_beep(370, .1);
        break;
      case 103:
        async_beep(392, .1);
        break;
      case 121:
        async_beep(415, .1);
        break;
      case 104:
        async_beep(440, .1);
        break;
      case 117:
        async_beep(466, .1);
        break;
      case 106:
        async_beep(494, .1);
        break;
      case 107:
        async_beep(523, .1);
        break;
      }
      return 0;
    }
    if (char_code == 9 || (char_code >= 32 && char_code <= 126)) {
      console_putbytes(&c, 1);
    } else if (char_code == 13) {
      c = 10;  // \n
      console_putbytes(&c, 1);
    } else if (char_code < 32) {
      if(char_code == 12) {
        console_putbytes("\f", 1);
      } else if (char_code == 7){
        async_beep(392, .1);
      } else if (char_code == 16){ // CTRL + P
        piano = 1;
      } else {
        printf("^%c", char_code + 64);
      }
    } else if (char_code == 127) {
      console_putbytes("\b \b", 3);
    }
  }
  return 0;
}
int cons_write(const char *str, long size) {
  console_putbytes(str, size);
  return size;
}

void cons_echo(int on) { echo_on = on; }
