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
#include<android/log.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#define BUFFER_SIZE 3
#define LOG_TAG "inject"
#define LOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##args)
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args)

char *sos[] = { "linker"
		"libdvm.so", "libnativehelper.so", "libandroid_runtime.so",
		"libmath.so", "test", "libc.so", NULL };

void call_shit(struct elf_info *einfo) {
	unsigned long addr2 = 0;
	unsigned long rel_addr = find_sym_in_rel(einfo, "math_shit");
	regs_t regs;
	ptrace_read(einfo->pid, rel_addr, &addr2, sizeof(long));
	printf("math_shit rel addr\t %lx\n", rel_addr);
	printf("addr2 is \t %lx\n", addr2);
	ptrace_readreg(einfo->pid, &regs);
	ptrace_dump_regs(&regs, "before call to call_shit\n");
#ifdef THUMB
	regs.ARM_lr = 1;
#else
	regs.ARM_lr = 0;
#endif
	regs.ARM_r0 = 5;
	regs.ARM_r1 = 6;
	regs.ARM_r2 = 7;
	regs.ARM_r3 = 8;
	{
		int a5 = 9;
		ptrace_push(einfo->pid, &regs, &a5, 4);
		ptrace_push(einfo->pid, &regs, &a5, 4);
		ptrace_push(einfo->pid, &regs, &a5, 4);
		ptrace_push(einfo->pid, &regs, &a5, 4);
		ptrace_push(einfo->pid, &regs, &a5, 4);
		ptrace_push(einfo->pid, &regs, &a5, 4);
		ptrace_push(einfo->pid, &regs, &a5, 4);
		ptrace_push(einfo->pid, &regs, &a5, 4);
		ptrace_push(einfo->pid, &regs, &a5, 4);
		a5 = 10;
		ptrace_push(einfo->pid, &regs, &a5, 4);
	}
	regs.ARM_pc = addr2;
	ptrace_writereg(einfo->pid, &regs);
	ptrace_cont(einfo->pid);
	printf("done %d\n", ptrace_wait_for_signal(einfo->pid, SIGSEGV));
	ptrace_readreg(einfo->pid, &regs);
	ptrace_dump_regs(&regs, "before return call_shit\n");
}

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

char * str_contact(const char *str1,const char *str2)
{
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
	if ((from_fd = open(from, O_RDONLY)) == -1) /*open file readonly,返回-1表示出错，否则返回文件描述符*/
	{
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
	copy(str_contact(argv[1], "libhook.so"), "/data/system/inject/libhook.so");
	copy(str_contact(argv[1], "hook.dex"), "/data/system/inject/hook.dex");

	pid = find_pid_of("system_server");
	ptrace_attach(pid);

	ptrace_find_dlinfo(pid);

	handle = ptrace_dlopen(pid, "/data/system/inject/libhook.so", 1);
	printf("ptrace_dlopen handle %p\n", handle);
	proc = (long) ptrace_dlsym(pid, handle, "main");
	printf("main = %lx\n", proc);
	//replace_all_rels(pid, "connect", proc, sos);
	ptrace_call(pid, proc, 0, (ptrace_arg*) NULL);
	ptrace_detach(pid);
	exit(0);

}
