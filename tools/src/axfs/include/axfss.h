#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifndef NOSTDOUT
#include <stdio.h>
#endif

#include <stdlib.h>
#include <stdint.h>
#include <sys/ioctl.h>
//#include <linux/fs.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <time.h>
#define BT_BUF_SIZE 100
#pragma pack(1)
#pragma once
#define BUSYWAIT for(int i=0;i<2000000;i++);

#ifdef NOSTDOUT

int puts (const char*){
	return 0;
}
int printf (const char*, ...){
	return 0;
}
int perror (const char*){
	return 0;
}

#endif

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

page* fat = NULL;
inode* inodes = NULL;


typedef struct {
        page pag;
        uint64_t offset;
        uint32_t inode;
        uint32_t uses;
}last_page;

uint8_t cache_lock=0;

last_page* last=NULL;

void pivot_cache(int32_t a, int32_t b){
	printf("pivoting cache entry % with %\n",a,b);
	last_page c = *(last+a);
	*(last+a)=*(last+b);
	*(last+b)=c;
	return;
}

uint64_t min_free=0;

uint32_t modify_inode(header* a, uint32_t num, inode* inode_struct, int fd){
	puts("entering modify_inode");
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
	/*inode* inodes=mmap(NULL, inode_length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, inode_start);
	if (inodes == -1) {
                perror("mmap failed");
                return -1;
        }*/
	inode_struct->modified=time(NULL);
	*(inodes+num)=*(inode_struct);
	//munmap(inodes, inode_length);
	if(inode_struct->links==0){
		return -1;
	}
	return num;
}

uint32_t get_inode(header* a, uint32_t num, inode* inode_struct, int fd){
	puts("entering get_inode");
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
        /*inode* inodes=mmap(NULL, inode_length, PROT_READ, MAP_SHARED, fd, inode_start);
        if (inodes == -1) {
                perror("mmap failed");
                return -1;
        }*/
        *(inode_struct)=*(inodes+num);
	//munmap(inodes, inode_length);
	if(inode_struct->links==0){
		return -1;
	}
        return num;
}

uint32_t create_new_inode(header* a, inode* inode_struct, int fd){
	puts("entering create_new_inode");
	//set the permissions in inode struct first before
	const uint64_t inode_start = (1<<a->blocksize)*(a->resblocks + a->fatsize + 1);
	const uint64_t fat_tart = (1<<a->blocksize)*(a->resblocks + 1);
	const uint64_t data_start = (1<<a->blocksize)*(a->resblocks + a->fatsize + a->rootdirsize + 1);
	const uint64_t fat_length = (1<<a->blocksize)*a->fatsize;
	const uint64_t inode_length = (1<<a->blocksize)*a->inodes;
	//map inode map and FAT
	/*inode* inodes=mmap(NULL, inode_length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, inode_start);
	if (inodes == -1) {
                perror("mmap failed");
                return -1;
        }
	page* fat=mmap(NULL, fat_length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, fat_start);
	if (fat == -1) {
                perror("mmap failed");
                return -1;
        }*/
	for(uint64_t i = 0; i<inode_length/32; i++){
		if((inodes+i)->links==0){
			printf("found empty inode at %u\n", i);
			inode_struct->links=1; //just to avoid getting overwritten
			inode_struct->created=time(NULL);
			inode_struct->modified=time(NULL);
			//find a free starting FAT, write -1 to sign it as ended
			for(uint64_t j = 0; i<fat_length/6; j++){
				if(page2int(*(fat+j))==0){
					printf("\t\t\t\t\tfound free FAT at %lu\n", j);
					*(fat+j)=int2page(-1);
					inode_struct->start=int2page(j);
					*(inodes+i)=*inode_struct;
					return i;
				}
			}
			puts("disk is likely full");
		}
	}
	//munmap(inodes, inode_length);
	//munmap(fat, fat_length);
	printf("all inodes are used\n");
	return -1;
}

