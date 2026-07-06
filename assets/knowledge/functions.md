# 1. 命令解释执行 (Command Execution)

## 详细描述 (Detailed Description)
命令解释执行是 Shell 的核心功能，负责解析用户输入的命令行，识别命令名称、参数和选项，然后调用相应的程序执行。Shell 会在 PATH 环境变量指定的目录中查找外部命令，或直接执行内置命令（如 cd、exit 等）。

执行流程：
1. **词法分析**: 将输入字符串分解为 tokens（命令、参数、操作符等）
2. **语法分析**: 解析命令结构，识别重定向、管道等特殊操作
3. **路径解析**: 查找可执行文件的完整路径
4. **进程创建**: 使用 fork() 创建子进程
5. **程序执行**: 使用 exec() 系列函数执行目标程序
6. **等待完成**: 父进程使用 wait() 等待子进程结束

## 示例代码 (Example Code)

```bash
# 1. 执行外部程序（绝对路径）
/usr/bin/ls
/bin/echo "Hello World"

# 2. 执行外部程序（相对路径）
./my_program
../build/app

# 3. 执行 PATH 中的命令
ls -l
grep "pattern" file.txt
python3 script.py

# 4. 带参数和选项的命令
ls -la /var/log                    # 长格式列出所有文件
grep -r "error" /var/log           # 递归搜索
tar -xzvf archive.tar.gz           # 解压文件

# 5. 内置命令执行
cd /home/user                      # 改变目录
pwd                                # 打印当前目录
export PATH=$PATH:/new/path        # 设置环境变量
echo $HOME                         # 显示环境变量

# 6. 复合命令执行
cd /tmp ; touch test.txt ; ls      # 顺序执行，不管前一个是否成功
mkdir newdir && cd newdir          # 逻辑与，前一个成功才执行后一个
cd nonexist || echo "Failed"       # 逻辑或，前一个失败才执行后一个

# 7. 命令替换
echo "Today is $(date)"            # 现代语法
echo "Files: `ls`"                 # 传统语法

# 8. 带引号的命令（处理空格和特殊字符）
echo "Hello World"                 # 双引号：允许变量扩展
echo 'Price: $100'                 # 单引号：阻止变量扩展
./program "file with spaces.txt"   # 将带空格的文件名作为单个参数
```

## 实现要点 (Implementation Notes)
- 需要正确解析命令行，处理引号、转义字符
- 区分内置命令和外部命令
- 使用 fork() + exec() 执行外部命令
- 内置命令直接在当前进程中执行（如 cd、export）
- 正确处理命令的返回值（退出状态）

# 2. 命令补全 (Command Completion)

## 详细描述 (Detailed Description)
命令补全是一个交互式功能，通过快捷键（主要是 Tab 键）触发，自动补全用户正在输入的命令、路径或文件名。这大大提高了输入效率，减少拼写错误。

补全类型：
1. **命令补全**: 根据已输入的前缀，在 PATH 中查找匹配的可执行文件
2. **路径补全**: 补全文件系统路径，包括目录和文件名
3. **参数补全**: 根据命令的上下文补全参数（高级功能）

工作原理：
- **Tab 键按一次**: 如果只有一个匹配项，自动补全；如果有多个匹配项，补全到公共前缀
- **Tab 键按两次**: 显示所有可能的补全选项列表
- 使用 readline 库实现用户输入和补全功能

## 示例代码 (Example Code)

```bash
# 1. 命令补全示例
$ gre[Tab]           # 自动补全为 "grep"（假设只有一个匹配）
$ g[Tab][Tab]        # 显示所有以 g 开头的命令：gcc, git, grep, gunzip...

# 2. 路径补全示例
$ cd /usr/lo[Tab]    # 自动补全为 "/usr/local/"
$ ls ~/Doc[Tab]      # 自动补全为 "~/Documents/"
$ cat /etc/pas[Tab]  # 自动补全为 "/etc/passwd"

# 3. 文件名补全（多个匹配）
$ ls test[Tab][Tab]  # 显示所有以 test 开头的文件
test.c  test.h  test.txt  test_data.json

# 4. 带空格的文件名补全
$ cat my\ f[Tab]     # 自动补全为 "my\ file.txt" 或 "my file.txt"

# 5. 相对路径补全
$ cd ../[Tab][Tab]   # 显示上级目录的所有子目录
$ ./[Tab][Tab]       # 显示当前目录下的所有可执行文件

# 6. 环境变量补全
$ echo $HO[Tab]      # 自动补全为 "$HOME"
$ cd $P[Tab][Tab]    # 显示所有以 P 开头的环境变量
```

