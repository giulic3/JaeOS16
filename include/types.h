#ifndef TYPES_H
#define TYPES_H

/*type state_t is defined here*/
#include <uARMtypes.h>

#include "const.h"
#include "clist.h"

struct pcb_t {

	struct pcb_t *p_parent; /* pointer to parent */
	struct semd_t *p_cursem; /* pointer to the semd_t on which process blocked */
	state_t p_s; /*processor state */
	struct clist p_list; /* process list */
	struct clist p_children; /*children list entry point*/
	struct clist p_siblings; /* children list: links to the siblings*/
	
	/* new fields */
	pid_t pid; /* assigned process id */
	int res_wait; /* resources needed by the process, if blocked (always <= 0) */

	cputime_t kernel_time; /* time spent in the nucleus */
  	cputime_t global_time; /* time spent in execution in the CPU (user time + kernel time)*/
  	
  	state_t oldnew_areas[EXCP_COUNT]; /* exception state vector to store old and new areas*/

   	int tags[3]; /* if tags[i] = TRUE the process has specified its own handler of type i*/
   				/* 0 for SYS, 1 for TLB, 2 for PGMTP as defined in const.h */

};

struct semd_t{

	int *s_semAdd; /*pointer to the semaphore*/
	struct clist s_link; /*ASL linked list*/
	struct clist s_procq; /*blocked process queue */
};


#endif
