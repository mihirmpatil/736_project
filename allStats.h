#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>


#define PROC "/proc"

typedef struct{
	long int pid;
	char name[128];
	long double cpuUsage;
}ProcStats;

typedef struct{
	int numProcs;
	ProcStats *procStats;
	long double cpuUsage;
}SystemStats;

//struct for open FILE pointers of a process
typedef struct{
	int pid;
	FILE *statFile;
	FILE *mstatFile; 
}OpenFiles;

//Contains a list of ProcFiles
typedef struct{
	int size;
	int numFiles;
	long int *pids;
}AllPids;

//Get all live pids
AllPids* getAllPids();

//Get stats for all live pids
SystemStats* getAllProcStats();
