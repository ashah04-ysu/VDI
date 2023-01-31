#ifndef VDI_READ
#define VDI_READ
#include <cstdint>
#include <string>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "vdifile.h"
#include "mbr.h"
#include "superblock.h"
#include "group_descriptor.h"
#include "inode.h"
#include "dir.h"

using namespace std;

int vdiOpen(VDIFile* vdi, char* fn);
void vdiClose(VDIFile* f);
off_t vdiSeek(VDIFile* f, off_t offset, int whence);
ssize_t vdiRead(VDIFile* f, void* buf, ssize_t n);
ssize_t vdiWrite(VDIFile* f, void* buf, ssize_t n);
int readVdiMap(VDIFile* f, unsigned int vdimap[]);
int readMbr(VDIFile * f, BootSector & boot);
int readSuperblock(VDIFile* f, BootSector& boot, unsigned int vdimap[], ext2_super_block& super);
unsigned int computeLocation(unsigned int location, VDIFile* f, BootSector boot_sector, unsigned int vdimap[]);
int readGroupDescriptor(VDIFile* f, BootSector boot_sector, unsigned int vdimap[], unsigned int block_size,
                        ext2_group_descriptor group_descriptor[], unsigned int block_group_count);
unsigned char* readBitmap(unsigned int block_size, unsigned int block_id, VDIFile* f, BootSector boot_sector,
                          unsigned int vdimap[]);
ext2_inode readInode(VDIFile* f, BootSector boot_sector, unsigned int vdimap[], unsigned int inode_count,
                     unsigned int block_size, ext2_super_block super_block, ext2_group_descriptor group_descriptor[]);
bool getDirEntry(ext2_dir_entry_2& found, unsigned char* data_block, unsigned int size_diff, string fname,
                 bool display);
int readBlock(ext2_inode inode, unsigned int block_num, unsigned int block_size, VDIFile* f, BootSector boot_sector,
              unsigned int vdimap[], unsigned char* buf);
void computeIndex(unsigned int block_num, unsigned int block_size, int& direct_number, int& indirect_index,
                  int& index_2, int& index_3);

#endif
