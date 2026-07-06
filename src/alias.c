#include "shell.h"
#include "alias.h"

static AliasList alias_list;

void init_alias(void)
{
    memset(&alias_list, 0, sizeof(AliasList));
}

void set_alias(const char *name, const char *value)
{
    // 检查是否已存在
    for (int i = 0; i < alias_list.count; i++)
    {
        if (strcmp(alias_list.entries[i].name, name) == 0)
        {
            // 更新现有别名
            free(alias_list.entries[i].value);
            alias_list.entries[i].value = strdup(value);
            return;
        }
    }

    // 添加新别名
    if (alias_list.count < MAX_ALIAS)
    {
        alias_list.entries[alias_list.count].name = strdup(name);
        alias_list.entries[alias_list.count].value = strdup(value);
        alias_list.count++;
    }
    else
    {
        fprintf(stderr, "alias: too many aliases\n");
    }
}

void remove_alias(const char *name)
{
    for (int i = 0; i < alias_list.count; i++)
    {
        if (strcmp(alias_list.entries[i].name, name) == 0)
        {
            free(alias_list.entries[i].name);
            free(alias_list.entries[i].value);

            // 移动后续元素
            for (int j = i; j < alias_list.count - 1; j++)
            {
                alias_list.entries[j] = alias_list.entries[j + 1];
            }
            alias_list.count--;
            return;
        }
    }

    fprintf(stderr, "unalias: %s: not found\n", name);
}

char *expand_alias(const char *cmd)
{
    // 提取第一个词
    char *cmd_copy = strdup(cmd);
    char *first_word = strtok(cmd_copy, " \t");

    if (first_word == NULL)
    {
        free(cmd_copy);
        return strdup(cmd);
    }

    // 查找别名
    for (int i = 0; i < alias_list.count; i++)
    {
        if (strcmp(alias_list.entries[i].name, first_word) == 0)
        {
            // 找到别名，进行替换
            const char *rest = cmd + strlen(first_word);
            while (*rest == ' ' || *rest == '\t')
                rest++;

            char *expanded = malloc(strlen(alias_list.entries[i].value) + strlen(rest) + 2);
            sprintf(expanded, "%s %s", alias_list.entries[i].value, rest);

            free(cmd_copy);
            return expanded;
        }
    }

    free(cmd_copy);
    return strdup(cmd);
}

void print_aliases(void)
{
    for (int i = 0; i < alias_list.count; i++)
    {
        printf("alias %s='%s'\n",
               alias_list.entries[i].name,
               alias_list.entries[i].value);
    }
}

void free_alias(void)
{
    for (int i = 0; i < alias_list.count; i++)
    {
        free(alias_list.entries[i].name);
        free(alias_list.entries[i].value);
    }
}
