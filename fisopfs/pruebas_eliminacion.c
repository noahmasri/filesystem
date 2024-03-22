#define _DEFAULT_SOURCE
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <sys/file.h>
#include <unistd.h>

void
removing_a_directory_works_and_allows_you_to_create_it_again()
{
	printf("inicia prueba: eliminar directorio vacio y recrearlo\n");
	const char path[100] = "se_borra";
	int new_dir = mkdir(path, S_IFDIR | S_IRUSR | S_IWUSR);
	assert(new_dir == 0);
	int removed = rmdir(path);
	assert(removed == 0);
	int new_dir_again = mkdir(path, S_IFDIR | S_IRUSR | S_IWUSR);
	assert(new_dir_again == 0);
	printf("fin prueba \n---\n");
}

void
deleting_file_works_and_allows_you_to_create_it_again()
{
	printf("inicia prueba: eliminar archivo\n");
	const char path[100] = "camila.txt";
	int new_file = creat(path, S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO);
	assert(new_file > 0);
	close(new_file);
	int status = remove(path);
	int new_file_denuevo = creat(path, S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO);
	assert(new_file_denuevo > 0);
	close(new_file_denuevo);
	printf("fin prueba \n---\n");
}

void
deleting_files_in_a_directory_non_root_allows_you_to_create_it_again()
{
	const char path[100] = "soy_un_dir";
	printf("inicia prueba: borrar archivo en directorio interno \n");
	int new_dir = mkdir(path, S_IFDIR | S_IRUSR | S_IWUSR);
	assert(new_dir == 0);
	const char path_archivo[100] = "soy_un_dir/soy_un_archivo.txt";
	int new_file = creat(path_archivo, S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO);
	assert(new_file > 0);
	close(new_file);
	int status = remove(path_archivo);
	int new_file_denuevo =
	        creat(path_archivo, S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO);
	assert(new_file_denuevo > 0);
	close(new_file_denuevo);
	printf("fin prueba \n---\n");
}

int
main()
{
	removing_a_directory_works_and_allows_you_to_create_it_again();
	deleting_file_works_and_allows_you_to_create_it_again();
	deleting_files_in_a_directory_non_root_allows_you_to_create_it_again();
	return 0;
}