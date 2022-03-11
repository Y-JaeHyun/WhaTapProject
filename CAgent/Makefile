CC = gcc
DEFS = -i./
LIBS = -ldl
CFLAGS = -g -Wall
#EXTRALIBS = 

OBJS = agent.o
TARGET = agent

all: $(TARGET)

clean:
	rm -f *.o
	rm -f $(TARGET)


$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS)

#$(EXTRALIBS) $(DEFS) $(LIBS)
