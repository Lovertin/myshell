# MyShell - 简易 Shell 命令行解释器

这是一个用 C 语言编写的 Linux 命令行解释程序（Shell），实现了基础命令执行、内建命令、管道、重定向、后台运行、历史记录、别名和命令补全等功能。

## 功能特性

### 基础功能
- **外部命令执行**：支持执行系统中的任何可执行程序（如 `ls`、`cat`、`grep` 等）
- **进程管理**：使用 `fork()` + `execvp()` 创建子进程执行命令
- **信号处理**：正确处理 `SIGINT`、`SIGTSTP`、`SIGCHLD` 等信号

### 内建命令
- `cd [dir]` - 改变当前工作目录
- `exit` - 退出 Shell
- `echo [args...]` - 输出文本，支持环境变量展开（如 `$HOME`）
- `type [cmd]` - 显示命令类型（内建命令或外部程序路径）
- `history` - 显示命令历史记录
- `alias [name=value]` - 设置或显示别名
- `unalias [name]` - 删除别名

### 高级特性
- **管道（Pipe）**：支持多级管道，如 `ls | grep test | wc -l`
- **I/O 重定向**：
  - 输入重定向：`command < input.txt`
  - 输出重定向：`command > output.txt`
  - 追加输出：`command >> output.txt`
- **后台运行**：在命令末尾加 `&`，如 `sleep 10 &`
- **命令历史**：使用上下箭头浏览历史，支持 `!n` 执行第 n 条历史命令
- **别名功能**：自定义命令简写，如 `alias ll='ls -la'`
- **Tab 补全**：支持命令名和文件名的自动补全

## 编译和运行

### 前置要求
- Linux 操作系统（Ubuntu、CentOS、欧拉等）
- GCC 编译器
- GNU Readline 库

安装 Readline 库：
```bash
# Ubuntu/Debian
sudo apt-get install libreadline-dev

# CentOS/RHEL
sudo yum install readline-devel
```

### 编译
```bash
make
```

### 运行
```bash
./myshell
```

### 清理
```bash
make clean
```

## 使用示例

```bash
# 启动 Shell
$ ./myshell

# 基本命令
user@host:~$ ls -la
user@host:~$ cd /tmp
user@host:/tmp$ pwd

# 管道
user@host:~$ ps aux | grep bash | wc -l

# 重定向
user@host:~$ echo "Hello World" > test.txt
user@host:~$ cat < test.txt
user@host:~$ ls >> output.txt

# 后台运行
user@host:~$ sleep 10 &
[bg] 12345

# 别名
user@host:~$ alias ll='ls -la'
user@host:~$ ll

# 历史记录
user@host:~$ history
    1  ls -la
    2  cd /tmp
    3  pwd
user@host:~$ !2

# 环境变量
user@host:~$ echo $HOME
user@host:~$ echo $PATH

# 退出
user@host:~$ exit
```

## 项目结构

```
myshell/
├── Makefile              # 构建脚本
├── include/              # 头文件目录
│   ├── shell.h           # 主头文件
│   ├── parser.h          # 命令解析模块
│   ├── executor.h        # 命令执行模块
│   ├── builtins.h        # 内建命令模块
│   ├── history.h         # 历史记录模块
│   ├── alias.h           # 别名模块
│   └── completion.h      # 命令补全模块
├── src/                  # 源文件目录
│   ├── main.c            # 程序入口
│   ├── prompt.c          # 提示符生成
│   ├── parser.c          # 命令解析
│   ├── executor.c        # 命令执行
│   ├── builtins.c        # 内建命令实现
│   ├── history.c         # 历史记录管理
│   ├── alias.c           # 别名管理
│   └── completion.c      # Tab 补全功能
└── README.md             # 本文件
```

## 技术实现

### 核心技术
- **进程控制**：`fork()`、`execvp()`、`wait()`、`waitpid()`
- **管道通信**：`pipe()`、`dup2()`
- **文件操作**：`open()`、`close()`
- **信号处理**：`signal()`、`SIGCHLD`、`SIGINT`、`SIGTSTP`
- **GNU Readline**：交互式输入、命令历史、Tab 补全

### 架构设计
采用模块化设计，各模块职责清晰：
- **主循环**：读取-解析-执行循环
- **解析器**：将命令字符串解析为结构化数据
- **执行器**：处理命令执行、管道、重定向
- **内建命令**：在主进程中执行特殊命令
- **历史记录**：管理命令历史
- **别名系统**：命令别名映射
- **补全系统**：基于 Readline 的智能补全

## 注意事项

1. 本项目仅支持 Linux 系统（依赖 POSIX 系统调用）
2. Windows 用户需要使用 WSL（Windows Subsystem for Linux）
3. 确保已安装 `libreadline-dev` 库
4. 部分高级 Shell 功能（如通配符展开、作业控制）未实现

## 开发者

本项目为操作系统课程设计实验项目。

## 许可证

MIT License
