#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <time.h>

#pragma pack(1)
#pragma once
typedef struct{
        uint8_t reserved[8]; //reserved for a jump instruction (x86 is fine with 3 bytes, but some arches need more)
        uint8_t blocksize; //(2^x), 16 is a healthy maximum
        uint8_t volume[32]; //volume name
        uint16_t resblocks; //number of reserved blocks
        uint32_t fatsize; //how large is the FAT in blocks (700MB for a 512GB volume with a block size of 4096)
        uint64_t size; //in blocks (including superblock, reserved, FAT, inodes and rootdir), max 256T blocks
        uint32_t inodes; //how many inodes to allocate, immediately after the FAT, in blocks
	uint8_t rootdirsize; //MUST be set to 0
        uint16_t c;
        uint16_t h;
        uint16_t s; //legacy CHS
        uint16_t exfatsize; //extended FAT size, for multi zettabyte volumes only
        uint64_t used; //used sectors (unused)
}header;

typedef struct{
        uint32_t lower;
        uint16_t upper;
}page; //0 = free (nothing can point to it by definition, except if it is the first in the chain)
//-1 = end of chain

typedef struct{
        uint16_t permissions; // standard x4 octal
        uint16_t user;
        uint16_t group;
	uint16_t era; //high part of UNIX time, shared between created and modified
        uint32_t created; //unsigned UNIX timestamp, up to ~2106
        uint32_t modified;
        uint16_t links; //how many files are linked to this inode, 0 means inode is free
        uint64_t size; //in bytes
        page start;
}inode;

typedef struct{
        uint8_t name[128-1-4];
        uint8_t attributes; //from least to most significant: directory symlink unused(6)
        uint32_t inode;
}file;

uint64_t page2int(page a){
	return (uint64_t)a.lower|((uint64_t)a.upper<<32);
}

page int2page(uint64_t a){
	page b;
	b.lower=a;
	b.upper=(uint64_t)(a>>32);
	return b;
}

uint32_t modify_inode(header* a, uint32_t num, inode* inode_struct, int fd){
	const uint64_t inode_start = (1<<a->blocksize)*(a->resblocks + a->fatsize + 1);
	const uint64_t fat_start = (1<<a->blocksize)*(a->resblocks + 1);
	const uint64_t data_start = (1<<a->blocksize)*(a->resblocks + a->fatsize + a->rootdirsize + 1);
	const uint64_t fat_length = (1<<a->blocksize)*a->fatsize;
	const uint64_t inode_length = (1<<a->blocksize)*a->inodes;
	//map inodes
	if (inode_length>>5 < num){
		puts("inode out of range");
		return -1;
	}
	inode* inodes=mmap(NULL, inode_length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, inode_start);
	if (inodes == -1) {
                perror("mmap failed");
                return -1;
        }
	inode_struct->modified=time(NULL);
	*(inodes+num)=*(inode_struct);
	munmap(inodes, inode_length);
	if(inode_struct->links==0){
		return -1;
	}
	return num;
}

uint32_t get_inode(header* a, uint32_t num, inode* inode_struct, int fd){
        const uint64_t inode_start = (1<<a->blocksize)*(a->resblocks + a->fatsize + 1);
        const uint64_t fat_start = (1<<a->blocksize)*(a->resblocks + 1);
        const uint64_t data_start = (1<<a->blocksize)*(a->resblocks + a->fatsize + a->rootdirsize + 1);
        const uint64_t fat_length = (1<<a->blocksize)*a->fatsize;
        const uint64_t inode_length = (1<<a->blocksize)*a->inodes;
        //map inodes 
        if (inode_length>>5 < num){
                puts("inode out of range");
                return -1;
        }
        inode* inodes=mmap(NULL, inode_length, PROT_READ, MAP_SHARED, fd, inode_start);
        if (inodes == -1) {
                perror("mmap failed");
                return -1;
        }
        *(inode_struct)=*(inodes+num);
	munmap(inodes, inode_length);
	if(inode_struct->links==0){
		return -1;
	}
        return num;
}

