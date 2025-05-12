int print_log(char *cmp_path) {
  int log_fd;
  int len;
  char buf[BUF_MAX];
  char *backup_time;
  char *backup_path = (char*)malloc(sizeof(char) * PATHMAX);
  char *origin_path = (char*)malloc(sizeof(char) * PATHMAX);
  char *temp_path = (char*)malloc(sizeof(char) * PATHMAX);
  
  if((log_fd = open(commit_log_PATH, O_RDONLY)) == -1) {
    fprintf(stderr, "ERROR: open error for '%s'\n", commit_log_PATH);
    return -1;
  }
  
  while(len = read(log_fd, buf, BUF_MAX)) {
    char *ptr = strchr(buf, '\n');
    ptr[0] = '\0';
    struct stat path_stat;
    lseek(log_fd, -(len-strlen(buf))+1, SEEK_CUR);
    if((strstr(buf, COMMIT_SEP)) != NULL) {
      if(sscanf(buf, "commit: \"%[^\"]\" - new file: \"%[^\"]\"", backup_path, origin_path) == 2) {
		
      		
      		if(S_ISDIR(path_stat.st_mode)){
      		
      		}
      		else{
      			
      		}
      }
    }
  }
}

//Print all logs
int print_log_all() {
  int log_fd;
  int len;
  char buf[BUF_MAX];
  char *backup_time;
  char *backup_path = (char*)malloc(sizeof(char) * PATHMAX);
  char *origin_path = (char*)malloc(sizeof(char) * PATHMAX);
  
  if((log_fd = open(commit_log_PATH, O_RDONLY)) == -1) {
    fprintf(stderr, "ERROR: open error for '%s'\n", commit_log_PATH);
    return -1;
  }
  
  while(len = read(log_fd, buf, BUF_MAX)) {
    char buffer_temp[BUF_MAX];
    char *temp_path = (char*)malloc(sizeof(char) * PATHMAX);
    char *temp_path2 = (char*)malloc(sizeof(char) * PATHMAX);
    char *ptr = strchr(buf, '\n');
    ptr[0] = '\0';
    struct stat path_stat;
    lseek(log_fd, -(len-strlen(buf))+1, SEEK_CUR);
    if((strstr(buf, COMMIT_SEP)) != NULL) {
      if(buffer_temp == NULL || !strcmp(buffer_temp, "")){
	      if(sscanf(buf, "commit: \"%[^\"]\" - new file: \"%[^\"]\"", backup_path, origin_path) == 2) {
		 strcat(temp_path, "commit: \"");
		 strcat(temp_path, backup_path);
		 strcat(temp_path, "\"");
		 strcat(buffer_temp, temp_path);
		 strcat(temp_path2, " - new file: \"");
		 strcat(temp_path2, origin_path);
		 strcat(temp_path2, "\"");
		 strcat(buffer_temp, temp_path2);
	      }
      }
      else{
       	      if(sscanf(buf, "commit: \"%[^\"]\" - new file: \"%[^\"]\"", backup_path, origin_path) == 2){
		 strcat(temp_path, " - new file: \"");
		 strcat(temp_path, origin_path);
		 strcat(temp_path, "\"\n");
		 strcat(buffer_temp, temp_path);
	      }
      }
    }
    if(buffer_temp != NULL || strcmp(buffer_temp, ""))
	    printf("%s\n", buffer_temp);
  }
}

int log_process(command_parameter *parameter) {

  // print everything
  if(parameter->filename == NULL || !strcmp(parameter->filename,"")) {
    print_log_all();
  }
  // print specified
  else if(!strcmp(parameter->command, "log")) {
    if((full_path = cvt_path_2_realpath(parameter->filename)) == NULL) {
        fprintf(stderr, "ERROR: '%s' is wrong path\n", parameter->filename);
        return -1;
    }
    // File name length is bigger than 256 bytes
    if(strlen(parameter->filename) > FILE_MAX){
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
    
    print_log(full_path);
  }

  return 0;
}
