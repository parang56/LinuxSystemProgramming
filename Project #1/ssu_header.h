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

#define true 1
#define false 0

#define HASH_MD5  33

#define HASH_BACKUP  1
#define HASH_REMOVE  2
#define HASH_RECOVER 3
#define HASH_LIST    4
#define HASH_HELP    5

#define NAMEMAX 255
#define PATHMAX 4096
#define STRMAX 4096

char exeNAME[PATHMAX];
char exePATH[PATHMAX];
char homePATH[PATHMAX];
char backupPATH[PATHMAX];
int hash;

int RemoveEmptyDir(char *);
int isFileBackedUp(char*, char*);
int Path_Type(const char*);
void Init();
void BackUpFile(char*, char*, char*);
void BackUpDir(char*, char*, char*);
char* MakeBackUpDir(char*);
int BackUpCommand(char*, char*, char*);
int md5(char*, char*);
void getPathUpToTimeStamp(const char*, char*);
char* getFileName(char*);
char* extractAfterName(char*, char*);
void create_directory(const char*);

bool DirNonExist = false;
bool isOriginalNew = false;

typedef struct LogNode {
	char originalPath[PATH_MAX];
	char backupPath[PATH_MAX];
	char timeStamp[16];
	int tmpRemoveNum;
	bool wasRemoved;
	bool hasChecked;
	bool originalRemoved;
	struct LogNode *next;
} LogNode;

typedef struct LogLinkedList{
	LogNode *head;
	LogNode *tail;
} LogLinkedList;

//Init log linked list
LogLinkedList* createLogLinkedList() {
	LogLinkedList* list = (LogLinkedList*)malloc(sizeof(LogLinkedList));
	if(list){
		list->head = NULL;
		list->tail = NULL;
	}
	return list;
}

//Function to create new node
LogNode* createNewLogNode(char* originalPath, char* backupPath){
	LogNode* newNode = (LogNode*)malloc(sizeof(LogNode));
	if(newNode){
		strncpy(newNode->originalPath, originalPath, PATH_MAX);
		strncpy(newNode->backupPath, backupPath, PATH_MAX);
		newNode->next = NULL;
	}
	return newNode;
}

//Count number of slashes in path for depth
int getPathDepth(const char* path){
	int depth = 0;
	while(*path) {
		if(*path == '/') depth++;
		path++;
	}
	return depth;
}

//Compare two path's depths and lexicographical order
int comparePaths(const void* a, const void* b) {
	const char* path1 = *(const char**)a;
	const char* path2 = *(const char**)b;
	
	int depth1 = getPathDepth(path1);
	int depth2 = getPathDepth(path2);
	
	//First, compare by directory depths
	if(depth1 != depth2){
		return depth1 - depth2;
	}
	
	//If depths are equal, compare lexicographically	
	return strcmp(path1, path2);
}

char** sortPathsBFS(char** paths, int numPaths){
	qsort(paths, numPaths, sizeof(char*), comparePaths);
	
	return paths;
}

//For bfs, remove duplicates that we stored and sort the paths.
char** removeDuplicatesAndBFS(char** paths, int *numPaths){
	if(numPaths == 0) return NULL;
	
	//Sort paths
	sortPathsBFS(paths, *numPaths);
	
	char** uniquePaths = malloc(*numPaths * sizeof(char *));
	int uniqueCount = 0;
	
	//Always add first path
	uniquePaths[uniqueCount++] = strdup(paths[0]);
	
	//Iterate through sorted paths, add to uniquePaths if not dupliacted
	for(int i = 1; i<*numPaths; i++){
		if(strcmp(paths[i], paths[i-1]) != 0){
			uniquePaths[uniqueCount++] = strdup(paths[i]);
		}
	}
	
	for(int i=0; i<*numPaths; i++){
		free(paths[i]);
	}
	free(paths);
	
	*numPaths = uniqueCount;
	
	return uniquePaths;
}