uint32_t create_new_inode(header* a, inode* inode_struct, int fd){
	//set the permissions in inode struct first before
	const uint64_t inode_start = (1<<a->blocksize)*(a->resblocks + a->fatsize + 1);
	const uint64_t fat_start = (1<<a->blocksize)*(a->resblocks + 1);
	const uint64_t data_start = (1<<a->blocksize)*(a->resblocks + a->fatsize + a->rootdirsize + 1);
	const uint64_t fat_length = (1<<a->blocksize)*a->fatsize;
	const uint64_t inode_length = (1<<a->blocksize)*a->inodes;
	//map inode map and FAT
	inode* inodes=mmap(NULL, inode_length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, inode_start);
	if (inodes == -1) {
                perror("mmap failed");
                return -1;
        }
	page* fat=mmap(NULL, fat_length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, fat_start);
	if (fat == -1) {
                perror("mmap failed");
                return -1;
        }
	for(uint64_t i = 0; i<inode_length/32; i++){
		if((inodes+i)->links==0){
			printf("found empty inode at %u\n", i);
			inode_struct->links=1; //just to avoid getting overwritten
			inode_struct->created=time(NULL);
			inode_struct->modified=time(NULL);
			//find a free starting FAT, write -1 to sign it as ended
			for(uint64_t j = 0; i<fat_length/6; j++){
				if(page2int(*(fat+j))==0){
					printf("found free FAT at %lu\n", j);
					*(fat+j)=int2page(-1);
					inode_struct->start=int2page(j);
					*(inodes+i)=*inode_struct;
					return i;
				}
			}
			puts("disk is likely full");
		}
	}
	munmap(inodes, inode_length);
	munmap(fat, fat_length);
	printf("all inodes are used\n");
	return -1;
}

uint64_t shrink_inode(header* a, uint32_t inode_num, uint64_t count, int fd){
	//remove count bytes from inode
        const uint64_t inode_start = (1<<a->blocksize)*(a->resblocks + a->fatsize + 1);
        const uint64_t fat_start = (1<<a->blocksize)*(a->resblocks + 1);
        const uint64_t data_start = (1<<a->blocksize)*(a->resblocks + a->fatsize + a->rootdirsize + a->inodes + 1);
        const uint64_t fat_length = (1<<a->blocksize)*a->fatsize;
	const uint64_t blocksize = (1<<a->blocksize);
	int64_t ret = 0;
	inode inode_struct;
	if(get_inode(a, inode_num, &inode_struct, fd)==-1){
		puts("nonexistant inode");
		return -1;
	}
	page current = inode_struct.start;
	page* fat = mmap(NULL, fat_length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, fat_start);
        if (fat == -1) {
                perror("mmap failed");
                return -1;
        }
	inode_struct.modified=time(NULL);
	//check if we actually need to deallocate blocks
	if((inode_struct.size - count)/blocksize == inode_struct.size/blocksize){
		inode_struct.size=inode_struct.size-count;
		modify_inode(a, inode_num, &inode_struct, fd);
		ret = inode_struct.size;
		goto ret;
	}
	//calculate the number of blocks to traverse
	uint64_t blocks_to_traverse = (inode_struct.size-count)/blocksize;
	printf("skipping %lu blocks\n",blocks_to_traverse);
	for(uint64_t i = 0; i<blocks_to_traverse; i++){
		printf("at block %lu\n",page2int(current));
		if(page2int(current)==0xFFFFFFFFFFFF){
			puts("unexpected EOF");
			ret=-1;
			goto ret;
		}
		current=*(fat+page2int(current));
	}
	page next = *(fat+page2int(current));
	*(fat+page2int(current))=int2page(-1); // *current now contains -1, current the next block
	current=next;
	//replace the last with -1 and the rest with 0
	//calculate how many blocks to zero
	blocks_to_traverse = inode_struct.size/blocksize - (inode_struct.size - count)/blocksize;
	printf("zeroing %lu blocks\n",blocks_to_traverse);
	for(uint64_t i = 0; i<blocks_to_traverse; i++){
		printf("zeroing %lu\n", page2int(current));
		next = *(fat+page2int(current));
		*(fat+page2int(current))=int2page(0);
		current=next;
	}
	inode_struct.size-=count;
	ret = inode_struct.size;
	ret:
		munmap(fat, fat_length);
		modify_inode(a, inode_num, &inode_struct, fd);
		return ret;
}

