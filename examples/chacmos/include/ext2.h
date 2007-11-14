#ifndef __ext2_h
#define __ext2_h

typedef struct {
  dword_t s_inodes_count;
  dword_t s_blocks_count;
  dword_t s_r_blocks_count;
  dword_t s_free_blocks_count;
  dword_t s_free_inodes_count;
  dword_t s_first_data_block;
  dword_t s_log_block_size;
  dword_t s_log_frag_size;
  dword_t s_blocks_per_group;
  dword_t s_frags_per_group;
  dword_t s_inodes_per_group;
  dword_t s_mtime;
  dword_t s_wtime;
  word_t s_mnt_count;
  word_t s_max_mnt_count;
  word_t s_magic;
  word_t s_state;
  word_t s_errors;
  word_t s_pad;
  dword_t s_lastcheck;
  dword_t s_checkinterval;
  dword_t s_creator_os;
  dword_t s_rev_level;
  word_t s_def_resuid;
  word_t s_def_resgid;
  dword_t s_reserved[235];
} superblock_t;

typedef struct {
  dword_t bg_block_bitmap;
  dword_t bg_inode_bitmap;
  dword_t bg_inode_table;
  word_t bg_free_blocks_count;
  word_t bg_free_inodes_count;
  word_t bg_used_dirs_count;
  word_t bg_pad;
  dword_t bg_reserved[3];
} blockdesc_t;

typedef struct {
  word_t i_mode;
  word_t i_uid;
  dword_t i_size;
  dword_t i_atime;
  dword_t i_ctime;
  dword_t i_mtime;
  dword_t i_dtime;
  word_t i_gid;
  word_t i_links_count;
  dword_t i_blocks;
  dword_t i_flags;
  dword_t anything1;
  dword_t i_block[12];
  dword_t i_indir1;
  dword_t i_indir2;
  dword_t i_indir3;
  dword_t i_version;
  dword_t i_file_acl;
  dword_t i_dir_acl;
  dword_t i_faddr;
  dword_t anything2[3];
} inode_t;  

#endif
