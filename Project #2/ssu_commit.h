bool hasCommit = false;

//Create directory recursively going down (used in recover.h)
void create_directory(const char* directory){
	char InnerDir[PATH_MAX];
	char *tmp = NULL;
	size_t len;
	
	sprintf(InnerDir, "%s", directory);
	len = strlen(InnerDir);
	
	//If last char is '/', remove it.
	if(InnerDir[len-1] == '/') {
		InnerDir[len - 1] = 0;
	}
	
	//With the string we have, keep going down and making directories.
	//Eg. With /home/junmoon/a/b/c -> make /a, then /a/b, then /a/b/c
	for(tmp = InnerDir + 1; *tmp; tmp++) {
		if(*tmp == '/') {
			*tmp = 0;
			mkdir(InnerDir, 0777);
			*tmp = '/';
		}
	}
	//Eg. The code block above makes only till /a/b, this line here makes the complete /a/b/c
	mkdir(directory, 0777);
}

//Function to return changed lines of two files.
int compare_files(FILE *file1, FILE *file2) {
    char line1[MAX_LINE_LENGTH];
    char line2[MAX_LINE_LENGTH];
    
    while (!feof(file1) || !feof(file2)) {
        if (fgets(line1, MAX_LINE_LENGTH, file1) == NULL) {  // End of file1
            while (fgets(line2, MAX_LINE_LENGTH, file2) != NULL) (insertions)++;
            break;
        }
        if (fgets(line2, MAX_LINE_LENGTH, file2) == NULL) {  // End of file2
            (deletions)++;
            while (fgets(line1, MAX_LINE_LENGTH, file1) != NULL) (deletions)++;
            break;
        }
        if (strcmp(line1, line2) != 0) {
            (deletions)++;
            (insertions)++;
        }
    }

    return 0;
}

//Function to sync original file and backup file's stat
int sync_file_attributes(const char *origin_path, const char *backup_path) {
    struct stat origin_stats;
    struct utimbuf new_times;

    // Get the file attributes from the original file
    if (stat(origin_path, &origin_stats) == -1) {
        perror("Failed to get file attributes");
        return -1; // Return an error if the operation fails
    }

    // Update the modification and access times of the backup file
    new_times.actime = origin_stats.st_atime; // Access time
    new_times.modtime = origin_stats.st_mtime; // Modification time

    if (utime(backup_path, &new_times) == -1) {
        perror("Failed to update file times");
        return -1; // Return an error if the operation fails
    }

    // Change the mode of the backup file to match the original file
    if (chmod(backup_path, origin_stats.st_mode) == -1) {
        perror("Failed to set file permissions");
        return -1; // Return an error if the operation fails
    }

    return 0; // Return success
}

// Function to backup file if new or modified, and write to commit log if new or modified or removed.
int backup_commit_file(command_parameter *parameter, char* origin_path, char* backup_path, char* time_str, int opt, FILE* file, char* origin_backup_path) {
  int fd1, fd2;
  FILE* fp1;
  FILE* fp2;
  int len;
  char buf[BUF_MAX*2];
  char backup_file_path[PATH_MAX*2];
  char temp_path[PATH_MAX] = "";
  char tmp_backupPath[PATH_MAX] = "";
  /*
  //check already backuped
  if(!(check_already_backuped(origin_path)) {
    return 0;
  }*/

  char *result = findDivergingPath(backup_path, origin_path);
  strcat(temp_path, "/");
  strcat(temp_path, result);
  strcpy(tmp_backupPath, repoPath);
  strcat(tmp_backupPath, "/");
  strcat(tmp_backupPath, parameter->filename);
  strcat(tmp_backupPath, temp_path);
  
  char tmp_dirname_backupPath[PATH_MAX];
  strcpy(tmp_dirname_backupPath, tmp_backupPath);
  strcpy(tmp_dirname_backupPath,dirname(tmp_dirname_backupPath));
  create_directory(tmp_dirname_backupPath);
  
  sprintf(backup_file_path, "%s/%s", tmp_dirname_backupPath, get_file_name(origin_path));
  //New file option
  if(opt == 0){
	  if((fd1 = open(origin_path, O_RDONLY)) == -1) {
	    fprintf(stderr, "ERROR: open error for '%s'\n", origin_path);
	    RemoveEmptyDirRecursive(tmp_dirname_backupPath);
	    return -1;
	  }
	  
	  if((fd2 = open(backup_file_path, O_WRONLY|O_CREAT|O_TRUNC, 0777)) == -1) {
	    printf("2\n");
	    fprintf(stderr, "ERROR: open error for '%s'\n", backup_file_path);
	    return -1;
	  }
	  
	  //copy file and increase insertions cnt
	  while(len = read(fd1, buf, BUF_MAX)) {
	    insertions += len;
	    write(fd2, buf, len);
	  }
	  
	  sync_file_attributes(origin_path, backup_file_path);
  
	  close(fd1);
	  close(fd2);
  }
  //Modified file option
  else if(opt== 1){
  	  fp1 = fopen(origin_path, "r");
	  fp2 = fopen(origin_backup_path, "r");
	  if(!fp1 || !fp2){
	  	fprintf(stderr, "Error opening files.\n");
	  	if(fp1) fclose(fp2);
	  	if(fp2) fclose(fp1);
	  	exit(1);
	  }
	  
	  //Get difference of lines (insertions, deletions) between two files
	  compare_files(fp1, fp2);
  
	  fclose(fp1);
	  fclose(fp2);
	  if((fd1 = open(origin_path, O_RDONLY)) == -1) {
	    fprintf(stderr, "ERROR: open error for '%s'\n", origin_path);
	    RemoveEmptyDirRecursive(tmp_dirname_backupPath);
	    return -1;
	  }
	  
	  if((fd2 = open(backup_file_path, O_WRONLY|O_CREAT|O_TRUNC, 0777)) == -1) {
	    printf("2\n");
	    fprintf(stderr, "ERROR: open error for '%s'\n", backup_file_path);
	    return -1;
	  }
	  
	  //copy file and increase insertions cnt
	  while(len = read(fd1, buf, BUF_MAX)) {
	    write(fd2, buf, len);
	  }
	  
	  sync_file_attributes(origin_path, backup_file_path);
	  close(fd1);
	  close(fd2);
  }
  
  if(opt == 0) //new
  	sprintf(buf, "commit: \"%s\" - new file: \"%s\"", get_file_name(backup_path), origin_path);
  else if(opt == 1) //modified
  	sprintf(buf, "commit: \"%s\" - modified: \"%s\"", get_file_name(backup_path), origin_path);
  else if(opt == 2) //original removed
  	sprintf(buf, "commit: \"%s\" - removed:  \"%s\"", get_file_name(backup_path), origin_path);
  	
  fprintf(file, "%s\n", buf);
  RemoveEmptyDirRecursive(tmp_dirname_backupPath);
  hasCommit = true;
  return 1;
}

