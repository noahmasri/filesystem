#define FUSE_USE_VERSION 30
#define _DEFAULT_SOURCE
#include <fuse.h>
#include "fisopfs.h"
#include "save_in_file.h"

#define END_STRING '\0'

enum type {
	DIRECTORY,
	FIL,
};

size_t file_count = 0;  // cantidad de archivos en us
size_t free_file_list[MAX_FILES];  // posiciones en el vector de archivos que no estan en uso
file_t files[MAX_FILES] = { 0 };

size_t directory_count =
        1;  // cantidad de directorios en uso, arranca en 1, por el root
size_t free_directory_list[MAX_DIRECTORIES];  // posiciones en el vector de directorios que no estan en uso
directory_t directories[MAX_DIRECTORIES] = { 0 };


void *
fisopfs_init(struct fuse_conn_info *conn)
{
	printf("[debug] fisopfs_init");
	for (size_t i = 0; i < MAX_FILES; i++) {
		free_file_list[i] = MAX_FILES - i - 1;
	}

	// insertamos al reves para intentar disminuir complejidad en busqueda
	for (size_t i = 1; i < MAX_DIRECTORIES; i++) {
		free_directory_list[i] = MAX_DIRECTORIES - i - 1;
	}

	mode_t mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
	directories[0].stats = load_creation_stats(mode);  // add root directory
	directories[0].occupied = true;
	directories[0].file_count = 0;

	unparsing("our_fs.fisopfs");
	return conn;
}

// separa el string cuando encuentra splitter. modifica el vector original
char *
split_line(char *buf, char splitter)
{
	int i = 0;
	while (buf[i] != splitter && buf[i] != END_STRING)
		i++;

	if (buf[i] == '\0')
		return NULL;

	buf[i++] = END_STRING;

	return &buf[i];
}

// busca un archivo en un directorio en el indice dado
int
search_in_dir(size_t index, char *path)
{
	directory_t found_dir = directories[index];
	for (size_t j = 0; j < found_dir.file_count; j++) {
		if (strcmp(files[found_dir.file_pos[j]].name, path) == 0) {
			return j;
		}
	}
	return -ENOENT;
}

// recorre todo el root buscando la entrada pedida y devuelve su indice en el
// vector de entradas sin importar si es directorio o archivo. devuelve el
// indice en el vector de entries del root.
int
search_for_entry_in_root(char *path, enum type *type)
{
	int index = search_in_dir(0, path);
	if (index >= 0) {
		if (type)
			*type = FIL;
		return directories[0].file_pos[index];
	}

	int recorridos = 0;
	for (size_t i = 1; i < MAX_DIRECTORIES; i++) {
		if (!directories[i].occupied)
			continue;
		if (strcmp(directories[i].name, path) == 0) {
			if (type)
				*type = DIRECTORY;
			return i;
		}
		recorridos++;
		if (recorridos >= directory_count)
			break;
	}
	return -1;
}

void
load_stats(struct stat *st, struct fisop_stats stats)
{
	st->st_uid = stats.uid;
	st->st_gid = stats.gid;
	st->st_mode = stats.mode;
	st->st_atim = stats.atime;
	st->st_mtim = stats.mtime;
	st->st_size = stats.size;
	st->st_nlink = 1;
}

// busca el indice en el root y carga esas stats
int
update_stats_searching_in_root(char *entry, struct stat *st)
{
	enum type type;
	int entry_pos = search_for_entry_in_root(entry, &type);
	if (entry_pos == -1)
		return -ENOENT;
	struct fisop_stats stats;
	if (type == DIRECTORY)
		stats = directories[entry_pos].stats;
	else
		stats = files[entry_pos].stats;

	load_stats(st, stats);
	return 0;
}

// busca la primera parte del path (el directorio en que esta el archivo) en el root
// y luego busca el archivo en dicho directorio hallado. luego carga las stats de este.
int
update_stats_searching_in_other_dirs(char *entry,
                                     char *second_entry,
                                     struct stat *st)
{
	enum type type;
	int i = search_for_entry_in_root(entry, &type);
	if (i < 0 || type == FIL) {
		return -ENOENT;
	}
	int j = search_in_dir(i, second_entry);
	if (j < 0) {
		return -ENOENT;
	}
	directory_t found_dir = directories[i];
	struct fisop_stats stats = files[found_dir.file_pos[j]].stats;
	load_stats(st, stats);
	return 0;
}

