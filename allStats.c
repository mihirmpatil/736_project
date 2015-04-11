#include "allStats.h"
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
				snprintf(path, sizeof(path), "%s/%s/stat",PROC, dent->d_name);
				//Try opening stat file if exists
				if( (statFile=fopen(path, "r"))!=NULL )
				{
					//get pid and ppid
					fscanf(statFile,"%s %*s %*s %s",pidStr, ppidStr);
					//if user process
					if(atoi(ppidStr)!=0 && atoi(ppidStr)!=1)
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

	//free all resources
	free(pTA);
	free(pTB);
	for(i=0; i<numPids; i++)
		free(openFiles[i]);
	free(openFiles);
	free(allPids->pids);
	free(allPids);
	
	return systemStats;
}


