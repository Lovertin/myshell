# Shell 程序详细架构设计文档

## 一、项目概述

本项目为一个用 C 语言编写的 Linux 命令行解释程序（Shell），支持基础命令执行、内建命令、管道、重定向、后台运行、历史记录、别名和命令补全等功能。

---

## 二、文件结构

```
myshell/
├── Makefile              # 构建脚本
├── include/
│   ├── shell.h           # 主头文件，全局定义与函数声明
│   ├── parser.h          # 命令解析模块头文件
│   ├── executor.h        # 命令执行模块头文件
│   ├── builtins.h        # 内建命令模块头文件
│   ├── history.h         # 历史记录模块头文件
│   ├── alias.h           # 别名模块头文件
│   └── completion.h      # 命令补全模块头文件
├── src/
│   ├── main.c            # 程序入口，主循环
│   ├── prompt.c          # 提示符打印
│   ├── parser.c          # 命令解析（切分、别名替换、管道/重定向识别）
│   ├── executor.c        # 命令执行（fork/exec/wait、管道、重定向、后台）
│   ├── builtins.c        # 内建命令实现（cd, exit, type, echo, alias, history等）
│   ├── history.c         # 历史记录管理
│   ├── alias.c           # 别名管理
│   └── completion.c      # Tab 补全功能（基于 readline）
└── README.md             # 项目说明
```

---

## 三、核心数据结构

### 3.1 命令结构体

```c
#define MAX_ARGS 64
#define MAX_PIPES 16
#define MAX_HISTORY 1000
#define MAX_ALIAS 128
#define MAX_CMD_LEN 1024

/* 单条简单命令 */
typedef struct {
    char *args[MAX_ARGS];   // 参数列表，args[0] 为命令名
    int   argc;             // 参数个数
    char *input_file;       // 输入重定向文件（< file），NULL 表示无
    char *output_file;      // 输出重定向文件（> file），NULL 表示无
    int   append;           // 1 表示追加模式（>>），0 表示覆盖
} SimpleCommand;

/* 完整命令行（可含管道） */
typedef struct {
    SimpleCommand commands[MAX_PIPES];  // 管道连接的多条简单命令
    int num_commands;                   // 命令段数
    int background;                     // 1 表示后台运行（&）
} Pipeline;
```

### 3.2 历史记录结构

```c
typedef struct {
    char *entries[MAX_HISTORY];  // 循环数组存储历史命令
    int   count;                 // 已记录总数
    int   start;                 // 循环数组起始索引
} History;
```

### 3.3 别名结构

```c
typedef struct {
    char *name;     // 别名
    char *value;    // 对应的实际命令字符串
} AliasEntry;

typedef struct {
    AliasEntry entries[MAX_ALIAS];
    int count;
} AliasList;
```

---

## 四、模块设计

### 4.1 主循环模块（main.c）

**职责：** 程序入口，驱动 Shell 的读取-解析-执行循环。

```c
int main(int argc, char *argv[]) {
    // 初始化：历史记录、别名表、readline 补全
    init_history();
    init_alias();
    init_completion();

    while (1) {
        // 1. 通过 readline 读取输入（自带提示符和 Tab 补全支持）
        char *cmd_line = read_command();
        if (cmd_line == NULL) break;  // EOF (Ctrl+D) 退出

        // 2. 跳过空行
        if (strlen(cmd_line) == 0) { free(cmd_line); continue; }

        // 3. 添加到历史记录
        add_to_history(cmd_line);

        // 4. 处理 !n 历史命令调用
        cmd_line = expand_history(cmd_line);

        // 5. 别名展开
        cmd_line = expand_alias(cmd_line);

        // 6. 解析命令行
        Pipeline pipeline = parse_command(cmd_line);

        // 7. 执行
        execute_pipeline(&pipeline);

        free(cmd_line);
    }

    cleanup();
    return 0;
}
```

---

### 4.2 提示符模块（prompt.c）

**职责：** 生成并返回格式化的提示符字符串。

**接口：**
```c
// 返回格式为 "username@hostname:current_dir$ " 的提示符
char *get_prompt(void);
```

**实现要点：**
- 使用 `getenv("USER")` 获取用户名
- 使用 `gethostname()` 获取主机名
- 使用 `getcwd()` 获取当前工作目录
- 将 HOME 目录替换为 `~` 显示

---

### 4.3 命令解析模块（parser.c）

**职责：** 将原始命令字符串解析为结构化的 `Pipeline` 对象。

**接口：**
```c
// 解析完整命令行，返回 Pipeline 结构
Pipeline parse_command(const char *cmd_line);
```

**解析流程：**

