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

/* 在字符串中查找 '2>' 或 '2>>' 操作符 */
static char *find_stderr_redir(char *str, int *is_append)
{
    char *p = str;
    while (*p)
    {
        // 检查 2>&1 (stderr 合并到 stdout)
        if (*p == '2' && *(p + 1) == '>' && *(p + 2) == '&' && *(p + 3) == '1')
        {
            *is_append = 0;
            return p; // 返回位置，由调用者判断具体类型
        }
        // 检查 2>> (stderr 追加)
        if (*p == '2' && *(p + 1) == '>' && *(p + 2) == '>')
        {
            *is_append = 1;
            return p;
        }
        // 检查 2> (stderr 覆盖)
        if (*p == '2' && *(p + 1) == '>' && *(p + 2) != '>' && !(*(p + 2) == '&' && *(p + 3) == '1'))
        {
            *is_append = 0;
            return p;
        }
        p++;
    }
    return NULL;
}

/* 在字符串中查找 '&>' 或 '&>>' 操作符 */
static char *find_both_redir(char *str, int *is_append)
{
    char *p = str;
    while (*p)
    {
        if (*p == '&' && *(p + 1) == '>' && *(p + 2) == '>')
        {
            *is_append = 1;
            return p;
        }
        if (*p == '&' && *(p + 1) == '>')
        {
            *is_append = 0;
            return p;
        }
        p++;
    }
    return NULL;
}

static void parse_simple_command(SimpleCommand *cmd, char *cmd_str)
{
    memset(cmd, 0, sizeof(SimpleCommand));

    char *str = strdup(cmd_str);
    char *ptr = str;

    // 1. 处理 &> 或 &>>（同时重定向 stdout 和 stderr，优先级最高）
    int both_append = 0;
    char *both_redir = find_both_redir(ptr, &both_append);
    if (both_redir != NULL)
    {
        *both_redir = '\0';
        char *filename = both_redir + (both_append ? 3 : 2); // 跳过 &>> 或 &>
        filename = trim_whitespace(filename);
        // 截断后续重定向
        char *end = strchr(filename, '<');
        if (end)
            *end = '\0';
        end = strchr(filename, '>');
        if (end)
            *end = '\0';
        cmd->both_file = strdup(trim_whitespace(filename));
        cmd->both_append = both_append;
    }

    // 2. 处理 2>&1（stderr 合并到 stdout）
    char *stderr_to_out = strstr(ptr, "2>&1");
    if (stderr_to_out != NULL && cmd->both_file == NULL)
    {
        *stderr_to_out = '\0';
        cmd->stderr_to_stdout = 1;
        // 去除后面的空格
        char *rest = stderr_to_out + 4;
        // 如果后面还有东西，保留
        if (*rest)
        {
            // 将剩余部分重新连接到 ptr
            // 但不需要，因为后续还会处理
        }
    }

    // 3. 处理 2> 或 2>>（错误重定向）
    int error_append = 0;
    char *error_redir = find_stderr_redir(ptr, &error_append);
    if (error_redir != NULL)
    {
        // 检查是否为 2>&1（已处理）
        if (strncmp(error_redir, "2>&1", 4) == 0)
        {
            // 已处理
        }
        else
        {
            *error_redir = '\0';
            int is_append_flag = error_append;
            char *filename = error_redir + (is_append_flag ? 3 : 2); // 跳过 2>> 或 2>
            filename = trim_whitespace(filename);
            char *end = strchr(filename, '<');
            if (end)
                *end = '\0';
            end = strchr(filename, '>');
            if (end)
                *end = '\0';
            cmd->error_file = strdup(trim_whitespace(filename));
            cmd->error_append = is_append_flag;
        }
    }

    // 4. 重新处理 2>&1 (因为 strstr 可能被之前的修改影响)
    if (cmd->error_file == NULL && cmd->both_file == NULL)
    {
        char *stderr_to_out2 = strstr(ptr, "2>&1");
        if (stderr_to_out2)
        {
            *stderr_to_out2 = '\0';
            cmd->stderr_to_stdout = 1;
        }
    }

    // 5. 处理 <（输入重定向）
    char *input_redir = strstr(ptr, "<");
    if (input_redir != NULL && cmd->both_file == NULL)
    {
        *input_redir = '\0';
        char *filename = input_redir + 1;
        filename = trim_whitespace(filename);
        char *end = strchr(filename, '>');
        if (end)
            *end = '\0';
        cmd->input_file = strdup(trim_whitespace(filename));
    }

    // 6. 处理 > 或 >>（输出重定向）
    char *output_redir = strstr(ptr, ">>");
    char *output_single = NULL;
    if (output_redir == NULL)
    {
        output_single = strstr(ptr, ">");
    }

    if (cmd->both_file == NULL)
    {
        if (output_redir != NULL)
        {
            // 确保不是 2>> 的一部分
            if (output_redir == ptr || *(output_redir - 1) != '2')
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
        }
        else if (output_single != NULL)
        {
            // 确保不是 2> 或 &> 的一部分
            if ((output_single == ptr || (*(output_single - 1) != '2' && *(output_single - 1) != '&')))
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
        }
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
        if (cmd->error_file)
            free(cmd->error_file);
        if (cmd->both_file)
            free(cmd->both_file);
    }
}

