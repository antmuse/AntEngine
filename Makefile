#ture = debug
#false = release with symbol
#release = release without symbol
DEBUG ?= release
export BUILD_DIR = $(shell pwd)


MK_DIRS = $(BUILD_DIR)/Source \
		  $(BUILD_DIR)/Source/Test



.DEFAULT:
all:path
	@for dir in $(MK_DIRS); \
		do \
		make -C $$dir; \
		done
	@echo "------------------------build mode = $(DEBUG) finished"
	@ls -lhr ./Lib/*.a
	@ls -lhr ./Bin/*.bin



.PHONY: path
path:
	$(shell mkdir -p "$(BUILD_DIR)/Lib")



.PHONY: clean
clean:
	rm -rf Bin/AntTest.bin
	rm -rf Bin/Temp/*
	rm -rf Lib/libAntEngine.a
	@echo "------------------------cleared"


tags:
	ctags -R *


help:
	@echo "------------------------make, will build release bin without symbol"
	@echo "------------------------make DEBUG=true"
	@echo "------------------------make DEBUG=false, will build release bin with symbol"
