#ifndef UTILS
#define UTILS

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>

void strupp(char *beg);
int startswith(const char *str, char *pattern);
int endswith(const char *str, char *pattern);
int startswith_char(const char *str, char c);
int endswith_char(const char *str, char c);

int push_path(char *source, const char *target, int is_dir);

void show_file_info(char *output, char *filename, char *basename, struct stat *info_p);
void mode_to_letters(int mode, char str[]);
char *uid_to_name(uid_t uid);
char *gid_to_name(gid_t gid);

void GetPrimaryIp(char *buffer, size_t buflen);

#endif