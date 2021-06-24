#include "stdio.h"
#include "interrupt.h"
#include "scheduler.h"
#include "cpu.h"
#include "string.h"
#include "floppy.h"

/** Code adapted from OSDev forum */
/******************************************************************************
* Copyright (c) 2007 Teemu Voipio                                             *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL    *
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER  *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
******************************************************************************/

#define FLOPPY_BASE 0x03f0

#define FLOPPY_144_CODE 4
#define FLOPPY_144_SECTORS_PER_TRACK 18
#define FLOPPY_144_SECTOR_SIZE 512
#define FLOPPY_144_CYLINDER_FACE_SIZE (FLOPPY_144_SECTORS_PER_TRACK * FLOPPY_144_SECTOR_SIZE)
#define FLOPPY_144_HEADS 2
#define FLOPPY_144_CYLINDER_SIZE (FLOPPY_144_CYLINDER_FACE_SIZE * FLOPPY_144_HEADS)

// we statically reserve a totally uncomprehensive amount of memory
// must be large enough for whatever DMA transfer we might desire
// and must not cross 64k borders so easiest thing is to align it
// to 2^N boundary at least as big as the block
extern char floppy_dmabuf[FLOPPY_144_CYLINDER_SIZE];
unsigned floppy_dmacyl = -1;

bool interrupted = false;
void floppy_IT() {
  outb(0x20, 0x20);
  interrupted = true;
}

enum registers {
  /** digital output register */
  DOR  = 2,
  /** master status register, read only */
  MSR  = 4,
  /** data FIFO, in DMA operation for commands */
  FIFO = 5,
  /** configuration control register, write only */
  CCR  = 7 
};

enum commands {
  CMD_SPECIFY = 3,
  CMD_WRITE_DATA = 5,
  CMD_READ_DATA = 6,
  CMD_RECALIBRATE = 7,
  CMD_SENSE_INTERRUPT = 8,
  CMD_SEEK = 15
};

static const char * drive_types[8] = {
    "none",
    "360kB 5.25\"",
    "1.2MB 5.25\"",
    "720kB 3.5\"",

    "1.44MB 3.5\"",
    "2.88MB 3.5\"",
    "unknown type",
    "unknown type"
};


//
// The MSR byte: [read-only]
// -------------
//
//  7   6   5    4    3    2    1    0
// MRQ DIO NDMA BUSY ACTD ACTC ACTB ACTA
//
// MRQ is 1 when FIFO is ready (test before read/write)
// DIO tells if controller expects write (1) or read (0)
//
// NDMA tells if controller is in DMA mode (1 = no-DMA, 0 = DMA)
// BUSY tells if controller is executing a command (1=busy)
//
// ACTA, ACTB, ACTC, ACTD tell which drives position/calibrate (1=yes)
//
//

/** Sleep delay in ms */
void timer_sleep(int ms) {
  unsigned long quartz;
  unsigned long ticks;
  clock_settings(&quartz, &ticks);
  wait_clock(current_clock() + (quartz / ticks) * (unsigned long)ms / 1000l);
}

void wait_interrupt() {
  while (!interrupted) timer_sleep(10);
}

void write_cmd(int base, char cmd) {
  int i; // do timeout, 60 seconds
  for(i = 0; i < 600; i++) {
    timer_sleep(10);
    if(0x80 & inb(base+MSR)) {
      outb(cmd, base+FIFO);
      return;
    }
  }
  assert(0);
}
unsigned char read_data(int base) {
  int i; // do timeout, 60 seconds
  for(i = 0; i < 600; i++) {
    timer_sleep(10);
    if(0x80 & inb(base+MSR)) {
      return inb(base+FIFO);
    }
  }
  assert(0);
  return 0; // not reached
}


//
// The DOR byte: [write-only]
// -------------
//
//  7    6    5    4    3   2    1   0
// MOTD MOTC MOTB MOTA DMA NRST DR1 DR0
//
// DR1 and DR0 together select "current drive" = a/00, b/01, c/10, d/11
// MOTA, MOTB, MOTC, MOTD control motors for the four drives (1=on)
//
// DMA line enables (1 = enable) interrupts and DMA
// NRST is "not reset" so controller is enabled when it's 1
//
enum { motor_off = 0, motor_on, motor_wait };
static volatile int motor_ticks = 0;
static volatile int motor_state = 0;

void motor(int base, int onoff) {
  if(onoff) {
    if(!motor_state) {
      // need to turn on
      outb(0x1c, base + DOR);
      timer_sleep(500); // wait 500 ms = hopefully enough for modern drives
    }
    motor_state = motor_on;
  } else {
    if(motor_state == motor_wait) {
      printf("motor: strange, fd motor-state already waiting..\n");
    }
    motor_ticks = 300; // 3 seconds, see timer() below
    motor_state = motor_wait;
  }
}

