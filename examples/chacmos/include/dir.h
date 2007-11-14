#ifndef __dir_h
#define __dir_h

#include "l4/l4.h"
#define EXT2_NAME_LENGTH 255

typedef struct {
  dword_t inode; 			// Inode Number
  word_t rec_length; 			// Directory entry length
  word_t  name_length; 			// Name length
  char name[EXT2_NAME_LENGTH]; 		// File name
} ext2_dir_entry;

#endif