## 实现要点 (Implementation Notes)
- 使用 readline 库的 `rl_completion_matches()` 函数
- 需要实现自定义的 completion generator 函数
- 命令补全：遍历 PATH 环境变量中的目录，查找匹配的可执行文件
- 路径补全：使用 `opendir()` 和 `readdir()` 遍历目录内容
- 需要处理特殊字符：空格、引号、反斜杠等
- 设置 readline 的自定义补全函数：`rl_attempted_completion_function`
- 实现 TAB 键绑定和双击 TAB 显示所有选项的功能

# 3. 查阅历史功能 (History Feature)

## 详细描述 (Detailed Description)
历史功能记录用户在 Shell 中执行过的所有命令，允许用户查看、搜索和重新执行这些命令。这大大提高了工作效率，特别是需要重复执行复杂命令时。

核心功能：
1. **命令记录**: 将每个执行的命令保存到历史列表中
2. **历史查看**: 使用 `history` 命令查看历史记录
3. **历史搜索**: 通过 Ctrl+R 或 grep 搜索特定命令
4. **历史执行**: 使用特殊语法快速重新执行历史命令
5. **持久化**: 历史记录保存在文件中（如 ~/.bash_history）

## 示例代码 (Example Code)

```bash
# 1. 查看历史命令
history                # 显示所有历史命令（带编号）
history 10             # 显示最近 10 条命令
history | tail -20     # 显示最后 20 条命令

# 2. 搜索历史命令
history | grep "docker"        # 搜索包含 "docker" 的命令
history | grep "git commit"    # 搜索 git commit 相关命令

# 3. 使用 ! 快捷执行历史命令
!142           # 执行历史记录中第 142 号命令
!!             # 重新执行上一条命令（最常用）
!-1            # 重新执行上一条命令（同 !!）
!-3            # 重新执行倒数第三条命令

# 4. 前缀匹配执行
!ls            # 执行最近一次以 "ls" 开头的命令
!cd            # 执行最近一次以 "cd" 开头的命令
!git           # 执行最近一次以 "git" 开头的命令

# 5. 快捷键搜索（交互式）
# Ctrl + R -> 进入逆向搜索模式
# 输入关键字，实时匹配历史记录
# 按 Enter 执行，按 Ctrl + R 继续查找上一个匹配
# 按 Ctrl + G 或 Esc 退出搜索模式

# 6. 获取上一条命令的参数
ls /var/log/nginx/access.log
cat !$         # !$ 代表上一条命令的最后一个参数
               # 等价于: cat /var/log/nginx/access.log

# 7. 获取上一条命令的所有参数
cp /src/file1.txt /dest/
ls !*          # !* 代表上一条命令的所有参数

# 8. 历史替换
^old^new       # 将上一条命令中的 old 替换为 new 后执行
# 例如：ls /tmo -> ^tmo^tmp -> ls /tmp

# 9. 清空历史记录
history -c     # 清空当前会话的历史记录
> ~/.bash_history  # 清空历史文件
```

## 实现要点 (Implementation Notes)
- 使用 readline 库的 history 功能：`add_history()`, `read_history()`, `write_history()`
- 维护一个历史命令链表或数组
- 实现历史文件的读取和写入（通常在 ~/.myshell_history）
- 支持历史命令的编号显示
- 实现 `!` 语法的解析和执行：
  - `!n`: 执行第 n 号命令
  - `!!`: 执行上一条命令
  - `!prefix`: 执行最近的以 prefix 开头的命令
- 集成 readline 的 Ctrl+R 搜索功能
- 设置历史记录的最大数量限制（如 1000 条）
- 在 Shell 退出时保存历史到文件

# 4. 别名功能 (Alias)

## 详细描述 (Detailed Description)
别名功能允许用户为常用的复杂命令创建简短的快捷方式。这不仅节省了输入时间，还减少了输入错误的可能性。别名在命令解析的最早阶段展开，替换为实际的命令字符串。

