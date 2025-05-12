#include "ssu_header.h"

//Pass cmd_bit and print accordingly to the bit
void help(int cmd_bit) {
  if(!cmd_bit) {
    printf("Usage: \n");
  }

  if(!cmd_bit || cmd_bit & CMD_ADD) {
    printf("%s add <PATH> [OPTION]... : add new daemon process of <PATH> if <PATH> is file\n    -d : add new daemon process of <PATH> if <PATH> is directory\n    -r : add new daemon process of <PATH> recursive if <PATH> is directory\n    -t <TIME> : set daemon process time to <TIME> sec (default : 1sec)\n", (cmd_bit?"Usage:":"  >"));
  }

  if(!cmd_bit || cmd_bit & CMD_REMOVE) {
    printf("%s remove <DAEMON_PID> : delete daemon process with <DAEMON_PID>\n", (cmd_bit?"Usage:":"  >"));
  }

  if(!cmd_bit || cmd_bit & CMD_LIST) {
    printf("%s list [DAEMON_PID] : show daemon process list or dir tree\n", (cmd_bit?"Usage:":"  >"));
  }
  
  if(!cmd_bit || cmd_bit & CMD_HELP) {
    printf("%s help [COMMAND] : show commands for program\n", (cmd_bit?"Usage:":"  >"));
  }
  
  if(!cmd_bit || cmd_bit & CMD_EXIT) {
    printf("%s exit : exit program\n", (cmd_bit?"Usage:":"  >"));
  }
}

//Function that is called from ssu_sync when command is help
int help_process(command_parameter *parameter) {
  //If no second param for help, print all helps. Else print specified help command.
  if(parameter->filename == NULL) {
    help(0);
  } else if(!strcmp(parameter->filename, "add")) {
    help(CMD_ADD);
  } else if(!strcmp(parameter->filename, "remove")) {
    help(CMD_REMOVE);
  } else if(!strcmp(parameter->filename, "list")) {
    help(CMD_LIST);
  }  else if(!strcmp(parameter->filename, "help")) {
    help(CMD_HELP);
  } else if(!strcmp(parameter->filename, "exit")) {
    help(CMD_EXIT);
  } else {
    fprintf(stderr, "ERROR: invalid command -- '%s'\n", parameter->filename);
    return -1;
  }

  return 0;
}
