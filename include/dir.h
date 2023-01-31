#ifndef DIR
#define DIR

/**
* Lists the ext2_dir_entry_2 structure.
* Source: http://cs.smith.edu/~nhowe/262/oldlabs/ext2.html#direntry
*/
struct __attribute__ ((packed)) ext2_dir_entry_2{
  unsigned int inode; //Inode number
  unsigned short rec_len; //Directory entry length
  unsigned char name_len; //Name length
  unsigned char file_type; //Type of file [1 - Regular Name, 2 - Directory etc.]
  unsigned char name[225]; //File name

	};
#endif
