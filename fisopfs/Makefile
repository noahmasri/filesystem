CC = gcc save_in_file.c
CFLAGS := -ggdb3 -O2 -Wall -std=c11
CFLAGS += -Wno-unused-function -Wvla

# Flags for FUSE
LDLIBS := $(shell pkg-config fuse --cflags --libs)

# Name for the filesystem!
FS_NAME := fisopfs

all: build
	
build: $(FS_NAME)

format: .clang-files .clang-format
	xargs -r clang-format -i <$<

docker-build:
	docker build -t fisopfs:latest .

docker-run: docker-build
	docker container run -it --rm --name=fisopfs \
		--device /dev/fuse \
		--cap-add SYS_ADMIN \
		--security-opt apparmor:unconfined \
		fisopfs:latest \
		bash 

docker-attach: docker-build
	docker exec -it fisopfs bash

clean:
	rm -rf $(EXEC) *.o core vgcore.* $(FS_NAME)

.PHONY: all build clean format docker-build docker-run docker-attach