//NAME: Xiwei Ma,Yunong Ye
//EMAIL: xiweimacn@163.com,yeyunong@hotmail.com
//ID: 704755732,004757414

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include "EXT2_header.h"

char *filename;
int fd_image;
//int stdout;
char *output_file = "output.csv";
char *buf;

int block_size;
__u32* inode_block;
int group_block_bitmap;
int group_inode_bitmap;
//int group_num;

struct ext2_super_block *sp;
struct ext2_group_desc *gp;

void get_superblock_summary()
{
    /*
    single new-line terminated line, comprised of eight comma-separated fields
    SUPERBLOCK
    total number of blocks (decimal)
    total number of i-nodes (decimal)
    block size (in bytes, decimal)
    i-node size (in bytes, decimal)
    blocks per group (decimal)
    i-nodes per group (decimal)
    first non-reserved i-node (decimal)
    */
    int check = pread(fd_image, buf, 1024, 1024);
    if(check == -1){
        fprintf(stderr, "Fail to read: %s \n", strerror(errno));
        exit(2);
    }
    sp = (struct ext2_super_block *) buf;
    fprintf(stdout,"SUPERBLOCK,");
    int num_of_blocks = sp->s_blocks_count;
    fprintf(stdout,"%d,", num_of_blocks);
    int num_of_inodes = sp->s_inodes_count;
    fprintf(stdout,"%d,", num_of_inodes);
    block_size = EXT2_MIN_BLOCK_SIZE << (sp->s_log_block_size);
    fprintf(stdout,"%d,", block_size);
    fprintf(stdout,"%d,", sp->s_inode_size);
    fprintf(stdout,"%d,", sp->s_blocks_per_group);
    fprintf(stdout,"%d,", sp->s_inodes_per_group);
    fprintf(stdout,"%d\n", sp->s_first_ino);
}

void get_group_summary()
{
    
    //group_num = (sp->s_blocks_count) / (sp->s_blocks_per_group) + 1;
    //int inode_remain = (sp->s_inodes_count) % (sp->s_inodes_per_group);
    /*
    GROUP
    group number (decimal, starting from zero)
    total number of blocks in this group (decimal)
    total number of i-nodes in this group (decimal)
    number of free blocks (decimal)
    number of free i-nodes (decimal)
    block number of free block bitmap for this group (decimal)
    block number of free i-node bitmap for this group (decimal)
    block number of first block of i-nodes in this group (decimal)
    */
   int block_num_group = (sp->s_blocks_count) % (sp->s_blocks_per_group);
   int num_inode = sp->s_inodes_per_group;
    /*int i;
    for (i = 0; i < group_num; i++)
    {
        int check = pread(fd_image, buf, sizeof(struct ext2_group_desc), 2048);
        if(check == -1){
            fprintf(stderr, "Fail to read: %s \n", strerror(errno));
            exit(2);
        }
        gp = (ext2_group_desc *)buf;
        fprintf(stdout,"GROUP,");
        fprintf(stdout,"%d,", i );
        fprintf(stdout,"%d,",);
        fprintf(stdout,"%d,", );
        fprintf(stdout,"%d,", );
        fprintf(stdout,"%d,", );
        fprintf(stdout,"%d,", );
    }
    */
    int check = pread(fd_image, buf, sizeof(struct ext2_group_desc), 2048);
    if(check == -1){
        fprintf(stderr, "Fail to read: %s \n", strerror(errno));
        exit(2);
    }
    gp = (struct ext2_group_desc *)buf;
    fprintf(stdout,"GROUP,");
    fprintf(stdout,"%d,", 0 );
    fprintf(stdout,"%d,", block_num_group);
    fprintf(stdout,"%d,", num_inode);
    fprintf(stdout,"%d,", gp->bg_free_blocks_count);
    fprintf(stdout,"%d,", gp->bg_free_inodes_count);
    group_block_bitmap = gp->bg_block_bitmap;
    fprintf(stdout,"%d,", group_block_bitmap);
    group_inode_bitmap = gp->bg_inode_bitmap;
    fprintf(stdout,"%d,", group_inode_bitmap);
    fprintf(stdout,"%d\n", gp->bg_inode_table);
}

void get_free_block_entries()
{
    int i;
    for(i = 0; i < block_size; i++)
    {
        char c;
        int check = pread(fd_image, &c, 1, group_block_bitmap * block_size + i);
        if(check == -1){
            fprintf(stderr, "Fail to read: %s \n", strerror(errno));
            exit(2);
        }
        int j;
        for(j = 0; j < 8; j++) 
		{
			if((c & (1 << j)) == 0) 
			{
				fprintf(stdout, "BFREE,%d\n",(i*8)+j+1);
			}
		}
    }
}

void get_free_inode_entries()
{
    int i;
    for(i = 0; i < block_size; i++)
    {
        char c;
        int check = pread(fd_image, &c, 1, group_inode_bitmap * block_size + i);
        if(check == -1){
            fprintf(stderr, "Fail to read: %s \n", strerror(errno));
            exit(2);
        }
        int j;
        for(j = 0; j < 8; j++) 
		{
			if((c & (1 << j)) == 0) 
			{
				fprintf(stdout, "IFREE,%d\n",(i*8)+j+1);
			}
		}
    }
}


