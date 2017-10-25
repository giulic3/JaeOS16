CC = arm-none-eabi-gcc
LD = arm-none-eabi-ld
IN = include/
SRC = source/
CFLAGS = -mcpu=arm7tdmi -c -I $(IN) -I "/usr/include/uarm"
OBJECTS		= $(SRC)p2test.o $(SRC)initial.o $(SRC)interrupts.o $(SRC)exceptions.o $(SRC)syscall.o $(SRC)scheduler.o $(SRC)pcb.o $(SRC)asl.o
UARM_LIBS	= /usr/include/uarm/ldscripts/elf32ltsarm.h.uarmcore.x -o jaeos16 /usr/include/uarm/crtso.o /usr/include/uarm/libuarm.o
UARM_HEADERS	= /usr/include/uarm/libuarm.h /usr/include/uarm/uARMconst.h /usr/include/uarm/uARMtypes.h /usr/include/uarm/arch.h

P1_HEADERS = $(IN)clist.h $(IN)pcb.h $(IN)asl.h
P2_HEADERS = $(IN)const.h $(IN)types.h $(IN)initial.h $(IN)interrupts.h $(IN)exceptions.h $(IN)syscall.h $(IN)scheduler.h $(IN)p2test.h

all : jaeos16

phase2 : $(OBJECTS)
	$(LD) -T $(UARM_LIBS)	
	elf2uarm -k jaeos16

jaeos16 : $(OBJECTS)
	$(LD) -T $(UARM_LIBS) $(OBJECTS)
	elf2uarm -k jaeos16

pcb.o : $(SRC)pcb.c $(IN)pcb.h $(IN)clist.h $(IN)types.h $(IN)const.h
	$(CC) $(CFLAGS) -o $(SRC)pcb.o $(SRC)pcb.c

asl.o : $(SRC)asl.c $(P1_HEADERS) $(IN)const.h
	$(CC) $(CFLAGS) -o $(SRC)asl.o $(SRC)asl.c

p2test.o : $(SRC)p2test.c $(UARM_HEADERS) $(IN)p2test.h $(IN)pcb.h
	$(CC) $(CFLAGS) -o $(SRC)p2test.o $(SRC)p2test.c

initial.o : $(SRC)initial.c $(P1_HEADERS) $(P2_HEADERS) 
	$(CC) $(CFLAGS) -o $(SRC)initial.o $(SRC)initial.c

interrupts.o :  $(SRC)interrupts.c $(UARM_HEADERS) $(IN)const.h $(IN)syscall.h $(IN)initial.h $(IN)scheduler.h $(IN)asl.h 
	$(CC) $(CFLAGS) -o $(SRC)interrupts.o $(SRC)interrupts.c

exceptions.o :  $(SRC)exceptions.c $(UARM_HEADERS) $(IN)exceptions.h $(IN)initial.h $(IN)const.h $(IN)types.h $(IN)clist.h $(IN)pcb.h $(IN)syscall.h 
	$(CC) $(CFLAGS) -o $(SRC)exceptions.o $(SRC)exceptions.c

syscall.o :  $(SRC)syscall.c $(UARM_HEADERS) $(P1_HEADERS) $(IN)syscall.h $(IN)initial.h $(IN)const.h $(IN)types.h $(IN)interrupts.h $(IN)scheduler.h 
	$(CC) $(CFLAGS) -o $(SRC)syscall.o $(SRC)syscall.c

scheduler.o :  $(SRC)scheduler.c $(UARM_HEADERS) $(IN)scheduler.h $(IN)clist.h $(IN)pcb.h $(IN)initial.h $(IN)interrupts.h $(IN)types.h $(IN)const.h 
	$(CC) $(CFLAGS) -o $(SRC)scheduler.o $(SRC)scheduler.c

clean :
	rm -rf $(SRC)*o jaeos16

cleanall :
	rm -rf $(SRC)*o jaeos16 jaeos16.core.uarm jaeos16.stab.uarm

