#include "shell.h"
#include "builtins.h"
#include "history.h"
#include "alias.h"
#include "job.h"
#include <sys/stat.h>

static int builtin_cd(SimpleCommand *cmd);
static int builtin_exit(SimpleCommand *cmd);
static int builtin_echo(SimpleCommand *cmd);
static int builtin_type(SimpleCommand *cmd);
static int builtin_history(SimpleCommand *cmd);
static int builtin_alias(SimpleCommand *cmd);
static int builtin_unalias(SimpleCommand *cmd);

int is_builtin(const char *cmd_name)
{
    if (cmd_name == NULL)
        return 0;
    return strcmp(cmd_name, "cd") == 0 ||
           strcmp(cmd_name, "exit") == 0 ||
           strcmp(cmd_name, "echo") == 0 ||
           strcmp(cmd_name, "type") == 0 ||
           strcmp(cmd_name, "history") == 0 ||
           strcmp(cmd_name, "alias") == 0 ||
           strcmp(cmd_name, "unalias") == 0 ||
           strcmp(cmd_name, "jobs") == 0 ||
           strcmp(cmd_name, "fg") == 0 ||
           strcmp(cmd_name, "bg") == 0;
}

int execute_builtin(SimpleCommand *cmd)
{
    if (cmd->argc == 0)
        return -1;

    const char *name = cmd->args[0];

    if (strcmp(name, "cd") == 0)
    {
        return builtin_cd(cmd);
    }
    else if (strcmp(name, "exit") == 0)
    {
        return builtin_exit(cmd);
    }
    else if (strcmp(name, "echo") == 0)
    {
        return builtin_echo(cmd);
    }
    else if (strcmp(name, "type") == 0)
    {
        return builtin_type(cmd);
    }
    else if (strcmp(name, "history") == 0)
    {
        return builtin_history(cmd);
    }
    else if (strcmp(name, "alias") == 0)
    {
        return builtin_alias(cmd);
    }
    else if (strcmp(name, "unalias") == 0)
    {
        return builtin_unalias(cmd);
    }
    else if (strcmp(name, "jobs") == 0)
    {
        return builtin_jobs(cmd);
    }
    else if (strcmp(name, "fg") == 0)
    {
        return builtin_fg(cmd);
    }
    else if (strcmp(name, "bg") == 0)
    {
        return builtin_bg(cmd);
    }

    return -1;
}

static int builtin_cd(SimpleCommand *cmd)
{
    const char *dir;

    if (cmd->argc < 2)
    {
        dir = getenv("HOME");
        if (dir == NULL)
        {
            fprintf(stderr, "cd: HOME not set\n");
            return -1;
        }
    }
    else
    {
        dir = cmd->args[1];
    }

    if (chdir(dir) != 0)
    {
        perror("cd");
        return -1;
    }

    return 0;
}

static int builtin_exit(SimpleCommand *cmd)
{
    (void)cmd;
    cleanup();
    exit(0);
}

static int builtin_echo(SimpleCommand *cmd)
{
    for (int i = 1; i < cmd->argc; i++)
    {
        char *arg = cmd->args[i];

        // 简单的环境变量展开
        if (arg[0] == '$')
        {
            char *value = getenv(arg + 1);
            if (value)
            {
                printf("%s", value);
            }
        }
        else
        {
            printf("%s", arg);
        }

        if (i < cmd->argc - 1)
        {
            printf(" ");
        }
    }
    printf("\n");
    return 0;
}

static int builtin_type(SimpleCommand *cmd)
{
    if (cmd->argc < 2)
    {
        fprintf(stderr, "type: missing argument\n");
        return -1;
    }

    const char *name = cmd->args[1];

    // 检查是否为内建命令
    if (is_builtin(name))
    {
        printf("%s is a shell builtin\n", name);
        return 0;
    }

    // 在 PATH 中查找
    char *path_env = getenv("PATH");
    if (path_env == NULL)
    {
        printf("%s: not found\n", name);
        return -1;
    }

    char *path = strdup(path_env);
    char *saveptr;
    char *dir = strtok_r(path, ":", &saveptr);

    while (dir != NULL)
    {
        char fullpath[MAX_CMD_LEN];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, name);

        struct stat st;
        if (stat(fullpath, &st) == 0 && (st.st_mode & S_IXUSR))
        {
            printf("%s is %s\n", name, fullpath);
            free(path);
            return 0;
        }

        dir = strtok_r(NULL, ":", &saveptr);
    }

    free(path);
    printf("%s: not found\n", name);
    return -1;
}

static int builtin_history(SimpleCommand *cmd)
{
    if (cmd->argc >= 2)
    {
        // history N：显示最近 N 条
        int n = atoi(cmd->args[1]);
        if (n > 0)
        {
            print_history_n(n);
            return 0;
        }
    }
    // 默认：显示全部
    print_history();
    return 0;
}

