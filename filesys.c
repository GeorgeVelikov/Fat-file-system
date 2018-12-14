#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "filesys.h"


diskblock_t  virtualDisk [MAXBLOCKS] ;           // define our in-memory virtual, with MAXBLOCKS blocks
fatentry_t   FAT         [MAXBLOCKS] ;           // define a file allocation table with MAXBLOCKS 16-bit entries
fatentry_t   rootDirIndex            = 0 ;       // rootDir will be set by format
direntry_t * currentDir              = NULL ;
fatentry_t   currentDirIndex         = 0 ;


// -----------------------------------------------------------------------------
// pre-defined
void writedisk ( const char * filename )
{
   printf ( "writedisk> virtualdisk[0] = %s\n", virtualDisk[0].data ) ;
   FILE * dest = fopen( filename, "w" ) ;
   if ( fwrite ( virtualDisk, sizeof(virtualDisk), 1, dest ) < 0 )
      fprintf ( stderr, "write virtual disk to disk failed\n" ) ;
   //write( dest, virtualDisk, sizeof(virtualDisk) ) ;
   fclose(dest) ;
   
}

void readdisk ( const char * filename )
{
   FILE * dest = fopen( filename, "r" ) ;
   if ( fread ( virtualDisk, sizeof(virtualDisk), 1, dest ) < 0 )
      fprintf ( stderr, "write virtual disk to disk failed\n" ) ;
   //write( dest, virtualDisk, sizeof(virtualDisk) ) ;
      fclose(dest) ;
}

// D
void format ()
{
	// buffer block
	diskblock_t block;
	// calc number of fat blocks given the definitions in the header
	int fatblocksneeded = MAXBLOCKS/FATENTRYCOUNT;
	// prepare block 0 : fill it with '\0',
	block = emptyBlock();
	
	// use strcpy() to copy some text to it for test purposes
	strcpy(block.data, "CS3026 Operating Systems Assignment");
	
	// write block 0 to virtual disk
	memmove(virtualDisk[0].data, &block.data, BLOCKSIZE);
	
	// prepare FAT table
	FAT[0] = ENDOFCHAIN;
	FAT[1] = 2;
	FAT[2] = ENDOFCHAIN;
	FAT[3] = ENDOFCHAIN;

	for (int i=4; i<MAXBLOCKS; i++)
		FAT[i] = UNUSED;

	// write FAT blocks to virtual disk
	copyFAT();

	// prepare root directory
	block = emptyDirBlock();
	// write root directory block to virtual disk
	rootDirIndex = fatblocksneeded + 1;
	memmove(virtualDisk[rootDirIndex].data, &block.data, BLOCKSIZE);
	
	// set current directory to be root
	currentDirIndex = rootDirIndex;
}

// C

