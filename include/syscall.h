#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "types.h"
#include "const.h"

extern unsigned int pid_generator(unsigned int x, unsigned int y);
extern int create_process(state_t *statep);
extern void terminate_process(pid_t pid);
extern void terminate_children(struct pcb_t* p, struct pcb_t *p_const);
extern void lookfor_progeny(struct pcb_t *parent, pid_t pid, int *found);
extern void semaphore_operation(int *semaddr, int weight);
extern void specify_sysbp(memaddr pc, memaddr sp, unsigned int flags);
extern void specify_tlb(memaddr pc, memaddr sp, unsigned int flags); 
extern void specify_pgmtrap(memaddr pc, memaddr sp, unsigned int flags);
extern void exit_trap(unsigned int excptype, unsigned int retval);
extern void get_cputime(cputime_t *global, cputime_t *user);
extern void wait_clock();
extern unsigned int io_devop(unsigned int command, int intlNo, unsigned int dnum);
extern pid_t getpid();

#endif
