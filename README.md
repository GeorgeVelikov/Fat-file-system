Fat file system OS made with C
Part of the assignment for CS3026 Operating Systems in University of Aberdeen

Has the following functionalities
- format					- format()
- making/opening files		- myfopen("location/filename", mode)
- add char to file			- myfputc(char, myfopen(...))
- making directories			- mymkdir("location/dirname")
- list dir					- mylistdir("location/folderToCheck")
- copy file					- myfcp("location/source", "location/destination")
- remove file/dir				- myremove("location/target")
- mychdir					- not properly implemented, can be sort of seen in mymkdir

> mymkdir("/dir1")
> mymkdir("/dir1/dir2")
> mylistdir() will check the dir we are currently in - contents of dir1, which should be dir2

note: mymkdir changes the current dir location to the dir before target, in this case dir1
