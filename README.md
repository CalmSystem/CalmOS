# CalmOS

## About The Project

![Shell help screenshot][main-screenshot]

CalmOS is a toy operating system created during an Ensimag project.
It is minimal and designed only for x86 32bit (IA32) architecture.


### Built With

* Make
* GCC
* Qemu
* Love and Calm

### Features

* Priority based scheduler
* Message queue synchronization
* Dynamic stack allocation
* User-space protection
* Basic shell
* FAT16 filesystem
* Floppy disk drive
  * 1.44MB disk read/write
* Mouse support
  * Click to paint on screen
* PC Speaker music
  * Custom Music files player
    * `play imperial.mbp`
  * Keyboard synthesizer
    * Toggle with Ctrl+P

## Getting Started

### Prerequisites

* Make
* GCC 9+
* qemu-system-i386
* udisksctl (for floppy disk)

### Installation

1. Clone the repo
   ```sh
   git clone https://github.com/CalmSystem/CalmOS.git
   ```
2. Compile system and user-space
   ```sh
   make build
   ```
3. Compile floppy disk image
   ```sh
   make all
   ```

### Usage

* Start with floppy disk
   ```sh
   make run-now
   ```
* Start without disk
   ```sh
   make run-raw
   ```
* Compile and start
   ```sh
   make now
   ```


## Contributors

Code template created by [Ensimag](https://ensimag.grenoble-inp.fr/) teachers (Gregory Mounie et al.)

Features implemented by 3 students during a 3 weeks project.

## Resources

* [EnsiWiki: Project official documentation (FR)](https://ensiwiki.ensimag.fr/index.php?title=Projet_syst%C3%A8me)
* [OSDev.org: Awesome wiki about OS development](https://wiki.osdev.org/Main_Page)

## License

Distributed under the GPLv3 License. See `LICENSE` for more information.


[main-screenshot]: help-screen.png
