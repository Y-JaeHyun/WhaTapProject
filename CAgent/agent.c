#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>
#include <pwd.h>

#include "agent.h"
#include "fileSend.h"
#include "ini.h"
//#include "list.h"
#include "linkedList.h"

#include <linux/fs.h>
#include <sys/ioctl.h>

#define CSV_HEAD_LEN 32

void ProcessSet(LinkedList_t *list, int pid, const char *processName, const char *cmd2) {
	ProcessInfo pInfo = {0, };
	ProcessInfo *searchInfo; 
	pInfo.pid = pid;
	
	if ((searchInfo = searchNodeData(list, &pInfo)) == NULL) {
		snprintf(pInfo.pName, sizeof(pInfo.pName), "%s", processName);

		if (cmd2 != NULL) 
			snprintf(pInfo.cmd2, sizeof(pInfo.cmd2), "%s", cmd2);
		else 
			pInfo.cmd2[0] = '\0';
		pInfo.readByte= -1;
		pInfo.writeByte = -1;
		searchInfo = appendLinkedList(list, &pInfo);
	}
	searchInfo->check = 1;
	getStatInfo(searchInfo);
}

/*
 * In Linux we have three ways to determine "process name":
 * 1. /proc/PID/stat has "...(name)...", among other things. It's so-called "comm" field.
 * 2. /proc/PID/cmdline's first NUL-terminated string. It's argv[0] from exec syscall.
 * 3. /proc/PID/exe symlink. Points to the running executable file.
 */

int getProcessInfo(LinkedList_t *list, const char *processName) {
	DIR *dir;
	struct dirent *dir_entry;
	char fileName[MAX_BUFF] = {0, };
	char comm[MAX_BUFF] = {0, };
	char cmdline[CMD_BUFF] = {0, };
	char exe[CMD_BUFF] = {0, };
	const char *pCmd;
	FILE *fp;
	char *cmd2;

	dir = opendir("/proc/");

	//  보다는 readlink가 좋을것 같은데 권한등에 의해 못보는 케이스 존재
	while (NULL != (dir_entry = readdir(dir))) {
		memset(comm, '\0', sizeof(comm));
		memset(cmdline, '\0', sizeof(cmdline));
		memset(exe, '\0', sizeof(exe));
		pCmd = NULL;
		cmd2 = NULL;

		if (strspn(dir_entry->d_name, "0123456789") == strlen(dir_entry->d_name)) { // pid directory
			//CMDLINE
			snprintf(fileName, MAX_BUFF, "/proc/%s/cmdline", dir_entry->d_name);
			
			if ((fp = fopen(fileName, "r")) != NULL) {
				fscanf(fp, "%[^\n]", cmdline);
				fclose(fp);
				
				if ((cmd2 = strchr(cmdline, '\0')) != NULL) {
					cmd2 += 1;
				}

				pCmd = strrchr(cmdline, '/');
				if (!pCmd ) {
					pCmd = &cmdline[0];
				} else {
					pCmd += 1;
				}

				if (strlen(processName) == strlen(pCmd) && strncmp(processName, pCmd, strlen(processName)) == 0) {
					ProcessSet(list, atoi(dir_entry->d_name), processName, cmd2);
					continue;
				}
			}		

			//COMM 비교
			snprintf(fileName, MAX_BUFF, "/proc/%s/comm", dir_entry->d_name);
			if ((fp = fopen(fileName, "r")) != NULL) {
				fscanf(fp, "%s", comm);
				fclose(fp);
				if (strncmp(processName, comm, 15) == 0 && comm[14] == '\0') {
					ProcessSet(list, atoi(dir_entry->d_name), processName, cmd2);
					continue;
				}
			}

			//exe 비교
			snprintf(fileName, MAX_BUFF, "/proc/%s/exe", dir_entry->d_name);
			if (readlink(fileName, exe, sizeof(exe)) != -1) {
				pCmd = strrchr(exe, '/');
				if (!pCmd ) {
					pCmd = &exe[0];
				} else {
					pCmd += 1;
				}
				if (strlen(processName) == strlen(pCmd) && strncmp(processName, pCmd, strlen(processName)) == 0) {
					ProcessSet(list, atoi(dir_entry->d_name), processName, cmd2);
					continue;
				}


			}
		}
	}
	return SUCCESS;

}

