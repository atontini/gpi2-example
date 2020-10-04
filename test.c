#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include <GASPI.h>

#ifndef NITER
#define NITER 1000
#endif

#ifndef NELEM
#define NELEM 1024
#endif

#define SUCCESS_OR_DIE(f...)                                            \
  do                                                                    \
  {                                                                     \
    const gaspi_return_t r = f;                                         \
                                                                        \
    if (r != GASPI_SUCCESS)                                             \
    {                                                                   \
      printf ("Error: '%s' [%s:%i]: %i\n", #f, __FILE__, __LINE__, r); \
                                                                        \
      exit (EXIT_FAILURE);                                              \
    }                                                                   \
  } while (0)


int main (){
  struct timeval ti[4], tf[4];
  double dT[4];

  const gaspi_segment_id_t segment_id_leftSndBuf = 0;
  const gaspi_segment_id_t segment_id_leftRxBuf = 1;

  const gaspi_segment_id_t segment_id_rightSndBuf = 2;
  const gaspi_segment_id_t segment_id_rightRxBuf = 3;

  const gaspi_segment_id_t segment_id_topSndBuf = 4;
  const gaspi_segment_id_t segment_id_topRxBuf = 5;

  const gaspi_segment_id_t segment_id_botSndBuf = 6;
  const gaspi_segment_id_t segment_id_botRxBuf = 7;

  int leftError = 0, rightError = 0;
  int leftNfailure = 0, rightNfailure=0;

  int topError = 0, botError = 0;
  int topNfailure = 0, botNfailure=0;

  gaspi_size_t const segment_size = NELEM * sizeof (double);

  gaspi_notification_id_t reqRight = 0;
  gaspi_notification_id_t reqLeft = 1;
  gaspi_notification_id_t reqTop = 2;
  gaspi_notification_id_t reqBot = 3;

  gaspi_notification_id_t id;
  gaspi_notification_t value;

  gaspi_rank_t rank;

  gaspi_rank_t peer_left;
  gaspi_rank_t peer_right;
  gaspi_rank_t peer_top;
  gaspi_rank_t peer_bot;

  gaspi_rank_t size;

  gaspi_queue_id_t queue_id = 0;

  gaspi_offset_t off = 0;

  gaspi_number_t queue_size_max;
  gaspi_number_t queue_size;

  SUCCESS_OR_DIE (gaspi_proc_init (GASPI_BLOCK));

  SUCCESS_OR_DIE (gaspi_proc_rank (&rank));
  SUCCESS_OR_DIE (gaspi_proc_num (&size));

  /////////////////////////////////////////////////////////////////////////////

  // Calculating peers
  peer_left  =  ((rank    % NPIX) == 0)     ? (rank+NPIX-1)           : (rank-1);
  peer_right = (((rank+1) % NPIX) == 0)     ? (rank-NPIX+1)           : (rank+1);
  peer_top   =   (rank >= (NPIX*(NPIY-1))) ? (rank-(NPIX*(NPIY-1))) : (rank+NPIX);
  peer_bot   =   (rank <   NPIX)            ? (rank+(NPIX*(NPIY-1))) : (rank-NPIX);

  /////////////////////////////////////////////////////////////////////////////

  int i, k;

  // Delta-times are initialized.
  dT[0] = 0.0;
  dT[1] = 0.0;
  dT[2] = 0.0;
  dT[3] = 0.0;


  // Create segment for data
  SUCCESS_OR_DIE(gaspi_segment_create(segment_id_leftSndBuf,	segment_size,	GASPI_GROUP_ALL, GASPI_BLOCK,GASPI_MEM_INITIALIZED));
  SUCCESS_OR_DIE(gaspi_segment_create(segment_id_leftRxBuf,	segment_size,	GASPI_GROUP_ALL, GASPI_BLOCK,GASPI_MEM_INITIALIZED));

  SUCCESS_OR_DIE(gaspi_segment_create(segment_id_rightSndBuf,	segment_size,	GASPI_GROUP_ALL, GASPI_BLOCK,GASPI_MEM_INITIALIZED));
  SUCCESS_OR_DIE(gaspi_segment_create(segment_id_rightRxBuf,	segment_size,	GASPI_GROUP_ALL, GASPI_BLOCK,GASPI_MEM_INITIALIZED));

  SUCCESS_OR_DIE(gaspi_segment_create(segment_id_topSndBuf,	segment_size,	GASPI_GROUP_ALL, GASPI_BLOCK,GASPI_MEM_INITIALIZED));
  SUCCESS_OR_DIE(gaspi_segment_create(segment_id_topRxBuf,	segment_size,	GASPI_GROUP_ALL, GASPI_BLOCK,GASPI_MEM_INITIALIZED));

  SUCCESS_OR_DIE(gaspi_segment_create(segment_id_botSndBuf,	segment_size,	GASPI_GROUP_ALL, GASPI_BLOCK,GASPI_MEM_INITIALIZED));
  SUCCESS_OR_DIE(gaspi_segment_create(segment_id_botRxBuf,	segment_size,	GASPI_GROUP_ALL, GASPI_BLOCK,GASPI_MEM_INITIALIZED));


  // Buffer pointers
  double *leftSndBuf = NULL;
  double *leftRxBuf = NULL;

  double *rightSndBuf = NULL;
  double *rightRxBuf = NULL;

  double *topSndBuf = NULL;
  double *topRxBuf = NULL;

  double *botSndBuf = NULL;
  double *botRxBuf = NULL;


  // Get initial pointer to each segment
  SUCCESS_OR_DIE (gaspi_segment_ptr (segment_id_leftSndBuf, (void **)  &leftSndBuf));
  SUCCESS_OR_DIE (gaspi_segment_ptr (segment_id_leftRxBuf, (void **)  &leftRxBuf));

  SUCCESS_OR_DIE (gaspi_segment_ptr (segment_id_rightSndBuf, (void **)  &rightSndBuf));
  SUCCESS_OR_DIE (gaspi_segment_ptr (segment_id_rightRxBuf, (void **)  &rightRxBuf));

  SUCCESS_OR_DIE (gaspi_segment_ptr (segment_id_topSndBuf, (void **)  &topSndBuf));
  SUCCESS_OR_DIE (gaspi_segment_ptr (segment_id_topRxBuf, (void **)  &topRxBuf));

  SUCCESS_OR_DIE (gaspi_segment_ptr (segment_id_botSndBuf, (void **)  &botSndBuf));
  SUCCESS_OR_DIE (gaspi_segment_ptr (segment_id_botRxBuf, (void **)  &botRxBuf));

  for(i = 0; i < NELEM; i++){
    // Initialize right buffer
    rightSndBuf[i] = (double)i + ((double)rank*0.1);
    rightRxBuf[i] = -1.0;

    // Initialize left buffer
    leftSndBuf[i] = (double)i + ((double)rank*0.01);
    leftRxBuf[i] = -1.0;

    // Initialize top buffer
    topSndBuf[i] = (double)i + ((double)rank*0.001);
    topRxBuf[i] = -1.0;

    // Initialize left buffer
    botSndBuf[i] = (double)i + ((double)rank*0.0001);
    botRxBuf[i] = -1.0;
  }

  if ( rank == 0 ){
    fprintf(stdout, "running with %d GPI ranks w/ %d iterations \n", size, NITER);
  }


  SUCCESS_OR_DIE(gaspi_barrier (GASPI_GROUP_ALL, GASPI_BLOCK));

  SUCCESS_OR_DIE (gaspi_queue_size_max (&queue_size_max));

  // Start testing NITER times
  for (k = 0; k < NITER; k++) {
    SUCCESS_OR_DIE (gaspi_queue_size (queue_id, &queue_size));

    if(queue_size == queue_size_max){
      SUCCESS_OR_DIE (gaspi_wait(queue_id, GASPI_BLOCK));
    }

    gettimeofday(&ti[0], NULL);

    // Sending right buffer
    SUCCESS_OR_DIE(gaspi_write_notify(segment_id_rightSndBuf, off, peer_right, segment_id_rightRxBuf, off, NELEM*sizeof(double), reqRight,  rank+1, queue_id, GASPI_BLOCK));

    // Receiving right buffer
    SUCCESS_OR_DIE(gaspi_notify_waitsome(segment_id_rightRxBuf, reqRight, 1, &id, GASPI_BLOCK));
    SUCCESS_OR_DIE (gaspi_notify_reset (segment_id_rightRxBuf, id, &value));

    gettimeofday(&tf[0], NULL);

    dT[0] += (double)(tf[0].tv_sec  - ti[0].tv_sec )*1.e6 +
      (double)(tf[0].tv_usec - ti[0].tv_usec);


    gettimeofday(&ti[1], NULL);

    SUCCESS_OR_DIE (gaspi_queue_size (queue_id, &queue_size));

    if(queue_size == queue_size_max){
      SUCCESS_OR_DIE (gaspi_wait(queue_id, GASPI_BLOCK));
    }

    // Sending left buffer
    SUCCESS_OR_DIE(gaspi_write_notify(segment_id_leftSndBuf, off, peer_left, segment_id_leftRxBuf, off, NELEM*sizeof(double), reqLeft,  rank+1, queue_id, GASPI_BLOCK));

    // Receiving left buffer
    SUCCESS_OR_DIE(gaspi_notify_waitsome(segment_id_leftRxBuf, reqLeft, 1, &id, GASPI_BLOCK));
    SUCCESS_OR_DIE (gaspi_notify_reset (segment_id_leftRxBuf, id, &value));

    gettimeofday(&tf[1], NULL);

    dT[1] += (double)(tf[1].tv_sec  - ti[1].tv_sec )*1.e6 +
      (double)(tf[1].tv_usec - ti[1].tv_usec);

    if ( NPIY > 1 ) {
      SUCCESS_OR_DIE (gaspi_queue_size (queue_id, &queue_size));

      if(queue_size == queue_size_max){
        SUCCESS_OR_DIE (gaspi_wait(queue_id, GASPI_BLOCK));
      }

      gettimeofday(&ti[2], NULL);

      // Sending top buffer
      SUCCESS_OR_DIE(gaspi_write_notify(segment_id_topSndBuf, off, peer_bot, segment_id_topRxBuf, off, NELEM*sizeof(double), reqTop,  rank+1, queue_id, GASPI_BLOCK));

      // Receiving top buffer
      SUCCESS_OR_DIE(gaspi_notify_waitsome(segment_id_topRxBuf, reqTop, 1, &id, GASPI_BLOCK));
      SUCCESS_OR_DIE (gaspi_notify_reset (segment_id_topRxBuf, id, &value));

      gettimeofday(&tf[2], NULL);

      dT[2] += (double)(tf[2].tv_sec  - ti[2].tv_sec )*1.e6 +
        (double)(tf[2].tv_usec - ti[2].tv_usec);

      SUCCESS_OR_DIE (gaspi_queue_size (queue_id, &queue_size));

      if(queue_size == queue_size_max){
        SUCCESS_OR_DIE (gaspi_wait(queue_id, GASPI_BLOCK));
      }

      gettimeofday(&ti[3], NULL);

      // Sending bottom buffer
      SUCCESS_OR_DIE(gaspi_write_notify(segment_id_botSndBuf, off, peer_top, segment_id_botRxBuf, off, NELEM*sizeof(double), reqBot,  rank+1, queue_id, GASPI_BLOCK));

      // Receiving bottom buffer
      SUCCESS_OR_DIE(gaspi_notify_waitsome(segment_id_botRxBuf, reqBot, 1, &id, GASPI_BLOCK));
      SUCCESS_OR_DIE (gaspi_notify_reset (segment_id_botRxBuf, id, &value));

      gettimeofday(&tf[3], NULL);

      dT[3] += (double)(tf[3].tv_sec  - ti[3].tv_sec )*1.e6 +
        (double)(tf[3].tv_usec - ti[3].tv_usec);
    }

    leftError=0;
    rightError=0;
    topError=0;
    botError=0;

    for(i = 0; i < NELEM; i++){

      // Checking right buffer
      double rightExpData = (double)i+ ((double)peer_left*0.1);

      if ( rightRxBuf[i] != rightExpData) {
        rightError++;
      }

      rightRxBuf[i]=-1.0;

      // Checking left buffer
      double leftExpData = (double)i+ ((double)peer_right*0.01);

      if ( leftRxBuf[i] != leftExpData) {
        leftError++;
      }

      leftRxBuf[i]=-1.0;


      if ( NPIY > 1 ) {

        // Checking top buffer
        double topExpData = (double)i+ ((double)peer_top*0.001);

        if ( topRxBuf[i] != topExpData) {
          topError++;
        }

        topRxBuf[i]=-1.0;


        // Checking bot buffer
        double botExpData = (double)i+ ((double)peer_bot*0.0001);

        if ( botRxBuf[i] != botExpData) {
          botError++;
        }

        botRxBuf[i]=-1.0;
      }

    }

    if ( leftError > 0 ) {
      leftNfailure++;
    }

    if ( rightError > 0 ) {
      rightNfailure++;
    }

    if ( NPIY > 1 ) {
      if ( topError > 0 ) {
        topNfailure++;
      }
      if ( botError > 0 ) {
        botNfailure++;
      }
    }

    SUCCESS_OR_DIE(gaspi_barrier (GASPI_GROUP_ALL, GASPI_BLOCK));

  } // End for of NITER tests

  // Printing results of error checking
  for (i=0; i<size; i++) {
    if ( i == rank ) {
      if ( leftNfailure > 0 ) 
        fprintf(stderr,"RANK%d: %d/%d FAILED LEFT BUFFER\n", rank, leftNfailure, NITER);
      else 
        fprintf(stdout,"RANK%d: SUCCESS LEFT BUFFER TOOK ON AVG %.2fs\n", rank, dT[0]/(double)NITER);


      if ( rightNfailure > 0 ) 
        fprintf(stderr,"RANK%d: %d/%d FAILED RIGHT BUFFER\n", rank, rightNfailure, NITER);
      else 
        fprintf(stdout,"RANK%d: SUCCESS RIGHT BUFFER TOOK ON AVG %.2fs\n", rank, dT[1]/(double)NITER);

      if ( NPIY > 1 ) {
        if ( topNfailure > 0 ) 
          fprintf(stderr,"RANK%d: %d/%d FAILED TOP BUFFER\n", rank, topNfailure, NITER);
        else 
          fprintf(stdout,"RANK%d: SUCCESS TOP BUFFER TOOK ON AVG %.2fs\n", rank, dT[2]/(double)NITER);

        if ( botNfailure > 0 ) 
          fprintf(stderr,"RANK%d: %d/%d FAILED BOT BUFFER\n", rank, botNfailure, NITER);
        else 
          fprintf(stdout,"RANK%d: SUCCESS BOT BUFFER TOOK ON AVG %.2fs\n", rank, dT[3]/(double)NITER);
      }  
    }
    SUCCESS_OR_DIE(gaspi_barrier (GASPI_GROUP_ALL, GASPI_BLOCK));
  }

  SUCCESS_OR_DIE (gaspi_proc_term (GASPI_BLOCK));

  return - leftError - rightError - topError - botError;
}