```
原始字符串
    │
    ▼
检测末尾 '&' → 设置 background 标志
    │
    ▼
按 '|' 分割为多段子命令字符串
    │
    ▼
对每段子命令：
    ├── 检测并提取 '<' 后的输入文件名
    ├── 检测并提取 '>' 或 '>>' 后的输出文件名
    └── 按空格/引号分词，填充 args[] 数组
    │
    ▼
返回 Pipeline 结构体
```

**实现要点：**
- 支持双引号包裹的带空格参数
- 正确处理转义字符
- 分词时使用自定义的 tokenizer（不使用 `strtok`，因为需要处理引号）

---

### 4.4 命令执行模块（executor.c）

**职责：** 根据解析结果，调度执行内建命令或外部命令，处理管道和重定向。

**接口：**
```c
// 执行整条管道命令
void execute_pipeline(Pipeline *pipeline);
```

**执行流程图：**

```
execute_pipeline()
    │
    ├── num_commands == 1 且为内建命令？
    │       ├── 是 → 直接调用 execute_builtin()
    │       └── 否 → 进入 fork/exec 流程
    │
    └── num_commands > 1（管道）
            │
            ▼
        创建 (num_commands - 1) 个 pipe
            │
            ▼
        循环 fork 每个子进程：
            ├── 非首个命令：dup2(prev_pipe[0], STDIN)
            ├── 非末尾命令：dup2(curr_pipe[1], STDOUT)
            ├── 处理该命令自身的 < > 重定向
            ├── 关闭所有无关 fd
            └── execvp(args[0], args)
            │
            ▼
        父进程关闭所有管道 fd
            │
            ├── background == 0 → waitpid 等待所有子进程
            └── background == 1 → 打印 PID，不等待
```

**单命令执行（无管道）详细逻辑：**

```c
void execute_single(SimpleCommand *cmd, int background) {
    pid_t pid = fork();
    if (pid == 0) {
        // 子进程
        // 输入重定向
        if (cmd->input_file) {
            int fd = open(cmd->input_file, O_RDONLY);
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        // 输出重定向
        if (cmd->output_file) {
            int flags = O_WRONLY | O_CREAT | (cmd->append ? O_APPEND : O_TRUNC);
            int fd = open(cmd->output_file, flags, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        // 执行
        execvp(cmd->args[0], cmd->args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // 父进程
        if (!background) {
            int status;
            waitpid(pid, &status, 0);
        } else {
            printf("[bg] %d\n", pid);
        }
    } else {
        perror("fork");
    }
}
```

---

### 4.5 内建命令模块（builtins.c）

**职责：** 在主进程中执行不能通过子进程完成的命令。

**接口：**
```c
// 判断是否为内建命令
int is_builtin(const char *cmd_name);

// 执行内建命令，返回 0 成功，非 0 失败
int execute_builtin(SimpleCommand *cmd);
```

**支持的内建命令列表：**

| 命令 | 实现方式 |
|------|----------|
| `cd [dir]` | 调用 `chdir(dir)`，无参数时切换到 HOME |
| `exit` | 调用 `cleanup()` 后 `exit(0)` |
| `echo [args...]` | 逐个打印参数，处理环境变量 `$VAR` 展开 |
| `type [cmd]` | 判断命令类型：builtin / 在 PATH 中的路径 |
| `history` | 调用 `print_history()` 显示历史记录 |
| `alias [name=value]` | 无参数时打印所有别名；有参数时注册别名 |
| `unalias [name]` | 删除指定别名 |

---

### 4.6 历史记录模块（history.c）

**职责：** 管理命令历史，支持记录、查询和 `!n` 扩展。

**接口：**
```c
void  init_history(void);
void  add_to_history(const char *cmd);
void  print_history(void);           // 打印所有历史记录（带编号）
char *expand_history(char *cmd);     // 将 "!n" 替换为第 n 条历史命令
void  free_history(void);
```

**实现要点：**
- 使用循环数组存储，超过 `MAX_HISTORY` 时覆盖最旧记录
- 同时调用 `add_history()`（readline 库）使上下箭头可浏览历史
- `!n` 中 n 为从 1 开始的历史编号

---

### 4.7 别名模块（alias.c）

**职责：** 管理别名映射表，提供别名展开功能。

**接口：**
```c
void  init_alias(void);
void  set_alias(const char *name, const char *value);
void  remove_alias(const char *name);
char *expand_alias(char *cmd);       // 如果命令首词匹配别名则替换
void  print_aliases(void);           // 打印所有已定义别名
void  free_alias(void);
```

