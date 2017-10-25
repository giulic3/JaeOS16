#include "../include/initial.h"
#include "../include/asl.h"
#include "../include/const.h"
#include "../include/clist.h"
#include "../include/types.h"
#include "../include/pcb.h"
#include "../include/interrupts.h"
#include "../include/scheduler.h"
#include <arch.h>
#include <uARMtypes.h>
#include <uARMconst.h>

int create_process(state_t *statep);
void lookfor_progeny(struct pcb_t *parent, pid_t pid, int *found);
void terminate_process(pid_t pid);
void terminate_children(struct pcb_t *p, struct pcb_t *p_const);
void semaphore_operation(int *semaddr, int weight);
void specify_sysbp(memaddr pc, memaddr sp, unsigned int flags);
void specify_tlb(memaddr pc, memaddr sp, unsigned int flags);
void specify_pgmtrap(memaddr pc, memaddr sp, unsigned int flags);
void exit_trap(unsigned int excptype, unsigned int retval);
void get_cputime(cputime_t *global, cputime_t *user);
void wait_clock();
unsigned int io_devop(unsigned int command, int intlNo, unsigned int dnum);
pid_t getpid();

/* helper function used in sys create_process to assign id to process */
unsigned int pid_generator(unsigned int x, unsigned int y){

    unsigned int pow = 10;
    while(y >= pow)
        pow *= 10;
    return x * pow + y;        
}

int create_process(state_t *statep){

	struct pcb_t *child;

	if (child = allocPcb()){

		child->kernel_time = 0;
		child->global_time = 0;

		copy_state(&child->p_s, statep);

		/* insert child in the current process children's list */
		insertChild(current_process, child);
		/* assign id */
		child->pid = pid_generator(child->pid, counter); 
		counter++;

		/* insert the new process into the readyqueue*/
		insertProcQ(&readyqueue, child);
		/* update number of living processes*/
		process_count++; 
		return (child->pid);

	}
	else
		return -1;

}

/* parent is the process from which the search starts, pid is the id of the process to kill,
found is a flag that indicates if the process was found. 
if at the end of the search found = FALSE, terminate_process calls panic() */
void lookfor_progeny(struct pcb_t *parent, pid_t pid, int* found){

	struct pcb_t *child;
	void *tmp = NULL;
	int weight = 0;

	if (parent->pid == pid){

		*found = 1;
		terminate_children(parent, parent);
		
		/* if the process to kill was blocked on a semaphore */
		if (parent->p_cursem != NULL){
			weight = parent->res_wait;

			/* update semaphore value only if not a device semaphore */	
			if (!(((parent->p_cursem->s_semAdd) >= &device_sem[0]) && ((parent->p_cursem->s_semAdd) <= &device_sem[MAX_DEVICES-1])))
				*(parent->p_cursem->s_semAdd) -= weight;

			/* remove parent from the queue of process blocked on the semaphore */
			outBlocked(parent);
			soft_block_count--;
		}
		
		process_count--;
		outChild(parent);
		outProcQ(&readyqueue, parent);
		freePcb(parent);

	}
	else {
		/* recursively search process with given id */
		clist_foreach(child, &(parent->p_children), p_siblings, tmp){
			if (*found == 0)
				lookfor_progeny(child, pid, found);
			else
				break;
		}

	}
}


void terminate_process(pid_t pid){   

	/* if pid == 0 kills parent and its progeny */
	if ((pid == 0) || (pid == current_process->pid)) {
		terminate_children(current_process, current_process); 
		process_count--;
		
		outChild(current_process);
		outProcQ(&readyqueue, current_process);
		freePcb(current_process); 

		current_process = NULL;
		flag = LOADNEXT;
	}

	/* if pid != 0 search process with given id and kill it and its progeny */
	else{
		struct pcb_t *tmp;
		int found = 0;
		lookfor_progeny(current_process, pid, &found);
		if (found == 0) PANIC(); 

		/* the handler will call the scheduler with flag = LOADCURRENT */

	}

}

