#include "shell.h"
#include "completion.h"
#include "builtins.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>

static char *command_generator(const char *text, int state);
static char **shell_completion(const char *text, int start, int end);

void init_completion(void)
{
    // 注册自定义补全函数
    rl_attempted_completion_function = shell_completion;
}

static char **shell_completion(const char *text, int start, int end)
{
    (void)end;

    // 如果在行首，补全命令名
    if (start == 0)
    {
        return rl_completion_matches(text, command_generator);
    }

    // 否则使用默认的文件名补全
    return NULL;
}

static char *command_generator(const char *text, int state)
{
    static int list_index;
    static size_t len;
    static DIR *dir;
    static char *path_dirs[256];
    static int path_count;
    static int current_dir;

    // 内建命令列表
    static const char *builtins[] = {
        "cd", "exit", "echo", "type", "history", "alias", "unalias", NULL};

    // 初始化
    if (state == 0)
    {
        list_index = 0;
        len = strlen(text);
        current_dir = -1;
        path_count = 0;
        dir = NULL;

        // 解析 PATH 环境变量
        char *path_env = getenv("PATH");
        if (path_env)
        {
            char *path = strdup(path_env);
            char *saveptr;
            char *token = strtok_r(path, ":", &saveptr);

            while (token && path_count < 256)
            {
                path_dirs[path_count++] = strdup(token);
                token = strtok_r(NULL, ":", &saveptr);
            }
            free(path);
        }
    }

    // 首先匹配内建命令
    while (builtins[list_index])
    {
        const char *name = builtins[list_index++];
        if (strncmp(name, text, len) == 0)
        {
            return strdup(name);
        }
    }

    // 然后搜索 PATH 目录
    while (current_dir < path_count)
    {
        if (dir == NULL)
        {
            current_dir++;
            if (current_dir >= path_count)
                break;
            dir = opendir(path_dirs[current_dir]);
            if (dir == NULL)
                continue;
        }

        struct dirent *entry = readdir(dir);
        if (entry == NULL)
        {
            closedir(dir);
            dir = NULL;
            continue;
        }

        if (entry->d_name[0] == '.')
            continue;

        if (strncmp(entry->d_name, text, len) == 0)
        {
            // 检查是否可执行
            char fullpath[1024];
            snprintf(fullpath, sizeof(fullpath), "%s/%s",
                     path_dirs[current_dir], entry->d_name);

            if (access(fullpath, X_OK) == 0)
            {
                return strdup(entry->d_name);
            }
        }
    }

    // 清理
    if (state == 0 || current_dir >= path_count)
    {
        for (int i = 0; i < path_count; i++)
        {
            free(path_dirs[i]);
        }
        if (dir)
            closedir(dir);
    }

    return NULL;
}
