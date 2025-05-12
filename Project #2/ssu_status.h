//Function to check tree if the path exists in tree
int check_tree(char* path) {
  char* tmp_path = (char*)malloc(sizeof(char)*PATH_MAX);
  char* ptr;
  char* tmp = (char *)malloc(sizeof(char *) * PATHMAX);
  dirNode* curr_dir;
  fileNode* curr_file;
  backupNode* curr_backup;

  strcpy(tmp_path, path);
  curr_dir = backup_dir_list->subdir_head;
  
  while((ptr = strchr(tmp_path, '/')) != NULL) {
    char* dir_name = substr(tmp_path, 0, strlen(tmp_path) - strlen(ptr));
    
    while(curr_dir->next_dir != NULL) {
      if(!strcmp(dir_name, curr_dir->next_dir->dir_name)) {
        break;
      }
      
      curr_dir = curr_dir->next_dir;
    }
    if(curr_dir->next_dir == NULL) {
      return 0;
    }
    
    tmp_path = ptr + 1;
    if(strchr(tmp_path, '/')) {
      curr_dir = curr_dir->next_dir->subdir_head;
    }
  }
  
  curr_file = curr_dir->next_dir->file_head;
  
  while(curr_file->next_file != NULL) {
    if(!strcmp(tmp_path, curr_file->next_file->file_name)) {
      break;
    }
    if(curr_file->next_file != NULL)
    	//tmp = curr_file->next_file->file_path;
    	curr_file = curr_file->next_file;
  }

  if(curr_file->next_file == NULL) {
    return 0;
  }

  curr_backup = curr_file->next_file->backup_head;
  
  while(curr_backup->next_backup != NULL) {
    if(!strcmp(path, curr_backup->next_backup->origin_path)) {
    	return 1;
    }
    curr_backup = curr_backup->next_backup;
  }
  return 0;
}

