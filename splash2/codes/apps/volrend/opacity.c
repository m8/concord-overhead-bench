#line 270 "../../../../splash2/codes//null_macros/c.m4.null"

#line 1 "opacity.C"
/*************************************************************************/
/*                                                                       */
/*  Copyright (c) 1994 Stanford University                               */
/*                                                                       */
/*  All rights reserved.                                                 */
/*                                                                       */
/*  Permission is given to use, copy, and modify this software for any   */
/*  non-commercial purpose as long as this copyright notice is not       */
/*  removed.  All other uses, including redistribution in whole or in    */
/*  part, are forbidden without prior written permission.                */
/*                                                                       */
/*  This software is provided with absolutely no warranty and no         */
/*  support.                                                             */
/*                                                                       */
/*************************************************************************/

/******************************************************************************
*                                                                             *
*    opacity.c:  Compute opacity map using region boundary method.  Shading   *
*              transition width used is zero.                                 *
*                                                                             *
******************************************************************************/

#include <string.h>
#include "incl.h"

/* The following declarations show the layout of the .opc file.              */
/* If changed, the version number must be incremented and code               */
/* written to handle loading of both old and current versions.               */

				/* Version for new .opc files:               */
#define	OPC_CUR_VERSION   1	/*   Initial release                         */
short opc_version;		/* Version of this .opc file                 */

short opc_len[NM];	        /* Size of this opacity map                  */

int opc_length;		/* Total number of opacities in map          */
				/*   (= product of lens)                     */
OPACITY *opc_address;	        /* Pointer to opacity map                    */

/* End of layout of .opc file.                                               */


#line 43
#include <pthread.h>
#line 43
#include <sys/time.h>
#line 43
#include <unistd.h>
#line 43
#include <stdlib.h>
#line 43
#include "TriggerActionDecl.h"
#line 43
extern pthread_t PThreadTable[];
#line 43


#include "anl.h"

void Compute_Opacity()
{
  long i;

  /* to allow room for gradient operator plus 1-voxel margin   */
  /* of zeros if shading transition width > 0.  Zero voxels    */
  /* are independent of input map and can be outside inset.    */
  for (i=0; i<NM; i++) {
    opc_len[i] = map_len[i] - 2*INSET;
  }
  opc_length = opc_len[X] * opc_len[Y] * opc_len[Z];
  Allocate_Opacity(&opc_address, opc_length);

  printf("    Computing opacity map...\n");

  Global->Index = NODE0;

  int curr_index = thrIndex;
#ifndef SERIAL_PREPROC
  for (i=1; i<num_nodes; i++) {
#line 66
	long	i, Error;
#line 66

#line 66
	for (i = 0; i < () - 1; i++) {
#line 66
		Error = pthread_create(&PThreadTable[i], NULL, (void * (*)(void *))(Opacity_Compute), (void*)thrIndex);
#line 66
    thrIndex++;
#line 66
		if (Error != 0) {
#line 66
			printf("Error in pthread_create().\n");
#line 66
			exit(-1);
#line 66
		}
#line 66
	}
#line 66
	Opacity_Compute((void*)0);
#line 66
}
#endif

  Opacity_Compute(curr_index);
}


void Allocate_Opacity(address, length)
     OPACITY **address;
     long length;
{
  long i;

  printf("    Allocating opacity map of %ld bytes...\n",
	 length*sizeof(OPACITY));

  *address = (OPACITY *)malloc(length*sizeof(OPACITY));;

  if (*address == NULL)
    Error("    No space available for map.\n");

/*  POSSIBLE ENHANCEMENT:  Here's where one might distribute the
    opacity map among physical memories if one wanted to.
*/

  for (i=0; i<length; i++) *(*address+i) = 0;

}