uint64_t shrink_inode(header* a, uint32_t inode_num, uint64_t count, int fd){
	puts("entering shrink_inode");
	//remove count bytes from inode
        const uint64_t inode_start = (1<<a->blocksize)*(a->resblocks + a->fatsize + 1);
        const uint64_t fat_start = (1<<a->blocksize)*(a->resblocks + 1);
        const uint64_t data_start = (1<<a->blocksize)*(a->resblocks + a->fatsize + a->rootdirsize + a->inodes + 1);
        const uint64_t fat_length = (1<<a->blocksize)*a->fatsize;
	const uint64_t blocksize = (1<<a->blocksize);
	int64_t ret = 0;

	for(int i = 0;i<(1<<16)-1;i++){
                if((last+i)->inode==inode_num){
                        cache_lock=1;
			printf("zeroing inode %d at %d\n", inode_num, i);
                        (last+i)->offset=0;
                        (last+i)->pag = int2page(0);
                        (last+i)->inode=0;
                        (last+i)->uses=0;
			cache_lock=0;
                        break;
                }
        }

	inode inode_struct;
	if(get_inode(a, inode_num, &inode_struct, fd)==-1){
		puts("nonexistant inode");
		return -1;
	}
	printf("count: %lu\n",count);
	page current = inode_struct.start;
	inode_struct.modified=time(NULL);
	//check if we actually need to deallocate blocks
	uint64_t size=inode_struct.size;
	uint64_t blocks_before;
	if(size==0||size==1) blocks_before=1;
	else blocks_before = (size-1)/blocksize+1;

	uint64_t blocks_after;
	if(size-count==0) blocks_after=1;
	else blocks_after = (size-count-1)/blocksize+1;

	printf("shrinking inode %lu\n", inode_num);
	if(blocks_before==blocks_after){
		puts("nothing to do");
		inode_struct.size=size-count;
		ret=inode_struct.size;
		goto ret;
	}
	skip:
	printf("traversing %lu blocks\n",blocks_after-1);
	for(uint64_t i = 0; i<blocks_after-1;i++)
		current=*(fat+page2int(current));
	printf("deallocating %lu blocks\n",blocks_before-blocks_after);
	page next = *(fat+page2int(current));
	printf("start of chain: %lu\n", next);
	*(fat+page2int(current))=int2page(-1);
	current=next;
	for(uint64_t i=0; i<blocks_before-blocks_after;i++){
		printf("freeing block %lu\n",next);
		next=*(fat+page2int(current));
		*(fat+page2int(current))=int2page(0);
		current=next;
		//if(page2int(current)<min_free) min_free=page2int(current);
	}
	min_free=0;
	inode_struct.size=size-count;
	ret=size-count;
	ret:
		//munmap(fat, fat_length);
		modify_inode(a, inode_num, &inode_struct, fd);
		return ret;
}

int32_t delete_inode(header* a, uint32_t inode_num, int fd){
	puts("entering delete_inode");
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
	*(fat+page2int(inode_struct.start))=int2page(0);
	inode_struct.links=0; //this functionally deletes the inode
	modify_inode(a, inode_num, &inode_struct, fd);
	ret = inode_num;
	ret:
		//munmap(fat, fat_length);
		return ret;
}

int64_t extend_inode(header* a, uint32_t inode_num, uint64_t count, int fd){
        puts("entering extend_inode");
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
        printf("extend: inode: %d\n",inode_num);
	uint64_t size = inode_struct.size;
        //calculate if we need new blocks
        if(size%blocksize+count<=blocksize){
                if(size%blocksize==0) goto skip;
                puts("nothing to do");
                inode_struct.size=size+count;
                ret=inode_struct.size;
                goto ret;
        }
        skip:
        page current=inode_struct.start;
        uint64_t block2stop;
        if(size==0||size==1)block2stop=0;
        else block2stop = (size-1)/blocksize;
	uint64_t alloc = (size+count-1)/blocksize-block2stop;
	uint8_t lock = 0;
	static unsigned int period = 0;

	if( size>blocksize*12 ){
                //scan for the inode
		int64_t most_used=-1;
		int32_t uses=0;
                for(int i = 0; i<(1<<16)-1; i++){
			//printf("scanning index %d with inode %d\n",i,(last+i)->inode);
			if((last+i)->uses>uses && most_used!=-1){
				most_used=i;
				uses=(last+i)->uses;
				if((period&7)==6)
				if(most_used<i)
					pivot_cache(i, most_used);
			}
			if((last+i)->inode==inode_num){
                                printf("cache hit at %d for inode %d\n", i, inode_num);
				if( page2int((last+i)->pag) != 0        &&      (last+i)->offset<size ){
                                        //recalculate seek
                                        block2stop = (size-(last+i)->offset-1)/blocksize;
                                        current=(last+i)->pag;
                                }
                                (last+i)->uses+=1;
				if((last+i)->uses>uses && most_used!=-1){
					pivot_cache(i, most_used);
				}
				break;
                        }
                }
		period++;
        }

	printf("size: %lu\n",size);
	printf("traversing %lu blocks\n",block2stop);
        for(uint64_t i = 0; i<block2stop;i++){
                current=*(fat+page2int(current));
	}
	//now current points to -1
        //calculate blocks to allocate
        printf("allocating %lu blocks\n",alloc);
	uint64_t free=min_free;
	uint64_t onelast=page2int(current);
	for(uint64_t i=0; i<alloc;i++){
                for(uint64_t j=free+1; j<fat_length/6;j++){
			//printf("traversing block %lu\n", j);
			if(page2int(*(fat+j))==0){
                                printf("empty block at %lu\n",j);
                                *(fat+page2int(current))=int2page(j);
                                current=int2page(j);
				free=j;
				if(i+2==alloc)
					onelast=j;
                                break;
                        }
                }
        }
	*(fat+free)=int2page(-1);
	min_free=free;
        inode_struct.size=size+count;
        ret=size+count;

	puts("memdump of FAT");
	for(int i = 0; i<10; i++){
		printf("%lu\n",page2int(*(fat+i)));
	}

        ret:
                printf("new size: %lu\n",inode_struct.size);
                modify_inode(a, inode_num, &inode_struct, fd);

                return ret;
}


