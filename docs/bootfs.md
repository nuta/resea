# Boot FS
Bootfs is a simple file system embedded in the kernel executable. It contains
the first userland process image (typically bootstrap server) and some essential
servers to boot the system. It's equivalent to *initramfs* in Linux.

## On-Disk Format

### File System Header

```
+---------------------------------------------+
|                 Jump Code                   |
|    (The entrypoint of bootstrap server)     |
+---------------------------------------------+
|                  Version                    |
+---------------------------------------------+
|              File System Size               |
|          (excluding this header)            |
+---------------------------------------------+
|              Number of Files                |
+---------------------------------------------+
|              Reserved (padding)             |
+---------------------------------------------+
|                  File #1                    |
+---------------------------------------------+
|                  File #2                    |
+---------------------------------------------+
|                    ...                      |
+---------------------------------------------+
```

### File
```
+---------------------------------------------+
|                 File Path                   |
|             (terminated by NUL)             |
+---------------------------------------------+
|                 File Size                   |
+---------------------------------------------+
|                Padding Len                  |
+---------------------------------------------+
|                 Reserved                    |
+---------------------------------------------+
|               File Content                  |
+---------------------------------------------+
|                 Padding                     |
+---------------------------------------------+
```
