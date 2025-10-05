# Directories
SRC_DIR = src
BOOTLOADER_DIR = $(SRC_DIR)/bootloader
UTILS_DIR = $(SRC_DIR)/utils
DESCRIPTOR_TABLES_DIR = $(SRC_DIR)/descriptor_tables
DISPLAY_DIR = $(SRC_DIR)/display
BUILD_DIR = build
INTERMEDIATE_DIR = $(BUILD_DIR)/inter

# Tools
ASSEMBLER = nasm
C_COMPILER = gcc
LINKER = ld
OBJ_COPY = objcopy
IMAGE_MAKER = dd
EMULATOR = qemu-system-i386

# Flags
ASSEMBLER_FLAGS_BINARY = -f bin -I $(SRC_DIR)
ASSEMBLER_FLAGS_ELF = -f elf32
C_COMPILER_FLAGS = -m32 -ffreestanding -g -nostdlib -fno-stack-protector -O0 -fno-omit-frame-pointer -Wall -Wextra -I $(SRC_DIR)
OBJ_COPY_FLAGS = -O binary
LINKER_FLAGS = -m elf_i386
EMULATOR_FLAGS = -drive
EMULATOR_DEBUG_FLAGS = -s -S -monitor stdio # then type info registers

# Source Files
BOOT_SOURCE = $(BOOTLOADER_DIR)/bootloader.asm
GDT_SOURCE = $(DESCRIPTOR_TABLES_DIR)/gdt.asm
START_PROTECTED_SOURCE = $(BOOTLOADER_DIR)/start_protected.asm
KERNEL_MAIN_SOURCE = $(SRC_DIR)/kernel.c
IDT_SOURCE = $(DESCRIPTOR_TABLES_DIR)/idt.c
ISR_SOURCE = $(DESCRIPTOR_TABLES_DIR)/isr.asm
VGA_SOURCE = $(DISPLAY_DIR)/vga.c
LINKER_SOURCE = linker.ld

# Include Files
TYPES_HEADER = $(UTILS_DIR)/types.h
IDT_HEADER = $(DESCRIPTOR_TABLES_DIR)/idt.h
PORTS_HEADER = $(UTILS_DIR)/ports.h
VGA_HEADER = $(DISPLAY_DIR)/vga.h
WELCOME_SCREEN_HEADER = $(DISPLAY_DIR)/welcome_screen.h

# Intermediate Files
BOOT_BINARY =  $(INTERMEDIATE_DIR)/bootloader.bin
START_PROTECTED_OBJECT = $(INTERMEDIATE_DIR)/start_protected.o
KERNEL_MAIN_OBJECT = $(INTERMEDIATE_DIR)/kernel.o
IDT_OBJECT = $(INTERMEDIATE_DIR)/idt.o
ISR_OBJECT = $(INTERMEDIATE_DIR)/isr.o
VGA_OBJECT = $(INTERMEDIATE_DIR)/vga.o
SECOND_STAGE_LINKED = $(INTERMEDIATE_DIR)/second_stage.elf
SECOND_STAGE_BINARY = $(INTERMEDIATE_DIR)/second_stage.bin

# Final Target
TARGET = $(BUILD_DIR)/disk.img

# Phony Targets
.PHONY: prepare all run _run clean _clean debug gdb

# Main Build Target
all: prepare $(TARGET)