static int
fisopfs_getattr(const char *path, struct stat *st)
{
	printf("[debug] fisopfs_getattr - path: %s\n", path);
	if (!path || strlen(path) == 0)
		return -ENOENT;

	if (strcmp(path, "/") == 0) {
		struct fisop_stats stats = directories[0].stats;
		load_stats(st, stats);
		return 0;
	}

	char aux[PATH_MAX];
	strcpy(aux, path);
	char *entry = split_line((char *) aux, '/');
	char *second_entry = split_line(entry, '/');

	if (!second_entry || strlen(second_entry) == 0) {
		return update_stats_searching_in_root(entry, st);
	}

	return update_stats_searching_in_other_dirs(entry, second_entry, st);
}

// busca un directorio. como todos los directorios estan en root, es indistinto
// buscarlo en los entries que en el vector de directorios.
int
search_for_directory(char *path)
{
	int recorridos = 0;
	for (size_t i = 0; i < directory_count; i++) {
		if (!directories[i].occupied)
			continue;
		if (strcmp(directories[i].name, path) == 0)
			return i;
		recorridos++;
		if (recorridos >= directory_count)
			break;
	}
	return -1;
}

// carga todos los archivos del directorio en buffer mientras
// siga habiendo lugar en buffer.
void
store_dir(void *buffer, size_t index, fuse_fill_dir_t filler)
{
	directory_t dir = directories[index];
	for (size_t i = 0; i < dir.file_count; i++) {
		int status = filler(buffer, files[dir.file_pos[i]].name, NULL, 0);
		if (status == 1)
			break;
	}
}

// carga todas las entradas del directorio raiz en buffer mientras
// siga habiendo lugar en buffer.
void
store_root(void *buffer, fuse_fill_dir_t filler)
{
	store_dir(buffer, 0, filler);

	size_t recorridos = 0;
	int status;
	for (size_t i = 1; i < MAX_DIRECTORIES; i++) {
		if (directories[i].occupied) {
			status = filler(buffer, directories[i].name, NULL, 0);
			if (status == 1)
				return;
			recorridos++;
			if (recorridos >= directory_count)
				break;
		}
	}
}

static int
fisopfs_readdir(const char *path,
                void *buffer,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_readdir - path: %s\n", path);

	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);

	// Si nos preguntan por el directorio raiz
	if (strcmp(path, "/") == 0) {
		clock_gettime(CLOCK_REALTIME, &directories[0].stats.atime);
		store_root(buffer, filler);
		return 0;
	}

	char aux[PATH_MAX];
	strcpy(aux, path);
	char *entry = split_line(aux, '/');
	char *second_entry = split_line(entry, '/');
	if (second_entry && strlen(second_entry) > 0)
		return -ENOTDIR;
	else {
		int index = search_for_directory(entry);
		if (index < 0)
			return -ENOENT;
		clock_gettime(CLOCK_REALTIME, &directories[index].stats.atime);
		store_dir(buffer, index, filler);
	}
	return 0;
}

// busca el indice absoluto del archivo, es decir, su posicion en el vector de
// archivos. si es requerido ademas, se le puede pasar los parametros
// index_in_dir y parent_dir, que devuelven la posicion en el vector interno de
// parent dir, y la posicion en el vector de directorios de dicho parent dir.
int
find_file_index(char *entry, char *second_entry, int *index_in_dir, int *parent_dir)
{
	if (!second_entry || strlen(second_entry) == 0) {
		int index = search_in_dir(0, entry);
		if (index < 0)
			return -ENOENT;
		if (parent_dir)
			*parent_dir = 0;
		if (index_in_dir)
			*index_in_dir = index;
		return directories[0].file_pos[index];
	}

	int dir_index = search_for_directory(entry);
	if (dir_index < 0)
		return -ENOENT;

	if (parent_dir)
		*parent_dir = dir_index;

	int file_index = search_in_dir(dir_index, second_entry);
	if (file_index < 0)
		return -ENOENT;
	if (index_in_dir)
		*index_in_dir = file_index;
	return directories[dir_index].file_pos[file_index];
}

