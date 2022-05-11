#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define MAXLINE 4096        //最大命令长度
#define MAXWORD 10          //最大词数
#define default_prompt "->" //默认的提示符
#define Env "PROMPT"        //环境变量
#define RWRWRW (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IROTH) //重定向输出文件权限

void SetEnv(void) //设置提示符
{
    char prompt[10]; //存放用户设置的提示符

    if (getenv(Env) == NULL) //如果该环境变量不存在，设置为默认
        setenv(Env, default_prompt, 1);

    printf("Input the PROMPT you want ('<CR>' to keep before).\n"); //读取用户输入的值
    fgets(prompt, 10, stdin);

    if (prompt[0] == '\n') //如果只是回车就不重新设置
        ;

    else
    {
        prompt[strlen(prompt) - 1] = '\0'; //有其他输入就去掉尾部回车，设置新值
        setenv(Env, prompt, 1);
    }

    printf("OK, let's go!\n");  //设置成功提示
    printf("%s ", getenv(Env)); //打印环境变量
}

void Sig_int(int sigo) //遇到ctrl+c信号正常退出
{
    printf("\nexit!\n");
    exit(0);
}

void Split_string(char *buf, char *command[10], int *reinput, int *reoutput) //分割命令语句
{
    command[0] = strtok(buf, " ");//去除命令行之前的空格
    int i = 1;
    while ((command[i] = (char *)strtok(NULL, " ")) != NULL) //判断是否有输入输出重定向，返回其在命令数组中的位置
    {
        if (strcmp(command[i], "<") == 0)
        {
            *reinput = i;
        }
        else if (strcmp(command[i], ">") == 0)
        {
            *reoutput = i;
        }
        i++;
    }
}

int main(void)
{
    char buf[MAXLINE];      //输入命令的char数组
    char *command[MAXWORD]; //存放命令分割之后的二维数组
    pid_t pid; 
    int fdin, fdout; //文件描述符
    int status;
    int reinput_flag;  //输入重定向标志
    int reoutput_flag; //输出重定向标志
    int bg_flag = 0;   //后台运行标志

    if (signal(SIGINT, Sig_int) == SIG_ERR) //捕捉信号
        printf("signal error!");

    SetEnv(); //设置环境变量

    while (fgets(buf, MAXLINE, stdin) != NULL)
    {
        int reinput_flag = 0;  //输入重定向标志
        int reoutput_flag = 0; //输出重定向标志
        if (buf[strlen(buf) - 1] == '\n')
            buf[strlen(buf) - 1] = 0;

        Split_string(buf, command, &reinput_flag, &reoutput_flag); //分割命令语句

        if (command[0] == 0) //无输入不做响应
        {
            printf("%s ", getenv(Env));
            continue;
        }

        if ((pid = fork()) < 0) // fork子进程
        {
            printf("fork error\n");
        }
        else if (pid == 0)
        {
            if (reinput_flag != 0) //判断有没有输入输出重定向
            {
                close(0);
                fdin = open(command[reinput_flag + 1], O_RDONLY);
                command[reinput_flag] = NULL; //截取execvp执行的长度
            }
            if (reoutput_flag != 0)
            {
                close(1);
                fdout = open(command[reoutput_flag + 1], O_WRONLY | O_TRUNC | O_CREAT, RWRWRW);
                command[reoutput_flag] = NULL; //截取execvp执行的长度
            }

            if (strcmp(command[0], "bg") == 0)
            {
                bg_flag = 1;
                if (setsid() < 0) //终端退出，子进程后台可以继续运行
                    printf("setsid error:%s\n", strerror(errno));
            }
            if (execvp(command[0 + bg_flag], &command[0 + bg_flag]) < 0)//执行分割好的命令
                printf("%s\n", strerror(errno));
        }

        if (strcmp(command[0], "bg") == 0) //有后台运行就不用等待
            ;
        else if ((pid = waitpid(pid, &status, 0)) < 0) //等待子进程
        {
            printf("waitpaid error\n");
            exit(0);
        }
        printf("%s ", getenv(Env)); //打印提示符
    }
    exit(0);
}
