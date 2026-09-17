#pragma pack(1)
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <math.h>
#include <string.h>
#include "axfs.h"
int fd;
header* a=NULL;
int main(int argc, char* argv[]){
	if (argc==1){
		printf("size of inode: %ld\nsize of file: %ld\nsize of header: %ld\nsize of page: %ld\n", sizeof(inode),sizeof(file), sizeof(header),sizeof(page));
		puts("arg 1 = file to format\narg 2 = block size (in format 2^x)\narg 3 = volume name\narg 4 = inodes(in blocks)\n");
	}
	else if(argc==2){
		int fd = open(argv[1], O_RDONLY);
		header boot;
		read(fd, &boot, sizeof(boot));
		printf("block size = %d\n", 1<<boot.blocksize);
                printf("volume name = %s\n", boot.volume);
                printf("FAT size = %d sectors\n", boot.fatsize);
                printf("size in sectors = %lu\n", boot.size);
                printf("inodes = %d\n", boot.inodes<<boot.blocksize-5);
		printf("used space: %lu sectors\n", boot.used);
		close(fd);
	}
	else if(argc==3){
		fd = open(argv[1], O_RDWR);
		header boot;
                read(fd, &boot, sizeof(boot));
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
		BUSYWAIT
		get_inode(&boot, atoi(argv[2]), &result, fd);
		printf("size of file: %lu\n", result.size);
		char* data=malloc(result.size);
		printf("actual bytes read: %lu\n",read_inode(&boot, atoi(argv[2]), &result, data, 0, 2000, fd));
		puts("data: ");
		write(0,data,2000);
	}

	else{
		//let's do static for now
		int fd = open(argv[1], O_RDWR);
		struct stat stats;
		uint64_t size;
		if(ioctl(fd, BLKGETSIZE64, &size)==-1){
			stat(argv[1], &stats);
			size=stats.st_size;
		}
		printf("size = %ld\n",size);
		lseek(fd,0,SEEK_SET);
		uint64_t tot_sectors=size/(1<<atoi(argv[2]));
		header boot={
				"\0\0\0\0\0\0\0\0", //not bootable
				(uint8_t)atoi(argv[2]), //sector size
				"name", //volume name
				0, //no reserved, but starts after one block
				ceil((double)(tot_sectors-atoi(argv[4])-1)/(1<<atoi(argv[2]))*(double)6), //size of FAT
				tot_sectors,//size
				atoi(argv[4]),//inodes
				0,//root directory, keep it to 0
				0,0,0, //chs
				0,//used for multi petabyte partitions
				0,//reserved
		};
		header* a =malloc(sizeof(header));
		*a=boot;
		strcpy(boot.volume,argv[3]);
		printf("block size = %d\n", 1<<boot.blocksize);
		printf("volume name = %s\n", boot.volume);
		printf("FAT size = %d sectors\n", boot.fatsize);
		printf("size in sectors = %lu\n", boot.size);
		printf("inodes = %d\n", boot.inodes<<boot.blocksize-5);
		puts("proceed?");
		char yes='n';
		scanf("%c", &yes);
		if(yes=='y'){
			puts("writing boot sector");
			write(fd, &boot, sizeof(header));
			puts("zeroing the FAT");
			unsigned char* empty=calloc(1<<boot.blocksize,1);
			lseek(fd,1<<boot.blocksize,SEEK_SET);
			for(uint32_t i = 0; i<boot.fatsize;i++)
				write(fd, empty, 1<<boot.blocksize);
			puts("zeroing inodes");
			for(uint32_t i = 0; i<boot.inodes;i++)
				write(fd, empty, 1<<boot.blocksize);
			inode inode_struct={493,0,0,0,0,0,0,0,{0,0}};
			puts("allocating initial root directory (inode 0)");
	        const uint64_t inode_start = (1<<a->blocksize)*(a->resblocks + a->fatsize + 1);
        const uint64_t fat_start = (1<<a->blocksize)*(a->resblocks + 1);
        const uint64_t data_start = (1<<a->blocksize)*(a->resblocks + a->fatsize + a->rootdirsize + a->inodes + 1);
        const uint64_t fat_length = (1<<a->blocksize)*a->fatsize;
        const uint64_t inode_length = (1<<a->blocksize)*a->inodes;
        fat = (page*)mmap(NULL, fat_length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, fat_start);
        if(fat==-1)
                perror("mmap failed");
        inodes=mmap(NULL, inode_length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, inode_start);
			create_new_inode(&boot, &inode_struct, fd);
			//extend_inode(&boot, 0, 4096, fd);
			file links = {".", 1, 0}; //name . ,attributes b01 (directory), link to 0
			write_inode(&boot, 0, &links, 0, 128, fd);
			strcpy(links.name,"..");
			write_inode(&boot, 0, &links, 128, 128, fd);
			get_inode(&boot, 0, &inode_struct, fd);
			inode_struct.links=2;
			modify_inode(&boot, 0, &inode_struct, fd);
			puts("format successful");
		}
	}
}
