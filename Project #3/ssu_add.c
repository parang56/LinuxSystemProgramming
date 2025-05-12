#include "ssu_header.h"

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
	//Eg. The code block above makes only till /a/b, this line here makes /a/b/c
	mkdir(directory, 0777);
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

//Function to remove first part of path, i.e. /testdir/a/b -> /a/b
const char* remove_first_segment(const char* path) {
	const char* slash = strchr(path, '/');
	if(slash != NULL){
		const char* second_slash = strchr(slash + 1, '/');
		if(second_slash != NULL)
			return second_slash;
	}
	return path;
}

// Function to backup file to backup dir
int backup_file(char* origin_path, char* backup_path, char* time_str, int pid, backupNode* curr_backup, fileNode* curr_file) {
  int fd1, fd2;
  FILE* fp1;
  FILE* fp2;
  int len;
  char buf[BUF_MAX*2];
  char backup_file_path[PATH_MAX*2];
  char temp_path[PATH_MAX] = "";
  char tmp_backupPath[PATH_MAX] = "";

  //Find diverging path of exePATh and origin_path (return the part after the same parts)
  char *result = findDivergingPath(exePATH, origin_path);
  if(result[0] != '/')
	  sprintf(temp_path, "/%s", result);
  else
  	  strcat(temp_path, result);
  	  
  strcpy(temp_path, remove_first_segment(temp_path));
  strcpy(tmp_backupPath, backup_dir_path);
  strcat(tmp_backupPath, temp_path);
  
  char tmp_dirname_backupPath[PATH_MAX];
  strcpy(tmp_dirname_backupPath, tmp_backupPath);
  
  strcpy(tmp_dirname_backupPath,dirname(tmp_dirname_backupPath));
  
  //Create empty directory recursively for the path parameter
  create_directory(tmp_dirname_backupPath);
  
  sprintf(backup_file_path, "%s/%s_%s", tmp_dirname_backupPath, get_file_name(origin_path), time_str);
  
  //Set Node's backuppath to backup_file_path
  strcpy(curr_backup->backup_path, backup_file_path);
	
  if((fd1 = open(origin_path, O_RDONLY)) == -1) {
    fprintf(stderr, "ERROR: open error for '%s'\n", origin_path);
    RemoveEmptyDirRecursive(tmp_dirname_backupPath);
    return -1;
  }
  
  if((fd2 = open(backup_file_path, O_WRONLY|O_CREAT|O_TRUNC, 0777)) == -1) {
    fprintf(stderr, "ERROR: open error for '%s'\n", backup_file_path);
    return -1;
  }
  
  //copy file and increase insertions cnt
  while(len = read(fd1, buf, BUF_MAX)) {
    write(fd2, buf, len);
  }
  
  //sync backup file's mtime and atime to original's time  
  sync_file_attributes(origin_path, backup_file_path);

  close(fd1);
  close(fd2);
  
  //remove empty files
  RemoveEmptyDirRecursive(tmp_dirname_backupPath);
  
  return 1;
}

//Function to exit (kill daemon) if signal SIGUSR1 is received
void handle_sigusr1(int sig){
	exit(0);
}

//Function that creates daemon process for path
int add_file(char* origin_path, command_parameter *parameter) {
  char *buf = (char *)malloc(sizeof(char *) * BUF_MAX);
  backup_log_path = (char *)malloc(sizeof(char *) * PATHMAX);
  backup_dir_path = (char *)malloc(sizeof(char *) * PATHMAX);
  char *time_str;
  char *now_time_str;
  char backup_file_path[PATH_MAX];
  struct stat path_stat;
  pid_t pid;
  int fd, maxfd;
  
  FILE *fp = NULL;
  FILE *backup_log_fp = NULL;
  
  if ((pid = fork()) < 0) {
      fprintf(stderr, "fork error\n");
      exit(1);
  } else if (pid != 0) {
      //Print to terminal
      printf("monitoring started (%s) : %d\n", origin_path, pid);
      exit(0); // Parent exits
  } 
  
  pid = getpid();
  
  FILE* file = fopen(moniter_list_PATH, "a");
  if((file == NULL)) {
     fprintf(stderr, "ERROR: open error for '%s'\n", moniter_list_PATH);
     return -1;
  }
  
  sprintf(buf, "%d : %s", pid, origin_path);
  
  //Print to moniter_list.log
  fprintf(file, "%s\n", buf);
  fclose(file);
  
  //Make .log path and dir path for the log
  sprintf(backup_log_path, "%s/%d.log", backupPATH, pid);  
  sprintf(backup_dir_path, "%s/%d", backupPATH, pid);  
  
  //Make directory for the dir path
  mkdir(backup_dir_path, 0777);
  
  // Attempt to open a file to write log messages.
  fp = fopen(backup_log_path, "w");
  if (!fp) {
      fprintf(stderr, "error opening fp\n");
      exit(1);
  }
  fclose(fp); 
  
  // Create a new session + Make process into a daemon process
  setsid(); 
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  
  maxfd = getdtablesize();
  for (fd = 0; fd < maxfd; fd++)
      close(fd);
  umask(0);
  chdir("/");
  fd = open("/dev/null", O_RDWR);

  dup(0);
  dup(0);
  
  //Init backup_dir_list
  backup_dir_list = (dirNode*)malloc(sizeof(dirNode));
  dirNode_init(backup_dir_list);

  now_time_str = cvt_time_2_str(time(NULL));

  //Register signal handler for SIGUSR1
  signal(SIGUSR1, handle_sigusr1);

  while (1) {
  	//Make or update tree based on origin_path 
	if(parameter->commandopt & OPT_D || parameter->commandopt & OPT_R) {
		backup_list_dir(origin_path, "", now_time_str, parameter->commandopt, backup_dir_list);
	}
	else{
		backup_list_insert(backup_dir_list, now_time_str, origin_path, "");
	}
	//Go through tree to log [create], [modify], and [remove] backup files.
	go_through_tree(backup_dir_list, parameter->commandopt, pid, backup_dir_path);

	//print_list(backup_dir_list, 0, 0);

	// Sleep for <TIME> seconds (default : 1)
	sleep(parameter->time);
  }
  
  return 1;
}

//Function to execute add command
int add_process(command_parameter *parameter) {

  //Set full_path for child process
  if((full_path = cvt_path_2_realpath(parameter->filename)) == NULL) {
        fprintf(stderr, "ERROR: '%s' is wrong path\n", parameter->filename);
        exit(1);
  }

  add_file(full_path, parameter);

  return 0;
}
