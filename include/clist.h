#ifndef _CLIST_H
#define _CLIST_H


typedef unsigned int size_t;

#define container_of(ptr, type, member) ({      \
		    const typeof( ((type *)0)->member ) *__mptr = (ptr);  \
		    (type *)( (char *)__mptr - offsetof(type,member) );})

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

/* Struct clist definition: it's the type of the tail pointer of the circular list and the type of field used to link the elements */
struct clist {
	struct clist *next;
};

/* Constant used to initialize an empty list */
#define CLIST_INIT {NULL}

/* Add the structure pointed to by elem as the last element of a circular list */
#define clist_enqueue(elem, clistp, member) ({ \
	if (clist_empty(*clistp)) { \
		((clistp)->next) = &(elem->member); \
		((elem->member).next) = &(elem->member); \
	} \
	else { \
		(elem->member).next = &((clist_head(elem, *clistp, member))->member); \
		((clist_tail(elem, *clistp, member))->member).next = &(elem->member);  \
		(clistp)->next = &(elem->member); \
	} \
})

/* Add the structure pointed to by elem as the first element of a circular list */
#define clist_push(elem, clistp, member) ({ \
	if (clist_empty(*clistp)) { \
		((clistp)->next) = &(elem->member); \
		((elem->member).next) = &(elem->member); \
	} \
	else { \
		(elem->member).next = &((clist_head(elem, *clistp, member))->member); \
		((clist_tail(elem, *clistp, member))->member).next = &(elem->member); \
	} \
})

/* Clist_empty returns true in the circular list is empty, false otherwise */
#define clist_empty(clistx) (((clistx).next)==NULL)

/* Return the pointer of the first element of the circular queue */
#define clist_head(elem, clistx, member)\
	container_of((((clistx).next)->next), typeof(*elem), member)

/* Return the pointer of the last element of the circular queue */
#define clist_tail(elem, clistx, member) \
	container_of(((clistx).next), typeof(*elem), member) 

/* Delete the first element of the list (this macro does not return any value) */
#define clist_pop(clistp) clist__dequeue(clistp)

#define clist_dequeue(clistp)({\
	if ((clistp)->next == ((clistp)->next)->next)\
		(clistp)->next = NULL;\
	else \
		((clistp)->next)->next = (((clistp)->next)->next)->next;})

/* Delete from a circular list the element whose pointer is elem */
#define clist_delete(elem, clistp, member)({ \
	int retval = 1;\
	typeof(*elem) *__scan = NULL; void *__tmp = NULL;\
	if (!clist_empty(*clistp)){\
		if (clist_head(elem, *clistp, member) == elem){\
			clist_dequeue(clistp);\
			retval = 0;\
		}\
	    	else{\
	        	for(__scan = clist_head(elem, *clistp, member), (__tmp) = NULL; \
			&((__scan)->member) != (__tmp); \
			(__scan) = container_of((((__scan)->member).next),typeof(*elem), member), \
			(__tmp) = ((clistp)->next)->next){\
		  		 if (container_of((((__scan)->member).next), typeof(*elem), member) == elem){\
						if (clist_tail(elem, *clistp, member) == elem){\
							((clistp)->next) = &(__scan->member);\
							((__scan)->member).next = ((elem)->member).next;\
						 	retval = 0;\
						}\
						else{\
							((__scan)->member).next = ((elem->member).next);\
					      		retval = 0;\
						}\
		   		 }\
	       		}\
    		}\
	}\
	retval;\
})

/* This macro has to be used as a for instruction */
/*#define clist_foreach(scan, clistp, member, tmp)\
	if (!clist_empty(*clistp))\
		for(scan = container_of((((clistp)->next)->next), typeof(*scan), member), (tmp) = NULL; \
		&((scan)->member) != (tmp); \
		(scan) = container_of((((scan)->member).next),typeof(*scan), member), \
		(tmp) = ((clistp)->next)->next) 
*/
/* This macro has to be used as a for instruction */
#define clist_foreach(scan, clistp, member, tmp)\
	if (!clist_empty(*clistp))\
		for(scan = container_of((((clistp)->next)->next), typeof(*scan), member), (tmp) = NULL; \
		(tmp) != (clistp)->next ; \
		(tmp) = &((scan)->member),\
		(scan) = container_of((((scan)->member).next),typeof(*scan), member))


/* This macro should be used after the end of a clist_foreach cycle using the same args. 
It returns false if the cycle terminated by a break and true if it scanned all the elements */
#define clist_foreach_all(scan, clistp, member, tmp)\
	 ((clist_empty(*clistp)) || ((tmp) == (clistp)->next))


/* This macro should be used inside a clist_foreach cycle to add an element before the current one */
#define clist_foreach_add(elem, scan, clistp, member, tmp)({\
	if (tmp) {(elem)->member.next = &((scan)->member); \
	((struct clist*)tmp)->next = &(elem)->member; }\
	else clist_push(elem, clistp, member);})
// se tmp è null significa che siamo al primo giro (=devo inserire subito, in testa)


/* This macro should be used inside a clist_foreach loop to delete the current element */
#define clist_foreach_delete(scan, clistp, member, tmp)\
	if (clist_head(scan, *clistp, member) == scan)\
		clist_dequeue(clistp);\
	else if (clist_tail(scan, *clistp, member) == (scan)){\
		struct clist *todelete = tmp;\
		(clistp)->next = todelete;\
		(todelete)->next = ((scan)->member).next;\
	}\
	else {\
		struct clist *todelete = tmp;\
		(todelete)->next = ((scan->member).next);\
	}

#endif
