export DEBUG = true
export BUILD_DIR = $(shell pwd)


MK_DIRS = $(BUILD_DIR)/Source \
          $(BUILD_DIR)/Source/Test

.DEFAULT:
all:
	@for dir in $(MK_DIRS); \
		do \
		make -C $$dir; \
		done
	@echo "------------------------build Debug=$(DEBUG) finished"


.PHONY: clean
clean:
	rm -rf Bin/AntTest.bin
	rm -rf Bin/Temp/*
	rm -rf Lib/libAntEngine.a
	@echo "------------------------cleared"


tags:
	ctags -R *
