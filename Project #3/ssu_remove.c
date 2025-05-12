#include "ssu_header.h"

//Function to return path for corresponding pid
char* return_path_for_pid(int pid_check){
	FILE* file = fopen(moniter_list_PATH, "r");
	if(!file){
		fprintf(stderr, "monitor_list.log not initalized!\n");
		exit(1);
	}
	
	char line[PATH_MAX * 2];
	char* originalPath = (char *)malloc(sizeof(char) * PATHMAX);
	int pid;
	while(fgets(line, sizeof(line), file)) {
		if(sscanf(line, "%d : %4096[^\n]", &pid, originalPath) == 2) {
			if(pid_check == pid){
				return originalPath;
			}
		}
	}
	return NULL;
}

//Called from removeFiles() to remove a backup directory recursively
int remove_directory(const char *path) {
    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    int r = -1;

    if (d) {
        struct dirent *p;

        r = 0;

        while (!r && (p=readdir(d))) {
            int r2 = -1;
            char *buf;
            size_t len;

            // Skip the names "." and ".." as we don't want to recurse on them.
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) {
                continue;
            }

            len = path_len + strlen(p->d_name) + 2; 
            buf = malloc(len);

            if (buf) {
                struct stat statbuf;

                snprintf(buf, len, "%s/%s", path, p->d_name);

                if (!stat(buf, &statbuf)) {
                    if (S_ISDIR(statbuf.st_mode)) {
                        r2 = remove_directory(buf);
                    } else {
                        r2 = unlink(buf);
                    }
                }
                free(buf);
            }
            r = r2;
        }
        closedir(d);
    }

    if (!r) {
        r = rmdir(path);
    }

    return r;
}

//Function to remove backup dir and file with PID name
int removeFiles(char* pid){
	struct dirent **namelist;
	struct stat statbuf;
	int cnt;
	char *tmppath = (char *)malloc(sizeof(char *) * PATHMAX);
	
	if((cnt = scandir(backupPATH, &namelist, NULL, alphasort)) == -1) {
		fprintf(stderr, "ERROR: scandir error for %s\n", backupPATH);
		return 0;
  	}
  	
  	//Go through backup directory and remove names with the pid in it
  	for(int i = 0; i < cnt; i++) {
		if(!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;
		sprintf(tmppath, "%s/%s", backupPATH, namelist[i]->d_name);
		if (lstat(tmppath, &statbuf) < 0) {
			fprintf(stderr, "ERROR: lstat error for %s\n", tmppath);
			return 0;
		}
		
		//If the path has the pid in its name remove it
		if(strstr(tmppath, pid) != NULL){
			if(S_ISDIR(statbuf.st_mode)){
				remove_directory(tmppath);
			}
			else if(S_ISREG(statbuf.st_mode)){
				remove(tmppath);
			}
		}
	}
}

//Remove the pid line from monitor_list.log
int update_monitor_log(char* id_to_remove){
    char line[PATH_MAX + 10];
    long length;
    char *buffer;
    size_t total_length = 0;
    int flag = 0;

    FILE* file = fopen(moniter_list_PATH, "r+");
    if (file == NULL) {
        perror("Failed to open file");
        return 1;
    }

    //get file size
    fseek(file, 0, SEEK_END);
    length = ftell(file);
    rewind(file);

    buffer = (char *)malloc(sizeof(char) * length);
    if (buffer == NULL) {
        fprintf(stderr, "Memory allocation failed");
        fclose(file);
        return 1;
    }

    //Read file into buffer
    char *buffer_ptr = buffer;
    while (fgets(line, PATH_MAX + 10, file) != NULL) {
        //Append line to buffer unless it's the one to remove (Dont append the one to remove)
        char id_string[PATH_MAX + 10];
        snprintf(id_string, sizeof(id_string), "%s :", id_to_remove);
        if (strncmp(line, id_string, strlen(id_string)) != 0) {
            size_t line_len = strlen(line);
            memcpy(buffer_ptr, line, line_len);
            buffer_ptr += line_len;
            total_length += line_len;
        } else{
            flag = 1;
        }
    }

    //Truncate file and write the new data
    ftruncate(fileno(file), 0);
    rewind(file);
    fwrite(buffer, sizeof(char), total_length, file);
    fclose(file);
    free(buffer);
    
    return flag;
}

//Called from ssu_sync if command was remove
int remove_process(command_parameter *parameter) {

	char* pid = parameter->filename;
	int pid_int;
	
	//parameter->filename is string pid, so turn it into int
	pid_int = atoi(parameter->filename);
	if(pid_int == 0){
		fprintf(stderr, "usage : remove <pid>\n");
		return -1;
	}
	
	//get the corresponding path for the pid
	char* originalPath = return_path_for_pid(pid_int);
	if(originalPath == NULL){
		fprintf(stderr, "ERROR: pid : \"%d\" does not exist in monitor_list.log!\n", pid_int);
		return -1;
	}
	
	//Remove pid from monitor_list.log. If no corresponding pid, print error
	if(update_monitor_log(pid) == 0){
		fprintf(stderr, "ERROR: pid : \"%d\" does not exist in monitor_list.log!\n", pid_int);
		return -1;
	}
	
	//Send SIGUSR1 signal to daemon to kill the daemon
	if(pid_exists(pid_int) == 1) {
		kill(pid_int, SIGUSR1);
	}
	
	printf("monitoring ended (%s) : %s\n", originalPath, pid);
	
	//Remove the backup log and files whose name have pid in it
	removeFiles(pid);

	return 0;
}
