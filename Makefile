# Directories
SRC_DIR = src
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
ASSEMBLER_FLAGS_BINARY = -f bin -I src/
ASSEMBLER_FLAGS_ELF = -f elf32
C_COMPILER_FLAGS = -m32 -ffreestanding -nostdlib -fno-stack-protector
OBJ_COPY_FLAGS = -O binary
LINKER_FLAGS = -m elf_i386
EMULATOR_FLAGS = -drive
EMULATOR_DEBUG_FLAGS = -monitor stdio # then type info refegisters

# Source Files
BOOT_SOURCE = $(SRC_DIR)/bootloader.asm
GDT_SOURCE = $(SRC_DIR)/gdt.asm
START_PROTECTED_SOURCE = $(SRC_DIR)/start_protected.asm
KERNEL_MAIN_SOURCE = $(SRC_DIR)/kernel.c
IDT_SOURCE = $(SRC_DIR)/idt.c
ISR_SOURCE = $(SRC_DIR)/isr.asm
LINKER_SOURCE = linker.ld

# Include Files
TYPES_HEADER = $(SRC_DIR)/types.h

# Intermediate Files
BOOT_BINARY =  $(INTERMEDIATE_DIR)/bootloader.bin
START_PROTECTED_OBJECT = $(INTERMEDIATE_DIR)/start_protected.o
KERNEL_MAIN_OBJECT = $(INTERMEDIATE_DIR)/kernel.o
IDT_OBJECT = $(INTERMEDIATE_DIR)/idt.o
ISR_OBJECT = $(INTERMEDIATE_DIR)/isr.o
SECOND_STAGE_LINKED = $(INTERMEDIATE_DIR)/second_stage.elf
SECOND_STAGE_BINARY = $(INTERMEDIATE_DIR)/second_stage.bin

# Final Target
TARGET = $(BUILD_DIR)/disk.img

# Phony Targets
.PHONY: all run _run clean _clean

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

# Build Bootloader
$(BOOT_BINARY): $(BOOT_SOURCE) $(GDT_SOURCE)
	$(ASSEMBLER) $(ASSEMBLER_FLAGS_BINARY) $(BOOT_SOURCE) -o $(BOOT_BINARY)

# Build Protected Mode Startup
$(START_PROTECTED_OBJECT): $(START_PROTECTED_SOURCE) $(IDT_OBJECT)
	$(ASSEMBLER) $(ASSEMBLER_FLAGS_ELF) $(START_PROTECTED_SOURCE) -o $(START_PROTECTED_OBJECT)

# Build IDT and ISR
$(IDT_OBJECT): $(IDT_SOURCE) $(ISR_OBJECT) $(TYPES_HEADER)
	$(C_COMPILER) $(C_COMPILER_FLAGS) -c $(IDT_SOURCE) -o $(IDT_OBJECT)

$(ISR_OBJECT): $(ISR_SOURCE)
	$(ASSEMBLER) $(ASSEMBLER_FLAGS_ELF) $(ISR_SOURCE) -o $(ISR_OBJECT)

# Build Kernel
$(KERNEL_MAIN_OBJECT): $(KERNEL_MAIN_SOURCE) $(TYPES_HEADER)
	$(C_COMPILER) $(C_COMPILER_FLAGS) -c $(KERNEL_MAIN_SOURCE) -o $(KERNEL_MAIN_OBJECT)

# Link Second Stage
$(SECOND_STAGE_LINKED): $(LINKER_SOURCE) $(START_PROTECTED_OBJECT) $(KERNEL_MAIN_OBJECT)
	$(LINKER) $(LINKER_FLAGS) -T $(LINKER_SOURCE) -o $(SECOND_STAGE_LINKED) $(START_PROTECTED_OBJECT) $(KERNEL_MAIN_OBJECT) $(IDT_OBJECT) $(ISR_OBJECT)

# Convert Second Stage to Binary
$(SECOND_STAGE_BINARY): $(SECOND_STAGE_LINKED)
	$(OBJ_COPY) $(OBJ_COPY_FLAGS) $(SECOND_STAGE_LINKED) $(SECOND_STAGE_BINARY)

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

debug:
	$(EMULATOR) $(EMULATOR_FLAGS) format=raw,file=$(TARGET) $(EMULATOR_DEBUG_FLAGS)

# Clean Targets
clean:
	rm -rf $(BUILD_DIR)

_clean:
	rm -rf $(INTERMEDIATE_DIR)