核心概念：
1. **别名定义**: 使用 `alias name='command'` 创建别名
2. **别名展开**: Shell 在执行前将别名替换为实际命令
3. **别名查看**: 使用 `alias` 命令列出所有已定义的别名
4. **别名删除**: 使用 `unalias name` 删除指定别名
5. **别名优先级**: 别名优先于同名的外部命令执行

工作原理：
- Shell 维护一个别名哈希表或链表
- 在解析命令时，首先检查命令名是否是已定义的别名
- 如果是别名，则将其替换为对应的命令字符串
- 别名可以包含参数，但不能覆盖内置命令（通常）

## 示例代码 (Example Code)

```bash
# 1. 查看所有已定义的别名
alias                  # 显示所有别名
alias | grep "git"     # 查找与 git 相关的别名

# 2. 创建简单别名
alias ll='ls -lh'                    # 长格式列表，人类可读的文件大小
alias la='ls -la'                    # 显示所有文件（包括隐藏文件）
alias ..='cd ..'                     # 快速返回上级目录
alias ...='cd ../..'                 # 返回上两级目录

# 3. 创建复杂命令的别名
alias gs='git status'
alias ga='git add'
alias gc='git commit'
alias gp='git push'
alias gl='git log --oneline --graph --decorate'

# 4. 带默认参数的别名
alias grep='grep --color=auto'       # grep 总是高亮显示匹配项
alias df='df -h'                     # 以人类可读格式显示磁盘使用情况
alias du='du -h'                     # 以人类可读格式显示目录大小
alias mkdir='mkdir -p'               # 自动创建父目录

# 5. 安全相关的别名
alias rm='rm -i'                     # 删除前确认
alias cp='cp -i'                     # 覆盖前确认
alias mv='mv -i'                     # 移动前确认

# 6. 组合命令别名
alias update='sudo apt update && sudo apt upgrade'
alias ports='netstat -tulanp'
alias myip='curl ifconfig.me'
alias weather='curl wttr.in'

# 7. 目录导航别名
alias home='cd ~'
alias proj='cd ~/projects'
alias docs='cd ~/Documents'

# 8. 删除别名
unalias ll                           # 删除 ll 别名
unalias gs ga gc                     # 删除多个别名

# 9. 临时绕过别名（使用 \）
alias ls='ls --color=auto'
\ls                                  # 执行原始的 ls 命令，不使用别名

# 10. 查看特定别名的定义
alias ll                             # 显示 ll 别名的定义

# 11. 带参数的别名（使用函数替代）
# 别名不能直接接收参数，但可以通过函数实现
alias gitc='git commit -m'           # 这个别名需要后接消息
gitc "Initial commit"                # 使用方式

# 12. 实用别名示例集合
alias c='clear'
alias h='history'
alias j='jobs'
alias path='echo $PATH | tr ":" "\n"'  # 美化显示 PATH
alias now='date +"%Y-%m-%d %H:%M:%S"'
alias timestamp='date +%s'
```

## 持久化配置 (Persistence Configuration)

```bash
# 将别名写入配置文件以永久生效
# Bash 用户: 编辑 ~/.bashrc
# Zsh 用户: 编辑 ~/.zshrc

echo "alias ll='ls -lh'" >> ~/.bashrc
source ~/.bashrc         # 重新加载配置文件使其生效
```

## 实现要点 (Implementation Notes)
- 使用哈希表或链表存储别名（key: 别名名称, value: 实际命令）
- 实现 `alias` 命令：
  - 无参数：列出所有别名
  - 带参数（name=value）：创建/更新别名
  - 只有名称：显示该别名的定义
- 实现 `unalias` 命令：删除指定的别名
- 在命令解析时，检查命令名是否为别名
- 如果是别名，进行字符串替换
- 注意递归别名的处理（避免无限循环）
- 支持从配置文件加载别名（启动时）
- 别名展开只发生在命令的第一个单词

# 5. 后台处理 (Background Processing)

## 详细描述 (Detailed Description)
后台处理允许用户将耗时的任务放到后台执行，这样终端不会被阻塞，用户可以继续输入其他命令。这对于长时间运行的任务（如编译、下载、服务器启动等）非常有用。

核心概念：
1. **前台进程**: 占用终端，阻塞用户输入的进程
2. **后台进程**: 在后台运行，不阻塞终端的进程
3. **作业控制**: 管理多个进程的启动、暂停、继续和终止
4. **进程状态**: Running（运行中）、Stopped（暂停）、Done（完成）