void get_directory_entries(int parent_inode)
{
    int i;
    for(i = 0; i < EXT2_N_BLOCKS; i++)
    {
        if(inode_block[i] == 0)
            break;
        int dir_offset = 0;
        int start_offset = inode_block[i] * block_size;
	int check_type = 1;
        while(dir_offset < block_size && check_type)
        {
	    int check = pread(fd_image, buf, sizeof(struct ext2_dir_entry), start_offset + dir_offset);
            if(check == -1){
                fprintf(stderr, "Fail to read: %s \n", strerror(errno));
                exit(2);
            }
            struct ext2_dir_entry *dir = (struct ext2_dir_entry *)buf;
            check_type = dir->file_type;


	    if(dir->inode != 0)
            {
                /*
                    DIRENT
                    parent inode number (decimal) ... the I-node number of the directory that contains this entry
                    logical byte offset (decimal) of this entry within the directory
                    inode number of the referenced file (decimal)
                    entry length (decimal)
                    name length (decimal)
                    name (string, surrounded by single-quotes).
                */
               fprintf(stdout, "DIRENT,");
               fprintf(stdout,"%d,%d,%d,", parent_inode, dir_offset , dir->inode);
               fprintf(stdout, "%d,%d,'%s'\n",dir->rec_len, dir->name_len, dir->name);
            }
            dir_offset += dir->rec_len;
        }
    }
}

/*
void get_indirect_helper(int block_num,int level, int parent_inode)
{
    int *arr = (int *)buf;
    int indirect_num = block_size/4;
    int i;
    for(i = 0; i < indirect_num ;i++)
    {
        if(arr[i] == 0)
            return;
        int dir_offset = 0;
        int start_offset = arr[i] * block_size;
        while(dir_offset < block_size)
        {
            struct ext2_dir_entry dir;
            int check = pread(fd_image, &dir, sizeof(struct ext2_dir_entry), start_offset + dir_offset);
            if(check == -1){
                fprintf(stderr, "Fail to read: %s \n", strerror(errno));
                exit(2);
            }
            if(dir.inode != 0)
            {
               fprintf(stdout, "INDIRECT,");
               fprintf(stdout,"%d,%d,%d,", parent_inode, level, block_num + i);
               fprintf(stdout, "%d,%d\n", block_num, block_num + i + 1);
            }
            
            INDIRECT
            I-node number of the owning file (decimal)
            (decimal) level of indirection for the block being scanned ... 1 for single indirect, 2 for double indirect, 3 for triple
            logical block offset (decimal) represented by the referenced block. If the referenced block is a data block, this is the logical block offset of that block within the file. If the referenced block is a single- or double-indirect block, this is the same as the logical offset of the first data block to which it refers.
            block number of the (1, 2, 3) indirect block being scanned (decimal) . . . not the highest level block (in the recursive scan), but the lower level block that contains the block reference reported by this entry.
            block number of the referenced block (decimal)
            
            dir_offset += dir.rec_len;
        }
    }
}

void get_indirect_block_references(int parent_inode)
{
    int block_number = inode_block[12];
    if( block_number <= 0)
        return;
    pread(fd_image, buf, block_size, block_number * block_size);
    get_indirect_helper(12,1,parent_inode);

    block_number = inode_block[13];
    if( block_number <= 0)
        return;
    int *arr2 = malloc(block_size);
    pread(fd_image, arr2 , block_size, block_number * block_size);
    int i;
    for(i = 0; i < block_size/4 ; i++)
    {
        pread(fd_image, buf, block_size, arr2[i] * block_size);
        get_indirect_helper(13,2,parent_inode);
    }

    block_number = inode_block[14];
    if( block_number <= 0)
        return;
    int *arr3 = malloc(block_size);
    pread(fd_image, arr2 , block_size, block_number * block_size);
    for(i = 0; i < block_size/4 ; i++)
    {
        pread(fd_image, arr3, block_size, arr2[i] * block_size);
        int j;
        for(j = 0; j <  block_size/4 ; j++)
        {
            pread(fd_image, buf, block_size, arr3[j] * block_size);
            get_indirect_helper(14,3,parent_inode);
        }
    }
}
*/
/*
void get_indirect_block_references(int parent_inode, int level, int block_num)
{
    int block_number = inode_block[block_num];
    int indirect_num = block_size/4;
    int buffer[indirect_num];
    memset(buffer, 0, sizeof(buffer));
    pread(fd_image, buffer, block_size, block_number * block_size);
    int i;
    for(i = 0; i < indirect_num ;i++)
    {
        int block = buffer[i];
        if(block != 0)
        {
            fprintf(stdout, "INDIRECT,");
            fprintf(stdout,"%d,%d,%d,", parent_inode, level, block_num + i);
            fprintf(stdout, "%d,%d\n", block_number, block);
            if(level != 1)
            {
                fprintf(stdout, "emmm,%d\n",i);
                get_indirect_block_references(parent_inode,level-1, block);
            }
        }
    }
}*/

