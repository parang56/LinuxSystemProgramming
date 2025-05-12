#define OPENSSL_API_COMPAT 0x10100000L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <wait.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <stdbool.h>
#include <libgen.h>
#include <utime.h>

#define true 1
#define false 0

#define HASH_MD5  33

#define CMD_ADD      0b00000001
#define CMD_REMOVE   0b00000010
#define CMD_STATUS   0b00000100
#define CMD_COMMIT   0b00001000
#define CMD_REVERT   0b00010000
#define CMD_LOG      0b00100000
#define CMD_HELP     0b01000000
#define CMD_EXIT     0b10000000
#define NOT_CMD	     0b00000000

#define ADD_SEP        "add "
#define REMOVE_SEP     "remove "
#define COMMIT_SEP     "commit"

#define HASH_BACKUP  1
#define HASH_REMOVE  2
#define HASH_RECOVER 3
#define HASH_LIST    4
#define HASH_HELP    5

#define FILE_MAX 255
#define PATHMAX 4096
#define STRMAX 4096
#define BUF_MAX 4096
#define MAX_LINE_LENGTH 1024

#define OPT_A		0b00001
#define OPT_C		0b00010
#define OPT_D		0b00100
#define OPT_N		0b01000
#define NOT_OPT 0b00000

char *commanddata[10]={
    "add",
    "remove",
    "status",
    "commit",
    "recover",
    "log",
    "help"
  };

char exeNAME[PATHMAX];
char exePATH[PATHMAX];
char homePATH[PATHMAX];
char commit_log_PATH[PATHMAX];
char staging_log_PATH[PATHMAX];
char repoPath[PATHMAX];
char pwd_path[PATHMAX];
int hash;

char *full_path;
char *tmp;
int init();
int md5(char*, char*);
char *cvt_path_2_realpath(char* path);
int checkIfBackuped(char* path, char* path2);
int files_changed, insertions, deletions;

typedef struct command_parameter {
  char *command;
  char *filename;
  char *tmpname;
  int commandopt;
  char *argv[10];
} command_parameter;


typedef struct _backupNode {
  struct _dirNode *root_version_dir;
  struct _fileNode *root_file;

  char backup_time[13];
  char origin_path[PATH_MAX];
  char backup_path[PATH_MAX];
  
  bool wasRemoved;;

  struct _backupNode *prev_backup;
  struct _backupNode *next_backup;
} backupNode;

typedef struct _fileNode {
  struct _dirNode *root_dir;

  int backup_cnt;
  char file_name[FILE_MAX];
  char file_path[PATH_MAX];
  backupNode *backup_head;

  struct _fileNode *prev_file;
  struct _fileNode *next_file;
} fileNode;

typedef struct _dirNode {
  struct _dirNode *root_dir;

  int file_cnt;
  int subdir_cnt;
  int backup_cnt;
  char dir_name[FILE_MAX];
  char dir_path[PATH_MAX];
  fileNode *file_head;
  struct _dirNode *subdir_head;

  struct _dirNode* prev_dir;
  struct _dirNode *next_dir;
} dirNode;

typedef struct _pathNode {
  char path_name[FILE_MAX];
  int depth;

  struct _pathNode *prev_path;
  struct _pathNode *next_path;

  struct _pathNode *head_path;
  struct _pathNode *tail_path;
} pathNode;

dirNode* backup_dir_list;
dirNode* commit_dir_list;

int path_list_init(pathNode *curr_path, char *path);
backupNode *backupNode_remove(backupNode *backup_head, char *backup_time, char *file_name, char *dir_path, char *backup_path);
backupNode *backupNode_insert(backupNode *backup_head, char *backup_time, char *file_name, char *dir_path, char *backup_path);
void backupNode_init(backupNode *backup_node);
void fileNode_remove(fileNode *file_node);
void fileNode_init(fileNode *file_node);
dirNode *dirNode_append(dirNode* dir_head, char* dir_name, char* dir_path);
dirNode *dirNode_insert(dirNode* dir_head, char* dir_name, char* dir_path);
void dirNode_init(dirNode *dir_node);
int backup_list_insert(dirNode* dirList, char* backup_time, char* path, char* backup_path);
int check_root_dir(dirNode *dir_node);
int add_cnt_root_dir(dirNode *dir_node, int cnt);