void motor_kill(int base) {
  outb(0x0c, base + DOR);
  motor_state = motor_off;
}

//
// THIS SHOULD BE STARTED IN A SEPARATE THREAD.
//
//
int sleep_timer(void* arg) {
  (void)arg;
  while (1) {
    // sleep for 500ms at a time, which gives around half
    // a second jitter, but that's hardly relevant here.
    timer_sleep(500);
    if(motor_state == motor_wait) {
      motor_ticks -= 50;
      if(motor_ticks <= 0) {
        motor_kill(FLOPPY_BASE);
      }
    }
  }
}

void check_interrupt(int base, int *st0, int *cyl) {
   
  write_cmd(base, CMD_SENSE_INTERRUPT);

  *st0 = read_data(base);
  *cyl = read_data(base);
}

// Move to cylinder 0, which calibrates the drive..
int calibrate(int base) {

  int i, st0, cyl = -1; // set to bogus cylinder

  motor(base, motor_on);

  for(i = 0; i < 10; i++) {
    interrupted = false;
    // Attempt to positions head to cylinder 0
    write_cmd(base, CMD_RECALIBRATE);
    write_cmd(base, 0); // argument is drive, we only support 0

    wait_interrupt();
    check_interrupt(base, &st0, &cyl);
    
    if(st0 & 0xC0) {
      static const char * status[] =
      { 0, "error", "invalid", "drive" };
      printf("calibrate: status = %s\n", status[st0 >> 6]);
      continue;
    }

    if(!cyl) { // found cylinder 0 ?
      motor(base, motor_off);
      return 0;
    }
  }

  printf("calibrate: 10 retries exhausted\n");
  motor(base, motor_off);
  return -1;
}


int reset(int base) {

  interrupted = false;
  outb(0x00, base + DOR); // disable controller
  outb(0x0C, base + DOR); // enable controller

  wait_interrupt(); // sleep until interrupt occurs

  {
    int st0, cyl; // ignore these here..
    check_interrupt(base, &st0, &cyl);
  }

  // set transfer speed 500kb/s
  outb(0x00, base + CCR);

  //  - 1st byte is: bits[7:4] = steprate, bits[3:0] = head unload time
  //  - 2nd byte is: bits[7:1] = head load time, bit[0] = no-DMA
  //
  //  steprate    = (8.0ms - entry*0.5ms)*(1MB/s / xfer_rate)
  //  head_unload = 8ms * entry * (1MB/s / xfer_rate), where entry 0 -> 16
  //  head_load   = 1ms * entry * (1MB/s / xfer_rate), where entry 0 -> 128
  //
  write_cmd(base, CMD_SPECIFY);
  write_cmd(base, 0xdf); /* steprate = 3ms, unload time = 240ms */
  write_cmd(base, 0x02); /* load time = 16ms, no-DMA = 0 */

  // it could fail...
  if(calibrate(base)) return -1;
   
  return 0;
}

// Seek for a given cylinder, with a given head
int seek(int base, unsigned cyli, int head) {

  unsigned i, st0, cyl = -1; // set to bogus cylinder

  motor(base, motor_on);

  for(i = 0; i < 10; i++) {
    interrupted = false;
    // Attempt to position to given cylinder
    // 1st byte bit[1:0] = drive, bit[2] = head
    // 2nd byte is cylinder number
    write_cmd(base, CMD_SEEK);
    write_cmd(base, head<<2);
    write_cmd(base, cyli);

    wait_interrupt();
    check_interrupt(base, (int*)&st0, (int*)&cyl);

    if(st0 & 0xC0) {
      static const char * status[] =
      { "normal", "error", "invalid", "drive" };
      printf("seek: status = %s\n", status[st0 >> 6]);
      continue;
    }

    if(cyl == cyli) {
      motor(base, motor_off);
      return 0;
    }

  }

  printf("seek: 10 retries exhausted\n");
  motor(base, motor_off);
  return -1;
}

// Used by dma_init and do_track to specify direction
typedef enum {
    dir_read = 1,
    dir_write = 2
} dir;


