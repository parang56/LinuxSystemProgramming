//Function to insert path into list
int backup_list_insert(dirNode* dirList, char* backup_time, char* path, char* backup_path) {
  char* ptr;
  if(ptr = strchr(path, '/')) {
    char* dir_name = substr(path, 0, strlen(path) - strlen(ptr));
    
    if(check_path_access2(dirList->dir_path) != -1){
    	dirNode* curr_dir = dirNode_insert(dirList->subdir_head, dir_name, dirList->dir_path);
    	backup_list_insert(curr_dir, backup_time, ptr+1, backup_path);
    	curr_dir->backup_cnt++;
    }

  } else {
    char* file_name = path;
    if(check_path_access2(dirList->dir_path) != -1){
    	fileNode* curr_file = fileNode_insert(dirList->file_head, file_name, dirList->dir_path);
    	backupNode_insert(curr_file->backup_head, backup_time, file_name, dirList->dir_path, backup_path);
    }
  }

  return 0;
}

//Function to remove(untrack) path into list
int backup_list_remove(dirNode* dirList, char* backup_time, char* path, char* backup_path) {
  char* ptr;
  if(ptr = strchr(path, '/')) {
    char* dir_name = substr(path, 0, strlen(path) - strlen(ptr));
    dirNode* curr_dir = dirNode_insert(dirList->subdir_head, dir_name, dirList->dir_path);
    backup_list_remove(curr_dir, backup_time, ptr+1, backup_path);
    curr_dir->backup_cnt++;

  } else {
    char* file_name = path;
    fileNode* curr_file = fileNode_insert(dirList->file_head, file_name, dirList->dir_path);
    backupNode_remove(curr_file->backup_head, backup_time, file_name, dirList->dir_path, backup_path);
  }

  return 0;
}

//Function used in print_list to print depth of tree
void print_depth(int depth, int is_last_bit) {
  for(int i = 1; i <= depth; i++) {
    if(i == depth) {
      if((1 << depth) & is_last_bit) {
          printf("┗ ");
      } else {
          printf("┣ ");
      }
      break;
    }
    if((1 << i) & is_last_bit) {
      printf("  ");
    } else {
      printf("┃ ");
    }
  }
}

//Function used to print tree in tree structure
void print_list(dirNode* dirList, int depth, int last_bit) {
  dirNode* curr_dir = dirList->subdir_head->next_dir;
  fileNode* curr_file = dirList->file_head->next_file;
 
  while(curr_dir != NULL) {
    last_bit |= (curr_dir->next_dir == NULL)?(1 << depth):0;
    
    print_depth(depth, last_bit);
    //printf("%s/ %d %d %d\n", curr_dir->dir_name, curr_dir->file_cnt, curr_dir->subdir_cnt, curr_dir->backup_cnt);
    printf("%s/\n", curr_dir->dir_name);
    print_list(curr_dir, depth+1, last_bit);
    curr_dir = curr_dir->next_dir;
  }
  
  while(curr_file != NULL) {
    last_bit |= (curr_file->next_file == NULL)?(1 << depth):0;

    print_depth(depth, last_bit);
    //printf("%s %d\n", curr_file->file_name, curr_file->backup_cnt);
    printf("%s\n", curr_file->file_name);
    
    backupNode* curr_backup = curr_file->backup_head->next_backup;
    while(curr_backup != NULL) {
      print_depth(depth+1, (
        last_bit | ((curr_backup->next_backup == NULL)?(1 << depth+1):0)
        ));
      printf("wR: %d, bp: %s\n", curr_backup->wasRemoved, curr_backup->backup_path);
      curr_backup = curr_backup->next_backup;
    }
    curr_file = curr_file->next_file;
  }
}

