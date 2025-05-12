#include "ssu_header.h"

char *commanddata[10]={
    "add",
    "remove",
    "list",
    "help"
};

//Function to process parameters and check if values and paths are okay
int ParameterProcessing(int argcnt, char **arglist, int command, command_parameter *parameter) {
	int option = 0;
	
	switch(command) {
		case CMD_ADD: {
			struct stat buf;
			
			//If path is not included, error
			if(parameter->filename == NULL || !strcmp(parameter->filename,"")) {
				fprintf(stderr, "ERROR: <PATH> is not included\n");
				help(CMD_ADD);
				return -1;
			}
			  
			free(full_path);
			full_path = NULL;
			
			//Convert relative path to full path
			if((full_path = cvt_path_2_realpath(parameter->filename)) == NULL) {
				fprintf(stderr, "ERROR: '%s' is wrong path\n", parameter->filename);
				return -1;
			}
			
			//Check if path already exists
			if(go_through_monitor_log(full_path) == -1){
				fprintf(stderr, "ERROR: pid already exists for path!\n");
				return -1;
			}
			
			//If path is too long, print error and return
			if(strlen(full_path) > PATHMAX){
				fprintf(stderr, "ERROR: Path is too long!\n");
				return -1;
			}
			
			//Not normal file or directory
			if(Path_Type(full_path) == -1){
			    	fprintf(stderr, "ERROR: '%s' is wrong path\n", parameter->filename);
			    	return -1;
			}
			
			//Check lstat on path
			if (lstat(full_path, &buf) < 0) {
				fprintf(stderr, "ERROR: lstat error for %s\n", parameter->filename);
				return -1;
			}
			    
			// Path length is bigger than 4096 bytes -> error
			if(strlen(full_path) > PATHMAX){
			    	fprintf(stderr, "ERROR: <PATH> is too long\n");
				return -1;
			}
			    
			//If file doesn't exist
			if(access(full_path, F_OK) == -1) {
				fprintf(stderr, "ERROR: '%s' is wrong path\n", parameter->filename);
				return -1;
			}
			
			optind = 0;
			//Check if options are okay
			while((option = getopt(argcnt, arglist, "drt:")) != -1) {
				if(option != 'd' && option != 'r' && option != 't') {
					help(CMD_ADD);
					return -1;
				}
				switch(option){
					case 'd':
						parameter->commandopt |= OPT_D;
						break;
					case 'r':
						parameter->commandopt |= OPT_R;
						break;
					case 't':
						parameter->commandopt |= OPT_T;
						
						//If there is a character that is not a digit, print error and return
						for(int i=0; optarg[i] != '\0'; i++){
							if(!isdigit(optarg[i])){
								fprintf(stderr, "Error: option for '-t' must be a natural number!\n");
								return -1;
							}
						}
						
						//If number is <= 0, print error and return
						parameter->time = atoi(optarg);
						
						if(parameter->time <= 0){
							fprintf(stderr, "Error: option for '-t' must be a natural number!\n");
							return -1;
						}
						break;
				}
			}
			
			//If file and if there is a -d or -r option, print error.
			if(S_ISREG(buf.st_mode)){
				if(parameter->commandopt & OPT_D || parameter->commandopt & OPT_R){
					fprintf(stderr, "Error : Used -d or -r when input is file!\n");
					return -1;
				}
			}
			
			//If dir and if there is no -d or -r option, print error.
			if(S_ISDIR(buf.st_mode)){
				if(!(parameter->commandopt & OPT_D) && !(parameter->commandopt & OPT_R)){
					fprintf(stderr, "Error : Need -d or -r when input is directory!\n");
					return -1;
				}
			}
			
			//If no access to path exit (e.g. .repo or root)
			if(check_path_access(full_path, parameter->commandopt) == -1) {
				return -1;
			}
			
			break;
		}
		case CMD_REMOVE: {
			struct stat statbuf;
			if(parameter->filename == NULL || !strcmp(parameter->filename,"")) {
				fprintf(stderr, "ERROR: <PID> is not included\n");
				help(CMD_REMOVE);
				return -1;
			}
			break;
		}
		
		return 1; 
	}
}

