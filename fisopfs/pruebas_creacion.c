#define _DEFAULT_SOURCE
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <sys/file.h>
#include <unistd.h>

void
creating_file_returns_valid_file_descriptor()
{
	const char path[100] = "cami.txt";
	const char path2[100] = "manu.txt";
	printf("inicia prueba: crear los archivos %s y %s en el directorio "
	       "root\n",
	       path,
	       path2);
	int new_file = creat(path, S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO);
	assert(new_file > 2);
	int new_file2 = creat(path2, S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO);
	assert(new_file2 > 2);
	printf("fin prueba \n---\n");
	close(new_file);
	close(new_file2);
}

void
creating_existing_file_returns_invalid_file_descriptor()
{
	const char path[100] = "cami.txt";
	printf("inicia prueba: crear nuevamente archivo %s en root\n", path);
	int new_file = creat(path, S_IFREG | S_IRUSR | S_IWUSR);
	assert(new_file < 0);
	printf("fin prueba \n---\n");
}

void
creating_file_in_invalid_dir_returns_invalid_file_descriptor()
{
	const char path[100] = "hola/noah.txt";
	printf("inicia prueba: crear archivo %s (no existe la carpeta hola)\n",
	       path);
	int new_file = creat(path, S_IFREG | S_IRUSR | S_IWUSR);
	assert(new_file < 0);
	printf("fin prueba \n---\n");
}

void
creating_file_in_now_valid_dir_returns_valid_file_descriptor()
{
	const char path[100] = "hola/noah.txt";
	printf("inicia prueba: crear archivo %s (ahora si existe hola)\n", path);
	int new_file = creat(path, S_IFREG | S_IRUSR | S_IWUSR);
	assert(new_file > 2);
	printf("fin prueba \n---\n");
	close(new_file);
}

void
creating_directory_works()
{
	const char path[100] = "hola";
	printf("inicia prueba: crear directorio %s\n", path);
	int new_dir = mkdir(path, S_IFDIR | S_IRUSR | S_IWUSR);
	assert(new_dir == 0);
	printf("fin prueba \n---\n");
	creating_file_in_now_valid_dir_returns_valid_file_descriptor();
}

void
creating_in_other_than_root_shouldnt_work()
{
	printf("inicia prueba: crear directorio dentro de otro directorio\n");
	const char path[100] = "hola/patricio";
	int new_dir = mkdir(path, S_IFDIR | S_IRUSR | S_IWUSR);
	assert(new_dir == -1);
	printf("fin prueba \n---\n");
}

int
main()
{
	creating_file_returns_valid_file_descriptor();
	creating_existing_file_returns_invalid_file_descriptor();
	creating_file_in_invalid_dir_returns_invalid_file_descriptor();
	creating_directory_works();
	creating_in_other_than_root_shouldnt_work();
	return 0;
}