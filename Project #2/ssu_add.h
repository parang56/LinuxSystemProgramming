int add_file(char* origin_path) {
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
   
   //Path is directory
   if(S_ISDIR(path_stat.st_mode)){
	//check already backuped
  	if(check_already_backedup_dir(full_path, "", "", 0, 0)) {
  		printf("\"%s\" already exist in staging area\n", relative_path);
        	return 0;
  	}
   }
   //Path is file
   else if(S_ISREG(path_stat.st_mode)){ 
  	//check already backuped
  	if(check_already_backuped(full_path)) {
  		printf("\"%s\" already exist in staging area\n", relative_path);
        	return 0;
  	}
  }
  fclose(file);


  file = fopen(staging_log_PATH, "a");
  if((file == NULL)) {
     fprintf(stderr, "ERROR: open error for '%s'\n", staging_log_PATH);
     return -1;
  }
  
  //Add to staging log
  sprintf(buf, "add \"%s\"", full_path);
  fprintf(file, "%s\n", buf);
  
  fclose(file);
  
  //Print to terminal
  relative_path = convertToRelativePath(pwd_path, full_path);
  printf("add \"%s\"\n", relative_path);
  
  //Update the linked list tree
  if(init() == -1) {
    	fprintf(stderr, "ERROR: init error.\n");
    	return -1;
   }
  
  return 1;
}

//Function to execute add command
int add_process(command_parameter *parameter) {
  struct stat statbuf;
  if(parameter->filename == NULL || !strcmp(parameter->filename,"")) {
    fprintf(stderr, "ERROR: <PATH> is not included\n");
    help(CMD_ADD);
  }
  
  else if(!strcmp(parameter->command, "add")) {
    if((full_path = cvt_path_2_realpath(parameter->filename)) == NULL) {
        fprintf(stderr, "ERROR: '%s' is wrong path\n", parameter->filename);
        exit(1);
    }
    //Not normal file or directory
    if(Path_Type(full_path) == -1){
    	fprintf(stderr, "ERROR: '%s' is wrong path\n", parameter->filename);
    	exit(1);
    }
    
    // Path length is bigger than 4096 bytes -> error
    if(strlen(full_path) > PATHMAX){
    	fprintf(stderr, "ERROR: <PATH> is too long\n");
        exit(1);
    }
    
    //If no access to path exit (e.g. .repo or root)
    if(check_path_access(full_path)) {
    	exit(1);
    }
    
    //If file doesn't exist
    if(access(full_path, F_OK) == -1) {
      fprintf(stderr, "ERROR: '%s' is wrong path\n", parameter->filename);
      exit(1);
    }
    
    //Add path to staging log
    add_file(full_path);
  }

  return 0;
}
