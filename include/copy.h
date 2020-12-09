#pragma once


#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <dirent.h>

/* traverse the src_name directory, compare all files and subdirectories
   with the corresponding entries in dest_name directory, copies entries which do not
   exist in dest_name, indend is a number for printing subdirectories with identation */
void traverse(char* src_name, char* dest_name, int indent);

/* find a file or dir named name in directory dir, returns 1 on success and 0 if file
   does not exists */
int lookup(const char* name, const char* dir);

int check_dest_dir(char* src_name, char* dst_name);

/* copy src to dst, type specifies whether it is a regular file or a directory */
void copy(char* src, char* dst, int type);

/* create a backup directory dst_name or open if one already exists */
void init_dest_dir(const char* dst_name);

/* change modification time of a file/directory dest_name */
void change_time(char* dest_name);

/* init daemon process, monitoring src directory and backing it up into dst directory */
void init_daemon(char* src, char* dst);

/* kill daemon */
void daemon_stop();

/* print daemon logs in log_path directory (if used with rc interface program, then
   by default log_path is current working directory) */
void daemon_print(char* log_path);
    