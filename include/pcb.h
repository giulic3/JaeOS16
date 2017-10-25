#ifndef _PCB_H
#define _PCB_H
#include "types.h"

/* process queue maintainance */

extern void freePcb(struct pcb_t *p);
extern struct pcb_t *allocPcb(void);
extern void initPcbs(void);
extern void insertProcQ(struct clist *q, struct pcb_t *p);
extern struct pcb_t *removeProcQ(struct clist *q);
extern struct pcb_t *outProcQ(struct clist *q, struct pcb_t *p);
extern struct pcb_t *headProcQ(struct clist *q);

/* process tree maintainance*/

extern int emptyChild(struct pcb_t *p);
extern void insertChild(struct pcb_t *parent, struct pcb_t *p);
extern struct pcb_t *removeChild(struct pcb_t *p);
extern struct pcb_t *outChild(struct pcb_t *p);

#endif