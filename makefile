#环境定义
COMPLIE_PREFIX =
#生成的执行文件定义
OUT_BIN = test_server
ENV_FLAG_LINK = -g  -m64 -DYY_P_HAS_IPV6 -DYY_ENV_X86 -DYY_ENV_LINUX  -DYY_ENV_M64 -D_LARGEFILE64_SOURCE=1 -D_FILE_OFFSET_BITS=64 -Wall 
ENV_FLAG_C = -g  -m64 -DYY_P_HAS_IPV6 -DYY_ENV_X86  -DYY_ENV_LINUX  -DYY_ENV_M64 -D_LARGEFILE64_SOURCE=1 -D_FILE_OFFSET_BITS=64 -Wall 

#环境定义
CPP=$(COMPLIE_PREFIX)g++
CC=$(COMPLIE_PREFIX)gcc

CFLAGS =   $(ENV_FLAG_C)
CPPFLAGS =   $(ENV_FLAG_C)
LINKFLAGS =  $(ENV_FLAG_LINK)

LIBS = -lpthread -lrt
OBJS_CPP = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
OBJS_C += $(patsubst %.c,%.o,$(wildcard *.c))
OBJS_ALL = $(OBJS_CPP) $(OBJS_C)
OBJS_EXBIN = 
OBJS_EXLIB =
OBJS_BIN = $(filter-out $(OBJS_EXBIN) , $(OBJS_ALL))
OBJS_LIB = $(filter-out $(OBJS_EXLIB) , $(OBJS_ALL))

OUT_LIB = lib$(OUT_BIN).a

all:tag_parpare  $(OUT_BIN) 

#准备工作
tag_parpare:clean
	mkdir -p ./bin
	mkdir -p ./objs	

$(OUT_BIN):$(OBJS_BIN)	
	$(CPP) $(LINKFLAGS) -o ./bin/$(OUT_BIN) $(addprefix ./objs/,$(OBJS_BIN)) $(LIBS) 
	#$(STRIP) ./bin/$(OUT_BIN)
	
%.o:%.cpp
	$(CPP) -c $(CPPFLAGS)  $< -o ./objs/$@

%.o:%.c
	$(CC) -c $(CFLAGS)   $< -o ./objs/$@

clean:
	rm -rf  ./objs/*.o *.o  ./bin/$(OUT_BIN)