//Executes at start of main
void Init() {
	getcwd(exePATH, PATHMAX);
	sprintf(homePATH, "%s", getenv("HOME"));
	strcat(backupPATH, homePATH);
	strcat(backupPATH, "/backup");
	
	sprintf(pwd_path, "%s", exePATH);
  	
  	//Try opening dir, if it exists -> close, if it doesnt exist -> make, or else error
	DIR* dir = opendir(backupPATH);
	if(dir){
		closedir(dir);
	}
	else if (ENOENT == errno) {
		mkdir(backupPATH, 0777);
	}
	else{
		fprintf(stderr, "Could not create directory: %s\n", backupPATH);
	}
	
	//Make path to monitor_list .log
	strcat(moniter_list_PATH, backupPATH);
	strcat(moniter_list_PATH, "/monitor_list.log");
	
	//Make pid.log file
	FILE *file = fopen(moniter_list_PATH, "a");
	fclose(file);
}

// In the child process, excute given command to the process
void CommandFun(char **arglist) {
  int (*commandFun)(command_parameter * parameter);
  command_parameter parameter={arglist[0], arglist[1], atoi(arglist[2]), atoi(arglist[3])};
  
  if(!strcmp(parameter.command, commanddata[0])) {
    commandFun = add_process;
  } else if(!strcmp(parameter.command, commanddata[1])) {
    commandFun = remove_process;
  } else if(!strcmp(parameter.command, commanddata[2])) {
    commandFun = list_process;
  } else if(!strcmp(parameter.command, commanddata[3])) {
    commandFun = help_process;
  }
  
  if(commandFun(&parameter) != 0) {
    exit(1);
  }
}

//Add "command" to parameter -> indicating that we will run a command on the child process
void CommandExec(command_parameter parameter) {
  pid_t pid;

  parameter.argv[0] = "command";
  parameter.argv[1] = (char *)malloc(sizeof(char *) * 32);
  sprintf(parameter.argv[1], "%d", hash);

  parameter.argv[2] = parameter.command;
  parameter.argv[3] = parameter.filename;
  parameter.argv[4] = (char *)malloc(sizeof(char *) * 32);
  sprintf(parameter.argv[4], "%d", parameter.time);
  parameter.argv[5] = (char *)malloc(sizeof(char *) * 32);
  sprintf(parameter.argv[5], "%d", parameter.commandopt);
  parameter.argv[6] = (char *)0;
  
  //Create child process and execute command
  if((pid = fork()) < 0) {
    fprintf(stderr, "ERROR: fork error\n");
    exit(1);
  } else if(pid == 0) {
    execv(exeNAME, parameter.argv);
    exit(0);
  } else {
    pid = wait(NULL);
  }
}

// This is executed in the parent process
void Prompt() {  
  char input[STRMAX*2];
  int argcnt = 0;
  char **arglist = NULL;
  int command;
  int option;
  command_parameter parameter={(char *)0, (char *)0, 1, 0};
  
  //Continue to print prompt until exit
  while(true) {
    arglist = NULL;
    argcnt = 0;
    printf("%d> ", STUDENT_NUM);
    
    fgets(input, sizeof(input), stdin);
    
    input[strlen(input) - 1] = '\0';

    arglist = GetSubstring(input, &argcnt, " ");
    
    //If empty prompt, print the prompt again
    if(argcnt == 0){
      continue;
    }
    
    //If command is correct, set command. If command was incorrect, print help usage
    if (strcmp(arglist[0], "add") == 0) {
    //check options using getopt
        command = CMD_ADD;
    } else if (strcmp(arglist[0], "remove") == 0) {
        command = CMD_REMOVE;
    } else if (strcmp(arglist[0], "list") == 0) {
        command = CMD_LIST;
    }  else if (strcmp(arglist[0], "help") == 0) {
        command = CMD_HELP;
    } else if(!strcmp(arglist[0], "exit")) {
	command = CMD_EXIT;
    	exit(0);
    } else {
	command = NOT_CMD;
    }
    
    //Run ComandExec if correct command
    if(command & (CMD_ADD | CMD_REMOVE | CMD_LIST | CMD_HELP)) {
      ParameterInit(&parameter);
      parameter.command = arglist[0];
      parameter.filename = (argcnt > 1) ? arglist[1] : NULL;
      
      if(ParameterProcessing(argcnt, arglist, command, &parameter) != -1) {
       	CommandExec(parameter);
      }
    } 
    //Run help if command was help or incorrect command
    else if(command == NOT_CMD) {
      help(0);
    }
    
    printf("\n");
  }
}

int main(int argc, char* argv[]) {
  Init();
  //Executed in child process with command
  if(!strcmp(argv[0], "command")) {
    CommandFun(argv+2);
  } 
  //Execute in parent process
  else {
    strcpy(exeNAME, argv[0]);
    
    hash = HASH_MD5;
    Prompt();
  }
	
  exit(0);
}