int64_t write_inode(header *a, uint32_t inode_num, void *data,uint64_t seek, uint64_t count, int fd) {
	const uint64_t blocksize = 1 << a->blocksize;
	const uint64_t fat_start = blocksize * (a->resblocks + 1);
	const uint64_t data_start = blocksize * (a->resblocks + a->fatsize + a->rootdirsize + a->inodes + 1);
	const uint64_t fat_length = blocksize * a->fatsize;
	inode ino;
	printf("write: inode: %d\n",inode_num);
	if (get_inode(a,inode_num, &ino, fd) == -1) return -1;
	if (seek > ino.size) return -1;

	//expand if necessary
	if (seek + count > ino.size) {
		if (extend_inode(a, inode_num,seek+count-ino.size,fd)<0) return -1;
	 	get_inode(a, inode_num, &ino, fd);
	}
	//starting position
	uint64_t seek_blocks = seek / blocksize;
	uint64_t seek_bytes= seek % blocksize;
	page current = ino.start;
	printf("seek blocks: %lu\n",seek_blocks);
	uint64_t seek_dif = seek_blocks;
	//checking if cache hit:

	int32_t index = -1;
        int32_t freeblock = -1;
        int32_t leastused = -1;
	static unsigned int period=0;
        if( seek_blocks>5 ){
                //scan for the inode
                uint32_t uses=-1;
		int32_t use=0;
		int64_t  most_used=-1;
                for(int i = 0; i<(1<<16)-1; i++){
                        if((last+i)->uses>use){
				use=(last+i)->uses;
				if((period&8)==2){
					if(most_used<i && most_used!=-1){
						pivot_cache(i, most_used);
					}
				}
				most_used=i;
			}
			if((last+i)->offset==0&&freeblock==-1){
                                freeblock=i;
                        }
                        if((last+i)->uses<uses){
                                uses=(last+i)->uses;
                                leastused=i;
                        }
                        if((last+i)->inode==inode_num){
				(last+i)->uses++;
				if((last+i)->uses>uses && most_used!=-1){
					if((last+i)->uses>use && most_used<i){
						pivot_cache(i, most_used);
						index=most_used;
					}
					else index=i;
				}
                                else index=i;
                                break;
                        }
                }
		period++;
        }
        printf("cached index: %d\n",index);
        printf("freeblock: %d\n",freeblock);
	if(index!=-1)
        if( page2int((last+index)->pag) != 0    &&      (last+index)->offset<=seek ){
                //recalculate seek
                seek_blocks = (seek-(last+index)->offset)/blocksize;
                seek_bytes = (seek-(last+index)->offset)%blocksize;
                (last+index)->uses+=1;
                current=(last+index)->pag;
        }

        seek_dif-=seek_blocks;
	if(index!=-1)
        printf("cached page: %lu\n",page2int((last+index)->pag));
        printf("seek blocks: %lu\n",seek_blocks);
        // 7. Traverse FAT to starting block
        uint64_t traversed=0;
        for (uint64_t i = 0; i < seek_blocks; i++) {
                //printf("going to block %lu\n",page2int(current));
                current = *(fat + page2int(current));
                traversed++;
                if(i==seek_blocks-2){
			cache_lock=1;
                        if(index!=-1){
                                puts("caching entry");
                                (last+index)->pag=current;
                                (last+index)->offset=(seek_dif+traversed)*blocksize;
                        }else if (freeblock!=-1){
                                puts("caching entry(newblock)");
                                (last+freeblock)->pag=current;
                                (last+freeblock)->offset=(seek_dif+traversed)*blocksize;
                                (last+freeblock)->inode=inode_num;
                        }else if (leastused != -1){
                                puts("caching entry(overwrite)");
                                (last+leastused)->inode=inode_num;
                                (last+leastused)->pag=current;
                                (last+leastused)->offset=(seek_dif+traversed)*blocksize;
                        }
			cache_lock=0;
			printf("offset: %lu, page: %lu\n",(seek_dif+traversed)*blocksize,page2int(current));
                }
        }

	uint8_t *buf = (uint8_t*)data;
	uint64_t bytes_written=0;
	uint64_t remaining = count;
	//first block
	uint64_t to_write = blocksize - seek_bytes;
	if (to_write > remaining) to_write = remaining;
	uint64_t result = pwrite(fd, buf + bytes_written, to_write,data_start + page2int(current)*blocksize + seek_bytes);
	printf("result: %lu\n",result);
	if (result < 0)
		return -1;
	bytes_written += result;
	remaining -= result;
	//subsequent blocks
	while (remaining > 0) {
		if (page2int(current) == 0xFFFFFFFFFFFF)break;
		printf("going to block %lu",page2int(current));
		current = *(fat + page2int(current));
		to_write = (remaining < blocksize) ? remaining : blocksize;

		result = pwrite(fd, buf + bytes_written, to_write, data_start + page2int(current) * blocksize);
		if (result <0)return -1;
		bytes_written += result;
		remaining -= result;
	}
	//munmap(fat, fat_length);
	return bytes_written;
}


