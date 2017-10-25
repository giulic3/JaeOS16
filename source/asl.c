#include "../include/const.h"
#include "../include/clist.h"
#include "../include/types.h"

HIDDEN struct clist aslh = CLIST_INIT; 		
HIDDEN struct clist semdFree = CLIST_INIT; 	


/* HELPER FUNCTIONS */

/* Insert in the semdFree list the semaphore pointed by s.
Note that the semaphore is not removed from the ASL list */ 
HIDDEN void freeSemd(struct semd_t *s){
	clist_enqueue(s, &semdFree, s_link);
}

/* Return true if the process queue of the semaphore s is empty, otherwise return false. */
HIDDEN int emptyProcQ(struct semd_t *s){
	if (clist_empty(s->s_procq))
		return TRUE;
	else 
		return FALSE;
}

/* Search the semaphore whose descriptor is s in the list of semaphores where the head is semh.
If the semaphore is found, the function returns the descriptor, otherwise it returns NULL. */
HIDDEN struct semd_t *findSemd(int *semAdd, struct clist semh){

	void *tmp = NULL; struct semd_t *scan = NULL;

	clist_foreach(scan, &semh, s_link, tmp){
		if (semAdd == scan->s_semAdd) 
			return scan;

		else if(semAdd < scan->s_semAdd){
			return NULL;
		}
	}

	return NULL;
}




/* PUBLIC FUNCTIONS */

/*Insert the ProcBlk pointed to by p at the tail of the process queue associated with the semaphore whose physical address is semAdd and set the
semaphore address of p to semAdd. 
If the semaphore is currently not active, allocate a new descriptor from the semdFree list, insert it in the ASL (in ascending order), initialize all of the fields and proceed as above. 
If a new semaphore descriptor needs to be allocated and the semdFree list is empty, return TRUE. 
In all other cases return FALSE.*/
int insertBlocked(int *semAdd, struct pcb_t *p){
	if ((p != NULL) && (semAdd != NULL))
	{	
		//Search the semaphore whose address is semAdd in the ASL
		void *tmp = NULL; 

		struct semd_t *scan = findSemd(semAdd, aslh);
		if (scan != NULL){
			p->p_cursem = scan;
			(p->p_cursem)->s_semAdd = semAdd;


			insertProcQ(&(scan->s_procq), p);
			return FALSE;
		}

		// Semaphore not found, alloc a new descriptor

		// If the semdFree is empty, impossible to allocate a new descriptor
		if (clist_empty(semdFree))
			return TRUE;

		// Allocation of a new descriptor
		else {
			struct semd_t *n = clist_head(n, semdFree, s_link);
			clist_dequeue(&semdFree);

			n->s_semAdd = semAdd;
			p->p_cursem = n;
			n->s_procq.next = NULL;
			insertProcQ(&(n->s_procq), p);	
			tmp = NULL; scan = NULL; 

			// Insert in ascending order
			clist_foreach(scan, &aslh, s_link, tmp){
				if(n->s_semAdd < scan->s_semAdd){
					clist_foreach_add(n, scan, &aslh, s_link, tmp);
					return FALSE;
				}
			}
			if(clist_foreach_all(scan,&aslh,s_link,tmp)){
	    			clist_enqueue(n, &aslh, s_link);
	    		}		
			return FALSE;
		}	
	}
}

/*Search the ASL for a descriptor of this semaphore. 
If none is found, return NULL; otherwise, remove the first ProcBlk (head) from the process queue of the found semaphore descriptor and return a pointer to it. 
If the process queue for this semaphore becomes empty remove the semaphore descriptor from the ASL and return it to the semdFree list.*/
struct pcb_t *removeBlocked(int *semAdd){

	if(semAdd == NULL)
		return NULL;

	struct semd_t *scan = findSemd(semAdd, aslh);
	if (scan != NULL){
		struct pcb_t *p = (struct pcb_t *)headProcQ(&(scan->s_procq));
			p->p_cursem = NULL;
			clist_dequeue(&(scan->s_procq));

			if(emptyProcQ(scan)){
				clist_delete(scan, &aslh, s_link);
				freeSemd(scan);
			}
			return p;
	}

	return NULL;
}

/*Remove the ProcBlk pointed to by p from the process queue associated with p’s semaphore on the ASL. 
If ProcBlk pointed to by p does not appear in the process queue associated with p’s semaphore, which is an error condition, return NULL; otherwise, return p.*/
struct pcb_t *outBlocked(struct pcb_t *p){

	void *tmp = NULL; struct pcb_t *scan = NULL;
	struct semd_t *s = p->p_cursem;
	clist_foreach(scan, &(s->s_procq), p_list, tmp)
	{
		if(scan == p){
			clist_foreach_delete(scan, &(s->s_procq), p_list, tmp);
			if (emptyProcQ(s)){
				clist_delete(s, &aslh, s_link);
				freeSemd(s);
			}	
			return p;
		}
	}
	return NULL; 	
}

/*Return a pointer to the ProcBlk that is at the head of the process queue associated with the semaphore semAdd. 
Return NULL if semAdd is not found on the ASL or if the process queue associated with semAdd is empty.*/
struct pcb_t *headBlocked(int *semAdd){
	struct semd_t *scan = findSemd(semAdd, aslh);
	if (scan != NULL){

				struct pcb_t *ret = (struct pcb_t *)headProcQ(&(scan->s_procq));
				return ret; 
	}

	return NULL; 
}


/*Initialize the semdFree list to contain all the elements of the array */
void initASL(){
	static struct semd_t semdTable[MAXPROC];
	int i;
	aslh.next = NULL;

	semdFree.next = &(semdTable[MAXPROC-1].s_link);
	(semdTable[MAXPROC-1].s_link).next = &(semdTable[0].s_link);

	for (i = 1; i < MAXPROC; i++){
		(semdTable[i-1].s_link).next = &(semdTable[i].s_link);
	}
}


