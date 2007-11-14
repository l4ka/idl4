#ifndef __filesvr_h
#define __filesvr_h

#define MAXFCB       10
#define FILEBUFFER   0x20000000
#define SECTORBUFFER 0x30000000
#define FILE 1
#define DIRECTORY 2

typedef struct {
  l4_threadid_t taskID;
  int objID;
  unsigned int position;
  int mode;
  int inode_type; 	// Type of the Inode (Directory or File)
  int active;
  unsigned int file_length;
  char dir_buffer [1024]; // Buffer for the directory
} fcb_t;

extern fcb_t fcbTable[MAXFCB];

void read(int objID, int pos, int length, char **buffer);
int get_length(int objID);
int get_superblock(handle_t blockdevice);
int get_mode(int objID);
void put_superblock(void);

#endif
