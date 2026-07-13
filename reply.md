# Myshell 项目演示脚本（约7分钟）

---

## 0:00 - 0:30 | 开场 & 项目概述

**大家好，今天我来演示我们实现的 Linux Shell 项目 — myshell。**

> myshell 是一个用 C 语言编写的命令行解释器，核心逻辑是：**读取命令 → 解析 → 创建子进程 → 执行**。它支持外部命令、内建命令、管道、重定向、后台运行、历史记录、别名和作业控制。

**先编译运行：**

```bash
# 编译
make

# 启动
./myshell
```

可以看到提示符变成了 `root@hostname:~/myshell$ `，说明 Shell 已经启动了。

---

## 0:30 - 1:30 | 基础命令执行

**我们先演示最基础的功能：执行外部命令。**

```bash
ls -la
}
```

> 当输入 `ls -la` 时，Shell 会通过 `fork()` 创建子进程，子进程调用 `execvp("ls", args)` 执行，父进程通过 `waitpid()` 等待结束。

```bash
echo "hello myshell"
cd /tmp
pwd
```

> `cd` 是内建命令 — 它必须在当前进程中执行，因为 `chdir()` 只影响调用它的进程。我们看代码：

```c
// 在 builtins.c 中
static int builtin_cd(SimpleCommand *cmd) {
    const char *dir = (cmd->argc < 2) ? getenv("HOME") : cmd->args[1];
    if (chdir(dir) != 0) {
        perror("cd");
        return -1;
    }
    return 0;
}
```

**这里的关键设计理念是：内建命令直接在当前进程调用函数执行，外部命令通过 `fork + exec` 在子进程执行。**

---

## 1:30 - 2:30 | 管道与重定向

**接下来是管道和 I/O 重定向 — 这是 Shell 最重要的特性之一。**

```bash
# 输出重定向（覆盖模式）
ls -la > output.txt
cat output.txt

# 输出重定向（追加模式）
echo "new line" >> output.txt
cat output.txt

# 输入重定向
wc -l < output.txt

# 管道
ls -la | grep "txt"
```

> 管道的核心机制是：创建 `pipe()`，左侧进程的 stdout 连到写端，右侧进程的 stdin 连到读端。我们看代码：

```c
// 在 executor.c 的 execute_pipeline_internal() 中
// 创建管道
for (int i = 0; i < num_commands - 1; i++) {
    pipe(pipefds[i]);
}

// 每个子进程：
// 不是第一个 → 从前一个管道读
if (i > 0) dup2(pipefds[i-1][0], STDIN_FILENO);
// 不是最后一个 → 写入下一个管道
if (i < num_commands - 1) dup2(pipefds[i][1], STDOUT_FILENO);
```

**设计要点：** 所有子进程使用同一个进程组 ID（PGID），这是作业控制的基础。

**我们还实现了更丰富的重定向：**

```bash
# 标准错误重定向
ls nonexistent 2> error.log
cat error.log

# 同时重定向 stdout 和 stderr
command &> all.log
command > out.log 2>&1
```

> `2>&1` 的实现很简单：`dup2(STDOUT_FILENO, STDERR_FILENO)` 让 stderr 指向 stdout 当前指向的位置。

---

## 2:30 - 3:30 | 复合命令

**Shell 还支持用 `;`、`&&`、`||` 连接多个命令：**

```bash
# 顺序执行
echo "第一行" ; echo "第二行" ; echo "第三行"

# 逻辑与：只有前面成功才执行后面
mkdir newdir && cd newdir && pwd

# 逻辑或：只有前面失败才执行后面
cd /nonexist || echo "目录不存在"
```

> 解析器在 `parse_compound()` 中通过 `find_next_operator()` 函数扫描字符串，遇到 `;`、`&&`、`||` 就分割成多个 Pipeline。注意这里要正确识别引号内的内容，避免把引号里的 `;` 误判。

