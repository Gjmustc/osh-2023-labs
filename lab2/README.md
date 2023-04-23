## shell 编写说明

大致尝试实现功能：
- pwd 命令用于打印当前工作目录
- cd 用于更改当前工作目录为指定目录（cd 在没有第二个参数时，默认进入家目录）
- 多管道语法（只测试了三条）
- ctrl+c、ctrl+d 信号处理

测试过程中会遇到的与期望不符之处（bug）：
- 在进入shell后若第一条命令直接输入ctrl+c，则与ctrl+d、exit效果相同，若在输入其他命令后再输入ctrl+c则只会丢弃命令、不会退出shell
- 多管道语法（如ls | cat -n | grep 1)运行后有时会自动退出shell，原因不明
- 在执行若干命令后，可能需要两次输入ctrl+d才可退出shell

> 附加几天内尝试撰写的shell不同版本,有些带printf的debug输出（makefile只能编译最新版shell.cpp)
> 非常非常非常希望助教抽空开个习题课啥的讲解一下这个shell的编写思路！（感谢.jpg）