time_t getUptime() {
	FILE * fp;
	int sec = 0, ssec;
	
	if ((fp = fopen("/proc/uptime", "r")) != NULL) {
		fscanf(fp, "%d.%ds", &sec, &ssec);
		fclose(fp);
	}
	return sec;
}

uint64_t getCPUTotalUsed() {
	FILE *fp;
	uint64_t cpu_time[10] = {0, };
	uint64_t total = 0;
	int i = 0;

	if ((fp = fopen("/proc/stat", "r")) == NULL) {
		return FAIL;
	}

	if (fscanf(fp, "%*s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu", 
				&cpu_time[0], &cpu_time[1], &cpu_time[2], &cpu_time[3],
				&cpu_time[4], &cpu_time[5], &cpu_time[6], &cpu_time[7],
				&cpu_time[8], &cpu_time[9]) == EOF) {
		fclose(fp);
		return FAIL;
	}

	fclose(fp);

	for (i = 0; i < 10; i ++) {
		total += cpu_time[i];
	}
	
	return total;
}

struct pstat {
	int32_t ppid;
	uint64_t starttime;
	unsigned long int utime_ticks;
	unsigned long int stime_ticks;
	long int cutime_ticks;
	long int cstime_ticks;
	uint64_t rss; //Resident  Set  Size in bytes
};


int getStatInfo(ProcessInfo *pInfo) {
	char fileName[MAX_BUFF] = {0, };
	FILE *fp;
	struct pstat stat = {0, };
	int uid = -1;
	struct passwd *pws;

	snprintf(fileName, MAX_BUFF, "/proc/%d/stat", pInfo->pid);
	if ((fp = fopen(fileName, "r")) == NULL) {
		return FAIL;
	}

	if (fscanf(fp, "%*d (%*[^)]%*[)] %*c %d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu"
				"%lu %ld %ld %*d %*d %*d %*d %llu %*u %llu",
				&stat.ppid, &stat.utime_ticks, &stat.stime_ticks,
				&stat.cutime_ticks, &stat.cstime_ticks, &stat.starttime,
				&stat.rss) == EOF) {
		fclose(fp);
		return -1;
	}
	fclose(fp);

	pInfo->ppid = stat.ppid;
	pInfo->starttime = stat.starttime;
	pInfo->memoryByte = stat.rss * getpagesize();
	pInfo->beforeCpuUsed = pInfo->cpuUsed;
	pInfo->cpuUsed = stat.utime_ticks + stat.stime_ticks + stat.cutime_ticks + stat.cstime_ticks;

	snprintf(fileName, MAX_BUFF, "/proc/%d/status", pInfo->pid);
	if ((fp = fopen(fileName, "r")) == NULL) {
		return FAIL;
	}

	fscanf(fp, "%*[^\n] %*[^\n] %*[^\n] %*[^\n] %*[^\n] %*[^\n] %*[^\n] %*[^\n] " //8 line skip
			"Uid:\t%d\t", &uid);
	fclose(fp);

	pws = getpwuid(uid);
	snprintf(pInfo->userName, sizeof(pInfo->userName), "%s", pws->pw_name);

	snprintf(fileName, MAX_BUFF, "/proc/%d/io", pInfo->pid);
	if ((fp = fopen(fileName, "r")) == NULL) {
		printf("Check Permission Denied!\n");
		pInfo->beforeReadByte = -1;
		pInfo->beforeWriteByte = -1;
		exit (1);
	}

	pInfo->beforeReadByte = pInfo->readByte;
	pInfo->beforeWriteByte = pInfo->writeByte;

	fscanf(fp, "%*[^\n] %*[^\n] %*[^\n] %*[^\n] "
			"read_bytes: %llu "
			"write_bytes: %llu", &pInfo->readByte, &pInfo->writeByte);
	fclose(fp);


	return SUCCESS;
}

int appendProcessFunc(void *node, void *data, size_t size) {
	memcpy(node, data, size);
	return 0;
}

int compFunc(void *node, void *data) {
	ProcessInfo *pInfo = (ProcessInfo *)data;
	ProcessInfo *beforeInfo = (ProcessInfo *)node;
	if (pInfo->pid == beforeInfo->pid) return 1;
	return 0;
}


// 정리 필요
typedef struct etc {
	//LinkList_t *beforeList;
	time_t time;
	time_t uptime;
	uint64_t totalCpuUsed;
	long number_of_processors;
	char separator;
	FILE *fp;
	long clk_tick;
	const char *url;
	int sleepTime;
	double blockSize;
}ETC;


