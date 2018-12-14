#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "filesys.h"

void d_task()
{
	format();
	writedisk("virtualdiskD3_D1");
}

void c_task()
{
	format();
	printf("\n");
	MyFILE *fileWrite = myfopen("testfile.txt", "w");

	const char *text = "I mastered programming within hours. I wrote my OWN versions of the two most popular programs in the world. \
The first is Hello World. While rudimentary, and lacking in threading capabilities, it boasts an extremely user-friendly and intuitive interface. \
I wrote it in c++ using gcc compiler, Asus monitor, and human fingers. The second is Good Bye world. While it may not be as widely known as Hello World, \
it still offers the EXACT same amount of utility that the hello world program offers. I wrote this in Java using some coffee grounds I found at the bottom \
of my Starbucks cup. I compiled it myself with a straw. I too am thinking of expanding on my Hello World program and turning it into a multi-greeting, \
synchronized planetary operating system based off the not widely known Iinux kernel (which I also made by hand using binary found in the background \
of the Matrix (the first), specifically minutes 22 thru 34 as a base.) I've yet to compile my binary into assembly, and then compile the assembly \
into c++ so that I can finally recompile into an exe, but when I do... I'll be rich! I'm going to name the OS Look Out World, or LOW for short. \
So look out world, there will be a new LOW coming from me soon!!";

	for (int i=0; i<FOURKB; i++)
		myfputc(text[i%strlen(text)], fileWrite);

	myfclose(fileWrite);
	
	FILE *systemFile = fopen("testfileC3_C1_copy.txt", "w");
	MyFILE *fileRead = myfopen("testfile.txt", "r");
	for (int i=0; i<FOURKB; i++)
	{
		char ch = myfgetc(fileRead);
		if (ch==EOF)
			break;
		printf("%c", ch);
		fprintf(systemFile, "%c", ch);
	}
	myfclose(fileRead);
	fclose(systemFile);	
	printf("\n\n");
	writedisk("virtualdiskC3_C1");
}

void b_task()
{
	const char *text = "Good evening.";
	char **list;
	format();
	// i do not support recursive mkdir (mkdir -p)
	mymkdir("/myfirstdir"); // creates but does not enter it
	mymkdir("/myfirstdir/myseconddir"); // enter myfirstdir and create myseconddir
	mymkdir("myseconddir/mythirddir");	// relative path creation as we're already in myfirstdir
	// in this case we're currently in myseconddir
	list = mylistdir("/myfirstdir/myseconddir");
	printDirList(list);
	writedisk("virtualdiskB3_B1_a");
	
	MyFILE *fileWrite = myfopen("/myfirstdir/myseconddir/testfile.txt", "w");
	
	for (int i=0; i<sizeof(text)*10; i++)
	
		myfputc(text[i%strlen(text)], fileWrite);

	myfclose(fileWrite);
	
	//myfcp("/myfirstdir/myseconddir/testfile.txt", "/myfirstdir/myseconddir/woahboi.txt");
	//myremove("/myfirstdir/myseconddir/testfile.txt");
	
	list = mylistdir("/myfirstdir/myseconddir");
	printDirList(list);
	writedisk("virtualdiskB3_B1_b");
	printf("\n\n");
}

void a_task()
{
	format();
	const char *text = "Segfaults are fun.";
	char **list;
	mymkdir("/myfirstdir");
	mymkdir("/myfirstdir/myseconddir");
	
	MyFILE *fileWrite1 = myfopen("/myfirstdir/myseconddir/testfile1.txt", "w");
	for (int i=0; i<sizeof(text); i++)
		myfputc(text[i%strlen(text)], fileWrite1);
	myfclose(fileWrite1);
	
	list = mylistdir("/myfirstdir/myseconddir");
	printDirList(list);
	
	//already in myseconddir as mymkdr 'accesses' the folders
	
	list = mylistdir("/myfirstdir/myseconddir/");
	printDirList(list);
	MyFILE *fileWrite2 = myfopen("testfile2.txt", "w");
	for (int i=0; i<sizeof(text)*2; i++)
		myfputc(text[i%strlen(text)], fileWrite2);
	myfclose(fileWrite2);
	
	mymkdir("thirddir");
	
	MyFILE *fileWrite3 = myfopen("testfile3.txt", "w");
	for (int i=0; i<sizeof(text)*3; i++)
		myfputc(text[i%strlen(text)], fileWrite3);
	myfclose(fileWrite3);

	writedisk("virtualdiskA5_A1_a");
	
	myremove("testfile1.txt");
	myremove("testfile2.txt");
	
	writedisk("virtualdiskA5_A1_b");
	
	mychdir("thirddir");
	myremove("testfile3.txt");
	
	writedisk("virtualdiskA5_A1_c");
	
	myrmdir("thirddir");
	
	/// lack of mychdir stop me from doing the tasks forwards and it seg faults here
	// 																			/
	mychdir("/firstdir"); //< -------------------------------------------------/
	
	myrmdir("/seconddir");
	mychdir("/");
	myrmdir("firstdir");
	
	printf("\n");
	writedisk("virtualdiskA5_A1_d");
}

void main()
{
	d_task();
	
	c_task();
	
	b_task();
	
	a_task();
}