MyFILE * myfopen(const char * filename, const char * mode)
{
	// check if we're in the correct mode
	if (!(strcmp(mode, "r") == 0 ||
		  strcmp(mode, "w") == 0))
		return FALSE;
		
	if (filename[0] == '/')
		currentDirIndex = rootDirIndex;
		
	// buffer string
	char *b = calloc(MAXPATHLENGTH, sizeof(char));
	// delimeter
	char *d= "/";
	// remainder of the string
	char *r = calloc(MAXPATHLENGTH, sizeof(char));
	// put the path to the buffer
	strcpy(b, filename);
	
	// grab a token from the buffer string separated by '/'
	for(char *token=strtok_r(b, d, &r); token; token=strtok_r(NULL, d, &r))
	{
		
		int fileLength;
		if(r!=NULL)
		{
			int location = getDir(token);
			if (location == EOF)
				exit(0);
			currentDirIndex = virtualDisk[currentDirIndex].dir.entrylist[location].firstblock;
		}
		else
		{
			
			// create a file,
			MyFILE *file = malloc(sizeof(MyFILE));
			file->pos = 0;
			file->buffer = emptyBlock();
			// assign the mode to the file r/w
			strcpy(file->mode, mode);
			
			// if it's for reading
			if(strcmp(mode, "r") == 0)
			{
				// if file does not exist, exit
				if(getFileLocation(token) == FALSE) 
					exit(0);
				
				// if it exists it returns the block it resides in
				file->blockno = getFileLocation(token);
				return file;
			}
			// if file exists
			if(getFileLocation(token) != FALSE)
			{
				
				// path is the blocknumber
				int path = getFileLocation(token);
		
				// if the block is smaller than root dir, in this case 3, quit
				if(path <= BLOCKSIZE/FATENTRYCOUNT) exit(0);
				
				// end the fat chain
				setFatChain(path);
				// save the fat table
				copyFAT();
				
				// empty all dir entries to "overwrite" the file
				diskblock_t dirBlock;
				fatentry_t nextDir = rootDirIndex;
				// while the fat is not eoc
				while(nextDir != ENDOFCHAIN)
				{
					// set the current working directory as our buffer fat
					currentDirIndex = nextDir;
					// move the nextDir-th block to the buffer block
					memmove(&dirBlock.data, virtualDisk[nextDir].data, BLOCKSIZE);
					// set currentDir entry list as the one from the buffer block
					currentDir = dirBlock.dir.entrylist;
					// for all entries in the buffer block
					for(int i=0; i<dirBlock.dir.nextEntry; i++)
						// if the entry matches the file's name
						if(strcmp(currentDir[i].name, filename) == 0)
						{
							// clean the block
							currentDir[i].firstblock = getUnusedBlock();
							currentDir[i].entrylength = sizeof(currentDir);
							break;
						}
					// nextdir++
					nextDir = FAT[nextDir];
				}
				
				// get the block o
				int location = getUnusedDir();
				currentDir[location].unused = FALSE;
			}
			// if file does not exist
			else
			{
				// get the next unused block
				int index = getUnusedBlock();
				// innit a buffer block
				diskblock_t block;
				
				// fill in buffer block with currentDir block, should be empty/formatted
				memmove(&block.data, virtualDisk[currentDirIndex].data, BLOCKSIZE);
				
				// create a new entry and allocate memory for it
				direntry_t *newEntry = calloc(1, sizeof(direntry_t));
				// set the entry as a file
				newEntry->isdir = FALSE;
				newEntry->unused = FALSE;
				newEntry->firstblock = index;
				// set next fat block as EOC
				FAT[index] = ENDOFCHAIN;
				// save fat table
				copyFAT();
				
				// get first unusred dir entry in the first dir block that has free entries
				int entryNumber = getUnusedDir();
				// set the global currentDir to the location where the file is to be created
				currentDir = block.dir.entrylist;
				// save the entry in the current dir the file name
				currentDir[entryNumber] = *newEntry;
				strcpy(currentDir[entryNumber].name, token);

				// save the buffer block to the virtual disk
				memmove(virtualDisk[currentDirIndex].data, &block.data, BLOCKSIZE);
				// save the blocknumber where the file is saved (starting point)
				file->blockno = index;
				
				return file;
			}
		}
	}	
}

void myfputc(int b, MyFILE *stream)
{
	// check if the mode is in reading, if yes - return 
	if (strcmp(stream->mode, "r") == FALSE)
		return;
	
	// if the position of the "text pointer" is equal or bigger than blocksize (1024 in this case), stops at 1024
	if (stream->pos >= BLOCKSIZE)
	{
		// get the block number of the next unused block
		int indexOfUnusedBlock		= getUnusedBlock()		;
		// change the fat table so that the block is checked as used
		FAT[stream->blockno] 		= indexOfUnusedBlock	;
		// set next block as eoc
		FAT[indexOfUnusedBlock] 	= ENDOFCHAIN			;
		// save fat table
		copyFAT();
		
		// reset the string stream pos/"text pointer" (kind of) 
		stream->pos = 0;
		// save the buffer to the virtual disk (since the buffer is 1024bytes)
		memmove(virtualDisk[stream->blockno].data, &stream->buffer.data, BLOCKSIZE);
		// empty the buffer
		stream->buffer = emptyBlock();
		// assign the new unused block number to the buffer
		stream->blockno = indexOfUnusedBlock;
	}
	// put the character in the buffer and increment the position of the text pointer
	stream->buffer.data[stream->pos] = b;
	stream->pos++;	

}

