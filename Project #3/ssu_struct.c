#include "ssu_header.h"

//Function to insert path into list
int backup_list_insert(dirNode* dirList, char* backup_time, char* path, char* backup_path) {
  char* ptr;

  if(ptr = strchr(path, '/')) {
	char* dir_name = substr(path, 0, strlen(path) - strlen(ptr));
	dirNode* curr_dir = dirNode_insert(dirList->subdir_head, dir_name, dirList->dir_path);
	backup_list_insert(curr_dir, backup_time, ptr+1, backup_path);
	curr_dir->backup_cnt++;

  } else {
	char* file_name = path;
	fileNode* curr_file = fileNode_insert(dirList->file_head, file_name, dirList->dir_path);
	backupNode_insert(curr_file->backup_head, backup_time, file_name, dirList->dir_path, backup_path);
  }

  return 0;
}

//Function to init a dirNode
void dirNode_init(dirNode *dir_node) {
  dir_node->root_dir = NULL;

  dir_node->file_cnt = 0;
  dir_node->subdir_cnt = 0;
  dir_node->backup_cnt = 0;

  dir_node->file_head = (fileNode*)malloc(sizeof(fileNode));
  dir_node->file_head->prev_file = NULL;
  dir_node->file_head->next_file = NULL;
  dir_node->file_head->root_dir = dir_node;

  dir_node->subdir_head = (dirNode*)malloc(sizeof(dirNode));
  dir_node->subdir_head->prev_dir = NULL;
  dir_node->subdir_head->next_dir = NULL;
  dir_node->subdir_head->root_dir = dir_node;

  dir_node->prev_dir = NULL;
  dir_node->next_dir = NULL;
}

dirNode *dirNode_get(dirNode* dir_head, char* dir_name) {
  dirNode *curr_dir = dir_head->next_dir;
  
  while(curr_dir != NULL) {
    if(curr_dir != NULL && !strcmp(dir_name, curr_dir->dir_name)) {
      return curr_dir;
    }
    curr_dir = curr_dir->next_dir;
  }
  return NULL;
}

//Function to insert dirNode in tree
dirNode *dirNode_insert(dirNode* dir_head, char* dir_name, char* dir_path) {
  dirNode *curr_dir = dir_head;
  dirNode *next_dir, *new_dir;
  
  while(curr_dir != NULL) {
    next_dir = curr_dir->next_dir;

    if(next_dir != NULL && !strcmp(dir_name, next_dir->dir_name)) {
      return curr_dir->next_dir;
    } else if(next_dir == NULL || strcmp(dir_name, next_dir->dir_name) < 0) {
      new_dir = (dirNode*)malloc(sizeof(dirNode));
      dirNode_init(new_dir);

      new_dir->root_dir = dir_head->root_dir;
      
      sprintf(new_dir->dir_name, "%s", dir_name);
      sprintf(new_dir->dir_path, "%s%s/", dir_path, dir_name);

      new_dir->prev_dir = curr_dir;
      new_dir->next_dir = next_dir;
      
      if(next_dir != NULL) {
        next_dir->prev_dir = new_dir;
      }
      curr_dir->next_dir = new_dir;

      dir_head->root_dir->subdir_cnt++;

      return curr_dir->next_dir;
    }
    curr_dir = next_dir;
  }
  return NULL;
}

//Function to append dirnode
dirNode *dirNode_append(dirNode* dir_head, char* dir_name, char* dir_path) {
  dirNode *curr_dir = dir_head;
  dirNode *next_dir, *new_dir;
  
  while(curr_dir != NULL) {
    next_dir = curr_dir->next_dir;
    if(next_dir == NULL) {
      new_dir = (dirNode*)malloc(sizeof(dirNode));
      dirNode_init(new_dir);

      new_dir->root_dir = dir_head->root_dir;
      
      sprintf(new_dir->dir_name, "%s", dir_name);
      sprintf(new_dir->dir_path, "%s%s/", dir_path, dir_name);

      new_dir->prev_dir = curr_dir;
      new_dir->next_dir = next_dir;
      
      if(next_dir != NULL) {
        next_dir->prev_dir = new_dir;
      }
      curr_dir->next_dir = new_dir;

      return curr_dir->next_dir;
    }
    curr_dir = next_dir;
  }
  return NULL;
}