static void dma_init(dir dir) {

  union {
    unsigned char b[4]; // 4 bytes
    unsigned long l;    // 1 long = 32-bit
  } a, c; // address and count

  a.l = (unsigned) &floppy_dmabuf;
  c.l = (unsigned) FLOPPY_144_CYLINDER_SIZE - 1;  // -1 because of DMA counting

  // check that address is at most 24-bits (under 16MB)
  // check that count is at most 16-bits (DMA limit)
  // check that if we add count and address we don't get a carry
  // (DMA can't deal with such a carry, this is the 64k boundary limit)
  if((a.l >> 24) || (c.l >> 16) || (((a.l&0xffff)+c.l)>>16)) {
    panic("dma_init: static buffer problem\n");
  }

  unsigned char mode;
  switch(dir) {
    // 01:0:0:01:10 = single/inc/no-auto/to-mem/chan2
    case dir_read:  mode = 0x46; break;
    // 01:0:0:10:10 = single/inc/no-auto/from-mem/chan2
    case dir_write: mode = 0x4a; break;
    default: panic("dma_init: invalid direction");
              return; // not reached, please "mode user uninitialized"
  }

  outb(0x06, 0x0a);   // mask chan 2

  outb(0xff, 0x0c);   // reset flip-flop
  outb(a.b[0], 0x04); //  - address low byte
  outb(a.b[1], 0x04); //  - address high byte

  outb(a.b[2], 0x81); // external page register

  outb(0xff, 0x0c);   // reset flip-flop
  outb(c.b[0], 0x05); //  - count low byte
  outb(c.b[1], 0x05); //  - count high byte

  outb(mode, 0x0b);   // set mode (see above)

  outb(0x02, 0x0a);   // unmask chan 2
}

int sleep_timer_pid = -1;

// This monster does full cylinder (both tracks) transfer to
// the specified direction (since the difference is small).
//
// It retries (a lot of times) on all errors except write-protection
// which is normally caused by mechanical switch on the disk.
//
int do_track(int base, unsigned cyl, dir dir) {
   
  // transfer command, set below
  unsigned char cmd;

  // Read is MT:MF:SK:0:0:1:1:0, write MT:MF:0:0:1:0:1
  // where MT = multitrack, MF = MFM mode, SK = skip deleted
  //
  // Specify multitrack and MFM mode
  static const int flags = 0xC0;
  switch(dir) {
    case dir_read:
        cmd = CMD_READ_DATA | flags;
        break;
    case dir_write:
        cmd = CMD_WRITE_DATA | flags;
        break;
    default:

        panic("do_track: invalid direction");
        return 0; // not reached, but pleases "cmd used uninitialized"
  }

  // start process dealing with floppy sleep
  if (!sleep_timer_pid) sleep_timer_pid = start_background(sleep_timer, 128, 1, "floppy_sleep_timer", NULL);

  // seek both heads
  if(seek(base, cyl, 0)) return -1;
  if(seek(base, cyl, 1)) return -1;

  int i;
  for(i = 0; i < 20; i++) {
    motor(base, motor_on);

    // init dma..
    dma_init(dir);

    timer_sleep(100); // give some time (100ms) to settle after the seeks

    interrupted = false;

    write_cmd(base, cmd);  // set above for current direction
    write_cmd(base, 0);    // 0:0:0:0:0:HD:US1:US0 = head and drive
    write_cmd(base, cyl);  // cylinder
    write_cmd(base, 0);    // first head (should match with above)
    write_cmd(base, 1);    // first sector, strangely counts from 1
    write_cmd(base, 2);    // bytes/sector, 128*2^x (x=2 -> 512)
    write_cmd(base, 18);   // number of tracks to operate on
    write_cmd(base, 0x1b); // GAP3 length, 27 is default for 3.5"
    write_cmd(base, 0xff); // data length (0xff if B/S != 0)
    
    wait_interrupt(); // don't SENSE_INTERRUPT here!

    // first read status information
    unsigned char st0, st1, st2, rcy, rhe, rse, bps;
    st0 = read_data(base);
    st1 = read_data(base);
    st2 = read_data(base);
    /*
      * These are cylinder/head/sector values, updated with some
      * rather bizarre logic, that I would like to understand.
      *
      */
    rcy = read_data(base);
    rhe = read_data(base);
    rse = read_data(base);
    // bytes per sector, should be what we programmed in
    bps = read_data(base);

    (void)rse;
    (void)rhe;
    (void)rcy;

    int error = 0;

    if(st0 & 0xC0) {
      static const char * status[] =
      { 0, "error", "invalid command", "drive not ready" };
      printf("do_sector: status = %s\n", status[st0 >> 6]);
      error = 1;
    }
    if(st1 & 0x80) {
      printf("do_sector: end of cylinder\n");
      error = 1;
    }
    if(st0 & 0x08) {
      printf("do_sector: drive not ready\n");
      error = 1;
    }
    if(st1 & 0x20) {
      printf("do_sector: CRC error\n");
      error = 1;
    }
    if(st1 & 0x10) {
      printf("do_sector: controller timeout\n");
      error = 1;
    }
    if(st1 & 0x04) {
      printf("do_sector: no data found\n");
      error = 1;
    }
    if((st1|st2) & 0x01) {
      printf("do_sector: no address mark found\n");
      error = 1;
    }
    if(st2 & 0x40) {
      printf("do_sector: deleted address mark\n");
      error = 1;
    }
    if(st2 & 0x20) {
      printf("do_sector: CRC error in data\n");
      error = 1;
    }
    if(st2 & 0x10) {
      printf("do_sector: wrong cylinder\n");
      error = 1;
    }
    if(st2 & 0x04) {
      printf("do_sector: uPD765 sector not found\n");
      error = 1;
    }
    if(st2 & 0x02) {
      printf("do_sector: bad cylinder\n");
      error = 1;
    }
    if(bps != 0x2) {
      printf("do_sector: wanted 512B/sector, got %d", (1<<(bps+7)));
      error = 1;
    }
    if(st1 & 0x02) {
      printf("do_sector: not writable\n");
      error = 2;
    }

    if(!error) {
      motor(base, motor_off);
      return 0;
    }
    if(error > 1) {
      printf("do_sector: not retrying..\n");
      motor(base, motor_off);
      return -2;
    }
  }

  printf("do_sector: 20 retries exhausted\n");
  motor(base, motor_off);
  return -1;

}

