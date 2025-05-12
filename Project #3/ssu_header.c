#include "ssu_header.h"

//Init Function for parameter
void ParameterInit(command_parameter *parameter) {
  parameter->command = (char *)malloc(sizeof(char *) * PATH_MAX);
  parameter->filename = (char *)malloc(sizeof(char *) * PATH_MAX);
  parameter->commandopt = 0;
  parameter->time = 1;
}

//Function to return substring of a string
char *substr(char *str, int beg, int end) {
  char *ret = (char*)malloc(sizeof(char) * (end-beg+1));

  for(int i = beg; i < end && *(str+i) != '\0'; i++) {
    ret[i-beg] = str[i];
  }
  ret[end-beg] = '\0';

  return ret;
}

//Return substr
char *c_str(char *str) {
  return substr(str, 0, strlen(str));
}

//Function to convert a path to a realpath
char *cvt_path_2_realpath(char* path) {
  pathNode *path_head;
  pathNode *curr_path;
  char *ptr;
  char *origin_path = (char *)malloc(sizeof(char *) * PATHMAX);
  char *ret_path = (char *)malloc(sizeof(char *) * PATHMAX);
  strcpy(ret_path, "");
  path_head = (pathNode*)malloc(sizeof(pathNode));
  path_head->depth = 0;
  path_head->tail_path = path_head;
  path_head->head_path = path_head;
  path_head->next_path = NULL;
  if(path[0] != '/') {
    strcat(origin_path, pwd_path);
    strcat(origin_path, "/");
    strcat(origin_path, path);
  } else {
    strcpy(origin_path, path);
  }
  if(path_list_init(path_head, origin_path) == -1) {
    return NULL;
  }
  curr_path = path_head->next_path;
  while(curr_path != NULL) {
    strcat(ret_path, curr_path->path_name);
    if(curr_path->next_path != NULL) {
      strcat(ret_path, "/");
    }
    curr_path = curr_path->next_path;
  }
  if(strlen(ret_path) == 0) {
    strcpy(ret_path, "/");
  }
  return c_str(ret_path);
}	

//md5 hash function
int md5(char *target_path, char *hash_result)
{
	FILE *fp;
	unsigned char hash[MD5_DIGEST_LENGTH];
	unsigned char buffer[SHRT_MAX];
	int bytes = 0;
	MD5_CTX md5;

	if ((fp = fopen(target_path, "rb")) == NULL){
		printf("ERROR: fopen error for %s\n", target_path);
		return 1;
	}

	MD5_Init(&md5);
	while ((bytes = fread(buffer, 1, SHRT_MAX, fp)) != 0)
		MD5_Update(&md5, buffer, bytes);
	
	MD5_Final(hash, &md5);

	for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
		sprintf(hash_result + (i * 2), "%02x", hash[i]);
	hash_result[HASH_MD5-1] = 0;
	fclose(fp);
	
	return 0;
}

//Get file name of path
char *get_file_name(char* path) {
  int i;
  char tmp_path[PATH_MAX];
  char *file_name = (char*)malloc(sizeof(char) * FILE_MAX);
  
  strcpy(tmp_path, path);
  for(i = 0; tmp_path[i]; i++) {
    if(tmp_path[i] == '/') strcpy(file_name, tmp_path+i+1);
  }

  return file_name;
}

char *QuoteCheck(char **str, char del) {
  char *tmp = *str+1;
  int i = 0;

  while(*tmp != '\0' && *tmp != del) {
    tmp++;
    i++;
  }
  if(*tmp == '\0') {
    *str = tmp;
    return NULL;
  }
  if(*tmp == del) {
    for(char *c = *str; *c != '\0'; c++) {
      *c = *(c+1);
    }
    *str += i;
    for(char *c = *str; *c != '\0'; c++) {
      *c = *(c+1);
    }
  }
}

//Return token of string
char *Tokenize(char *str, char *del) {
  int i = 0;
  int del_len = strlen(del);
  static char *tmp = NULL;
  char *tmp2 = NULL;

  if(str != NULL && tmp == NULL) {
    tmp = str;
  }

  if(str == NULL && tmp == NULL) {
    return NULL;
  }

  char *idx = tmp;

  while(i < del_len) {
    if(*idx == del[i]) {
      idx++;
      i = 0;
    } else {
      i++;
    }
  }
  if(*idx == '\0') {
    tmp = NULL;
    return tmp;
  }
  tmp = idx;

  while(*tmp != '\0') {
    if(*tmp == '\'' || *tmp == '\"') {
      QuoteCheck(&tmp, *tmp);
      continue;
    }
    for(i = 0; i < del_len; i++) {
      if(*tmp == del[i]) {
        *tmp = '\0';
        break;
      }
    }
    tmp++;
    if(i < del_len) {
      break;
    }
  }

  return idx;
}

