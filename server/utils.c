#include "const.h"
#include <string.h>

static int startswith(const char *str, char *pattern)
{
    return 1;
}
static int endswith(const char *str, char *pattern)
{
    return 1;
}

static int startswith_char(const char *str, char c)
{
    return str[0] == c;
}
static int endswith_char(const char *str, char c)
{
    return str[strlen(str) - 1] == c;
}

int pop_path(char *source)
{
    if (strlen(source) == 1 && source[0] == '/')
        return 0;
    int p = strlen(source) - 1;
    while (source[p] != '/')
        source[p--] = 0;
    return 1;
}

int push_path(char *source, const char *target, int is_dir)
{
    if (startswith_char(target, '/'))
    {
        strcpy(source, target);
        return 1;
    }

    char tmp[BUFFER_SIZE];
    strcpy(tmp, source);
    int tmp_ptr = strlen(tmp);

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