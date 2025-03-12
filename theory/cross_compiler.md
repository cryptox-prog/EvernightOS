## Cross Compiling
Cross compilers are needed for creating programs that target different executable formats, CPUs, operating systems, etc. Since we will be writing 32-bit code on a 64-bit machine, such a compiler is necessary.

### ELF32
ELF stands for Executable and Linkable Format, which is the format for binary files in UNIX-based systems. ELF32 is the ELF format for 32-bit architectures.

### Steps to Enable Cross Compiling on an Arch Linux Machine:

1. **Edit `pacman.conf` to Enable the Multilib Repository**
    - Open the file: `sudo nano /etc/pacman.conf`
    - Uncomment the following lines:
        ```
        [multilib]
        Include = /etc/pacman.d/mirrorlist
        ```
    - Rebuild the package database: `sudo pacman -Syu`

2. **Install the Necessary 32-bit Libraries**
    - Run: `sudo pacman -S lib32-glibc gcc-multilib gdb binutils`
    - Descriptions:
        - `lib32-glibc`: Contains 32-bit versions of the standard C libraries.
        - `gcc-multilib`: Allows the use of the `-m32` flag to compile programs in 32-bit mode.
        - `gdb`: GNU Debugger for both 32-bit and 64-bit programs.
        - `binutils`: Provides essential tools for working with binaries, such as `ld` (linker), `as` (GNU Assembler), `objdump` (inspects compiled binaries), etc.

3. **Install NASM (Netwide Assembler)**
    - NASM uses Intel syntax and can assemble code in 32-bit mode with the `elf32` flag.
    - Run: `sudo pacman -S nasm`
    - Test: `nasm -v`

4. **Install QEMU**
    - QEMU emulates a 32-bit ELF environment, useful for testing low-level code.
    - Run: `sudo pacman -S qemu` (choose the full version if multiple options are shown)
    - Test: `qemu-system-i386 --version`

## References:
- [Cross-Compiler](https://wiki.osdev.org/GCC_Cross-Compiler#Introduction)
- [ELF](https://wiki.osdev.org/ELF)
- [lib32-glibc](https://stackoverflow.com/questions/11460782/what-is-glibc-what-is-it-used-for)
- [gcc-multilib](https://askubuntu.com/questions/855945/what-exactly-does-gcc-multilib-mean-on-ubuntu)
- binutils
    - [binutils](https://www.gnu.org/software/binutils/)
    - [Wikipedia: GNU - Binutils](https://en.wikipedia.org/wiki/GNU_Binutils)
