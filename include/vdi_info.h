#ifndef VDIINFO
#define VDIINFO
#include "vdi_read.h"

void display_vdihead(VDIFile *f);
void display_vdimap(unsigned int vdimap[]);
void display_MBR(BootSector boot);
void display_superblock(ext2_super_block super);
void display_group_descriptor(ext2_group_descriptor group[], unsigned int count);
void display_inode(ext2_inode inode);

#endif
