#include "utils.h"
#include "commands.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <unistd.h>

#include <assert.h>
#include <string.h>
#include <ctype.h>

// ---------------- String --------------------

// change to upper case
void strupp(char *beg)
{
    while (*beg)
        *beg = toupper(*beg), beg++;
}

int startswith(const char *str, char *pattern)
{
    int n = strlen(str), m = strlen(pattern);
    if (n < m)
        return 0;
    for (int i = 0; i < m; i++)
        if (str[i] != pattern[i])
            return 0;
    return 1;
}

int endswith(const char *str, char *pattern)
{
    int n = strlen(str), m = strlen(pattern);
    if (n < m)
        return 0;
    for (int i = 0; i < m; i++)
        if (str[i] != pattern[n - m + i])
            return 0;
    return 1;
}

int startswith_char(const char *str, char c)
{
    return str[0] == c;
}
int endswith_char(const char *str, char c)
{
    return str[strlen(str) - 1] == c;
}

// back to parent dir, assume not end with '/'
static int pop_path(char *source)
{
    // root: fail
    if (strlen(source) == 1 && source[0] == '/')
        return 0;
    int p = strlen(source) - 1;
    if (source[p] == '/') // end with '/'
        source[p--] = 0;
    while (source[p] != '/')
        source[p--] = 0;
    return 1;
}

// append path (target) to the end of path (source)
int push_path(char *source, const char *target, int is_dir)
{
    // absolute path
    if (startswith_char(target, '/'))
    {
        strcpy(source, target);
        return 1;
    }

    char tmp[BUFFER_SIZE];
    strcpy(tmp, source);
    int tmp_ptr = strlen(tmp);
    if (!endswith_char(tmp, '/'))
        tmp[tmp_ptr++] = '/';

    char buffer[BUFFER_SIZE];
    int buffer_ptr = 0;
    int m = strlen(target);
    for (int i = 0; i <= m; i++)
    {
        if (target[i] == '/' || (i == m && target[i - 1] != '/'))
        {
            buffer[buffer_ptr] = 0;
            if (!strcmp(buffer, "."))
            {
                // do nothing
            }
            else if (!strcmp(buffer, ".."))
            {
                // back to parent dir
                if (!pop_path(tmp))
                    return 0;
            }
            else
            {
                for (int j = 0; j < buffer_ptr; j++)
                    tmp[tmp_ptr++] = buffer[j];
            }

            if (i < m && target[i] == '/')
                tmp[tmp_ptr++] = '/';
            buffer_ptr = 0;
        }
        else
        {
            buffer[buffer_ptr++] = target[i];
        }
    }

    if (is_dir && tmp[tmp_ptr - 1] != '/')
        tmp[tmp_ptr++] = '/';

    tmp[tmp_ptr++] = '\0';
    strcpy(source, tmp);
    return 1;
}

// ---------------- LIST --------------------

// Reference: [linux 下用 c 实现 ls -l 命令 - Ritchie丶 -
// 博客园](https://www.cnblogs.com/Ritchie/p/6272693.html)

void show_file_info(char *output, char *filename, char *basename,
                    struct stat *info_p)
{
    // if (filename[0] != '.')
    {
        char *uid_to_name(), *ctime(), *gid_to_name(), *filemode();
        void mode_to_letters();

        char modestr[11];

        mode_to_letters(info_p->st_mode, modestr); /*模式到字符的转换*/

        sprintf(output, "%s %4d %-8s %-8s %8ld %.12s %s\r\n", modestr,
                (int)info_p->st_nlink, uid_to_name(info_p->st_uid),
                gid_to_name(info_p->st_gid), (long)info_p->st_size,
                4 + ctime(&info_p->st_mtime), basename);
    }
}

void mode_to_letters(int mode, char str[])
{
    strcpy(str, "----------");

    if (S_ISDIR(mode))
        str[0] = 'd'; /*目录*/
    if (S_ISCHR(mode))
        str[0] = 'c'; /*字符文件*/
    if (S_ISBLK(mode))
        str[0] = 'b'; /*块文件*/

    if (mode & S_IRUSR)
        str[1] = 'r';
    if (mode & S_IWUSR)
        str[2] = 'w';
    if (mode & S_IXUSR)
        str[3] = 'x';

    if (mode & S_IRGRP)
        str[4] = 'r';
    if (mode & S_IWGRP)
        str[5] = 'w';
    if (mode & S_IXGRP)
        str[6] = 'x';

    if (mode & S_IROTH)
        str[7] = 'r';
    if (mode & S_IWOTH)
        str[8] = 'w';
    if (mode & S_IXOTH)
        str[9] = 'x';
}

char *uid_to_name(uid_t uid)
/*
 *返回和 uid 相应的用户名的指针
 */
{
    struct passwd *getpwuid(), *pw_ptr;
    static char numstr[10];

    if ((pw_ptr = getpwuid(uid)) == NULL)
    {
        sprintf(numstr, "%d", uid); /*没有对应的用户名则 uid 存入
                                       numstr,返回后以字符串的形式打印 uid*/
        return numstr;
    }
    else
        return pw_ptr->pw_name; /*打印用户名*/
}

char *gid_to_name(gid_t gid)
{
    struct group *getgrgid(), *grp_ptr;
    static char numstr[10];

    if ((grp_ptr = getgrgid(gid)) == NULL)
    {
        sprintf(numstr, "%d", gid);
        return numstr;
    }
    else
        return grp_ptr->gr_name;
}

// ---------------- Network --------------------

// Reference: [c++ - Get the IP address of the machine - Stack Overflow](https://stackoverflow.com/questions/212528/get-the-ip-address-of-the-machine)
void GetPrimaryIp(char *buffer, size_t buflen)
{
    assert(buflen >= 16);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    assert(sock != -1);

    const char *kGoogleDnsIp = "8.8.8.8";
    uint16_t kDnsPort = 53;
    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(kGoogleDnsIp);
    serv.sin_port = htons(kDnsPort);

    int err = connect(sock, (const struct sockaddr *)&serv, sizeof(serv));
    assert(err != -1);

    struct sockaddr_in name;
    socklen_t namelen = sizeof(name);
    err = getsockname(sock, (struct sockaddr *)&name, &namelen);
    assert(err != -1);

    const char *p = inet_ntop(AF_INET, &name.sin_addr, buffer, buflen);
    assert(p);

    close(sock);
}