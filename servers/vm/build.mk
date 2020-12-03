name := vm
description := The memory and task manager
boot_task := y
libs-y += elf
objs-y += main.o task.o mm.o ool.o pages.o bootfs.o bootfs_image.o shm.o

$(build_dir)/bootfs_image.o: $(bootfs_bin)
