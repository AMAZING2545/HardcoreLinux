#include "include/axfss.h"

header* a = NULL;

int fd;

int main(int argc, char** argv){
                fd = open(argv[1], O_RDWR);
                header boot;
                read(fd, &boot, sizeof(header));
                printf("block size = %d\n", 1<<boot.blocksize);
                printf("volume name = %s\n", boot.volume);
                printf("FAT size = %d sectors\n", boot.fatsize);
                printf("size in sectors = %lu\n", boot.size);
                printf("inodes = %d\n", boot.inodes<<boot.blocksize-5);
                inode result;
                a=&boot;
                const uint64_t inode_start = (1<<a->blocksize)*(a->resblocks + a->fatsize + 1);
                const uint64_t fat_start = (1<<a->blocksize)*(a->resblocks + 1);
                const uint64_t data_start = (1<<a->blocksize)*(a->resblocks + a->fatsize + a->rootdirsize + a->inodes + 1);
                const uint64_t fat_length = (1<<a->blocksize)*a->fatsize;
                const uint64_t inode_length = (1<<a->blocksize)*a->inodes;
                fat = (page*)mmap(NULL, fat_length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, fat_start);
                inodes=mmap(NULL, inode_length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, inode_start);

		scan_filesystem(a,fd);
}