工作原理：
- 使用 `&` 符号将命令放到后台执行
- Shell 不等待后台进程结束，立即返回命令提示符
- 使用作业控制命令（jobs、fg、bg）管理后台任务
- 后台进程的标准输入被重定向（通常无法接收用户输入）

## 示例代码 (Example Code)

```bash
# 1. 启动后台任务
sleep 100 &                          # 在后台运行 sleep 命令
[1] 12345                            # Shell 输出：作业编号和进程 ID

# 2. 多个后台任务
sleep 50 &
python3 server.py &
./long_running_task &

# 3. 查看所有后台任务
jobs                                 # 列出当前 Shell 的所有作业
# 输出示例：
# [1]   Running                 sleep 100 &
# [2]-  Running                 python3 server.py &
# [3]+  Stopped                 vim file.txt

# 4. 查看详细的作业信息
jobs -l                              # 显示作业编号和 PID
jobs -p                              # 只显示 PID
jobs -r                              # 只显示运行中的作业
jobs -s                              # 只显示暂停的作业

# 5. 将后台任务调回前台
fg                                   # 将最近的后台任务调回前台
fg %1                                # 将作业编号 1 调回前台
fg %python                           # 将命令包含 "python" 的作业调回前台

# 6. 暂停前台任务（Ctrl + Z）
vim file.txt                         # 启动 vim
# 按 Ctrl + Z 暂停
[1]+  Stopped                 vim file.txt

# 7. 将暂停的任务转到后台继续运行
bg                                   # 继续运行最近暂停的作业
bg %1                                # 继续运行作业编号 1
bg %2 %3                             # 继续运行多个作业

# 8. 后台任务的输出重定向
./program &                          # 后台运行，但输出仍显示在终端
./program > output.log 2>&1 &        # 重定向输出和错误到文件
./program &> output.log &            # 简化写法（现代 Shell）

# 9. 使用 nohup 让任务在退出 Shell 后继续运行
nohup ./long_task &                  # 即使退出 Shell，任务仍继续
nohup ./long_task > /dev/null 2>&1 & # 忽略所有输出

# 10. 终止后台任务
kill %1                              # 终止作业编号 1
kill 12345                           # 通过 PID 终止进程
kill -9 %2                           # 强制终止作业编号 2

# 11. 等待后台任务完成
sleep 10 &
wait                                 # 等待所有后台任务完成
wait %1                              # 等待特定作业完成
wait 12345                           # 等待特定 PID 完成

# 12. 实用示例组合
# 编译大型项目并在后台运行
make -j4 > build.log 2>&1 &

# 启动开发服务器并继续工作
npm start &

# 下载文件到后台
wget http://example.com/largefile.zip &

# 批量后台任务
for i in {1..5}; do
    ./process_file_$i.sh > log_$i.txt &
done
wait                                 # 等待所有任务完成
echo "All tasks completed"
```

## 快捷键总结 (Keyboard Shortcuts)

```bash
Ctrl + Z    # 暂停当前前台进程，将其放入后台（Stopped 状态）
Ctrl + C    # 终止当前前台进程
```

## 实现要点 (Implementation Notes)
- 维护一个作业列表（job list），记录所有后台任务
- 每个作业包含：作业编号、进程 ID、命令、状态
- 实现 `&` 操作符的解析：
  - 创建子进程后，父进程不调用 wait()
  - 立即返回命令提示符
- 实现 `jobs` 命令：遍历作业列表，显示状态
- 实现 `fg` 命令：
  - 将指定作业的进程组设为前台进程组
  - 使用 `tcsetpgrp()` 设置终端的前台进程组
  - 调用 `waitpid()` 等待进程
- 实现 `bg` 命令：
  - 向暂停的进程发送 SIGCONT 信号
  - 让进程在后台继续运行
- 处理 SIGCHLD 信号：
  - 当子进程状态改变时更新作业列表
  - 清理已完成的后台进程
- 使用 `setpgid()` 为每个作业创建独立的进程组
- 支持作业规范：%n（作业号）、%string（命令前缀）

# 6. I/O 重定向 (I/O Redirection)

## 详细描述 (Detailed Description)
I/O 重定向允许改变命令的标准输入、标准输出和标准错误的默认位置。在 Unix/Linux 系统中，每个进程都有三个标准文件描述符：
- **文件描述符 0 (stdin)**: 标准输入，默认从键盘读取
- **文件描述符 1 (stdout)**: 标准输出，默认输出到终端
- **文件描述符 2 (stderr)**: 标准错误，默认输出到终端

