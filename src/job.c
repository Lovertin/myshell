#include "shell.h"
#include "job.h"
#include <sys/ioctl.h>
#include <termios.h>

static JobList job_list;

void init_jobs(void)
{
    memset(&job_list, 0, sizeof(JobList));
    job_list.next_id = 1;
}

int add_job(pid_t pgid, pid_t *pids, int num_pids, const char *command, int background)
{
    if (job_list.count >= MAX_JOBS)
    {
        fprintf(stderr, "jobs: too many jobs\n");
        return -1;
    }

    Job *job = &job_list.jobs[job_list.count];
    job->id = job_list.next_id++;
    job->pgid = pgid;
    job->pids = malloc(sizeof(pid_t) * num_pids);
    memcpy(job->pids, pids, sizeof(pid_t) * num_pids);
    job->num_pids = num_pids;
    job->command = strdup(command);
    job->status = JOB_RUNNING;
    job->notified = 0;

    job_list.count++;

    // 后台任务输出作业信息
    if (background)
    {
        printf("[%d] %d\n", job->id, pgid);
    }

    return job->id;
}

void update_job_status(pid_t pid, int status)
{
    for (int i = 0; i < job_list.count; i++)
    {
        Job *job = &job_list.jobs[i];
        for (int j = 0; j < job->num_pids; j++)
        {
            if (job->pids[j] == pid)
            {
                if (WIFEXITED(status) || WIFSIGNALED(status))
                {
                    // 检查是否所有进程都已退出
                    int all_done = 1;
                    for (int k = 0; k < job->num_pids; k++)
                    {
                        if (job->pids[k] != pid)
                        {
                            int wstatus;
                            pid_t ret = waitpid(job->pids[k], &wstatus, WNOHANG);
                            if (ret == 0)
                            {
                                all_done = 0;
                                break;
                            }
                        }
                    }
                    if (all_done)
                    {
                        if (WIFSIGNALED(status))
                        {
                            job->status = JOB_TERMINATED;
                        }
                        else
                        {
                            job->status = JOB_DONE;
                        }
                        job->notified = 0;
                    }
                }
                else if (WIFSTOPPED(status))
                {
                    job->status = JOB_STOPPED;
                    job->notified = 0;
                }
                else if (WIFCONTINUED(status))
                {
                    job->status = JOB_RUNNING;
                }
                return;
            }
        }
    }
}

Job *find_job_by_id(int id)
{
    for (int i = 0; i < job_list.count; i++)
    {
        if (job_list.jobs[i].id == id)
        {
            return &job_list.jobs[i];
        }
    }
    return NULL;
}

Job *find_job_by_pgid(pid_t pgid)
{
    for (int i = 0; i < job_list.count; i++)
    {
        if (job_list.jobs[i].pgid == pgid)
        {
            return &job_list.jobs[i];
        }
    }
    return NULL;
}

Job *find_job_by_prefix(const char *prefix)
{
    if (prefix == NULL)
        return NULL;

    // 跳过 % 前缀
    if (*prefix == '%')
        prefix++;

    // 如果是数字，按 ID 查找
    if (*prefix >= '0' && *prefix <= '9')
    {
        int id = atoi(prefix);
        return find_job_by_id(id);
    }

    // 按命令前缀查找
    for (int i = job_list.count - 1; i >= 0; i--)
    {
        if (strncmp(job_list.jobs[i].command, prefix, strlen(prefix)) == 0)
        {
            return &job_list.jobs[i];
        }
    }

    return NULL;
}

Job *get_recent_job(void)
{
    // 返回最近添加的未完成作业
    for (int i = job_list.count - 1; i >= 0; i--)
    {
        if (job_list.jobs[i].status == JOB_RUNNING ||
            job_list.jobs[i].status == JOB_STOPPED)
        {
            return &job_list.jobs[i];
        }
    }
    return NULL;
}