int myfgetc(MyFILE *stream)
{
	// snapshot the block number in use
	int index = stream->blockno;
	
	// check if the block number is an EOC, the mode of the file is reading or the data in contains is null
	if(index == ENDOFCHAIN
	|| strcmp(stream->mode, "r") == TRUE)
		return EOF;
	// if it passes the if, return null
		
	// since I couldn't figure out a way to reset the text poibter position we just check
	// if it's a multiple of blocksize (1024) which is when we would reset it
	if (stream->pos % BLOCKSIZE == 0)
	{
		// the blocknumber of the buffer is the next block in the fat table
		stream->blockno = FAT[index];
		// save the virtual disk block to the buffer block from which we read
		memcpy(&stream->buffer, &virtualDisk[index], BLOCKSIZE);
		// reset the position of the text pointer
		stream->pos = 0;	
	}
	// return the char and increment the text pointer
	return stream->buffer.data[stream->pos++];
}

void myfclose(MyFILE *stream)
{
	// if the mode is in write
	if(strcmp(stream->mode, "w") == 0)
	{
		// get the next unused block
		int next = getUnusedBlock();
		// set fat table block as used
		FAT[stream->blockno] = next;
		// move eoc to the next fat block
		FAT[next] = ENDOFCHAIN;
		// save fat table
		copyFAT();
		// save the incomplete buffer (this is because we wouldnt always have 1024 byte filled buffer blocks
		memmove(virtualDisk[stream->blockno].data, &stream->buffer.data, BLOCKSIZE);
	}
}

// B
void mymkdir(char *path)
{
	// create buffer bock
	diskblock_t block;
	
	// makesure the last char is not /, can be fooled by having "folder//"
	if(path[strlen(path)-1] == '/')
		path[strlen(path)-1] = '\0'; //gives warning if NULL :(
	// i've noticed thatif you make a root dir mymkdir("/") then it will crash
	
	// set root directory if first char is root
	if(path[0] == '/')
		currentDirIndex = rootDirIndex;
	
	// buffer string
	char *b = calloc(sizeof(path), sizeof(char));
	// delimeter
	char *d= "/";
	// remainder of the string
	char *r = calloc(MAXPATHLENGTH, sizeof(char));
	// put the path to the buffer
	strcpy(b, path);
	
	// strtok gives a token if s is a string s[0] to s[x-1] if s[x] is d (or the starting point of some string d)
	for(char *token=strtok_r(b, d, &r); token; token=strtok_r(NULL, d, &r))
	{
		// get block number of the directory extracted from the path
		int location = getDir(token);
		// if the path is correct
		if(r!=NULL)
		{
			// if the block number is -1
			if(location == EOF)
				exit(0);
			// set the current working directory as the dir entry we're iterating through (0,1,2) in this case
			currentDirIndex = virtualDisk[currentDirIndex].dir.entrylist[location].firstblock;
		}
		else
		{	
			// if the dir number is not -1 we exit
			if(location != EOF)
				exit(0);
				
			// reuse the int to get the first unused entry in the dir blocks
			location = getUnusedDir();
			// put the token (part of the path) as a name in the direntry
			strcpy(currentDir[location].name, token);
			// set dir entry properties
			currentDir[location].isdir = TRUE;
			currentDir[location].unused = FALSE;
			currentDir[location].firstblock = getUnusedBlock();
			
			// fill it up with a clear block
			block = emptyDirBlock();
			memmove(virtualDisk[currentDir[location].firstblock].data, &block.data, BLOCKSIZE);
			
			// set the fat table
			FAT[currentDir[location].firstblock] = ENDOFCHAIN;
			copyFAT();
		}
		// if the remainder is not '\0', happens once we've been through all subpaths
	}
}