void get_indirect_block_references(int parent_inode, int level, int block_number, int block_num)
{
    int indirect_num = block_size/4;
    int buffer[indirect_num];
    memset(buffer, 0, sizeof(buffer));
    pread(fd_image, buffer, block_size, block_number * block_size);
    int i;
    for(i = 0; i < indirect_num ;i++)
    {
        int block = buffer[i];
        if(block != 0)
        {
            fprintf(stdout, "INDIRECT,");
            fprintf(stdout,"%d,%d,%d,", parent_inode, level, block_num);
            fprintf(stdout, "%d,%d\n", block_number, block);
            if(level != 1)
            {
                get_indirect_block_references(parent_inode,level-1, block, block_num);
            }
            else
                ++block_num;
        }
        else
        {
            if(level == 1)
                ++block_num;
            else if (level == 2)
            {
                block_num += 256;
            }
            else
            {
                block_num += 65536;
            }
        }
    }
}


void timefun(__u32 time, char * result)
{
  time_t raw = time;
  struct tm* gtime = gmtime(&raw);
  strftime(result, 30, "%m/%d/%g %H:%M:%S", gtime);
}

void get_inode_summary()
{
    __u32 inode_table = gp->bg_inode_table;
    int table_offset  =  inode_table*block_size;
    int i;
    int temp = (int)sp->s_inodes_per_group;
    int sp_inode_size = (int)sp->s_inode_size;
    for(i=0;i<block_size;i++)
    {
        char c;
        int check = pread(fd_image, &c, 1, group_inode_bitmap*block_size+i);
        if(check == -1){
            fprintf(stderr, "Fail to read: %s \n", strerror(errno));
            exit(2);
        }
        int j;
        for(j = 0; j < 8; j++) 
        {
            if((i*8+j+1) >temp )
                break;
            if(c&(1<<j))
            {
                struct ext2_inode curr_inode;
                int inode_num = (i*8)+j+1;
                if(pread(fd_image,&curr_inode,sp_inode_size,table_offset+sp_inode_size*(inode_num-1))==-1)
                {
                    fprintf(stderr, "Fail to read: %s \n", strerror(errno));
                    exit(2);
                }
                if(curr_inode.i_links_count==0)
                    continue;
                char file_type;
                __u16 inode_mode = curr_inode.i_mode&0x0FFF;
                if (S_ISREG(curr_inode.i_mode))
                    file_type = 'f';
                else if(S_ISDIR(curr_inode.i_mode))
                    file_type = 'd';
                else if(S_ISLNK(curr_inode.i_mode))
                    file_type = 's';
                else
                    file_type = '?';
                __u32 inode_owner = curr_inode.i_uid;
                __u32 inode_group = curr_inode.i_gid;
                __u16 inode_linkcount  = curr_inode.i_links_count;
                char ctime[30], atime[30], mtime[30];
                timefun(curr_inode.i_ctime, ctime);
                timefun(curr_inode.i_atime, atime);
                timefun(curr_inode.i_mtime, mtime);
                __u32 inode_size  =curr_inode.i_size;
                __u32 inode_blocks  = curr_inode.i_blocks;
                fprintf(stdout,"INODE,%u,%c,%o,%u,%u,%u,%s,%s,%s,%u,%u,",inode_num,file_type,inode_mode,inode_owner,inode_group,inode_linkcount,ctime,mtime,atime,inode_size,inode_blocks);
                int k;
                for(k=0;k<14;k++)
                {
                    fprintf(stdout,"%u,",curr_inode.i_block[k]);
                }
                fprintf(stdout,"%u\n",curr_inode.i_block[k]);
                inode_block = curr_inode.i_block;
                if(file_type=='d')
                {
                    get_directory_entries(inode_num);
                }
                if(file_type=='d'||file_type=='f')
                {
                    //get_indirect_block_references(inode_num);
                    
                    get_indirect_block_references(inode_num,1,inode_block[12],12);
                    get_indirect_block_references(inode_num,2,inode_block[13],268);
                    get_indirect_block_references(inode_num,3,inode_block[14],65804);
                }
            }   
        }
    }
}


int main(int argc, char **argv)
{
    if(argc != 2)
    {
        fprintf(stderr,"Wrong number of arguments. \n");
        exit(1);
    }
    else
        filename = argv[1];

    fd_image = open(filename, O_RDONLY);
    if(fd_image < 0)
    {
        fprintf(stderr,"Open image error. \n");
        exit(1);
    }
    
    /*
    stdout = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(stdout == -1){
        fprintf(stderr, "Fail to create output file: %s \n", strerror(errno));
        exit(2);
    }    
*/
    buf = (char*)malloc(1024);

    get_superblock_summary();
    get_group_summary();
    get_free_block_entries();
    get_free_inode_entries();
    get_inode_summary();





}


