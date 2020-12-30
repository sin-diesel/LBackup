#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <sys/wait.h>
#include <sys/time.h>
#include <poll.h>

/* These are commands that can be sent to daemon in order to control its behaviour */
enum commands_t {
    STOP = 0,
    PRINT,
    SET,
};


/* Checks if the input is correct, returns 0 on succes, -1 if errors have been detected */
int check_args(int argc, char** argv);

/* Checks if the specified dst directory is in src directory, returns -1 if dst is inside 
   inside source and 0 otherwise  */
int check_dest_dir(char* src, char* dst);

/* check if src directory exists, returns -1 if directory does not exists and 0 otherwise */
int check_source_dir(char* src);

/* init daemon process, monitoring src directory and backing it up into dst directory
   links behaviour is responsible for handling symbolic links */
void init_daemon(char* src, char* dst, int links_behaviour);

/* traverses the src_name directory, compares all files and subdirectories
   with the corresponding entries in dest_name directory, copies entries which do not
   exist in dest_name, indent is a number describing the indent size used to print
   directories status in log file */
void traverse(char* src, char* dst, int indent);

/* finds a file or dir named name in directory dir, returns 1 on success and 0 if file
   does not exists */
int lookup(const char* name, const char* dir);


/* copy src to dst, type specifies whether it is a regular file or a directory */
void copy(char* src, char* dst, int type);

/* create a backup directory dst_name or open if one already exists */
void init_dest_dir(const char* dst);

/* change modification time of a file/directory dst */
void change_time(char* dst);

/* kill daemon */
void daemon_stop();

/* print daemon logs in log_path directory (if used with ./lbp-ui interface program, then
   by default log_path is current working directory) */
void daemon_print(char* log_path);

/* run the backup program */
void run_backup(char* src, char* dst);


extern int lnk_type; /* This is declare in lbp.c */
    