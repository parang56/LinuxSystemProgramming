#define OPENSSL_API_COMPAT 0x10100000L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
#include <ctype.h>

#define true 1
#define false 0

#define HASH_MD5  33

#define STUDENT_NUM 20202904

#define CMD_ADD      0b00001
#define CMD_REMOVE   0b00010
#define CMD_LIST     0b00100
#define CMD_HELP     0b01000
#define CMD_EXIT     0b10000
#define NOT_CMD	     0b00000

#define FILE_MAX 255
#define PATHMAX 4096
#define STRMAX 4096
#define BUF_MAX 4096

#define OPT_D		0b001
#define OPT_R		0b010
#define OPT_T		0b100
#define NOT_OPT         0b000

char exeNAME[PATHMAX];
char exePATH[PATHMAX];
char homePATH[PATHMAX];
char pwd_path[PATHMAX];
char backupPATH[PATHMAX];
char moniter_list_PATH[PATHMAX];
int hash;

char *full_path;
char *tmp;

typedef struct command_parameter {
  char *command;
  char *filename;
  int time;
  int commandopt;
  char *argv[10];
} command_parameter;

typedef struct _backupNode {
  struct _dirNode *root_version_dir;
  struct _fileNode *root_file;

  char backup_time[30];
  char origin_path[PATH_MAX];
  char backup_path[PATH_MAX];
  
  bool wasRemoved;

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

typedef struct pathList_ {
  struct pathList_ *next;
  struct pathList_ *prev;
  char path[FILE_MAX];

} pathList;

dirNode* backup_dir_list;
dirNode* pid_list;

char buffer[PATHMAX];

char *exe_path;
char *home_path;

char *backup_dir_path;
char* backup_log_path;

/*********** Functions and Variables ************/

//ssu_header.c
void ParameterInit(command_parameter *parameter);
char *substr(char *str, int beg, int end);
char *c_str(char *str);
char *cvt_path_2_realpath(char* path);
int md5(char *target_path, char *hash_result);
char *get_file_name(char* path);
char *Tokenize(char *str, char *del);
void pid_log_append(int pid, char* origin_path, char* flag);
void go_through_tree(dirNode* dirList, int opt_bit, int pid, char* backup_dir_path);
int go_through_monitor_log(char* full_path);
int check_tree(char* path);
char **GetSubstring(char *str, int *cnt, char *del);
int path_list_init(pathNode *curr_path, char *path);
char* findDivergingPath(const char *path1, const char *path2);
int cvtHash(char *target_path, char *hash_result);
int cmpHash(char *path1, char *path2);
int backup_list_dir(char* origin_path, char* backup_path, char* time_str, int opt_bit, dirNode* backup_list);
int check_path_access(char* path, int opt);
int Path_Type(const char* path);
char *cvt_time_2_str(time_t time);
void RemoveEmptyDirRecursive(char* path);
char* get_current_time(void);
int pid_exists(pid_t pid);

//ssu_add.c functions
void create_directory(const char* directory);
int sync_file_attributes(const char *origin_path, const char *backup_path);
const char* remove_first_segment(const char* path);
int backup_file(char* origin_path, char* backup_path, char* time_str, int pid, backupNode* curr_backup, fileNode* curr_file);
void handle_sigusr1(int sig);
int add_file(char* origin_path, command_parameter *parameter);
int add_process(command_parameter *parameter);

//ssu_remove.c
char* return_path_for_pid(int pid_check);
int remove_directory(const char *path);
int removeFiles(char* pid);
int update_monitor_log(char* id_to_remove);
int remove_process(command_parameter *parameter);

//ssu_list.c
void print_depth(int depth, int is_last_bit);
void print_list(dirNode* dirList, int depth, int last_bit);
void create_list_for_pid(int pid, char* path);
int list_process(command_parameter *parameter);

//ssu_help.c
void help(int cmd_bit);
int help_process(command_parameter *parameter);

//ssu_struct.c
int backup_list_insert(dirNode* dirList, char* backup_time, char* path, char* backup_path);
void dirNode_init(dirNode *dir_node);
dirNode *dirNode_insert(dirNode* dir_head, char* dir_name, char* dir_path);
dirNode *dirNode_append(dirNode* dir_head, char* dir_name, char* dir_path);
void fileNode_init(fileNode *file_node);
fileNode *fileNode_insert(fileNode *file_head, char *file_name, char *dir_path);
void backupNode_init(backupNode *backup_node);
backupNode *backupNode_insert(backupNode *backup_head, char *backup_time, char *file_name, char *dir_path, char *backup_path);

//ssu_sync.c
int ParameterProcessing(int argcnt, char **arglist, int command, command_parameter *parameter);
void Init();
void CommandFun(char **arglist);
void CommandExec(command_parameter parameter);
void Prompt();

