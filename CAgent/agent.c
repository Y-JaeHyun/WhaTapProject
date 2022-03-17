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

#include "agent.h"
#include "fileSend.h"
#include "ini.h"
#include "list.h"

#define HEADFORMAT "time%cprocess%cpid%ccpuUsage%cmemory\n"
#define CSVFORMAT "%ld%c%s%c%d%c%lf%c%llu\n"

#if 0
int getPID(List_t *list, const char *processName) {
	char cmd[MAX_BUFF] = {0, };
	char buf[MAX_BUFF] = {0, };
	FILE *fp;
	char *token;
	
	if (list == NULL) return FAIL;

	snprintf(cmd, MAX_BUFF, "pidof %s", processName);
	if ((fp = popen(cmd, "r")) == NULL ) {
		return FAIL;
	}
	fgets(buf, MAX_BUFF, fp);
	pclose(fp);

	if (strlen(buf) == 0) return FAIL;

	token = strtok(buf, " ");
	while (token != NULL) {
		ProcessInfo pInfo = {0, };
		pInfo.pid = atoi(token);
		snprintf(pInfo.pName, sizeof(pInfo.pName), "%s", processName);
		appendList(list, &pInfo);
		token = strtok(NULL, " ");
	}
	return SUCCESS;
}
#else 

int getPID(List_t *list, const char *processName) {
	DIR *dir;
	struct dirent *dir_entry;
	char cmd[MAX_BUFF] = {0, };
	char buf[MAX_BUFF] = {0, };
	FILE *fp;

	dir = opendir("/proc/");

	//  보다는 readlink가 좋을것 같은데 권한등에 의해 못보는 케이스 존재
	while (NULL != (dir_entry = readdir(dir))) {
		if (strspn(dir_entry->d_name, "0123456789") == strlen(dir_entry->d_name)) { // pid directory
			snprintf(cmd, MAX_BUFF, "cat /proc/%s/comm 2>/dev/null", dir_entry->d_name);
			if ((fp = popen(cmd, "r")) == NULL) {
				return FAIL;
			}
			fgets(buf, MAX_BUFF, fp);
			pclose(fp);

			if (strlen(processName) != strlen(buf) - 1 || strncmp(processName, buf, strlen(processName)) != 0) continue;

			ProcessInfo pInfo = {0, };
			pInfo.pid = atoi(dir_entry->d_name);
			snprintf(pInfo.pName, sizeof(pInfo.pName), "%s", processName);
			appendList(list, &pInfo);

		}
	}
	return SUCCESS;

}

#endif 

uint64_t getCPUTotalUsed() {
	char cmd[MAX_BUFF] = {0, };
	char buf[MAX_BUFF] = {0, };
	FILE *fp;
	char *pos = NULL;
	snprintf(cmd, MAX_BUFF, "cat /proc/stat | head -n 1 |awk '{print $2 + $3 + $4 + $5 + $6 + $7 + $8 + $9 + $10 + $11}'");
	if ((fp = popen(cmd, "r")) == NULL ) {
		return FAIL;
	}
	fgets(buf, MAX_BUFF, fp);
	pclose(fp);

	strtoll(buf, &pos, 10);

	return strtoll(buf, &pos, 10);;
}

