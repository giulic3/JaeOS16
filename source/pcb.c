#include "../include/clist.h"
#include "../include/types.h"
#include "../include/const.h"

HIDDEN struct clist pcbFree = CLIST_INIT; 


/* ALLOCATION AND DEALLOCATION */

/* Insert the element pointed to by p onto the pcbFree list*/
void freePcb(struct pcb_t *p){
	clist_enqueue(p, &pcbFree, p_list);
}

/* Return NULL if the pcbFree list is empty; otherwise, remove an element from the pcbFree list and inizialize all the fields */
struct pcb_t *allocPcb(){

	int i;

	if(pcbFree.next == NULL){
		return(NULL);
	}
	else{ 
		struct pcb_t *tmp = clist_head(tmp, pcbFree, p_list);
		clist_dequeue(&pcbFree);

		tmp->p_parent = NULL;
		tmp->p_cursem = NULL;
		(tmp->p_list).next = NULL;
		(tmp->p_children).next = NULL;
		(tmp->p_siblings).next = NULL; 
			
		/* Initialize the fields of the state_t struct */
		tmp->p_s.a1 = 0;
    	tmp->p_s.a2 = 0;
    	tmp->p_s.a3 = 0;
    	tmp->p_s.a4 = 0;
    	tmp->p_s.v1 = 0;
    	tmp->p_s.v2 = 0;
    	tmp->p_s.v3 = 0;
    	tmp->p_s.v4 = 0;
    	tmp->p_s.v5 = 0;
    	tmp->p_s.v6 = 0;
    	tmp->p_s.sl = 0;
    	tmp->p_s.fp = 0;
    	tmp->p_s.ip = 0;
    	tmp->p_s.sp = 0;
    	tmp->p_s.lr = 0;
    	tmp->p_s.pc = 0;
    	tmp->p_s.cpsr = 0;
    	tmp->p_s.CP15_Control = 0;
    	tmp->p_s.CP15_EntryHi = 0;
    	tmp->p_s.CP15_Cause = 0;
    	tmp->p_s.TOD_Hi = 0;
    	tmp->p_s.TOD_Low = 0;

    	tmp->pid = (pid_t)&tmp;
		tmp->res_wait = 0;

		tmp->kernel_time = 0;
		tmp->global_time = 0;

		for(i = 0; i < 3; i++)
			tmp->tags[i] = 0;
		return(tmp);
	}	
}

/*Initialize the pcbFree list to contain all the elements of the static array of MAXPROC ProcBlk’s.*/
void initPcbs(void){
	/* Supporting array for initialization*/
	static struct pcb_t pcbs[MAXPROC];
	int i;
	pcbFree.next = &(pcbs[MAXPROC-1].p_list);
	(pcbs[MAXPROC-1].p_list).next = &(pcbs[0].p_list);
	for (i = 1; i < MAXPROC; i++){
		(pcbs[i-1].p_list).next = &(pcbs[i].p_list);
	}
}


/* PROCESS QUEUE MAINTAINANCE */


/*Insert the ProcBlk pointed to by p into the process queue whose list-tail pointer is q */
void insertProcQ(struct clist *q, struct pcb_t *p){
 	clist_enqueue(p, q, p_list);
}

/* Remove the first element (head) from the process queue whose list tail pointer is q. 
Return NULL if the process queue was initially empty; otherwise return the pointer to the removed element */
struct pcb_t *removeProcQ(struct clist *q){
	if(clist_empty(*q))
		return NULL;
	else {
		struct pcb_t *tmp = clist_head(tmp, *q, p_list); 
		clist_dequeue(q);
		return tmp;
	}				
}

/*Remove the ProcBlk pointed to by p from the process queue whose list-tail pointer is q. 
If the desired entry is not in the indicated queue (an error condition), return NULL; otherwise, return p. */
struct pcb_t *outProcQ(struct clist *q, struct pcb_t *p){	
	struct pcb_t *scan; void *tmp = NULL;
	clist_foreach(scan, q, p_list, tmp) {
		if (scan == p){
			clist_foreach_delete(scan, q, p_list, tmp);
			return p;
		}
	}
	return NULL;			 
} 

/*Return a pointer to the first ProcBlk from the process queue whose list-tail pointer is q. 
Do not remove this ProcBlk from the process queue. Return NULL if the process queue is empty. */
struct pcb_t *headProcQ(struct clist *q){
	if (clist_empty(*q))
		return NULL;
	else {
		struct pcb_t *tmp = clist_head(tmp, *q, p_list);
		return tmp;
	}
}


/* PROCESS TREE MAINTAINANCE */


/*Return TRUE if the ProcBlk pointed to by p has no children. Return FALSE otherwise.*/
int emptyChild(struct pcb_t *p){
	if (clist_empty(p->p_children))
		return TRUE; 
	 else 
	 	return FALSE;
}

/*Make the ProcBlk pointed to by p a child of the ProcBlk pointed to by parent.*/
void insertChild(struct pcb_t *parent, struct pcb_t *p){
	clist_enqueue(p, &(parent->p_children), p_siblings);
	p->p_parent = parent; 
}


/*Make the first child of the ProcBlk pointed to by p no longer a child of p.
Return NULL if initially there were no children of p. Otherwise, return a pointer to this removed first child ProcBlk.*/
struct pcb_t *removeChild(struct pcb_t *p){
	if (emptyChild(p))
		return NULL;
	else{
		struct pcb_t *child = clist_head(p, p->p_children, p_siblings);
		child->p_parent = NULL;
		clist_dequeue(&(p->p_children));
		return child;
	}
}

/*Make the ProcBlk pointed to by p no longer the child of its parent. 
If the ProcBlk pointed to by p has no parent, return NULL; otherwise, return p.*/
struct pcb_t *outChild(struct pcb_t *p){
	if (!(p->p_parent))
		return NULL;
	else
	{
		struct pcb_t *tmp = p;
		struct pcb_t *dad = p->p_parent;
		p->p_parent = NULL;
		clist_delete(p, &(dad->p_children), p_siblings);
		return tmp;
	}
}