int32_t delete_inode(header* a, uint32_t inode_num, int fd){
        const uint64_t inode_start = (1<<a->blocksize)*(a->resblocks + a->fatsize + 1);
        const uint64_t fat_start = (1<<a->blocksize)*(a->resblocks + 1);
        const uint64_t data_start = (1<<a->blocksize)*(a->resblocks + a->fatsize + a->rootdirsize + a->inodes + 1);
        const uint64_t fat_length = (1<<a->blocksize)*a->fatsize;
	const uint64_t blocksize = (1<<a->blocksize);
	int64_t ret = 0;
	if(inode_num==0){
		puts("cannot delete root directory");
		return -1;
	}
	inode inode_struct;
	if(get_inode(a, inode_num, &inode_struct, fd)==-1){
		puts("nonexistant inode");
		return -1;
	}
	shrink_inode(a, inode_num, inode_struct.size, fd);
	page* fat = mmap(NULL, fat_length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, fat_start);
	*(fat+page2int(inode_struct.start))=int2page(0);
	if (fat == -1) {
                perror("mmap failed");
                return -1;
        }
	inode_struct.links=0; //this functionally deletes the inode
	modify_inode(a, inode_num, &inode_struct, fd);
	ret = inode_num;
	ret:
		munmap(fat, fat_length);
		return ret;
}

int64_t extend_inode(header* a, uint32_t inode_num, uint64_t count, int fd){
	//add count bytes to inode (not initialized, may contain garbage)
        const uint64_t inode_start = (1<<a->blocksize)*(a->resblocks + a->fatsize + 1);
        const uint64_t fat_start = (1<<a->blocksize)*(a->resblocks + 1);
        const uint64_t data_start = (1<<a->blocksize)*(a->resblocks + a->fatsize + a->rootdirsize + a->inodes + 1);
        const uint64_t fat_length = (1<<a->blocksize)*a->fatsize;
	const uint64_t blocksize = (1<<a->blocksize);
	int64_t ret = 0;
	inode inode_struct;
	if(get_inode(a, inode_num, &inode_struct, fd)==-1){
		puts("nonexistant inode");
		return -1;
	}
	page current = inode_struct.start;
	page* fat = mmap(NULL, fat_length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, fat_start);
        if (fat == -1) {
                perror("mmap failed");
                return -1;
        }
	inode_struct.modified = time(NULL);
	//check if we actually need to allocate new blocks
	if(inode_struct.size%blocksize + count <= blocksize){
		inode_struct.size=inode_struct.size+count;
		modify_inode(a, inode_num, &inode_struct, fd);
		ret = inode_struct.size;
		goto ret;
	}
	//clamp size
	printf("requested count: %lu\n",count);
	count+=inode_struct.size%blocksize - blocksize;
	inode_struct.size = inode_struct.size - inode_struct.size%blocksize + blocksize;
	printf("clamped old size: %lu\n",inode_struct.size);
	printf("clamped count %lu\n",count);
	//traverse FAT until EOF
	for(uint64_t i = 0; i<inode_struct.size/blocksize-1; i++){
		printf("at block %lu\n",page2int(current));
		if(page2int(current)==0xFFFFFFFFFFFF){
			puts("unexpected EOF");
			ret=-1;
			goto ret;
		}
		current=*(fat+page2int(current));
	}
	//find empty blocks and allocate them
	uint64_t allocate = count/blocksize;
	if(count%blocksize!=1)
		allocate++;
	uint64_t last_free=-1;
	printf("blocks to allocate: %lu\n", allocate);
	for(uint64_t i = 0; i<allocate; i++){
		//find a free block
		for(uint64_t j = last_free+1; j<fat_length/6; j++){
			printf("page %lu: %lu\n", j, page2int(*(fat+j)));
			if(page2int(*(fat+j))==0){
				last_free=j;
				*(fat+page2int(current))=int2page(j);
				printf("found block %lu\n",j);
				current=int2page(j);
				break;
			}
		}
	}
	*(fat+page2int(current))=int2page(-1);
	inode_struct.size+=count;
	ret = inode_struct.size;
	ret:
		munmap(fat, fat_length);
		modify_inode(a, inode_num, &inode_struct, fd);
		return ret;
}