int getStatInfo(ProcessInfo *pInfo) {
	char cmd[MAX_BUFF] = {0, };
	char buf[MAX_BUFF] = {0, };
	FILE *fp;
	char *pos, *token;

	snprintf(cmd, MAX_BUFF, "cat /proc/%d/stat | awk -F')' '{print$2}' | awk '{print $2 \" \" $20 \" \" $22 \" \" $12 + $13 + $14 + $15}'", pInfo->pid);
	if ((fp = popen(cmd, "r")) == NULL ) {
		return FAIL;
	}
	fgets(buf, MAX_BUFF, fp);
	pclose(fp);

	token = strtok(buf, " ");
	if (token == NULL) return FAIL;
	pInfo->ppid = atoi(token);


	token = strtok(NULL, " ");
	if (token == NULL) return FAIL;
	pInfo->starttime = strtoll(token, &pos, 10);

	token = strtok(NULL, " ");
	if (token == NULL) return FAIL;
	pInfo->memoryByte = atoi(token) * getpagesize();

	token = strtok(NULL, " ");
	if (token == NULL) return FAIL;
	pInfo->cpuUsed = strtoll(token, &pos, 10);

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


int getProcessStat(void *node, void *etc) {
	ProcessInfo *pInfo = (ProcessInfo *)node;
	if (pInfo->pid ==0) return FAIL;
	getStatInfo(pInfo);

	return SUCCESS;
}

// 정리 필요
typedef struct etc {
	List_t *beforeList;
	time_t time;
	uint64_t totalCpuUsed;
	long number_of_processors;
	char separator;
	FILE *fp;
}ETC;

int printProcessStat(void *node, void *etc) {
	ProcessInfo *pInfo = (ProcessInfo *)node;
	ETC *etcInfo = (ETC *)etc;
	ProcessInfo *beforeInfo = searchNode(etcInfo->beforeList, node);
	uint64_t processCpuUsed;
	char separator = etcInfo->separator;

	if (beforeInfo) {
		processCpuUsed = pInfo->cpuUsed - beforeInfo->cpuUsed;
	} else {
		processCpuUsed = pInfo->cpuUsed;
	}
	pInfo->cpuUsage = ((double)processCpuUsed) / etcInfo->totalCpuUsed * 100.0 * etcInfo->number_of_processors;

	fprintf(etcInfo->fp, CSVFORMAT,
			etcInfo->time, separator,
			pInfo->pName, separator,
			pInfo->pid, separator,
			pInfo->cpuUsage, separator,
			pInfo->memoryByte); 
	return SUCCESS;
}

int main () {
	List_t *list[2] = {0, }; // before, now
	int index = 0;
	ETC etc = {0, };
	etc.number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
	//sysconf(_SC_CLK_TCK);
	uint64_t nowCpuTotalUsed;
	uint64_t beforeCpuTotalUsed = 0;

	ini_t *config = ini_load("config/config.ini");

	const char *url = ini_get(config, "server", "url");
	const char *fileSavePath = ini_get(config, "agent", "fileSavePath");
	const char *fileName = ini_get(config, "agent", "fileName");
	const char *processName = ini_get(config, "agent", "process");

	char fileFullName[128] = {0, };
	char buff[2][32] = {{0, }, };
	int sleepTime;
	int min;
	ini_sget(config, "agent", "separator", "'%c'", &etc.separator);
	ini_sget(config, "agent", "sleep", "%d", &sleepTime);
	ini_sget(config, "agent", "min", "%d", &min);

	ini_free(config);

	while (1) {
		if (list[index] == NULL) {
			list[index] = initList(sizeof(ProcessInfo), appendProcessFunc, compFunc);
		}

		etc.beforeList = list[!index];
		etc.time = time(NULL);

		if (getPID(list[index], processName) == FAIL) {
			printf("PID GET ERROR\n");
		}

		if ((nowCpuTotalUsed = getCPUTotalUsed()) == FAIL) {
			printf("CPU TOTAL USED ERROR\n");
		}
		etc.totalCpuUsed = nowCpuTotalUsed - beforeCpuTotalUsed;
		beforeCpuTotalUsed = nowCpuTotalUsed;

		if (circuitList(list[index], getProcessStat, NULL) == FAIL) {
			printf("Process Stat GET ERROR\n");
		}
	
		mkdir(fileSavePath, 0777);


		if (min == 0) {
			strftime(buff[index], sizeof(buff[index]), "%Y%m%d", localtime(&etc.time));
		} else {
			strftime(buff[index], sizeof(buff[index]), "%Y%m%d%H%M", localtime(&etc.time));
		}

		if (strncmp(buff[index], buff[!index], strlen(buff[index])) != 0) {
			if (strlen(fileFullName) != 0) {
				sendFile(url,fileFullName);
			}
			snprintf(fileFullName, sizeof(fileFullName), "%s/%s%s.csv", fileSavePath, fileName, buff[index]);
		}

		if (access(fileFullName, F_OK) != 0) {
			etc.fp = fopen(fileFullName, "w");
			fprintf(etc.fp, HEADFORMAT, etc.separator, etc.separator, etc.separator, etc.separator);
		} else {
			etc.fp = fopen(fileFullName, "a");
		}

		if (circuitList(list[index], printProcessStat, &etc) == FAIL) {
			printf("Process Stat Print ERROR\n");
		}

		fclose(etc.fp);

		index = !index;

		destroyList(list[index]);
		list[index] = NULL;

		sleep(sleepTime);
	}
	return SUCCESS;
	
}
