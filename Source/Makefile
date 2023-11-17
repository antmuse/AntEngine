#build lib

CC = gcc
CXX = g++
CXX_FLAGS = -std=c++11 -m64 

ifeq ($(DEBUG),true)
	CXX_FLAGS += -ggdb -O0
	BUILD_TYPE = debug
else ifeq ($(DEBUG),false)
	CXX_FLAGS += -ggdb -O3
	BUILD_TYPE = releaseSymbol
else
	CXX_FLAGS += -O3
	BUILD_TYPE = release
endif

# include paths
INC_DIR = -I$(BUILD_DIR)/Include \
		  -I$(BUILD_DIR)/Depend/http_parser \
		  -I$(BUILD_DIR)/Depend/jsoncpp \
		  -I$(BUILD_DIR)/Depend/openssl \
		  -I$(BUILD_DIR)/Depend/lua/src \
		  -I/usr/include/mysql \


SRCS = $(wildcard *.cpp)
SRCS += $(wildcard Linux/*.cpp)
SRCS += $(wildcard Net/*.cpp)
SRCS += $(wildcard db/*.cpp)
SRCS += $(wildcard Script/*.cpp)
SRCS += $(wildcard Net/HTTP/*.cpp)
SRCS += $(wildcard Net/RedisClient/*.cpp)
SRCS += $(wildcard ../Depend/http_parser/*.cpp)
SRCS += $(wildcard ../Depend/jsoncpp/*.cpp)

OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)
BIN = libAntEngine.a

BIN := $(addprefix $(BUILD_DIR)/Lib/,$(BIN))

OBJ_DIR = $(BUILD_DIR)/Bin/Temp/objs/$(BUILD_TYPE)/AntEngine
DEP_DIR = $(BUILD_DIR)/Bin/Temp/deps/AntEngine


OBJS := $(addprefix $(OBJ_DIR)/,$(OBJS))
DEPS := $(addprefix $(DEP_DIR)/,$(DEPS))


#------------------------------------------------------
all:TEMP_DIR $(DEPS) $(OBJS) $(BIN)
	@echo "------------------------bin=$(BIN)"


ifneq ("$(wildcard $(DEPS))","")
	-@include $(DEPS)  
endif


$(BIN):$(OBJS)
	@echo "------------------------build $(BIN)"
	@echo "------------------------build $(BUILD_TYPE) mode"
	@ar rcs $(BIN) $^


$(OBJ_DIR)/%.o:%.cpp
	$(CXX) $(CXX_FLAGS) $(INC_DIR) -o $@ -c $(filter %.cpp,$^)


$(DEP_DIR)/%.d:%.cpp
	$(CXX) $(INC_DIR) -MM $^ -MF $@

TEMP_DIR:
	#$(shell mkdir -p $(OBJ_DIR))
	#$(shell mkdir -p $(OBJ_DIR)/Net)
	$(shell mkdir -p $(OBJ_DIR)/Linux)
	$(shell mkdir -p $(OBJ_DIR)/db)
	$(shell mkdir -p $(OBJ_DIR)/Script)
	$(shell mkdir -p $(OBJ_DIR)/Net/HTTP)
	$(shell mkdir -p $(OBJ_DIR)/Net/RedisClient)
	$(shell mkdir -p $(OBJ_DIR)/../Depend/http_parser)
	$(shell mkdir -p $(OBJ_DIR)/../Depend/jsoncpp)
	#$(shell mkdir -p $(DEP_DIR))
	#$(shell mkdir -p $(DEP_DIR)/Net)
	$(shell mkdir -p $(DEP_DIR)/Linux)
	$(shell mkdir -p $(DEP_DIR)/db)
	$(shell mkdir -p $(DEP_DIR)/Script)
	$(shell mkdir -p $(DEP_DIR)/Net/HTTP)
	$(shell mkdir -p $(DEP_DIR)/Net/RedisClient)
	$(shell mkdir -p $(DEP_DIR)/../Depend/http_parser)
	$(shell mkdir -p $(DEP_DIR)/../Depend/jsoncpp)

