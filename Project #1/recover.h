bool isRecoverMostRecent = false;
bool isRecoverToNew = false;
bool isOriginalExist = true;
bool single = false;
char *newPath;

int recoverFile(char* absolutePath, char* timeStamp, int num, char* filename, char* backupDir)
{
	char *filehash = (char *)malloc(33);
  	
  	char *buf = (char*)malloc(sizeof(char*)*STRMAX);
  	int fd1, fd2;
  	ssize_t len;
  	
  	bool hasRecovered = false;
  	
  	//Use md5() to hash file that we are trying to backup
    	md5(absolutePath, filehash); 
 
	for(LogNode *current = list->head; current != NULL; current = current->next) {
		if(strcmp(current->timeStamp, "") && current->wasRemoved != true && current->hasChecked != true) { 
			//Only recover is num is equal
			if(current->tmpRemoveNum != num) 
				continue; 
			bool isSame = false;
			bool isContinue = false;
			
			//If DirNonExist was set to true
			if(DirNonExist == true){
				char* tmp = (char *)malloc(sizeof(char *) * PATHMAX);
				strcpy(tmp, current->originalPath);
				if(isCheckDir != true){
					if(!strcmp(tmp, absolutePath))
						isSame = true;
					
					if(isSame != true){
						while(strcmp(dirname(tmp), absolutePath)){
							//No instance found, break;
							if(strlen(tmp) == 1){
								isContinue = true;
								break;
							}
						}
					}
				}
			}
			//No instance found, don't execute code below and continue
			if(isContinue == true)
				continue;
				
			//Get filename from absolutePath
			char *curfilename = strrchr(current->backupPath, '/');
			if(!curfilename) curfilename = current->backupPath;
			else curfilename++;
			
			//If original path is different continue
			if(strcmp(absolutePath, current->originalPath))
				continue;
				
			char *tmphash = (char *)malloc(33);
			md5(current->backupPath, tmphash);
			
			//If hash was same and we arent using -n, print not changed with and continue;
			if(!strcmp(filehash, tmphash) && isRecoverToNew == false) {
				printf("\"%s\" not changed with \"%s\"\n", current->backupPath, current->originalPath);
				hasRecovered = true;
				current->hasChecked = true;
				continue;
			}
			
			//Open the original file
			if((fd1 = open(current->backupPath, O_RDONLY)) < 0) {
				fprintf(stderr, "Failed to open source file\n");
				exit(1);
			}
			
			//If isRecoverToNew is false, backup in original path
			if(isRecoverToNew == false) {
				char *tmpPath = (char *)malloc(sizeof(char *) * PATHMAX);
				char time[14] = "123456789123/";
				
				//isOriginalNew is true (the original directory doesnt' exist)
				if(isOriginalNew == true){
					char* newtmp = (char *)malloc(sizeof(char *) * PATHMAX);
					strcpy(newtmp, current->originalPath);
					backupDir = getFullPath(dirname(newtmp));
					sprintf(tmpPath, "%s/%s", backupDir, filename);
				}
				//Original directory exists 
				else{
					char* tmpPath2 = current->backupPath + strlen(backupPATH) + strlen(time);
					char* tmp3 = (char *)malloc(sizeof(char *) * PATHMAX);
					int k = 0;
					strcpy(tmp3, tmpPath2);
					tmp3 = dirname(tmp3);
					k = strlen(tmp3);
					tmp3++;
					sprintf(tmpPath, "%s/%s", backupDir, tmp3);
					getFullPath(tmpPath);
					if(k==1){
						sprintf(tmpPath, "%s/%s", backupDir, filename);
					}
					else if(k > 1){
						sprintf(tmpPath, "%s/%s/%s", backupDir, tmp3, filename);
					}
					else{
						fprintf(stderr, "What error is this?\n");
					}
				}
				//Error opening 
				if((fd2 = open(current->originalPath, O_CREAT | O_WRONLY | O_TRUNC, 0777)) < 0){
					fprintf(stderr, "Failed to open/create original file\n");
					close(fd1);
					exit(1);
				}
				//print "recovered to"
				printf("\"%s\" recovered to \"%s\"\n", current->backupPath, current->originalPath);
				hasRecovered = true;
				//Add the recover log to ssubak.log
				BackUpLog(timeStamp, current->backupPath, current->originalPath, 2);
			} 
			//Else backup to new directory 
			else {
				char *tmpPath = (char *)malloc(sizeof(char *) * PATHMAX);
				char time[14] = "123456789123/";
				//isOriginalNew is true (the original directory doesnt' exist)
				if(isOriginalNew == true){
					char* newtmp = (char *)malloc(sizeof(char *) * PATHMAX);
					strcpy(newtmp, current->originalPath);
					backupDir = getFullPath(dirname(newtmp));
					sprintf(tmpPath, "%s/%s", backupDir, filename);
				}
				//Original directory exists 
				else{
					char* tmpPath2 = current->backupPath + strlen(backupPATH) + strlen(time);
					char* tmp3 = (char *)malloc(sizeof(char *) * PATHMAX);
					int k = 0;
					strcpy(tmp3, tmpPath2);
					tmp3 = dirname(tmp3);
					k = strlen(tmp3);
					tmp3++;
					sprintf(tmpPath, "%s/%s", backupDir, tmp3);
					getFullPath(tmpPath);
					if(k==1){
						sprintf(tmpPath, "%s/%s", backupDir, filename);
					}
					else if(k > 1){
						sprintf(tmpPath, "%s/%s/%s", backupDir, tmp3, filename);
					}
					else{
						fprintf(stderr, "What error is this?\n");
					}
				}
				// Open/Create backup file
				if((fd2 = open(tmpPath, O_CREAT | O_WRONLY | O_TRUNC, 0777)) < 0){
					fprintf(stderr, "Failed to open/create new file\n");
					close(fd1);
					exit(1);
				}
				//print "recovered to"
				printf("\"%s\" recovered to \"%s\"\n", current->backupPath, tmpPath);
				hasRecovered = true;
				//Add the recover log to ssubak.log
				BackUpLog(timeStamp, current->backupPath, tmpPath, 2);
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
			
			char* timeStampDir = (char *)malloc(sizeof(char *) * PATHMAX);
			getPathUpToTimeStamp(current->backupPath, timeStampDir);
			
			//remove file
			remove(current->backupPath);
			
			current->wasRemoved = true;
			current->hasChecked = true;
			
			//If directory is empty, remove
			RemoveEmptyDirRecursive(timeStampDir);
		}
	}
	return hasRecovered;
}

//
int isFileBackedUp_Recover(char *absolutePath, char* timeStamp, char* backupDir) {
	char *dirPath = (char *)malloc(sizeof(char *) * PATHMAX);
	
	bool hasBeenBackedUp = false;
	int checkidx = 0;
	int idx = 0;
	bool hasRecovered = false;
	
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
			/******************* NEED TO CHANGE LATER **************************/
			int doesMatch = sscanf(current->backupPath, "/home/backup/%[^/]/", timeStamp);
			if(doesMatch != 1) {
				printf("backuppath is probably wrong\n");
				exit(1);
			}
			sprintf(current->timeStamp, "%s", timeStamp);
		}
		//If current->oP's path_type is -1, it means it doesn't exist, so set current->oR to true
		else if(Path_Type(current->originalPath) == -1){
			current->originalRemoved = true;
			continue;
		}
	}
	
	if(hasBeenBackedUp == true) {
		for(LogNode *current = list->head; current != NULL; current = current->next) {
			//If timeStamp in linked list isn't empty, print timeStamp and file size
			if(strcmp(current->timeStamp, "") && current->wasRemoved != true && current->hasChecked != true){	
				
				if(strcmp(current->originalPath, absolutePath)) {
						continue;
				}
				checkidx++;
			}
		}
	}
	
	//No file exists, print error
	if(checkidx == 0) {
		bool printError = true;
		
		//If a backup file exists for this file, set printError to false
		for(LogNode *current = list->head; current != NULL; current = current->next) {
			if(!strcmp(current->originalPath, absolutePath)) {
				if(current->hasChecked == true){
					printError = false;
					return 1;
				}
			}
		}
		//No backup file exists for this file, print error
		if(printError == true)
			fprintf(stderr, "Error : No backup file exists for %s\n", absolutePath);
			
		//Set wasRemoved and hasChecked to true for the file we just checked.
		for(LogNode *current = list->head; current != NULL; current = current->next) {
				if(!strcmp(current->originalPath, absolutePath)) {
						current->wasRemoved= true;
						current->hasChecked = true;
				}
		}
		return 0;
	}
	
	//Only one backuped file exists, erase and print log
	else if(checkidx == 1) {
		hasRecovered = recoverFile(absolutePath, timeStamp, 0, filename, backupDir);
		hasBeenBackedUp = false;
	}
	
	//More than one backuped file exists, choose to recover
	//If hasBeenBackedUp is true, print absolutePath, timestamp, and corresponding file sizes.
	if(hasBeenBackedUp == true) {
		//Print options for recovery
		if(isRecoverMostRecent == false) {
			printf("backup files of %s\n", absolutePath);
			printf("0. exit\n");
			for(LogNode *current = list->head; current != NULL; current = current->next) {
				if(current->hasChecked == true)
					continue;
	
				//If original paths are different, continue		
				if(strcmp(current->originalPath, absolutePath)) 
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
					free(numWithCommas);
				}
			}
		}
		//-l option was used
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
				
				//If timeStamp in linked list isn't empty, print timeStamp and file size
				if(strcmp(current->timeStamp, "") && current->wasRemoved != true && current->hasChecked != true){
					idx++;
					current->tmpRemoveNum = idx;
					//current->hasChecked = true;
				}
			}
		}
		
		//Only get inputs if is recover most recent is false
		if(isRecoverMostRecent != true){
			printf(">> ");
			int num;
			scanf("%d", &num);
			//If num is 0, exit
			if(num == 0) { exit(0); }
			//If num is bigger than idx, print error message
			else if(num > idx) { fprintf(stderr, "Please input a correct number. Exiting...\n"); }
			//If num is one of the numbers printed out, erase the file
			else{
				hasRecovered = recoverFile(absolutePath, timeStamp, num, filename, backupDir);
			}
		}
		//Else recover latest backup file
		else {
			hasRecovered = recoverFile(absolutePath, timeStamp, idx, filename, backupDir);
		}
	}
	return hasRecovered;
}
