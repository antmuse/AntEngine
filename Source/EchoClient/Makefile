#build EchoClient

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


BIN = EchoClient.bin
SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)
LIBS = -L$(BUILD_DIR)/Lib -lAntEngine -lmysqlclient -llua -lssl -lcrypto -lz -lpthread -ldl
BIN := $(addprefix $(BUILD_DIR)/Bin/,$(BIN))

OBJ_DIR = $(BUILD_DIR)/Bin/Temp/objs/$(BUILD_TYPE)/EchoClient
DEP_DIR = $(BUILD_DIR)/Bin/Temp/deps/EchoClient

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
	$(CXX) $(CXX_FLAGS) -o $@ $^ $(LIBS)


$(OBJ_DIR)/%.o:%.cpp
	$(CXX) $(CXX_FLAGS) $(INC_DIR) -o $@ -c $(filter %.cpp,$^)


$(DEP_DIR)/%.d:%.cpp
	$(CXX) $(INC_DIR) -MM $^ -MF $@

TEMP_DIR:
	$(shell mkdir -p $(OBJ_DIR)/EchoClient)
	$(shell mkdir -p $(DEP_DIR)/EchoClient)
