#include <stdlib.h>

#include "interfaces/filesvr_server.h"
#include "interfaces/block_client.h"
#include "interfaces/generic_client.h"
#include "chacmos.h"
#include "ext2.h"

#define MAXBLOCKDESC 128

sdword api_block[] = { GENERIC_API, BLOCK_API };

handle_t blockdev;
superblock_t superblock;
blockdesc_t blockdesc[MAXBLOCKDESC];
int bdev_blocksize, bdev_capacity;
int fs_blocksize, fs_bgrpcnt, fs_scale;
char sectorbuffer[32768];

void memcpy(char *dest, char *src, int size)

{
  while (size--)
    *dest++ = *src++;
}

void get_inode(int objID, inode_t *inode)

{
  if (objID==0) objID=2;  // evil magic
  int which_bgrp = (objID-1) / superblock.s_inodes_per_group;
  int inode_offset = (objID-1) % superblock.s_inodes_per_group;
  int inode_table_at = (blockdesc[which_bgrp].bg_inode_table*fs_scale);
  int inode_in_block = inode_offset/(bdev_blocksize/sizeof(inode_t));
  int inode_blockofs = inode_offset%(bdev_blocksize/sizeof(inode_t));
  int size;
  char *ptr;
  inode_t *itemp_p;
  CORBA_Environment env = idl4_default_environment;

  #ifdef DEBUG
  printf("getting inode %d to %X\n",objID,(int)inode);
  printf("bgrp %d, ofs %d, table at %d, in block %d, blockofs %d\n",which_bgrp,
     inode_offset,inode_table_at,inode_in_block,inode_blockofs);
  #endif
  
  if (block_read(blockdev.svr,blockdev.obj,inode_table_at+inode_in_block,1,&size,&ptr,&env)!=ESUCCESS)
    enter_kdebug("cannot read inode");

  memcpy(sectorbuffer, ptr, bdev_blocksize);
  CORBA_free(ptr);
  
  itemp_p = (inode_t*)&sectorbuffer;
  *inode=itemp_p[inode_blockofs];
}

int get_superblock(handle_t blockdevice)

{
  CORBA_Environment env = idl4_default_environment;
  char *ptr;
  superblock_t *p_sb;
  int size;

  blockdev=blockdevice;

  if (generic_implements(blockdev.svr,blockdev.obj,sizeof(api_block)/sizeof(int),(sdword*)&api_block,&env)!=EYES)
    enter_kdebug("interface not supported");
    
  if (block_get_capacity(blockdev.svr,blockdev.obj,&bdev_capacity,&env)!=ESUCCESS)
    enter_kdebug("cannot get capacity");
    
  if (block_get_blocksize(blockdev.svr,blockdev.obj,&bdev_blocksize,&env)!=ESUCCESS)
    enter_kdebug("cannot get block size");

  if (bdev_blocksize>4096) 
    enter_kdebug("blocksize >4k");

  size = (bdev_blocksize>2048) ? bdev_blocksize : 2048;
  if (block_read(blockdev.svr,blockdev.obj,0,size/bdev_blocksize,&size,&ptr,&env)!=ESUCCESS)
    enter_kdebug("cannot read superblock");
    
  memcpy(sectorbuffer, ptr, size);
  CORBA_free(ptr);

  p_sb=(superblock_t*)&sectorbuffer[1024];
  superblock=*p_sb;
  fs_blocksize=1024*(1<<superblock.s_log_block_size);
  
  if (fs_blocksize<bdev_blocksize) 
    enter_kdebug("won't work, blocks are too small");

  fs_scale=fs_blocksize/bdev_blocksize;  
  fs_bgrpcnt=(superblock.s_blocks_count/superblock.s_blocks_per_group);
  if ((fs_bgrpcnt*superblock.s_blocks_per_group)<superblock.s_blocks_count)
    fs_bgrpcnt++;
    
  if (fs_bgrpcnt>MAXBLOCKDESC)
    enter_kdebug("too many blockgroups");
  
  size = MAXBLOCKDESC*sizeof(blockdesc_t);
  if (block_read(blockdev.svr,blockdev.obj,(1+superblock.s_first_data_block)*fs_scale,size/bdev_blocksize,&size,&ptr,&env)!=ESUCCESS)
    enter_kdebug("cannot read block descriptors");
    
  memcpy((char*)blockdesc, ptr, MAXBLOCKDESC*sizeof(blockdesc_t));
  CORBA_free(ptr);

  return ESUCCESS;
}

void read_block(inode_t *inode,int blocknr,char *buffer)

{
  CORBA_Environment env = idl4_default_environment;
  int startblock;
  int size;
  char *ptr;
  
  // 1 indirect block should be enough
  
  if (blocknr<12) 
    {
      startblock=inode->i_block[blocknr]*fs_scale;
    } else {
             int *idxbuffer;
             if (block_read(blockdev.svr,blockdev.obj,inode->i_indir1*fs_scale,fs_scale,&size,(char**)&idxbuffer,&env)!=ESUCCESS)
               enter_kdebug("failed to read indir1");
             startblock=idxbuffer[blocknr-12]*fs_scale;
             CORBA_free(idxbuffer);
           }
	   
  if (block_read(blockdev.svr,blockdev.obj,startblock,fs_scale,&size,&ptr,&env)!=ESUCCESS)
    enter_kdebug("failed to read data");
  memcpy(buffer, ptr, size);
  CORBA_free(ptr);
}

void read(int objID, int pos, int length, char **buffer)

{
  inode_t inode;
  int start_block = pos/fs_blocksize;
  int blockcnt = 1+((pos+length-1)/fs_blocksize)-start_block;
  int offset = pos%fs_blocksize;
  
  get_inode(objID,&inode);
  
  for (int i=0;i<blockcnt;i++)
    read_block(&inode,start_block+i,&sectorbuffer[fs_blocksize*i]);
  
  *buffer=&sectorbuffer[offset];  
}

int get_length(int objID)

{
  inode_t inode;
  get_inode(objID,&inode);
  return inode.i_size;
}


int get_mode(int objID)

{ 
  inode_t inode;
  get_inode(objID,&inode);
  return inode.i_mode;
}

void put_superblock(void)

{
}
