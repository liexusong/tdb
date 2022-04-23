#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/user.h>
#include <stdlib.h>
#include <stdio.h>

void load_executable_file(const char *target_file)
{
    /* 1) 运行跟踪(debug)当前进程 */
    ptrace(PTRACE_TRACEME, 0, 0, 0);
 
    /* 2) 加载并且执行被调试的程序可执行文件 */
    execl(target_file, target_file, 0);
}

void send_debug_command(pid_t debug_pid)
{
    int status;
    int counter = 0;
    struct user_regs_struct regs;
    unsigned long long instr;

    printf("Tiny debugger started...\n");
 
    /* 1) 等待被调试进程(子进程)发送信号 */
    wait(&status);
 
    while (WIFSTOPPED(status)) {
        counter++;

        /* 2) 获取当前寄存器信息 */
        ptrace(PTRACE_GETREGS, debug_pid, 0, &regs);

        /* 3) 获取 EIP 寄存器的值 */
        instr = ptrace(PTRACE_PEEKTEXT, debug_pid, regs.rip, 0);

        printf("[%u.  EIP = 0x%08llx.  instr = 0x%08llx\n",
               counter, regs.rip, instr);

        /* 4) 将被调试进程设置为单步调试 */
        ptrace(PTRACE_SINGLESTEP, debug_pid, 0, 0);
 
        /* 5) 等待被调试进程(子进程)发送信号 */
        wait(&status);
    }
 
    printf("Tiny debugger exited...\n");
}

int main(int argc, char** argv)
{
    pid_t child_pid;
 
    if (argc < 2) {
        fprintf(stderr, "Expected a program name as argument\n");
        return -1;
    }
 
    child_pid = fork();
    
    if (child_pid == 0) {               // 1) 子进程：被调试进程
        load_executable_file(argv[1]);  // 加载可执行文件
    } else if (child_pid > 0) {         // 2) 父进程：调试进程
        send_debug_command(child_pid);  // 发送调试命令
    } else {
        perror("fork");
        return -1;
    }
 
    return 0;
}