//Function to write to pid.log for status of tracked file.
void pid_log_append(int pid, char* origin_path, char* flag){
        
      	FILE* fp = fopen(backup_log_path, "a");
  	if (fp == NULL) {
      		fprintf(stderr, "error opening fp\n");
      		exit(1);
  	}
        char *time_str = get_current_time();
        if(!strcmp(flag, "create"))
	      	fprintf(fp, "[%s][create][%s]\n", time_str, origin_path);
	else if(!strcmp(flag, "remove"))
		fprintf(fp, "[%s][remove][%s]\n", time_str, origin_path);
	else if(!strcmp(flag, "modify"))
		fprintf(fp, "[%s][modify][%s]\n", time_str, origin_path);
	else{
		fprintf(stderr, "Wrong flag input for function pid_log_append\n");
		fclose(fp);
		exit(1);
	}
      	fclose(fp);
}

//Function to go through tree and log [create], [modify], [remove] and backup files accordingly
void go_through_tree(dirNode* dirList, int opt_bit, int pid, char* backup_dir_path) {
  int last_bit = 0;
  int depth = 0;
  struct stat statbuf, stat1, stat2;
  pid = getpid();
  dirNode* curr_dir = dirList->subdir_head->next_dir;
  fileNode* curr_file = dirList->file_head->next_file;
  
  while(curr_file != NULL) {
    last_bit |= (curr_file->next_file == NULL)?(1 << depth):0;
    backupNode* curr_backup = curr_file->backup_head->next_backup;
  
    while(curr_backup != NULL) {
      //If no backup of this file exists, backup the file and write [create].. in pid log.
      //Or if original file was removed before, but recreated.
      if(curr_backup->backup_path == NULL || !strcmp(curr_backup->backup_path, "") || (lstat(curr_backup->origin_path, &statbuf) == 0 && curr_backup->wasRemoved == true)){
      
        pid_log_append(pid, curr_backup->origin_path, "create");
        backup_file(curr_backup->origin_path, backup_dir_path, cvt_time_2_str(time(NULL)), pid, curr_backup, curr_file);
        curr_backup->wasRemoved = false;
      }
      //If original file was removed, write [remove]... in pid log.
      else if(lstat(curr_backup->origin_path, &statbuf) < 0 && curr_backup->wasRemoved == false) {
      	pid_log_append(pid, curr_backup->origin_path, "remove");
      	curr_backup->wasRemoved = true;
      }
      //Check if original file version is different(modified) from most recent backup version
      else if(curr_backup->next_backup == NULL){
      	if(lstat(curr_backup->origin_path, &stat1) != 0){
      		break;
      	} 
      	if(lstat(curr_backup->backup_path, &stat2) != 0){
      		break;
      	}
      	//If mtime is different for the two files
      	if((stat1.st_mtime != stat2.st_mtime)){
      		//If hash is different
      		if(cmpHash(curr_backup->origin_path, curr_backup->backup_path)){
	      		pid_log_append(pid, curr_backup->origin_path, "modify");
	      		backup_file(curr_backup->origin_path, backup_dir_path, cvt_time_2_str(time(NULL)), pid, curr_backup, curr_file);
			curr_backup->wasRemoved = false;
        	}
      	}
      }
      
      curr_backup = curr_backup->next_backup;
    }
    curr_file = curr_file->next_file;
  }
  while(curr_dir != NULL) {
    last_bit |= (curr_dir->next_dir == NULL)?(1 << depth):0;
    go_through_tree(curr_dir, depth+1, last_bit, backup_dir_path);
    curr_dir = curr_dir->next_dir;
  }
}

//Function to check if path exists in monitor_log
int go_through_monitor_log(char* full_path){
	char* tmp_path = (char *)malloc(sizeof(char *) * PATHMAX);
	FILE* file = fopen(moniter_list_PATH, "r");
	if(!file){
		fprintf(stderr, "monitor_list.log not initalized!\n");
		exit(1);
	}
	
	char line[PATH_MAX * 2];
	char *originalPath = (char *)malloc(sizeof(char *) * PATHMAX);
	int pid;
	while(fgets(line, sizeof(line), file)) {
		if(sscanf(line, "%d : %4096[^\n]", &pid, originalPath) == 2) {
			strcpy(tmp_path, full_path);
			if(!strcmp(tmp_path, originalPath)){
				if(pid_exists(pid) == 1){
					fclose(file);
					return -1;
				}
			}
		}
	}
	fclose(file);
	return 1;
}

