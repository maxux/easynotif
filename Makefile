CC      = i686-pc-linux-gnu-gcc
DEFINES = 
CFLAGS  = -W -Wall -O2 -pipe
LDFLAGS = -lpthread -lncurses
EXEC    = easynotif
CLEANE  = 

SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c %.h
	$(CC) $(DEFINES) -o $@ -c $< $(CFLAGS)

clean:
	rm -fv *.o

mrproper: clean
	rm -fv $(EXEC) $(CLEANE)