void print_jobs(int show_pid, int show_pgid, int only_running, int only_stopped)
{
    for (int i = 0; i < job_list.count; i++)
    {
        Job *job = &job_list.jobs[i];

        // 过滤
        if (only_running && job->status != JOB_RUNNING)
            continue;
        if (only_stopped && job->status != JOB_STOPPED)
            continue;

        // 跳过已完成的（除非明确要求显示）
        if (job->status == JOB_DONE || job->status == JOB_TERMINATED)
        {
            if (!only_running && !only_stopped)
            {
                // 默认显示已完成一次后不再显示
                if (job->notified)
                    continue;
                job->notified = 1;
            }
            else
            {
                continue;
            }
        }

        // 状态字符串
        const char *status_str;
        char status_marker = ' ';
        switch (job->status)
        {
        case JOB_RUNNING:
            status_str = "Running";
            status_marker = (i == job_list.count - 1) ? '+' : '-';
            break;
        case JOB_STOPPED:
            status_str = "Stopped";
            status_marker = (i == job_list.count - 1) ? '+' : '-';
            break;
        case JOB_DONE:
            status_str = "Done";
            break;
        case JOB_TERMINATED:
            status_str = "Terminated";
            break;
        default:
            status_str = "Unknown";
            break;
        }

        // 输出格式：[id] 标记 状态  命令
        if (show_pid)
        {
            printf("[%d]%c %-10s ", job->id, status_marker, status_str);
            for (int j = 0; j < job->num_pids; j++)
            {
                printf("%d ", job->pids[j]);
            }
            printf("    %s\n", job->command);
        }
        else if (show_pgid)
        {
            printf("[%d]%c %-10s %d    %s\n",
                   job->id, status_marker, status_str,
                   job->pgid, job->command);
        }
        else
        {
            printf("[%d]%c  %-10s %s\n",
                   job->id, status_marker, status_str,
                   job->command);
        }
    }
}

int fg_job(Job *job)
{
    if (job == NULL)
    {
        fprintf(stderr, "fg: no current job\n");
        return -1;
    }

    // 将作业的进程组设为前台进程组
    tcsetpgrp(STDIN_FILENO, job->pgid);

    // 如果作业已暂停，发送 SIGCONT 继续运行
    if (job->status == JOB_STOPPED)
    {
        kill(-job->pgid, SIGCONT);
    }

    job->status = JOB_RUNNING;

    // 等待作业完成或暂停
    int status;
    pid_t wpid;
    do
    {
        wpid = waitpid(-job->pgid, &status, WUNTRACED);
        if (wpid > 0)
        {
            update_job_status(wpid, status);
        }
    } while (job->status == JOB_RUNNING);

    // 将 shell 自身设回前台进程组
    tcsetpgrp(STDIN_FILENO, getpgrp());

    if (job->status == JOB_STOPPED)
    {
        printf("\n[%d]+  Stopped                 %s\n", job->id, job->command);
        return 0;
    }

    // 作业完成，清理
    if (WIFEXITED(status))
    {
        return WEXITSTATUS(status);
    }
    return 0;
}

int bg_job(Job *job)
{
    if (job == NULL)
    {
        fprintf(stderr, "bg: no current job\n");
        return -1;
    }

    if (job->status != JOB_STOPPED)
    {
        fprintf(stderr, "bg: job %d is already in the background\n", job->id);
        return -1;
    }

    // 发送 SIGCONT 继续运行
    kill(-job->pgid, SIGCONT);
    job->status = JOB_RUNNING;

    printf("[%d] %s &\n", job->id, job->command);
    return 0;
}

void clean_done_jobs(void)
{
    int i = 0;
    while (i < job_list.count)
    {
        Job *job = &job_list.jobs[i];
        if (job->status == JOB_DONE || job->status == JOB_TERMINATED)
        {
            // 释放资源
            free(job->pids);
            free(job->command);

            // 移动后续元素
            for (int j = i; j < job_list.count - 1; j++)
            {
                job_list.jobs[j] = job_list.jobs[j + 1];
            }
            job_list.count--;
        }
        else
        {
            i++;
        }
    }
}

void free_jobs(void)
{
    for (int i = 0; i < job_list.count; i++)
    {
        free(job_list.jobs[i].pids);
        free(job_list.jobs[i].command);
    }
    job_list.count = 0;
}