# Week 2

Motivation: To examine the NVidia proprietary driver. 

Research background: looking mostly through the nouveau driver process and their clean room ideas as well as their commits.


## Notes

The NVidia driver is surprisingly large, but this makes sense when considering the number of chips it supports.

Reverse Engineering the NVidia x64 linux driver

`binwalk NVIDIA-Linux-x86_64-418.43.run`

```
DECIMAL       HEXADECIMAL     DESCRIPTION
--------------------------------------------------------------------------------
0             0x0             Executable script, shebang: "/bin/sh"
9688          0x25D8          Unix path: /usr/share/X11/xorg.conf.d."
10053         0x2745          Unix path: /oss.sgi.com/projects/ogl-sample/ABI/) mandates this"
16455         0x4047          Unix path: /lib/modules/KERNEL-NAME/kernel/drivers/video/' and"
16525         0x408D          Unix path: /lib/modules/KERNEL-NAME/build/', respectively."
31515         0x7B1B          Unix path: /usr/share/glvnd/egl_vendor.d."
32436         0x7EB4          Unix path: /usr/share/egl/egl_external_platform.d."
36708         0x8F64          Unix path: /usr/local/ssl/bin /usr/local/bin /usr/bin /bin"
41615         0xA28F          Unix path: /usr/local/ssl/bin /usr/local/bin /usr/bin /bin"
45192         0xB088          ELF, 64-bit LSB executable, AMD x86-64, version 1 (SYSV)
57108         0xDF14          xz compressed data
60634         0xECDA          xz compressed data
3182085       0x308E05        xz compressed data
3203521       0x30E1C1        xz compressed data
3224953       0x313579        xz compressed data
101342530     0x60A5D42       xz compressed data
101423601     0x60B99F1       xz compressed data
101509743     0x60CEA6F       xz compressed data
101595722     0x60E3A4A       xz compressed data
101676081     0x60F7431       xz compressed data
```

After extracting the files we consider each file individually with `file *`
```
308E05:     data
308E05.xz:  XZ compressed data
30E1C1:     data
30E1C1.xz:  XZ compressed data
313579:     data
313579.xz:  XZ compressed data
60A5D42:    data
60A5D42.xz: XZ compressed data
60B99F1:    data
60B99F1.xz: XZ compressed data
60CEA6F:    data
60CEA6F.xz: XZ compressed data
60E3A4A:    data
60E3A4A.xz: XZ compressed data
60F7431:    data
60F7431.xz: XZ compressed data
B088.elf:   ELF 64-bit LSB executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, for GNU/Linux 2.4.0, stripped
DF14.xz:    XZ compressed data
ECDA:       POSIX tar archive (GNU)
ECDA.xz:    XZ compressed data
```

It seems that the only true xz compressed data is ECDA.xz. Extracting it shows many files. Looking at the directory structure shows that we seem to have unpacked the system correctly: `tree -d`.

```
├ 32
│   └ libglvnd_install_checker
├ html
├ kernel
│   ├ common
│   │   └ inc
│   ├ nvidia
│   ├ nvidia-drm
│   ├ nvidia-modeset
│   └ nvidia-uvm
│       └ hwref
│           ├ kepler
│           │   └ gk104
│           ├ pascal
│           │   └ gp100
│           ├ turing
│           │   └ tu102
│           └ volta
│               └ gv100
└ libglvnd_install_checker

20 directories
```

Note: From what I’ve seen it seems available to also run the binary to extract it. In the kernel folder the “nv-kernel.o-binary” is the main nvidia binary. Throwing it into ghidra we can see lots of
very interesting functions. From the exports we can see lots of useful functions. 

- os_mem_set
- os_get_current_thread
- os_is_isr
- out_string

These all seem to be very reasonable functions expected here.

It is also clear that there are many thousands of functions that make up the core of the DRM.

## Conclusions

This was a fun week looking through driver structure and files. With no goal in mind with the driver its difficult to pin down somethinging to look at in particular, but I believe I have the starting point if I were to continue.
