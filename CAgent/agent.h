#ifndef __AGENT_H__
#define __AGENT_H__

#define MAX_BUFF 512
#define NAME_LEN 32
#define CHECK_LIMIT 32
enum {
	FAIL = -1,
	SUCCESS = 0
};

typedef struct processInfo {
	int pid;
	char pName[NAME_LEN];
	uint32_t cpuUsed;
	double cpuUsage;
	uint64_t memoryByte; //rss	
}ProcessInfo;




#endif //__AGENT_H__