int64_t read_inode(header *a, uint32_t inode_num, inode *inode_struct,  void *data, uint64_t seek, uint64_t count, int fd) {
	const uint64_t blocksize = 1 << a->blocksize;
	const uint64_t fat_start = blocksize * (a->resblocks + 1);
	const uint64_t data_start = blocksize * (a->resblocks + a->fatsize + a->rootdirsize + a->inodes + 1);
	const uint64_t fat_length = blocksize * a->fatsize;

	if (get_inode(a, inode_num, inode_struct, fd) ==-1) return -1;
	if(seek >= inode_struct->size)
		return 0;
	//clamp
	if(seek + count > inode_struct->size)
		count = inode_struct->size - seek;
	if(!count) return 0;
	printf("read: inode %d\n",inode_num);
	uint64_t seek_blocks = seek / blocksize;
	uint64_t seek_bytes  = seek % blocksize;
	page current = inode_struct->start;
	uint64_t seek_dif = seek_blocks;
	//checking if cache hit:

        int32_t index = -1;
        int32_t freeblock = -1;
        int32_t leastused = -1;
        if( seek_blocks>5 ){
                //scan for the inode
                uint32_t uses=-1;
                for(int i = 0; i<(1<<16)-1; i++){
			if((last+i)->offset==0&&freeblock==-1){
                                freeblock=i;
                        }
                        if((last+i)->uses<uses){
                                uses=(last+i)->uses;
                                leastused=i;
                        }
                        if((last+i)->inode==inode_num){
                                index=i;
                                break;
                        }
                }
        }
        printf("cached index: %d\n",index);
        if(index!=-1)
        if( page2int((last+index)->pag) != 0    &&      (last+index)->offset<=seek ){
                //recalculate seek
                seek_blocks = (seek-(last+index)->offset)/blocksize;
                seek_bytes = (seek-(last+index)->offset)%blocksize;
                (last+index)->uses+=1;
                current=(last+index)->pag;
        }

        seek_dif-=seek_blocks;
	if(index!=-1)
        printf("cached page: %lu\n",page2int((last+inode_num)->pag));
        printf("seek blocks: %lu\n",seek_blocks);
        // 7. Traverse FAT to starting block
        uint64_t traversed=0;
        for (uint64_t i = 0; i < seek_blocks; i++) {
                printf("read: going to block %lu\n",page2int(current));
                current = *(fat + page2int(current));
                traversed++;
		if(cache_lock==0)
                if(i==seek_blocks-2){
			cache_lock=1;
                        if(index!=-1){
                                puts("caching entry");
                                (last+index)->pag=current;
                                (last+index)->offset=(seek_dif+traversed)*blocksize;
                        }else if (freeblock!=-1){
                                puts("caching entry(newblock)");
                                (last+freeblock)->pag=current;
                                (last+freeblock)->offset=(seek_dif+traversed)*blocksize;
                                (last+freeblock)->inode=inode_num;
                        }else if (leastused != -1){
                                puts("caching entry(overwrite)");
                                (last+leastused)->inode=inode_num;
                                (last+leastused)->pag=current;
                                (last+leastused)->offset=(seek_dif+traversed)*blocksize;
                        }
			printf("page: %lu, offset: %lu\n",(seek_dif+traversed)*blocksize,page2int(current));
                	cache_lock=0;
		}
        }

	uint8_t *buf = (uint8_t *)data;
	uint64_t bytes_read = 0;
	uint64_t remaining = count;

	// first block
	uint64_t to_read = blocksize - seek_bytes;
	if (to_read > remaining) to_read = remaining;
	uint64_t result = pread(fd, buf + bytes_read, to_read,data_start + page2int(current) * blocksize + seek_bytes);
    	if (result < 0)return -1;
	bytes_read += result;
	remaining -= result;
	while (remaining > 0) {
		if (page2int(current) == 0xFFFFFFFFFFFF) break;
		current = *(fat +page2int(current));
		to_read = (remaining < blocksize) ? remaining : blocksize;
		result = pread(fd, buf + bytes_read, to_read , data_start + page2int(current) * blocksize);
		if (result < 0)return -1;
		bytes_read += result;
		remaining -=result;
		}

//	(last_in_chain+inode_num)->offset=int2page(offset);
//	(last_in_chain+inode_num)->pag = current;
   // munmap(fat, fat_length);
	return bytes_read;
}

