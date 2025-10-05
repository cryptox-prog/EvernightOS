# Ports in x86 Architecture
Ports are special registers, which require the CPU to change the control signal to read from or write to. So devices like keyboard (need not be a external device) can expose certain number of ports altering the values in these ports allows us to control the behaivior of said device. These registers are called by specifying their 16-bit "address", in Intel the instruction like `IN AL, 0x92` will let us read the value in port `0x92` into register `AL`.

## 0x92
Take for example the system control port A, it is a 16-bit port (register), it has the address `0x92`, the motherboard chipset exporses this port, its different bits are responsible for different tasks such as the lsb (0th bit) resets the CPU, while the 1st bit is responsible for checking if the A20 line is turned on. 

## VGA
Next let us take the example of VGA. This is Video Graphics Array, we will be conserned with the VGA Standard (IBM released a video card for it way back, but we are conserend with the "Standard", which is the ports, registers, memory layout, signal timings which define the interactions of a video card which **could** be controlled with VGA). VGA is rather complex and as such has over 300 internal registers, so rather than directly exposing them as ports they give 2 gate ports one defines the index of the the register to select, and other the read write value of that register (they expose more than just 2). The gate prots are `0x3d4` and `0x3d5`, they are called CRTC address and data registers (since VGA works with CRT hardware), 8-bit index of register is written to `0x3d4` and value read from or written to `0x3d5`. There are more ports but beyond the scope of this document.

| Index  | Name                       | Description                                                                                            |
| ------ | -------------------------- | ------------------------------------------------------------------------------------------------------ |
| `0x00` | Horizontal Total           | Total number of character clocks per scanline                                                          |
| `0x01` | Horizontal Display End     | The last displayed character position on a line, effectively the screen width in characters minus one  |
| `0x02` | Start Horizontal Blank     | When horizontal blanking begins                                                                        |
| `0x03` | End Horizontal Blank       | When horizontal blanking ends                                                                          |
| `0x0a` | Cursor Start               | Cursor scanline start position                                                                         |
| `0x0b` | Cursor End                 | Cursor scanline end position                                                                           |
| `0x0e` | Cursor Location High       | Cursor position high byte                                                                              |
| `0x0f` | Cursor Location Low        | Cursor position low byte                                                                               |
| `0x12` | Vertical Display End       | Number of rows minus one                                                                               |


There are more ports in VGA, but they are beyond the scope of this document.

## References
- [A20](https://wiki.osdev.org/A20_Line)
- [VGA](https://wiki.osdev.org/VGA_Hardware#Port_0x3C4,%5C_0x3CE,%5C_0x3D4)

**TODO:** Add better refrences
