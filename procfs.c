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
#define INUMPROC  	10000
#define INUMPID  	20000
#define INUMFILE  	30000
#define DIRENTSIZE	500

enum pidfiles { CMDLINE, CWD, EXE, FDINFO, STATUS };

extern struct inode* iget2(uint, uint);

static struct inode procfs[NPROC];
static struct inode pidinodes[NPROC][5];
static struct dirent dirents[DIRENTSIZE];

void
getFileName(int inum, char* name) {
	int i;

	for (i = 0; i < DIRENTSIZE; i++) {
		if (dirents[i].inum == inum) {
			strncpy(name, dirents[i].name, DIRSIZ);
			break;
		}
	}
}

void
getPidName(int pid, char* buf) {
	int i = 0;
	int tempPid = pid;

	for (i = 0; i < DIRSIZ; i++) {
		buf[i] = 0;
	}

	i = 0;

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
	int i, j, pid;
	struct inode* ip2;
	char exepath[100];

	for (i = 0; i < NPROC; i++) {
		if (procfs[i].inum == ip->inum) {
			ip->dev 	= procfs[i].dev;
			ip->flags 	= procfs[i].flags;
			ip->type 	= procfs[i].type;
			ip->major 	= procfs[i].major;
			ip->minor 	= procfs[i].minor;
			ip->size 	= procfs[i].size;

			return;
		}

		for (j = 0; j < 5; j++) {
			if (pidinodes[i][j].inum == ip->inum) {
				switch(j) {
					case CWD:
						pid = (ip->inum - INUMPID)/10;
						ip2 = iget2(ROOTDEV, getcwd(pid));
						
						ip->inum 	= ip2->inum;
						ip->dev 	= ip2->dev;
						ip->flags 	= ip2->flags;
						ip->ref		= ip2->ref;
						ip->type 	= ip2->type;
						ip->major 	= ip2->major;
						ip->minor 	= ip2->minor;
						ip->size 	= ip2->size;
						memmove(ip->addrs, ip2->addrs, NDIRECT+1);

						break;

					case EXE:
						pid = (ip->inum - INUMPID)/10;
						getexe(pid, exepath);
						ip2 = namei(exepath);

						ip->inum 	= ip2->inum;
						ip->dev 	= ip2->dev;
						ip->flags 	= ip2->flags;
						ip->ref		= ip2->ref;
						ip->type 	= ip2->type;
						ip->major 	= ip2->major;
						ip->minor 	= ip2->minor;
						ip->size 	= ip2->size;
						memmove(ip->addrs, ip2->addrs, NDIRECT+1);

						break;

					default:
						ip->dev 	= pidinodes[i][j].dev;
						ip->flags 	= pidinodes[i][j].flags;
						ip->type 	= pidinodes[i][j].type;
						ip->major 	= pidinodes[i][j].major;
						ip->minor 	= pidinodes[i][j].minor;
						ip->size 	= pidinodes[i][j].size;
				}

				return;
			
			}
		}
	}
}

int
procfsread(struct inode *ip, char *dst, int off, int n) {
	int pids[NPROC];
	int i;
	int nonZeroPids = 0;
	int j;
	int ret = 0;
	int direntindex = 0;
	char cmdline_str[14] 	= "cmdline";
	char cwd_str[14] 		= "cwd";
	char exe_str[14] 		= "exe";
	char fdinfo_str[14]		= "fdinfo";
	char status_str[14] 	= "status";
	char filename[14];
	char file[500];
	int pid;
	int filelength;

	n = sizeof(struct dirent);
	if (ip->minor == PROCDIR) {
		cleanInodes();
		getpids(pids);

		for (i = 0; i < NPROC; i++) {
			procfs[i].dev = PROCFS;
			procfs[i].inum = pids[i] + INUMPROC;
			procfs[i].flags = 0 | I_VALID;
			procfs[i].type = T_DEV;
			procfs[i].major = PROCFS;
			procfs[i].minor = PIDDIR;
			procfs[i].size = 5 * n;
			procfs[i].nlink = 0;

			if (pids[i] != 0)
				nonZeroPids++;
		}
		ip->nlink++;

		i = off / n;

		if (i < nonZeroPids) {
			((struct dirent*)dst)->inum = procfs[i].inum;
			getPidName(pids[i], ((struct dirent*)dst)->name);
			ret = n;
		}
	} else if (ip->minor == PIDDIR) {
		for (i = 0; i < NPROC; i++) {
			pid = procfs[i].inum - INUMPROC;
			if (ip->inum == procfs[i].inum) {
				for (j = 0; j < 5; j++) {
					pidinodes[i][j].dev = PROCFS;
					pidinodes[i][j].inum = pid*10 + INUMPID + j;
					pidinodes[i][j].flags = 0 | I_VALID;
					pidinodes[i][j].type = T_DEV;
					pidinodes[i][j].major = PROCFS;
					pidinodes[i][j].minor = FILE;
					pidinodes[i][j].size = 5 * n;
					pidinodes[i][j].nlink = 0;
				}

				ip->nlink++;
				j = off / n;

				if (j < 5) {
					((struct dirent*)dst)->inum = pidinodes[i][j].inum;

					switch(j) {
						case CMDLINE:
							strncpy(((struct dirent*)dst)->name, cmdline_str, DIRSIZ);
							break;
						case CWD:
							strncpy(((struct dirent*)dst)->name, cwd_str, DIRSIZ);
							break;
						case EXE:
							strncpy(((struct dirent*)dst)->name, exe_str, DIRSIZ);
							break;
						case FDINFO:
							strncpy(((struct dirent*)dst)->name, fdinfo_str, DIRSIZ);
							break;
						case STATUS:
							strncpy(((struct dirent*)dst)->name, status_str, DIRSIZ);
					}

					dirents[direntindex].inum = ((struct dirent*)dst)->inum;
					strncpy(dirents[direntindex].name, ((struct dirent*)dst)->name, DIRSIZ);

					ret = n;
				}
			}
		}
	} else if (ip->minor == FILE) {
		getFileName(ip->inum, filename);
		pid = (ip->inum - INUMPID)/10;

		if (strncmp(filename, cmdline_str, DIRSIZ) == 0) {
			getcmdline(pid, file);

			filelength = strlen(file);
			if (off < filelength) {
				strncpy(dst, file+off, n);
				ret = n;
			}
		} else if (strncmp(filename, cwd_str, DIRSIZ) == 0) {

			((struct dirent*)dst)->inum = ip->inum;
			strncpy(((struct dirent*)dst)->name, cwd_str, DIRSIZ);

		} else if (strncmp(filename, exe_str, DIRSIZ) == 0) {

			((struct dirent*)dst)->inum = ip->inum;
			strncpy(((struct dirent*)dst)->name, exe_str, DIRSIZ);

		} else if (strncmp(filename, fdinfo_str, DIRSIZ) == 0) {
			
		} else if (strncmp(filename, status_str, DIRSIZ) == 0) {
			getstatus(pid, file);

			filelength = strlen(file);
			if (off < filelength) {
				strncpy(dst, file+off, n);
				ret = n;
			}
		}
	}

  	return (ret);
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