/*
 * 复合命令解析
 *
 * 在字符串中查找下一个复合操作符（;、&&、||），
 * 同时正确识别引号内的内容，避免误判。
 * 返回找到的操作符位置，或 NULL 如果没有找到。
 */
static char *find_next_operator(char *str, CompoundOp *op)
{
    char *p = str;
    int in_single_quote = 0;
    int in_double_quote = 0;

    while (*p)
    {
        // 跟踪引号状态
        if (*p == '\'' && !in_double_quote)
        {
            in_single_quote = !in_single_quote;
            p++;
            continue;
        }
        if (*p == '"' && !in_single_quote)
        {
            in_double_quote = !in_double_quote;
            p++;
            continue;
        }

        // 在引号内，不解析操作符
        if (in_single_quote || in_double_quote)
        {
            p++;
            continue;
        }

        // 检查 ;;
        if (*p == ';' && *(p + 1) == ';')
        {
            // 双分号不是我们支持的操作符，跳过
            p += 2;
            continue;
        }

        // 检查 ||
        if (*p == '|' && *(p + 1) == '|')
        {
            *op = OP_OR;
            return p;
        }

        // 检查 &&
        if (*p == '&' && *(p + 1) == '&')
        {
            *op = OP_AND;
            return p;
        }

        // 检查 ;
        if (*p == ';')
        {
            *op = OP_SEQ;
            return p;
        }

        p++;
    }

    return NULL;
}

CompoundCommand parse_compound(const char *cmd_line)
{
    CompoundCommand compound;
    memset(&compound, 0, sizeof(CompoundCommand));

    // 复制一份用于修改
    char *line = strdup(cmd_line);
    char *ptr = line;

    // 第一个操作符是 OP_NONE
    compound.operators[0] = OP_NONE;

    while (ptr && *ptr)
    {
        if (compound.num_pipelines >= MAX_COMPOUND)
            break;

        ptr = trim_whitespace(ptr);
        if (*ptr == '\0')
            break;

        // 查找下一个操作符
        CompoundOp op = OP_NONE;
        char *op_pos = find_next_operator(ptr, &op);

        char *segment_str;
        if (op_pos)
        {
            // 截断字符串，提取当前段
            *op_pos = '\0';
            segment_str = strdup(ptr);
            ptr = op_pos + (op == OP_SEQ ? 1 : 2); // 跳过操作符字符
        }
        else
        {
            // 最后一段
            segment_str = strdup(ptr);
            ptr = NULL;
        }

        // 解析该段为 Pipeline
        segment_str = trim_whitespace(segment_str);
        if (strlen(segment_str) > 0)
        {
            compound.pipelines[compound.num_pipelines] = parse_command(segment_str);
            if (op_pos && compound.num_pipelines > 0)
            {
                compound.operators[compound.num_pipelines] = op;
            }
            compound.num_pipelines++;
        }

        free(segment_str);

        // 如果还有后续操作符，记录它
        if (op_pos && op != OP_NONE && compound.num_pipelines < MAX_COMPOUND)
        {
            compound.operators[compound.num_pipelines] = op;
        }
    }

    free(line);
    return compound;
}

void free_compound(CompoundCommand *compound)
{
    for (int i = 0; i < compound->num_pipelines; i++)
    {
        free_pipeline(&compound->pipelines[i]);
    }
}