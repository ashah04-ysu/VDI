#ifndef INODE
#define INODE

/**
* Lists the ext2_inode structure.
* Source: http://www.nongnu.org/ext2-doc/ext2.html#DEF-INODES
*/
struct __attribute__ ((packed)) ext2_inode{
  unsigned short mode;
  unsigned short uid;
  unsigned int size;
  unsigned int atime;
  unsigned int ctime;
  unsigned int mtime;
  unsigned int dtime;
  unsigned short gid;
  unsigned short links_count;
  unsigned int blocks;
  unsigned int flags;
  union __attribute__((packed)){
    struct __attribute__((packed)){
      unsigned int l_i_reserved1;
			} linux1;
    struct __attribute__((packed)){
      unsigned int h_i_translator;
			} hurd1;
    struct __attribute__((packed)){
      unsigned int m_i_reserved1;
			} masix1;
		} osd1;
  unsigned int i_block[15];
  unsigned int i_generation;
  unsigned int i_file_acl;
  unsigned int i_dir_acl;
  unsigned int i_faddr;
  union __attribute__((packed)){
    struct __attribute__((packed)){
      unsigned char l_i_frag;
      unsigned char l_i_fsize;
      unsigned short i_pad1;
      unsigned short l_i_uid_high;
      unsigned short l_i_gid_high;
      unsigned int l_i_reserved2;
			} linux2;
    struct __attribute__((packed)){
      unsigned char h_i_frag;
      unsigned char h_i_fsize;
      unsigned short h_i_mode_high;
      unsigned short h_i_uid_high;
      unsigned short h_i_gid_high;
      unsigned int h_i_author;
			} hurd2;
    struct __attribute__((packed)){
      unsigned char m_i_frag;
      unsigned char m_i_fsize;
      unsigned short m_pad1;
      unsigned int m_i_reserved2[2];

			} masix2;
		} osd2;
	};
#endif
