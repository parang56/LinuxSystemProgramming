void help(int cmd_bit) {
  if(!cmd_bit) {
    printf("Usage: \n");
  }

  if(!cmd_bit || cmd_bit & CMD_ADD) {
    printf("%s add <PATH> : record path to staging area, path will tracking modification\n", (cmd_bit?"Usage:":"  >"));
  }

  if(!cmd_bit || cmd_bit & CMD_REMOVE) {
    printf("%s remove <PATH> : record path to staging area, path will not tracking modification\n", (cmd_bit?"Usage:":"  >"));
  }

  if(!cmd_bit || cmd_bit & CMD_STATUS) {
    printf("%s status : show staging area status\n", (cmd_bit?"Usage:":"  >"));
  }
  
  if(!cmd_bit || cmd_bit & CMD_COMMIT) {
    printf("%s commit <NAME> : backup staging area with commit name\n", (cmd_bit?"Usage:":"  >"));
  }

  if(!cmd_bit || cmd_bit & CMD_REVERT) {
    printf("%s revert <NAME> : recover commit version with commit name\n", (cmd_bit?"Usage:":"  >"));
  }

  if(!cmd_bit || cmd_bit & CMD_LOG) {
    printf("%s log : show commit log\n", (cmd_bit?"Usage:":"  >"));
  }

  if(!cmd_bit || cmd_bit & CMD_HELP) {
    printf("%s help : show commands for program\n", (cmd_bit?"Usage:":"  >"));
  }
  
  if(!cmd_bit || cmd_bit & CMD_EXIT) {
    printf("%s exit program\n", (cmd_bit?"Usage:":"  >"));
  }
}

int help_process(command_parameter *parameter) {
  if(parameter->filename == NULL) {
    help(0);
  } else if(!strcmp(parameter->filename, "add")) {
    help(CMD_ADD);
  } else if(!strcmp(parameter->filename, "remove")) {
    help(CMD_REMOVE);
  } else if(!strcmp(parameter->filename, "status")) {
    help(CMD_STATUS);
  } else if(!strcmp(parameter->filename, "commit")) {
    help(CMD_COMMIT);
  } else if(!strcmp(parameter->filename, "revert")) {
    help(CMD_REVERT);
  } else if(!strcmp(parameter->filename, "log")) {
    help(CMD_LOG);
  } else if(!strcmp(parameter->filename, "help")) {
    help(CMD_HELP);
  } else if(!strcmp(parameter->filename, "exit")) {
    help(CMD_EXIT);
  } else {
    fprintf(stderr, "ERROR: invalid command -- '%s'\n", parameter->filename);
    return -1;
  }

  return 0;
}