通过重定向，可以将这些输入/输出流重定向到文件、设备或其他命令。

核心操作符：
- `>` : 输出重定向（覆盖）
- `>>` : 输出重定向（追加）
- `<` : 输入重定向
- `2>` : 错误重定向
- `&>` : 同时重定向输出和错误
- `2>&1` : 将错误重定向到输出

## 示例代码 (Example Code)

```bash
# 1. 标准输出重定向（覆盖模式）
echo "hello" > output.txt            # 创建文件并写入（覆盖已有内容）
ls -l > filelist.txt                 # 将目录列表保存到文件
date > timestamp.txt                 # 保存当前时间

# 2. 标准输出重定向（追加模式）
echo "world" >> output.txt           # 追加内容到文件末尾
echo "line 2" >> log.txt             # 连续追加
ls -la >> filelist.txt               # 追加更多内容

# 3. 标准错误重定向
ls nonexistent_file 2> error.log     # 只重定向错误信息
command_that_fails 2> /dev/null      # 丢弃错误信息
grep "pattern" file.txt 2> err.txt   # 错误保存到文件

# 4. 同时重定向标准输出和标准错误（方法一）
command > output.log 2>&1            # 传统语法
ls -l /root > result.txt 2>&1        # 输出和错误都写入同一文件

# 5. 同时重定向标准输出和标准错误（方法二）
command &> output.log                # 现代 Shell 简化语法
ls -l /root &> result.txt            # 等价于上面的写法
command &>> output.log               # 追加模式

# 6. 分别重定向输出和错误
command > output.txt 2> error.txt    # 正常输出和错误分开保存
./script.sh > stdout.log 2> stderr.log

# 7. 标准输入重定向
cat < input.txt                      # 从文件读取输入
wc -l < file.txt                     # 统计文件行数
sort < unsorted.txt                  # 排序文件内容

# 8. Here Document（多行输入重定向）
cat << EOF > config.txt
line 1
line 2
line 3
EOF

# 使用 Here Document 创建脚本
cat << 'END' > script.sh
#!/bin/bash
echo "Hello World"
END

# 9. Here String（字符串输入重定向）
cat <<< "Hello World"                # 将字符串作为输入
grep "pattern" <<< "$variable"       # 在变量中搜索

# 10. 文件描述符操作
# 打开文件描述符 3 用于写入
exec 3> myfile.txt
echo "data" >&3                      # 写入到文件描述符 3
exec 3>&-                            # 关闭文件描述符 3

# 11. 输入输出同时重定向
cat < input.txt > output.txt         # 读取 input.txt，写入 output.txt
sort < unsorted.txt > sorted.txt     # 排序并保存结果

# 12. 复杂的重定向组合
{
    echo "Start processing"
    ls -la
    date
    echo "End processing"
} > full_report.txt 2>&1             # 多个命令的输出重定向到同一文件

# 13. 重定向到 /dev/null（丢弃输出）
command > /dev/null                  # 丢弃标准输出
command 2> /dev/null                 # 丢弃错误输出
command &> /dev/null                 # 丢弃所有输出

# 14. 追加到文件并同时显示在终端（tee 命令）
ls -la | tee output.txt              # 保存到文件并显示在终端
command | tee -a log.txt             # 追加模式

# 15. 实用示例
# 保存命令输出和时间戳
{ date; ls -la; } > report_$(date +%Y%m%d).txt

# 静默执行（忽略所有输出）
./noisy_script.sh &> /dev/null

# 分离正常和错误输出
./program 1> success.log 2> error.log

# 将错误转换为正常输出
command 2>&1 | grep "error"

# 同时输出到多个文件
command | tee file1.txt | tee file2.txt > file3.txt
```

## 重定向顺序说明

```bash
# 注意顺序很重要！
command > file 2>&1     # 正确：先重定向 stdout，再将 stderr 重定向到 stdout
command 2>&1 > file     # 错误：stderr 指向旧的 stdout（终端），stdout 才重定向到文件

# 等价写法
command > file 2>&1     # 传统写法
command &> file         # 现代简化写法
```

