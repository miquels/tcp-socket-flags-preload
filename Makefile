CC=gcc
CFLAGS=-fPIC -Wall
LDFLAGS=-shared -Wl,--version-script=export.map -Wl,-soname,libtcpsocketflags.so
#LDFLAGS=-shared -Wl,--retain-symbols-file,symbols.map -Wl,-soname,libtcpsocketflags.so
LDLIBS=-ldl -lpthread

OBJS=config.o ipnetwork.o sys_connect.o sys_accept.o util.o

libtcpsocketflags.so: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

main: main.o $(OBJS)
	$(CC) -o main main.o $(OBJS)

clean:
	rm -f *.so *.o
