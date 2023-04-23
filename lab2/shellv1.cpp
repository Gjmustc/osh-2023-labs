#include <iostream>
#include <string>
#include <vector>
#include <sstream>
// PATH_MAX 等常量
#include <climits>
// POSIX API
#include <unistd.h>
// wait
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <fstream>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <algorithm>
#define BUF_SIZE 256
#define ERROR -999
std::vector<std::string> split(std::string s, const std::string &delimiter);
// 经典的 cpp string split 实现
// https://stackoverflow.com/a/14266139/11691878
std::vector<std::string> split(std::string s, const std::string &delimiter)
{
    std::vector<std::string> res;
    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos)
    {
        token = s.substr(0, pos);
        res.push_back(token);
        s = s.substr(pos + delimiter.length());
    }
    res.push_back(s);
    return res;
}
int cmd_exit_builtin(std::vector<std::string> args)
{
    if (args.size() <= 1)
    {
        return 0;
    }
    // std::string 转 int
    std::stringstream code_stream(args[1]);
    int code = 0;
    code_stream >> code;
    // 转换失败
    if (!code_stream.eof() || code_stream.fail())
    {
        std::cout << "Invalid exit code\n";
        return -1;
    }
    return code;
}
std::string getpwd()
{
    char *buffer = getcwd(NULL,0);
    if(buffer){
        std::string path = buffer;
        free(buffer);
        return path;
    }
    return "";
}
int cmd_pwd_builtin(std::vector<std::string> args)
{
    std::cout << getpwd() <<"\n";
}
int cmd_cd_builtin(char **arg_ptrs)
{
    if (arg_ptrs[1] == nullptr)
    {
        char *userhome;
        userhome = getenv("HOME");
        strcpy(arg_ptrs[1], userhome);
    }
    if (!chdir(arg_ptrs[1]))
    {
        char *dir = (char *)malloc(sizeof(char) * PATH_MAX);
        if (getcwd(dir, 0))
        {
           // std::cout << dir;
            return 0;
        }
    }
    else
    {
        std::cout << "cd chdir() error!\n";
        return -1;
    }
}

