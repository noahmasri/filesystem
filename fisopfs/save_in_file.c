#include "save_in_file.h"
#include "fisopfs.h"

// TYPE:name;mode;uid;gid;size;atime;mtime
#define FORMAT "%[^:]:%[^;];%d;%u;%u;%ld;%[^;];%[^\n]\n"
#define FILE_FORMAT "FILE:%s;%d;%u;%u;%ld;%s;%s\n"
#define DIR_FORMAT "DIR:%s;%d;%u;%u;%ld;%s;%s\n"
#define FILE_END "__ENDCONTENT__"

void
create_timespec_from_string(const char *datetime, struct timespec *timespec_struct)
{
	struct tm tm_struct;
	memset(&tm_struct, 0, sizeof(struct tm));
	memset(timespec_struct, 0, sizeof(struct timespec));

	if (strptime(datetime, "%Y-%m-%d %H:%M:%S", &tm_struct)) {
		timespec_struct->tv_sec = mktime(&tm_struct);
		timespec_struct->tv_nsec = 0;
	}
}

void
create_string_from_timespec(struct timespec date, char datetime[50])
{
	struct tm *tm = localtime(&(date.tv_sec));
	if (tm) {
		strftime(datetime, 50, "%Y-%m-%d %H:%M:%S", tm);
	}
}

void
write_all_files_data(size_t dir_index, FILE *data)
{
	for (size_t i = 0; i < directories[dir_index].file_count; i++) {
		file_t curr = files[directories[dir_index].file_pos[i]];
		char time_a[50];
		char time_m[50];
		create_string_from_timespec(curr.stats.atime, time_a);
		create_string_from_timespec(curr.stats.mtime, time_m);
		fprintf(data,
		        FILE_FORMAT,
		        curr.name,
		        curr.stats.mode,
		        curr.stats.uid,
		        curr.stats.gid,
		        curr.stats.size,
		        time_a,
		        time_m);
		fprintf(data, "%s\n", curr.content);
		fprintf(data, "%s\n", FILE_END);
	}
}

void
parsing(const char *path)
{
	FILE *data = fopen(path, "w");
	if (!data)
		return;

	write_all_files_data(0, data);
	size_t recorridos = 0;
	for (size_t j = 1; j < MAX_DIRECTORIES; j++) {
		if (directories[j].occupied) {
			directory_t curr = directories[j];
			char time_a[50];
			char time_m[50];
			create_string_from_timespec(curr.stats.atime, time_a);
			create_string_from_timespec(curr.stats.mtime, time_m);
			fprintf(data,
			        DIR_FORMAT,
			        curr.name,
			        curr.stats.mode,
			        curr.stats.uid,
			        curr.stats.gid,
			        curr.stats.size,
			        time_a,
			        time_m);
			write_all_files_data(j, data);
			recorridos++;
		}
		if (recorridos >= directory_count)
			break;
	}
	fclose(data);
}

void
handle_file_insertion(FILE *file,
                      char *cur_dir,
                      char *filename,
                      struct fisop_stats stats)
{
	char path[PATH_MAX];
	strcpy(path, cur_dir);
	strcat(path, "/");
	strcat(path, filename);
	char content[MAX_CONTENT];
	memset(content, 0, MAX_CONTENT);
	char linea[MAX_CONTENT];
	memset(linea, 0, MAX_CONTENT);
	int leidos = fscanf(file, "%[^\n]\n", linea);
	while (leidos != 0 && strcmp(linea, FILE_END) !=
	                              0) {  // read content until it says its over
		if (MAX_CONTENT - strlen(content) - 1 < strlen(linea))
			break;
		strcat(content, linea);
		printf("linea: %s. content ahora quedo como: %s\n", linea, content);
		leidos = fscanf(file, "%[^\n]\n", linea);
	}
	create_file(path, stats, content);
}

void
handle_directory_insertion(char *cur_dir, char *name, struct fisop_stats stats)
{
	strcpy(cur_dir, "");
	strcat(cur_dir, "/");
	strcat(cur_dir, name);
	create_dir(name, stats);
}

void
unparsing(char *path)
{
	if (strlen(path) > PATH_MAX)
		return;

	FILE *file = fopen(path, "r");
	if (!file)
		return;

	char cur_dir[PATH_MAX] = "";
	char name[PATH_MAX];
	char time_a[50];
	char time_m[50];
	struct fisop_stats cur_stats;
	char type[5];  // FILE or DIR

	while (fscanf(file,
	              FORMAT,
	              type,
	              name,
	              &cur_stats.mode,
	              &cur_stats.uid,
	              &cur_stats.gid,
	              &cur_stats.size,
	              time_a,
	              time_m) == 8) {
		if (strcmp(type, "DIR") == 0) {
			create_timespec_from_string(time_a, &cur_stats.atime);
			create_timespec_from_string(time_m, &cur_stats.mtime);
			handle_directory_insertion(cur_dir, name, cur_stats);
		} else if (strcmp(type, "FILE") == 0) {
			create_timespec_from_string(time_a, &cur_stats.atime);
			create_timespec_from_string(time_m, &cur_stats.mtime);
			handle_file_insertion(file, cur_dir, name, cur_stats);
		} else
			break;

		if (feof(file)) {
			break;
		}
	}

	fclose(file);
}