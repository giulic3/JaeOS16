#include "../include/initial.h"
#include "../include/const.h"
#include "../include/types.h"
#include "../include/clist.h"
#include "../include/pcb.h"
#include "../include/syscall.h"
#include "../include/scheduler.h"

#include <uARMconst.h>
#include <libuarm.h>

/* helper function to copy fields from one state_t structure to another one */
void copy_state(state_t *dest, state_t *src) {

  dest->a1 = src->a1;
  dest->a2 = src->a2;
  dest->a3 = src->a3;
  dest->a4 = src->a4;
  dest->v1 = src->v1;
  dest->v2 = src->v2;
  dest->v3 = src->v3;
  dest->v4 = src->v4;
  dest->v5 = src->v5;
  dest->v6 = src->v6;
  dest->sl = src->sl;
  dest->fp = src->fp;
  dest->ip = src->ip;
  dest->sp = src->sp;
  dest->lr = src->lr;
  dest->pc = src->pc;
  dest->cpsr = src->cpsr;
  dest->CP15_Control = src->CP15_Control;
  dest->CP15_EntryHi = src->CP15_EntryHi;
  dest->CP15_Cause = src->CP15_Cause;
  dest->TOD_Hi = src->TOD_Hi;
  dest->TOD_Low = src->TOD_Low;
}

void pgmtrap_handler(void){

  /* disable interrupts */
  setSTATUS(STATUS_ALL_INT_DISABLE(getSTATUS()));

  /* TOD is stored at the beginning and at the end of each handler to calculate time spent by the process in it */
  cputime_t kernel_end;
  kernel_start = getTODLO();

  unsigned int cause;
  state_t* state = (state_t*)PGMTRAP_OLDAREA;

  /* if current has not specified its own program trap handler */
  if (current_process->tags[PGMTP] == FALSE){

    terminate_process(current_process->pid);
    scheduler();
  }

  else {

    copy_state(&(current_process->oldnew_areas[EXCP_PGMT_OLD]), state);
    cause = state->CP15_Cause;
    current_process->oldnew_areas[EXCP_PGMT_NEW].a1 = cause;     
    /* update kernel time */
    kernel_end = getTODLO();
    current_process->kernel_time += (kernel_end - kernel_start); 

    /* enabling all interrupts and passing control to the high level program trap handler */
    (current_process->oldnew_areas[EXCP_PGMT_NEW]).cpsr = 
    STATUS_ALL_INT_ENABLE((current_process->oldnew_areas[EXCP_PGMT_NEW]).cpsr);
    LDST(&(current_process->oldnew_areas[EXCP_PGMT_NEW]));

    }

}