int cmd_pipe(char **arg_ptrs, int pipe_num, int background)
{
    static pid_t pid_pipe = fork();
    if (pid_pipe < 0)
    {
        std::cout << "fork() error\n";
        return -1;
    }
    else if (pid_pipe == 0) // 进入子进程
    {
        char *sharespace = "sharespace.txt"; // 共享文件
        int i;
        for (i = 0; i < pipe_num; i++)
        {
            pid_t pid_tmp = fork();
            if (pid_tmp < 0)
            {
                std::cout << "fork() error\n";
            }
            else if (pid_tmp == 0)
            {
                if (i)
                {
                    if (close(0))
                        std::cout << "close() error!\n";
                    open(sharespace, O_RDONLY); // 输入重定向
                }
                close(1);
                remove(sharespace);                                   // 由于当前sharespace文件正在open中，会等到解引用后才删除文件                                         // 由于当前sharespace文件正在open中，会等到解引用后才删除文件
                open(sharespace, O_WRONLY | O_CREAT | O_TRUNC, 0666); // os.open(file, flags[, mode]);
                if (i == 0)                                           // 第一条命令ls无参数
                {
                    if (execvp(arg_ptrs[0], NULL) == -1)
                        std::cout << "execvp() error!\n";
                    i++;
                    continue;
                }
                else if (execvp(arg_ptrs[i], arg_ptrs + (i + 1)) == -1) // 默认除了第一个命令（如ls），其后管道里每个子命令有且只有一个参数
                    std::cout << "execvp() error!\n";
                i += 2;
            }
            else
            {
                waitpid(pid_tmp, NULL, 0);
            }
        }
        // 接下来需要执行管道的最后一条命令（可能接重定向输出）
        close(0);
        open(sharespace, O_RDONLY); // 输入重定向
        if (strcmp(arg_ptrs[i + 2], ">") == 0)
        {
            close(1);
            int fdout = open(arg_ptrs[i + 3], O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
            if (dup2(fdout, STDOUT_FILENO) < 0)
                std::cerr << "Error: Failed to redirect stdout" << std::endl;
        }
        if (strcmp(arg_ptrs[i + 2], ">>") == 0)
        {
            close(1);
            int fdout = open(arg_ptrs[i + 3], O_RDWR | O_CREAT | O_APPEND, S_IRWXU);
            if (dup2(fdout, STDOUT_FILENO) < 0)
                std::cerr << "Error: Failed to redirect stdout" << std::endl;
        }
        if (execvp(arg_ptrs[i], arg_ptrs + (i + 1)) == -1) // 默认每个子命令只有一个参数
            std::cout << "execvp() error!\n";
        return 0;
    }
    else
    {
        if (!background)
        {
            waitpid(pid_pipe, NULL, 0);
            return 0;
        }
    }
}
int cmd_ls(char **arg_ptrs, int background) // 单命令ls重定向
{
    pid_t pid = fork();
    if (pid < 0)
    {
        std::cout << "fork() error\n";
        return -1;
    }
    else if (pid == 0)
    {
        if (strcmp(arg_ptrs[1], ">") == 0)
        {
            int fd = open(arg_ptrs[2], O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
            close(STDOUT_FILENO);
            dup2(fd, STDOUT_FILENO);
        }
        if (strcmp(arg_ptrs[1], ">>") == 0)
        {
            int fd = open(arg_ptrs[2], O_RDWR | O_CREAT | O_APPEND, S_IRWXU);
            close(STDOUT_FILENO);
            dup2(fd, STDOUT_FILENO);
        }
        if (execvp(arg_ptrs[0], NULL) == -1) // 默认每个子命令只有一个参数
            std::cout << "execvp() error!\n";
        return 0;
    }
    else
    {
        if (!background)
        {
            waitpid(pid, NULL, 0); // 进程回收程序
            return 0;
        }
    }
}

int cmd_cat(char **arg_ptrs, int background) // 单命令ls重定向
{
    pid_t pid = fork();
    if (pid < 0)
    {
        std::cout << "fork() error\n";
        return -1;
    }
    else if (pid == 0)
    {
        if (strcmp(arg_ptrs[1], "<") == 0)
        {
            int fd = open(arg_ptrs[2], O_RDONLY, 0644);
            if (fd < 0)
                std::cout << "openfile fail!\n";
            close(STDIN_FILENO);
            dup2(fd, STDIN_FILENO);
        }
        if (execvp(arg_ptrs[0], NULL) == -1) // 默认每个子命令只有一个参数
            std::cout << "execvp() error!\n";
        return 0;
    }
    else
    {
        if (!background)
        {
            waitpid(pid, NULL, 0); // 进程回收程序
            return 0;
        }
    }
}
int cmd_fg(std::vector<std::string> args)
{
    if (!args[1].empty()) // 提供进程id
    {
        pid_t cur_pid = tcgetpgrp(stoi(args[1]));
        printf("current front pid: %d\n", cur_pid);
        int result = tcsetpgrp(STDIN_FILENO, stoi(args[1]));
        signal(SIGTTOU, SIG_IGN);
        if (result == -1)
            std::cout << "tcsetpgrp() error!\n";
        return 0;
    }
    else // 不提供进程id，对最近的后台程序操作
    {
    }
}
int cmd_bg(std::vector<std::string> args)
{
}
static pid_t pid0;       // 定义成静态变量，是为了能在handler中作为区分父/子进程的标志
void handler(int signum) // 在不同进程下对ctrl+c的处理程序
{
    printf("\npid0 = %d\n", pid0);
    if (pid0 == 0)
    {
        fflush(STDIN_FILENO);
        sleep(5);
        exit(3); // 退出状态设为3,表示是从shell的子进程返回shell父进程，不是要退出shell本身
    }
    else
        printf("\n");
}
int main() // myshell
{
    // 不同步 iostream 和 cstdio 的 buffer
    std::ios::sync_with_stdio(false);
    // 用来存储读入的一行命令
    std::string cmd;
    int background = 0;          // 判断是否末尾加‘&’变为后台程序
    int status_main, status_sub; // 定义为全局变量
    signal(SIGINT, handler);     // 对父进程shell的ctrl+c处理

    while (true) // shell进程持续存在
    {
        // 打印提示符
        std::cout << getpwd() <<"# ";

        // 读入一行。std::getline 结果不包含换行符。
        std::getline(std::cin, cmd);
        // 按空格分割命令为单词，逐一压栈存储
        std::vector<std::string> args = split(cmd, " ");
        if (args[args.size() - 1] == "&")
        {
            background = 1;
        }
        int pipe_num = 0; // 对管道数进行计数后删除
        for (unsigned i = 0; i < args.size(); i++)
        {
            if (args[i].compare("|") == 0)
                pipe_num++;
        }
        std::vector<std::string>::iterator newpos = remove(args.begin(), args.end(), "|");
        args.erase(newpos, args.end());
        //  std::vector<std::string> 转 char ** // （方便查找特定字符）
        char **arg_ptrs = (char **)malloc(sizeof(char **) * (args.size() + 1));
        if (!arg_ptrs)
        {
            printf("Malloc fail!\n");
            exit(1);
        }
        // char *arg_ptrs[args.size() + 1];
        int cmd_len = args.size();
        for (int i = 0; i < cmd_len; i++)
        {
            arg_ptrs[i] = &args[i][0];
        }
        // exec p 系列的 argv 需要以 nullptr 结尾
        arg_ptrs[args.size()] = nullptr;

        //printf("1\n");
        // 没有可处理的命令，不作任何操作，shell进程继续
        if (args.empty())
        {
            continue;
        }
        if (args[0] == "exit")
        {
            //printf("2\n");
            if (cmd_exit_builtin(args))
                exit(ERROR);
            break;
        }
        if (args[0] == "pwd")
        {
            //printf("3\n");
            if (cmd_pwd_builtin(args))
                exit(ERROR);
        }
        if (args[0] == "cd")
        {
            //printf("4\n");
            if (cmd_cd_builtin(arg_ptrs))
                exit(ERROR);
        }
        if (args[0] == "wait")
        {
            //printf("5\n");
            while (wait(NULL) != -1)
                continue;
        }
        
        pid0 = fork();
        if (pid0 == 0) // shell的子进程
        {
            //printf("6\n");
            signal(SIGINT, handler);       // 对子进程ctrl+c处理
            waitpid(pid0, &status_sub, 0); // 等待子进程运行外部命令结束后再继续shell程序
            if (pipe_num)
            {
                //printf("7\n");
                cmd_pipe(arg_ptrs, pipe_num, background);
            }
            else if (args[0] == "ls")
            {
                //printf("8\n");
                cmd_ls(arg_ptrs, background);
            }
            else if (args[0] == "cat")
            {
                //printf("9\n");
                cmd_cat(arg_ptrs, background);
            }
            if (args[0] == "fg")
            {
                //printf("10\n");
                cmd_fg(args);
            }
        }
        else if (pid0 > 0) // 父进程
        {
            //printf("11\n");
            int ret = waitpid(pid0, &status_main, WNOHANG);
            if (ret == -1)
                std::cout << "No such subpids!\n";
            else if (ret == 0) // 说明还有子进程存在
                continue;
            else if (ret > 0)
            {
                //printf("12\n");
                if (WIFEXITED(status_main)) // 是不是正常退出
                    std::cout << "wrong: WIFEXITED(status_main)";
                if (WIFSIGNALED(status_main)) // 是不是异常终止
                    std::cout << "wrong: WIFSIGNALED(status_main)";
                if (status_main == 0)
                    exit(0); // status为0表明用户在shell(实际是shell子进程)中输入了exit，说明用户实际想退出shell程序
            }
        }
    }
}