char buffer[PATHMAX];
char untracked_buffer[PATHMAX];

//Function to return substring of a string
char *substr(char *str, int beg, int end) {
  char *ret = (char*)malloc(sizeof(char) * (end-beg+1));

  for(int i = beg; i < end && *(str+i) != '\0'; i++) {
    ret[i-beg] = str[i];
  }
  ret[end-beg] = '\0';

  return ret;
}

char *exe_path;
char *home_path;

//Return substr
char *c_str(char *str) {
  return substr(str, 0, strlen(str));
}

//Function to convert a path to a realpath
char *cvt_path_2_realpath(char* path) {
  pathNode *path_head;
  pathNode *curr_path;
  char *ptr;
  char origin_path[PATH_MAX];
  char ret_path[PATH_MAX] = "";
  path_head = (pathNode*)malloc(sizeof(pathNode));
  path_head->depth = 0;
  path_head->tail_path = path_head;
  path_head->head_path = path_head;
  path_head->next_path = NULL;
  if(path[0] != '/') {
    //sprintf(origin_path, "%s/%s", pwd_path, path);
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
		//printf("ERROR: fopen error for %s\n", target_path);
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

// Function to convert an absolute path to a relative path
char* convertToRelativePath(const char* basePath, const char* fullPath) {
    // Check if fullPath actually starts with basePath
    if (strncmp(basePath, fullPath, strlen(basePath)) != 0) {
        printf("Error: fullPath does not start with basePath\n");
        return NULL;
    }

    // Calculate the relative path's length
    size_t relativePathLen = strlen(fullPath) - strlen(exePATH) + 2; // +2 for "./"
    char* relativePath = (char*)malloc(relativePathLen + 1); // +1 for null terminator

    if (relativePath == NULL) {
        printf("Memory allocation failed\n");
        return NULL;
    }

    // Prepend "./" to the relative path
    strcpy(relativePath, ".");
    strcat(relativePath, fullPath + strlen(exePATH)); // Skip basePath length
    
    //printf("basePath: %s, fullPath: %s, rP: %s\n", basePath, fullPath, relativePath);

    return relativePath;
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

// Function to find the diverging part of path2 from path1
char* findDivergingPath(const char *path1, const char *path2) {
    int i = 0;

    // Continue until either path ends or characters differ
    while (path1[i] == path2[i] && path1[i] != '\0' && path2[i] != '\0') {
        i++;
    }

    // If we are not at the end of path2, move to the start of the next segment
    if (path2[i] != '\0') {
        // If the divergence is not at a path separator, find the next path separator
        if (path2[i] != '/') {
            while (i > 0 && path2[i-1] != '/') {
                i--; // Move back to the previous '/'
            }
        }
        
        // Allocate memory and copy the diverging path
        char* divergingPath = strdup(&path2[i]);
        return divergingPath;
    }

    // If the entire path2 is a prefix of path1 or paths are identical, return an empty string
    return strdup("");
}

// Turn string into hash
int cvtHash(char *target_path, char *hash_result) {
  md5(target_path, hash_result);
}

// Compare hashes of path1 and path2
int cmpHash(char *path1, char *path2) {
  char *hash1 = (char *)malloc(sizeof(char) * HASH_MD5);
  char *hash2 = (char *)malloc(sizeof(char) * HASH_MD5);

  cvtHash(path1, hash1);
  cvtHash(path2, hash2);

  return strcmp(hash1, hash2);
}

// Check if path is being tracked in tree linked list
int check_already_backuped(char* path) {
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
    if(!cmpHash(path, curr_backup->next_backup->origin_path)) {
    	//If in tree and was removed, turn wasRemoved to false so we track it again
    	if(curr_backup->next_backup->wasRemoved == true){
    		curr_backup->next_backup->wasRemoved = false;
    	}
    	//If in tree and was not removed, return 1
    	else
	      return 1;
    }
    curr_backup = curr_backup->next_backup;
  }
  //Else return 0
  return 0;
}

//Init Function for parameter
void ParameterInit(command_parameter *parameter) {
  parameter->command = (char *)malloc(sizeof(char *) * PATH_MAX);
  parameter->filename = (char *)malloc(sizeof(char *) * PATH_MAX);
  parameter->tmpname = (char *)malloc(sizeof(char *) * PATH_MAX);
	parameter->commandopt = 0;
}

//function to check if there is any not backed up file in the path we are seeking
int check_already_backedup_dir(char* origin_path, char* backup_path, char* time_str, int opt_bit, int log_fd) {
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
      //fprintf(stderr, "ERROR: scandir error for %s\n", origin_dir_path);
      return -1;
    }

    for(int i = 0; i < cnt; i++) {
      if(!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;

      char tmp_path[PATH_MAX];
      sprintf(tmp_path, "%s/%s", c_str(origin_dir_path), namelist[i]->d_name);
      if (lstat(tmp_path, &statbuf) < 0) {
        fprintf(stderr, "ERROR: lstat error for %s\n", tmp_path);
        return -1;
      }

      if(S_ISREG(statbuf.st_mode)) {
        char new_backup_path[PATH_MAX];
        sprintf(new_backup_path, "%s%s", backup_path, c_str(curr_dir->dir_name));
        
        //If even one is not backed up, add to log
        if(check_already_backuped(tmp_path) != 1){
        	return 0;
        }
        
        //backup_cnt += sub_backup_cnt;
      } else {

        sprintf(sub_backup_path, "%s%s/%s", backup_path, c_str(curr_dir->dir_name), namelist[i]->d_name);
        
        check_already_backedup_dir(sub_backup_path, sub_backup_path, "", 0, 0);
        //backup_cnt += sub_backup_cnt;
      }
    }
    curr_dir = curr_dir->next_dir;
  }

  // If all is backed up, return 1;
  return 1;
}

// Function to check if path was already removed.
// This is removed2 because 1 prints errors, this doesnt.
int check_already_removed2(char* path) {  
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
  char* relative_path = (char *)malloc(sizeof(char *) * PATHMAX);
  if(full_path == NULL){
      if((full_path = cvt_path_2_realpath(path)) == NULL) {
        fprintf(stderr, "ERROR: '%s' is wrong path\n", path);
        return -1;
      }
    }
  relative_path = convertToRelativePath(pwd_path, full_path);
  
  while(curr_backup->next_backup != NULL) {
    //hash is same
    if(!cmpHash(path, curr_backup->next_backup->origin_path)) {
      //wasRemoved is false = not erased -> return 0
      if(curr_backup->next_backup->wasRemoved == false){
  	return 0;
      }
    }
    curr_backup = curr_backup->next_backup;
  }
  // All was removed -> return 1
  return 1;
}

//Function to check if directory has a file that is not being not tracked
int check_already_removed_dir(char* origin_path, char* backup_path, char* time_str, int opt_bit, int log_fd) {
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
      if(!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;

      char tmp_path[PATH_MAX];
      sprintf(tmp_path, "%s/%s", c_str(origin_dir_path), namelist[i]->d_name);
      if (lstat(tmp_path, &statbuf) < 0) {
        fprintf(stderr, "ERROR: lstat error for %s\n", tmp_path);
        return -1;
      }

      if(S_ISREG(statbuf.st_mode)) {
        char new_backup_path[PATH_MAX];
        sprintf(new_backup_path, "%s%s", backup_path, c_str(curr_dir->dir_name));
        
        //If even one is not removed, add to log
        if(check_already_removed2(tmp_path) == 0){
        	return 0;
        }
      } else {

        sprintf(sub_backup_path, "%s%s/%s", backup_path, c_str(curr_dir->dir_name), namelist[i]->d_name);
        
        check_already_backedup_dir(sub_backup_path, sub_backup_path, "", 0, 0);
        //backup_cnt += sub_backup_cnt;
      }
    }
    curr_dir = curr_dir->next_dir;
  }

  // If all is removed, return 0;
  return 1;
}