//Check if process(pid) exists
int pid_exists(pid_t pid){
	//pid exists, doesnt acutally kill
	if(kill(pid, 0) == 0){
		return 1;
	}
	else{
		//Process doesnt exist
		if(errno == ESRCH) {
			return 0;
		}
		//Process exists, but no permission 
		else if(errno == EPERM) {
			return 1;
		} 
		//Unexpected error
		else {
			return -1;
		}
	}
}

//Function to check if path exists in tree list
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

//Get sub string
char **GetSubstring(char *str, int *cnt, char *del) {
  *cnt = 0;
  int i = 0;
  char *token = NULL;
  char *templist[100] = {NULL, };
  token = Tokenize(str, del);
  if(token == NULL) {
    return NULL;
  }

  while(token != NULL) {
    templist[*cnt] = token;
    *cnt += 1;
    token = Tokenize(NULL, del);
  }
  
	char **temp = (char **)malloc(sizeof(char *) * (*cnt + 1));
	for (i = 0; i < *cnt; i++) {
		temp[i] = templist[i];
	}
	return temp;
}

// Used in cvt_path_2_realpath, init path list
int path_list_init(pathNode *curr_path, char *path) {
  pathNode *new_path = curr_path;
  char *ptr;
  char *next_path = "";

  if(!strcmp(path, "")) return 0;

  if(ptr = strchr(path, '/')) {
    next_path = ptr+1;
    ptr[0] = '\0';
  }

  if(!strcmp(path, "..")) {
    new_path = curr_path->prev_path;
    new_path->tail_path = new_path;
    new_path->next_path = NULL;
    new_path->head_path->tail_path = new_path;

    new_path->head_path->depth--;

    if(new_path->head_path->depth == 0) return -1;
  } else if(strcmp(path, ".")) {
    new_path = (pathNode*)malloc(sizeof(pathNode));
    strcpy(new_path->path_name, path);

    new_path->head_path = curr_path->head_path;
    new_path->tail_path = new_path;

    new_path->prev_path = curr_path;
    new_path->next_path = curr_path->next_path;
    
    curr_path->next_path = new_path;
    new_path->head_path->tail_path = new_path;

    new_path->head_path->depth++;
  }

  if(strcmp(next_path, "")) {
    return path_list_init(new_path, next_path);
  }

  return 0;
}

//Function to find the diverging part of path2 from path1
char* findDivergingPath(const char *path1, const char *path2) {
    int i = 0;

    //Continue until either path ends or characters differ
    while (path1[i] == path2[i] && path1[i] != '\0' && path2[i] != '\0') {
        i++;
    }

    //If we are not at the end of path2, move to the start of the next segment
    if (path2[i] != '\0') {
        //If the divergence is not at a path separator, find the next path separator
        if (path2[i] != '/') {
            while (i > 0 && path2[i-1] != '/') {
                i--; // Move back to the previous '/'
            }
        }
        
        // Allocate memory and copy the diverging path
        char* divergingPath = strdup(&path2[i]);
        
        return divergingPath;
    }

    //If the entire path2 is a prefix of path1 or paths are identical, return an empty string
    return strdup("");
}

//Turn string into hash
int cvtHash(char *target_path, char *hash_result) {
  md5(target_path, hash_result);
}

//Compare hashes of path1 and path2
int cmpHash(char *path1, char *path2) {
  char *hash1 = (char *)malloc(sizeof(char) * HASH_MD5);
  char *hash2 = (char *)malloc(sizeof(char) * HASH_MD5);

  cvtHash(path1, hash1);
  cvtHash(path2, hash2);

  return strcmp(hash1, hash2);
}

