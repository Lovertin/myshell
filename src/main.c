#include "shell.h"
#include "parser.h"
#include "executor.h"
#include "builtins.h"
#include "history.h"
#include "alias.h"
#include "completion.h"
#include <readline/readline.h>
#include <readline/history.h>

int main(int argc, char *argv[])
{
    // 初始化：历史记录、别名表、readline 补全
    init_history();
    init_alias();
    init_completion();
    setup_signals();

    while (1)
    {
        // 1. 通过 readline 读取输入（自带提示符和 Tab 补全支持）
        char *prompt = get_prompt();
        char *cmd_line = readline(prompt);
        free(prompt);

        if (cmd_line == NULL)
        {
            printf("\n");
            break; // EOF (Ctrl+D) 退出
        }

        // 2. 跳过空行
        if (strlen(cmd_line) == 0)
        {
            free(cmd_line);
            continue;
        }

        // 3. 添加到历史记录
        add_to_history(cmd_line);

        // 4. 处理 !n 历史命令调用
        char *expanded_cmd = expand_history(cmd_line);
        if (expanded_cmd == NULL)
        {
            free(cmd_line);
            continue;
        }
        free(cmd_line);
        cmd_line = expanded_cmd;

        // 5. 别名展开
        char *alias_expanded = expand_alias(cmd_line);
        free(cmd_line);
        cmd_line = alias_expanded;

        // 6. 解析命令行
        Pipeline pipeline = parse_command(cmd_line);

        // 7. 执行
        if (pipeline.num_commands > 0)
        {
            execute_pipeline(&pipeline);
        }

        free(cmd_line);
        free_pipeline(&pipeline);
    }

    cleanup();
    return 0;
}

void cleanup(void)
{
    free_history();
    free_alias();
}
