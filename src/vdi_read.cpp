#include "vdi_read.h"

using namespace std;

/**
 * Mimcs the UNIX open system call
 * Opens the vdifile
 */
int vdiOpen(VDIFile* vdi, char* fn)
{
	vdi->file = open(fn, O_RDWR);
	vdi->cursor = 0;
	if (vdi->file < 0) return 1;
	return vdi->file;
}

/**
 * Mimics the UNIX close system call
 * Closes the vdifile
 */
void vdiClose(VDIFile* f)
{
	close(f->file);
}

/**
 * Mimics the UNIX lseek system call
 * Seeks to the given location inside the vdifile
 */
off_t vdiSeek(VDIFile* f, off_t offset, int whence)
{
	off_t location;
	if (whence == SEEK_SET)
	{
		location = lseek(f->file, offset, whence);
		if (location < 0)
		{
			cout << "Error seeking the vdi header" << endl;
			return 1;
		}
		f->cursor = location;
	}
	if (whence == SEEK_CUR)
	{
		location = lseek(f->file, offset, whence);
		if (location < 0)
		{
			cout << "Error seeking the vdi header" << endl;
			return 1;
		}
		f->cursor += offset;
	}
	if (whence == SEEK_END)
	{
		location = lseek(f->file, offset, whence);
		if (location < 0)
		{
			cout << "Error seeking the vdi header" << endl;
			return 1;
		}
		f->cursor = offset + f->fileSize;
	}
	return f->cursor;
}

/**
 * Mimics the UNIX read system call.
 * Reads a vdifile after the cursor has been placed.
 */
ssize_t vdiRead(VDIFile* f, void* buf, ssize_t n)
{
	ssize_t numBytes = read(f->file, buf, n);
	if (numBytes != n)
	{
		cout << "Header not read correctly" << endl;
		return 1;
	}
	return 0;
}

/**
 * Reads the vdiMap.
 * Performs lseek and read in the same function.
 */
int readVdiMap(VDIFile* f, unsigned int vdiMap[])
{
	off_t offset = lseek(f->file, f->header.offsetBlocks, SEEK_SET);
	if (offset < 0)
	{
		cout << "Failed to seek vdiMap" << endl;
		return 1;
	}
	int numMap = read(f->file, vdiMap, 4 * (f->header.blocksInHdd));
	if (numMap < 0)
	{
		cout << "Failed to read vdiMap" << endl;
		return 1;
	}
	return 0;
}


/**
 * Reads the MBR.
 */
int readMbr(VDIFile* f, BootSector& boot)
{
	off_t offset = lseek(f->file, f->header.offsetData, SEEK_SET);
	if (offset < 0)
	{
		cout << "Failed to seek to MBR" << endl;
		return 1;
	}
	int numMbr = read(f->file, &boot, sizeof(boot));
	if (numMbr != sizeof(boot))
	{
		cout << "Failed to read MBR" << endl;
		return 1;
	}
	return 0;
}

/**
 * Reads the superblock
 */
int readSuperblock(VDIFile* f, BootSector& boot, unsigned int vdiMap[], ext2_super_block& super)
{
	unsigned int superblockLocation = computeLocation(1024, f, boot, vdiMap);
	if (lseek(f->file, superblockLocation, SEEK_SET) < 0)
	{
		cout << "Failed to seek to the superblock!" << endl;
		return 1;
	}
	if (read(f->file, &super, sizeof(super)) != 1024)
	{
		cout << "Failed to read superblock correctly!" << endl;
		return 1;
	}
	return 0;
}

/**
 * Helper function to computeLocation the super block location.
 * This function was basically separated out for dynamic files.
 */
unsigned int computeLocation(unsigned int location, VDIFile* f, BootSector bootSector, unsigned int vdiMap[])
{
	unsigned int address1 = f->header.offsetData;
	unsigned int address2 = bootSector.partitionTable[0].sector_1 * 512 + location;
	unsigned int offset = address2 % f->header.blockSize;
	unsigned int block = address2 / f->header.blockSize;
	unsigned int translated_address = vdiMap[block];
	unsigned int final_address = address1 + translated_address * f->header.blockSize + offset; //Combines the address [Especially useful with dynamic files]
	return final_address;
}

/**
 * Reads the group descriptor.
 */
