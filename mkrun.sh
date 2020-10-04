#!/bin/bash

if [[ -z $NITER ]]; then
  NITER=100
fi

if [[ -z $NPIX ]]; then
  NPIX=2
  echo "WARNING: SETTING NPIX TO $NPIX"
fi

if [[ -z $NPIY ]]; then
  NPIY=2
  echo "WARNING: SETTING NPIX TO $NPIY"
fi

if [[ -z $NNODES ]]; then
  NNODES=1
fi

if [[ -z $NELEM ]]; then
  NELEM=32
fi


#-----------------------------------------------------------

if [[ -z $NPI ]]; then
  NPI=$(($NPIX*$NPIY))
fi

if [[ $NPI -ne 1 ]]; then
  LNPI=$(($NPI/$NNODES))
else
  LNPI=1
fi

if [[ -z $NCORE ]]; then
  NCORE=$((16/$LNPI))    #-- number of cpus per task
fi

echo "set NNODES=${NNODES}"
echo "set NPI=${NPI}"
echo "set LNPI=${LNPI}"
echo "set NCORE=${NCORE}"

#-----------------------------------------------------------

ts=`date +"%Y-%m-%d-%H-%M-%S"`

JOBDIR="gpi-test-${NPIX}x${NPIY}-$ts"

/usr/bin/mkdir -p $JOBDIR

cd $JOBDIR

ln -s ../*.c .
ln -s ../Makefile .
ln -s ../printmachinefile .

#-----------------------------------------------------------

module purge

MACHINEFILE="machinefile"

#GPI
module load devtoolset-7 gpi2/1.3.0

CC=gcc
CFLAGS="-O3 -W -Wall -fopenmp -lm -I${GPIINC} -L${GPILIB}"
RUN="gaspi_run -m $MACHINEFILE "

DEF="NPI=$NPI LNPI=$LNPI NPIX=$NPIX NPIY=$NPIY NITER=$NITER"

make clean && make NPI="$NPI" LNPI="$LNPI" NPIX="$NPIX" NPIY="$NPIY" NITER="$NITER" NELEM="$NELEM"

if [[ $? != 0 ]];
then
  echo "compilation FAILED !"
  exit 1
fi

#-----------------------------------------------------------

cat > "run.slurm" << EOF
#!/bin/bash
#
#SBATCH --job-name=gpi-test
#SBATCH --error=gpi-test-%j.err
#SBATCH --output=gpi-test-%j.out
#SBATCH --nodes=$NNODES
#SBATCH --mem=64000
#SBATCH --ntasks-per-node=$LNPI
#SBATCH --cpus-per-task=$NCORE
#SBATCH --cpu-freq=performance
#SBATCH --exclusive
#SBATCH --partition=shortrun
#SBATCH --requeue

echo "SLURM_JOB_NODELIST=\$SLURM_JOB_NODELIST SLURM_NTASKS=\$SLURM_NTASKS SLURM_JOB_NUM_NODES=\$SLURM_JOB_NUM_NODES"

source printmachinefile
printmachinefile > $MACHINEFILE

echo "module load devtoolset-7 gpi2/1.3.0"

export OMP_NUM_THREADS=1

echo "Running $NPI ($NPIX x $NPIY) tasks, $LNPI task/node"

$RUN ./test.exe

EOF

sbatch run.slurm

#-----------------------------------------------------------

exit 0

