#build server

CC := gcc
CXX := g++
CXX_FLAGS := -std=c++11 -m64 

ifeq ($(DEBUG),true)
	CXX_FLAGS += -ggdb -O0
	BUILD_TYPE := debug
else ifeq ($(DEBUG),false)
	CXX_FLAGS += -ggdb -O3
	BUILD_TYPE := releaseSymbol
else
	CXX_FLAGS += -O3
	BUILD_TYPE := release
endif

BUILD_DIR:=../..
# include paths
INC_DIR := -I$(BUILD_DIR)/Include \
		   -I$(BUILD_DIR)/Depend/jsoncpp \
		   -I$(BUILD_DIR)/Depend/openssl \
		   -I$(BUILD_DIR)/Depend/lua/src \
		   -I/usr/include/mysql \


BIN  := $(BUILD_DIR)/Bin/Server.bin
SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)
DEPS := $(SRCS:.cpp=.d)
LIBS := -L$(BUILD_DIR)/Lib -lAntEngine -lmysqlclient -llua -lssl -lcrypto -lz -lpthread -ldl

OBJ_DIR := $(BUILD_DIR)/Bin/Temp/$(BUILD_TYPE)/Server

OBJS := $(addprefix $(OBJ_DIR)/,$(OBJS))
DEPS := $(addprefix $(OBJ_DIR)/,$(DEPS))

#------------------------------------------------------
all:TEMP_DIR $(DEPS) $(OBJS) $(BIN)
	@echo "------------------------bin=$(BIN)"

$(BIN):$(OBJS)
	@echo "------------------------build $(BIN)"
	@echo "------------------------build $(BUILD_TYPE) mode"
	$(CXX) $(CXX_FLAGS) -o $@ $^ $(LIBS)


ifneq ("$(wildcard $(DEPS))","")
sinclude $(DEPS)
endif


$(OBJ_DIR)/%.o:%.cpp
	$(CXX) $(CXX_FLAGS) $(INC_DIR) -o $@ -c $<


$(OBJ_DIR)/%.d:%.cpp
	@echo -n "$(@D)/" > $@
	g++ $(INC_DIR) -MM $^ >> $@

TEMP_DIR:
	mkdir -p $(OBJ_DIR)