## 实现要点 (Implementation Notes)
- 解析命令行时识别重定向操作符：`>`, `>>`, `<`, `2>`, `2>&1`, `&>` 等
- 在 `fork()` 之后、`exec()` 之前，在子进程中执行重定向：
  - 使用 `open()` 打开目标文件
  - 使用 `dup2()` 复制文件描述符
  - 使用 `close()` 关闭不需要的文件描述符
- 输出重定向（`>`）：
  ```c
  int fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
  dup2(fd, STDOUT_FILENO);  // 将 stdout 重定向到文件
  close(fd);
  ```
- 追加重定向（`>>`）：
  ```c
  int fd = open("output.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
  dup2(fd, STDOUT_FILENO);
  close(fd);
  ```
- 输入重定向（`<`）：
  ```c
  int fd = open("input.txt", O_RDONLY);
  dup2(fd, STDIN_FILENO);  // 将 stdin 重定向到文件
  close(fd);
  ```
- 错误重定向（`2>&1`）：
  ```c
  dup2(STDOUT_FILENO, STDERR_FILENO);  // 将 stderr 指向 stdout
  ```
- 处理多个重定向时注意顺序
- 错误处理：检查文件打开失败、权限不足等情况

# 7. 通信管道建立 (Pipeline)

## 详细描述 (Detailed Description)
管道（Pipeline）是 Unix/Linux 系统中最强大的特性之一，它允许将多个命令连接起来，使得一个命令的输出直接成为下一个命令的输入。这种机制体现了 Unix "做一件事并做好"的哲学，通过组合简单的工具完成复杂的任务。

核心概念：
1. **管道符 `|`**: 连接两个命令，左侧命令的 stdout 连接到右侧命令的 stdin
2. **数据流**: 数据在管道中流动，不需要中间文件
3. **并发执行**: 管道两端的命令同时运行，不是顺序执行
4. **缓冲区**: 内核提供管道缓冲区，默认通常为 64KB

工作原理：
- Shell 为管道中的每个命令创建子进程
- 使用 `pipe()` 系统调用创建管道（一对文件描述符）
- 左侧命令的 stdout 连接到管道的写端
- 右侧命令的 stdin 连接到管道的读端
- 两个进程并发执行，通过管道传递数据

## 示例代码 (Example Code)

```bash
# 1. 基本管道使用
cat /etc/passwd | grep "root"        # 过滤包含 "root" 的行
ls -l | wc -l                        # 统计文件数量
ps aux | grep "python"               # 查找 python 进程

# 2. 多级管道（3个或更多命令）
cat /var/log/syslog | grep "error" | wc -l        # 统计错误行数
ps -ef | grep "nginx" | grep -v "grep" | wc -l    # 统计 nginx 进程数
ls -l | awk '{print $5, $9}' | sort -n            # 提取文件大小并排序

# 3. 数据处理管道
# 查找、排序、去重
cat names.txt | sort | uniq

# 统计最常用的命令
history | awk '{print $2}' | sort | uniq -c | sort -rn | head -10

# 4. 文本处理管道
# 查找并统计特定类型的文件
find . -name "*.c" | wc -l

# 提取特定字段并统计
cat access.log | cut -d' ' -f1 | sort | uniq -c | sort -rn

# 5. 系统监控管道
# 查找占用 CPU 最多的进程
ps aux | sort -nrk 3 | head -5

# 查找占用内存最多的进程
ps aux | sort -nrk 4 | head -5

# 实时监控文件变化
tail -f /var/log/syslog | grep "error"

# 6. 数据过滤管道
# 查找大文件
ls -lh | grep "^-" | awk '{if($5+0 > 100) print $9, $5}'

# 提取 IP 地址并统计
cat access.log | grep -oE "[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}" | sort | uniq -c

# 7. 管道与重定向结合
# 过滤后保存结果
cat data.txt | grep "pattern" | sort > result.txt

# 处理输入并追加到文件
cat input.txt | tr '[:lower:]' '[:upper:]' >> output.txt

# 8. 复杂管道示例
# 分析网络连接
netstat -an | grep "ESTABLISHED" | awk '{print $5}' | cut -d: -f1 | sort | uniq -c | sort -rn

# 磁盘使用分析
du -sh * | sort -h | tail -10

# 查找并统计代码行数
find . -name "*.c" -o -name "*.h" | xargs cat | wc -l

# 9. 使用 tee 进行分流
# 同时保存到文件和继续管道处理
ls -l | tee filelist.txt | grep "^d"

# 多次分流
command | tee output1.txt | process1 | tee output2.txt | process2

# 10. 管道与命令替换
# 统计特定进程的数量
echo "Running processes: $(ps aux | grep python | grep -v grep | wc -l)"

# 11. 错误处理管道
# 将错误也通过管道传递
command 2>&1 | grep "error"

# 分别处理输出和错误
{ command 2>&1 1>&3 | grep "error"; } 3>&1

# 12. 实用管道组合
# CSV 数据处理
cat data.csv | cut -d',' -f2 | tail -n +2 | sort -n | head -10

# 日志分析：提取时间戳和错误信息
tail -n 1000 app.log | grep "ERROR" | awk '{print $1, $2, $NF}'

# 文件内容去重
cat file.txt | sort | uniq > unique.txt

# 查找目录下最大的文件
find . -type f -exec ls -lh {} \; | sort -k5 -hr | head -10

# 统计代码文件的行数
find . -name "*.py" | xargs wc -l | sort -n

# 网络接口流量监控
ifconfig | grep "RX packets\|TX packets"

# 13. 数据转换管道
# JSON 格式化（假设有 jq 工具）
echo '{"name":"test","value":123}' | jq '.'

# 大小写转换
echo "Hello World" | tr '[:upper:]' '[:lower:]'

# 字符替换
echo "hello world" | tr ' ' '_'

# 14. 管道的进阶用法
# 并行处理（使用命名管道）
mkfifo mypipe
command1 > mypipe &
command2 < mypipe

# 管道中的错误忽略
command1 2>/dev/null | command2

# 15. 组合多个工具的强大示例
# 分析 Web 服务器访问日志
cat access.log | \
  awk '{print $1}' | \
  sort | \
  uniq -c | \
  sort -rn | \
  head -20 | \
  awk '{print $2, $1}'

# 系统资源使用报告
ps aux | \
  awk '{sum+=$3} END {print "Total CPU:", sum"%"}' && \
ps aux | \
  awk '{sum+=$4} END {print "Total Memory:", sum"%"}'
```

