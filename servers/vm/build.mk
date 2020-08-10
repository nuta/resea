name := vm
obj-y += main.o task.o mm.o ool.o pages.o bootfs.o bootfs_image.o

$(build_dir)/bootfs_image.o: $(bootfs_bin)