int32_t eval_permissions(header* a, uint32_t inum, uint16_t mask/*3 bits for now*/, uint16_t user, uint16_t* groups, uint16_t groupc, int fd){

	//-1 = access denied
	//0 = allowed
	if(user==0)
		return 0; //root bypasses all permissions
	inode inod;
	get_inode(a,inum,&inod,fd);
	//check others first
	if(((inod.permissions&7)&mask)==mask){
		puts("permissions satisfied");
		return 0;
	}
	//check user now
	if(user==inod.user){
		if((((inod.permissions>>6)&7)&mask)==mask){
			puts("permissions satisfied");
			return 0;
		}
	}
	//check groups
	for(int i=0; i<groupc; i++){
		if(*(groups+i)==inod.group){
			if((((inod.permissions>>3)&7)&mask)==mask){
				puts("permissions satisfied");
				return 0;
			}
		}
	}
	puts("permission denied");
	return -1;
}

uint64_t path2inode (header* a, char* p, uint16_t user, uint16_t* groups, uint16_t groupc, int fd){
	//returnes a inode based on the path
	char* path = calloc(4096,1);
	char** pathv = calloc(1024,8);
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
	printf("path: ");
	write(1,path+1,4095);
	puts("");
	inode directory;
	get_inode(a,0,&directory,fd);
	file* dir=malloc(directory.size);
	file dirstruct={"/",1,0};
	read_inode(a,0,&directory,dir,0,directory.size,fd);
	printf("size of root directory: %lu\n", directory.size);
	for(int i=0; i<pathix; i++){
		for(int j = 0; j<directory.size/128;j++){
			//if(*((char*)(dir+j))==0)
			//	continue;
			printf("comparing %s with %s\n",(dir+j)->name,*(pathv+i));
			int cmp = strcmp((dir+j)->name, *(pathv+i));
			if(cmp==0){
				dirstruct=*(dir+j);
				break;
			}
			if(j+1==directory.size/128){
				free(path);
				free(pathv);
				free(dir);
				puts("no such file or directory");
				return -1; //no such file or directory
			}
		}
		free(dir);
		//stop at symlink
		if(dirstruct.attributes==2){
			//safe to break
			break;
		}
		if(dirstruct.attributes==1){
			//directory
			get_inode(a,dirstruct.inode,&directory,fd);
			dir=malloc(directory.size);
			//check execute bit
			if (eval_permissions(a,dirstruct.inode, 01, user, groups, groupc, fd)){
				puts("permission denied");
				free(dir);
				free(path);
				free(pathv);
				return -2;
			}
			read_inode(a,dirstruct.inode,&directory,dir,0,directory.size,fd);
		}
		else{
			if(i!=pathix-1){
				free(path);
				free(pathv);
				puts("not a directory");
				return -1;
			}
		}
	}
	inode inod;
	get_inode(a,dirstruct.inode,&inod,fd);
	printf("inode: %d, links %d\n",dirstruct.inode,inod.links);
	free(path);
	free(pathv);
	if(dirstruct.attributes == 1)
		free(dir);
	return dirstruct.inode|((uint64_t)dirstruct.attributes<<32);
}

