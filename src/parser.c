#include "shell.h"
#include "parser.h"
#include <ctype.h>

static char *trim_whitespace(char *str);
static void parse_simple_command(SimpleCommand *cmd, char *cmd_str);
static char **tokenize(char *str, int *count);

Pipeline parse_command(const char *cmd_line)
{
    Pipeline pipeline;
    memset(&pipeline, 0, sizeof(Pipeline));

    char *line = strdup(cmd_line);
    line = trim_whitespace(line);

    // 检测末尾 '&'
    int len = strlen(line);
    if (len > 0 && line[len - 1] == '&')
    {
        pipeline.background = 1;
        line[len - 1] = '\0';
        line = trim_whitespace(line);
    }

    // 按 '|' 分割命令
    char *saveptr;
    char *token = strtok_r(line, "|", &saveptr);

    while (token != NULL && pipeline.num_commands < MAX_PIPES)
    {
        token = trim_whitespace(token);
        parse_simple_command(&pipeline.commands[pipeline.num_commands], token);
        pipeline.num_commands++;
        token = strtok_r(NULL, "|", &saveptr);
    }

    free(line);
    return pipeline;
}

static void parse_simple_command(SimpleCommand *cmd, char *cmd_str)
{
    memset(cmd, 0, sizeof(SimpleCommand));

    char *str = strdup(cmd_str);
    char *ptr = str;

    // 处理重定向
    char *input_redir = strstr(ptr, "<");
    char *output_redir = strstr(ptr, ">>");
    char *output_single = NULL;

    if (output_redir == NULL)
    {
        output_single = strstr(ptr, ">");
    }

    // 提取输入重定向
    if (input_redir != NULL)
    {
        *input_redir = '\0';
        char *filename = input_redir + 1;
        filename = trim_whitespace(filename);
        // 如果还有其他重定向，截断
        char *end = strchr(filename, '>');
        if (end)
            *end = '\0';
        cmd->input_file = strdup(trim_whitespace(filename));
    }

    // 提取输出重定向
    if (output_redir != NULL)
    {
        *output_redir = '\0';
        char *filename = output_redir + 2;
        filename = trim_whitespace(filename);
        char *end = strchr(filename, '<');
        if (end)
            *end = '\0';
        cmd->output_file = strdup(trim_whitespace(filename));
        cmd->append = 1;
    }
    else if (output_single != NULL)
    {
        *output_single = '\0';
        char *filename = output_single + 1;
        filename = trim_whitespace(filename);
        char *end = strchr(filename, '<');
        if (end)
            *end = '\0';
        cmd->output_file = strdup(trim_whitespace(filename));
        cmd->append = 0;
    }

    // 分词
    ptr = trim_whitespace(ptr);
    char **tokens = tokenize(ptr, &cmd->argc);
    for (int i = 0; i < cmd->argc && i < MAX_ARGS - 1; i++)
    {
        cmd->args[i] = tokens[i];
    }
    cmd->args[cmd->argc] = NULL;

    free(tokens);
    free(str);
}

static char **tokenize(char *str, int *count)
{
    char **tokens = malloc(sizeof(char *) * MAX_ARGS);
    *count = 0;

    char *ptr = str;
    while (*ptr && *count < MAX_ARGS - 1)
    {
        // 跳过空格
        while (*ptr && isspace(*ptr))
            ptr++;
        if (!*ptr)
            break;

        // 处理引号
        if (*ptr == '"' || *ptr == '\'')
        {
            char quote = *ptr++;
            char *start = ptr;
            while (*ptr && *ptr != quote)
                ptr++;
            int len = ptr - start;
            tokens[*count] = malloc(len + 1);
            strncpy(tokens[*count], start, len);
            tokens[*count][len] = '\0';
            if (*ptr)
                ptr++; // 跳过结束引号
        }
        else
        {
            char *start = ptr;
            while (*ptr && !isspace(*ptr))
                ptr++;
            int len = ptr - start;
            tokens[*count] = malloc(len + 1);
            strncpy(tokens[*count], start, len);
            tokens[*count][len] = '\0';
        }
        (*count)++;
    }

    return tokens;
}

static char *trim_whitespace(char *str)
{
    while (isspace(*str))
        str++;
    if (*str == '\0')
        return str;

    char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end))
        end--;
    *(end + 1) = '\0';

    return str;
}

void free_pipeline(Pipeline *pipeline)
{
    for (int i = 0; i < pipeline->num_commands; i++)
    {
        SimpleCommand *cmd = &pipeline->commands[i];
        for (int j = 0; j < cmd->argc; j++)
        {
            free(cmd->args[j]);
        }
        if (cmd->input_file)
            free(cmd->input_file);
        if (cmd->output_file)
            free(cmd->output_file);
    }
}
