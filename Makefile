CC = gcc
CFLAGS = 
LDFLAGS = 
 
SRC = $(wildcard *.c)
OBJS = $(SRC:.c=.o)
AOUT = shell_custom
 
all : $(AOUT) clean
 
shell_custom : $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^
%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<
clean :
	@rm *.o
cleaner :
	@rm $(AOUT)
