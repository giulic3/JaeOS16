#include <uARMconst.h>
#include <uARMtypes.h>
#include "../include/const.h"
#include "../include/initial.h"
#include "../include/syscall.h"
#include "../include/scheduler.h"
#include "../include/asl.h"

unsigned int find_dnum(unsigned int *dev_bitmap);
void dev_handler(int int_type);
void timer_handler();
void terminal_handler();
void interrupt_handler();

/* helper function to find device number given an interrupt line with pending interrupt */
unsigned int find_dnum(unsigned int *dev_bitmap){

	unsigned int mask = 0x00000001;
	unsigned int i = 0; 
	while(((mask & *dev_bitmap) != mask) && (i < 8)){
		mask = mask << 1;
		i++;
	}

	return i;
}

void interrupt_handler(){

	/* while handling one interrupt disable the others */
	setSTATUS(STATUS_ALL_INT_DISABLE(getSTATUS()));

	cputime_t kernel_end;
  	kernel_start = getTODLO();
  	unsigned int dnum;
	int sem_val;
	unsigned int *dev_bitmap;
	devreg_t *devreg;	
	unsigned int cause = getCAUSE();

	/* when an interrupt occurs while a process is executing, the scheduler loads the next ready process*/
	flag = LOADNEXT;
	state_t *state = (state_t *)INT_OLDAREA;

	if (current_process){

	  	copy_state(&current_process->p_s, state);
	  	/* resuming process execution from the point where the interrupt occurred */
	  	current_process->p_s.pc -= WORD_SIZE; 

    }
  	
  	/* calling specific handler according to the interrupt type*/
	if(CAUSE_IP_GET(cause, INT_TIMER))
		timer_handler();

	else if (CAUSE_IP_GET(cause, INT_DISK))
		dev_handler(INT_DISK);

	else if (CAUSE_IP_GET(cause, INT_TAPE))
		dev_handler(INT_TAPE);

	else if (CAUSE_IP_GET(cause, INT_UNUSED))
		dev_handler(INT_UNUSED);

	else if (CAUSE_IP_GET(cause, INT_PRINTER))
		dev_handler(INT_PRINTER);

	else if (CAUSE_IP_GET(cause, INT_TERMINAL)){
		terminal_handler();
	}

	/* update kernel time */
	if (current_process){

		kernel_end = getTODLO();
	  	current_process->kernel_time += (kernel_end - kernel_start);

  	}
  	/* enable all interrupts, including timer int */
	setSTATUS(STATUS_ALL_INT_ENABLE(getSTATUS()));

  	scheduler();
}


 /* helper function to handle interrupts from device, int_type replaces INT_TAPE, INT_DISK, INT_TERMINAL etc. */
void dev_handler(int int_type){	

	unsigned int dnum;
	int sem_val;
	unsigned int *dev_bitmap;
	devreg_t *devreg;
	struct pcb_t *tmp;

	/*  getting interrupt line address */
	dev_bitmap = (memaddr*)CDEV_BITMAP_ADDR(int_type);
	/* getting device number inside line */
	dnum = find_dnum(dev_bitmap);
	/* getting device register address given device number and interrupt line */
	devreg = (devreg_t*)DEV_REG_ADDR(int_type, dnum);

	sem_val = device_sem[EXT_IL_INDEX(int_type)*DEV_PER_INT + dnum];

	/* performing a sys3 on the device semaphore only if its values is less than 1 */
	if (sem_val < 1){

		tmp = (struct pcb_t*)headBlocked(&device_sem[EXT_IL_INDEX(int_type)*DEV_PER_INT + dnum]);
		tmp->p_s.a1 = devreg->dtp.status;
		semaphore_operation(&device_sem[EXT_IL_INDEX(int_type)*DEV_PER_INT + dnum], 1);
	}
	/* ack to device */
	devreg->dtp.command = DEV_C_ACK;

}

void timer_handler(){

	/* 100 ms (approx.) are expired */
	if (pseudoclockflag == TRUE){
		/* unlock all processes blocked on the pseudoclock timer semaphore */
		while (device_sem[CLOCK_SEM] < 0){ 
			semaphore_operation(&device_sem[CLOCK_SEM], 1);
		}

		/* set scheduler execution mode */
			flag = RESET;
	}

	/* sending ack to the pseudoclock */
	setTIMER(SCHED_BOGUS_SLICE);
}

void terminal_handler(){

	struct pcb_t* unblocked;
	unsigned int* dev_bitmap;
	int sem_val;
	devreg_t* devreg;
	unsigned int dnum;

	dev_bitmap = (memaddr*)CDEV_BITMAP_ADDR(INT_TERMINAL);
	dnum = find_dnum(dev_bitmap);
	devreg = (devreg_t*)DEV_REG_ADDR(INT_TERMINAL, dnum);

	/* if terminal is in reading mode */
	 if((devreg->term.recv_status & 0x000000FF) == DEV_TRCV_S_CHARRECV){

		sem_val = device_sem[EXT_IL_INDEX(INT_TERMINAL)*DEV_PER_INT+ DEV_PER_INT + dnum];

		/* performing a sys3 on the device semaphore only if its values is less than 1 */
		if (sem_val < 1){

			unblocked = (struct pcb_t*)headBlocked(&device_sem[EXT_IL_INDEX(INT_TERMINAL)*DEV_PER_INT+ DEV_PER_INT + dnum]);
			unblocked->p_s.a1 = devreg->term.recv_status;
			semaphore_operation(&device_sem[EXT_IL_INDEX(INT_TERMINAL)*DEV_PER_INT+ DEV_PER_INT + dnum], 1);

		}
		/* sending ack to terminal device */
		devreg->term.recv_command = DEV_C_ACK;
	}
	
	/* else if terminal is in writing mode */
	if((devreg->term.transm_status & 0x000000FF) == DEV_TTRS_S_CHARTRSM){

		sem_val = device_sem[EXT_IL_INDEX(INT_TERMINAL)*DEV_PER_INT + dnum];

		if (sem_val < 1){

			unblocked = (struct pcb_t*)headBlocked(&device_sem[EXT_IL_INDEX(INT_TERMINAL)*DEV_PER_INT + dnum]);
			unblocked->p_s.a1 = devreg->term.transm_status;
			semaphore_operation(&device_sem[EXT_IL_INDEX(INT_TERMINAL)*DEV_PER_INT + dnum], 1);
		}
		/* sending ack to terminal device */
		devreg->term.transm_command = DEV_C_ACK;
	}

}