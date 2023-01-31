#ifndef GROUPDESCRIPTOR
#define GROUPDESCRIPTOR

/**
* lists the ext2_group_desc structure for each block group on the disk.
* Source: http://cs.smith.edu/~nhowe/262/oldlabs/ext2.html#group
*/
struct __attribute__ ((packed)) ext2_group_descriptor{
  unsigned int block_bitmap;
  unsigned int inode_bitmap;
  unsigned int inode_table;
  unsigned short free_blocks_count;
  unsigned short free_inodes_count;
  unsigned short used_dirs_count;
  unsigned short pad;
  unsigned int reserved[3];

	};

#endif
