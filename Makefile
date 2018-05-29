CC 		= gcc
CFLAGS 		= -Wall -Werror -fpic
LDFLAGS 	= -shared
RM 		= rm -f
TARGET_LIB 	= libdisplay.so

SRCS = src/Display.c
OBJS = Display.o

.PHONY: all
all: Display

Display: 
	$(CC) -c $(CFLAGS) $(SRCS)
	$(CC) $(LDFLAGS) -o $(TARGET_LIB) $(OBJS)
	$(RM) $(OBJS)

.PHONY: clean
clean:
	-$(RM) $(TARGET_LIB) $(OBJS)