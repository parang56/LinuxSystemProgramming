int remove_file(char* origin_path) {
  int fd1, fd2;
  int len;
  char *buf = (char *)malloc(sizeof(char *) * BUF_MAX);
  char* relative_path = (char *)malloc(sizeof(char *) * PATHMAX);
  struct stat path_stat;
  relative_path = convertToRelativePath(pwd_path, full_path);
   
   FILE *file = fopen(staging_log_PATH, "r");
   if((file == NULL)) {
     fprintf(stderr, "ERROR: open error for '%s'\n", staging_log_PATH);
     return -1;
   }
   
  stat(full_path, &path_stat);
   
   if(S_ISDIR(path_stat.st_mode)){
	//check already backuped
  	if(check_already_removed_dir(full_path, "", "", 0, 0)) {
  		printf("\"%s\" already removed from staging area\n", relative_path);
        	return 0;
  	}
   }
   else { 
  	//check already removed
	if(check_already_removed(full_path)) {
		return 0;
	}
  }
  
  fclose(file);

   file = fopen(staging_log_PATH, "a");
   if((file == NULL)) {
     fprintf(stderr, "ERROR: open error for '%s'\n", staging_log_PATH);
     return -1;
   }
  
  sprintf(buf, "remove \"%s\"", full_path);
  fprintf(file, "%s\n", buf);
  fclose(file);
  relative_path = convertToRelativePath(pwd_path, full_path);
  
  //Print to terminal
  printf("remove \"%s\"\n", relative_path);
  
  //Update tree
  if(init() == -1) {
    	fprintf(stderr, "ERROR: init error.\n");
    	return -1;
   }
  
  return 1;
}

//Function to execute remove command
int remove_process(command_parameter *parameter) {
  if(parameter->filename == NULL || !strcmp(parameter->filename,"")) {
    fprintf(stderr, "ERROR: <PATH> is not included\n");
    help(CMD_REMOVE);
  }
  else if(!strcmp(parameter->command, "remove")) {
    if((full_path = cvt_path_2_realpath(parameter->filename)) == NULL) {
        fprintf(stderr, "ERROR: '%s' is wrong path\n", parameter->filename);
        return -1;
    }
    //Not normal file or directory
    if(Path_Type(full_path) == -1){
    	fprintf(stderr, "ERROR: '%s' is wrong path\n", parameter->filename);
    	exit(1);
    }
    if(strlen(full_path) > PATHMAX){
    	fprintf(stderr, "ERROR: <PATH> is too long\n");
        return -1;
    }
    if(check_path_access(full_path)) {
    	return -1;
    } 
    if(access(full_path, F_OK)) {
      fprintf(stderr, "ERROR: '%s' is wrong path\n", parameter->filename);
      return -1;
    }	
    remove_file(full_path);
  } 

  return 0;
}
