#ifndef UTILS
#define UTILS

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>

// ---------------- String ---------------------

// change to upper case
void strupp(char *beg);

int startswith(const char *str, char *pattern);
int endswith(const char *str, char *pattern);
int startswith_char(const char *str, char c);
int endswith_char(const char *str, char c);

// append path (target) to the end of path (source)
int push_path(char *source, const char *target, int is_dir);

// ---------------- LIST -----------------------

void show_file_info(char *output, char *filename, char *basename, struct stat *info_p);
void mode_to_letters(int mode, char str[]);
char *uid_to_name(uid_t uid);
char *gid_to_name(gid_t gid);

// ---------------- Network --------------------

void GetPrimaryIp(char *buffer, size_t buflen);

#endif