// chequea si el archivo tiene los permisos necesarios para realizar la accion del modo dicho
int
check_permits(file_t file, int mode)
{
	switch (mode) {
	case R_OK:
		if ((file.stats.uid == getuid() && (file.stats.mode & S_IRUSR)) ||
		    (file.stats.gid == getgid() && (file.stats.mode & S_IRGRP)) ||
		    (file.stats.mode & S_IROTH)) {
			return 0;
		}
	case W_OK:
		if ((file.stats.uid == getuid() && (file.stats.mode & S_IWUSR)) ||
		    (file.stats.gid == getgid() && (file.stats.mode & S_IWGRP)) ||
		    (file.stats.mode & S_IWOTH)) {
			return 0;
		}
	case X_OK:
		if ((file.stats.uid == getuid() && (file.stats.mode & S_IXUSR)) ||
		    (file.stats.gid == getgid() && (file.stats.mode & S_IXGRP)) ||
		    (file.stats.mode & S_IXOTH)) {
			return 0;
		}
	}
	printf("[debug] no permits");
	return -EACCES;
}

static int
fisopfs_read(const char *path,
             char *buffer,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	if (!path || !buffer)
		return -ENOENT;

	printf("[debug] fisopfs_read- path: %s\n", path);
	char aux[PATH_MAX];
	strcpy(aux, path);
	char *entry = split_line(aux, '/');
	char *second_entry = split_line(entry, '/');

	int index = find_file_index(entry, second_entry, NULL, NULL);
	if (index < 0) {
		return index;
	}

	file_t file = files[index];

	if (offset + size > file.stats.size)
		size = file.stats.size - offset;

	size = size > 0 ? size : 0;
	strncpy(buffer, file.content + offset, size);
	clock_gettime(CLOCK_REALTIME, &file.stats.atime);

	return size;
}

// returns filename
char *
add_to_directory(char *path, size_t *parent_dir)
{
	if (!path || strlen(path) == 0)
		return NULL;

	char aux[PATH_MAX];
	strcpy(aux, path);
	char *entry = split_line(aux, '/');
	char *second_entry = split_line(entry, '/');

	if (!second_entry || strlen(second_entry) == 0) {
		if (parent_dir) {
			*parent_dir = 0;
		}
		return entry;
	} else {
		for (size_t i = 1; i < directory_count; i++) {
			if (directories[i].occupied &&
			    strcmp(directories[i].name, entry) == 0) {
				if (parent_dir) {
					*parent_dir = i;
				}
				return second_entry;
			}
		}
	}

	return NULL;
}

struct fisop_stats
load_creation_stats(mode_t mode)
{
	struct fisop_stats stats;
	stats.mode = mode;
	stats.uid = getuid();
	stats.gid = getgid();
	stats.size = 0;
	clock_gettime(CLOCK_REALTIME, &stats.atime);
	clock_gettime(CLOCK_REALTIME, &stats.mtime);
	return stats;
}

int
fisopfs_access(const char *path, int mode)
{
	if (!path) {
		return -ENOENT;
	}
	printf("[debug] fisopfs_access- path: %s\n", path);
	char aux[PATH_MAX];
	strcpy(aux, path);
	char *entry = split_line(aux, '/');
	char *second_entry = split_line(entry, '/');

	int index = find_file_index(entry, second_entry, NULL, NULL);
	if (index < 0)
		return index;

	file_t file = files[index];
	return check_permits(file, mode);
}

int
create_file(char *path, struct fisop_stats stats, char *content)
{
	file_t nuevo;
	nuevo.stats = stats;
	size_t parent_dir;
	char *file_name = add_to_directory(path, &parent_dir);
	if (!file_name)
		return -1;
	strcpy(nuevo.name, file_name);
	if (!content) {
		memset(nuevo.content, 0, MAX_CONTENT);
	} else {
		strcpy(nuevo.content, content);
	}

	nuevo.parent_dir = parent_dir;
	file_count++;
	int new_index = free_file_list[MAX_FILES - file_count];
	files[new_index] = nuevo;

	directories[parent_dir].file_pos[directories[parent_dir].file_count] =
	        new_index;
	directories[parent_dir].file_count++;

	return 0;
}

static int
fisopfs_creat(const char *pathname, mode_t mode, struct fuse_file_info *info)
{
	printf("[debug] fisopfs_creat - path: %s\n", pathname);
	if (file_count >= MAX_FILES) {
		return -ENOMEM;
	}

	if (strlen(pathname) > PATH_MAX) {
		return -E2BIG;
	}

	if (!(mode & S_IFMT)) {
		mode = S_IFREG | mode;
	}

	if (!S_ISREG(mode)) {
		return -EINVAL;
	}

	char aux[PATH_MAX];
	strcpy(aux, pathname);
	struct fisop_stats stats = load_creation_stats(mode);
	if (create_file(aux, stats, NULL) != 0) {
		return -EINVAL;
	}

	return 0;
}