// Check if path was already removed (with errors)
int check_already_removed(char* path) {  
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
  char* relative_path = (char *)malloc(sizeof(char *) * PATHMAX);
  if(full_path == NULL){
      if((full_path = cvt_path_2_realpath(path)) == NULL) {
        fprintf(stderr, "ERROR: '%s' is wrong path\n", path);
        return -1;
      }
    }
  relative_path = convertToRelativePath(pwd_path, full_path);
  
  while(curr_backup->next_backup != NULL) {
    //hash is same
    if(!cmpHash(path, curr_backup->next_backup->origin_path)) {
      //wasRemoved is true
      if(curr_backup->next_backup->wasRemoved == true){
      	printf("\"%s\" already removed from staging area\n", relative_path);
  	return 1;
      }
      else{
      	curr_backup->next_backup->wasRemoved = true;
      }
      	
    }
    curr_backup = curr_backup->next_backup;
  }
  return 0;
}

//Function to check if directory has a file that is not being tracked
int backup_dir(char* origin_path, char* backup_path, char* time_str, int opt_bit, int log_fd, dirNode* backup_list) {
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
      //fprintf(stderr, "ERROR: scandir error for %s\n", origin_dir_path);
      return -1;
    }

    for(int i = 0; i < cnt; i++) {
      //Exclude '.', '..', and the repo directory
      if(!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..") || strstr(namelist[i]->d_name, "repo") != NULL) continue;

      char tmp_path[PATH_MAX];
      sprintf(tmp_path, "%s/%s", c_str(origin_dir_path), namelist[i]->d_name);
      
      if (lstat(tmp_path, &statbuf) < 0) {
        fprintf(stderr, "ERROR: lstat error for %s\n", tmp_path);
        return -1;
      }

      if(S_ISREG(statbuf.st_mode)) {
        char new_backup_path[PATH_MAX];
        sprintf(new_backup_path, "%s%s", backup_path, c_str(curr_dir->dir_name));
        backup_list_insert(backup_list, "", tmp_path, backup_path);
        backup_cnt += sub_backup_cnt;
      } else {

        sprintf(sub_backup_path, "%s%s/%s", origin_path, c_str(curr_dir->dir_name), namelist[i]->d_name);
        backup_dir(sub_backup_path, "", "", 0, 0, backup_list);

        backup_cnt += sub_backup_cnt;
      }
    }
    curr_dir = curr_dir->next_dir;
  }

  return backup_cnt;
}

