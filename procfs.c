#include "types.h"
#include "stat.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

#define PROCDIR 	0
#define PIDDIR  	1
#define FILE 		2
#define INUMADD  	10000

static struct inode procfs[NPROC];
//static struct inode pidinodes[5];

void
getPidName(int pid, char* buf) {
	int i = 0;
	int tempPid = pid;

	while (pid != 0) {
		pid = pid / 10;
		i++;
	}
	i--;

	pid = tempPid;

	while (i >= 0) {
		buf[i] = (pid % 10) + 48;
		pid = pid / 10;
		i--;
	}
}

void
cleanInodes() {
	int i;
	for (i = 0; i < NPROC; i++) {
		procfs[i].inum = 0;
	}
}

int 
procfsisdir(struct inode *ip) {
	if (ip->minor == FILE)
  		return 0;
  	return 1;
}

void 
procfsiread(struct inode* dp, struct inode *ip) {
	/*int i;

	for (i = 0; i < NPROC; i++) {
		if (procfs[i].inum == ip->inum) {
			ip->dev = procfs[i].dev;
			ip->ref = procfs[i].ref;
			ip->flags = procfs[i].flags;
			ip->type = procfs[i].type;
			ip->major = procfs[i].major;
			ip->minor = procfs[i].minor;
			ip->size = procfs[i].size;
		}
	}*/
}

int
procfsread(struct inode *ip, char *dst, int off, int n) {
	int pids[NPROC];
	int i;
	int nonZeroPids = 0;

	/*cprintf("read off: %d\n", off);
	cprintf("size of dirent: %d\n", sizeof(struct dirent));*/

	if (ip->minor == PROCDIR) {
		cleanInodes();
		getpids(pids);
		n = sizeof(struct dirent);

		for (i = 0; i < NPROC; i++) {
			//cprintf("read pid: %d\n", pids[i]);

			procfs[i].dev = PROCFS;
			procfs[i].inum = pids[i] + INUMADD;
			procfs[i].ref = 0;
			procfs[i].flags = 0 | I_VALID;
			procfs[i].type = T_DEV;
			procfs[i].major = PROCFS;
			procfs[i].minor = PIDDIR;
			procfs[i].size = 5 * n;
			procfs[i].nlink = 0;

			if (pids[i] != 0)
				nonZeroPids++;

			//cprintf("read name: %s\n", dirents[i].name);
		}

		ip->size = nonZeroPids * n;
		ip->nlink++;
		i = off / n;
/*
		cprintf("non zero: %d\n",nonZeroPids);
		cprintf("ip size: %d\n",ip->size);
		cprintf("i: %d\n",i);
		*/
		((struct dirent*)dst)->inum = procfs[i].inum;
		getPidName(pids[i], ((struct dirent*)dst)->name);

		/*cprintf("inum: %d\n",((struct dirent*)dst)->inum);
		cprintf("pid: %d\n", pids[i]);
		cprintf("name: %s\n",((struct dirent*)dst)->name);*/
	} else if (ip->minor == PIDDIR) {

	}

  	return (off + n);
}

int
procfswrite(struct inode *ip, char *buf, int n)
{
  return 0;
}

void
procfsinit(void)
{
  devsw[PROCFS].isdir = procfsisdir;
  devsw[PROCFS].iread = procfsiread;
  devsw[PROCFS].write = procfswrite;
  devsw[PROCFS].read = procfsread;
}
