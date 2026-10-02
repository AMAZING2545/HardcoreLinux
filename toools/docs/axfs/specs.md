# UL2FS official spec
_this article contains only the on disk layout_ <br>
_for more info consult the source code_

## general layout

| entry | size | contains |
|--------|--------|--------|
| superblock | 1 block | boot sector + some padding |
| reserved | 0 - 64Ki blocks | misc/bootloaders/padding |
| file allocation table | 1 - 4Gi (~256Ti) blocks | linked lists, free blocks map |
| inodes table | 1 - 4Gi blocks | inodes |
| second reserved space | 0 - 255 blocks | misc (used to house the root dir) |
| data | 4 - 256Ti blocks | everything else |

### superblock entries

| entry | size | compulsory |
| ------------- |-------------|------------|
| JMP instruction | 8 bytes | yes |
| block size(2^x) | 1 byte | yes |
| name | 32 bytes | yes |
| reserved | 2 bytes | yes |
| lower FAT size | 4 bytes | yes |
| size | 8 bytes | yes |
| inodes | 4 bytes | yes |
| second reserved space(for optional journaling) | 1 byte | yes (but nullable) |
| CHS | 6 bytes | yes (but nullable) |
| upper FAT size | 2 bytes | no |
| used | 8 bytes | no |
| user/manufacturer definable | 24 bytes | no |

_note: size may vary among implementation, but guaranteed to be at least 62 bytes and at most 96 bytes_

the superblock is stored at the start of the first block, which may inglobate the boot sector

### file allocation table

each entry is 6 bytes and can store a pointer to a page, a free page or a end of chain

| range | comment |
|-------|---------|
| 0 | free block |
| 0-0xFFFFFFFFFFFE | block pointer |
| 0xFFFFFFFFFFFF | EoC |

### inode entries

_every entry is compulsory and must add up to 32 bytes_

| entry | size |
| -------- | -------- |
| octal digits | 5 digits ( 2 bytes ) |
| user | 2 bytes |
| group | 2 bytes |
| unused (future use) | 2 bytes |
| created timestamp | 4 bytes |
| modified timestamp (includes metadata) | 4 bytes |
| hard links | 2 bytes |
| size | 8 bytes |
| start page | 6 bytes |

### directory entry structure

_it must add up to a multiple of two (32,64,128,256)_ <br>
_the minimal superblock structure only supports 128 byte entries with 4 byte inodes_

| entry | size |
|-------|-------|
| name | usually 123 bytes |
| attributes | 1 byte |
| inode | 4 or 6 bytes |
