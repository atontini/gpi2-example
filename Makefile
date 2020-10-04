NPI	= 2                         # Total number of MPI-task
LNPI	= 2                         # Number of MPI-task/node
NPIX	= 1
NPIY	= 2
NITER	= 1000
NELEM	= 32

all: test

test2D: test.c
	gcc -Wall -O3 -I${GPIINC} -DNPI=${NPI} -DLNPI=${LNPI} -DNPIX=${NPIX} -DNPIY=${NPIY} -DNITER=${NITER} -DNELEM=${NELEM} -o test2D.exe test2D.c -L${GPILIB} -lGPI2 -libverbs -lm -lpthread



clean:
	/bin/rm -f test.exe
