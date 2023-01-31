#include "vdi_write.h"
using namespace std;

/**
Sources used: http://cs.smith.edu/~nhowe/262/oldlabs/ext2.html#locate_file
http://www.nongnu.org/ext2-doc/ext2.html#DEF-INODES
https://courses.cs.washington.edu/courses/cse451/09sp/projects/project3light/project3_light.html
https://www.tldp.org/LDP/tlk/fs/filesystem.html
*/

bool isInodeFree(unsigned char* bitmap, unsigned int blockSize, unsigned int inodeBlockOffset, unsigned int& address){
	for (int i = 0; i < blockSize; i++)
	{
		unsigned char currentByte = *(bitmap + i);
		for (int j = 0; j < blockSize; j++)
		{
			if (!((currentByte >> j) & 0x1))
			{
				address = (inodeBlockOffset + j + i * 8) + 1;
				*(bitmap + i) = ((1 << j) | currentByte);
				return true;
			}
		}
	}
	return false;
}

int writeSuperblock(VDIFile* f, BootSector boot, unsigned int vdiMap[], ext2_super_block& superBlock)
{
	unsigned int superblockLocation = computeLocation(1024, f, boot, vdiMap);
	if (lseek(f->file, superblockLocation, SEEK_SET) < 0)
	{
		cout << "Failed to seek to the superblock" << endl;
		return 1;
	}
	if (write(f->file, &superBlock, sizeof(superBlock)) != 1024)
	{
		cout << "Superblock was not written correctly!" << endl;
		return 1;
	}
	return 0;
}

bool isBlockFree(unsigned char* bitmap, unsigned int blockSize, unsigned int blockOffset, unsigned int& address)
{
	for (int i = 0; i < blockSize; i++)
	{
		unsigned char current_byte = *(bitmap + i);
		for (int j = 0; j < 8; j++)
		{
			if (!((current_byte >> j) & 0x1))
			{
				address = blockOffset + i * 8 + j;
				if (blockSize == 1024) address++;
				*(bitmap + i) = ((1 << j) | current_byte);
				return true;
			}
		}
	}
	return false;
}

int writeGroupDescriptor(VDIFile* f, BootSector boot, unsigned int vdiMap[], unsigned int blockSize,
                         ext2_group_descriptor groupDescriptor[], unsigned int numberBlockGroups)
{
	unsigned int groupStart = 0;
	if (blockSize == 1024) groupStart = blockSize * 2;
	else groupStart = blockSize;

	unsigned int groupLoc = computeLocation(groupStart, f, boot, vdiMap);
	if (lseek(f->file, groupLoc, SEEK_SET) < 0)
	{
		cout << "Failed to seek to the group desc" << endl;
		return 1;
	}
	size_t size = sizeof(ext2_group_descriptor) * numberBlockGroups;
	if (write(f->file, groupDescriptor, size) != size)
	{
		cout << "Failed to write group desc" << endl;
		return 1;
	}
	return 0;
}

int writeBitmap(VDIFile* f, BootSector boot, unsigned int vdiMap[], unsigned char* bitmap, unsigned int blockSize,
                unsigned int blockId)
{
	unsigned int loc = computeLocation(blockId * blockSize, f, boot, vdiMap);
	if (lseek(f->file, loc , SEEK_SET) < 0)
	{
		cout << "Failed to seek to bitmap" << endl;
		return 1;
	}
	if (write(f->file, bitmap, blockSize) != blockSize)
	{
		cout << "Failed to write bitmap" << endl;
		return 1;
	}
	return 0;
}

