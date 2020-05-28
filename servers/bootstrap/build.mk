name := bootstrap
obj-y += main.o pages.o bootfs.o

$(build_dir)/bootfs.o: $(bootfs_bin)