/* p is the starting point of the research, p_const stores a constant pointer to current_process */
void terminate_children(struct pcb_t *p, struct pcb_t *p_const){

int weight = 0;
int *semaddr ;

	while(!emptyChild(p))
			terminate_children(removeChild(p), p_const);

	if ((p!= NULL) && (p != p_const)){ 			
		
		/* if the process is blocked on a semaphore*/
		if (p->p_cursem != NULL){ 

			weight = p->res_wait;			
			/*adjust semaphore value only if not a device semaphore */
			if (!(((p->p_cursem->s_semAdd) >= &device_sem[0]) && (p->p_cursem->s_semAdd) <= &device_sem[MAX_DEVICES-1]))
				*(p->p_cursem->s_semAdd) -= weight;

			/* remove p from semaphore queue on which is blocked */
			outBlocked(p);
			soft_block_count--;
		}	

		process_count--;
		freePcb(p);
	}

}

/*if weight > 0, performs a V(), releases resources, else if weight < 0, performs a P(), requests resources*/

void semaphore_operation(int *semaddr, int weight){

	struct pcb_t *tmp;

	if (semaddr == NULL)
		PANIC();

	if 	(weight == 0)
		terminate_process(current_process->pid);

	/* passeren */
	else if (weight < 0){ 
		*semaddr += weight;

			if (*semaddr < 0) {
				current_process->res_wait = weight;
				
				/* update kernel time before blocking the process on the semaphore*/
				current_process->kernel_time += (getTODLO() - kernel_start);
				current_process->global_time += (getTODLO() - startTOD);

				if (insertBlocked(semaddr, current_process)){
					PANIC();
				}

				soft_block_count++;
				/* set scheduler execution mode */
				flag = LOADNEXT;
				current_process = NULL;
			}
	}

	/* weight > 0, verhogen */
	else{ 

		int sum = 0; 
		*semaddr += weight;	
		struct pcb_t *scan = headBlocked(semaddr);

		if(scan != NULL){
			void *tmp1 = NULL;
			/*sum of the res_wait of each process in the semaphore queue*/
			clist_foreach(scan, &(scan->p_cursem->s_procq), p_list, tmp1)
				sum = sum - scan->res_wait; 
			
		}
		/* available resources not assigned yet */
		sum = sum + *semaddr; 

		/*check if there is a blocked process and if there are enough resources to unlock it */
		while (((tmp = headBlocked(semaddr)) != NULL) && (tmp->res_wait >= -sum) && (sum != 0)) {
		
				sum += tmp->res_wait;
				tmp = outBlocked(tmp);	
				tmp->p_cursem = NULL;			
				soft_block_count--;
				tmp->res_wait = 0;
				insertProcQ(&readyqueue, tmp);
		}	
	}
}

void specify_sysbp(memaddr pc, memaddr sp, unsigned int flags){

	unsigned int asid;

	/* if specify_sysbp has already been called */
	if(current_process->tags[SYS] == TRUE){
		terminate_process(current_process->pid);
		scheduler();	
	}

	current_process->tags[SYS] = TRUE;

	/* copy current processor state into process's old area */
	STST(&(current_process->oldnew_areas[EXCP_SYS_OLD]));

	copy_state(&current_process->oldnew_areas[EXCP_SYS_NEW], &current_process->p_s);
	current_process->oldnew_areas[EXCP_SYS_NEW].pc = pc;
	current_process->oldnew_areas[EXCP_SYS_NEW].sp = sp;

	/*isolate necessary bits to update New Area cpsr register and set VM mode*/
	flags = flags & (0x80000007); 
	current_process->oldnew_areas[EXCP_SYS_NEW].cpsr &= (0x7FFFFFF8); 
	current_process->oldnew_areas[EXCP_SYS_NEW].cpsr |= flags;

	/* get EntryHi register content */
	asid = getEntryHi();
	current_process->oldnew_areas[EXCP_SYS_NEW].CP15_EntryHi = setEntryHi(asid);

}

void specify_tlb(memaddr pc, memaddr sp, unsigned int flags){

	unsigned int asid;

	/* if a handler was already specified */
	if(current_process->tags[TLB] == TRUE){
		terminate_process(current_process->pid);
		scheduler();
	}

	current_process->tags[TLB] = TRUE;

	STST(&(current_process->oldnew_areas[EXCP_TLB_OLD]));

	copy_state(&current_process->oldnew_areas[EXCP_TLB_NEW], &current_process->p_s);
	current_process->oldnew_areas[EXCP_TLB_NEW].pc = pc;
	current_process->oldnew_areas[EXCP_TLB_NEW].sp = sp;

	flags = flags & (0x80000007); 

	current_process->oldnew_areas[EXCP_TLB_NEW].cpsr &= (0x7FFFFFF8);
	current_process->oldnew_areas[EXCP_TLB_NEW].cpsr |= flags;

	asid = getEntryHi();
	current_process->oldnew_areas[EXCP_TLB_NEW].CP15_EntryHi = setEntryHi(asid);

}

