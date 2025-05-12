#include "ssu_header.h"
#include "ssu_help.h"
#include "ssu_add.h"
#include "ssu_struct.h"
#include "ssu_init.h"
#include "ssu_remove.h"
#include "ssu_status.h"
#include "ssu_commit.h"
#include "ssu_log.h"

#define STUDENT_NUM 20202904
#define COMMAND_LEN 256

//Executes at start of main
void Init() {
	getcwd(exePATH, PATHMAX);
	//sprintf(homePATH, "%s", getenv("HOME"));
	strcat(repoPath, exePATH);
	strcat(repoPath, "/.repo");
	
	sprintf(pwd_path, "%s", exePATH);
  
  	//Try opening dir, if it exists -> close, if it doesnt exist -> make, or else error
	DIR* dir = opendir(repoPath);
	if(dir){
		closedir(dir);
	}
	else if (ENOENT == errno) {
		mkdir(repoPath, 0777);
	}  
	else{
		fprintf(stderr, "Could not create directory: %s\n", repoPath);
	}
	
	//Make path to commit log
	strcat(commit_log_PATH, repoPath);
	strcat(commit_log_PATH, "/.commit.log");
	
	//Make path to staging log
	strcat(staging_log_PATH, repoPath);
	strcat(staging_log_PATH, "/.staging.log");
	
	//Make commit log file, Make staging log file
	FILE *file = fopen(commit_log_PATH, "a");
	FILE *file2 = fopen(staging_log_PATH, "a");
	
	//Make linked list (tree) from staging_log_Path
	init();
	
	if(file != NULL){
		fclose(file);
	}
	if(file2 != NULL){
		fclose(file2);
	}
}

// In the child process, excute given command to the process
void CommandFun(char **arglist) {
  int (*commandFun)(command_parameter * parameter);
  command_parameter parameter={arglist[0], arglist[1], arglist[2]};
  
  if(!strcmp(parameter.command, commanddata[0])) {
    commandFun = add_process;
  } else if(!strcmp(parameter.command, commanddata[1])) {
    commandFun = remove_process;
  } else if(!strcmp(parameter.command, commanddata[2])) {
    commandFun = status_process;
  } else if(!strcmp(parameter.command, commanddata[3])) {
    commandFun = commit_process;
  } else if(!strcmp(parameter.command, commanddata[5])) {
    commandFun = log_process;
  } else if(!strcmp(parameter.command, commanddata[6])) {
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
  parameter.argv[4] = parameter.tmpname;
  parameter.argv[5] = (char *)0;
  
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
  char input[STRMAX];
  int argcnt = 0;
  char **arglist = NULL;
  int command;
  command_parameter parameter={(char *)0, (char *)0, (char *)0, 0};
  
  //Continue to print prompt until exit
  while(true) {
    printf("%d> ", STUDENT_NUM);
    fgets(input, sizeof(input), stdin);
    input[strlen(input) - 1] = '\0';
    
    if(input == NULL){
        printf("\n");
    	continue;
    }
    if((arglist = GetSubstring(input, &argcnt, " \t")) == NULL) {
      continue;
    }

    //If empty prompt, print the prompt again
    if(argcnt == 0) continue;

    //If command is correct, set command. If command was incorrect, print help usage
    if (strcmp(arglist[0], "add") == 0) {
        command = CMD_ADD;
    } else if (strcmp(arglist[0], "help") == 0) {
        command = CMD_HELP;
    } else if (strcmp(arglist[0], "remove") == 0) {
        command = CMD_REMOVE;
    } else if (strcmp(arglist[0], "status") == 0) {
        command = CMD_STATUS;
    } else if (strcmp(arglist[0], "commit") == 0) {
        command = CMD_COMMIT;
    } else if (strcmp(arglist[0], "log") == 0) {
        command = CMD_LOG;
    } else if(!strcmp(arglist[0], "exit")) {
	command = CMD_EXIT;
    	exit(0);
    } else {
	command = NOT_CMD;
    }
    
    //Run ComandExec if correct command
    if(command & (CMD_ADD | CMD_REMOVE | CMD_STATUS | CMD_COMMIT | CMD_LOG | CMD_HELP)) {
      ParameterInit(&parameter);
      parameter.command = arglist[0];
      parameter.filename = (argcnt > 1) ? arglist[1] : NULL;
      CommandExec(parameter);
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
