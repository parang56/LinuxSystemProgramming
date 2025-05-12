#include "ssu_header.h"
#include "ssu_help.h"
#include "backup.h"
#include "remove.h"
#include "recover.h"

//Executes at start of main
void Init() {
	char *ssubakPath = (char *)malloc(sizeof(char *) * PATHMAX);
	
	//Create linked list
	list = createLogLinkedList();
	if(!list) {
		fprintf(stderr, "Failed to create backup log list\n");
		exit(1);
	}

	getcwd(exePATH, PATHMAX);
	sprintf(homePATH, "%s", getenv("HOME"));

	//******************************************************//
	sprintf(backupPATH, "/home/backup");
	//******************************************************//
  
	if (access(backupPATH, F_OK))
		mkdir(backupPATH, 0777);
	
	//Make path to ssubak.log
	sprintf(ssubakPath, "%s/%s", backupPATH, "ssubak.log");
	
	//Make log file
	FILE *file = fopen(ssubakPath, "a");
	fclose(file);
	
	//Make linked list from ssubak.log
	parseLogFile(ssubakPath, list);
}

int main(int argc, char* argv[]) {
	//Create backup folder, ssubak.log, and linked list for ssubak.log
  	Init();
  	struct stat path_stat;
  	int option;
  	bool caseD, caseR, caseY, caseA, caseL, caseN;
  	caseD = caseR = caseY = caseA = caseL = caseN = 0;
	char absolutePath[PATHMAX];
	char timeStamp[16];
	char *backupDir = (char *)malloc(sizeof(char *) * PATHMAX);
	char *recoverDir = (char *)malloc(sizeof(char *) * PATHMAX);
	char *backupPath = (char *)malloc(sizeof(char *) * PATHMAX);
	
	//Get current time
	time_t now = time(NULL);
	strftime(timeStamp, sizeof(timeStamp), "%y%m%d%H%M%S", localtime(&now));
	
	//If argument count is smaller than 2, print usage.
    	if (argc < 2) {
      		fprintf(stderr, "ERROR: wrong input.\n./ssu_backup help : show commands for program\n");
      		rmdir(backupPATH);
      		exit(1);
    	}
	
	//If argv[1] isn't one of the commands, print error.
 	if(strcmp(argv[1], "backup") && strcmp(argv[1], "remove") && strcmp(argv[1], "recover") && strcmp(argv[1], "list") && strcmp(argv[1], "help")) {
        	fprintf(stderr, "ERROR: invalid command -- '%s'\n/ssu_backup help : show commands for program\n", argv[1]);
        	rmdir(backupPATH);
        	exit(1);
   	}
    	
    	//realpath(argv[2], absolutePath) == NULL
    	//Check if path is file(0), directory(1), or no access or does not exist(-1)
	int type = Path_Type(argv[2]);
    
    	//If argv[1] is one of the commands, hash = the_command
	if(!strcmp(argv[1], "backup")) { hash = HASH_BACKUP; }
	else if(!strcmp(argv[1], "remove")) { hash = HASH_REMOVE; }
	else if(!strcmp(argv[1], "recover")) { hash = HASH_RECOVER; }
	else if(!strcmp(argv[1], "list")) { hash = HASH_LIST; }
	else if(!strcmp(argv[1], "help")) { hash = HASH_HELP; }
    
   	//"./ssu_backup backup" has been called
    	if(hash == HASH_BACKUP){
    		if(argv[2] == NULL){
			fprintf(stderr, "ERROR: missing operand <PATH>\n");
			exit(1);
		}
		
		//If path exceeds 4096 bytes, print error and exit
	    	if(strlen(argv[2]) > PATHMAX || strlen(absolutePath) > PATHMAX){
	    		fprintf(stderr, "ERROR: Input path is too long!\n");
	    		exit(1);
	    	}
		
		//If path is incorrect, print error and exit
		if(realpath(argv[2], absolutePath) == NULL) {
	    		fprintf(stderr, "ERROR: '%s' does not exist\n", absolutePath);
	    		exit(1);
	    	}
	    	
		//If not in user directory, print error
		if(strncmp(exePATH, absolutePath, strlen(exePATH)) != 0 || strncmp(backupPATH, absolutePath, strlen(backupPATH) != 0)) {
			fprintf(stderr, "ERROR: path must be in user directory\n - '%s' is not in user directory.\n", argv[2]);
			
			exit(1);
		}
		
		//Make backupDir
		backupDir = makeBackUpDir(timeStamp);
		
    		//Path is file so proceed with BackUpFile()
    		if(type == 0) { 
    			while((option = getopt(argc, argv, "dry")) != -1) {
    				//Option incorrect input
	    			if(option != 'y') {
					fprintf(stderr, "Usage: backup <filename> <-y>\nA file can only be used with the option <-y> or no option at all.\n");
					rmdir(backupDir);
					exit(1);
				}
				//Option was -y, set isForceBackup to true
				else{
					isForceBackup = true;
				}
			}

	    		BackUpFile(absolutePath, timeStamp, backupDir); 
	    		//Remove empty directories after removal
    			RemoveEmptyDirRecursive(backupDir);
    		}
    		
    		//Path is directory so check for options and then proceed
    		else if(type == 1) {
	    		if(argv[3] == NULL){
				fprintf(stderr, "ERROR: '%s' is a directory.\n - use '-d | -r' option or input file path.\n", absolutePath);
				rmdir(backupDir);
				exit(1);
			}
    			//check options using getopt
	    		while((option = getopt(argc, argv, "dry")) != -1) {
	    			if(option != 'd' && option != 'r' && option != 'y') {
					fprintf(stderr, "Usage : backup <directory> < -d | -r | -y >\n");
					rmdir(backupDir);
					exit(1);
				}
				switch(option){
					case 'd':
						isCheckDir = false;
						caseD = true;
						break;
					case 'r':
						isCheckDir = true;
						caseR = true;
						break;
					case 'y':
						caseY = true;
						isForceBackup = true;
						break;
					default :
						fprintf(stderr, "Usage : backup <directory> < -d | -r | -y >\n");
						rmdir(backupDir);
						exit(1);
				}
			}
			//If option -d and -r exists, set to -r (isCheckDir to true)
			if(caseD == true && caseR == true){
				isCheckDir = true;
			}
			
			//If input was directory but only had -y option, print error and exit
			else if(caseD == false && caseR == false && caseY == true){
				fprintf(stderr, "Usage : If directory backup, need < -d | -r > for -y option.\n");
				rmdir(backupDir);
				exit(1);
			}
			//Execute func BackUpDir
			BackUpDir(absolutePath, timeStamp, backupDir);
    		}
    		//Remove empty directories after backup
    		RemoveEmptyDirRecursive(backupDir);
    	}
    	
    	//"./ssu_backup remove" has been called
    	else if(hash == HASH_REMOVE){  
    		bool hasFile = false;
		bool hasDir = false;
		bool hasErased = false;
		bool hasErasedtmp = false;
		
		//If only ./ssu_backup remove -> print error message and exit
    		if(argv[2] == NULL){
			fprintf(stderr, "Usage : remove <directory> < -d | -r | -a >\n");
			exit(1);
		}

	    	//If path exceeds 4096 bytes, print error and exit
	    	if(strlen(argv[2]) > PATHMAX || strlen(absolutePath) > PATHMAX){
	    		fprintf(stderr, "ERROR: Input path is too long!\n");
	    		exit(1);
	    	}
	    	
	    	realpath(argv[2], absolutePath);
	    	//If not in user directory, print error
		if(strncmp(exePATH, absolutePath, strlen(exePATH)) != 0 || strncmp(backupPATH, absolutePath, strlen(backupPATH) != 0)) {
			fprintf(stderr, "ERROR: path must be in user directory\n - '%s' is not in user directory.\n", argv[2]);
			
			exit(1);
		}
		
		//In case original file/dir doesn't exist, create absolutePath again.
		char* temp = (char *)malloc(sizeof(char *) * PATHMAX);
		strcpy(temp, absolutePath);
		temp = basename(temp);
		char *found = strstr(argv[2], temp);
		char* subString = found + strlen(temp);
		strcat(absolutePath, subString);
		strcpy(temp, absolutePath);
		temp = dirname(temp);
		//Temporarily make the path leading to the absolutePath
		getFullPath(temp); 
		
		//Go through list, and if absolutePath was same as the backuped oP, set hasFile to true.
		for(LogNode *current = list->head; current != NULL; current = current->next) {
			if(!strcmp(current->originalPath, absolutePath)){
				hasFile = true;
				current->originalRemoved = true;
			}
		}
		
		//If hasFile wasn't true, this means aP was either directory or non-existent.
		//So go through list, and if absolutePath was same as the backuped oP, set hasDir to true.
		if(hasFile != true) {
			//If input is directory and original doesn't exist, but it is in the log
			for(LogNode *current = list->head; current != NULL; current = current->next) {
				if(strncmp(current->originalPath, absolutePath, strlen(absolutePath)) == 0){
					char* tmp = (char *)malloc(sizeof(char *) * PATHMAX);
					char* tmp2 = (char *)malloc(sizeof(char *) * PATHMAX);
					bool isDifferent = false;
					strcpy(tmp, current->originalPath);
					
					//If base path is not the same as aP
					//and goes down to '/', it is different.
					while(strcmp(dirname(tmp), absolutePath)){
						if(strlen(tmp) == 1){
							isDifferent = true;
							break;
						}
					}
					
					//If not different(same), compare it's original path with aP and if
					//they are different, it was removed.
					if(isDifferent == false){
						if(strcmp(current->originalPath, absolutePath)){
								current->originalRemoved = true;
								hasDir = true;
						}
					}
					//If different, set hasChecked to true so we don't check it again.
					else{
						current->hasChecked = true;
					}
				}
			}
		
		}
	    		
    		//Path is file so proceed with BackUpFile()
    		if(type == 0) { 
    			while((option = getopt(argc, argv, "a")) != -1) {
    				//Option incorrect input
	    			if(option != 'a') {
	    				fprintf(stderr, "Usage: remove <filename> <-a>\nA file can only be used with the option <-a> or no option at all.\n");
					exit(1);
				}
				//Option was -a, set isForceRemove to true
				else{
					isForceRemove = true;
				}
			}
			//Function to 1. Check backuped files, 2. Backup files
			hasErasedtmp = isFileBackedUp_Remove(absolutePath, timeStamp);
    		}
    		
    		//Path is directory or non-existent
    		else{
    			if(hasFile == true) {
	    			while((option = getopt(argc, argv, "a")) != -1) {
	    				//Option incorrect input
		    			if(option != 'a') {
		    				fprintf(stderr, "Usage: remove <filename> <-a>\nA file can only be used with the option <-a> or no option at all.\n");
						exit(1);
					}
					//Option was -a, set isForceRemove to true
					else{
						isForceRemove = true;
					}	
				}
				single = true;
				hasErasedtmp = isFileBackedUp_Remove(absolutePath, timeStamp);
	    		}
	    		
	    		if(hasDir == true){
	    			//DirNonExist = true;
	    			if(argv[3] == NULL){
	    				fprintf(stderr, "Usage : ./ssu_backup remove directory < -d | -r | -a > \n");
					exit(1);
	    			}
	    			//check options using getopt
		    		while((option = getopt(argc, argv, "dra")) != -1) {
		    			if(option != 'd' && option != 'r' && option != 'a') {
						fprintf(stderr, "Usage : ./ssu_backup remove directory < -d | -r | -a >\n");
						exit(1);
					}
					switch(option){
						case 'd':
							isCheckDir = false;
							caseD = 1;
							break;
						case 'r':
							isCheckDir = true;
							caseR = 1;
							break;
						case 'a':
							caseA = true;
							isForceRemove = true;
							break;
						default :
							fprintf(stderr, "Usage: remove <directory> <-d | -r | -a>\n");
							exit(1);
					}
				}
			
			
				//If option -d and -r exists, set to -r (isCheckDir to true)
				if(caseD == true && caseR == true){
					isCheckDir = true;
				}
				
				//Set conditions if isCheckDir (-d) option was used.
				for(LogNode *current = list->head; current != NULL; current = current->next) {
					if(current->originalRemoved == true && current->hasChecked == false){
						if(isCheckDir==false){
							char* tmp = (char *)malloc(sizeof(char *) * PATHMAX);
							bool isDifferent = false;
							strcpy(tmp, current->originalPath);
							
							if(strcmp(dirname(tmp), absolutePath) ){
								current->hasChecked = true;
								current->wasRemoved = true;
							}
						}
					}
				}
				int capacity = 10;
				char **paths = malloc(capacity * sizeof(char*));
				int i =0 ;
				
				//Get the paths we are going to remove.
				for(LogNode *current = list->head; current != NULL; current = current->next) {
					if(current->originalRemoved == true && current->hasChecked == false){
						if( (i+1) >= capacity){
							capacity *= 2;
							paths = realloc(paths, capacity * sizeof(char*));
							if(!paths){
								fprintf(stderr, "Memory allocation failed!\n");
								exit(1);
							}
						}
						paths[i++] = strdup(current->originalPath);
					}
				}
				
				//Sort so we can remove in a BFS like manner
				paths = removeDuplicatesAndBFS(paths, &i);
				
				for(int k=0; k < i; k++){
					hasErased = isFileBackedUp_Remove(paths[k], timeStamp);
					if(hasErased == true){
						hasErasedtmp = true;
					}
				}
		    	}
    		}
    		
    		//Nothing was removed at all, print error and exit
    		if(!hasErasedtmp) {
	    		fprintf(stderr, "No backup file or directory exists for the path: %s\n", absolutePath);
	    		exit(1);
	    	}
    		
    		//Remove empty files
    		RemoveEmptyDirRecursive(temp);
    	}
    	
    	//"./ssu_backup recover" has been called
    	else if(hash == HASH_RECOVER){
    		bool hasFile = 0;
		bool hasDir = 0; 
		bool hasErased = false;
		bool hasErasedtmp = false;
    		if(argv[2] == NULL){
			fprintf(stderr, "Usage : recover <directory> < -d | -r | -l | -n >\n");
			exit(1);
		}
		
		//If path exceeds 4096 bytes, print error and exit
	    	if(strlen(argv[2]) > PATHMAX || strlen(absolutePath) > PATHMAX){
	    		fprintf(stderr, "ERROR: Input path is too long!\n");
	    		exit(1);
	    	}
		
		realpath(argv[2], absolutePath);
	    	//If not in user directory, print error
		if(strncmp(exePATH, absolutePath, strlen(exePATH)) != 0 || strncmp(backupPATH, absolutePath, strlen(backupPATH) != 0)) {
			fprintf(stderr, "ERROR: path must be in user directory\n - '%s' is not in user directory.\n", argv[2]);
			
			exit(1);
		}
		
		//In case original file/dir doesn't exist, create absolutePath again.
		char* temp = (char *)malloc(sizeof(char *) * PATHMAX);
		strcpy(temp, absolutePath);
		temp = basename(temp);
		char *found = strstr(argv[2], temp);
		char* subString = found + strlen(temp);
		strcat(absolutePath, subString);
		strcpy(temp, absolutePath);
		temp = dirname(temp);
		getFullPath(temp);
		
	    	//If the original file doesn't exist, but it is in the log -> erase from backup folder
		for(LogNode *current = list->head; current != NULL; current = current->next) {
			if(!strcmp(current->originalPath, absolutePath)){
				hasFile = true;
			}
		}
		if(hasFile != true){
			//If input is directory and original doesn't exist, but it is in the log
			for(LogNode *current = list->head; current != NULL; current = current->next) {
				if(strncmp(current->originalPath, absolutePath, strlen(absolutePath)) == 0){
					char* tmp = (char *)malloc(sizeof(char *) * PATHMAX);
					bool isDifferent = false;
					strcpy(tmp, current->originalPath);
					
					//While shortening string, if never same and tmp's strnlen goes to 1,
					//the two strings are different so set isDifferent to true
					while(strcmp(dirname(tmp), absolutePath)){
						if(strlen(tmp) == 1){
							isDifferent = true;
							break;
						}
					}
					
					//If not different, compare it's original path with aP and if
					//they are different, it was removed.
					if(isDifferent == false){
						if(strcmp(current->originalPath, absolutePath)){
								current->originalRemoved = true;
								hasDir = true;
						}
					}
					//If isDifferent was true, we checked it so set current->hC to true.
					else{
						current->hasChecked = true;
					}
				}
			}
		}
		
		//Set hasDir to false if hasFile was true
		if(hasFile == true){
			hasDir = false;
		}
		
		//If no instance of the path exists, print error and exit
		if(!hasFile && !hasDir) {
	    		fprintf(stderr, "No file exists at '%s' and no backup file exists for this file.\n", absolutePath);
	    		exit(1);
	    	}
    		//Path is file so proceed with BackUpFile()
    		if(type == 0) { 
    			while((option = getopt(argc, argv, "ln:")) != -1) {
    				//Option incorrect input
	    			if(option != 'l' && option != 'n') {
	    				fprintf(stderr, "Usage : ./ssu_backup recover filename < -l | -n > < new path>\n");
					exit(1);
				}
				switch(option) {
					case 'l':
						caseL = true;
						isRecoverMostRecent = true;
						break;
					case 'n':
						isRecoverToNew = true;
						
						//Make New Directory and get full path to new directory
						realpath(argv[argc-1], recoverDir);
					    	char *filetmpname = getFileName(recoverDir);
						char *tmp = extractAfterName(argv[argc-1], filetmpname);
						char *fullPath = (char *)malloc(sizeof(char *) * PATHMAX);
						sprintf(fullPath, "%s/%s", recoverDir, tmp);
					    	create_directory(fullPath);
					    	
					    	size_t length = strlen(fullPath);
					    	if(fullPath[length -1] == '/')
					    		fullPath[length-1] = '\0';
					    	
					    	newPath = fullPath;
						break;
				}
			}
			//If newPath has incorrect path, print error and exit.
			if(isRecoverToNew == true){
				if(strncmp(exePATH, newPath, strlen(exePATH)) != 0 || strncmp(backupPATH, newPath, strlen(backupPATH) != 0)) {
					fprintf(stderr, "ERROR: path must be in user directory\n - '%s' is not in user directory.\n", newPath);
					
					exit(1);
				}
			}
			//Run isFileBackedUp_Recover with the bools and options
			single = true;
			hasErasedtmp = isFileBackedUp_Recover(absolutePath, timeStamp, newPath);
    		}
    		
    		//Type was 0 or -1.
    		else {
    			//hasFile was set to true going through list
    			if(hasFile == true) {
    				//Set bools according to the options
	    			while((option = getopt(argc, argv, "ln:")) != -1) {
	    				//Option incorrect input
		    			if(option != 'l' && option != 'n') {
		    				fprintf(stderr, "Usage : ./ssu_backup recover filename < -l | -n > < new path>\n");
		    				help_recover();
						exit(1);
					}
					switch(option) {
						case 'l':
							caseL = true;
							isRecoverMostRecent = true;
							break;
						case 'n':
							isRecoverToNew = true;
							isOriginalExist = false;
							
							//Make New Directory and get full path to new directory
						    	newPath = getFullPath(argv[argc-1]);
							break;
					}
				}
				
				if(isRecoverToNew == true){
					//If newPath has incorrect path, print error and exit.
					if(strncmp(exePATH, newPath, strlen(exePATH)) != 0 || strncmp(backupPATH, newPath, strlen(backupPATH) != 0)) {
						fprintf(stderr, "ERROR: path must be in user directory\n - '%s' is not in user directory.\n", newPath);
						
						exit(1);
					}
				}
				
				//Run isFileBackedUp_Recover with the bools and options
				single = true;
				hasErasedtmp = isFileBackedUp_Recover(absolutePath, timeStamp, newPath);
	    		}
	    		//hasDir was set to true going through list
	    		if(hasDir == true){
	    			DirNonExist = true;
	    			
	    			//Error if argv[3] is null
	    			if(argv[3] == NULL){
	    				fprintf(stderr, "Usage : ./ssu_backup recover directory < -d | -r | -l | -n > < new path>\n");
					exit(1);
	    			}
	    			//check options using getopt
		    		while((option = getopt(argc, argv, "drln:")) != -1) {
		    			if(option != 'd' && option != 'r' && option != 'l' && option != 'n') {
						fprintf(stderr, "Usage : ./ssu_backup recover directory < -d | -r | -l | -n > < new path>\n");
						exit(1);
					}
					switch(option){
						case 'd':
							isCheckDir = false;
							caseD = true;
							break;
						case 'r':
							isCheckDir = true;
							caseR = true;
							break;
						case 'l':
							caseL = true;
							isRecoverMostRecent = true;
							break;
						case 'n':
							caseN = true;
							isRecoverToNew = true;
							//Make New Directory and get full path to new directory
						    	newPath = getFullPath(argv[argc-1]);
							break;
						default :
							fprintf(stderr, "Usage : ./ssu_backup recover directory < -d | -r | -l | -n > < new path>\n");
							exit(1);
					}
				}
				//original path doesn't exist, so we have to act like we're backing up to new path.
				if(caseN == false) {
					if(type == -1)
						isRecoverToNew = true;
					isOriginalNew = true;
					char *swap = (char *)malloc(sizeof(char *) * PATHMAX);
					strcpy(swap, absolutePath);
				    	newPath = getFullPath(swap);
			    	}
			    	
			    	if(isRecoverToNew == true){
				    	//If newPath has incorrect path, print error and exit.
				    	if(strncmp(exePATH, newPath, strlen(exePATH)) != 0 || strncmp(backupPATH, newPath, strlen(backupPATH) != 0)) {
						fprintf(stderr, "ERROR: path must be in user directory\n - '%s' is not in user directory.\n", newPath);
						
						exit(1);
					}
				}
			    	
				//No -d or -r but -n exists, print error and exit
				if(caseN == true && caseD == false && caseR == false){
					RemoveEmptyDirRecursive(newPath);
					fprintf(stderr, "Error : Need -d or -r for -n\n");
					exit(1);	
				}
				
				//If option -d and -r exists, set to -r (isCheckDir to true)
				if(caseD == true && caseR == true){
					isCheckDir = true;
				}
				
				//Set conditions if isCheckDir (-d) option was used.
				for(LogNode *current = list->head; current != NULL; current = current->next) {
					if(current->originalRemoved == true && current->hasChecked == false){
						if(isCheckDir==false){
							char* tmp = (char *)malloc(sizeof(char *) * PATHMAX);
							bool isDifferent = false;
							strcpy(tmp, current->originalPath);
							if(strcmp(dirname(tmp), absolutePath)){
								current->hasChecked = true;
								current->wasRemoved = true;
							}
						}
					}
				}
				
				//Get the paths we are going to remove.
				int capacity = 10;
				char **paths = malloc(capacity * sizeof(char*));
				int i =0 ;
				for(LogNode *current = list->head; current != NULL; current = current->next) {
					if(current->originalRemoved == true && current->hasChecked == false){
						if( (i+1) >= capacity){
							capacity *= 2;
							paths = realloc(paths, capacity * sizeof(char*));
							if(!paths){
								fprintf(stderr, "Memory allocation failed!\n");
								exit(1);
							}
						}
						paths[i++] = strdup(current->originalPath);
					}
				}
				
				//Sort so we can remove in a BFS like manner
				paths = removeDuplicatesAndBFS(paths, &i);
				
				for(int k=0; k < i; k++){
					hasErased = isFileBackedUp_Recover(paths[k], timeStamp, newPath);
					if(hasErased == true){
						hasErasedtmp = true;
					}
				}
				
	    		}
    		}
    		
    		//If no file recovered at all, print error and exit
    		if(!hasErasedtmp) {
	    		fprintf(stderr, "No file exists at '%s' and no backup file exists for this file.\n", absolutePath);
	    		exit(1);
	    	}
	    	
	    	//Remove empty files.
    		RemoveEmptyDirRecursive(temp);
    	}
    	
    	//"./ssu_backup list" has been called
    	else if(hash == HASH_LIST){
   		fprintf(stderr, "This part has not been completed!\n");
   		exit(1);
    	}
    	
    	//If hash was help, print out help	
    	else if(hash == HASH_HELP){
	    	if(argv[2] == NULL) help();
		else if(!strcmp(argv[2], "backup")) help_backup();
		else if(!strcmp(argv[2], "remove")) help_remove();
		else if(!strcmp(argv[2], "recover")) help_recover();
		else if(!strcmp(argv[2], "list")) help_list();
		else { 
			fprintf(stderr, "Incorrect usage : help <backup | remove | recover | list>\n");
			exit(1); 
		}
    	}
    	
        exit(0);
}
