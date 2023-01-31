#ifndef SUPERBLOCK
#define SUPERBLOCK

struct __attribute__ ((packed)) ext2_super_block{
  unsigned int s_inodes_count;
  unsigned int s_blocks_count;
  unsigned int s_r_blocks_count;
  unsigned int s_free_blocks_count;
  unsigned int s_free_inodes_count;
  unsigned int s_first_data_block;
  unsigned int s_log_block_size;
  unsigned int s_log_frag_size;
  unsigned int s_blocks_per_group;
  unsigned int s_frags_per_group;
  unsigned int s_inodes_per_group;
  unsigned int s_mtime;
  unsigned int s_wtime;
  unsigned short s_mnt_count;
  unsigned short s_max_mnt_count;
  unsigned short s_magic;
  unsigned short s_state;
  unsigned short s_errors;
  unsigned short s_minor_rev_level;
  unsigned int s_lastcheck;
  unsigned int s_checkinterval;
  unsigned int s_creator_os;
  unsigned int s_rev_level;
  unsigned short s_def_resuid;
  unsigned short s_def_resgid;
  
  unsigned int s_first_ino;
  unsigned short s_inode_size;
  unsigned short s_block_group_nr;
  unsigned int s_feature_compat;
  unsigned int s_feature_incompat;
  unsigned int s_feature_ro_compat;
  unsigned char s_uuid[16];
  char s_volume_name[16];
  char s_last_mounted[64];
  unsigned int s_algorithm_usage_bitmap;

  unsigned char s_prealloc_blocks;
  unsigned char s_prealloc_dir_blocks;
  unsigned short s_padding_1;

  unsigned char s_journal_uuid[16];
  unsigned int s_journal_inum;
  unsigned int s_journal_dev;
  unsigned int s_last_orphan;
  unsigned int s_hash_seed[4];
  unsigned char s_def_hash_version;
  unsigned char s_reserved_char_pad;
  unsigned short s_reserved_word_pad;
  unsigned int s_default_mount_opts;
  unsigned int s_first_meta_bg;
  unsigned int s_reserved[190];

	};

#endif
