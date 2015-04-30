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

//Struct for storing pTime


//Return a structure of all user pids
AllPids* getAllPids(){
	//Initialize the vector of pids
	AllPids* allPids = malloc(sizeof(AllPids));
	allPids->numFiles=0;
	allPids->size=10;
	allPids->pids=malloc(10*sizeof(long int));

	//go through all the directories in proc. Le sigh
	
	DIR *dir;
	FILE *statFile;
	char ppidStr[33];
	char pidStr[33];
	char path[512];
	dir=opendir(PROC);
	struct dirent *dent;
	if(dir!=NULL)
	{	
		while((dent=readdir(dir)))
		{
			//if is a directory and not . or ..
			if( dent->d_type == DT_DIR && strcmp(dent->d_name, ".")!=0 && strcmp(dent->d_name, "..")!=0 )
			{
				//now check if it is a pid directory
				//or screw that, just open and see if it has the stat file inside it
				//and then check if ppid is not root. i.e user process
				snprintf(path, sizeof(path), "%s/%s/stat",PROC, dent->d_name);
				//printf("%s\n",path);
				//Try opening stat file if exists
				if( (statFile=fopen(path, "r"))!=NULL )
				{
					//get pid and ppid
					fscanf(statFile,"%s %*s %*s %s",pidStr, ppidStr);
					//if user process
					if(atoi(ppidStr)!=0)// && atoi(ppidStr)!=1)
					{
						// add this open file descriptor to a vector, IF I HAD ONE, oh well
						//If list at capacity then resize
						if(allPids->size==allPids->numFiles)
						{
							allPids->size+=10;
							allPids->pids=realloc(allPids->pids, (allPids->size)*sizeof(long int*));
						}
						//Add the pid to the list
						//allPids->pids[allPids->numFiles]=malloc(sizeof(long int));
						sscanf(pidStr,"%ld",&allPids->pids[allPids->numFiles]);
						allPids->numFiles++;
					}
					fclose(statFile);
				}
			}
		}
		closedir(dir);
	}
	else
	{
		printf("Error opening directory\n");
	}

	return allPids;
}