//Function to make list for path that is a directory
int backup_list_dir(char* origin_path, char* backup_path, char* time_str, int opt_bit, dirNode* backup_list) {
  struct dirent **namelist;
  struct stat statbuf;
  dirNode *dirList;
  dirNode *curr_dir;
  char sub_backup_path[PATH_MAX];
  int cnt;
  int sub_backup_cnt = 0;
  int backup_cnt = 0;

  dirList = (dirNode*)malloc(sizeof(dirNode));
  dirNode_init(dirList);
  dirNode_append(dirList, "", "");

  curr_dir = dirList->next_dir;

  while(curr_dir != NULL) {
    char origin_dir_path[PATH_MAX];
    sprintf(origin_dir_path, "%s%s", origin_path, c_str(curr_dir->dir_name));

    if((cnt = scandir(origin_dir_path, &namelist, NULL, alphasort)) == -1) {
      return -1;
    }

    for(int i = 0; i < cnt; i++) {
      //Exclude '.', '..', and the repo directory
      if(!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..") || strstr(namelist[i]->d_name, "repo") != NULL || strstr(namelist[i]->d_name, ".swp") != NULL) continue;

      char tmp_path[PATH_MAX];
      sprintf(tmp_path, "%s/%s", c_str(origin_dir_path), namelist[i]->d_name);
      
      if (lstat(tmp_path, &statbuf) < 0) {
        fprintf(stderr, "ERROR: lstat error for %s\n", tmp_path);
        return -1;
      }

      if(S_ISREG(statbuf.st_mode)) {
        char new_backup_path[PATH_MAX];
        sprintf(new_backup_path, "%s%s", backup_path, c_str(curr_dir->dir_name));
        backup_list_insert(backup_list, time_str, tmp_path, backup_path);
        backup_cnt += sub_backup_cnt;
      } else if(opt_bit & OPT_R){

        sprintf(sub_backup_path, "%s%s/%s", origin_path, c_str(curr_dir->dir_name), namelist[i]->d_name);
        backup_list_dir(sub_backup_path, backup_path, time_str, opt_bit, backup_list);

        backup_cnt += sub_backup_cnt;
      }
    }
    curr_dir = curr_dir->next_dir;
  }

  return backup_cnt;
}

//Function to check path accessability
int check_path_access(char* path, int opt) {
  char *origin_path = (char*)malloc(sizeof(char)*(strlen(path)+1));
  char* ptr;
  int depth = 0;
  
  strcpy(origin_path, path);

  while(ptr = strchr(origin_path, '/')) {
    char *tmp_path = substr(origin_path, 0, strlen(origin_path) - strlen(ptr));

    depth++;
    origin_path = ptr+1;
    
    //While path is being cut down, if it never contains /home/, return -1
    if(depth == 2 && strcmp(substr(path, 0, strlen(path) - strlen(origin_path)), "/home/")) {
      fprintf(stderr, "ERROR: path must be in user directory\n");
      fprintf(stderr, " - '%s' is not in user directory\n", path);
      return -1;
    }
  }
  
  if(!strcmp(path, "/home")){
      fprintf(stderr, "ERROR: path must be in user directory\n");
      return -1;
  }
  
  //If path contains backupPATH return -1
  if(strstr(path, backupPATH) != NULL) {
      fprintf(stderr, "ERROR: path must be in user directory\n");
      fprintf(stderr, " - '%s' is not in user directory\n", path);
      return -1;
  }
  
  if(depth == 2 && (opt & OPT_D) && !strcmp(path, homePATH)){
  	return 0;
  }
  //If depth is too small (out of user dir) return -1
  else if(depth < 3) {
    fprintf(stderr, "ERROR: path must be in user directory\n");
    fprintf(stderr, " - '%s' is not in user directory\n", path);
    return -1;
  }
  
  //Else return 0
  return 0;
}

//Check if path is file, directory, or etc
int Path_Type(const char* path) {
	struct stat path_stat;
	//Wrong type or doesnt exist
	if(stat(path, &path_stat) != 0) return -1;
	
	//File
	if(S_ISREG(path_stat.st_mode)) return 1;
	
	//Directory
	if(S_ISDIR(path_stat.st_mode)) return 1;
	
	return -1;
}

//Function to get time for backup file. e.g. a.txt_20240503100000
char *cvt_time_2_str(time_t time) {
  struct tm *time_tm = localtime(&time);
  char *time_str = (char *)malloc(sizeof(char) * 32);
  sprintf(time_str, "%04d%02d%02d%02d%02d%02d",
    (time_tm->tm_year + 1900),
    time_tm->tm_mon + 1,
    time_tm->tm_mday,
    time_tm->tm_hour,
    time_tm->tm_min,
    time_tm->tm_sec
  );
  return time_str;
}

//Goes up folders to erase empty folders
void RemoveEmptyDirRecursive(char* path) {
	struct dirent **namelist;
  	char *parentPath = (char *)malloc(sizeof(char *) * PATHMAX);
  	int cnt;

	if(rmdir(path) != 0) { return; }
 
  	strncpy(parentPath, path, PATHMAX);
  	char* parent = dirname(parentPath);
  	
  	//If the parent dir is backupPATH, stop
  	if(!strcmp(parent, backupPATH)) { return; }
  	
  	//Recursively check parent directories to remove directories
  	RemoveEmptyDirRecursive(parent);
	
	return;
}

//Function to get time for pid.log
char* get_current_time(void){
	char* buf = malloc(30);
	if(buf == NULL){
		return NULL;
	}
	
	time_t current_time = time(NULL);
	struct tm *tm_info;
	
	tm_info = localtime(&current_time);
	
	if(strftime(buffer, 30, "%Y-%m-%d %H:%M:%S", tm_info) == 0){
		free(buf);
		return NULL;
	}
	
	return buffer;
}
