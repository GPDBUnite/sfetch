objects = BlockingBuffer.o  OffsetMgr.o

sources = BlockingBuffer.cpp OffsetMgr.cpp

app = sfetch

all: $(objects)
	g++  -o $(app) $(objects) -lcurl -lpthread

$(objects): $(sources)
	g++ -c $(sources) -g

test: $(app)
	./$(app) http://127.0.0.1:8080/s3/xaa

clean:
	rm $(objects) $(app)