//Function to init tree list from staging log
int init_backup_list(int log_fd) {
  int len;
  char buf[BUF_MAX];

  char *backup_time,  *backup_path;
  char *origin_path = (char*)malloc(sizeof(char) * PATHMAX);
  
  struct stat path_stat;
  

  while(len = read(log_fd, buf, BUF_MAX)) {
    char *ptr = strchr(buf, '\n');
    ptr[0] = '\0';
    lseek(log_fd, -(len-strlen(buf))+1, SEEK_CUR);

    if((ptr = strstr(buf, ADD_SEP)) != NULL) {
    
      if(sscanf(buf, "add \"%[^\"]\"", origin_path) == 1) {
      		stat(origin_path, &path_stat);
      		if(S_ISDIR(path_stat.st_mode)){
      			backup_dir(origin_path, "", "", 0,0, backup_dir_list);
      		}
      		else{
      			backup_list_insert(backup_dir_list, "", origin_path, "");
      		}
      }
    }
    
    if((ptr = strstr(buf, REMOVE_SEP)) != NULL) {
	if(sscanf(buf, "remove \"%[^\"]\"", origin_path) == 1) {
		stat(origin_path, &path_stat);
      		if(S_ISDIR(path_stat.st_mode)){
      			remove_dir(origin_path, "", "", 0,0);
      			//backup_list_remove(backup_dir_list, "", origin_path, "");
      		}
      		else{
      			backup_list_insert(backup_dir_list, "", origin_path, "");
      			check_already_removed(origin_path);
      		}
      }
    } 
  }
  
  //print_list(backup_dir_list, 0, 0);

  return 0;
}

//Function to init tree list from commit log
int init_commit_log_list(int log_fd) {
  int len;
  char buf[BUF_MAX];
  char *backup_time;
  char *backup_path = (char*)malloc(sizeof(char) * PATHMAX);
  char *temp_path = (char*)malloc(sizeof(char) * PATHMAX);
  char *origin_path = (char*)malloc(sizeof(char) * PATHMAX);
  //commit_dir_list = (dirNode*)malloc(sizeof(dirNode));
  backup_dir_list = (dirNode*)malloc(sizeof(dirNode));
  //dirNode_init(commit_dir_list);
  dirNode_init(backup_dir_list);
  
  while(len = read(log_fd, buf, BUF_MAX)) {
    char *ptr = strchr(buf, '\n');
    ptr[0] = '\0';
    struct stat path_stat;
    lseek(log_fd, -(len-strlen(buf))+1, SEEK_CUR);
    if((strstr(buf, COMMIT_SEP)) != NULL) {
      if(sscanf(buf, "commit: \"%[^\"]\" - new file: \"%[^\"]\"", backup_path, origin_path) == 2) {
      		stat(origin_path, &path_stat);
      		strcpy(temp_path, repoPath);
      		strcat(temp_path, "/");
      		strcat(temp_path, backup_path);
      		char *result = findDivergingPath(temp_path, origin_path);
      		strcat(temp_path, "/");
      		strcat(temp_path, result);
      		
      		if(S_ISDIR(path_stat.st_mode)){
      			backup_dir(origin_path, temp_path, "", 0,0, backup_dir_list);
      		}
      		else{
      			backup_list_insert(backup_dir_list, "", origin_path, temp_path);
      		}
      }
    }//here
  }

  return 0;
}

//Function to init tree list
int init() {
  int staging_log_fd;
  int commit_log_fd;
  
  if((staging_log_fd = open(staging_log_PATH, O_RDWR|O_CREAT, 0777)) == -1) {
    fprintf(stderr, "ERROR: open error for '%s'\n", staging_log_PATH);
    return -1;
  }
  
  if((commit_log_fd = open(commit_log_PATH, O_RDWR|O_CREAT, 0777)) == -1) {
    fprintf(stderr, "ERROR: open error for '%s'\n", commit_log_PATH);
    return -1;
  }
  
  if(init_commit_log_list(commit_log_fd) == -1)
  	return -1;
  	
  if(init_backup_list(staging_log_fd) == -1)
  	return -1;
  	
  close(staging_log_fd);
  close(commit_log_fd);

  return 0;
}
