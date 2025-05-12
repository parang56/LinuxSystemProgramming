//Create linked list
LogLinkedList* list;

//Boolean flag for option -y
bool isForceBackup = false;

//Boolean flag for checking for directories
bool isCheckDir = false;

//Function to check if the file was backed up
int isFileBackedUp(char *absolutePath, char* backupPath) {
	struct dirent **namelist;
	char *dirPath = (char *)malloc(sizeof(char *) * PATHMAX);
	char *filehash = (char *)malloc(33);
  	char *tmphash = (char *)malloc(33);
	int cnt;
	bool hasBeenBackedUp = false;
	
	//Use md5() to hash file that we are trying to backup
    	md5(absolutePath, filehash); 
	for(LogNode *current = list->head; current != NULL; current = current->next) {
		//If absolutePath and originalPath is same check hash
		if(!strcmp(current->originalPath, absolutePath)) {
			//check hash
			md5(current->backupPath, tmphash);
			//If hash is same set bool to true, and update dirPath to latest same file.
			if(!strcmp(filehash, tmphash)) { 
				hasBeenBackedUp = true;
				dirPath = current->backupPath;
			}
		}
	}
	
	//If multiple files with same hash exist, get the latest timestamp and print "already backuped"
	if(hasBeenBackedUp == true) {
		printf("\"%s\" already backuped to \"%s\"\n", absolutePath, dirPath);
		char* rmPath = dirname(dirPath);
		rmdir(rmPath);
		return 1; 
	}
	
	//If file hasn't been backed up return 0 
	return 0;
}

//Function to back up a file to backup dir.
void BackUpFile(char *absolutePath, char *timeStamp, char* backupDir) {
	int fd1, fd2;
	ssize_t len;
	char *buf = (char*)malloc(sizeof(char*)*STRMAX);
	char *tmpBackUpDir = (char*)malloc(sizeof(char*)*PATHMAX);
	char *backupPath = (char *)malloc(sizeof(char *) * PATHMAX);
	
	//Create temporary backup dir
	strcpy(tmpBackUpDir, backupDir);
	
	//Get filename from absolutePath
	char *filename = strrchr(absolutePath, '/');
	if(!filename) filename = absolutePath;
	else filename++;
	
	//Make backupPath from backupDir and file's name
	sprintf(backupPath, "%s/%s", backupDir, filename);
	
	//If force backup is false
	if(isForceBackup == false) {
		//Check if file is already backed up 
		if(isFileBackedUp(absolutePath, backupPath)) {
			return;
		}
	}

	//Append filename to new backup path
	sprintf(tmpBackUpDir + strlen(tmpBackUpDir), "/%s", filename);
	
	//Open the original file
	if((fd1 = open(absolutePath, O_RDONLY)) < 0) {
		fprintf(stderr, "Failed to open source file\n");
		exit(1);
	}
	
	// Open/Create backup file
	if((fd2 = open(tmpBackUpDir, O_CREAT | O_WRONLY | O_TRUNC, 0777)) < 0){
		fprintf(stderr, "Failed to open/create backup file\n");
		close(fd1);
		exit(1);
	}
	
	// Copy file
	while((len = read(fd1, buf, sizeof(buf))) > 0) {
		if(write(fd2, buf, len) != len) {
			fprintf(stderr, "Failed to write to backup file\n");
			close(fd1);
			close(fd2);
			exit(1);
		}
	}
	
	//Write backup message in terminal
    	printf("\"%s\" backuped to \"%s\"\n", absolutePath, backupPath);
	
	//Close files and free memory
	close(fd1);
	close(fd2);
	
	//Create ssubak.log or write in ssubak.log
	BackUpLog(timeStamp, absolutePath, backupPath, 0);
	
	return;
}

//Struct for BFS
typedef struct {
	char *dirPath;
	char *backupPath;
} DirQueueItem;

//Function to backup directories in a BFS like manner
void BackUpDir(char* absolutePath, char *timeStamp, char* backupDir) {
	struct dirent **namelist;
	struct stat statbuf;
  	char *tmppath = (char *)malloc(sizeof(char *) * PATHMAX);
  	char *backupPath = (char *)malloc(sizeof(char *) * PATHMAX);
  	int cnt;

	DirQueueItem *dirQueue = (DirQueueItem *)malloc(sizeof(DirQueueItem) * PATHMAX);
	int queueEnd = 0;

	//cnt is number of files in absolutePath
  	if((cnt = scandir(absolutePath, &namelist, NULL, alphasort)) == -1) {
		fprintf(stderr, "ERROR: scandir error for %s\n", absolutePath);
		return;
  	}
  	
  	//Check each file in absolutePath
  	for(int i = 0; i < cnt; i++) {
		if(!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;
		sprintf(tmppath, "%s/%s",absolutePath, namelist[i]->d_name);

		if (lstat(tmppath, &statbuf) < 0) {
			fprintf(stderr, "ERROR: lstat error for %s\n", tmppath);
			return;
		}
		
		//Get backupPath for file or directory inside backupDir
		sprintf(backupPath, "%s/%s", backupDir, namelist[i]->d_name);
		
		//BFS like -> save to queue to check directories later
		if(S_ISDIR(statbuf.st_mode) && (isCheckDir == true)) {
			dirQueue[queueEnd].dirPath = strdup(tmppath);
			dirQueue[queueEnd].backupPath = strdup(backupPath);
			queueEnd++;
		}
		//If file, run BackUpFIle()
		else if(S_ISREG(statbuf.st_mode)) {
			BackUpFile(tmppath, timeStamp, backupDir);
		}
	}
	
	//Check directories after files were checked
	for(int i=0; i<queueEnd; i++) {
		if(mkdir(dirQueue[i].backupPath, statbuf.st_mode) == -1) {
			fprintf(stderr, "Error : Couldn't create directory\n");
			continue;
		}
		//Recursively go through to check for inner files			
		BackUpDir(dirQueue[i].dirPath, timeStamp, dirQueue[i].backupPath);
		free(dirQueue[i].dirPath);
		free(dirQueue[i].backupPath);
	}
	
	//If backupDir is empty after backing up, remove directory
	rmdir(backupDir);
	
	return;
}