void specify_pgmtrap(memaddr pc, memaddr sp, unsigned int flags){

	unsigned int asid;

	if(current_process->tags[PGMTP]== TRUE){

		terminate_process(current_process->pid);
		scheduler();
	}
	current_process->tags[PGMTP] = TRUE;

	STST(&(current_process->oldnew_areas[EXCP_PGMT_OLD]));

	copy_state(&current_process->oldnew_areas[EXCP_PGMT_NEW], &current_process->p_s);
	current_process->oldnew_areas[EXCP_PGMT_NEW].pc = pc;
	current_process->oldnew_areas[EXCP_PGMT_NEW].sp = sp;

	flags = flags & (0x80000007); 

	current_process->oldnew_areas[EXCP_PGMT_NEW].cpsr &= (0x7FFFFFF8);
	current_process->oldnew_areas[EXCP_PGMT_NEW].cpsr |= flags;

	asid = getEntryHi(); 
	current_process->oldnew_areas[EXCP_PGMT_NEW].CP15_EntryHi = setEntryHi(asid);

}

void exit_trap(unsigned int excptype, unsigned int retval){

	/* loading pcb old state according to excptype value */
	switch(excptype){

		/* system call */
		case SYS:{
			current_process->oldnew_areas[EXCP_SYS_OLD].a1 = retval;
			LDST(&current_process->oldnew_areas[EXCP_SYS_OLD]);
			break;
		}
		/* tlb */
		case TLB:{
			current_process->oldnew_areas[EXCP_TLB_OLD].a1 = retval;
			LDST(&current_process->oldnew_areas[EXCP_TLB_OLD]);
			break;
		}
		/* program trap */
		case PGMTP:{
			current_process->oldnew_areas[EXCP_PGMT_OLD].a1 = retval;
			LDST(&current_process->oldnew_areas[EXCP_PGMT_OLD]);
			break;
		}

	}
	
}

void get_cputime(cputime_t *global, cputime_t *user){

	*global = current_process->global_time;
	*user = (current_process->global_time - current_process->kernel_time);
}

void wait_clock(){

	/* locks the requesting process on the pseudoclock timer semaphore */
	semaphore_operation(&device_sem[MAX_DEVICES-1], -1);
												
}

unsigned int io_devop(unsigned int command, int intlNo, unsigned int dnum){ 

    setSTATUS(STATUS_ALL_INT_ENABLE(getSTATUS()));  

	state_t* current_state;
	unsigned int status_word = 0;
	int term_read = 0;

	/* pointer to union representing a generic device, with two fields: dtpreg_t dtp and termreg_t term */
	devreg_t *devreg;

	/* terminals in writing mode precede reading terminals in the array since they have higher priority */
	/* check if it's a terminal in reading mode*/
	if ((intlNo == INT_TERMINAL) && (dnum >> 31))
		term_read = DEV_PER_INT;

	/* EXT_IL_INDEX(intlNo)*N_DEV_PER_IL is the starting index, MAX_DEVICES is 49 */
	semaphore_operation(&device_sem[EXT_IL_INDEX(intlNo)*DEV_PER_INT + term_read + dnum], -1);

	devreg = (devreg_t*)DEV_REG_ADDR(intlNo, dnum);

	if (intlNo == INT_TERMINAL){

		/* terminal in reading */
		if (term_read) {

			devreg->term.recv_command = command;
			status_word = devreg->term.recv_status;

		}
		/* terminal writing */
		else { 	
			devreg->term.transm_command = command;
			status_word = devreg->term.transm_status;
		}
	}

	else {

		devreg->dtp.command = command;
		status_word = devreg->dtp.status;
		
	}
	return status_word;
}

pid_t getpid(){

	return current_process->pid;
}



