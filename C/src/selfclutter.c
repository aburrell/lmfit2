/* selfclutter.c
   ==============

Ashton Reimer

ISAS
Last Update: August 2016
*/

#include "fit_structures.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>


/***********************************************************************************************/
/***********************************************************************************************/

/* Maximal lag0 Power Based Self-Clutter Estimator */
void Estimate_Maximum_Self_Clutter(int gate, FITPRMS *prm, double *pwr0,
  	  double *self_clutter) {
   /* 

      gate          - the range gate to calculate the self-clutter for
      *prm          - pointer to the radar parameter structure
      *pwr0         - pointer to array of lag0 power (in mag units, NOT DB) at each range
      *self_clutter - pointer to array to store output maximal self-clutter of each lag

      Example usage in FITACF (in the do_fit.c code after ptr[i].p_0 is set to SNR in units of DB around line 140):
      ***********************************************
      int status;
      int r;
      self_clutter = malloc(sizeof(float)*iptr->prm.mplgs);
      
      badrng=ACFBadLagZero(&iptr->prm,iptr->prm.mplgs,&iptr->prm.lag);

      for (r=0;r<iptr->prm.nrang;r++) {
          status = Cmpse(&iptr->prm, &iptr->prm.lag, r, &self_clutter, badrng, &pwrd);
          SOME_FUNCTION_THAT_USES_SELF-CLUTTER (ie. to make error bars)
      }
      free(self_clutter);
   */


   /* constants from prm structure */
   int nrang = prm->nrang;
   int smpfr = (prm->lagfr / prm->smsep); /*number of voltage samples to the first range*/
   int mppul = prm->mppul;                /*number of pulses in the pulse sequence*/
   int mplgs = prm->mplgs;
   int tp_in_tau = (prm->mpinc / prm->smsep);

   /* for loop indicies */
   int lag;
   int pul,pul1,pul2;

   /* temporary storage variables */
   int S1,S2;
   float term1, term2, term3;
   int r1[prm->mppul],r2[prm->mppul];           /*initialize integer arrays*/
   int temp;

   /*if (badrange < 0)*/
   int badrange = nrang;

   for(lag=0;lag < mplgs; lag++) {

       /* First, initialize self_clutter power to 0 */
       self_clutter[lag] = 0;

       /* Determine which ranges are intefering in the current lag */
       /* samples are we using for the current lag*/
       S1=tp_in_tau*prm->lag[0][lag] + gate + smpfr;
       S2=tp_in_tau*prm->lag[1][lag] + gate + smpfr;

       for (pul=0; pul < mppul; pul++) {
           /*Find the pulses that were transmitted before the samples were recorded
             and then save which range gates each pulse is coming from. */
           if (prm->pulse[pul]*tp_in_tau <= S1){
               temp = (S1 - prm->pulse[pul]*tp_in_tau - smpfr);
               /*Also we need to check and make sure we only save interfering range 
                 gates where we have valid lag0 power.*/
               if ((temp != gate) && (temp >= 0) && (temp < nrang) && (temp < badrange)) {
                   r1[pul]= temp;
               } else {
                   r1[pul]=-1000;
               }
           } else {
               r1[pul]=-1000;
           }
           if (prm->pulse[pul]*tp_in_tau <= S2){
               temp = (S2 - prm->pulse[pul]*tp_in_tau - smpfr);
               if ((temp != gate) && (temp >= 0) && (temp < nrang) && (temp < badrange)) {
                   r2[pul]= temp;
               } else {
                   r2[pul]=-1000;
               }
           } else {
               r2[pul]=-1000;
           }
       }

       /* initialize temporary variables */
       term1 = 0;
       term2 = 0;
       term3 = 0;

       /*First term in the summation for the self-clutter estimate (P_r*P_j^*)*/
       for (pul=0; pul < mppul; pul++) {
           if (r2[pul] != -1000) {
               term1 += sqrt(pwr0[gate]*pwr0[r2[pul]]);
           }
       }

       /*Second term in the summation for the self-clutter estimate (P_i*P_r^*)*/
       for (pul=0; pul < mppul; pul++) {
           if (r1[pul] != -1000) {
               term2 += sqrt(pwr0[gate]*pwr0[r1[pul]]);
           }
       }

       /*Third term in the summation for the self-clutter estimate (P_i*P_j^*)*/
       for (pul1=0; pul1 < mppul; pul1++) {
           for (pul2=0; pul2 < mppul; pul2++) {
               if ((r1[pul1] != -1000) && (r2[pul2] != -1000)) {
                   term3 += sqrt(pwr0[r1[pul1]]*pwr0[r2[pul2]]);
               }
           }
       }

       self_clutter[lag] = term1 + term2 + term3;

   } 
} 


