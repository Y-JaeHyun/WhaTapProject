CC = gcc
LIBS = ./fileSend/libfile.a ./lib/libstd.a -lcurl
CFLAGS = -g -Wall -I./fileSend -I./iniParser/src -I./lib
#EXTRALIBS = 

OBJS = agent.o iniParser/src/ini.o
TARGET = agent

SUBDIRS = fileSend lib

all: $(OBJS) $(TARGET)

clean:
	for DIRS in $(SUBDIRS);do\
		$(MAKE) -C $$DIRS clean;\
	done
	rm -f *.o
	rm -f $(TARGET)


%.o: %.c $(DEFS)
	$(CC) $(CFLAGS) -c $< -o $@ 

agent: agent.o
	for DIRS in $(SUBDIRS);do\
		$(MAKE) -C $$DIRS;\
	done
	$(CC) -o $@ $(OBJS) $(LIBS)