// TODO: Implement mylistdir
char **mylistdir(char *path)
{
	// if the first char from the path is /, then set current dir as root
	if(path[0] == '/')
		currentDirIndex = rootDirIndex;

	// buffer string
	char *b =  calloc(sizeof(path), sizeof(int));
	// delimeter
	char *d= "/";
	// remainder of the string
	char *r = calloc(MAXNAME, sizeof(char));
	
	// copy string path to b
	strcpy(b, path);
	
	// strtok gives a token if s is a string s[0] to s[x-1] if s[x] is d (or the starting point of some string d)
	for(char *token=strtok_r(b, d, &r); token; token=strtok_r(NULL, d, &r))
	{
		// get the block number of the token
		int location = getDir(token);
		// if it does not exist
		if (location == EOF)
			exit(0);
		
		// set the current dir index as the location inside the token
		currentDirIndex = virtualDisk[currentDirIndex].dir.entrylist[location].firstblock;
	}
	
	// create a buffer block
	diskblock_t block;
	// set a buffer for fat
	fatentry_t nextDir = currentDirIndex;
	
	// set aside memory for the array that will hold dirs/files (ls) // currently one index only
	char **result = malloc(sizeof(char*));
	int index = 0;
	
	// while the fat entry is not EOC
	while(nextDir != ENDOFCHAIN)
	{
		// get that fat block and save it on the buffer block
		memmove(&block.data, virtualDisk[nextDir].data, BLOCKSIZE);
		// set the currentdir list to the entries of the buffer block
		currentDir = block.dir.entrylist;
		// iterate over the entries in the dir block
		for(int i=0; i<block.dir.nextEntry; i++)
		{
			// if an entry is unused
			if(currentDir[i].unused == FALSE)
			{
				// set aside memory for the list entry equiv to the filename
				result[index] = calloc(sizeof(currentDir[i].name), sizeof(char));
				// copy the filename and put it in the list and in the index we just allocated memory for
				strcpy(result[index], currentDir[i].name);
				index++;
				// reallocate the space result takes by increasing it so it holds index+1
				// we +1 to the memory as we want to keep one entry for an eof
				result = realloc(result, ((1+index)*sizeof(char*)));
			}
		}
		// nextdir++
		nextDir = FAT[nextDir];
	}
	// set last entry as eof
	result[index] = NULL;
	
	// return the list of files/dirs
	return result;
}

// A
void mychdir(char *path)
{
	// Did not have enough time for this, wasn't as needed since mymkdir already kind of had that functionality
	if(strcmp(path, "..") == 0)
	{
		// go to parent
		diskblock_t buffer;
		memmove(&buffer.data, virtualDisk[currentDirIndex].data, BLOCKSIZE);
	}

}

void myrmdir(char *path)
{
	// cant remove root
	if(strcmp(path, "/") == 0)
		return;
	
	// set as root
	if(path[0] == '/')
		currentDirIndex = rootDirIndex;
		
	// buffer string	
	char *b = calloc(MAXPATHLENGTH, sizeof(char));
	// delimeter
	char *d= "/";
	// remainder of the string
	char *r = calloc(MAXPATHLENGTH, sizeof(char));
	// put the path to the buffer
	strcpy(b, path);
	
	// go over tokens
	for(char *token=strtok_r(b, d, &r); token; token=strtok_r(NULL, d, &r))
	{
		// if the remainder is not 0 (we haven't finished going over all toks
		if(r!=NULL)
		{
			// get location of tok
			int location = getDir(token);
			if (location == EOF)
				exit(0);
			// set currentdir
			currentDirIndex = virtualDisk[currentDirIndex].dir.entrylist[location].firstblock;
		}
		else
		{
			// parent is the dir in which the dir to dlete is stored, indextodel is which block the dir leads to
			int indexParent = currentDirIndex;
			int indexToDel = virtualDisk[indexParent].dir.entrylist[getDir(token)].firstblock;
			
			// impossible, but hey, just in case
			if (currentDirIndex == indexToDel)
				return;
			
			// bufer block	
			diskblock_t block;
			// assign the "empty" opened dir which we should delete
			memmove(&block.data, virtualDisk[indexToDel].data, BLOCKSIZE);
			if(isDirEmpty(block.dir))
			{
				// clear fat
				setFatChain(indexToDel);
				for(int i=0; i<block.dir.nextEntry; i++)
				{
					// create a fresh entry
					direntry_t *newEntry = calloc(1, sizeof(direntry_t));
					newEntry->isdir = FALSE;
					newEntry->unused = TRUE;
					newEntry->filelength = 0;
					newEntry->firstblock = ENDOFCHAIN;
					// save fat table
					copyFAT();
					// assign freshly cleaned entry to our buffer block
					block.dir.entrylist[i] = *newEntry;
					// save buffer block over the empty dir effectively destroying the dir, leaving us with the dir entry
					memmove(virtualDisk[indexToDel].data, &block.data, BLOCKSIZE);
				}
				// time to destroy the dir entry name
				// buffer block for the parent dir
				diskblock_t parentBlock;
				// assign dir where our dir to delete is stored in the entries
				memmove(&parentBlock.data, virtualDisk[indexParent].data, BLOCKSIZE);
				// clean when we have a match
				for(int i=0; i<parentBlock.dir.nextEntry; i++)
				{
					// if we a get a match in the dir entries with our token
					if(strcmp(parentBlock.dir.entrylist[i].name, token) == 0)
					{
						// clear fat
						setFatChain(indexParent);
						// big woops, clean everything, get unused block number
						int index = getUnusedBlock();
						// fresh entry
						direntry_t *newEntry = calloc(1, sizeof(direntry_t));
						newEntry->isdir = FALSE;
						newEntry->unused = TRUE;
						newEntry->filelength = 0;
						newEntry->firstblock = ENDOFCHAIN;
						FAT[index] = ENDOFCHAIN;
						// save fat table
						copyFAT();
						// assign fresh entry to the buffer block
						parentBlock.dir.entrylist[i] = *newEntry;
						// save cleaned buffer block to the virtual disk
						memmove(virtualDisk[indexParent].data, &parentBlock.data, BLOCKSIZE);
					}
				}
			}
			// this here saved my sanity
			else
				printf("\nCannot be deleted as directory is not empty");
		}
	}
}

