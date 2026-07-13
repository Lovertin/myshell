#ifndef JOB_H
#define JOB_H

#include "shell.h"

#define MAX_JOBS 64

/* 作业状态 */
typedef enum
{
    JOB_RUNNING,   // 运行中
    JOB_STOPPED,   // 已暂停
    JOB_DONE,      // 已完成
    JOB_TERMINATED // 已终止
} JobStatus;

/* 作业条目 */
typedef struct
{
    int id;           // 作业编号（从 1 开始）
    pid_t pgid;       // 进程组 ID
    pid_t *pids;      // 所有进程的 PID 数组
    int num_pids;     // 进程数
    char *command;    // 命令字符串
    JobStatus status; // 作业状态
    int notified;     // 是否已通知用户状态变化
} Job;

/* 作业列表 */
typedef struct
{
    Job jobs[MAX_JOBS];
    int count;
    int next_id; // 下一个作业编号
} JobList;

// 初始化作业系统
void init_jobs(void);

// 添加作业
int add_job(pid_t pgid, pid_t *pids, int num_pids, const char *command, int background);

// 更新作业状态（由 SIGCHLD 处理程序调用）
void update_job_status(pid_t pid, int status);

// 查找作业
Job *find_job_by_id(int id);
Job *find_job_by_pgid(pid_t pgid);
Job *find_job_by_prefix(const char *prefix);

// 获取最近的作业（用于 fg/bg 无参数时）
Job *get_recent_job(void);

// 打印作业列表
void print_jobs(int show_pid, int show_pgid, int only_running, int only_stopped);

// fg 命令：将作业调回前台
int fg_job(Job *job);

// bg 命令：让暂停的作业在后台继续运行
int bg_job(Job *job);

// 清理已完成的作业
void clean_done_jobs(void);

// 释放所有作业
void free_jobs(void);

#endif