int64_t write_inode(header* a, uint32_t inode_num, void* data, uint64_t seek, uint64_t count, int fd){
        //get inode start (in actual address, not LBA)
        const uint64_t inode_start = (1<<a->blocksize)*(a->resblocks + a->fatsize + 1);
        const uint64_t fat_start = (1<<a->blocksize)*(a->resblocks + 1);
        const uint64_t data_start = (1<<a->blocksize)*(a->resblocks + a->fatsize + a->rootdirsize + a->inodes + 1);
        const uint64_t fat_length = (1<<a->blocksize)*a->fatsize;
        printf("start of FAT: %lu\nstart of inodes: %lu\n", fat_start, inode_start);
	inode* inode_struct = malloc(sizeof(inode));
        if(get_inode(a, inode_num, inode_struct, fd)==-1){
		puts("nonexistan inode");
		return -1;
	}
        //map FAT to memory
        page current = inode_struct->start;
        page* fat = mmap(NULL, fat_length, PROT_READ, MAP_SHARED, fd, fat_start);
        if (fat == -1) {
 	               perror("mmap failed");
               return -1;
        }
	if (seek > inode_struct->size){
		puts("seek out of bounds");
		return -1;
	}
	inode_struct->modified=time(NULL);
	//clamp size
	printf("file size: %lu\nseek+count: %lu\n", inode_struct->size, seek+count);
	if(inode_struct->size < seek+count){
		printf("extending inode by %lu bytes\n",(seek+count)-inode_struct->size);
		extend_inode(a, inode_num,(seek+count)-inode_struct->size,fd);
		inode_struct->size+=(seek+count)-inode_struct->size;
	}
        uint64_t seek_blocks = seek>>a->blocksize;
        uint64_t seek_bytes = seek%(1<<a->blocksize);
	if(seek_bytes==0&&seek_blocks>0)
		seek_blocks++;
        int64_t ret=0;
	for(uint64_t i = 0; i<seek_blocks; i++){
		if(page2int(current)==0xFFFFFFFFFFFF){
			puts("unexpected EOF");
			ret=-1;
			goto ret;
		}
		current=*(fat+page2int(current));
		printf("at block %lu/n",page2int(current));
	}
	//now skip seek_bytes and write to it
	uint64_t counter = 0;//should be equal to count in the end
	uint64_t write_in_block = (1<<a->blocksize)-seek_bytes;
	if(write_in_block>=count)
		write_in_block = count;
	counter+=pwrite(fd, data, write_in_block, data_start + page2int(current)*(1<<a->blocksize) + seek_bytes);
	if(counter==count){
		ret=counter;
		goto ret;
	}
	seek_blocks = (count-counter)>>a->blocksize;
	seek_bytes = (count-counter)%(1<<a->blocksize);
	for(uint64_t i = 0; i<seek_blocks; i++){
		if(page2int(current)==0xFFFFFFFFFFFF){
			puts("unexpected EOF");
			ret=counter;
			goto ret;
		}
		current=*(fat+page2int(current));
		counter+=pwrite(fd, data+counter, (1<<a->blocksize), data_start + page2int(current)*(1<<a->blocksize));
		printf("writing to block %lu/n",page2int(current));
	}
	write_in_block = (1<<a->blocksize)-seek_bytes;
	if(!(count==counter))
		counter+=pwrite(fd, data+counter, write_in_block, data_start + page2int(current)*(1<<a->blocksize));
	ret = counter;
	ret:
		free(inode_struct);
		munmap(fat, fat_length);
		return ret;
}

