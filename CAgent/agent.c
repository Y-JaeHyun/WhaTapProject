#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>

#include "agent.h"

int getPID(ProcessInfo *pInfo, char *processName) {
	char cmd[MAX_BUFF] = {0, };
	char buf[MAX_BUFF] = {0, };
	FILE *fp;
	int i = 0;
	char *token;
	
	snprintf(cmd, MAX_BUFF, "pidof %s", processName);
	if ((fp = popen(cmd, "r")) == NULL ) {
		return FAIL;
	}
	fgets(buf, MAX_BUFF, fp);
	pclose(fp);

	if (strlen(buf) == 0) return FAIL;

	token = strtok(buf, " ");
	while (token != NULL) {
		if (i >= CHECK_LIMIT) break;
		pInfo[i].pid = atoi(token);
		snprintf(pInfo[i].pName, sizeof(pInfo[i].pName), "%s", processName);
		token = strtok(NULL, " ");
		i++;
	}
	return SUCCESS;
}

int getMemory(ProcessInfo *pInfo) {
	char cmd[MAX_BUFF] = {0, };
	char buf[MAX_BUFF] = {0, };
	FILE *fp;
	snprintf(cmd, MAX_BUFF, "cat /proc/%d/stat | awk -F')' '{print$2}' | awk '{print $22}'", pInfo->pid);
	if ((fp = popen(cmd, "r")) == NULL ) {
		return FAIL;
	}
	fgets(buf, MAX_BUFF, fp);
	pclose(fp);


	pInfo->memoryByte = atoi(buf) * getpagesize();
	return SUCCESS;

}

int getCPU(ProcessInfo *pInfo) {
	char cmd[MAX_BUFF] = {0, };
	char buf[MAX_BUFF] = {0, };
	FILE *fp;
	snprintf(cmd, MAX_BUFF, "cat /proc/%d/stat | awk '{print $14+ $15}'", pInfo->pid);
	if ((fp = popen(cmd, "r")) == NULL ) {
		return FAIL;
	}
	fgets(buf, MAX_BUFF, fp);
	pclose(fp);

	pInfo->cpuUsed = atoi(buf);

	return SUCCESS;
	
}

void printProcessInfo(ProcessInfo *pInfo) {
	int i = 0;
	for (; i < CHECK_LIMIT; i++) {
		if (pInfo[i].pid == 0) return;
		printf("==================================\n");
		printf("process Name : %s\n", pInfo[i].pName);
		printf("process ID : %d\n", pInfo[i].pid);
		printf("process CPU Used : %d\n", pInfo[i].cpuUsed);
		printf("process CPU Usage : %lf\n", pInfo[i].cpuUsage);
		printf("process RSS : %llu\n",  pInfo[i].memoryByte);
		printf("==================================\n");
	}
}




int main () {
	ProcessInfo pInfo[10] = {0, }; // TODO Dynamic 
	time_t t;
	struct tm *tm;
	while (1) {
		int i = 0;
		t = time(NULL);
		tm = localtime(&t);
		printf("%s\n", asctime(tm));
		if (getPID(pInfo, "tmux") == FAIL) {
			printf("PID GET ERROR\n");
		}

		for (; i < CHECK_LIMIT; i++) {
			if (pInfo[i].pid ==0) break;
			getMemory(&pInfo[i]);
		}

		printProcessInfo(pInfo);
		memset(pInfo, '\0', sizeof(ProcessInfo) * 10);
		sleep(20);
	}
	return SUCCESS;
	
}
