#ifndef FILESYS_H
#define FILESYS_H

#include <time.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define MAXBLOCKS     1024
#define BLOCKSIZE     1024
#define FATENTRYCOUNT (BLOCKSIZE / sizeof(fatentry_t))
#define DIRENTRYCOUNT ((BLOCKSIZE - (2*sizeof(int)) ) / sizeof(direntry_t))
#define MAXNAME       256
#define MAXPATHLENGTH 1024
#define FOURKB	 	  4096

#define UNUSED        -1
#define ENDOFCHAIN     0
//#define EOF			  -1


typedef unsigned char Byte ;

typedef short fatentry_t ;

typedef fatentry_t fatblock_t [ FATENTRYCOUNT ] ;

typedef struct direntry {
   int         entrylength ;   // records length of this entry (can be used with names of variables length)
   Byte        isdir ;
   Byte        unused ;
   time_t      modtime ;
   int         filelength ;
   fatentry_t  firstblock ;
   char   name [MAXNAME] ;
} direntry_t ;

typedef struct dirblock {
   int isdir ;
   int nextEntry ;
   int parentBlock;
   int parentEntry;
   direntry_t entrylist [ DIRENTRYCOUNT ] ; // the first two integer are marker and endpos
} dirblock_t ;

typedef Byte datablock_t [ BLOCKSIZE ] ;

typedef union block {
   datablock_t data ;
   dirblock_t  dir  ;
   fatblock_t  fat  ;
} diskblock_t ;

extern diskblock_t virtualDisk [ MAXBLOCKS ] ;

typedef struct filedescriptor {
   char        	mode[3] 	;
   fatentry_t  	blockno 	;           // block no
   int         	pos     	;           // byte within a block
   diskblock_t 	buffer  	;
} MyFILE ;

// pre-defined
void readdisk ( const char * filename );
void writedisk ( const char * filename ) ;

// a
void format() ;

// c
MyFILE * myfopen(const char * filename, const char * mode);
void myfclose(MyFILE * stream);
int myfgetc(MyFILE * stream);
void myfputc(int b, MyFILE * stream);

// b
void mymkdir(char *path);
char **mylistdir(char *path);

// a
void mychdir(char *path);
void myrmdir(char *path);
void myremove(char *path);

// bonus
void myftran(const char* sysFile, const char* vdFile);
void myfcp(char* src, char* dst);
void myfmv(char* src, char* dst);

// helpers
void copyFAT();
int getUnusedBlock();
int getUnusedDir();
int getFileLocation(const char *filename);
void setFatChain(int fatEntry);
diskblock_t emptyBlock();
diskblock_t emptyDirBlock();
int getDir(const char *path);
void printDirList(char **dirs);
int isDirEmpty(dirblock_t dir);

// testing
void printBlock(int blockIndex);

#endif

