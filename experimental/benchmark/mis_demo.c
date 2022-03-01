//------------------------------------------------------------------------------
// LAGraph/experimental/benchmark/mis_demo.c: benchmark for triangle centrality
//------------------------------------------------------------------------------

// LAGraph, (c) 2021 by The LAGraph Contributors, All Rights Reserved.
// SPDX-License-Identifier: BSD-2-Clause
// See additional acknowledgments in the LICENSE file,
// or contact permission@sei.cmu.edu for the full terms.

//------------------------------------------------------------------------------

// Contributed by Tim Davis, Texas A&M

// Usage:  mis_demo < matrixmarketfile.mtx
//         mis_demo matrixmarketfile.mtx
//         mis_demo matrixmarketfile.grb

#include "../../src/benchmark/LAGraph_demo.h"
#include "LAGraphX.h"
#include "LG_Xtest.h"

// #define NTHREAD_LIST 2

#define NTHREAD_LIST 1
#define THREAD_LIST 0

// #define NTHREAD_LIST 6
// #define THREAD_LIST 64, 32, 24, 12, 8, 4

// #define NTHREAD_LIST 7
// #define THREAD_LIST 40, 20, 16, 8, 4, 2, 1

#define LG_FREE_ALL                 \
{                                   \
    LAGraph_Delete (&G, NULL) ;     \
    GrB_free (&A) ;                 \
    GrB_free (&mis) ;               \
}

int main (int argc, char **argv)
{

    //--------------------------------------------------------------------------
    // initialize LAGraph and GraphBLAS
    //--------------------------------------------------------------------------

    char msg [LAGRAPH_MSG_LEN] ;

    GrB_Vector mis = NULL ;
    GrB_Matrix A = NULL ;
    LAGraph_Graph G = NULL ;

    // start GraphBLAS and LAGraph
    bool burble = false ;
    demo_init (burble) ;
    LAGraph_TRY (LAGraph_Random_Init (msg)) ;

    int ntrials = 3 ;
    ntrials = 3 ;
    printf ("# of trials: %d\n", ntrials) ;

    int nt = NTHREAD_LIST ;
    int Nthreads [20] = { 0, THREAD_LIST } ;
    int nthreads_max ;
    LAGraph_TRY (LAGraph_GetNumThreads (&nthreads_max, NULL)) ;
    if (Nthreads [1] == 0)
    {
        // create thread list automatically
        Nthreads [1] = nthreads_max ;
        for (int t = 2 ; t <= nt ; t++)
        {
            Nthreads [t] = Nthreads [t-1] / 2 ;
            if (Nthreads [t] == 0) nt = t-1 ;
        }
    }
    printf ("threads to test: ") ;
    for (int t = 1 ; t <= nt ; t++)
    {
        int nthreads = Nthreads [t] ;
        if (nthreads > nthreads_max) continue ;
        printf (" %d", nthreads) ;
    }
    printf ("\n") ;

    //--------------------------------------------------------------------------
    // read in the graph
    //--------------------------------------------------------------------------

    char *matrix_name = (argc > 1) ? argv [1] : "stdin" ;
    LAGraph_TRY (readproblem (&G, NULL,
        true, true, true, NULL, false, argc, argv)) ;

    GrB_Index n, nvals ;
    GrB_TRY (GrB_Matrix_nrows (&n, G->A)) ;
    GrB_TRY (GrB_Matrix_nvals (&nvals, G->A)) ;
    // LAGraph_TRY (LAGraph_DisplayGraph (G, LAGraph_SHORT, stdout, msg)) ;
    LAGraph_TRY (LAGraph_Property_RowDegree (G, msg)) ;

    //--------------------------------------------------------------------------
    // maximal independent set
    //--------------------------------------------------------------------------

    // warmup for more accurate timing
    double tic [2], tt ;
    LAGraph_TRY (LAGraph_Tic (tic, NULL)) ;
    LAGraph_TRY (LAGraph_MaximalIndependentSet (&mis, G, 1, NULL, msg)) ;
    LAGraph_TRY (LAGraph_Toc (&tt, tic, NULL)) ;
    LAGraph_TRY (LG_check_mis (G->A, mis, NULL, msg)) ;
    GrB_TRY (GrB_free (&mis)) ;
    printf ("warmup time %g sec\n", tt) ;

    for (int t = 1 ; t <= nt ; t++)
    {
        int nthreads = Nthreads [t] ;
        if (nthreads > nthreads_max) continue ;
        LAGraph_TRY (LAGraph_SetNumThreads (nthreads, msg)) ;
        double ttot = 0, ttrial [100] ;
        for (int trial = 0 ; trial < ntrials ; trial++)
        {
            int64_t seed = trial * n + 1 ;
            LAGraph_TRY (LAGraph_Tic (tic, NULL)) ;
            LAGraph_TRY (LAGraph_MaximalIndependentSet (&mis, G, seed, NULL,
                msg)) ;
            LAGraph_TRY (LG_check_mis (G->A, mis, NULL, msg)) ;
            GrB_TRY (GrB_free (&mis)) ;
            LAGraph_TRY (LAGraph_Toc (&ttrial [trial], tic, NULL)) ;
            ttot += ttrial [trial] ;
            printf ("seed %g threads %2d trial %2d: %12.6f sec\n",
                (double) seed, nthreads, trial, ttrial [trial]) ;
            fprintf (stderr,
                "seed %g threads %2d trial %2d: %12.6f sec\n",
                (double) seed, nthreads, trial, ttrial [trial]) ;
        }
        ttot = ttot / ntrials ;

        printf ("Avg: MIS nthreads: %3d time: %12.6f matrix: %s\n",
            nthreads, ttot, matrix_name) ;

        fprintf (stderr, "Avg: MIS nthreads: %3d time: %12.6f matrix: %s\n",
            nthreads, ttot, matrix_name) ;
    }

    fflush (stdout) ;
    LG_FREE_ALL ;
    LAGraph_TRY (LAGraph_Random_Finalize (msg)) ;
    LAGraph_TRY (LAGraph_Finalize (msg)) ;
    return (GrB_SUCCESS) ;
}

