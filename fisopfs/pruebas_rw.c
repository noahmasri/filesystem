#define _DEFAULT_SOURCE
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <sys/file.h>
#include <unistd.h>

static void
writing_in_file(void)
{
	printf("inicia prueba escritura en archivo\n");
	const char path[100] = "archivo1.txt";
	int new_file = creat(path, S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO);
	char buf[20] = "hola como va?";
	int state = write(new_file, buf, strlen(buf));

	assert(state == strlen(buf));
	close(new_file);
	printf("fin prueba \n---\n");
}

static void
reading_in_file_with_nothing_in_it(void)
{
	printf("inicia prueba lectura en archivo vacio\n");
	const char path[100] = "archivo2.txt";
	int new_file = creat(path, S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO);
	close(new_file);
	int fd = open(path, O_RDWR);
	char buf[6];
	int state = read(fd, buf, 5);
	assert(state == 0);
	printf("leer dio %i\n", state);
	close(fd);
	printf("fin prueba \n---\n");
}

static void
read_in_file_with_something_in_it(void)
{
	printf("inicia prueba lectura de archivo no vacio\n");
	const char path[100] = "se_lee.txt";
	int new_file = creat(path, S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO);
	char buf_leer[10];
	char buf_escribir[30] = "hola ahora ando muy bien";
	int escritos = write(new_file, buf_escribir, strlen(buf_escribir));
	close(new_file);
	int fd = open(path, O_RDWR);
	int leidos = read(fd, buf_leer, 9);
	assert(escritos == strlen(buf_escribir));
	assert(leidos == 9);
	close(fd);
	printf("fin prueba \n---\n");
}

int
main()
{
	writing_in_file();
	reading_in_file_with_nothing_in_it();
	read_in_file_with_something_in_it();
	return 0;
}