//Function to get total CPU plus all the user process cpu
SystemStats* getAllProcStats(){
	int i,j;
	FILE *fpS;
	char path[64];
	char utime[33];
	char stime[33];

	//get list of all pids
	AllPids *allPids=getAllPids();
	int numPids=allPids->numFiles;	
	
	//variables for process and system stats
	long double pCpu[2], sCpuA[7], sCpuB[7];
	//system usage and total variables for A and B
	long double sUA, sUB, sTA, sTB;

	//Initialize array of file pointers
	OpenFiles **openFiles=malloc(numPids*sizeof(OpenFiles*));
	//Initialize array of first and second samples
	long double *pTA=malloc(numPids*sizeof(long double));
	long double *pTB=malloc(numPids*sizeof(long double));
	for(i=0; i<numPids; i++)
	{
		openFiles[i]=malloc(sizeof(OpenFiles));
		openFiles[i]->statFile=NULL;
	}
	
	//Initailize list of procStats inside systemStats
	SystemStats *systemStats=malloc(sizeof(SystemStats));

	//Open all files at A
	//System stat
	fpS = fopen("/proc/stat", "r");
	//All user stat files
	for(i=0; i<numPids; i++)
	{
		sprintf(path, "/proc/%ld/stat",allPids->pids[i]);
		openFiles[i]->statFile =fopen(path, "r");
	}

	//Get sample values from files open at A
	//System samples
	fscanf(fpS, "%*s %Lf %Lf %Lf %Lf %Lf %Lf %Lf", &sCpuA[0], &sCpuA[1], &sCpuA[2], &sCpuA[3], &sCpuA[4], &sCpuA[5], &sCpuA[6]);
	fclose(fpS);
	//Process samples
	for(i=0; i<numPids; i++)
	{
		//Don't try read for non-existent processes
		if(openFiles[i]->statFile==NULL)
			pTA[i]=-1;//no sample for this pid
		else
		{
			//hacky way of reading 14th(utime) and 15th(stime) columns
			fscanf(openFiles[i]->statFile,"%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %s %s", utime, stime);
			sscanf(utime, "%Lf", &pCpu[0]);	
			sscanf(stime, "%Lf", &pCpu[1]);
			pTA[i]=pCpu[0]+pCpu[1];
			fclose(openFiles[i]->statFile);
		}
	}

	sleep(1);


	//Open all files at B
	//All user stat files
	for(i=0; i<numPids; i++)
	{
		sprintf(path, "/proc/%ld/stat",allPids->pids[i]);
		openFiles[i]->statFile =fopen(path, "r");
		//FIXME: This is where you open up the needed memory files
	}
	//System stat opened last
	fpS = fopen("/proc/stat", "r");

	//Get sample values from files open at B for which samples were collected at A
	//System samples
	fscanf(fpS, "%*s %Lf %Lf %Lf %Lf %Lf %Lf %Lf", &sCpuB[0], &sCpuB[1], &sCpuB[2], &sCpuB[3], &sCpuB[4], &sCpuB[5], &sCpuB[6]);
	fclose(fpS);
	//Process samples
	for(i=0; i<numPids; i++)
	{
		//Don't try read for non-existent processes
		if(openFiles[i]->statFile==NULL)
			pTB[i]=-1;//no sample for this pid
		else
		{
			//take samples only if samples were taken at A
			if(pTA[i]!=-1){
				//hacky way of reading 14th(utime) and 15th(stime) columns
				fscanf(openFiles[i]->statFile,"%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %s %s", utime, stime);
				sscanf(utime, "%Lf", &pCpu[0]);	
				sscanf(stime, "%Lf", &pCpu[1]);
				pTB[i]=pCpu[0]+pCpu[1];
			}
			else
				pTB[i]=-1;

			fclose(openFiles[i]->statFile);
		}
	}

	//calculate system usage
	//Usage: user+nice+system+irq+softirq
	sUA=sCpuA[0]+sCpuA[1]+sCpuA[2]+sCpuA[5]+sCpuA[6];
	sUB=sCpuB[0]+sCpuB[1]+sCpuB[2]+sCpuB[5]+sCpuB[6];
	//Total: usage+(idle+iowait)
	sTA=sUA+sCpuA[3]+sCpuA[4];
	sTB=sUB+sCpuB[3]+sCpuB[4];

	//total system cpu usage
	systemStats->cpuUsage= 100*((sUB-sUA)/(sTB-sTA));	

	systemStats->numProcs=0;
	//count number of pids that were still alive
	for(i=0; i<numPids; i++)
	{
		if(pTB[i]!=-1)
		{
			systemStats->numProcs++;
		}
	}

	//allocate the needed space for live pids
	systemStats->procStats=malloc(systemStats->numProcs*sizeof(ProcStats));

	//Now set cpu usage and pid for those live pids
	for(i=0, j=0; i<numPids; i++)
	{
		if(pTB[i]!=-1)
		{
			(systemStats->procStats[j]).cpuUsage= 100*((pTB[i]-pTA[i])/(sTB-sTA));
			systemStats->procStats[j].pid=allPids->pids[i];
			j++;
		}
	}

	return systemStats;


}