int printCSVHead(FILE *fp, char separator) {
	// HEAD FORMAT
	char head[MAX_BUFF] = {0, };
	int i = 0;
	int strLen = 0;
	int len = sizeof(CSVFORMAT) / sizeof(struct csvFormat);
	for (i = 0; i < len; i++) {
		strLen = strlen(head);
		snprintf(&head[strLen], sizeof(head) - strLen, "%s%c", CSVFORMAT[i].name, separator);
	}
	head[strlen(head)-1] = '\n';

	fprintf(fp, "%s", head);

	return SUCCESS;
}


int printCSVData(FILE *fp, const char * url, char separator) {
	char csv[MAX_BUFF] = {0, };
	int i = 0;
	int len = sizeof(CSVFORMAT) / sizeof(struct csvFormat);
	int strLen = 0;
	int fail = 0;
	for (i = 0; i < len; i++) {
		fail = 0;
		strLen = strlen(csv);
		if (CSVFORMAT[i].p == NULL) {
			fail = 1;
		} else {
			switch (CSVFORMAT[i].flag) {
				case LONG_INT:
					if (*(long int **)CSVFORMAT[i].p == NULL) {
						fail = 1;
					} else {
						snprintf(&csv[strLen], sizeof(csv) - strLen, "%ld%c", **(long int **)CSVFORMAT[i].p, separator);
					}
					break;
				case STRING:
					if (*(char **)CSVFORMAT[i].p == NULL) {
						fail = 1;
					} else {
						snprintf(&csv[strLen], sizeof(csv) - strLen, "%s%c", *(char **)CSVFORMAT[i].p, separator);
					}
					break;
				case INT:
					if (*(int **)CSVFORMAT[i].p == NULL) {
						fail = 1;
					} else {
						snprintf(&csv[strLen], sizeof(csv) - strLen, "%d%c", **(int **)CSVFORMAT[i].p, separator);
					}
					break;
				case LONG_LONG_INT:
					if (*(uint64_t **)CSVFORMAT[i].p == NULL) {
						fail = 1;
					} else {
						snprintf(&csv[strLen], sizeof(csv) - strLen, "%llu%c", **(uint64_t **)CSVFORMAT[i].p, separator);
					}
					break;
				case DOUBLE:
					if (*(double **)CSVFORMAT[i].p == NULL) {
						fail = 1;
					} else {
						snprintf(&csv[strLen], sizeof(csv) - strLen, "%lf%c", **(double **)CSVFORMAT[i].p, separator);
					}
					break;
				case NOT_SUPPORT:
					fail = 1;
					break;

			}

		}
		if (fail == 1) {
			snprintf(&csv[strLen], sizeof(csv) - strLen, "-%c", separator);

		}
	}
	csv[strlen(csv)-1] = '\n';

#if 1
	fprintf(fp, "%s", csv);
	sendRealTime(url, csv);

#else  //TEST

	printf("%s", csv);
#endif 


	return SUCCESS;

}
int printProcessStat(LinkedList_t *list, void *node, void *etc) {
	Node *temp = (Node *)node;
	ProcessInfo *pInfo = (ProcessInfo *)temp->data;
	ETC *etcInfo = (ETC *)etc;
	uint64_t processCpuUsed;
	char separator = etcInfo->separator;
	double cpuUsage = 0;
	double readBPS = 0;
	double writeBPS = 0;
	double readIOPS = 0;
	double writeIOPS = 0;

	if (pInfo->check == 0) { // 없어진 PID
		if (list->head == temp) {
			list->head = temp->next;
			if (list->head)
				list->head->prev = NULL;
		} else {
			temp->prev->next = temp->next;
			if (temp->next) 
				temp->next->prev = temp->prev;
		}
		
		free(temp->data);
		free(temp);
		return SUCCESS;
	}

	pInfo->check = 0;
	processCpuUsed = pInfo->cpuUsed - pInfo->beforeCpuUsed;
	cpuUsage = ((double)processCpuUsed) / etcInfo->totalCpuUsed * 100.0 * etcInfo->number_of_processors;
	pInfo->starttime = etcInfo->time - etcInfo->uptime + (pInfo->starttime / etcInfo->clk_tick);
	strftime(pInfo->startString, sizeof(pInfo->startString), "%Y%m%d%H%M", localtime(&pInfo->starttime));

	if (pInfo->beforeReadByte != -1) {
		readBPS = (double)(pInfo->readByte - pInfo->beforeReadByte) / 1024 / etcInfo->sleepTime;
		readIOPS = readBPS / etcInfo->blockSize;
	}

	if (pInfo->beforeWriteByte != -1) {
		writeBPS = (double)(pInfo->writeByte - pInfo->beforeWriteByte) / 1024 / etcInfo->sleepTime;
		writeIOPS = writeBPS / etcInfo->blockSize;
	}


	cData.time = &etcInfo->time;
	cData.pName= pInfo->pName;
	cData.pid = &pInfo->pid;
	cData.ppid = &pInfo->ppid;
	cData.cpuUsage = &cpuUsage;
	cData.memory = &pInfo->memoryByte;
	cData.createtime = &pInfo->starttime;
	cData.ctimeStr = pInfo->startString;
	cData.cmd2 = pInfo->cmd2;
	cData.uName = pInfo->userName;
	cData.readIOPS = &readIOPS;
	cData.writeIOPS = &writeIOPS;
	cData.readBPS = &readBPS;
	cData.writeBPS = &writeBPS;
	printCSVData(etcInfo->fp, etcInfo->url, separator);
	return SUCCESS;
}

