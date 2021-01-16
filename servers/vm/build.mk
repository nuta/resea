name := vm
description := The memory and task manager
boot_task := y
libs-y += elf
objs-y += main.o task.o ool.o page_alloc.o page_fault.o bootfs.o bootfs_image.o
objs-y += shm.o

$(build_dir)/bootfs_image.o: $(bootfs_bin)
