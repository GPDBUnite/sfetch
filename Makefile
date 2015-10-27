CPP=g++
CFLAGS=
INCLUDES=

cppsources = BlockingBuffer.cpp OffsetMgr.cpp URLParser.cpp utils.cpp 
csources  = http_parser.c

objects = $(cppsources:.cpp=.o) $(csources:.c=.o) 


app = sfetch

all: $(objects)
	g++  -o $(app) $(objects)  -lcurl -lpthread -lcrypto

.cpp.o:
	$(CPP) $(CFLAGS) $(INCLUDES) -c $<  -o $@

.c.o:
	$(CPP) $(CFLAGS) $(INCLUDES) -c $<  -o $@

test: $(app)
	./$(app) http://127.0.0.1:8080/s3/bigfile

clean:
	rm *.o $(app)