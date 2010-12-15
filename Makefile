TARGET1= rng2html

all: ${TARGET1}

CC=gcc
CFLAGS += -D_GNU_SOURCE -ggdb 
CFLAGS += -I. ` pkg-config --cflags glib-2.0`
CFLAGS += `xml2-config --cflags`

LDFLAGS += `xml2-config --libs`
LDFLAGS += `pkg-config --libs glib-2.0`


OBJS1=  rng2html.o \
	tree.o \
	space.o

OBJS2=  genclass.o

SHAREDOBJS= zalloc.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

${TARGET1}: ${SHAREDOBJS} ${OBJS1}
	$(CC) -o $@ $^ $(LDFLAGS)


clean:
	rm -f *.o *~
