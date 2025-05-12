bool isForceRemove;

int removeFile(char* absolutePath, char* timeStamp, int num, char* filename)
{
	char *filehash = (char *)malloc(33);
  	char *tmphash = (char *)malloc(33);
  	bool hasRemoved = false;
  	//Use md5() to hash file that we are trying to backup
    	md5(absolutePath, filehash); 
  	
	for(LogNode *current = list->head; current != NULL; current = current->next) {
		if(strcmp(current->timeStamp, "") && current->wasRemoved != true) {
			//If force remove is false, check if tmpRemoveNum == num
			//If force remove is true don't check num and erase files
			if(isForceRemove == false)
				if(current->tmpRemoveNum != num) continue; 
		
			//Get filename from absolutePath
			char *curfilename = strrchr(current->backupPath, '/');
			if(!curfilename) curfilename = current->backupPath;
			else curfilename++;
			
			//If name is different, continue
			if(strcmp(filename, curfilename))
				continue;

			//If original path is different continue
			if(strcmp(absolutePath, current->originalPath))
				continue;

			
			//remove file
			remove(current->backupPath);
			
			//print "removed by"
			printf("\"%s\" removed by \"%s\"\n", current->backupPath, current->originalPath);
			hasRemoved = true;
			char* timeStampDir = (char *)malloc(sizeof(char *) * PATHMAX);
			getPathUpToTimeStamp(current->backupPath, timeStampDir);
			
			//set current wasRemoved, hasChecked bool to true
			current->wasRemoved = true;
			current->hasChecked = true;
			
			for(LogNode *current2 = list->head; current2 != NULL; current2 = current2->next) {
				if(!strcmp(current->originalPath, current2->originalPath)){
					current2->hasChecked = true;
				}
			}
			
			//If directory is empty, remove
			RemoveEmptyDirRecursive(timeStampDir);
			
			//Add the remove log to ssubak.log
			BackUpLog(timeStamp, current->backupPath, current->originalPath, 1);
		}
	}
	return hasRemoved;
}

int isFileBackedUp_Remove(char *absolutePath, char* timeStamp) {
	struct dirent **namelist;
	char *dirPath = (char *)malloc(sizeof(char *) * PATHMAX);
	
	bool hasBeenBackedUp = false;
	int checkidx = 0;
	int idx = 0;
	bool hasErased = false;
	
	//Get filename from absolutePath
	char *filename = strrchr(absolutePath, '/');
	if(!filename) filename = absolutePath;
	else filename++;

  	//Iterate through linked list nodes
	for(LogNode *current = list->head; current != NULL; current = current->next) {
		//If absolutePath and originalPath is same, and if node wasn't removed
		//set linked list node's timestamp.
		if(!strcmp(current->originalPath, absolutePath)) {
			char timeStamp[16];
			hasBeenBackedUp = true;
			dirPath = current->backupPath;
			int doesMatch = sscanf(current->backupPath, "/home/backup/%[^/]/", timeStamp);
			if(doesMatch != 1) {
				printf("backuppath is probably wrong\n");
				exit(1);
			}
			sprintf(current->timeStamp, "%s", timeStamp);
		}
	}
	
	if(hasBeenBackedUp == true) {
		for(LogNode *current = list->head; current != NULL; current = current->next) {
			//If timeStamp in linked list isn't empty, print timeStamp and file size
			if(strcmp(current->timeStamp, "") && current->wasRemoved != true && current->hasChecked != true){
				//If original paths are different, continue		
				if(strcmp(current->originalPath, absolutePath)) {
					continue;
				}
				
				checkidx++;
			}
		}
	}
	
	//No file exists, print error
	if(checkidx == 0) {
		fprintf(stderr, "Error : No backup file exists for %s\n", absolutePath);
		for(LogNode *current = list->head; current != NULL; current = current->next) {
			if(!strcmp(current->originalPath, absolutePath)){
				current->hasChecked = true;
			}
		}
		return 0;
	}
	
	//Only one backuped file exists, erase and print log
	else if(checkidx == 1) {
		hasErased = removeFile(absolutePath, timeStamp, 0, filename);
		hasBeenBackedUp = false;
	}
	
	//More than one backuped file exists, choose to erase
	//If hasBeenBackedUp is true, print absolutePath, timestamp, and corresponding file sizes.
	if(hasBeenBackedUp == true) {
		//if force remove is false
		if(isForceRemove == false) {
			printf("backup files of %s\n", absolutePath);
			printf("0. exit\n");
			for(LogNode *current = list->head; current != NULL; current = current->next) {
				if(current->hasChecked == true)
					continue;
				
				off_t fsize;
				int fd;
				// Open file with O_RDONLY
				if((fd = open(current->backupPath, O_RDONLY)) < 0) {	
					continue;
				}

				//Get fsize by using lseek to move file offset to end
				if((fsize = lseek(fd, 0 , SEEK_END)) < 0) {
					fprintf(stderr, "lseek error\n");
					exit(1);
				}
				
				//If timeStamp in linked list isn't empty, print timeStamp and file size
				if(strcmp(current->timeStamp, "") && current->wasRemoved != true && current->hasChecked != true){
					char* numWithCommas = addCommas(fsize);
					idx++;
					current->tmpRemoveNum = idx;
					printf("%d. %s      %sbytes\n", idx, current->timeStamp, numWithCommas);
					current->hasChecked = true;
					free(numWithCommas);
				}
			}
		}
		//Force remove is true
		else{
			for(LogNode *current = list->head; current != NULL; current = current->next) {
				if(current->hasChecked == true)
					continue;
				
				off_t fsize;
				int fd;
				// Open file with O_RDONLY
				if((fd = open(current->backupPath, O_RDONLY)) < 0) {	
					continue;
				}

				//Get fsize by using lseek to move file offset to end
				if((fsize = lseek(fd, 0 , SEEK_END)) < 0) {
					fprintf(stderr, "lseek error\n");
					exit(1);
				}
				
				//Don't print, just add to list
				if(strcmp(current->timeStamp, "") && current->wasRemoved != true && current->hasChecked != true){
					idx++;
					current->tmpRemoveNum = idx;
					current->hasChecked = true;
				}
			}
		}
		//Only get inputs if force remove is false
		if(isForceRemove != true){
			printf(">> ");
			int num;
			scanf("%d", &num);
			//If num is 0, exit
			if(num == 0) { exit(0); }
			//If num is bigger than idx, print error message
			else if(num > idx) { fprintf(stderr, "Please input a correct number. Exiting...\n"); }
			//If num is one of the numbers printed out, erase the file
			else{
				hasErased = removeFile(absolutePath, timeStamp, num, filename);
			}
		}
		else {
			hasErased = removeFile(absolutePath, timeStamp, 0, filename);
		}
	}

	return hasErased;
}
