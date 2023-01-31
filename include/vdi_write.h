#ifndef VDIWRITE
#define VDIWRITE

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <cstdint>
#include <string>
#include "superblock.h"
#include "inode.h"
#include <unistd.h>
#include <fcntl.h>
#include "group_descriptor.h"
#include "mbr.h"
#include "dir.h"
#include "vdifile.h"
#include "vdi_read.h"
#include <vector>

int writeSuperblock(VDIFile* f, BootSector boot, unsigned int vdimap[], ext2_super_block& super_block);
int writeGroupDescriptor(VDIFile* f, BootSector boot, unsigned int vdimap[], unsigned int block_size,
                         ext2_group_descriptor group_desc[], unsigned int numberBlockGroups);
int writeBitmap(VDIFile* f, BootSector boot, unsigned int vdimap[], unsigned char* bitmap, unsigned int block_size,
                unsigned int blockID);
int writeBlock(VDIFile* f, BootSector boot, unsigned int vdimap[], ext2_inode inode,
               std::vector<unsigned int>& addresses, unsigned int block_num, unsigned int block_size,
               unsigned char* data);
int writeInode(VDIFile* f, BootSector boot, unsigned int vdimap[], ext2_inode inode, unsigned int inode_num,
               unsigned int block_size, ext2_super_block super_block, ext2_group_descriptor group_desc[]);
bool isBlockFree(unsigned char* bitmap, unsigned int block_size, unsigned int block_offset, unsigned int& address);
bool isInodeFree(unsigned char* bitmap, unsigned int block_size, unsigned int inode_block_offset,
                 unsigned int& address);

#endif
