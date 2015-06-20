#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_cmd(void)
{
  char *cmd;

  if (argptr(0, (char**)&cmd, sizeof(char)) < 0){
    return -1;
  }

  cmdline(cmd);
  return 0;
}

int
sys_exelink(void)
{
  char *link;

  if (argptr(0, (char**)&link, sizeof(char)) < 0){
    return -1;
  }

  exelink(link);
  return 0;
}

int
sys_getpids(void)
{
  int *pids;

  if (argptr(0, (char**)&pids, sizeof(char)) < 0){
    return -1;
  }

  getpids(pids);
  return 0;
}

int
sys_getcmdline(void)
{
  int pid;
  char *cmd;

  if(argint(0, &pid) < 0)
    return -1;

  if (argptr(1, (char**)&cmd, sizeof(char)) < 0){
    return -1;
  }

  getcmdline(pid, cmd);
  return 0;
}

int
sys_getexe(void)
{
  int pid;
  char *exe;

  if(argint(0, &pid) < 0)
    return -1;

  if (argptr(1, (char**)&exe, sizeof(char)) < 0){
    return -1;
  }

  getexe(pid, exe);
  return 0;
}

int
sys_getcwd(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;

  return getcwd(pid);
}

int
sys_getstatus(void)
{
  int pid;
  char *status;

  if(argint(0, &pid) < 0)
    return -1;

  if (argptr(1, (char**)&status, sizeof(char)) < 0){
    return -1;
  }

  getstatus(pid, status);
  return 0;
}

int
sys_getfdinfo(void)
{
  int pid;
  int fd;
  char *info;

  if(argint(0, &pid) < 0)
    return -1;

  if(argint(1, &fd) < 0)
    return -1;

  if (argptr(2, (char**)&info, sizeof(char)) < 0){
    return -1;
  }

  getfdinfo(pid, fd, info);
  return 0;
}

int
sys_getfds(void)
{
  int pid;
  int *fds;

  if(argint(0, &pid) < 0)
    return -1;

  if (argptr(1, (char**)&fds, sizeof(char)) < 0){
    return -1;
  }

  getfds(pid, fds);
  return 0;
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;
  
  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;
  
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
