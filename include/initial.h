#ifndef INITIAL_H
#define INITIAL_H

#include "types.h"

extern int process_count;
extern int soft_block_count;
extern struct clist readyqueue;
extern struct pcb_t *current_process ;
extern int device_sem[MAX_DEVICES];
extern int flag;
extern int pseudoclockflag;
extern cputime_t kernel_start;
extern unsigned int counter; 

#endif
