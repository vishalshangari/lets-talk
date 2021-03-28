# the compiler: gcc for C program, define as g++ for C++
CC = gcc
# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g -Wall
VALFLAGS = valgrind --leak-check=full
# the build target executable:
TARGET = lets-talk
all: $(TARGET)
$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c list.c server.c client.c -lpthread -lreadline -lrt
clean:
	$(RM) $(TARGET)	
valgrind: 
	valgrind --leak-check=full ./lets-talk 23433 127.0.0.1 23433