int readGroupDescriptor(VDIFile* f, BootSector bootSector, unsigned int vdiMap[], unsigned int blockSize,
                        ext2_group_descriptor groupDescriptor[], unsigned int blockGroupCount)
{
	unsigned int start = 0;
	if (blockSize == 1024) start = 2 * blockSize;
	else start = 1 * blockSize;
	//Typically, location would just be 1024 + blockSize, but this takes care of dynamic file as well.
	unsigned int location = computeLocation(start, f, bootSector, vdiMap);
	if (lseek(f->file, location, SEEK_SET) < 0)
	{
		cout << "Cannot seek to group descriptor table!" << endl;
		return 1;
	}
	if (read(f->file, groupDescriptor, sizeof(ext2_group_descriptor) * blockGroupCount) != sizeof(ext2_group_descriptor) *
		blockGroupCount)
	{
		cout << "Cannot read group descriptor!" << endl;
		return 1;
	}
	return 0;
}

/**
 * Reads the bitmap.
 * Again, this matters for dynamic files only.
 */
unsigned char* readBitmap(unsigned int blockSize, unsigned int blockId, VDIFile* f, BootSector bootSector,
                          unsigned int vdiMap[])
{
	unsigned char* bitmap;
	bitmap = (unsigned char *)malloc(blockSize);
	unsigned int loc = computeLocation(blockId * blockSize, f, bootSector, vdiMap);
	lseek(f->file,loc, SEEK_SET);
	read(f->file, bitmap, blockSize);
	return bitmap;
}

/**
 * Reads the inode.
 */
ext2_inode readInode(VDIFile* f, BootSector bootSector, unsigned int vdiMap[], unsigned int inodeCount,
                     unsigned int blockSize, ext2_super_block superBlock, ext2_group_descriptor groupDescriptor[])
{
	ext2_inode inode;
	inodeCount--;
	unsigned int groupCount = inodeCount / superBlock.s_inodes_per_group;
	unsigned int offset1 = inodeCount % superBlock.s_inodes_per_group;
	unsigned int inodesPerBlock = blockSize / sizeof(ext2_inode);
	unsigned int blockNum = groupDescriptor[groupCount].inode_table + (offset1 / inodesPerBlock);
	unsigned int offset2 = inodeCount % inodesPerBlock;
	unsigned int val = (blockNum * blockSize) + (offset2 * sizeof(ext2_inode));
	unsigned int loc = computeLocation(val, f, bootSector, vdiMap);
	lseek(f->file, loc , SEEK_SET);
	read(f->file, &inode, sizeof(ext2_inode));
	return inode;
}

/**
 * Fetches the directory entry.
 * The references for the bottom two functions is given below.
 * Source: http://cs.smith.edu/~nhowe/262/oldlabs/ext2.html#locate_file
 * Source: https://courses.cs.washington.edu/courses/cse451/09sp/projects/project3light/project3_light.html
 * Source: https://www.tldp.org/LDP/tlk/fs/filesystem.html
 */
bool getDirEntry(ext2_dir_entry_2& found, unsigned char* dataBlock, unsigned int sizeDiff, string fname, bool display_files)
{
	ext2_dir_entry_2 * entry = (ext2_dir_entry_2 *)malloc(sizeof(ext2_dir_entry_2));
	memcpy(entry, dataBlock, sizeof(*entry));
	unsigned int size = 0;
	while (size < sizeDiff)
	{
		char f_name[256];
		memcpy(f_name, entry->name, entry->name_len);
		f_name[entry->name_len] = '\0'; //Null character appended to the filename.
		if (entry->inode != 0)
		{
			if (display_files) cout << f_name << endl;
			else if ((string)f_name == fname)
			{
				found = *entry;
				free(entry);
				return true;
			}
		}
		else
		{
			size += sizeof(*entry);
			memcpy(entry, dataBlock + size, sizeof(*entry));
			continue;
		}
		size += entry->rec_len;
		memcpy(entry, dataBlock + size, sizeof(*entry));
	}
	free(entry);
	return false;
}

/**
 * Reads a block given its block number, block size, inode.
 * For this function, I mostly translated a C code from an ext2 reference website. I can't remember the source, however.
 */
