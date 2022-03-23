#ifndef __AGENT_H__
#define __AGENT_H__

#define MAX_BUFF 512
#define NAME_LEN 32
#define CHECK_LIMIT 32
#define DATE_BUFF 16
#define CMD_BUFF 1024
#define CSV_HEAD_LEN 32

enum {
	FAIL = -1,
	SUCCESS = 0
};

typedef struct processInfo {
	int check;
	int pid;
	int ppid;
	char pName[NAME_LEN];
	char cmd2[NAME_LEN];
	uint32_t cpuUsed;
	uint32_t beforeCpuUsed;
	uint64_t memoryByte; //rss	
	time_t starttime;
	char startString[DATE_BUFF];
	char userName[NAME_LEN];
	uint64_t readByte;
	uint64_t writeByte;
	uint64_t beforeReadByte;
	uint64_t beforeWriteByte;
}ProcessInfo;

struct csvFormat {
	char name[CSV_HEAD_LEN];
	char flag;
	void *p; // or getFunc()?
	
};

ProcessInfo *pProcessInfo;

enum {
	LONG_INT = 0,
	STRING,
	INT,
	LONG_LONG_INT,
	DOUBLE,
	NOT_SUPPORT,
};

struct csvData {
	time_t *time;
	time_t *createtime;
	char *ctimeStr;
	char *pName;
	char *cmd2;
	int *pid;
	int *ppid;
	double *cpuUsage;
	uint64_t *memory;
	char *uName;
	double *readIOPS;
	double *writeIOPS;
	double *readBPS;
	double *writeBPS;
	uint64_t *tt;
};

struct csvData cData = {0, };

struct csvFormat CSVFORMAT[] = {
	{"time", LONG_INT, &cData.time},
	{"createtime", LONG_INT, &cData.createtime},
	{"createtime(YYYYMMDD)", STRING, &cData.ctimeStr},
	{"processName", STRING, &cData.pName},
	{"CMD2", STRING, &cData.cmd2},
	{"pid", INT, &cData.pid},
	{"ppid", INT, &cData.ppid},
	{"user", STRING, &cData.uName},
	{"cpuUsage", DOUBLE, &cData.cpuUsage},
	{"memory", LONG_LONG_INT, &cData.memory}, 
	{"Read IOPS", DOUBLE, &cData.readIOPS},
	{"Write IOPS", DOUBLE, &cData.writeIOPS},
	{"Read BPS", DOUBLE, &cData.readBPS},
	{"Write BPS", DOUBLE, &cData.writeBPS},
};


int getStatInfo(ProcessInfo *pInfo);




#endif //__AGENT_H__