void
create_dir(char *name, struct fisop_stats stats)
{
	directory_t nuevo;
	strcpy(nuevo.name, name);
	nuevo.stats = stats;
	nuevo.file_count = 0;
	nuevo.occupied = true;
	directory_count++;
	int new_index = free_directory_list[MAX_DIRECTORIES - directory_count];
	directories[new_index] = nuevo;
}

static int
fisopfs_mkdir(const char *pathname, mode_t mode)
{
	printf("[debug] fisopfs_mkdir - path: %s\n", pathname);
	if (directory_count >= MAX_DIRECTORIES) {
		return -ENOMEM;
	}

	if (strlen(pathname) > PATH_MAX) {
		return -E2BIG;
	}

	if (!(mode & S_IFMT)) {
		mode = S_IFDIR | mode;
	}

	if (!S_ISDIR(mode)) {
		return -EINVAL;
	}

	create_dir((char *) pathname + 1, load_creation_stats(mode));
	return 0;
}

int
fisopfs_write(const char *path,
              const char *buffer,
              size_t size,
              off_t offset,
              struct fuse_file_info *info)
{
	if (!path || !buffer) {
		return -ENOENT;
	}
	printf("[debug] fisopfs_write - path: %s\n", path);
	char aux[PATH_MAX];
	strcpy(aux, path);
	char *entry = split_line(aux, '/');
	char *second_entry = split_line(entry, '/');

	int index = find_file_index(entry, second_entry, NULL, NULL);
	if (index < 0)
		return index;

	if (strlen(files[index].content) >= MAX_CONTENT)
		return -ENOMEM;

	if (offset > files[index].stats.size) {
		offset = files[index].stats.size;
	}

	if (offset + size > MAX_CONTENT - 1)
		size = MAX_CONTENT - 1 - offset;

	size = size > 0 ? size : 0;

	strncpy(files[index].content + offset, buffer, size);
	files[index].stats.size = strlen(files[index].content) + 1;
	files[index].content[strlen(files[index].content) + 1] = '\0';
	clock_gettime(CLOCK_REALTIME, &files[index].stats.mtime);
	clock_gettime(CLOCK_REALTIME, &files[index].stats.atime);

	return size;
}


static int
fisopfs_rmdir(const char *path)
{
	printf("[debug] fisopfs_rmdir, path: %s\n", path);
	char aux[PATH_MAX];
	strcpy(aux, path);
	char *entry = split_line(aux, '/');
	char *second_entry = split_line(entry, '/');
	if (second_entry)
		return -EACCES;
	int dir_index = search_for_directory(entry);
	if (dir_index < 0)
		return dir_index;

	directory_t direc = directories[dir_index];
	if (direc.file_count > 0)
		return -ENOTEMPTY;
	directories[dir_index].occupied = false;
	free_directory_list[MAX_DIRECTORIES - directory_count] = dir_index;
	directory_count--;
	return 0;
}


static int
fisopfs_unlink(const char *path)
{
	printf("[debug] fisopfs_unlink - path: %s \n", path);
	char aux[PATH_MAX];
	strcpy(aux, path);
	char *entry = split_line(aux, '/');
	char *second_entry = split_line(entry, '/');
	int index_in_dir;
	int parent_dir;
	int index =
	        find_file_index(entry, second_entry, &index_in_dir, &parent_dir);
	if (index < 0) {
		return -ENOENT;
	}

	free_file_list[MAX_FILES - file_count] = index;
	file_count--;

	directories[parent_dir].file_count--;
	directories[parent_dir].file_pos[index_in_dir] =
	        directories[parent_dir].file_pos[directories[parent_dir].file_count];

	return 0;
}

int
fisopfs_flush(const char *path, struct fuse_file_info *info)
{
	parsing("our_fs.fisopfs");
	return 0;
}

void
fisopfs_destroy(void *info)
{
	printf("[debug] calling destroy \n");
	fisopfs_flush("our_fs.fisopfs", NULL);
}

static struct fuse_operations operations = {
	.init = fisopfs_init,
	.getattr = fisopfs_getattr,
	.readdir = fisopfs_readdir,
	.create = fisopfs_creat,
	.mkdir = fisopfs_mkdir,
	.write = fisopfs_write,
	.rmdir = fisopfs_rmdir,
	.unlink = fisopfs_unlink,
	.read = fisopfs_read,
	.destroy = fisopfs_destroy,
	.flush = fisopfs_flush,
};

int
main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &operations, NULL);
}
