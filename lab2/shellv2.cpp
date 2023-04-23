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
int pstatus = 0; // 全局变量控制ctrl+c退出方式
// 后台进程结构体
typedef struct bg_process
{
    int pgid; // 进程组id
    std::vector<std::string> cmd;
} bgp;
// 用于存储后台进程的 pid 和命令
std::vector<bgp> bg_processes;
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

void handler(int signum) // 在不同进程下对ctrl+c的处理程序
{
    // printf("current pstatus = %d\n", pstatus);
    fflush(STDIN_FILENO);
    // exit(3);
    //  if (pstatus == 0) // 在shell中丢弃命令
    //  {
    //      fflush(STDIN_FILENO);
    //  }
    //  else if (pstatus == 1) // 从子进程回到shell等待命令输入
    //  {
    //      exit(3); // 退出状态设为3,表示是从shell的子进程返回shell父进程，不是要退出shell本身
    //  }
}
void redirect(std::vector<std::string> cmd, int fd_in, int fd_out)
{
    for (auto i = 0; i < cmd.size(); i++)
    {
        // 输入重定向
        if (cmd[i] == "<")
        {
            if (close(STDIN_FILENO))
                std::cout << "close() error!\n";
            int fd = open(cmd[i + 1].c_str(), O_RDONLY); // 输入重定向
            dup2(fd, fd_in);
            close(fd);
        }
        // 输出重定向
        if (cmd[i] == ">")
        {
            if (close(STDOUT_FILENO))
                std::cout << "close() error!\n";
            int fd = open(cmd[i + 1].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644); // 输出重定向
            dup2(fd, fd_out);
            close(fd);
        }
        if (cmd[i] == ">>")
        {
            if (close(STDOUT_FILENO))
                std::cout << "close() error!\n";
            int fd = open(cmd[i + 1].c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644); // 输出重定向
            dup2(fd, fd_out);
            close(fd);

        }
    }
}
int cmd_pipe(std::vector<std::string> &args, int background)
{
    // 计算管道数量并分割命令
    std::vector<std::vector<std::string>> cmds; // 按管道分割的命令集（管道内部可含重定向）
    std::vector<std::string> subcmd;
    for (auto i = 0; i < args.size(); i++)
    {
        if (args[i].compare("|") == 0)
        {
            cmds.push_back(subcmd);
            subcmd.clear();
        }
        else
        {
            subcmd.push_back(args[i]);
        }
    }
    cmds.push_back(subcmd);
    /**************************************/
    int fd[2];         // 搭建管道
    int fd_in, fd_out; // 控制每个文件的输入和输出
    fd_in = dup(STDIN_FILENO);
    fd_out = dup(STDOUT_FILENO);
    if (!background)
    {
        for (auto i = 0; i < cmds.size(); i++)
        {
            // 在管道中前一个命令的输出时后一个命令的输入
            if (i == cmds.size() - 1) // 管道最末层
            {
                fd_out = STDOUT_FILENO;
            }
            else // 管道中间层
            {
                if (pipe(fd) < 0)
                {
                    std::cout << "pipe failed\n";
                    continue;
                }
                fd_out = fd[1];
            }
            // 对首尾命令重定向执行
            //if (cmds.size() == 1)
            //    redirect(cmds[i], fd_in, fd_out);
            //else if (i == 0)
            //    redirect(cmds[i], fd_in, fd_out);
            //else if (i == cmds.size() - 1)
            //    redirect(cmds[i], fd_in, fd_out);
            redirect(cmds[i], fd_in, fd_out); // 管道内单命令的重定向符处理
            //  std::vector<std::string> 转 char **
            char *argv[cmds[i].size() + 1];
            for (auto j = 0; j < cmds[i].size(); j++)
            {
                argv[j] = &cmds[i][j][0]; // 拷贝参数
            }
            argv[cmds[i].size()] = nullptr; // execp 系列的 argv 需要以 nullptr 结尾

            pid_t pid_pipe = fork();
            if (pid_pipe < 0)
            {
                std::cout << "fork() error\n";
                return -1;
            }
            else if (pid_pipe == 0) // 进入子进程
            {
                signal(SIGINT, SIG_DFL); // 对shell的ctrl+c处理
                signal(SIGQUIT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);
                signal(SIGINT, handler); // 对shell的ctrl+c处理
                // 重定向
                close(fd[0]);
                dup2(fd_in, STDIN_FILENO);
                dup2(fd_out, STDOUT_FILENO);
                close(fd[1]);
                execvp(cmds[i][0].c_str(), argv);
                // exit(1);
            }
            else if (pid_pipe > 0) // 进入父进程
            {
                // 父进程
                // 等待子进程结束
                printf("12\n");
                int status;
                int ret = waitpid(pid_pipe, &status, 0);
                if (WIFSIGNALED(status))
                    std::cout << "\nChild killed by signal " << WTERMSIG(status) << "\n";
                close(fd[1]);
                dup2(fd[0], fd_in);
                close(fd[0]);
            }
        }
        dup2(fd_in, STDIN_FILENO);
        dup2(fd_out, STDOUT_FILENO);
    }
    // 后台命令创建子进程组进行管理
    else if (background == 1)
    {
        pid_t mpid = fork();
        pid_t pgid = getpgrp();
        if (mpid == -1)
        {
            std::cout << "fork failed\n";
            return -1;
        }
        else if (mpid == 0)
        {
            setpgrp(); // 为当前子进程设置进程组id
            for (auto i = 0; i < cmds.size(); i++)
            {
                // 在管道中前一个命令的输出时后一个命令的输入
                if (i == cmds.size() - 1) // 管道最末层
                {
                    fd_out = STDOUT_FILENO;
                }
                else // 管道中间层
                {
                    if (pipe(fd) < 0)
                    {
                        std::cout << "pipe failed\n";
                        continue;
                    }
                    fd_out = fd[1];
                }
                // 对首尾命令重定向执行
                if (cmds.size() == 1)
                    redirect(cmds[i], fd_in, fd_out);
                else if (i == 0)
                    redirect(cmds[i], fd_in, fd_out);
                else if (i == cmds.size() - 1)
                    redirect(cmds[i], fd_in, fd_out);
                // redirect(cmds[i], fd_in, fd_out);
                //  std::vector<std::string> 转 char **
                char *argv[cmds[i].size() + 1];
                for (auto j = 0; j < cmds[i].size(); j++)
                {
                    argv[j] = &cmds[i][j][0];
                }
                argv[cmds[i].size()] = nullptr; // exec p 系列的 argv 需要以 nullptr 结尾

                pid_t pid_pipe;
                pid_pipe = fork();
                if (pid_pipe < 0)
                {
                    std::cout << "fork() error\n";
                    return -1;
                }
                else if (pid_pipe == 0) // 进入子进程
                {
                    // 重定向
                    close(fd[0]);
                    dup2(fd_in, STDIN_FILENO);
                    dup2(fd_out, STDOUT_FILENO);
                    close(fd[1]);
                    execvp(cmds[i][0].c_str(), argv);
                    // exit(1);
                }
                else if (pid_pipe > 0) // 进入父进程
                {
                    // 父进程
                    // 等待子进程结束
                    int status;
                    waitpid(pid_pipe, &status, 0);
                    if (WIFSIGNALED(status))
                        std::cout << "\nChild killed by signal " << WTERMSIG(status) << "\n";
                    close(fd[1]);
                    dup2(fd[0], fd_in);
                    close(fd[0]);
                    // 将父进程放入前台
                    pid_t gpid_o = getpgrp();
                    tcsetpgrp(STDIN_FILENO, gpid_o);
                }
            }
            dup2(fd_in, STDIN_FILENO);
            dup2(fd_out, STDOUT_FILENO);
        }
        else if (mpid > 0)
        {
            // 父进程
            // 将子进程放入后台
            bgp bgp_subcmd = {mpid, args};
            bg_processes.push_back(bgp_subcmd);
            std::cout << "pgid: " << pgid << " is running in background\n";
            // 等待子进程结束
            int status;
            waitpid(mpid, &status, 0);
            if (WIFSIGNALED(status))
                std::cout << "\nChild killed by signal " << WTERMSIG(status) << "\n";
        }
    }
}
int main() // myshell
{
    // 不同步 iostream 和 cstdio 的 buffer
    std::ios::sync_with_stdio(false);
    // 用来存储读入的一行命令
    std::string cmd;
    int background = 0; // 判断是否末尾加‘&’变为后台程序

    while (true) // shell进程持续存在
    {
        // 打印提示符
        std::cout << "# ";

        // 读入一行。std::getline 结果不包含换行符。
        std::getline(std::cin, cmd);
        // 按空格分割命令为单词，逐一压栈存储
        // 判断cmd是否为EOF(CTRL+D)
        if (std::cin.eof())
        {
            std::cout << "\n";
            exit(0);
        }
        std::vector<std::string> args = split(cmd, " ");
        if (args[args.size() - 1] == "&")
        {
            background = 1;
        }
        signal(SIGINT, SIG_IGN); // 对shell的ctrl+c处理
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);

        //  std::vector<std::string> 转 char ** // （方便查找特定字符）
        // char **arg_ptrs = (char **)malloc(sizeof(char **) * (args.size() + 1));
        // if (!arg_ptrs)
        // {
        //     printf("Malloc fail!\n");
        //     exit(1);
        // }
        char *arg_ptrs[args.size() + 1];
        int cmd_len = args.size();
        for (int i = 0; i < cmd_len; i++)
        {
            arg_ptrs[i] = &args[i][0];
        }
        // exec p 系列的 argv 需要以 nullptr 结尾
        arg_ptrs[args.size()] = nullptr;

        // printf("start shell!\n");
        //  没有可处理的命令，不作任何操作，shell进程继续
        if (args.empty())
        {
            continue;
        }
        if (args[0] == "exit")
        {
            if (args.size() <= 1)
            {
                exit(0);
            }
            // std::string 转 int
            std::stringstream code_stream(args[1]);
            int code = 0;
            code_stream >> code;
            // 转换失败
            if (!code_stream.eof() || code_stream.fail())
            {
                std::cout << "Invalid exit code\n";
                exit(-1);
            }
            exit(code);
        }
        if (args[0] == "pwd")
        {
            printf("3\n");
            char *wkdir = (char *)malloc(sizeof(char) * PATH_MAX);
            if (!wkdir)
            {
                printf("Malloc fail!\n");
            }
            if (getcwd(wkdir, PATH_MAX) == nullptr)
            {
                printf("pwd getcwd() error!\n");
            }
            else
            {
                puts(wkdir);
                free(wkdir);
            }
            continue;
        }
        if (args[0] == "cd")
        {
            printf("4\n");
            // 没有参数默认为home
            const char *dir = (args.size() <= 1 || args[1] == "~" || args[1] == "~/") ? getenv("HOME") : args[1].c_str();
            if (chdir(dir) != 0)
                std::cout << "No such file or directory\n";
            char *wkdir = (char *)malloc(sizeof(char) * PATH_MAX);
            getcwd(wkdir, PATH_MAX);
            puts(wkdir);
            free(wkdir);
            continue;
        }
        if (args[0] == "wait") // 等待所有后台进程终止
        {
            printf("5\n");
            // 如果没有后台程序
            if (bg_processes.empty())
            {
                std::cout << "No background process\n";
                continue;
            }
            else
            {
                for (auto &bpid : bg_processes)
                {
                    int status;
                    waitpid(bpid.pgid, &status, 0);
                }
            }
            bg_processes.clear();
            continue;
        }
        cmd_pipe(args, background);
    }
    return 0;
}
