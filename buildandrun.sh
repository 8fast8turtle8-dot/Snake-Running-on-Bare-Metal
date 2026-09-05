# 1. Build Stage 1 boot sector (512 bytes)
nasm -f bin boot.asm -o boot.bin

# 2. Build Stage 2 loader (flat binary)
nasm -f bin Stage2.asm -o stage2.bin

# 3. Compile kernel C++ into object file
g++ -m32 -ffreestanding -fno-exceptions -fno-rtti -nostdlib -c kernel.cpp -o kernel.o

# 4. Link kernel into 32‑bit ELF at physical address 0x00010000
ld -m elf_i386 -nostdlib -Ttext=0x00010000 kernel.o -o kernel.elf

# 5. Convert ELF → raw flat binary (this is what Stage 2 loads)
objcopy -O binary kernel.elf kernel.bin

# 6. Create floppy/disk image
dd if=/dev/zero of=turtlos.img bs=512 count=2880

# 7. Write Stage 1 to sector 0
dd if=boot.bin of=turtlos.img conv=notrunc

# 8. Write Stage 2 to sector 1 (LBA 1)
dd if=stage2.bin of=turtlos.img bs=512 seek=1 conv=notrunc

# 9. Write kernel to sector 2 (LBA 2)
dd if=kernel.bin of=turtlos.img bs=512 seek=2 conv=notrunc

# 10. Boot it
qemu-system-i386 -drive format=raw,file=turtlos.img

