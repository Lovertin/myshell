#include "shell.h"
#include "executor.h"
#include "builtins.h"

static void execute_pipeline_internal(Pipeline *pipeline);

void execute_pipeline(Pipeline *pipeline)
{
    if (pipeline->num_commands == 0)
        return;

    // 单条命令且为内建命令
    if (pipeline->num_commands == 1 &&
        is_builtin(pipeline->commands[0].args[0]))
    {
        execute_builtin(&pipeline->commands[0]);
        return;
    }

    // 单条外部命令或管道
    if (pipeline->num_commands == 1)
    {
        execute_single(&pipeline->commands[0], pipeline->background);
    }
    else
    {
        execute_pipeline_internal(pipeline);
    }
}

void execute_single(SimpleCommand *cmd, int background)
{
    if (cmd->argc == 0)
        return;

    pid_t pid = fork();
    if (pid == 0)
    {
        // 子进程：恢复信号处理
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

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

        // 执行
        execvp(cmd->args[0], cmd->args);
        fprintf(stderr, "%s: command not found\n", cmd->args[0]);
        exit(127);
    }
    else if (pid > 0)
    {
        // 父进程
        if (!background)
        {
            int status;
            waitpid(pid, &status, 0);
        }
        else
        {
            printf("[bg] %d\n", pid);
        }
    }
    else
    {
        perror("fork");
    }
}

static void execute_pipeline_internal(Pipeline *pipeline)
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
            return;
        }
    }

    // 创建子进程
    for (int i = 0; i < num_commands; i++)
    {
        pids[i] = fork();
        if (pids[i] == 0)
        {
            // 子进程
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);

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

            // 关闭所有管道 fd
            for (int j = 0; j < num_commands - 1; j++)
            {
                close(pipefds[j][0]);
                close(pipefds[j][1]);
            }

            execvp(cmd->args[0], cmd->args);
            fprintf(stderr, "%s: command not found\n", cmd->args[0]);
            exit(127);
        }
        else if (pids[i] < 0)
        {
            perror("fork");
            return;
        }
    }

    // 父进程关闭所有管道
    for (int i = 0; i < num_commands - 1; i++)
    {
        close(pipefds[i][0]);
        close(pipefds[i][1]);
    }

    // 等待子进程
    if (!pipeline->background)
    {
        for (int i = 0; i < num_commands; i++)
        {
            waitpid(pids[i], NULL, 0);
        }
    }
    else
    {
        printf("[bg] started with %d processes\n", num_commands);
    }
}