//Function to execute backup_commit_file based on option
int commit_file(command_parameter *parameter, char* backupPATH, dirNode* dirList, int depth, int last_bit, FILE *file) {
  dirNode* curr_dir = dirList->subdir_head->next_dir;
  fileNode* curr_file = dirList->file_head->next_file;
  int ret = 0;
  struct stat path_stat;
  
  while(curr_file != NULL) {
    backupNode* curr_backup = curr_file->backup_head->next_backup;
    while(curr_backup != NULL) {
      char *temp_path = (char*)malloc(sizeof(char) * PATHMAX*2);
      char *get_cwd = (char*)malloc(sizeof(char) * PATHMAX);
      if(!strcmp(curr_backup->backup_path, "")){
        //new file
        if(curr_backup->wasRemoved == false){
        	ret = backup_commit_file(parameter, curr_backup->origin_path, backupPATH, "", 0, file, "");
        }
      }
      
      //removed file
      else if((stat(curr_backup->origin_path, &path_stat) != 0)){
      	ret = backup_commit_file(parameter, curr_backup->origin_path, backupPATH, "", 2, file, "");
      }
      
      //modified file
      else if(cmpHash(curr_backup->backup_path, curr_backup->origin_path)){
        ret = backup_commit_file(parameter, curr_backup->origin_path, backupPATH, "", 1, file, curr_backup->backup_path);
      }
      
      curr_backup = curr_backup->next_backup;
    }
    curr_file = curr_file->next_file;
  }
  
  while(curr_dir != NULL) {
    ret = commit_file(parameter, backupPATH, curr_dir, depth+1, last_bit, file);
    curr_dir = curr_dir->next_dir;
  }
  
  return ret;
}

//Function to execute commit command
int commit_process(command_parameter *parameter) {
  hasCommit = false;
  if(parameter->filename == NULL || !strcmp(parameter->filename,"")) {
    fprintf(stderr, "ERROR: <NAME> is not included\n");
    help(CMD_COMMIT);
  }
  
  else if(!strcmp(parameter->command, "commit")) {
    files_changed = insertions = deletions = 0;
    
    // File name length is bigger than 256 bytes
    if(strlen(parameter->filename) > FILE_MAX){
    	fprintf(stderr, "ERROR: <PATH> is too long\n");
        return -1;
    }
    
    struct dirent **namelist;
    DIR* dp;
    struct stat statbuf;
    int cnt = 0;
    char *tmppath = (char *)malloc(sizeof(char *) * PATHMAX);
    
    //cnt is number of files in absolutePath
    if((cnt = scandir(repoPath, &namelist, NULL, alphasort)) == -1) {
	fprintf(stderr, "ERROR: scandir error for %s\n", repoPath);
	exit(1);
    }
    //printf("parameter->filename: %s\n", parameter->filename);
    
    //Check each file in absolutePath
    for(int i = 0; i < cnt; i++) {
    	if(!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;
    	//printf("namelist[i]->d_name: %s\n", namelist[i]->d_name);
    	sprintf(tmppath, "%s/%s", repoPath, namelist[i]->d_name);

	if (lstat(tmppath, &statbuf) < 0) {
		fprintf(stderr, "ERROR: lstat error for %s\n", tmppath);
		exit(1);
	}
		
	if(S_ISDIR(statbuf.st_mode)) {
		if(!strcmp(namelist[i]->d_name, parameter->filename)){
			fprintf(stderr, "\"%s\" already exists in repo\n", parameter->filename);
			exit(1);
		}
	}
    }
    FILE *file = fopen(commit_log_PATH, "a");
    sprintf(tmppath, "%s/%s", repoPath, parameter->filename);
    mkdir(tmppath, 0777);
    print_status(backup_dir_list, 0, 0);
    commit_file(parameter, tmppath, backup_dir_list, 0, 0, file);

    if(hasCommit == false)
	    printf("Nothing to commit\n");
    else{
	printf("commit to \"%s\"\n", parameter->filename);
	printf("%d files changed, %d insertions(+), %d deletions(-)\n", files_changed, insertions, deletions);
    }
    if(strlen(buffer) != 0){
  	buffer[strlen(buffer)-1] = '\0';
  	printf("%s\n", buffer);
    }
    RemoveEmptyDirRecursive(tmppath);
  }
  
  return 0;
}