int readBlock(ext2_inode inode, unsigned int blockNum, unsigned int blockSize, VDIFile* f, BootSector bootSector,
              unsigned int vdiMap[], unsigned char* buf)
{
	if (blockNum * blockSize >= inode.size) return -1;
	int direct, single_index, double_index, triple_index;
	computeIndex(blockNum, blockSize, direct, single_index, double_index, triple_index);
	unsigned int direct_block_num = 0;
	unsigned int single_block_num = 0;
	unsigned int double_block_num = 0;
	unsigned int triple_block_num = inode.i_block[14];
	bool hole = false; //Tracks any holes
	if (triple_index != -1)
	{
		if (triple_block_num == 0) hole = true;
		else
		{
			if (single_index != -1 && double_index != -1 && direct == -1)
			{
				if (lseek(f->file, computeLocation(triple_block_num * blockSize, f, bootSector, vdiMap), SEEK_SET) < 0) return -1;
				if (read(f->file, buf, blockSize) < 0) return -1;
				if (triple_index >= (blockSize / 4)) return -1;
				double_block_num = *(((unsigned int *)buf) + triple_index);
			}
			else return -1;
		}
	}
	if (!hole && double_index != -1)
	{
		if (single_index != -1 && direct == -1)
		{
			if (triple_index == -1) double_block_num = inode.i_block[13];

			if (double_block_num == 0) hole = true;

			else
			{
				if (lseek(f->file, computeLocation(double_block_num * blockSize, f, bootSector, vdiMap), SEEK_SET) < 0) return -1;
				if (read(f->file, buf, blockSize) < 0) return -1;
				if (double_index >= (blockSize / 4)) return -1;
				single_block_num = *(((unsigned int *)buf) + double_index);
			}
		}
		else return -1;
	}
	if (!hole && single_index != -1)
	{
		if (direct == -1)
		{
			if (double_index == -1) single_block_num = inode.i_block[12];
			if (single_block_num == 0) hole = true;
			else
			{
				if (lseek(f->file, computeLocation(single_block_num * blockSize, f, bootSector, vdiMap), SEEK_SET) < 0) return -1;
				if (read(f->file, buf, blockSize) < 0) return -1;
				if (single_index >= (blockSize / 4))return -1;
				direct_block_num = *(((unsigned int *)buf) + single_index);
			}
		}
		else return -1;
	}
	if (!hole && direct != -1)
	{
		if (direct < 12) direct_block_num = inode.i_block[direct];
		else return -1;
	}
	if (hole) memset(buf, 0, blockSize);
	else
	{
		if (lseek(f->file, computeLocation(direct_block_num * blockSize, f, bootSector, vdiMap), SEEK_SET) < 0) return -1;
		if (read(f->file, buf, blockSize) < 0) return -1;
	}
	unsigned int difference = inode.size - (blockNum * blockSize);
	if (difference >= blockSize) return blockSize;
	return difference;
}

/**
 * Helper function to compute the indices.
 */
void computeIndex(unsigned int blockNum, unsigned int blockSize, int& directNumber, int& indirectIndex, int& double_index,
                  int& triple_index)
{
	if (blockNum <= 11)
	{
		directNumber = blockNum;
		indirectIndex = -1;
		double_index = -1;
		triple_index = -1;
		return;
	}
	unsigned int blocks_remaining = blockNum - 12;
	if (blocks_remaining < blockSize / 4)
	{
		directNumber = -1;
		double_index = -1;
		triple_index = -1;
		indirectIndex = blocks_remaining;
		return;
	}
	blocks_remaining -= blockSize / 4;
	if (blocks_remaining < (blockSize / 4) * (blockSize / 4))
	{
		directNumber = -1;
		triple_index = -1;
		double_index = blocks_remaining / (blockSize / 4);
		indirectIndex = blocks_remaining - (double_index) * (blockSize / 4);
		return;
	}
	blocks_remaining -= (blockSize / 4) * (blockSize / 4);
	if (blocks_remaining < (blockSize / 4) * (blockSize / 4) * (blockSize / 4))
	{
		directNumber = -1;
		triple_index = blocks_remaining / ((blockSize / 4) * (blockSize / 4));
		double_index = (blocks_remaining - (triple_index) * (blockSize / 4) * (blockSize / 4)) / (blockSize / 4);
		indirectIndex = (blocks_remaining - (triple_index) * (blockSize / 4) * (blockSize / 4)) - (double_index) * (blockSize / 4);
		return;
	}
}