int64_t create_file(header* a, char* p, uint16_t permissions, uint16_t user, uint16_t* groups, uint16_t groupc, int fd){
	puts("entered create_file");
	//find the last entry
	char* path = calloc(4096,1);
	strcpy(path,p);
	uint16_t last_slash_pos = 0;
	for (int i = 0; i<4096&&*path; i++){
		if(*(path+i)=='/')
			last_slash_pos = i;
	}
	*(path+last_slash_pos)=0;
	puts(path+last_slash_pos+1);
	uint64_t inum = path2inode (a, path, user, groups, groupc, fd);
	if(inum==-1||inum==-2){
		free(path);
		return -1;
	}
	if(inum>>32==0){
		free(path);
		puts("not a directory");
		return -2;
	}
	if(inum>>32==2){
		free(path);
		puts("refusing to follow symlink");
		return -1;
	}
	inode inod;
	get_inode(a, inum, &inod, fd);
	//check permissions (r, w and x)
	if(eval_permissions(a,inum, 07, user, groups, groupc, fd)){
		puts("permission denied");
		free(path);
		return -1;
	}
	//check if file already exists
	file* dir=calloc(inod.size/128,128);
	read_inode(a, inum, &inod, dir, 0, inod.size, fd);
	int free_slot=-1;
	for(int i=0; i<inod.size/128; i++){
		if(*((char*)(dir+i))==0){
			free_slot=i;
			continue;
		}
		else{
			if(!strcmp(path+last_slash_pos+1,(dir+i)->name)){
				puts("name already taken");
				free(path);
				inum=(dir+i)->inode;
				free(dir);
				return inum;
			}
		}
	}
	inode new_inode={permissions,user,*groups,0,0,0,0,0,{0,0}};
	uint32_t new_inum = create_new_inode(a,&new_inode,fd);
	if(new_inum==-1){
		free(path);
		free(dir);
		puts("error making inode");
		return -3;
	}
	printf("new inode: %d\n", new_inum);
	file new_file={" ",0,new_inum};
	strcpy(new_file.name, path+last_slash_pos+1);
	if(free_slot==-1)
		//write after
		write_inode(a,inum,&new_file,inod.size,128,fd);
	else
		write_inode(a,inum,&new_file,128*free_slot,128,fd);
	free(path);
	free(dir);
	return new_inum;
}