void addr_2_coff(uint32_t addr, uint16_t* cyl, uint32_t* offset, uint32_t* size) {
  if (offset) *offset = addr % FLOPPY_144_CYLINDER_SIZE;
  if (size) *size = FLOPPY_144_CYLINDER_SIZE - (addr % FLOPPY_144_CYLINDER_SIZE);
  if (cyl) *cyl = addr / FLOPPY_144_CYLINDER_SIZE;
}

int do_cached(dir dir, uint32_t addr, uint32_t* offset, uint32_t* size) {
  if (!has_floppy()) return -2;

  uint16_t cyl;
  addr_2_coff(addr, &cyl, offset, size);
  // Already in floppy_dmabuf
  if (dir == dir_read && cyl == floppy_dmacyl) return 0;
  int ret = do_track(FLOPPY_BASE, cyl, dir);
  if (ret >= 0) floppy_dmacyl = cyl;
  return ret;
}

const char* floppy_read_view(uint32_t addr, uint32_t* size) {
  uint32_t offset;
  int ret = do_cached(dir_read, addr, &offset, size);
  if (ret < 0) return NULL;

  return &floppy_dmabuf[offset];
}
int floppy_read(char* dst, uint32_t addr, uint32_t size) {
  uint32_t offset, working_size, done_size = 0;
  int ret;
  // Reading cylinders one by one
  while (size > done_size) {
    ret = do_cached(dir_read, addr + done_size, &offset, &working_size);
    if (ret < 0) return ret;

    if (working_size > size - done_size) working_size = size - done_size;
    memcpy(dst + done_size, &floppy_dmabuf[offset], working_size);
    done_size += working_size;
  }
  return 0;
}

int floppy_write(uint32_t addr, const char* src, uint32_t size) {
  uint32_t offset, working_size, done_size = 0;
  int ret;
  // Writing cylinders one by one
  while (size > done_size) {
    // Load cylinder
    ret = do_cached(dir_read, addr + done_size, &offset, &working_size);
    if (ret < 0) return ret;

    if (working_size > size - done_size) working_size = size - done_size;
    // Edit buffer
    memcpy(&floppy_dmabuf[offset], src + done_size, working_size);

    // Save cylinder
    ret = do_cached(dir_write, addr + done_size, NULL, NULL);
    if (ret < 0) return ret;
    done_size += working_size;
  }
  return 0;
}

bool has_floppy() {
  // Read drives list in CMOS
  outb(0x10, 0x70);
  uint8_t drives = inb(0x71);
  uint8_t drive0 = drives >> 4;
  if (drive0 == 0) {
    printf("No floppy disk found\n");
    return false;
  }
  if (drive0 != FLOPPY_144_CODE) {
    printf("Floppy driver only support %s got %s\n",
           drive_types[FLOPPY_144_CODE], drive_types[drive0]);
    return false;
  }
  return true;
}

void floppy_disk_write(void *arg, uint32_t addr, const void *src, size_t n) {
  (void)arg;
  int res;
  (void)res;
  res = floppy_write(addr, src, n);
  assert(res >= 0);
}
void floppy_disk_read(void *arg, void *dst, uint32_t addr, size_t n) {
  (void)arg;
  int res;
  (void)res;
  res = floppy_read(dst, addr, n);
  assert(res >= 0);
}
const void *floppy_disk_view(void *arg, uint32_t addr, uint32_t *size) {
  (void)arg;
  const void *res = floppy_read_view(addr, size);
  assert(res);
  return res;
}
bool load_floppy(struct disk_t *d) {
  if (!has_floppy())
    return false;

  d->arg = NULL;
  d->read = floppy_disk_read;
  d->write = floppy_disk_write;
  d->view = floppy_disk_view;
  return true;
}