void myremove(char *path)
{
	// cant remove root
	if(strcmp(path, "/") == 0)
		return;
	
	// set as root
	if(path[0] == '/')
		currentDirIndex = rootDirIndex;
		
	// buffer string	
	char *b = calloc(MAXPATHLENGTH, sizeof(char));
	// delimeter
	char *d= "/";
	// remainder of the string
	char *r = calloc(MAXPATHLENGTH, sizeof(char));
	// put the path to the buffer
	strcpy(b, path);
	
	// go over tokens
	for(char *token=strtok_r(b, d, &r); token; token=strtok_r(NULL, d, &r))
	{
		// if the remainder is not 0 (we haven't finished going over all toks)
		if(r!=NULL)
		{
			// get location of tok
			int location = getDir(token);
			if (location == EOF)
				exit(0);
			// set currentdir
			currentDirIndex = virtualDisk[currentDirIndex].dir.entrylist[location].firstblock;
		}
		else
		{
			// create a snapshot of the currentDirIndex
			int index = currentDirIndex;
			// create a buffer block
			diskblock_t block;
			// add the current dir to the buffer b lock
			memmove(&block.data, virtualDisk[index].data, BLOCKSIZE);
			
			// init fileLocation, getFileLocation doesn't quite work the way I needed it for this part
			int fileLocation;
			
			// get the dir entry in which the file is
			for(int i=0; i<block.dir.nextEntry; i++)
				if(strcmp(block.dir.entrylist[i].name, token) == 0)
					fileLocation = i;
			
			// set that dir entry as unused
			block.dir.entrylist[fileLocation].unused = TRUE;
			// clear the chain of fats
			int fb = block.dir.entrylist[fileLocation].firstblock;
			setFatChain(fb);
			// save fat table
		   	copyFAT();
		   	// save buffer block
		   	memmove(virtualDisk[index].data, &block.data, BLOCKSIZE);
		}
	}
}

// -----------------------------------------------------------------------------
// helper functions 
void copyFAT()
{
	// buffer block
	diskblock_t block;
	// block 1 and 2 are occupied by FAT in this case as maxblocks/fatentrycount is 2
	for (int blockNum=1, i=0; blockNum<= (MAXBLOCKS/FATENTRYCOUNT); blockNum++)
	{
		for(int entry=0; entry<FATENTRYCOUNT; entry++, i++)
			block.fat[entry] = FAT[i];
		// save the fat block to the virtual disk
		memmove(virtualDisk[blockNum].data, &block.data, BLOCKSIZE);
	}
}