/* 去除字符串首尾的引号（" 和 '） */
static char *strip_quotes(char *str)
{
    if (str == NULL)
        return NULL;

    size_t len = strlen(str);

    // 去除开头和结尾的引号
    while (len > 0 && (str[0] == '"' || str[0] == '\''))
    {
        memmove(str, str + 1, len);
        len--;
    }
    while (len > 0 && (str[len - 1] == '"' || str[len - 1] == '\''))
    {
        str[len - 1] = '\0';
        len--;
    }

    return str;
}

static int builtin_alias(SimpleCommand *cmd)
{
    if (cmd->argc < 2)
    {
        // 打印所有别名
        print_aliases();
        return 0;
    }

    // 逐个处理参数
    for (int i = 1; i < cmd->argc; i++)
    {
        char *arg = cmd->args[i];
        char *eq = strchr(arg, '=');

        if (eq == NULL)
        {
            // alias name：显示指定别名的定义（去除引号后查找）
            char *name_stripped = strdup(arg);
            strip_quotes(name_stripped);
            print_alias(name_stripped);
            free(name_stripped);
        }
        else
        {
            // alias name=value：创建/更新别名
            *eq = '\0';
            char *name = arg;
            char *value = eq + 1;

            // 如果值后面还有参数（引号内的空格被tokenizer拆分），将它们拼接起来
            if (i + 1 < cmd->argc)
            {
                // 计算所需总长度
                size_t val_len = strlen(value);
                for (int j = i + 1; j < cmd->argc; j++)
                {
                    val_len += strlen(cmd->args[j]) + 1; // +1 空格
                }

                char *full_value = malloc(val_len + 1);
                strcpy(full_value, value);
                for (int j = i + 1; j < cmd->argc; j++)
                {
                    strcat(full_value, " ");
                    strcat(full_value, cmd->args[j]);
                }

                // 去除 name 和 value 中残留的引号
                strip_quotes(name);
                strip_quotes(full_value);

                set_alias(name, full_value);
                free(full_value);
                break; // 已处理完所有剩余参数
            }
            else
            {
                // 去除 name 和 value 中残留的引号
                strip_quotes(name);
                strip_quotes(value);

                set_alias(name, value);
            }
        }
    }
    return 0;
}

static int builtin_unalias(SimpleCommand *cmd)
{
    if (cmd->argc < 2)
    {
        fprintf(stderr, "unalias: missing argument\n");
        return -1;
    }

    // 支持 unalias name1 name2 name3（删除多个别名）
    for (int i = 1; i < cmd->argc; i++)
    {
        remove_alias(cmd->args[i]);
    }
    return 0;
}

int builtin_jobs(SimpleCommand *cmd)
{
    int show_pid = 0;
    int show_pgid = 0;
    int only_running = 0;
    int only_stopped = 0;

    // 解析选项
    for (int i = 1; i < cmd->argc; i++)
    {
        if (strcmp(cmd->args[i], "-l") == 0)
        {
            show_pid = 1;
        }
        else if (strcmp(cmd->args[i], "-p") == 0)
        {
            show_pgid = 1;
        }
        else if (strcmp(cmd->args[i], "-r") == 0)
        {
            only_running = 1;
        }
        else if (strcmp(cmd->args[i], "-s") == 0)
        {
            only_stopped = 1;
        }
    }

    print_jobs(show_pid, show_pgid, only_running, only_stopped);
    return 0;
}

int builtin_fg(SimpleCommand *cmd)
{
    Job *job = NULL;

    if (cmd->argc >= 2)
    {
        // fg %n 或 fg %prefix
        job = find_job_by_prefix(cmd->args[1]);
        if (job == NULL)
        {
            fprintf(stderr, "fg: %s: no such job\n", cmd->args[1]);
            return -1;
        }
    }
    else
    {
        // fg 无参数：使用最近的作业
        job = get_recent_job();
        if (job == NULL)
        {
            fprintf(stderr, "fg: no current job\n");
            return -1;
        }
    }

    printf("%s\n", job->command);
    return fg_job(job);
}

int builtin_bg(SimpleCommand *cmd)
{
    Job *job = NULL;

    if (cmd->argc >= 2)
    {
        // bg %n 或 bg %prefix
        job = find_job_by_prefix(cmd->args[1]);
        if (job == NULL)
        {
            fprintf(stderr, "bg: %s: no such job\n", cmd->args[1]);
            return -1;
        }
    }
    else
    {
        // bg 无参数：使用最近的作业
        job = get_recent_job();
        if (job == NULL)
        {
            fprintf(stderr, "bg: no current job\n");
            return -1;
        }
    }

    return bg_job(job);
}