# Prepare Directories
prepare: $(BUILD_DIR) $(INTERMEDIATE_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(INTERMEDIATE_DIR): $(BUILD_DIR)
	mkdir -p $(INTERMEDIATE_DIR)

# Build Disk Image
$(TARGET): $(BOOT_BINARY) $(SECOND_STAGE_BINARY)
	$(IMAGE_MAKER) if=/dev/zero of=$(TARGET) bs=512 count=2880
	$(IMAGE_MAKER) if=$(BOOT_BINARY) of=$(TARGET) conv=notrunc
	$(IMAGE_MAKER) if=$(SECOND_STAGE_BINARY) of=$(TARGET) bs=512 seek=1 conv=notrunc
	@stage2_size=$$(stat -c%s $(SECOND_STAGE_BINARY)); \
	sectors=$$(( ($$stage2_size + 511) / 512 )); \
	echo "Second stage is $$sectors sectors"; \
	printf "$$(printf '\\x%02x' $$sectors)" | dd of=$(TARGET) bs=1 seek=509 conv=notrunc

# Build Bootloader
$(BOOT_BINARY): $(BOOT_SOURCE) $(GDT_SOURCE)
	$(ASSEMBLER) $(ASSEMBLER_FLAGS_BINARY) $(BOOT_SOURCE) -o $(BOOT_BINARY)

# Build Protected Mode Startup
$(START_PROTECTED_OBJECT): $(START_PROTECTED_SOURCE) $(IDT_OBJECT)
	$(ASSEMBLER) $(ASSEMBLER_FLAGS_ELF) $(START_PROTECTED_SOURCE) -o $(START_PROTECTED_OBJECT)

# Build IDT and ISR
$(IDT_OBJECT): $(IDT_SOURCE) $(ISR_OBJECT) $(TYPES_HEADER) $(IDT_HEADER)
	$(C_COMPILER) $(C_COMPILER_FLAGS) -c $(IDT_SOURCE) -o $(IDT_OBJECT)

$(ISR_OBJECT): $(ISR_SOURCE)
	$(ASSEMBLER) $(ASSEMBLER_FLAGS_ELF) $(ISR_SOURCE) -o $(ISR_OBJECT)

$(VGA_OBJECT): $(VGA_SOURCE) $(VGA_HEADER) $(TYPES_HEADER) $(PORTS_HEADER)
	$(C_COMPILER) $(C_COMPILER_FLAGS) -c $(VGA_SOURCE) -o $(VGA_OBJECT)

# Build Kernel
$(KERNEL_MAIN_OBJECT): $(KERNEL_MAIN_SOURCE) $(TYPES_HEADER) $(WELCOME_SCREEN_HEADER)
	$(C_COMPILER) $(C_COMPILER_FLAGS) -c $(KERNEL_MAIN_SOURCE) -o $(KERNEL_MAIN_OBJECT)
# Link Second Stage
$(SECOND_STAGE_LINKED): $(LINKER_SOURCE) $(START_PROTECTED_OBJECT) $(KERNEL_MAIN_OBJECT) $(VGA_OBJECT)
	$(LINKER) $(LINKER_FLAGS) -T $(LINKER_SOURCE) -o $(SECOND_STAGE_LINKED) $(START_PROTECTED_OBJECT) $(KERNEL_MAIN_OBJECT) $(IDT_OBJECT) $(ISR_OBJECT) $(VGA_OBJECT)

# Convert Second Stage to Binary
$(SECOND_STAGE_BINARY): $(SECOND_STAGE_LINKED)
	$(OBJ_COPY) $(OBJ_COPY_FLAGS) $(SECOND_STAGE_LINKED) $(SECOND_STAGE_BINARY)
	truncate -s %512 $(SECOND_STAGE_BINARY)

# Run Emulator
run: all
	$(EMULATOR) $(EMULATOR_FLAGS) format=raw,file=$(TARGET)

_run:
	@if [ ! -f "$(TARGET)" ]; then \
		echo "Error: $(TARGET) does not exist."; \
		exit 1; \
	else \
		echo "$(EMULATOR) $(EMULATOR_FLAGS) format=raw,file=$(TARGET)"; \
		$(EMULATOR) $(EMULATOR_FLAGS) format=raw,file=$(TARGET); \
	fi

debug: all
	$(EMULATOR) $(EMULATOR_FLAGS) format=raw,file=$(TARGET) $(EMULATOR_DEBUG_FLAGS)

gdb:
	gdb -ex "target remote localhost:1234" -ex "symbol-file $(SECOND_STAGE_LINKED)"

# Clean Targets
clean:
	rm -rf $(BUILD_DIR)

_clean:
	rm -rf $(INTERMEDIATE_DIR)