void syscall_handler(void){

  setSTATUS(STATUS_ALL_INT_DISABLE(getSTATUS()));
  /*set scheduler flag */
  flag = LOADCURRENT;
  
  unsigned int sys, a2, a3, a4;
  state_t *state = (state_t *)SYSBK_OLDAREA;
  /* save the state of the running process (prior to the exception) into the current process' state */
  copy_state(&current_process->p_s, state); 
  unsigned int cause = state->CP15_Cause;
  unsigned int tmp; 
   
  cputime_t kernel_end;
  kernel_start = getTODLO();

  sys = state->a1;
  a2 = state->a2;
  a3 = state->a3;
  a4 = state->a4;

    /* if handling a syscall of number >=1 and <= 11 */
    if (CAUSE_EXCCODE_GET(cause) == EXC_SYSCALL && sys >= 1 && sys <= 11){ 

      /* if in user mode, call program trap handler */
      if((current_process->p_s.cpsr & STATUS_SYS_MODE) == STATUS_USER_MODE){
      
        state_t *pgmtrap_old =  (state_t *) PGMTRAP_OLDAREA;
        /* store current process state into the program trap Old Area and set Cause register */
        copy_state((state_t*)pgmtrap_old, &current_process->p_s);
        pgmtrap_old->CP15_Cause = CAUSE_EXCCODE_SET(pgmtrap_old->CP15_Cause, EXC_RESERVEDINSTR);
        /* update kernel time */
        kernel_end = getTODLO();
        current_process->kernel_time += (kernel_end - kernel_start);   
        pgmtrap_handler();
      }

      /* execs a syscall according to sys value (1 to 11) if in kernel mode */
      else{         

        switch(sys){

          case CREATEPROCESS:
            current_process->p_s.a1 = create_process((state_t*)a2);  
            break;

          case TERMINATEPROCESS:
            terminate_process(a2);
            break;

          case SEMOP:    
            semaphore_operation((int*)a2, a3) ;
            break;

          case SPECSYSHDL:
            specify_sysbp(a2, a3, a4);
            break;

          case SPECTLBHDL:
            specify_tlb(a2, a3, a4);
            break;

          case SPECPGMTHDL:
            specify_pgmtrap(a2, a3, a4);
            break;

          case EXITTRAP:
            exit_trap(a2, a3);
            break;

          case GETCPUTIME:
            get_cputime((cputime_t*)a2, (cputime_t*)a3);
            break;

          case WAITCLOCK:
            wait_clock();
            break;

          case IODEVOP:
            current_process->p_s.a1 = io_devop(a2, a3, a4);
            break;

          case GETPID:
            current_process->p_s.a1 = getpid();
            break;
            
          default:
            PANIC();
            break;
        }

        if (current_process){

            kernel_end = getTODLO();
            current_process->kernel_time += (kernel_end - kernel_start); 
        }
        
        scheduler();

      }
    }

    /* syscall > 11 or breakpoint exception */
    else {   

      /* if process has not specified its own high level syscall handler */
      if (current_process->tags[SYS] == FALSE){

        terminate_process(current_process->pid);
        scheduler();
      }

      else { 

        copy_state(&(current_process->oldnew_areas[EXCP_SYS_OLD]), state);

        /*the four parameter register (a1-a4) are copied from SYS/Bp Old Area to the ProcBlk SYS/Bp New Area*/
        current_process->oldnew_areas[EXCP_SYS_NEW].a1 = sys;
        current_process->oldnew_areas[EXCP_SYS_NEW].a2 = a2;
        current_process->oldnew_areas[EXCP_SYS_NEW].a3 = a3;
        current_process->oldnew_areas[EXCP_SYS_NEW].a4 = a4;

        /* the lower 4 bits of SYS/Bp Old Area’s cpsr register are copied in the most significant positions of ProcBlk SYS/Bp New Area’s a1 register*/
        tmp = (state->cpsr) & 0x0000000F;
        /* shift to the 4 most significan bits */
        tmp = tmp << 28; 
        current_process->oldnew_areas[EXCP_SYS_NEW].a1 &= 0x0FFFFFFF;
        current_process->oldnew_areas[EXCP_SYS_NEW].a1 |= tmp;
     
        /* updating kernel time */
        kernel_end = getTODLO();
        current_process->kernel_time += (kernel_end - kernel_start);   

        /* enabling all interrupts and passing control to the high level sys handler */
        (current_process->oldnew_areas[EXCP_SYS_NEW]).cpsr = 
        STATUS_ALL_INT_ENABLE((current_process->oldnew_areas[EXCP_SYS_NEW]).cpsr);
        LDST(&(current_process->oldnew_areas[EXCP_SYS_NEW]));

      }

    }

}

void tlb_handler(void){

  /* disable interrupts */
  setSTATUS(STATUS_ALL_INT_DISABLE(getSTATUS()));

  state_t* state = (state_t*)TLB_OLDAREA;
  cputime_t kernel_end;
  kernel_start = getTODLO();

  /* if current has not specified its own high level tlb handler */
  if (current_process->tags[TLB] == FALSE){

    terminate_process(current_process->pid);
    scheduler();

  }

  copy_state(&(current_process->oldnew_areas[EXCP_TLB_OLD]), state);
  unsigned int cause = state->CP15_Cause;
  current_process->oldnew_areas[EXCP_TLB_NEW].a1 = cause; 
  /* update kernel time */
  kernel_end = getTODLO();
  current_process->kernel_time += (kernel_end - kernel_start); 

  /* enabling all interrupts and passing control to the high level tlb handler */
  (current_process->oldnew_areas[EXCP_TLB_NEW]).cpsr = 
  STATUS_ALL_INT_ENABLE((current_process->oldnew_areas[EXCP_TLB_NEW]).cpsr);        
  LDST(&(current_process->oldnew_areas[EXCP_TLB_NEW]));

}