#include "shell.h"
#include "parser.h"
#include "executor.h"
#include "builtins.h"
#include "history.h"
#include "alias.h"
#include "completion.h"
#include "job.h"
#include <readline/readline.h>
#include <readline/history.h>

int main(int argc, char *argv[])
{
    // 初始化：历史记录、别名表、作业控制、readline 补全
    init_history();
    init_alias();
    init_jobs();
    init_completion();
    setup_signals();

    while (1)
    {
        // 清理已完成的作业
        clean_done_jobs();

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

        // 6. 解析复合命令（支持 ;、&&、||）
        CompoundCommand compound = parse_compound(cmd_line);

        // 7. 执行复合命令
        if (compound.num_pipelines > 0)
        {
            execute_compound(&compound);
        }

        free(cmd_line);
        free_compound(&compound);
    }

    cleanup();
    return 0;
}

void cleanup(void)
{
    free_history();
    free_alias();
    free_jobs();
}
