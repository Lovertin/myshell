#include "shell.h"
#include "builtins.h"
#include "history.h"
#include "alias.h"
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
           strcmp(cmd_name, "unalias") == 0;
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
    (void)cmd;
    print_history();
    return 0;
}

static int builtin_alias(SimpleCommand *cmd)
{
    if (cmd->argc < 2)
    {
        // 打印所有别名
        print_aliases();
        return 0;
    }

    // 解析 name=value
    char *arg = cmd->args[1];
    char *eq = strchr(arg, '=');

    if (eq == NULL)
    {
        fprintf(stderr, "alias: invalid format, use: alias name=value\n");
        return -1;
    }

    *eq = '\0';
    char *name = arg;
    char *value = eq + 1;

    set_alias(name, value);
    return 0;
}

static int builtin_unalias(SimpleCommand *cmd)
{
    if (cmd->argc < 2)
    {
        fprintf(stderr, "unalias: missing argument\n");
        return -1;
    }

    remove_alias(cmd->args[1]);
    return 0;
}