int backup_list_remove(dirNode* dirList, char* backup_time, char* path, char* backup_path);

//add to be empty directory not backup
int remove_dir(char* origin_path, char* backup_path, char* time_str, int opt_bit, int log_fd) {
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
      //fprintf(stderr, "ERROR: scandir error for %s\n", origin_dir_path);
      return -1;
    }

    for(int i = 0; i < cnt; i++) {
      if(!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..") || strstr(namelist[i]->d_name, "repo") != NULL) continue;

      char tmp_path[PATH_MAX];
      sprintf(tmp_path, "%s/%s", c_str(origin_dir_path), namelist[i]->d_name);
      
      if (lstat(tmp_path, &statbuf) < 0) {
        fprintf(stderr, "ERROR: lstat error for %s\n", tmp_path);
        return -1;
      }

      if(S_ISREG(statbuf.st_mode)) {
        char new_backup_path[PATH_MAX];
        sprintf(new_backup_path, "%s%s", backup_path, c_str(curr_dir->dir_name));
	//printf("new_backup_path: %s\n", new_backup_path);
        backup_list_remove(backup_dir_list, "", tmp_path, backup_path);
        backup_cnt += sub_backup_cnt;
      } else {

        sprintf(sub_backup_path, "%s%s/%s", origin_path, c_str(curr_dir->dir_name), namelist[i]->d_name);
       
        remove_dir(sub_backup_path, "", "", 0, 0);

        backup_cnt += sub_backup_cnt;
      }
    }
    curr_dir = curr_dir->next_dir;
  }

  return backup_cnt;
}