//Add the commas to the bytes. ex: 1,234 bytes
char* addCommas(off_t num) {
	char temp[30]; 
	int tempIndex = 0, outIndex = 0;
	int digitCount = 0;
	
	if(num == 0){
		return strdup("0");
	}
	
	while(num>0) {
		if(digitCount == 3){
			temp[tempIndex++] = ',';
			digitCount = 0;
		}
		temp[tempIndex++] = (num % 10) + '0';
		num /= 10;
		digitCount++;
	}
	
	char* out = (char*)malloc(tempIndex+1);
	if(out == NULL){
		return NULL;
	}
	
	//Reverse string
	temp[tempIndex] = '\0';
	for(int i = tempIndex-1; i>=0; i--) {
		out[outIndex++] = temp[i];
	}
	out[outIndex] = '\0';
	
	return out;
}

void BackUpLog(char* timeStamp, char* absolutePath, char* backupPath, int flag) {
	char* logFile = (char *)malloc(sizeof(char *) * PATHMAX);
	sprintf(logFile, "%s/ssubak.log", backupPATH);

	//Open ssubak.log
	FILE *file = fopen(logFile, "a");

	if(file == NULL){
		fprintf(stderr, "Failed to open log file");
		exit(1);
	}
	//flag == 0 -> write backup log
	if(flag == 0) {
		//Write backup log message in ssubak.log
		fprintf(file, "%s : \"%s\" backuped to \"%s\"\n", timeStamp, absolutePath, backupPath);
	}
	//flag == 1 -> write remove log
	else if(flag == 1) {
		//Write remove log message in ssubak.log
		fprintf(file, "%s : \"%s\" removed by \"%s\"\n", timeStamp, absolutePath, backupPath);
	}
	
	//flag == 2 -> write recover log
	else if(flag == 2) {
		//Write recover log message in ssubak.log
		fprintf(file, "%s : \"%s\" recovered to \"%s\"\n", timeStamp, absolutePath, backupPath);
	}

	//Close file
	fclose(file);
}

//Get path up to filename, also create empty path to fullPath
char* getFullPath(char* here){
	char *recoverDir = (char *)malloc(sizeof(char *) * PATHMAX);
	//Make New Directory and get full path to new directory
	realpath(here, recoverDir);
    	char *filetmpname = getFileName(recoverDir);
	char *tmp = extractAfterName(here, filetmpname);
	char *fullPath = (char *)malloc(sizeof(char *) * PATHMAX);
	sprintf(fullPath, "%s/%s", recoverDir, tmp);
    	create_directory(fullPath); //Create path to fullPath
    	size_t length = strlen(fullPath);
    	if(fullPath[length -1] == '/')
    		fullPath[length-1] = '\0';
    	return fullPath;
}

//Get directory to timestamp
void getPathUpToTimeStamp(const char *fullPath, char *resultPath) {
	strcpy(resultPath, fullPath);
	
	// Find last '/'
	char *lastSlash = strrchr(resultPath, '/');
	
	if(lastSlash != NULL) {
		*(lastSlash + 1) = '\0';
	}
}

//Check if path is file, directory, or etc
int Path_Type(const char* path) {
	struct stat path_stat;
	//Wrong type or doesnt exist
	if(stat(path, &path_stat) != 0) return -1;
	
	//No read access
	if(access(path, R_OK) != 0){
		fprintf(stderr, "No Read Access Authority!\n");
		return -1;
	}
	
	//File
	if(S_ISREG(path_stat.st_mode)) return 0;
	//Directory
	if(S_ISDIR(path_stat.st_mode)) return 1;
	return -1;
}

//Append new record to list
void appendToLogList(LogLinkedList* list, char* originalPath, char* backupPath){
	if(!list) return;
	LogNode* newNode = createNewLogNode(originalPath, backupPath);
	if(!newNode) return;
	
	if(list->tail) {
		list->tail->next = newNode;
		list->tail = newNode;
	} else {
		list->head = list->tail = newNode;
	}
}	

