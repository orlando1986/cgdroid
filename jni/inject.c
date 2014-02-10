/*
 * inject.c
 *
 *  Created on: Jun 4, 2011
 *      Author: d
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include "utils.h"
#include <signal.h>
#include <sys/types.h>
#ifdef ANDROID
//#include <linker.h>
#endif
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <jni.h>
//#include <utils/Log.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#define BUFFER_SIZE 3

int find_pid_of(const char *process_name) {
	int id;
	pid_t pid = -1;
	DIR* dir;
	FILE *fp;
	char filename[32];
	char cmdline[256];

	struct dirent * entry;

	if (process_name == NULL)
		return -1;

	dir = opendir("/proc");
	if (dir == NULL)
		return -1;

	while ((entry = readdir(dir)) != NULL) {
		id = atoi(entry->d_name);
		if (id != 0) {
			sprintf(filename, "/proc/%d/cmdline", id);
			fp = fopen(filename, "r");
			if (fp) {
				fgets(cmdline, sizeof(cmdline), fp);
				fclose(fp);

				if (strcmp(process_name, cmdline) == 0) {
					/* process found */
					pid = id;
					break;
				}
			}
		}
	}
	closedir(dir);
	return pid;
}

char * str_contact(const char *str1,const char *str2) {
    char * result;
    result = (char*)malloc(strlen(str1) + strlen(str2) + 1); //str1的长度 + str2的长度 + \0;
    if(!result){ //如果内存动态分配失败
    LOGD("Error: malloc failed in concat! \n");
       exit(EXIT_FAILURE);
    }
    strcpy(result,str1);
    strcat(result,str2); //字符串拼接
    return result;
}

void copy(char* from, char* to) {
	int from_fd,to_fd;
	int bytes_read,bytes_write;
	char buffer[BUFFER_SIZE];
	char *ptr;
	if ((from_fd = open(from, O_RDONLY)) == -1) {/*open file readonly,返回-1表示出错，否则返回文件描述符*/
		LOGD("Open %s Error:%s\n", from, strerror(errno));
		exit(1);
	}

	/* 创建目的文件 */
	/* 使用了O_CREAT选项-创建文件,open()函数需要第3个参数,
	 mode=S_IRUSR|S_IWUSR表示S_IRUSR 用户可以读 S_IWUSR 用户可以写*/
	if ((to_fd = open(to, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) == -1) {
		LOGD("Open %s Error:%s\n", to, strerror(errno));
		exit(1);
	}

	/* 以下代码是一个经典的拷贝文件的代码 */
	while (bytes_read = read(from_fd, buffer, BUFFER_SIZE)) {
		/* 一个致命的错误发生了 */
		if ((bytes_read == -1) && (errno != EINTR))
			break;
		else if (bytes_read > 0) {
			ptr = buffer;
			while (bytes_write = write(to_fd, ptr, bytes_read)) {
				/* 一个致命错误发生了 */
				if ((bytes_write == -1) && (errno != EINTR))
					break;
				/* 写完了所有读的字节 */
				else if (bytes_write == bytes_read)
					break;
				/* 只写了一部分,继续写 */
				else if (bytes_write > 0) {
					ptr += bytes_write;
					bytes_read -= bytes_write;
				}
			}
			/* 写的时候发生的致命错误 */
			if (bytes_write == -1)
				break;
		}
	}
	close(from_fd);
	close(to_fd);
	system(str_contact("chmod 777 ", to));
}

int main(int argc, char *argv[]) {
	int pid;
	struct link_map *map;
	struct elf_info einfo;

	extern dl_fl_t ldl;

	void *handle = NULL;
	long proc = 0;
	long hooker_fopen = 0;
	(void) argc;

	LOGD("inject begin");
	copy(str_contact(argv[1], HOOK_LIB), str_contact(HOOK_PATH, HOOK_LIB));
	copy(str_contact(argv[1], HOOK_DEX), str_contact(HOOK_PATH, HOOK_DEX));
	copy(str_contact(argv[1], HOOK_ODEX), str_contact(ODEX_PATH, HOOK_ODEX));

	pid = find_pid_of("system_server");
	ptrace_attach(pid);

	ptrace_find_dlinfo(pid);

	handle = ptrace_dlopen(pid, str_contact(HOOK_PATH, HOOK_LIB), 1);
	printf("ptrace_dlopen handle %p\n", handle);
	proc = (long) ptrace_dlsym(pid, handle, "main");
	printf("main = %lx\n", proc);
	//replace_all_rels(pid, "connect", proc, sos);
	ptrace_call(pid, proc, 0, NULL);
	ptrace_detach(pid);
	LOGD("inject end");
	exit(0);
}