int64_t create_directory(header* a, char* p, uint16_t permissions, uint16_t user, uint16_t* groups, uint16_t groupc, int fd){
	//find the last entry
	char* path = calloc(4096,1);
	strcpy(path,p);
	uint16_t last_slash_pos = 0;
	for (int i = 0; i<4096&&*path; i++){
		if(*(path+i)=='/')
			last_slash_pos = i;
	}
	*(path+last_slash_pos)=0;
	puts(path+last_slash_pos+1);
	uint64_t inum = path2inode (a, path, user, groups, groupc, fd);
	if(inum==-1||inum==-2){
		free(path);
		return -1;
	}
	if(inum>>32==0){
		free(path);
		puts("not a directory");
		return -2;
	}
	if(inum>>32==2){
		free(path);
		puts("refusing to follow symlink");
		return -1;
	}
	inode inod;
	get_inode(a, inum, &inod, fd);
	//check permissions (r, w and x)
	if(eval_permissions(a,inum, 07, user, groups, groupc, fd)){
		free(path);
		puts("permission denied");
		return -1;
	}
	//check if file already exists
	file* dir=calloc(inod.size/128,128);
	read_inode(a, inum, &inod, dir, 0, inod.size, fd);
	int free_slot=-1;
	for(int i=0; i<inod.size/128; i++){
		if(*((char*)(dir+i))==0){
			free_slot=i;
			continue;
		}
		else{
			if(!strcmp(path+last_slash_pos+1,(dir+i)->name)){
				puts("name already taken");
				free(path);
				inum=(dir+i)->inode;
				free(dir);
				return inum;
			}
		}
	}
	inod.links=inod.links+1; //to include ..
	modify_inode(a,inum,&inod,fd);
	free(dir);
	inode new_inode={permissions,user,*groups,0,0,0,0,0,{0,0}};
	uint32_t new_inum = create_new_inode(a,&new_inode,fd);
	new_inode.links=2;
	modify_inode(a,new_inum,&new_inode,fd);
	if(new_inum==-1){
		puts("error making inode");
		free(path);
		return -1;
	}
	printf("new inode: %d\n", new_inum);
	file new_file={" ",1,new_inum};
	strcpy(new_file.name, path+last_slash_pos+1);
	free(path);
	if(free_slot==-1)
		//write after
		write_inode(a,inum,&new_file,inod.size,128,fd);
	else
		write_inode(a,inum,&new_file,128*free_slot,128,fd);
	//make . and ..
	dir=calloc(256,1);
	*(dir+0)=(file){".",1,new_inum};
	*(dir+1)=(file){"..",1,inum};
	write_inode(a, new_inum, dir, 0, 256, fd);
	free(dir);
	return new_inum;
}


int64_t create_symlink(header* a, char* p, char* dest, uint16_t user, uint16_t* groups, uint16_t groupc, int fd){
	//find the last entry
	char* path = calloc(4096,1);
	strcpy(path,p);
	uint16_t last_slash_pos = 0;
	for (int i = 0; i<4096&&*path; i++){
		if(*(path+i)=='/')
			last_slash_pos = i;
	}
	*(path+last_slash_pos)=0;
	puts(path+last_slash_pos+1);
	uint64_t inum = path2inode (a, path, user, groups, groupc, fd);
	if(inum==-1||inum==-2){
		free(path);
		return -1;
	}
	if(inum>>32==0){
		free(path);
		puts("not a directory");
		return -2;
	}
	if(inum>>32==2){
		free(path);
		puts("refusing to follow symlink");
		return -1;
	}
	inode inod;
	get_inode(a, inum, &inod, fd);
	//check permissions (r, w and x)
	if(eval_permissions(a,inum, 07, user, groups, groupc, fd)){
		puts("permission denied");
		return -1;
	}
	//check if file already exists
	file* dir=calloc(inod.size/128,128);
	read_inode(a, inum, &inod, dir, 0, inod.size, fd);
	int free_slot=-1;
	for(int i=0; i<inod.size/128; i++){
		if(*((char*)(dir+i))==0){
			free_slot=i;
			continue;
		}
		else{
			if(!strcmp(path+last_slash_pos+1,(dir+i)->name)){
				puts("name already taken");
				free(path);
				inum=(dir+i)->inode;
				free(dir);
				return inum;
			}
		}
	}
	inode new_inode={0777,user,*groups,0,0,0,0,0,{0,0}};
	uint32_t new_inum = create_new_inode(a,&new_inode,fd);
	if(new_inum==-1){
		puts("error making inode");
		return -3;
	}
	printf("new inode: %d\n", new_inum);
	//calculate size of target
	uint64_t siz=0;
	for(siz = 0; siz<4096; siz++)
		if(!*(dest+siz))
			break;

	write_inode(a,new_inum,dest,0,siz,fd);
	file new_file={" ",2,new_inum};
	strcpy(new_file.name, path+last_slash_pos+1);
	if(free_slot==-1)
		//write after
		write_inode(a,inum,&new_file,inod.size,128,fd);
	else
		write_inode(a,inum,&new_file,128*free_slot,128,fd);
	free(path);
	free(dir);
	return new_inum;
}

