#include "shell.h"
#include "executor.h"
#include "builtins.h"
#include "job.h"

static int execute_pipeline_internal(Pipeline *pipeline, int background);

int execute_pipeline(Pipeline *pipeline)
{
    if (pipeline->num_commands == 0)
        return 0;

    // 单条命令且为内建命令
    if (pipeline->num_commands == 1 &&
        is_builtin(pipeline->commands[0].args[0]))
    {
        return execute_builtin(&pipeline->commands[0]);
    }

    // 后台执行
    if (pipeline->background)
    {
        // 单条外部命令后台
        if (pipeline->num_commands == 1)
        {
            return execute_single(&pipeline->commands[0], 1);
        }
        else
        {
            // 管道后台
            return execute_pipeline_internal(pipeline, 1);
        }
    }

    // 前台执行：单条外部命令或管道
    if (pipeline->num_commands == 1)
    {
        return execute_single(&pipeline->commands[0], 0);
    }
    else
    {
        return execute_pipeline_internal(pipeline, 0);
    }
}

int execute_single(SimpleCommand *cmd, int background)
{
    if (cmd->argc == 0)
        return 0;

    pid_t pid = fork();
    if (pid == 0)
    {
        // 子进程：设置新的进程组
        setpgid(0, 0);
        // 恢复信号处理
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);

        // 输入重定向
        if (cmd->input_file)
        {
            int fd = open(cmd->input_file, O_RDONLY);
            if (fd < 0)
            {
                perror(cmd->input_file);
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        // 输出重定向
        if (cmd->both_file)
        {
            // &> 或 &>>：同时重定向 stdout 和 stderr
            int flags = O_WRONLY | O_CREAT | (cmd->both_append ? O_APPEND : O_TRUNC);
            int fd = open(cmd->both_file, flags, 0644);
            if (fd < 0)
            {
                perror(cmd->both_file);
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }
        else
        {
            if (cmd->output_file)
            {
                int flags = O_WRONLY | O_CREAT | (cmd->append ? O_APPEND : O_TRUNC);
                int fd = open(cmd->output_file, flags, 0644);
                if (fd < 0)
                {
                    perror(cmd->output_file);
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            // 错误重定向
            if (cmd->stderr_to_stdout)
            {
                // 2>&1：stderr 合并到 stdout
                dup2(STDOUT_FILENO, STDERR_FILENO);
            }
            else if (cmd->error_file)
            {
                int flags = O_WRONLY | O_CREAT | (cmd->error_append ? O_APPEND : O_TRUNC);
                int fd = open(cmd->error_file, flags, 0644);
                if (fd < 0)
                {
                    perror(cmd->error_file);
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDERR_FILENO);
                close(fd);
            }
        }

        // 后台进程：将 stdin 重定向到 /dev/null，防止被终端信号停止
        if (background)
        {
            int fd = open("/dev/null", O_RDONLY);
            if (fd >= 0)
            {
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
        }

        // 执行（终端控制完全由父进程管理）
        execvp(cmd->args[0], cmd->args);
        fprintf(stderr, "%s: command not found\n", cmd->args[0]);
        exit(127);
    }
    else if (pid > 0)
    {
        // 父进程：设置子进程的进程组
        setpgid(pid, pid);

        pid_t pids[1] = {pid};
        add_job(pid, pids, 1, cmd->args[0], background);

        if (!background)
        {
            // 将子进程设为前台进程组
            tcsetpgrp(STDIN_FILENO, pid);

            int status;
            waitpid(pid, &status, WUNTRACED);

            // 将 shell 设回前台
            tcsetpgrp(STDIN_FILENO, getpgrp());

            update_job_status(pid, status);

            if (WIFEXITED(status))
            {
                return WEXITSTATUS(status);
            }
            return -1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        perror("fork");
        return -1;
    }
}

static int execute_pipeline_internal(Pipeline *pipeline, int background)
{
    int num_commands = pipeline->num_commands;
    int pipefds[MAX_PIPES - 1][2];
    pid_t pids[MAX_PIPES];

    // 创建管道
    for (int i = 0; i < num_commands - 1; i++)
    {
        if (pipe(pipefds[i]) < 0)
        {
            perror("pipe");
            return -1;
        }
    }

    // 创建子进程
    for (int i = 0; i < num_commands; i++)
    {
        pids[i] = fork();
        if (pids[i] == 0)
        {
            // 子进程：设置新的进程组
            setpgid(0, 0);
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);

            SimpleCommand *cmd = &pipeline->commands[i];

            // 输入端
            if (i > 0)
            {
                dup2(pipefds[i - 1][0], STDIN_FILENO);
            }
            if (cmd->input_file)
            {
                int fd = open(cmd->input_file, O_RDONLY);
                if (fd < 0)
                {
                    perror(cmd->input_file);
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }

            // 输出端
            if (i < num_commands - 1)
            {
                dup2(pipefds[i][1], STDOUT_FILENO);
            }
            if (cmd->both_file)
            {
                int flags = O_WRONLY | O_CREAT | (cmd->both_append ? O_APPEND : O_TRUNC);
                int fd = open(cmd->both_file, flags, 0644);
                if (fd < 0)
                {
                    perror(cmd->both_file);
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO);
                dup2(fd, STDERR_FILENO);
                close(fd);
            }
            else
            {
                if (cmd->output_file)
                {
                    int flags = O_WRONLY | O_CREAT | (cmd->append ? O_APPEND : O_TRUNC);
                    int fd = open(cmd->output_file, flags, 0644);
                    if (fd < 0)
                    {
                        perror(cmd->output_file);
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }

                // 错误重定向
                if (cmd->stderr_to_stdout)
                {
                    dup2(STDOUT_FILENO, STDERR_FILENO);
                }
                else if (cmd->error_file)
                {
                    int flags = O_WRONLY | O_CREAT | (cmd->error_append ? O_APPEND : O_TRUNC);
                    int fd = open(cmd->error_file, flags, 0644);
                    if (fd < 0)
                    {
                        perror(cmd->error_file);
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd, STDERR_FILENO);
                    close(fd);
                }
            }

            // 关闭所有管道 fd
            for (int j = 0; j < num_commands - 1; j++)
            {
                close(pipefds[j][0]);
                close(pipefds[j][1]);
            }

            // 如果是内建命令，在子进程中直接执行
            if (is_builtin(cmd->args[0]))
            {
                int ret = execute_builtin(cmd);
                exit(ret);
            }

            execvp(cmd->args[0], cmd->args);
            fprintf(stderr, "%s: command not found\n", cmd->args[0]);
            exit(127);
        }
        else if (pids[i] < 0)
        {
            perror("fork");
            return -1;
        }
        // 父进程：设置子进程的进程组
        setpgid(pids[i], pids[0]);
    }

    // 父进程关闭所有管道
    for (int i = 0; i < num_commands - 1; i++)
    {
        close(pipefds[i][0]);
        close(pipefds[i][1]);
    }

    pid_t pgid = pids[0];

    // 记录作业
    char cmd_str[MAX_CMD_LEN] = "";
    for (int i = 0; i < pipeline->commands[0].argc; i++)
    {
        strcat(cmd_str, pipeline->commands[0].args[i]);
        if (i < pipeline->commands[0].argc - 1)
            strcat(cmd_str, " ");
    }
    for (int i = 1; i < num_commands; i++)
    {
        strcat(cmd_str, " | ");
        for (int j = 0; j < pipeline->commands[i].argc; j++)
        {
            strcat(cmd_str, pipeline->commands[i].args[j]);
            if (j < pipeline->commands[i].argc - 1)
                strcat(cmd_str, " ");
        }
    }
    add_job(pgid, pids, num_commands, cmd_str, background);

    if (!background)
    {
        // 将第一个子进程设为前台进程组
        tcsetpgrp(STDIN_FILENO, pgid);

        // 等待所有子进程
        int last_status = 0;
        for (int i = 0; i < num_commands; i++)
        {
            int status;
            pid_t wpid = waitpid(pids[i], &status, WUNTRACED);
            if (wpid > 0)
            {
                update_job_status(wpid, status);
            }
            if (i == num_commands - 1)
            {
                if (WIFEXITED(status))
                {
                    last_status = WEXITSTATUS(status);
                }
                else
                {
                    last_status = -1;
                }
            }
        }

        // 将 shell 设回前台
        tcsetpgrp(STDIN_FILENO, getpgrp());

        return last_status;
    }
    else
    {
        return 0;
    }
}

int execute_compound(CompoundCommand *compound)
{
    if (compound->num_pipelines == 0)
        return 0;

    int last_status = 0;

    // 始终执行第一个 pipeline
    last_status = execute_pipeline(&compound->pipelines[0]);

    // 根据操作符决定是否执行后续 pipeline
    for (int i = 1; i < compound->num_pipelines; i++)
    {
        CompoundOp op = compound->operators[i];

        switch (op)
        {
        case OP_SEQ:
            // 顺序执行：不管前一个是否成功，都执行
            last_status = execute_pipeline(&compound->pipelines[i]);
            break;

        case OP_AND:
            // 逻辑与：前一个成功（返回 0）才执行
            if (last_status == 0)
            {
                last_status = execute_pipeline(&compound->pipelines[i]);
            }
            break;

        case OP_OR:
            // 逻辑或：前一个失败（返回非 0）才执行
            if (last_status != 0)
            {
                last_status = execute_pipeline(&compound->pipelines[i]);
            }
            break;

        default:
            // 不应发生
            break;
        }
    }

    return last_status;
}