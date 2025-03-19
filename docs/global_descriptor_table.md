# Global Descriptor Table (GDT)

The Global Descriptor Table (GDT) is used as a reference to define the permissions and abilities of certain sections of memory have in protected mode. It is necessary to use a GDT because allowing all software applications on the system access to all portions of memory is unsafe.

## Segments in GDT

There are three segments in our GDT:

1. **Null Segment at 0** - By convention, the first entry is always zero (the `lgdt` register, which stores the length and start address of GDT, expects calculation for null).
2. **Code Segment at 1** - This should be executable and readable.
3. **Data Segment at 2** - This segment will not be executable but can be either readable or writable or both. In this case, we will make it writable and readable.

## Segment Structure

Each segment should have:

- **Limit** - The maximum offset allowed for the segment. It is 20 bits in total. In normal assembly, to have 2<sup>20</sup> bytes (~1MB) of total memory addressing, we utilize two registers in which the value of one of them is left-shifted by 4, i.e., <code>(16 * R<sub>1</sub>) + R<sub>2</sub></code>. This is called segmentation.
- **Base** - The start address of the segment. It is 32 bits in total. In protected mode, we are in 32-bit mode and thus have access to 4GB of total memory.
- **Access Flags** - The access level (ring0 to ring3), readable/writable/executable, present.
- **Granularity** - Determines whether the limit is determined in units of bytes or 4KB pages. Without granularity, only 1MB of the 4GB memory would be accessible. The activation of granularity shifts the ```(limit + 1)``` left by 12, i.e., 4KB, allowing us to cover 4GB. Note: limit + 1 because the count starts from 0.

## Segment Definition Structure

The following is the structure in which each segment will be defined:

1. **Lower 16-bits of Limit** - Self-explanatory.
2. **Lower 16-bits of Base Address** - Self-explanatory.
3. **Middle 8-bits of Base Address** - Separated from lower 16 because you can define 16 bits at maximum (DW) or 8 bits at minimum (DB).
4. **Access Flags 8-bits** (b<sub>1</sub> b<sub>2</sub> b<sub>3</sub> b<sub>4</sub> b<sub>5</sub> b<sub>6</sub> b<sub>7</sub> b<sub>8</sub>):
	- b<sub>1</sub>: Present bit, 1 if the segment is in memory, if 0 accessing it will cause Segment Not Present Error (fault). Note: The OS can edit GDT entries.
	- b<sub>2</sub>-b<sub>3</sub>: Descriptor Privilege Level (DPL), possible values range from 0 to 3, wherein 0 is the highest. This is also called ring level (ring0, ..., ring3).
	- b<sub>4</sub>: Descriptor Type, 1 for code and data segments, 0 for system segments.
	- b<sub>5</sub>-b<sub>8</sub>: Comprise the type field:
		- b<sub>5</sub>: Executable, 1 if executable code, 0 otherwise.
		- b<sub>6</sub>: Conforming, 1 if execution from lower access levels (i.e., ring<sub>2</sub> can execute ring<sub>0</sub> code).
		- b<sub>7</sub>: Readable, 1 if the segment can be read.
		- b<sub>8</sub>: Accessed, set by CPU when the segment is accessed.
5. **Granularity 4-bits and Upper 4-bits of Limit** (b<sub>1</sub> b<sub>2</sub> b<sub>3</sub> b<sub>4</sub> b<sub>5</sub> b<sub>6</sub> b<sub>7</sub> b<sub>8</sub>):
	- b<sub>1</sub>: Granularity (G), 0 limit is in bytes, 1 is in 4KB pages.
	- b<sub>2</sub>: Operand Size Flag (D/B), D - Default Operand Size, B - Big, if 1 then the segment is 32-bit (i.e., D) and the maximum offset is increased to 32-bit offset (i.e., B), otherwise 0 for 16-bit segment with max offset of 16 bits.
	- b<sub>3</sub>: Long Mode Flag (L), 1 for 64-bit code segments, 0 otherwise (even for 32-bit code segments).
	- b<sub>4</sub>: Available (AVL), set to 1 by the system software if the segment is being used by it for its own purpose, 0 otherwise.
	- b<sub>5</sub>-b<sub>8</sub>: Upper four bits of the limit.
6. **Upper 8-bits of Base Address** - Self-explanatory.

```Total = 16 + 16 + 8 + 8 + 4 + 4 + 8 = 64 bits = 8 bytes```. Thus, each segment's description in the GDT is 8 bytes.

Note: A page is a fixed length of contiguous block of memory (4KB in many systems).

## References

- [Segmentation](https://wiki.osdev.org/Segmentation)
- [Paging](https://wiki.osdev.org/Paging)
- Segment Descriptor
	- [Structure](https://wiki.osdev.org/Descriptor)
	- [Structure and Details](https://en.wikipedia.org/wiki/Segment_descriptor)
- [Segment Limits](https://wiki.osdev.org/Segment_Limits)
- [Ring Access](https://wiki.osdev.org/Security)
- [Editing GDT](https://stackoverflow.com/questions/63092564/how-to-update-gdt-entries-after-initial-loading)