int getUnusedBlock()
{
	// i starts from 4 in this case (3 is root dir)
	for (int i=MAXBLOCKS/FATENTRYCOUNT+1; i<MAXBLOCKS; i++)
		if (FAT[i] == UNUSED)
			return i;
	// if the block is not used in the fat, return the block number and exit
	exit(0);
	return -1;	 // avoid warnings
}

int getUnusedDir()
{
	// create buffer block and buffer fat entry
	diskblock_t block;
	fatentry_t nextDir = currentDirIndex;
	
	// go through all dir blocks
	while(nextDir != ENDOFCHAIN)
	{
		// save the current dir block to the buffer block
		memmove(&block.data, virtualDisk[nextDir].data, BLOCKSIZE);
		// for every dir entry in that specific dir block
		for (int i=0; i<block.dir.nextEntry; i++)
		{
			// if the dir entry is unused
			if(block.dir.entrylist[i].unused == TRUE)
			{
				// set that dir entry as the one to use and return its block num
				currentDirIndex = nextDir;
				currentDir = virtualDisk[currentDirIndex].dir.entrylist;
				return i;
			}
		}
		// checks the next block
		nextDir = FAT[nextDir];
	}
	// grabs the next unused block;
	int next = getUnusedBlock();
	// set it in the fat table
	FAT[currentDirIndex] = next;
	// move the EOC fat block
	FAT[next] = ENDOFCHAIN;
	// save the fat table
	copyFAT();
	
	// clean the dir block
	block = emptyDirBlock();
	// set our current working directory index as that of the newly found unused block
	currentDirIndex = next;
	// save the buffer block to the virtual disk
	memmove(virtualDisk[currentDirIndex].data, &block.data, BLOCKSIZE);
	// actually "enter" the current directory based on the index 
	currentDir = virtualDisk[currentDirIndex].dir.entrylist;
	// returns 0th entry 
	return 0;
}

int getFileLocation(const char *filename)
{
	// buffer dir block and buffer fat
	diskblock_t dirBlock;
	fatentry_t nextDir = currentDirIndex;
	
	// while we reach the end of the fat entries
	while(nextDir != ENDOFCHAIN)
	{
		// global current dir is the next fat entry
		currentDirIndex = nextDir;
		// grab the data from that block to our buffer
		memmove(&dirBlock.data, virtualDisk[nextDir].data, BLOCKSIZE);
		// current dir entry is the entries in the buffer block
		currentDir = dirBlock.dir.entrylist;
		// go through all buffer dir entries
		for(int i=0; i<dirBlock.dir.nextEntry; i++)
		{
			// if the name of the entry is the same as the file name
			if(strcmp(currentDir[i].name, filename) == 0)
				return currentDir[i].firstblock;
			// return the block where the entry is located
		}
		// nextDir++
		nextDir = FAT[nextDir];
	}
	// file does not exist
	return FALSE;
}

void setFatChain(int fatEntry)
{
	// next is the fat entry after fatEntry
	int next = FAT[fatEntry];
	// if the next fat entry is not eoc, recursively call endChain(fatEntry+1) //not literally fatEntry+1
	if (next != ENDOFCHAIN) setFatChain(next);	
	// clean by overwriting the data in the vd whose fat entry is eoc using the empty block
	memmove(virtualDisk[fatEntry].data, emptyBlock().data, BLOCKSIZE);
	// set next fatentry as unused
	FAT[fatEntry] = UNUSED;
}

diskblock_t emptyBlock()
{
	// create a block
	diskblock_t block;
	// set all of it's blocks as NULL
	memset(block.data, '\0', BLOCKSIZE);
	// return the newly cleaned block
	return block;
}