//Function to check and print for untracked files
void untracked_files(char* path) {
	struct dirent **namelist;
	DIR* dp;
	struct stat statbuf;
	int cnt = 0;
	char *tmppath = (char *)malloc(sizeof(char *) * PATHMAX);

	//cnt is number of files in exePATH
	if((cnt = scandir(path, &namelist, NULL, alphasort)) == -1) {
		fprintf(stderr, "ERROR: scandir error for %s\n", path);
		exit(1);
	}

	//Check each file in exePATH
	for(int i = 0; i < cnt; i++) {
		if(!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;
		//printf("namelist[i]->d_name: %s\n", namelist[i]->d_name);
		sprintf(tmppath, "%s/%s", path, namelist[i]->d_name);

		if (lstat(tmppath, &statbuf) < 0) {
			fprintf(stderr, "ERROR: lstat error for %s\n", tmppath);
			exit(1);
		}
		
		if(S_ISREG(statbuf.st_mode)){
			if(check_tree(tmppath) == 0){
				
				// Do we exclude .c, .h, .o and other non text files?
				// They were not excluded for this program
				/*
				char *ext_C;
				char *ext_H;
				char *ext_O;
				ext_C = strstr(tmppath, ".c");
				ext_H = strstr(tmppath, ".h");
				ext_O = strstr(tmppath, ".o");
				if((ext_C && strcmp(ext_C, ".c") == 0) || (ext_H && strcmp(ext_H, ".h") == 0) || (ext_O && strcmp(ext_O, ".o") == 0))
					continue;
				if(!strcmp(namelist[i]->d_name, "Makefile") || !strcmp(namelist[i]->d_name, "ssu_repo"))
					continue;
				*/
				
				char *temp_path = (char*)malloc(sizeof(char) * PATHMAX*2);
				char *relative_path = (char*)malloc(sizeof(char) * PATHMAX);
				char *get_cwd = (char*)malloc(sizeof(char) * PATHMAX);
				strcpy(temp_path, "  new file: ");
				get_cwd = getcwd(NULL, PATHMAX);
				make_relative_path(tmppath, get_cwd, relative_path);
				sprintf(temp_path, "%s\"%s\"\n", temp_path, relative_path);
				strcat(untracked_buffer, temp_path);
			}
		}
		else if(S_ISDIR(statbuf.st_mode)) {
			if(strcmp(tmppath, repoPath))
				untracked_files(tmppath);
		}
	}
}

//Function that prints files to be committed
void print_status(dirNode* dirList, int depth, int last_bit) {
  dirNode* curr_dir = dirList->subdir_head->next_dir;
  fileNode* curr_file = dirList->file_head->next_file;
  
  while(curr_dir != NULL) {
    last_bit |= (curr_dir->next_dir == NULL)?(1 << depth):0;
    
    print_status(curr_dir, depth+1, last_bit);
    curr_dir = curr_dir->next_dir;
  }
  
  while(curr_file != NULL) {
    last_bit |= (curr_file->next_file == NULL)?(1 << depth):0;
    
    backupNode* curr_backup = curr_file->backup_head->next_backup;
    while(curr_backup != NULL) {
      char *temp_path = (char*)malloc(sizeof(char) * PATHMAX*2);
      char *relative_path = (char*)malloc(sizeof(char) * PATHMAX);
      char *get_cwd = (char*)malloc(sizeof(char) * PATHMAX);
      struct stat path_stat;
      
      if(curr_backup->wasRemoved != true){
      
	      //backup path is empty -> never been committed
	      if(!strcmp(curr_backup->backup_path, "")){
		strcpy(temp_path, "  new file: ");
		get_cwd = getcwd(NULL, PATHMAX);
		make_relative_path(curr_backup->origin_path, get_cwd, relative_path);
		sprintf(temp_path, "%s\"%s\"\n", temp_path, relative_path);
	      	prepend_to_buffer(buffer, temp_path);
	      }
	      
	      //Original file removed -> removed file
	      else if((stat(curr_backup->origin_path, &path_stat) != 0)){
	      	strcpy(temp_path, "  removed:  ");
		get_cwd = getcwd(NULL, PATHMAX);
		make_relative_path(curr_backup->origin_path, get_cwd, relative_path);
		sprintf(temp_path, "%s\"%s\"\n", temp_path, relative_path);
	      	prepend_to_buffer(buffer, temp_path);
	      }
	      
	      //Different hash -> modified file
	      else if(cmpHash(curr_backup->backup_path, curr_backup->origin_path)){
	      	strcpy(temp_path, "  modified: ");
		get_cwd = getcwd(NULL, PATHMAX);
		make_relative_path(curr_backup->origin_path, get_cwd, relative_path);
		sprintf(temp_path, "%s\"%s\"\n", temp_path, relative_path);
	      	prepend_to_buffer(buffer, temp_path);
	      }
	      
	      files_changed++;
      }
      
      curr_backup = curr_backup->next_backup;
    }
    curr_file = curr_file->next_file;
  }
}

//Function to execute status command
int status_process(command_parameter *parameter) {
  /*
  if(parameter->filename != NULL || strcmp(parameter->filename,"")) {
    fprintf(stderr, "Wrong usage of status\n");
    help(CMD_STATUS);
    exit(1);
  }*/
  
  print_status(backup_dir_list, 0, 0);
  untracked_files(exePATH);
  
  if(strlen(buffer) != 0){
  	buffer[strlen(buffer)-1] = '\0';
  	printf("Changes to be committed:\n");
  	printf("%s\n\n", buffer);
  }else{
  	printf("Nothing to commit\n\n");
  }
  
  if(strlen(untracked_buffer) != 0){
  	untracked_buffer[strlen(untracked_buffer)-1] = '\0';
  	printf("Untracked files:\n");
  	printf("%s\n", untracked_buffer);
  }else{
  	//printf("Nothing to commit\n");
  }

  return 0;
}