int main () {
	LinkedList_t *list = initLinkedList(sizeof(ProcessInfo), compFunc);;
	int index = 0;
	ETC etc = {0, };
	etc.number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
	etc.clk_tick = sysconf(_SC_CLK_TCK);
	etc.uptime = getUptime();
	uint64_t nowCpuTotalUsed;
	uint64_t beforeCpuTotalUsed = 0;

	char block[MAX_BUFF] = {0, };
	FILE *fp;
	ini_t *config = ini_load("config/config.ini");

	etc.url = ini_get(config, "server", "url");
	const char *fileSavePath = ini_get(config, "agent", "fileSavePath");
	const char *fileName = ini_get(config, "agent", "fileName");
	const char *processName = ini_get(config, "agent", "process");

	char fileFullName[128] = {0, };
	char buff[2][32] = {{0, }, };
	int min;
	ini_sget(config, "agent", "separator", "'%c'", &etc.separator);
	ini_sget(config, "agent", "sleep", "%d", &etc.sleepTime);
	ini_sget(config, "agent", "min", "%d", &min);

	snprintf(block, MAX_BUFF, "/sys/class/block/%s/queue/logical_block_size", ini_get(config,"agent", "disk"));

	if ((fp = fopen(block, "r")) != NULL) {
		fscanf(fp, "%lf", &etc.blockSize);
		fclose(fp);
		etc.blockSize /= 1024;
		if (etc.blockSize == 0.0) {
			printf("Check Permission Denied!\n");
			exit(1);
		}
	} else {
		printf("Check Config Disk Name!\n");
		exit(1);
	}


	mkdir(fileSavePath, 0777);

	while (1) {
		etc.time = time(NULL);
		etc.uptime = getUptime();

		if (getProcessInfo(list, processName) == FAIL) {
			printf("INFO GET ERROR\n");
		}

		if ((nowCpuTotalUsed = getCPUTotalUsed()) == FAIL) {
			printf("CPU TOTAL USED ERROR\n");
		}

		etc.totalCpuUsed = nowCpuTotalUsed - beforeCpuTotalUsed;
		beforeCpuTotalUsed = nowCpuTotalUsed;

		if (min == 0) {
			strftime(buff[index], sizeof(buff[index]), "%Y%m%d", localtime(&etc.time));
		} else {
			strftime(buff[index], sizeof(buff[index]), "%Y%m%d%H%M", localtime(&etc.time));
		}

		if (strncmp(buff[index], buff[!index], strlen(buff[index])) != 0) {
			snprintf(fileFullName, sizeof(fileFullName), "%s/%s%s.csv", fileSavePath, fileName, buff[index]);
		}

		if (access(fileFullName, F_OK) != 0) {
			etc.fp = fopen(fileFullName, "w");
			printCSVHead(etc.fp, etc.separator);
		} else {
			etc.fp = fopen(fileFullName, "a");
		}

		if (circuitLinkedList(list, printProcessStat, &etc) == FAIL) {
			printf("Process Stat Print ERROR\n");
		}

		fclose(etc.fp);

		index = !index;

		sleep(etc.sleepTime);
	}
	destroyLinkedList(list);
	ini_free(config);
	return SUCCESS;
	
}
