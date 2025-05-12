#include "ssu_header.h"

char* make;
bool printmore = false;
bool secondflag = false;

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

//Print list for a directory path
void print_list(dirNode* dirList, int depth, int last_bit) {
  dirNode* curr_dir = dirList->subdir_head->next_dir;
  fileNode* curr_file = dirList->file_head->next_file;

  while(curr_dir != NULL && curr_file != NULL) {
    if(strcmp(curr_dir->dir_name, curr_file->file_name) < 0) {
      print_depth(depth, last_bit);
      printf("%s/\n", curr_dir->dir_name);
      print_list(curr_dir, depth+1, last_bit);
      curr_dir = curr_dir->next_dir;
    } else {
      print_depth(depth, last_bit);
      printf("%s\n", curr_file->file_name);
      
      backupNode* curr_backup = curr_file->backup_head->next_backup;
      while(curr_backup != NULL) {
        
        print_depth(depth+1, (
        last_bit | ((curr_backup->next_backup == NULL)?(1 << depth+1):0)
        ));
        printf("%s\n", curr_backup->backup_time);
        curr_backup = curr_backup->next_backup;
      }
      curr_file = curr_file->next_file;
    }
  }
  
  while(curr_dir != NULL) {
    last_bit |= (curr_dir->next_dir == NULL)?(1 << depth):0;
    
    print_depth(depth, last_bit);
    strcat(make, curr_dir->dir_name);
    
    if(!strcmp(full_path, make)){
    	printmore = true;
    }
    
    strcat(make, "/");
    if(printmore == false){
    	print_list(curr_dir, depth, last_bit);
    }
    else{
    	if(secondflag == false){
    		printf("%s\n", full_path);
    		secondflag = true;
    	}
    	else{
    		printf("%s/\n", curr_dir->dir_name);
    	}
	print_list(curr_dir, depth+1, last_bit);
    }
    curr_dir = curr_dir->next_dir;
  }
  
  while(curr_file != NULL) {
    last_bit |= (curr_file->next_file == NULL)?(1 << depth):0;

    print_depth(depth, last_bit);
    
    //path is a single file
    if(secondflag == false){
      char *tmp = (char *)malloc(sizeof(char *) * PATHMAX);
      strcpy(tmp, full_path);
      tmp = dirname(tmp);
      
      //Print directory for the file
      printf("%s\n", tmp);
      
      print_depth(depth+1, (1 << depth+1));
      printf("%s\n", curr_file->file_name);
      secondflag = true;
      backupNode* curr_backup = curr_file->backup_head->next_backup;
      while(curr_backup != NULL) {
        printf("  ");
        print_depth(depth+1, (
          last_bit | ((curr_backup->next_backup == NULL)?(1 << depth+1):0)
        ));
        printf("%s\n", curr_backup->backup_time);
        curr_backup = curr_backup->next_backup;
      }
    }
    else{
      printf("%s\n", curr_file->file_name);
      backupNode* curr_backup = curr_file->backup_head->next_backup;
      while(curr_backup != NULL) {
        print_depth(depth+1, (
          last_bit | ((curr_backup->next_backup == NULL)?(1 << depth+1):0)
        ));
        printf("%s\n", curr_backup->backup_time);
        curr_backup = curr_backup->next_backup;
      }
    }
    
    curr_file = curr_file->next_file;
  }
}

//Function to create a list for a pid input.
void create_list_for_pid(int pid, char* path){
	FILE* file = fopen(path, "r");
	if(!file){
		fprintf(stderr, "%s is wrong path!\n", path);
		exit(1);
	}
	char time_str[22], process_str[12]; 
	char line[PATH_MAX * 2];
	char* originalPath = (char *)malloc(sizeof(char) * PATHMAX);
	char* time_and_process = (char *)malloc(sizeof(char) * PATHMAX);
	struct stat stat_buf;
	
	while(fgets(line, sizeof(line), file)) {
		if(sscanf(line, "%21[^]]]%11[^]]][%4096[^]]]", time_str, process_str, originalPath) == 3){
			int len_time = strlen(time_str);
			int len_process = strlen(process_str);
			time_str[len_time] = ']';
			time_str[len_time+1] = '\0';
			process_str[len_process] = ']';
			process_str[len_process+1] = '\0';
			
			sprintf(time_and_process, "%s %s", process_str, time_str);
			
			backup_list_insert(pid_list, time_and_process, originalPath, "For List");
		} else{
			fprintf(stderr, "Failed to parse line : %s\n", line);
		}
	}
	printmore = false;
	secondflag = false;
	make = originalPath = (char *)malloc(sizeof(char) * PATHMAX);
	full_path = return_path_for_pid(pid);

	print_list(pid_list, 0, 0);
}	

//Function called from ssu_sync if command was list
int list_process(command_parameter *parameter) {
	//Print monitor_list.log if no parameter except command
	if(parameter->filename == NULL){
		FILE* file = fopen(moniter_list_PATH, "r");
		if(!file){
			fprintf(stderr, "monitor_list.log not initalized!\n");
			exit(1);
		}
		
		char line[PATH_MAX * 2];
		char* originalPath = (char *)malloc(sizeof(char) * PATHMAX);
		int pid;
		while(fgets(line, sizeof(line), file)) {
			printf("%s", line);
		}
	}
	//Print the list for specific pid
	else{
		int pid;
		char* originalPath = (char *)malloc(sizeof(char) * PATHMAX);
		char* pidLogFile = (char *)malloc(sizeof(char) * PATHMAX);
		char* pidLogPath = (char *)malloc(sizeof(char) * PATHMAX);
		
		pid_list = (dirNode*)malloc(sizeof(dirNode));
		dirNode_init(pid_list);
		//Turn pid into int and check if it is only digits
		pid = atoi(parameter->filename);
		if(pid == 0){
			fprintf(stderr, "Usage: list [DAEMON_PID] : show daemon process list or dir tree\n");
			exit(1);
		}
		
		//Check if pid exists in monitor_list.log
		if((originalPath = return_path_for_pid(pid)) == NULL){
			fprintf(stderr, "ERROR: pid : \"%d\" does not exist in monitor_list.log!\n", pid);
			exit(1);
		}
		
		//Check if daemon process for pid actually exists
		if(pid_exists(pid) != 1){
			fprintf(stderr, "ERROR: daemon process for pid \"%d\" is already removed!\n", pid);
			exit(1);
		}
		
		strcpy(pidLogFile, parameter->filename);
		strcat(pidLogFile, ".log");
		
		strcpy(pidLogPath, backupPATH);
		strcat(pidLogPath, "/");
		strcat(pidLogPath, pidLogFile);
		
		//Make list from reading the pid's log
		create_list_for_pid(pid, pidLogPath);
	}
	
	exit(0);
}