void Opacity_Compute(void* vpIndex)
{
  int index = (unsigned int)vpIndex;
  {
#line 99
    init_stats(index);
#line 99
    };

  long inx,iny,inz;	        /* Voxel location in object space            */
  long outx,outy,outz;	        /* Loop indices in image space               */
  long density;
  float magnitude;
  float opacity, grd_x,grd_y,grd_z;
  long zstart,zstop;
  long num_xqueue,num_yqueue,num_zqueue,num_queue;
  long xstart,xstop,ystart,ystop;
  long my_node;

  {pthread_mutex_lock(&(Global->IndexLock));};
  my_node = Global->Index++;
  {pthread_mutex_unlock(&(Global->IndexLock));};
  my_node = my_node%num_nodes;

/*  POSSIBLE ENHANCEMENT:  Here's where one might bind the process to a
    processor, if one wanted to.
*/

  num_xqueue = ROUNDUP((float)opc_len[X]/(float)voxel_section[X]);
  num_yqueue = ROUNDUP((float)opc_len[Y]/(float)voxel_section[Y]);
  num_zqueue = ROUNDUP((float)opc_len[Z]/(float)voxel_section[Z]);
  num_queue = num_xqueue * num_yqueue * num_zqueue;
  xstart = (my_node % voxel_section[X]) * num_xqueue;
  xstop = MIN(xstart+num_xqueue,opc_len[X]);
  ystart = ((my_node / voxel_section[X]) % voxel_section[Y]) * num_yqueue;
  ystop = MIN(ystart+num_yqueue,opc_len[Y]);
  zstart = (my_node / (voxel_section[X] * voxel_section[Y])) * num_zqueue;
  zstop = MIN(zstart+num_zqueue,opc_len[Z]);

#ifdef SERIAL_PREPROC
  zstart = 0;
  zstop = opc_len[Z];
  ystart = 0;
  ystop = opc_len[Y];
  xstart = 0;
  xstop = opc_len[X];
#endif

  for (outz=zstart; outz<zstop; outz++) {
    for (outy=ystart; outy<ystop; outy++) {
      for (outx=xstart; outx<xstop; outx++) {

	inx = INSET + outx;
	iny = INSET + outy;
	inz = INSET + outz;

	density = MAP(inz,iny,inx);
	if (density > density_epsilon) {

	  grd_x = (float)((long)MAP(inz,iny,inx+1) - (long)MAP(inz,iny,inx-1));
	  grd_y = (float)((long)MAP(inz,iny+1,inx) - (long)MAP(inz,iny-1,inx));
	  grd_z = (float)((long)MAP(inz+1,iny,inx) - (long)MAP(inz-1,iny,inx));
	  magnitude = grd_x*grd_x+grd_y*grd_y+grd_z*grd_z;

	  /* If (magnitude*grd_divisor)**2 is small, skip voxel             */
	  if (magnitude > nmag_epsilon) {
	    magnitude = .5*sqrt(magnitude);
	    /* For density * magnitude (d*m) operator:                      */
	    /*   Set opacity of surface to the product of user-specified    */
	    /*   functions of local density and gradient magnitude.         */
	    /*   Detects both front and rear-facing surfaces.               */
	    opacity = density_opacity[density] *
	      magnitude_opacity[(long)magnitude];
	    /* If opacity is small, skip shading and compositing of sample  */
	    if (opacity > opacity_epsilon)
	      OPC(outz,outy,outx) = NINT(opacity*MAX_OPC);
	  }
	}
	else
	  OPC(outz,outy,outx) = MIN_OPC;
      }
    }
  }
#ifndef SERIAL_PREPROC
  {
#line 176
	unsigned long	Error, Cycle;
#line 176
	long		Cancel, Temp;
#line 176

#line 176
	Error = pthread_mutex_lock(&(Global->SlaveBarrier).mutex);
#line 176
	if (Error != 0) {
#line 176
		printf("Error while trying to get lock in barrier.\n");
#line 176
		exit(-1);
#line 176
	}
#line 176

#line 176
	Cycle = (Global->SlaveBarrier).cycle;
#line 176
	if (++(Global->SlaveBarrier).counter != (num_nodes)) {
#line 176
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &Cancel);
#line 176
		while (Cycle == (Global->SlaveBarrier).cycle) {
#line 176
			Error = pthread_cond_wait(&(Global->SlaveBarrier).cv, &(Global->SlaveBarrier).mutex);
#line 176
			if (Error != 0) {
#line 176
				break;
#line 176
			}
#line 176
		}
#line 176
		pthread_setcancelstate(Cancel, &Temp);
#line 176
	} else {
#line 176
		(Global->SlaveBarrier).cycle = !(Global->SlaveBarrier).cycle;
#line 176
		(Global->SlaveBarrier).counter = 0;
#line 176
		Error = pthread_cond_broadcast(&(Global->SlaveBarrier).cv);
#line 176
	}
#line 176
	pthread_mutex_unlock(&(Global->SlaveBarrier).mutex);
#line 176
};
#endif
  print_timing_stats();
}


void Load_Opacity(filename)
     char filename[];
{
  char local_filename[FILENAME_STRING_SIZE];
  int fd;

  strcpy(local_filename,filename);
  strcat(local_filename,".opc");
  fd = Open_File(local_filename);

  Read_Shorts(fd,(unsigned char *)&opc_version, (long)sizeof(opc_version));
  if (opc_version != OPC_CUR_VERSION)
    Error("    Can't load version %d file\n",opc_version);

  Read_Shorts(fd,(unsigned char *)opc_len,(long)sizeof(map_len));

  Read_Longs(fd,(unsigned char *)&opc_length,(long)sizeof(opc_length));

  Allocate_Opacity(&opc_address,opc_length);

  printf("    Loading opacity map from .opc file...\n");
  Read_Bytes(fd,(unsigned char *)opc_address,(long)(opc_length*sizeof(OPACITY)));
  Close_File(fd);
}


void Store_Opacity(filename)
char filename[];
{
  char local_filename[FILENAME_STRING_SIZE];
  int fd;

  strcpy(local_filename,filename);
  strcat(local_filename,".opc");
  fd = Create_File(local_filename);

  opc_version = OPC_CUR_VERSION;
  strcpy(local_filename,filename);
  strcat(local_filename,".opc");
  fd = Create_File(local_filename);
  Write_Shorts(fd,(unsigned char *)&opc_version,(long)sizeof(opc_version));

  Write_Shorts(fd,(unsigned char *)opc_len,(long)sizeof(opc_len));
  Write_Longs(fd,(unsigned char *)&opc_length,(long)sizeof(opc_length));

  printf("    Storing opacity map into .opc file...\n");
  Write_Bytes(fd,(unsigned char *)opc_address,(long)(opc_length*sizeof(OPACITY)));
  Close_File(fd);
}


void Deallocate_Opacity(address)
OPACITY **address;
{
  printf("    Deallocating opacity map...\n");

/*  free(*address);;  */

  *address = NULL;
}
