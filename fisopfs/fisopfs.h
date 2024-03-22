#ifndef __FS__H__
#define __FS__H__

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#define MAX_CONTENT 4096
#define MAX_FILES 1000
#define MAX_DIRECTORIES 500

struct fisop_stats {
	mode_t mode;
	uid_t uid;             /* User ID of file owner */
	gid_t gid;             /* Group ID of file owner */
	off_t size;            /* Total file size (bytes) */
	struct timespec atime; /* Time of last file access */
	struct timespec mtime; /* Time of last file modification */
};

typedef struct file {
	char name[PATH_MAX];
	struct fisop_stats stats;
	char content[MAX_CONTENT];
	int parent_dir;  // if 0 then its in root
} file_t;

// no parent directory, always in root
typedef struct directory {
	char name[PATH_MAX];
	struct fisop_stats stats;
	size_t file_pos[MAX_FILES];  // posicion del vector en el vector de files
	size_t file_count;
	bool occupied;
} directory_t;

extern size_t file_count;  // cantidad de archivos en uso
extern size_t free_file_list[MAX_FILES];  // posiciones en el vector de archivos que no estan en uso
extern file_t files[MAX_FILES];

extern size_t directory_count;  // cantidad de directorios en uso, arranca en 1, por el root
extern size_t free_directory_list[MAX_DIRECTORIES];  // posiciones en el vector de directorios que no estan en uso
extern directory_t directories[MAX_DIRECTORIES];


int create_file(char *path, struct fisop_stats stats, char *content);
void create_dir(char *name, struct fisop_stats stats);
struct fisop_stats load_creation_stats(mode_t mode);
#endif