//Function to check path accessability
int check_path_access(char* path) {
  int i;
  int cnt;
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
  
  //If path contains repoPath return -1
  if(strstr(path, repoPath) != NULL) {
      fprintf(stderr, "ERROR: path must be in user directory\n");
      fprintf(stderr, " - '%s' is not in user directory\n", path);
      return -1;
  }
  
  //If depth is too small (out of user dir) return -1
  if(depth < 3) {
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
	if(S_ISREG(path_stat.st_mode)) return 0;
	//Directory
	if(S_ISDIR(path_stat.st_mode)) return 1;
	return -1;
}

//check_path_access but no fprintf
int check_path_access2(char* path) {
  int i;
  int cnt;
  char *origin_path = (char*)malloc(sizeof(char)*(strlen(path)+1));
  char* ptr;
  int depth = 0;
  
  strcpy(origin_path, path);

  while(ptr = strchr(origin_path, '/')) {
    char *tmp_path = substr(origin_path, 0, strlen(origin_path) - strlen(ptr));

    depth++;
    origin_path = ptr+1;

    if(depth == 2 && strcmp(substr(path, 0, strlen(path) - strlen(origin_path)), "/home/")) {
      return -1;
    }
  }
  if(strstr(path, "repo") != NULL)
      return -1;
  if(strstr(path, repoPath) != NULL) {
      return -1;
  }

  return 0;
}

//Goes up folders to erase empty folders
void RemoveEmptyDirRecursive(char* path) {
	struct dirent **namelist;
  	char *parentPath = (char *)malloc(sizeof(char *) * PATHMAX);
  	int cnt;

	if(rmdir(path) != 0) { return; }
 
  	strncpy(parentPath, path, PATHMAX);
  	char* parent = dirname(parentPath);
  	
  	//If the parent dir is root, stop
  	if(!strcmp(parent, repoPath)) { return; }
  	
  	//Recursively check parent directories to remove directories
  	RemoveEmptyDirRecursive(parent);
	
	return;
}

void make_relative_path(const char *path, const char *cwd, char *relative_path) {
    int i = 0, last_common_slash = 0;
    int need_up = 0, is_subpath = 1;

    // Find the last common slash position
    while (path[i] != '\0' && cwd[i] != '\0' && path[i] == cwd[i]) {
        if (path[i] == '/') {
            last_common_slash = i;
        }
        i++;
    }

    // Determine if the path is a direct subpath or requires navigating up
    if (cwd[i] != '\0') {
        is_subpath = 0; // Additional segments in cwd means not a direct subpath
    }

    // If there is remaining part in cwd, count how many directories we need to go up
    for (int j = i; cwd[j] != '\0'; j++) {
        if (cwd[j] == '/') {
            need_up++;
        }
    }

    // Start forming the relative path
    relative_path[0] = '\0'; // Clear the string

    if (is_subpath) {
        // Add "./" prefix to indicate current directory relative path
        strcat(relative_path, "./");
        if (path[i] == '/') {
            i++; // Skip the extra slash to avoid starting the subpath with a slash
        }
    } else {
        // Add necessary "../" to step back
        for (int k = 0; k < need_up; k++) {
            strcat(relative_path, "../");
        }
    }

    // Append the rest of the path after the last common slash
    if (path[i] != '\0') {
        strcat(relative_path, path + i);
    }
}

// Function to prepend `temp_path` to `buffer`
void prepend_to_buffer(char *buffer, const char *temp_path) {
    size_t len_buffer = strlen(buffer);
    size_t len_temp_path = strlen(temp_path);
    
    // Check if there's enough space in the buffer
    if (len_temp_path + len_buffer + 1 > PATH_MAX) {
        fprintf(stderr, "Buffer overflow prevented\n");
        return;
    }

    // Shift existing content to the right to make space for new content
    memmove(buffer + len_temp_path, buffer, len_buffer + 1); // +1 to include the null terminator

    // Copy new string to the start of the buffer
    memcpy(buffer, temp_path, len_temp_path);
}