diskblock_t emptyDirBlock()
{
	// create a new clean block
	diskblock_t block = emptyBlock();
	// set it as a dir block
	block.dir.nextEntry = FALSE;
	block.dir.isdir = TRUE;
	
	// create a new dir entry list
	direntry_t cleanEntry;
	// set all dir entries as null
	memset(&cleanEntry, '\0', sizeof(direntry_t));
	
	// set the entry properties
	cleanEntry.isdir = FALSE;
	cleanEntry.unused = TRUE;

	// iterate over all dir entries in the block
	for(int i=0;i <DIRENTRYCOUNT; i++)
	{
		// set all entries as the newly created entry (clean)
		block.dir.entrylist[i] = cleanEntry;
		block.dir.nextEntry = i;
	}
	// return the newly created block with clean dirs
	return block;
}

int getDir(const char *path)
{
	// buffer block and fat
	diskblock_t block;
	fatentry_t nextDir = currentDirIndex;
	// while the fat entry is not eoc
	while(nextDir != ENDOFCHAIN)
	{
		// move the fat block from the vd to the buffer block
		memmove(&block.data, virtualDisk[currentDirIndex].data, BLOCKSIZE);
		// for every dir in the block
		for(int i=0; i<block.dir.nextEntry; i++)
		{
			// if the string in the dir entry is the same as the path
			if (strcmp(block.dir.entrylist[i].name, path) == 0)
			{
				// set currenr dir to be path
				currentDirIndex = nextDir;
				currentDir = block.dir.entrylist;
				// return block number
				return i;
			}
		}
		// nextdir++
		nextDir = FAT[nextDir];
	}
	return EOF;
}

int isDirEmpty(dirblock_t dir)
{
	if(dir.nextEntry == 0) return TRUE;
	for(int i=0; i<dir.nextEntry; i++)
		if(dir.entrylist[i].unused == FALSE)
			return FALSE;
	return TRUE;
}

void printDirList(char **dirs)
{
	// this is just for printing the list of dirs and make it look like ls
	int i = 0;
	printf("\n");
	while(dirs[i] != '\0')
	{
		printf("\t%s", dirs[i]);
		i++;
	}
	printf("\n\n");
	return;
}
// -----------------------------------------------------------------------------
// extra
void myftran(const char* sysFile, const char* vdFile)
{
	// i know how to make them individually so that they would work, but something
	// seems to be wrong with my if/else statement logic
	// if systemfile exists and vd file does not sys->vd
	if (fopen(sysFile, "r") != NULL && getDir(vdFile) == EOF)
	{
		FILE *systemFile = fopen(sysFile, "r");
		MyFILE *virtualFile = myfopen(vdFile, "w");
		
		// for i up to the file length of the system file
		//for(int i=0; i< _filelength(_fileno(systemFile)); i++)
			myfputc(fgetc(systemFile), virtualFile);
		// put a char in the virtual file acquired from the systemfile	
		
		// close both files;
		myfclose(virtualFile);
		fclose(systemFile);
	}
	// else vd->sys
	else if(fopen(sysFile, "r") == NULL && getDir(vdFile) != EOF)
	{
		FILE *systemFile = fopen(sysFile, "w");
		MyFILE *virtualFile = myfopen(vdFile, "r");
		
		while(1)
		{
			char ch = myfgetc(virtualFile);
			if (ch == EOF) break;
			fputc(ch, systemFile);
		}

		// close both files;
		myfclose(virtualFile);
		fclose(systemFile);
	}
}

void myfcp(char* src, char* dst)
{
	MyFILE *source = myfopen(src, "r");
	MyFILE *destination = myfopen(dst, "w");

	while(1)
	{
		char ch = myfgetc(source);
		if( ch == '\0') break;
		myfputc(ch, destination);
	}
	myfclose(source);
	myfclose(destination);
}

void myfmv(char* src, char* dst)
{
	MyFILE *source = myfopen(src, "w");
	MyFILE *destination = myfopen(dst, "w");
	while(1)
	{
		char ch = myfgetc(source);
		if( ch == '\0') break;
		myfputc(ch, destination);
	}
	myfclose(source);
	myfclose(destination);
	// the concept I follow for the move is to create an exacty copy the same way
	// myfcp works and then just delete the source file
	myremove(src);
}

// -----------------------------------------------------------------------------
// testing purposes
void printBlock ( int blockIndex )
{
   printf ( "virtualdisk[%d] = %s\n", blockIndex, virtualDisk[blockIndex].data ) ;
}