//Function to get process statistics for given PID
ProcStats* getProcStats(int pid){
	//int i;
	//processes and system stats at A and B
	long double pA[2], pB[2], sA[7], sB[7];
	long double pTA, sTA, pTB, sTB;
	ProcStats *stats=malloc(sizeof(ProcStats));
	FILE *fpS, *fpU;
	//char *token;

	char buff[1000];
	char path[64];
	char utime[33];
	char stime[33];
	sprintf(path, "/proc/%d/stat",pid);

	fpS = fopen("/proc/stat", "r");
	fpU = fopen(path, "r");
	if(fpU==NULL)//no process
	{
		free(stats);
		return NULL;
	}
	//read cpu stats
	fscanf(fpS, "%*s %Lf %Lf %Lf %Lf %Lf %Lf %Lf", &sA[0], &sA[1], &sA[2], &sA[3], &sA[4], &sA[5], &sA[6]);//read total cpu time
	//read process stats line
	fgets(buff, sizeof(buff), fpU);
	fclose(fpS);
	fclose(fpU);	

	//hacky way of reading 14th and 15th column	
	sscanf(buff,"%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %s %s", utime, stime);	
	sscanf(utime, "%Lf", &pA[0]);	
	sscanf(stime, "%Lf", &pA[1]);
	
	//token way of reading 14th and 15th column
	/*
	token=strtok(buff, " ");
	//discard the first 13 columns
	for(i=1; i<13; i++)
		token=strtok(NULL, " ");
	//14th and 15th column are utime and stime
	token=strtok(NULL, " ");
	sscanf(token, "%Lf", &pA[0]);	
	token=strtok(NULL, " ");
	sscanf(token, "%Lf", &pA[1]);
	*/
	
	sleep(1);


	fpS = fopen("/proc/stat", "r");
	fpU = fopen(path, "r");
	if(fpU==NULL)//no process
	{
		free(stats);
		return NULL;
	}
	//read system cpu stats
	fscanf(fpS, "%*s %Lf %Lf %Lf %Lf %Lf %Lf %Lf", &sB[0], &sB[1], &sB[2], &sB[3], &sB[4], &sB[5], &sB[6]);//read total cpu time
	//read process stats line
	fgets(buff, sizeof(buff), fpU);
	fclose(fpS);
	fclose(fpU);


	//hacky way of reading 14th and 15th column	
	sscanf(buff,"%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %s %s", utime, stime);	
	sscanf(utime, "%Lf", &pB[0]);	
	sscanf(stime, "%Lf", &pB[1]);
	
	/*
	token=strtok(buff, " ");
	//discard the first 13 columns
	for(i=1; i<13; i++)
		strtok(NULL, " ");
	
	//14th and 15th column are utime and stime
	token=strtok(NULL, " ");
	sscanf(token, "%Lf", &pB[0]);
	token=strtok(NULL, " ");
	sscanf(token, "%Lf", &pB[1]);
	*/	

	//calculate process cpu usage
	//process total=utime+stime
	pTA=pA[0]+pA[1];
	pTB=pB[0]+pB[1];
	//system total cpu time
	sTA=sA[0]+sA[1]+sA[2]+sA[3]+sA[4]+sA[5]+sA[6];
	sTB=sB[0]+sB[1]+sB[2]+sB[3]+sB[4]+sB[5]+sB[6];


	stats->cpuUsage= 100*((pTB-pTA)/(sTB-sTA));
	printf("pTB:%Lf pTA:%Lf sTB:%Lf sTA:%Lf and cpuUsage:%Lf \n", pTB, pTA, sTB, sTA, stats->cpuUsage);
	return stats;

}


//Function to get system statistics
SystemStats* getSystemStats(){
	long double a[7], b[7];
	long double uTA, tTA, uTB, tTB;
	SystemStats *stats=malloc(sizeof(SystemStats));
	FILE *fp;

	fp = fopen("/proc/stat", "r");
	fscanf(fp, "%*s %Lf %Lf %Lf %Lf %Lf %Lf %Lf", &a[0], &a[1], &a[2], &a[3], &a[4], &a[5], &a[6]);
	fclose(fp);
	sleep(1);
	fp = fopen("/proc/stat", "r");
	fscanf(fp, "%*s %Lf %Lf %Lf %Lf %Lf %Lf %Lf", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5], &b[6]);
	fclose(fp);
	
	//Usage: user+nice+system+irq+softirq
	uTA=a[0]+a[1]+a[2]+a[5]+a[6];
	uTB=b[0]+b[1]+b[2]+b[5]+b[6];
	//Total: usage+(idle+iowait)
	tTA=uTA+a[3]+a[4];
	tTB=uTB+b[3]+b[4];

	stats->cpuUsage= 100*((uTB-uTA)/(tTB-tTA));
	return stats;
}


int main(int argc, char* argv[])
{

	int i;
/*
	SystemStats *sStats=getAllProcStats();
	printf("Total CPU usage: %Lf%\n", sStats->cpuUsage);
	printf("Number of processes:%d\n",sStats->numProcs);	
	for(i=0; i< sStats->numProcs; i++)
		printf("Pid:%ld cpuUsage:%Lf%\n",sStats->procStats[i].pid, sStats->procStats[i].cpuUsage);
*/

/*
	int i;
	AllPids *allPids=getAllPids();
	
	printf("Number of processes:%d\n", allPids->numFiles);
	for(i=0; i<allPids->numFiles; i++)
	{
		printf("%ld\n", allPids->pids[i]);
	}
	return 0;
*/

	//Read stats for pid	
	if(argc!=2)
	{
		printf("Usage: %s [pid]\n",argv[0]);
		return 0;
	}
	
	int pid=atoi(argv[1]);
	//SystemStats *sStats;
	ProcStats *pStats;
	//while(1)
	{
		pStats=getProcStats(pid);
		if(pStats!=NULL)
			printf("Process CPU usage: %Lf%\n", pStats->cpuUsage);
		else
			printf("No process with pid:%d\n",pid);

		//sStats=getSystemStats();
		//printf("Total CPU usage: %Lf%\n", sStats->cpuUsage);
	}
	
	return 0;
}