## 管道特性说明

```bash
# 1. 管道只传递 stdout，不传递 stderr
command1 | command2              # 只有 stdout 通过管道

# 2. 同时传递 stdout 和 stderr
command1 2>&1 | command2         # 错误和输出都通过管道

# 3. 管道的退出状态
# 通常返回最后一个命令的退出状态
# Bash 中可以使用 PIPESTATUS 数组查看所有命令的退出状态
echo ${PIPESTATUS[@]}            # 显示管道中所有命令的退出状态
```

## 实现要点 (Implementation Notes)
- 使用 `pipe()` 系统调用创建管道：
  ```c
  int pipefd[2];
  pipe(pipefd);  // pipefd[0] 是读端，pipefd[1] 是写端
  ```
- 对于每个管道，创建两个子进程（左右命令）
- 左侧命令（写入端）：
  ```c
  dup2(pipefd[1], STDOUT_FILENO);  // stdout 连接到管道写端
  close(pipefd[0]);                // 关闭读端
  close(pipefd[1]);                // 关闭原始写端描述符
  ```
- 右侧命令（读取端）：
  ```c
  dup2(pipefd[0], STDIN_FILENO);   // stdin 连接到管道读端
  close(pipefd[0]);                // 关闭原始读端描述符
  close(pipefd[1]);                // 关闭写端
  ```
- 父进程需要关闭所有管道描述符
- 对于多级管道（如 A | B | C），需要创建多个管道，递归或循环处理
- 正确处理文件描述符的关闭，避免死锁或资源泄漏
- 需要等待所有子进程结束
- 实现示例伪代码：
  ```c
  // 对于命令 cmd1 | cmd2
  int pipefd[2];
  pipe(pipefd);
  
  if (fork() == 0) {  // 子进程 1
      dup2(pipefd[1], STDOUT_FILENO);
      close(pipefd[0]);
      close(pipefd[1]);
      exec(cmd1);
  }
  
  if (fork() == 0) {  // 子进程 2
      dup2(pipefd[0], STDIN_FILENO);
      close(pipefd[0]);
      close(pipefd[1]);
      exec(cmd2);
  }
  
  close(pipefd[0]);
  close(pipefd[1]);
  wait(NULL);
  wait(NULL);
  ```