//Function to init a file node
void fileNode_init(fileNode *file_node) {
  file_node->root_dir = NULL;

  file_node->backup_cnt = 0;

  file_node->backup_head = (backupNode*)malloc(sizeof(backupNode));
  file_node->backup_head->prev_backup = NULL;
  file_node->backup_head->next_backup = NULL;
  file_node->backup_head->root_file = file_node;

  file_node->prev_file = NULL;
  file_node->next_file = NULL;
}

fileNode *fileNode_get(fileNode *file_head, char *file_name) {
  fileNode *curr_file = file_head->next_file;
  
  while(curr_file != NULL) {
    if(curr_file != NULL && !strcmp(file_name, curr_file->file_name)) {
      return curr_file;
    }
    curr_file = curr_file->next_file;
  }
  return NULL;
}

//Function to insert a file node into the list
fileNode *fileNode_insert(fileNode *file_head, char *file_name, char *dir_path) {
  fileNode *curr_file = file_head;
  fileNode *next_file, *new_file;
  
  while(curr_file != NULL) {
    next_file = curr_file->next_file;

    if(next_file != NULL && !strcmp(file_name, next_file->file_name)) {
      return curr_file->next_file;
    } else if(next_file == NULL || strcmp(file_name, next_file->file_name) < 0) {
      new_file = (fileNode*)malloc(sizeof(fileNode));
      fileNode_init(new_file);

      new_file->root_dir = file_head->root_dir;

      strcpy(new_file->file_name, file_name);
      strcpy(new_file->file_path, dir_path);
      strcat(new_file->file_path, file_name);
      
      new_file->prev_file = curr_file;
      new_file->next_file = next_file;
      
      if(next_file != NULL) {
        next_file->prev_file = new_file;
      }
      curr_file->next_file = new_file;

      file_head->root_dir->file_cnt++;

      return curr_file->next_file;
    }
    curr_file = next_file;
  }
  return NULL;
}

//Function to init backup node
void backupNode_init(backupNode *backup_node) {
  backup_node->root_version_dir = NULL;
  backup_node->root_file = NULL;

  backup_node->prev_backup = NULL;
  backup_node->next_backup = NULL;
}

backupNode *backupNode_get(backupNode *backup_head, char *backup_time) {
  backupNode *curr_backup = backup_head->next_backup;

  while(curr_backup != NULL) {
    if(curr_backup != NULL && !strcmp(backup_time, curr_backup->backup_time)) {
      return curr_backup;
    }
    curr_backup = curr_backup->next_backup;
  }

  return NULL;
}

//Funciton to insert backup node in correct position in tree list
backupNode *backupNode_insert(backupNode *backup_head, char *backup_time, char *file_name, char *dir_path, char *backup_path) {
  backupNode *curr_backup = backup_head;
  backupNode *next_backup, *new_backup;

  while(curr_backup != NULL) {
    next_backup = curr_backup->next_backup;
    if(next_backup == NULL)  {
         if(strcmp(backup_time, curr_backup->backup_time) != 0){
	      new_backup = (backupNode*)malloc(sizeof(backupNode));
	      backupNode_init(new_backup);

	      new_backup->root_file = backup_head->root_file;
	      
	      strcpy(new_backup->origin_path, dir_path);
	      strcat(new_backup->origin_path, file_name);
	      strcpy(new_backup->backup_path, backup_path);
	      strcpy(new_backup->backup_time, backup_time);
	      
	      new_backup->prev_backup = curr_backup;
	      new_backup->next_backup = next_backup;
	      
	      if(next_backup != NULL) {
		next_backup->prev_backup = new_backup;
	      }
	      curr_backup->next_backup = new_backup;

	      backup_head->root_file->backup_cnt++;
	      
	      return curr_backup->next_backup;
         }
    }
    curr_backup = curr_backup->next_backup;
  }
  return NULL;
}
