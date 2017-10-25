#include "../include/initial.h"
#include "../include/clist.h"
#include "../include/interrupts.h"
#include "../include/pcb.h"
#include "../include/types.h"
#include "../include/const.h"
#include <libuarm.h>

cputime_t startTOD; /* stores getTODLO() when a process starts execution */
cputime_t pseudo_start; /* stores getTODLO() after a pseudoclock tick */

void deadlock_detection();
void loadnext();

void scheduler(){

	setSTATUS(STATUS_ALL_INT_DISABLE(getSTATUS()));

	/* choosing scheduler execution mode according to flag value */ 
	/* 1. loading the first process */
	if (flag == START){

		setTIMER(SCHED_TIME_SLICE);
		pseudoclockflag = FALSE;
		pseudo_start = getTODLO();
		loadnext();
	}	

	/* 2. if 100 ms are expired, resetting pseudoclock variables */
	else if (flag == RESET){

		pseudoclockflag = FALSE;	
		pseudo_start = getTODLO();
		loadnext();
	}

	/* 3. loading next ready process */
	else if (flag == LOADNEXT)
		loadnext();

	/* 4. loading current process */
	else if (flag == LOADCURRENT) {

		(current_process->p_s).cpsr = STATUS_ALL_INT_ENABLE((current_process->p_s).cpsr);
		LDST(&(current_process->p_s));

	}

}

/* function that performs a simple deadlock detection */
void deadlock_detection(){

			if (process_count == 0)
				HALT();

			else if ((soft_block_count == 0) && (process_count > 0))
				PANIC();

			else if ((soft_block_count > 0) && (process_count > 0)){
				/* enable interrupts and wait for them */
				setTIMER(SCHED_TIME_SLICE);
				setSTATUS(STATUS_ALL_INT_ENABLE(getSTATUS()));
				WAIT(); 	
			}
}

void loadnext(){

	if(current_process != NULL){

		/* update time and insert in the readyQ*/
		current_process->global_time += getTODLO() - startTOD;
		insertProcQ(&readyqueue, current_process);
		current_process = NULL;
	}
	/* check if 100 ms (approx.) are going to expire */
	if ((getTODLO() - pseudo_start) > (SCHED_PSEUDO_CLOCK - SCHED_TIME_SLICE)){
		/* set remaining time to pseudoclock tick*/
		setTIMER(SCHED_PSEUDO_CLOCK - (getTODLO() - pseudo_start));
		/* to be sure the timer handler will treat the next timer interrupt as a pseudoclock tick */
		pseudoclockflag = TRUE;
		pseudo_start = getTODLO();
	}
	/* setting interval timer value */
	else setTIMER(SCHED_TIME_SLICE);

	/* extract a new ready process, if the readyqueue is empty check if the system is in deadlock */
	if((current_process = removeProcQ(&readyqueue)) == NULL)
		deadlock_detection();

	startTOD = getTODLO();

	/* loading the new processor state with all interrupts enabled */
	(current_process->p_s).cpsr = STATUS_ALL_INT_ENABLE((current_process->p_s).cpsr);
	LDST(&(current_process->p_s));

}