char* getFileName(char* absolutePath){
	//Get filename from absolutePath
	char *filename = strrchr(absolutePath, '/');
	if(!filename) filename = absolutePath;
	else filename++;
	
	return filename;
}

//Parse log file and write in list
void parseLogFile(char* logFilePath, LogLinkedList* list) {
	FILE* file = fopen(logFilePath, "r");
	if(!file){
		return;
	}

	char line[PATH_MAX * 2];
	char originalPath[PATH_MAX], backupPath[PATH_MAX];
	
	//Get lines from ssubak.log and get the originalPath and backupPath and put them in linked list.
	while(fgets(line, sizeof(line), file)) {
		if(sscanf(line, "%*s : \"%[^\"]\" backuped to \"%[^\"]\"", originalPath, backupPath) == 2) {
			appendToLogList(list, originalPath, backupPath);
		}
		else if(sscanf(line, "%*s : \"%[^\"]\" removed by \"%[^\"]\"", backupPath, originalPath) == 2) {
			for(LogNode *current = list->head; current != NULL; current = current->next) {
				if(!strcmp(current->backupPath, backupPath)) {
					current->wasRemoved = true;
				}
			}
		}
		else if(sscanf(line, "%*s : \"%[^\"]\" recovered to \"%[^\"]\"", backupPath, originalPath) == 2) {
			for(LogNode *current = list->head; current != NULL; current = current->next) {
				if(!strcmp(current->backupPath, backupPath)) {
					current->wasRemoved = true;
				}
			}
		}
	}
	
	fclose(file);
}

//Make directory within backup directory that has a timeStamp
char* makeBackUpDir(char* timeStamp) {
	char *backupDir = (char *)malloc(sizeof(char *) * PATHMAX);
	//Construct backupDir path
	sprintf(backupDir, "%s/%s", backupPATH, timeStamp);
	
	if(mkdir(backupDir, 0777) != 0) {
		fprintf(stderr, "Failed to create timestamped backup dir\n");
		exit(1);
	}
	return backupDir;
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

//Goes up folders to erase empty folders
void RemoveEmptyDirRecursive(char* path) {
	struct dirent **namelist;
  	char *parentPath = (char *)malloc(sizeof(char *) * PATHMAX);
  	int cnt;

	if(rmdir(path) != 0) { return; }
 
  	strncpy(parentPath, path, PATHMAX);
  	char* parent = dirname(parentPath);
  	
  	//If the parent dir is root, stop
  	if(!strcmp(parent, backupPATH)) { return; }
  	
  	//Recursively check parent directories to remove directories
  	RemoveEmptyDirRecursive(parent);
	
	return;
}

//Get substring without filename
char* extractAfterName(char* path, char* filename) {
    const char* key = filename;
    char* result = NULL;

    // Find the occurrence of 'filename' in the path.
    char* found = strstr(path, key);
    if (found != NULL) {
        // Calculate the start index of the substring after 'filename'
        int startIndex = (found - path) + strlen(key);
        // If the next character is a slash, skip it.
        if (path[startIndex] == '/') {
            startIndex++;
        }

        // Extract the substring.
        result = strdup(path + startIndex);
        if (result == NULL) {
            fprintf(stderr, "Failed to allocate memory for substring.\n");
            return NULL;
        }
    } else {
        // filename was not found in the path.
        fprintf(stderr, "'new_d' not found in the path.\n");
    }

    return result;
}

//Create directory recursively going down (used in recover.h)
void create_directory(const char* recoverDir){
	char InnerDir[PATH_MAX];
	char *tmp = NULL;
	size_t len;
	
	sprintf(InnerDir, "%s", recoverDir);
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
	//Eg. The code block above makes only till /a/b, this line here makes the complete /a/b/c
	mkdir(recoverDir, 0777);
}
