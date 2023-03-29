#include <stdio.h>
#include <string.h>

int main() {
    printf("Hello! PB21111682\n"); // Your Student ID
    printf("Now I'm testing your new system call - hello!\n");
    char buf[40]={0};
    int len = 40;
    printf("%d\n",syscall(548, buf, len));
    puts(buf);
    memset(buf, '*', sizeof(buf));
    len -=20;
    printf("%d\n",syscall(548, buf, len));
    puts(buf);
    while (1) {}
}
