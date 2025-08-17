#build nat server & client

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
		   -I$(BUILD_DIR)/Source/ServiceNAT \
 		   -I$(BUILD_DIR)/Source/ServiceNAT/ServerNAT \
		   -I$(BUILD_DIR)/Source/ServiceNAT/ClientNAT \
		   -I$(BUILD_DIR)/Depend/jsoncpp \
		   -I$(BUILD_DIR)/Depend/openssl \
		   -I$(BUILD_DIR)/Depend/lua/src \
		   -I/usr/include/mysql


BIN_SEV  := $(BUILD_DIR)/Bin/ServerNAT.bin
BIN_CLI  := $(BUILD_DIR)/Bin/ClientNAT.bin
SRCS_PUBLIC := $(wildcard *.cpp)
SRCS_SEV := ${SRCS_PUBLIC}
SRCS_SEV += $(wildcard ServerNAT/*.cpp)
SRCS_CLI := ${SRCS_PUBLIC}
SRCS_CLI += $(wildcard ClientNAT/*.cpp)
OBJS_SEV := $(SRCS_SEV:.cpp=.o)
DEPS_SEV := $(SRCS_SEV:.cpp=.d)
OBJS_CLI := $(SRCS_CLI:.cpp=.o)
DEPS_CLI := $(SRCS_CLI:.cpp=.d)
LIBS := -L$(BUILD_DIR)/Lib -lAntEngine -lmysqlclient -llua -lssl -lcrypto -lz -lpthread -ldl

OBJ_DIR := $(BUILD_DIR)/Bin/Temp/$(BUILD_TYPE)/ServiceNAT
OBJ_DIR_SEV := $(BUILD_DIR)/Bin/Temp/$(BUILD_TYPE)/ServiceNAT/ServerNAT
OBJ_DIR_CLI := $(BUILD_DIR)/Bin/Temp/$(BUILD_TYPE)/ServiceNAT/ClientNAT

OBJS_SEV := $(addprefix $(OBJ_DIR)/,$(OBJS_SEV))
DEPS_SEV := $(addprefix $(OBJ_DIR)/,$(DEPS_SEV))
OBJS_CLI := $(addprefix $(OBJ_DIR)/,$(OBJS_CLI))
DEPS_CLI := $(addprefix $(OBJ_DIR)/,$(DEPS_CLI))

#------------------------------------------------------
all:TEMP_DIR $(DEPS_SEV) $(DEPS_CLI) $(OBJS_SEV) $(OBJS_CLI) $(BIN_SEV) $(BIN_CLI)
	@echo "------------------------bin=$(BIN_SEV)"
	@echo "------------------------bin=$(BIN_CLI)"

$(BIN_SEV):$(OBJS_SEV)
	@echo "------------------------build $(BIN_SEV)"
	@echo "------------------------build $(BUILD_TYPE) mode"
	$(CXX) $(CXX_FLAGS) -o $@ $^ $(LIBS)

$(BIN_CLI):$(OBJS_CLI)
	@echo "------------------------build $(BIN_SEV)"
	@echo "------------------------build $(BUILD_TYPE) mode"
	$(CXX) $(CXX_FLAGS) -o $@ $^ $(LIBS)

ifneq ("$(wildcard $(DEPS_SEV))","")
sinclude $(DEPS_SEV)
endif

ifneq ("$(wildcard $(DEPS_CLI))","")
sinclude $(DEPS_CLI)
endif

$(OBJ_DIR)/%.o:%.cpp
	$(CXX) $(CXX_FLAGS) $(INC_DIR) -o $@ -c $<


$(OBJ_DIR)/%.d:%.cpp
	@echo -n "$(@D)/" > $@
	g++ $(INC_DIR) -MM $^ >> $@



TEMP_DIR:
	mkdir -p $(OBJ_DIR_SEV)
	mkdir -p $(OBJ_DIR_CLI)