```c
// 执行逻辑
switch (op) {
case OP_SEQ:  // 顺序执行
    last_status = execute_pipeline(&compound->pipelines[i]);
    break;
case OP_AND:  // 前一个成功才执行
    if (last_status == 0)
        last_status = execute_pipeline(&compound->pipelines[i]);
    break;
case OP_OR:   // 前一个失败才执行
    if (last_status != 0)
        last_status = execute_pipeline(&compound->pipelines[i]);
    break;
}
```

---

## 3:30 - 4:30 | 历史记录

**我们支持查看和重用历史命令：**

```bash
history
history 10
!42
```

> `history` 命令存储在一个循环数组中。`history N` 显示最近 N 条记录。`!42` 会从数组中取出第 42 条命令重新执行。

```c
// expand_history() 在 history.c 中
if (cmd[0] == '!') {
    int n = atoi(cmd + 1);
    // 从循环数组中取出第 n 条命令
    int idx = (start_idx + n - 1) % MAX_HISTORY;
    return strdup(hist.entries[idx]);
}
```

---

## 4:30 - 5:30 | 别名功能

**别名让用户可以用简短名称代替长命令：**

```bash
alias ll="ls -la"
alias gs="git status"

alias ll
alias

\ll
```

> `alias` 用一个结构体数组存储名称-值映射。`alias name=value` 存储，`alias name` 显示单条，`unalias name` 删除。

```c
// 当执行命令时，先在别名表中查找
char *alias_expanded = expand_alias(cmd_line);

// expand_alias() 检查第一个词是否在别名表中
// 如果在，替换为实际命令
// 如果以 \ 开头，跳过别名展开
```

**关键设计：** 别名展开发生在解析之前，所以别名可以包含管道、重定向等完整命令。

---

## 5:30 - 6:15 | 后台运行 & 作业控制

**后台运行和作业控制是 Shell 的高级功能：**

```bash
# 后台运行
sleep 30 &
sleep 20 &

# 查看后台任务
jobs -l

# 调回前台
fg %1

# 让暂停的任务在后台继续
bg
```

> 每个后台任务都被记录为一个 Job，包含进程组 ID、状态（Running/Stopped/Done）和命令字符串。`jobs` 显示所有作业，`fg %1` 将作业 1 调到前台。

```c
// 在 executor.c 中
// 每个子进程都在自己的进程组中
setpgid(0, 0);

// 前台任务：将进程组设为前台进程组
tcsetpgrp(STDIN_FILENO, pid);

// 等待完成或被暂停
waitpid(pid, &status, WUNTRACED);

// 完成后恢复 Shell 为前台
tcsetpgrp(STDIN_FILENO, getpgrp());
```

**重要设计：** Shell 自身忽略 SIGTTOU 和 SIGTTIN，防止在切换前台进程组时被信号停止。

---

## 6:15 - 7:00 | 代码架构总结

**最后总结一下项目的整体架构：**

```
src/main.c      ← Shell 主循环：读取 → 解析 → 执行
src/parser.c    ← 词法/语法分析：识别命令、参数、重定向、管道
src/executor.c  ← 命令执行：fork + exec + pipe + dup2
src/builtins.c  ← 内建命令：cd, exit, echo, history, alias 等
src/prompt.c    ← 提示符生成与信号处理
src/history.c   ← 历史记录存储与 !n 展开
src/alias.c     ← 别名管理
src/job.c       ← 作业控制：jobs, fg, bg
src/completion.c ← Tab 补全（readline 库）
```

**核心数据结构：**

- `SimpleCommand` — 单条命令（含参数和重定向信息）
- `Pipeline` — 由 `|` 连接的多个 `SimpleCommand`
- `CompoundCommand` — 由 `;`、`&&`、`||` 连接的多个 `Pipeline`
- `Job` — 作业（进程组 ID、状态、命令字符串）

**使用的关键系统调用：**
- `fork()` — 创建子进程
- `execvp()` — 在 PATH 中查找并执行程序
- `pipe()` — 创建管道
- `dup2()` — 复制文件描述符（实现重定向）
- `waitpid()` / `SIGCHLD` — 进程同步与资源回收
- `tcsetpgrp()` — 控制终端前台进程组
- `signal()` — 信号处理

**谢谢大家，我的演示到此结束。欢迎提问！**