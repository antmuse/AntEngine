#build lib

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

BUILD_DIR:=..

# include paths
INC_DIR := -I$(BUILD_DIR)/Include \
		   -I$(BUILD_DIR)/Depend/jsoncpp \
		   -I$(BUILD_DIR)/Depend/openssl \
		   -I$(BUILD_DIR)/Depend/lua/src \
		   -I/usr/include/mysql \


SRCS := $(wildcard *.cpp)
SRCS += $(wildcard Linux/*.cpp)
SRCS += $(wildcard Net/*.cpp)
SRCS += $(wildcard db/*.cpp)
SRCS += $(wildcard Script/*.cpp)
SRCS += $(wildcard Net/HTTP/*.cpp)
SRCS += $(wildcard Net/RedisClient/*.cpp)
SRCS += $(wildcard ../Depend/http_parser/*.cpp)
SRCS += $(wildcard ../Depend/jsoncpp/*.cpp)

OBJS := $(SRCS:.cpp=.o)
DEPS := $(SRCS:.cpp=.d)

BIN = libAntEngine.a
BIN := $(addprefix $(BUILD_DIR)/Lib/,$(BIN))

OBJ_DIR = $(BUILD_DIR)/Bin/Temp/$(BUILD_TYPE)/AntEngine

OBJS := $(addprefix $(OBJ_DIR)/,$(OBJS))
DEPS := $(addprefix $(OBJ_DIR)/,$(DEPS))


#------------------------------------------------------
all:TEMP_DIR $(DEPS) $(OBJS) $(BIN)
	@echo "------------------------bin=$(BIN)"


$(BIN):$(OBJS)
	@echo "------------------------build $(BIN)"
	@echo "------------------------build $(BUILD_TYPE) mode"
	@ar rcs $(BIN) $^


ifneq ("$(wildcard $(DEPS))","")
sinclude $(DEPS)  
endif

$(OBJ_DIR)/%.o:%.cpp
	# $(CXX) $(CXX_FLAGS) $(INC_DIR) -o $@ -c $(filter %.cpp,$^)
	$(CXX) $(CXX_FLAGS) $(INC_DIR) -o $@ -c $<


$(OBJ_DIR)/%.d:%.cpp
	# $(CXX) $(INC_DIR) -MM $^ -MF $@
	@echo -n "$(@D)/" > $@
	g++ $(INC_DIR) -MM $^ >> $@

TEMP_DIR:
	# deps dir same as objs dir.
	$(shell mkdir -p $(OBJ_DIR)/Linux)
	$(shell mkdir -p $(OBJ_DIR)/db)
	$(shell mkdir -p $(OBJ_DIR)/Script)
	$(shell mkdir -p $(OBJ_DIR)/Net/HTTP)
	$(shell mkdir -p $(OBJ_DIR)/Net/RedisClient)
	$(shell mkdir -p $(OBJ_DIR)/../Depend/jsoncpp)

