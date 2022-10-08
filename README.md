# patchchk

This tool can be used to change the version information of `.chk` image files, as used by many Netgear routers.

Some of these devices reject firmware files, whose version is lower than the currently installed one, preventing
downgrades, or in some cases flashing of custom firmware.

Usage is fairly straightforward. The version information can be displayed using

```
$ patchchk EX3700-V1.0.0.22_1.0.17.chk 
Board ID       : U12H319T00_NETGEAR
Region         : WW
Version        : 1.0.0.22_1.0.17
Sizes
- Header       : 58
- Kernel       : 5622416
- Root FS      : 0
Checksums
- Header       : 0xf8fc0a5a
- Kernel       : 0x292d8a81
- Root FS      : 0x00000000
- Image        : 0x292d8a81
```

To change the version to `1.0.0.22_1.0.18`, simply run

```
$ patchchk -v 1.0.0.22_1.0.18 EX3700-V1.0.0.22_1.0.17.chk
Version : 1.0.0.22_1.0.18
Checksum: 0xf9270a5b
```

Now verify using:
<pre>
$ patchchk EX3700-V1.0.0.22_1.0.17.chk 
Board ID       : U12H319T00_NETGEAR
Region         : WW
Version        : <b>1.0.0.22_1.0.17</b>
Sizes
- Header       : 58
- Kernel       : 5622416
- Root FS      : 0
Checksums
- Header       : <b>0xf9270a5b</b>
- Kernel       : 0x292d8a81
- Root FS      : 0x00000000
- Image        : 0x292d8a81
</pre>
