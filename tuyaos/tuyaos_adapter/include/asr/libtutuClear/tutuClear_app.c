// /*
//  * tutuClear_app.c
//  *
//  * History: 
//  * 
//  *  
//  */

// #define __EN_EXTERNALLY_ALLOCATION__ // enable this define for externally malloc TUTU objects
// #define __EN_DIAGNOSTICS__ //enable this to have the proper internal returned diagnostical information
// #define __EN_LIBSNDFILE__

// #define PCMIN_WAV_FILE   "xiaozhiguanjia_test.wav"

// // ----------------------------------------------------------------------------
// #include <stdlib.h>
// #include <stdio.h>
// #include <string.h>

// #ifdef _MSC_VER
// #pragma warning( disable : 4996 )
// #include "conio.h"
// #endif // _MSC_VER

// #include "tutuClear.h"

// #ifdef __EN_LIBSNDFILE__
// #include "sndfile.h"
// #endif // __EN_LIBSNDFILE__

// // ----------------------------------------------------------------------------
// int main(int argc,char *argv[])
// {
// 	//define Objects
//     void                    *pTUTUClearObject = NULL;
//     void                    *pExternallyAllocatedMem = NULL;
// 	//define input/output data buffers
// 	void					*pMicSig = NULL;

// 	//use files for data I/O 
// #ifdef __EN_LIBSNDFILE__
// 	SF_INFO 				tSfInfo;
// 	SNDFILE 				*pSndFileMicSig = NULL;
// #else // __EN_LIBSNDFILE__
// 	FILE                    *pfMicSig0 = NULL;
// #endif // __EN_LIBSNDFILE__
	
// 	int						iBfrSz;
// 	int						i;
// 	int						w32SmplSzInByte = 2;


// #ifdef __EN_LIBSNDFILE__
// 	tSfInfo.format = 0;
// 	if ((pSndFileMicSig = sf_open(PCMIN_WAV_FILE, SFM_READ, &tSfInfo)) == NULL)
// #else // __EN_LIBSNDFILE__
// 	if ((pfMicSig0 = fopen(acMICSIG, "rb")) == NULL)
// #endif // __EN_LIBSNDFILE__
//     {
//         printf("Error open input file %s to read!\r\n", PCMIN_WAV_FILE);
// #ifdef _MSC_VER
//         printf("Press any key to quit.\n");
//         getche();    
// #endif // _MSC_VER
//         return 0;
//     }

// 	/* =======================================================================
//       1. memory allocation
// 	======================================================================== */
// #ifdef __EN_EXTERNALLY_ALLOCATION__
// 	i = TUTUClear_QueryMemSz();
// 	printf("tutuClear DM usage = %d bytes\n", i);
//     pExternallyAllocatedMem = malloc(i);
// #endif // __EN_EXTERNALLY_ALLOCATION__


// 	/* =======================================================================
// 	  2. wakeup state initialization
// 	======================================================================== */
// 	iBfrSz = 320; //20ms stream data
//     pMicSig = (void *)malloc(iBfrSz*2);

//     if ((TUTUClear_Init(pExternallyAllocatedMem,
// 						&pTUTUClearObject)) != TUTU_OK)
//     {
//         printf("Fail to do TUTUClear_Init.\n");
// #ifdef _MSC_VER
//         printf("Press any key to quit.\n");
//         getche();    
// #endif // _MSC_VER
//         return 0;
//     }
    

//     /* ========================================================================
//        3. Processing loop
//     ======================================================================== */

//     i = 0;
	
//     while (1)
//     {
// 		W32 w32WakeWord;
//         // Prepare input buffers


// #ifdef __EN_LIBSNDFILE__

// 		if (sf_readf_short(pSndFileMicSig, pMicSig, iBfrSz) != iBfrSz) break;
// #else // __EN_LIBSNDFILE__
// 		if (fread(pMicSig, w32SmplSzInByte, iBfrSz * 2, pfMicSig0) != iBfrSz) break;
// #endif // __EN_LIBSNDFILE__

// 		TUTUClear_OneFrame(pTUTUClearObject, //accepting 20ms pcm stream
// 			(W16*)pMicSig,
// 			&w32WakeWord);

// 		if (w32WakeWord != 0)//check wakeup
// 		{
// 			printf("keyword spotted!\n");
// 		}

// 		i ++;
// 		printf("Output buffers of %4d frame(s) are dispatched.\r", i);

//     }

// 	printf("\r");

// 	/* ========================================================================
// 	 delete Objects and release workspace 
//     ==========================================================================*/
// 	TUTUClear_Release(&pTUTUClearObject);

// 	// release buffer
//     if (pExternallyAllocatedMem!=NULL) free(pExternallyAllocatedMem);
//     if (pMicSig!=NULL) free(pMicSig);


// #ifdef __EN_LIBSNDFILE__
// 	if (pSndFileMicSig != NULL) sf_close(pSndFileMicSig);
// #else // __EN_LIBSNDFILE__
// 	if (pfMicSig0 != NULL) fclose(pfMicSig0);
// #endif // __EN_LIBSNDFILE__

// #if defined(_MSC_VER) 
// 	printf("\nPress any key to continue...\n");
// 	getche();
// #endif // _MSC_VER

//     return 1;
// }
