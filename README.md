# EvernightOS

## Overview
- The aim of this project is design a basic 32-bit operating system from scratch using only assembly and C
- This is being to done to learn basic assembly and the basics of computer architechture

## Features
Currently, the OS is in an early stage of development and is non-functional. However, the following components have been implemented so far:
- A basic binary bootloader to switch from protected mode to real mode
- A Global Descriptor Table (GDT)
- An Interrupt Descriptor Table (IDT) with a default interrupt handling routine
- Basic display functions using VGA
- Keyboard input using interrupts

## Working On
The following features are currently under development:
- Basic commands processing
- A memory Allocator

## Tech Stack
- **Assembly:** i386 Assembly
- **Programming Language:** C (without standard libraries)
- **Compiler:** GCC (using the `-m32` flag for 32-bit compilation)
- **Assembler:** NASM
- **Emulation:** QEMU

## Future Plans
- Multitasking
- Filesystem
- Network