int writeBlock(VDIFile* f, BootSector boot, unsigned int vdiMap[], ext2_inode inode,
               std::vector<unsigned int>& address_list, unsigned int blockNum, unsigned int blockSize, unsigned char* data)
{
	if (blockNum * blockSize >= inode.size) return -1;
	unsigned char* buffer = (unsigned char*)malloc(blockSize);
	int direct;
	int single_index;
	int double_index;
	int triple_index;

	computeIndex(blockNum, blockSize, direct, single_index, double_index, triple_index);
	unsigned int tripleBlockNum = inode.i_block[14];
	unsigned int doubleBlockNum = 0;
	if (triple_index != -1)
	{
		if (double_index != -1 && single_index != -1 && direct == -1)
		{
			if (lseek(f->file, computeLocation(tripleBlockNum * blockSize, f, boot, vdiMap), SEEK_SET) < 0) return -1;
			if (read(f->file, buffer, blockSize) < 0) return -1;
			if (triple_index >= (blockSize / 4)) return -1;
			doubleBlockNum = *(((unsigned int *)buffer) + triple_index);
		}
		else return -1;
	}
	unsigned int singleBlockNum = 0;
	if (double_index != -1)
	{
		if (single_index != -1 && direct == -1)
		{
			if (triple_index == -1)
			{
				doubleBlockNum = inode.i_block[13];
			}
			if (doubleBlockNum == 0)
			{
				doubleBlockNum = address_list.back();
				address_list.pop_back();
				*(((unsigned int *)buffer) + triple_index) = doubleBlockNum;
				if (lseek(f->file, computeLocation(tripleBlockNum * blockSize, f, boot, vdiMap), SEEK_SET) < 0) return -1;
				if (write(f->file, buffer, blockSize) != blockSize) return -1;
			}
			if (lseek(f->file, computeLocation(doubleBlockNum * blockSize, f, boot, vdiMap), SEEK_SET) < 0) return -1;
			if (read(f->file, buffer, blockSize) < 0) return -1;
			if (double_index >= (blockSize / 4)) return -1;
			singleBlockNum = *(((unsigned int *)buffer) + double_index);
		}
		else return -1;
	}

	unsigned int directBlockNum = 0;
	if (single_index != -1)
	{
		if (direct == -1)
		{
			if (double_index == -1) singleBlockNum = inode.i_block[12];
			if (singleBlockNum == 0)
			{
				singleBlockNum = address_list.back();
				address_list.pop_back();
				*(((unsigned int *)buffer) + double_index) = singleBlockNum;
				if (lseek(f->file, computeLocation(doubleBlockNum * blockSize, f, boot, vdiMap), SEEK_SET) < 0) return -1;
				if (write(f->file, buffer, blockSize) != blockSize) return -1;
			}
			if (lseek(f->file, computeLocation(singleBlockNum * blockSize, f, boot, vdiMap), SEEK_SET) < 0) return -1;
			if (read(f->file, buffer, blockSize) < 0) return -1;
			if (single_index >= (blockSize / 4)) return -1;
			directBlockNum = *(((unsigned int *)buffer) + single_index);
		}
		else return -1;
	}

	if (direct != -1)
	{
		if (direct < 12) directBlockNum = inode.i_block[direct];
		else return -1;
	}

	if (directBlockNum == 0)
	{
		directBlockNum = address_list.back();
		address_list.pop_back();
		*(((unsigned int *)buffer) + single_index) = directBlockNum;
		if (lseek(f->file, computeLocation(singleBlockNum * blockSize, f, boot, vdiMap), SEEK_SET) < 0) return -1;
		if (write(f->file, buffer, blockSize) != blockSize) return -1;
	}

	if (lseek(f->file, computeLocation(directBlockNum * blockSize, f, boot, vdiMap), SEEK_SET) < 0) return -1;
	if (write(f->file, data, blockSize) != blockSize) return -1;
	unsigned int diff = inode.size - (blockNum * blockSize);
	if (diff >= blockSize) return blockSize;
	return diff;
}

int writeInode(VDIFile* f, BootSector boot, unsigned int vdiMap[], ext2_inode inode, unsigned int inodeNum,
                unsigned int blockSize, ext2_super_block superBlock, ext2_group_descriptor groupDescriptor[])
{
	inodeNum--;
	unsigned int groupNum = inodeNum / superBlock.s_inodes_per_group;
	unsigned int offset1 = inodeNum % superBlock.s_inodes_per_group;
	unsigned int inodesPerBlock = blockSize / sizeof(ext2_inode);
	unsigned int blockNum = groupDescriptor[groupNum].inode_table + (offset1 / inodesPerBlock);
	unsigned int offset2 = inodeNum % inodesPerBlock;
	unsigned int loc = computeLocation(blockNum * blockSize + offset2 * sizeof(ext2_inode), f, boot, vdiMap);
	if (lseek(f->file, loc, SEEK_SET) < 0) return 1;
	if (write(f->file, &inode, sizeof(ext2_inode)) != sizeof(ext2_inode)) return 1;

	return 0;
}