**实现要点：**
- 使用结构体数组线性查找（命令数量小，性能足够）
- 别名展开仅替换命令行的第一个词
- 防止递归展开（展开后不再二次查找别名）

---

### 4.8 命令补全模块（completion.c）

**职责：** 利用 readline 库实现 Tab 键自动补全。

**接口：**
```c
void init_completion(void);  // 注册 readline 补全回调
```

**实现要点：**
```c
#include <readline/readline.h>
#include <readline/history.h>

// 自定义补全生成器
char *command_generator(const char *text, int state);
char **shell_completion(const char *text, int start, int end);

void init_completion(void) {
    rl_attempted_completion_function = shell_completion;
}

char **shell_completion(const char *text, int start, int end) {
    if (start == 0) {
        // 命令名补全：搜索 PATH 目录 + 内建命令
        return rl_completion_matches(text, command_generator);
    }
    // 非首词：默认文件名补全（readline 自动处理）
    return NULL;
}
```

**补全策略：**
- 光标在行首（start == 0）：补全命令名（搜索 PATH 下可执行文件 + 内建命令列表）
- 光标在参数位置：使用 readline 默认的文件名补全

---

## 五、模块依赖关系

```
main.c
  ├── prompt.c         （提示符生成）
  ├── parser.c         （命令解析）
  │     └── alias.c    （别名展开）
  ├── executor.c       （命令执行）
  │     └── builtins.c （内建命令）
  │           ├── history.c   （history 命令实现）
  │           └── alias.c     （alias/unalias 命令实现）
  ├── history.c        （历史记录管理）
  └── completion.c     （Tab 补全）
```

---

## 六、信号处理设计

```c
// 在 main 初始化阶段注册信号处理
void setup_signals(void) {
    // Shell 主进程忽略 SIGINT (Ctrl+C)，防止 Shell 自身被终止
    signal(SIGINT, SIG_IGN);
    // 忽略 SIGTSTP (Ctrl+Z)
    signal(SIGTSTP, SIG_IGN);
    // 处理 SIGCHLD，回收后台子进程，避免僵尸进程
    signal(SIGCHLD, sigchld_handler);
}

void sigchld_handler(int sig) {
    // 非阻塞回收所有已终止的子进程
    while (waitpid(-1, NULL, WNOHANG) > 0);
}
```

**子进程中恢复默认信号处理：**
```c
// 在 fork 后的子进程中
signal(SIGINT, SIG_DFL);
signal(SIGTSTP, SIG_DFL);
```

---

## 七、错误处理策略

| 场景 | 处理方式 |
|------|----------|
| 命令不存在 | `execvp` 失败后打印 `"command not found"` 并 `exit(127)` |
| `fork()` 失败 | 打印错误信息，继续主循环 |
| 重定向文件打开失败 | 打印 `perror` 信息，子进程 `exit(1)` |
| `pipe()` 失败 | 打印错误信息，放弃本条命令 |
| `cd` 目标目录不存在 | 打印 `"No such file or directory"` |
| 空输入 / 仅空格 | 跳过，直接进入下一循环 |
| `!n` 中 n 超出范围 | 打印 `"event not found"` |

---

## 八、Makefile 构建脚本

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude
LDFLAGS = -lreadline

SRC_DIR = src
OBJ_DIR = obj
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
TARGET = myshell

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean
```

---

## 九、开发阶段规划

| 阶段 | 目标 | 涉及模块 |
|------|------|----------|
| 第一阶段 | 主循环 + 提示符 + 执行单条外部命令 | main.c, prompt.c, parser.c, executor.c |
| 第二阶段 | 内建命令（cd, exit, echo, type）+ 别名 + 历史记录 | builtins.c, history.c, alias.c |
| 第三阶段 | I/O 重定向 + 后台运行 + 单管道 | executor.c (扩展), parser.c (扩展) |
| 第四阶段 | 多级管道 + Tab 补全 + 健壮性完善 | executor.c (扩展), completion.c |

---

## 十、关键系统调用总结

| 系统调用 | 用途 |
|----------|------|
| `fork()` | 创建子进程执行外部命令 |
| `execvp()` | 在子进程中加载并执行程序 |
| `waitpid()` | 父进程等待子进程结束 |
| `pipe()` | 创建管道用于进程间通信 |
| `dup2()` | 重定向标准输入/输出到文件或管道 |
| `open()` / `close()` | 打开/关闭文件用于重定向 |
| `chdir()` | 改变当前工作目录（cd 命令） |
| `getcwd()` | 获取当前工作目录路径 |
| `signal()` | 注册信号处理函数 |
| `readline()` | 读取用户输入（支持编辑和补全） |