int64_t read_inode(header* a, uint32_t inode_num, inode* inode_struct, void* data, uint64_t seek, uint64_t count, int fd){
	//get inode start (in actual address, not LBA)
	const uint64_t inode_start = (1<<a->blocksize)*(a->resblocks + a->fatsize + 1);
	const uint64_t fat_start = (1<<a->blocksize)*(a->resblocks + 1);
	const uint64_t data_start = (1<<a->blocksize)*(a->resblocks + a->fatsize + a->rootdirsize + a->inodes + 1);
	const uint64_t fat_length = (1<<a->blocksize)*a->fatsize;
	//printf("address of data: %lu\n", (size_t)data);
	//seek for inode
	if(get_inode(a, inode_num, inode_struct, fd)==-1){
		puts("nonexistan inode");
		return -1;
	}
	//map FAT to memory
	page current = inode_struct->start;
	page* fat = (page*)mmap(NULL, fat_length, PROT_READ, MAP_SHARED, fd, fat_start);
	if (fat == -1) {
        	perror("mmap failed");
        	return -1;
    	}
	uint64_t seek_blocks = seek>>a->blocksize;
	uint64_t seek_bytes = seek%(1<<a->blocksize);
	int64_t ret=0;
	if(seek>=inode_struct->size){
		puts("seek out of bounds");
		printf("seek: %lu\nsize: %lu",seek,inode_struct->size);
		ret=-1;
		goto ret;
	}
	if(seek+count>inode_struct->size)
		count=(count<<1)+seek-inode_struct->size;
	//unwind the FAT until seek_block is reached
	for(uint64_t i = 0; i<seek_blocks; i++){
		if(page2int(current)==0xFFFFFFFFFFFF){
			puts("unexpected EOF");
			ret=-1;
			goto ret;
		}
		current=*(fat+page2int(current));
		printf("at block %lu/n",page2int(current));
	}
	//now skip seek_bytes and read from it
	uint64_t counter = 0;//should be equal to count in the end
	uint64_t read_in_block = (1<<a->blocksize)-seek_bytes;
	if(read_in_block>=count)
		read_in_block = count;
	counter+=pread(fd, data, read_in_block, data_start + page2int(current)*(1<<a->blocksize) + seek_bytes);
	if(counter==count){
		ret=counter;
		goto ret;
	}
	seek_blocks = (count-counter)>>a->blocksize;
	seek_bytes = (count-counter)%(1<<a->blocksize);
	for(uint64_t i = 0; i<seek_blocks; i++){
		if(page2int(current)==0xFFFFFFFFFFFF){
			puts("unexpected EOF");
			ret=counter;
			goto ret;
		}
		current=*(fat+page2int(current));
		counter+=pread(fd, data+counter, (1<<a->blocksize), data_start + page2int(current)*(1<<a->blocksize));
		printf("reading block %lu/n",page2int(current));
	}
	read_in_block = (1<<a->blocksize)-seek_bytes;
	current=*(fat+page2int(current));
	if(page2int(current)==0xFFFFFFFFFFFF){
		puts("warning: malformed FAT");
	}
	if(!(count==counter))
		counter+=pread(fd, data+counter, read_in_block, data_start + page2int(current)*(1<<a->blocksize));
	ret = counter;
	ret:
		munmap(fat, fat_length);
		return ret;
}

uint64_t path2inode (header* a, char* p, int fd){
	//returnes a inode based on the path
	char* path = calloc(4096,1);
	char** pathv = calloc(128,8);
	strcpy(path,p);
	int pathix=0;
	for(int i = 0; i<4096; i++){
		//printf("at index %d, char %c\n",i,*(path+i));
		if(*(path+i)=='/'&&*(path+i+1)=='\0'){
			*(path+i)=0;
			break;
		}
		if(*(path+i)=='/'){
			*(path+i)=0;
			*(pathv+pathix++)=(char*)path+i+1;
		}
	}
	inode directory;
	get_inode(a,0,&directory,fd);
	file* dir=calloc(directory.size/128,128);
	file dirstruct;
	read_inode(a,0,&directory,dir,0,directory.size,fd);
	printf("size of root directory: %lu\n", directory.size);
	for(int i=0; i<pathix; i++){
		for(int j = 0; j<directory.size/128;j++){
			if(*((char*)(dir+j))==0)
				continue;
			printf("comparing %s with %s\n",(dir+j)->name,*(pathv+i));
			int cmp = strcmp((dir+j)->name, *(pathv+i));
			if(cmp==0){
				dirstruct=*(dir+j);
				goto found;
			}
		}
		free(path);
		free(pathv);
		free(dir);
		puts("no such file or directory");
		return -1; //no such file or directory
		found:
		free(dir);
		//if it is a directory, continue, if it is a file return -2 if i!=pathix-1
		if((dirstruct.attributes&1)==1){
			//directory
			get_inode(a,dirstruct.inode,&directory,fd);
			dir=calloc(directory.size/128,128);
			read_inode(a,dirstruct.inode,&directory,dir,0,directory.size,fd);
		}
		else{
			if(i!=pathix-1){
				free(path);
				free(pathv);
				puts("not a directory");
				return -2;
			}
		}
	}
	return dirstruct.inode;
}
