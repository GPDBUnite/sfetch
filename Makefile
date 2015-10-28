
GTEST_DIR=gtest

CPP=g++
CFLAGS=
INCLUDES = -I.

sources = BlockingBuffer.cpp OffsetMgr.cpp URLParser.cpp utils.cpp http_parser.cpp
testcources = utils_test.cpp


objects = $(sources:.cpp=.o) 
testobjs = $(testcources:.cpp=.o) 

app = sfetch
testapp = $(app)_test

# Usually you shouldn't tweak such internal variables, indicated by a
# trailing _.
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)


all: build buildtest

$(testobjs) gtest-all.o gtest_main.o: INCLUDES += -I$(GTEST_DIR)/include -I$(GTEST_DIR)/


# For simplicity and to avoid depending on Google Test's
# implementation details, the dependencies specified below are
# conservative and not optimized.  This is fine as Google Test
# compiles fast and for ordinary users its source rarely changes.
gtest-all.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) $(INCLUDES) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest-all.cc

gtest_main.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) $(INCLUDES)  $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest_main.cc

gtest_main.a : gtest-all.o gtest_main.o
	$(AR) $(ARFLAGS) $@ $^

.PHONY: all test clean

build: $(objects)
	g++  -o $(app) $(objects) $(csources:.c=.o)  -lcurl -lpthread -lcrypto

%.o: %.cpp 
	$(CPP) $(CFLAGS) $(INCLUDES) -c $<  -o $@


buildtest: gtest_main.a $(testobjs)
	$(CPP) $(CPPFLAGS) $(CXXFLAGS) -lpthread $^ -o $(testapp)

test: buildtest
	./$(testapp)
	

clean:
	rm -f *.o $(app) $(testapp)