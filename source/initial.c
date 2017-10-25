#include "../include/pcb.h"
#include "../include/asl.h"
#include "../include/const.h"
#include "../include/clist.h"
#include "../include/types.h"

#include <uARMconst.h>
#include <uARMtypes.h>

#include "../include/p2test.h"
#include "../include/scheduler.h"
#include "../include/exceptions.h"
#include "../include/interrupts.h"
#include "../include/syscall.h"


int process_count; /* number of active processes in the system */
int soft_block_count; /* number of blocked processes */
struct clist readyqueue = CLIST_INIT; /* contains processes ready to be executed */
struct pcb_t *current_process ; /* pointer to process currently in execution */
int device_sem[MAX_DEVICES]; /* array of device semaphores */
int flag; /* used to set scheduler mode */
int pseudoclockflag; /* indicates if a pseudoclock interrupt has arrived */
cputime_t kernel_start; /* TOD when a process starts execution in the nucleus */
unsigned int counter = 0; /* counting variable for pid creation */

extern void test();

void main(void){

	state_t *new_areas[4];
	struct pcb_t *first_proc;
	int i = 0;

	/* 1. populate the 4 new areas: */
	new_areas[0] = (state_t*)INT_NEWAREA;
	new_areas[0]->pc = (memaddr)interrupt_handler; 

	new_areas[1] = (state_t*)TLB_NEWAREA;
	new_areas[1]->pc = (memaddr)tlb_handler; 

	new_areas[2] = (state_t*)PGMTRAP_NEWAREA;
	new_areas[2]->pc = (memaddr)pgmtrap_handler;

	new_areas[3] = (state_t*)SYSBK_NEWAREA;
	new_areas[3]->pc = (memaddr)syscall_handler;


	for(i = 0; i < 4; i++){
		  new_areas[i]->sp = RAM_TOP;
	 	  new_areas[i]->cpsr = STATUS_NULL;
      new_areas[i]->cpsr = new_areas[i]->cpsr | STATUS_SYS_MODE;
      new_areas[i]->cpsr = STATUS_ALL_INT_DISABLE(new_areas[i]->cpsr); 
	}


	/* 2. initialize level 2 data structures */
	initPcbs();
	initASL();

	/* 3. initialize all nucleus maintained variables */
	process_count = 0;
	soft_block_count = 0;
	current_process = NULL;

	/* 4. initialize all nucleus maintained semaphores, including the one for pseudoclock */
	for(i = 0; i < MAX_DEVICES; i++) 
    	device_sem[i] = 0;			


    /* 5. create the very first process */
    if (first_proc = allocPcb()) {

    	/* update number of processes in the system */
    	process_count++;
      /* assign id */
    	first_proc->pid = pid_generator(first_proc->pid, counter);
    	counter++;

    	/* virtual memory off */
    	(first_proc->p_s).CP15_Control = CP15_DISABLE_VM((first_proc->p_s).CP15_Control);

    	(first_proc->p_s).cpsr = STATUS_NULL;

  		/* all interrupts enabled */
  		(first_proc->p_s).cpsr = STATUS_ALL_INT_ENABLE((first_proc->p_s).cpsr);
    	
    	/* interval timer enabled */
    	(first_proc->p_s).cpsr = STATUS_ENABLE_TIMER((first_proc->p_s).cpsr);

      /* kernel mode on */
  		(first_proc->p_s).cpsr = (first_proc->p_s).cpsr | STATUS_SYS_MODE;

    	/* set stack pointer to RAMTOP-FRAMESIZE */
  		(first_proc->p_s).sp = RAM_TOP - FRAMESIZE;

    	/* set program counter to test address */
  		(first_proc->p_s).pc = (memaddr)test;

      /* first process ready to be scheduled */
	   	insertProcQ(&readyqueue, first_proc);
      
      /* initialize scheduler execution mode */
      flag = START;
      
      pseudoclockflag = FALSE;
      kernel_start = 0;
	   	scheduler(); 

    }

    else
    	PANIC();
}