uint64_t unlink_file(header* a, char* p, uint16_t user, uint16_t* groups, uint16_t groupc, int fd){
	//deletes directory entry and decrements link count
	char* path = calloc(4096,1);
	strcpy(path,p);
	uint16_t last_slash_pos = 0;
	for (int i = 0; i<4096&&*path; i++){
		if(*(path+i)=='/')
			last_slash_pos = i;
	}
	*(path+last_slash_pos)=0;
	puts(path+last_slash_pos+1);
	uint64_t inum = path2inode (a, path, user, groups, groupc, fd);
	if(inum==-1){
		free(path);
		return -1;
	}
	if(inum==-2){
		free(path);
		return -2;
	}
	if(inum>>32==0){
		free(path);
		puts("not a directory");
		return -1;
	}
	if(inum>>32==2){
		free(path);
		puts("refusing to follow symlink");
		return -1;
	}
	inode inod;
	get_inode(a, inum, &inod, fd);
	//check permissions of parent (r, w and x)
	if(eval_permissions(a,inum, 07, user, groups, groupc, fd)){
		puts("unlink: permission denied");
		return -2;
	}
	//check if file  exists
	file* dir=calloc(inod.size/128,128);
	read_inode(a, inum, &inod, dir, 0, inod.size, fd);
	int free_slot=-1;
	int ix=0;
	for(int i=0; i<inod.size/128; i++){
		if(*((char*)(dir+i))==0){
			free_slot=i;
			continue;
		}
		else{
			if(!strcmp(path+last_slash_pos+1,(dir+i)->name)){
				ix=i;
				goto success;
			}
		}
	}
	puts("no such file or directory");
	free(dir);
	free(path);
	return -1;
	success:
	//delete entry and decrement link count
	puts("deleting entry");
	file deleted={"\0",0,0};
	inode del;
	printf("inode of %s: %d\n", path, inum);
	get_inode(a,(dir+ix)->inode,&del,fd);
	if(del.links==1)
		delete_inode(a,(dir+ix)->inode,fd);
	else{
		del.links=del.links-1;
		modify_inode(a,(dir+ix)->inode,&del,fd);
	}
	free(dir);
	free(path);
	if(inum==0){
		puts("trying to delete root, ignoring request");
	}
	else write_inode(a,inum,&deleted,ix*128,128,fd);
	if(ix==inod.size/128-1){ //if the file struct 
		printf("size of inode: %d\n",inod.size);
		if(inum==0){
			puts("trying to delete root, ignoring request");
		}
		else shrink_inode(a,inum,128,fd);
	}
	return del.links;
}

int64_t size_chain(header* a, page current, int fd){
	const uint64_t blocksize = 1 << a->blocksize;
	const uint64_t fat_start = blocksize * (a->resblocks + 1);
	const uint64_t fat_length = blocksize * a->fatsize;
	int64_t i;
	if(page2int(current)==0xFFFFFFFFFFFF){
		puts("FSCK: invalid FAT page");
		return -1;
	}
	for(i = 0; i<fat_length/6; i++){
		current=*(fat+page2int(current));
		if(page2int(current)==0xFFFFFFFFFFFF){
			printf("FSCK: chain length (zero indexed): %lu\n",i);
			break;
		}
	}
	return i;
}

int64_t scan_filesystem(header* a, int fd){
	const uint64_t blocksize = 1 << a->blocksize;
	const uint64_t fat_start = blocksize * (a->resblocks + 1);
	const uint64_t data_start = blocksize * (a->resblocks + a->fatsize + a->rootdirsize + a->inodes + 1);
	const uint64_t fat_length = blocksize * a->fatsize;

	for(uint32_t i = 0; i< blocksize*a->inodes/32; i++){
		if((inodes+i)->links==0)
			continue;
		printf("FSCK: info of inode %d\n",i);
		printf("FSCK: \texpected size: %lu bytes\n\tstart: %lu\n",(inodes+i)->size,page2int((inodes+i)->start));
		int64_t size = size_chain(a,(inodes+i)->start,fd);
		printf("FSCK: \tactual size: %ld blocks\n",size);
		if((inodes+i)->size!=0){
			if(size==((inodes+i)->size-1)/blocksize)
				puts("FSCK: inode OK\n");
			else{
				printf("FSCK: inode %du has errors\nFSCK: press y to repair, nothing to continue",i);
				char answer;
				read(0,&answer,1);
				if(answer=='y'){
					//compare size with clamped size
					//int64_t clamped = (inodes+i)->size-1)/blocksize;
					//if(clamped>size)
					//shrink_inode(a,i,(inodes+i)->size-,fd
				}
			}
		}
		else if(size==0)
			puts("FSCK: inode OK\n");
	}
	puts("FSCK complete");
}
