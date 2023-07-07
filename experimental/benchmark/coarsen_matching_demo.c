#include "../../src/benchmark/LAGraph_demo.h"
#include "LG_internal.h"
#include "LAGraphX.h"

#define VERBOSE

#define DEFAULT_SIZE 10000
#define DEFAULT_DENSITY 0.5
#define DEFAULT_SEED 42

#define NTHREAD_LIST 1
#define THREAD_LIST 8

int main(int argc, char **argv)
{
    /*
    strategy:
    either stdin, file, or random
    */
    char msg [LAGRAPH_MSG_LEN] ;

    LAGraph_Graph G = NULL ;

    bool burble = false ; 
    demo_init (burble) ;

    //--------------------------------------------------------------------------
    // read in/build the graph
    //--------------------------------------------------------------------------
    char *matrix_name = (argc > 1) ? argv [1] : "stdin" ;

    // using -r will build a random graph
    bool random = (strcmp (matrix_name, "-r") == 0) ;
    bool force_stdin = (strcmp (matrix_name, "stdin") == 0) ;

    LG_TRY (LAGraph_Random_Init (msg)) ;

    if (!random) {
        LG_TRY (readproblem (&G, NULL,
            true, true, false, GrB_FP64, false, force_stdin ? 1 : argc, argv)) ;
    } else {
        GrB_Index n = (argc > 2 ? atoi (argv [2]) : DEFAULT_SIZE) ;
        double density = (argc > 3 ? atof (argv [3]) : DEFAULT_DENSITY) ;
        uint64_t seed = (argc > 4 ? atoll (argv [4]) : DEFAULT_SEED) ;
        
        GrB_Matrix A = NULL ;

        LG_TRY (LAGraph_Random_Matrix (&A, GrB_FP64, n, n, density, seed, msg)) ;
        GRB_TRY (GrB_eWiseAdd (A, NULL, NULL, GrB_PLUS_FP64, A, A, GrB_DESC_T1)) ;
        
        LG_TRY (LAGraph_New (&G, &A, LAGraph_ADJACENCY_UNDIRECTED, msg)) ;
        LG_TRY (LAGraph_Cached_NSelfEdges (G, msg)) ;
        LG_TRY (LAGraph_DeleteSelfEdges (G, msg)) ;
        // LG_TRY (LAGraph_Graph_Print (G, LAGraph_COMPLETE, stdout, msg)) ;
    }
    GrB_Index n ;
    GRB_TRY (GrB_Matrix_nrows (&n, G->A)) ;
    GrB_Matrix coarsened = NULL ;
    GrB_Vector *parent_result = NULL , *newlabels_result = NULL ;

    int nt = NTHREAD_LIST ;
    
    int Nthreads [20] = { 0, THREAD_LIST } ;
    int nthreads_max, nthreads_outer, nthreads_inner ;
    LG_TRY (LAGraph_GetNumThreads (&nthreads_outer, &nthreads_inner, NULL)) ;
#ifdef VERBOSE
    printf("nthreads_outer: %d, nthreads_inner: %d\n", nthreads_outer, nthreads_inner);
#endif
    nthreads_max = nthreads_outer * nthreads_inner ;
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
#ifdef VERBOSE
    printf ("threads to test: ") ;
#endif
    for (int t = 1 ; t <= nt ; t++)
    {
        int nthreads = Nthreads [t] ;
        if (nthreads > nthreads_max) continue ;
#ifdef VERBOSE
        printf (" %d", nthreads) ;
#endif
    }
#ifdef VERBOSE
    printf ("\n") ;
#endif

    // warmup for more accurate timing
    double tt = LAGraph_WallClockTime ( ) ;
    // GRB_TRY (LAGraph_Matrix_Print (E, LAGraph_COMPLETE, stdout, msg)) ;
    int res = (LAGraph_Coarsen_Matching (&coarsened, &parent_result, &newlabels_result, G, LAGraph_Matching_heavy, 0, 1, 1, DEFAULT_SEED, msg)) ;

    tt = LAGraph_WallClockTime ( ) - tt ;

    GRB_TRY (GrB_free (&coarsened)) ;
    GRB_TRY (GrB_free (parent_result)) ; // free vector (first list element)
    GRB_TRY (GrB_free (newlabels_result)) ;
    LG_TRY (LAGraph_Free ((void**)(&parent_result), msg)) ; // free pointer to list
    LG_TRY (LAGraph_Free ((void**)(&newlabels_result), msg)) ;
#ifdef VERBOSE
    printf ("warmup time %g sec\n", tt) ;
#endif

    // the GAP benchmark requires 16 trials
    int ntrials = 16 ;
    // ntrials = 1 ;    // HACK to run just one trial
#ifdef VERBOSE
    printf ("# of trials: %d\n", ntrials) ;
#endif

    for (int kk = 1 ; kk <= nt ; kk++)
    {
        int nthreads = Nthreads [kk] ;
        if (nthreads > nthreads_max) continue ;
        LG_TRY (LAGraph_SetNumThreads (1, nthreads, msg)) ;

#ifdef VERBOSE
        printf ("\n--------------------------- nthreads: %2d\n", nthreads) ;
#endif

        double total_time = 0 ;

        for (int trial = 0 ; trial < ntrials ; trial++)
        {
            int64_t seed = trial * n + 1 ;
            double tt = LAGraph_WallClockTime ( ) ;

            LG_TRY (LAGraph_Coarsen_Matching (&coarsened, &parent_result, &newlabels_result, G, LAGraph_Matching_heavy, 0, 1, 1, DEFAULT_SEED, msg)) ;

            tt = LAGraph_WallClockTime ( ) - tt ;

            GRB_TRY (GrB_free (&coarsened)) ;
            GRB_TRY (GrB_free (parent_result)) ; // free vector (first list element)
            GRB_TRY (GrB_free (newlabels_result)) ;
            LG_TRY (LAGraph_Free ((void**)(&parent_result), msg)) ;    // free pointer to list
            LG_TRY (LAGraph_Free ((void**)(&newlabels_result), msg)) ;
#ifdef VERBOSE
            printf ("trial: %2d time: %10.7f sec\n", trial, tt) ;
#endif
            total_time += tt ;
        }

        double t = total_time / ntrials ;

#ifndef VERBOSE
        printf("%.7f\n", t);
#endif

#ifdef VERBOSE
        printf ("single-level coarsening (heavy, nopreserve, combine): %3d: avg time: %10.7f (sec) matrix: %s\n",
                nthreads, t, (random ? "random" : matrix_name)) ;
        fprintf (stderr, "single-level coarsening (heavy, nopreserve, combine): %3d: avg time: %10.7f (sec) matrix: %s\n",
                nthreads, t, (random ? "random" : matrix_name)) ;
#endif
    }

    //--------------------------------------------------------------------------
    // free all workspace and finish
    //--------------------------------------------------------------------------
    LG_FREE_ALL ;

    LG_TRY (LAGraph_Finalize (msg)) ;
    return (GrB_SUCCESS) ;

    /*
    GrB_Vector *all_parents, *all_mappings ;
    GrB_Matrix coarsened ;

    GrB_Matrix A = G->A ;

    LG_TRY (LAGraph_Coarsen_Matching (&coarsened, &all_parents, &all_mappings, G, LAGraph_Matching_random, 0, 1, 1, 17, msg)) ;
    LG_TRY (LAGraph_Matrix_Print (coarsened, LAGraph_COMPLETE, stdout, msg)) ;
    // LG_TRY (LAGraph_Vector_Print (all_parents[0], LAGraph_COMPLETE, stdout, msg)) ;
    // LG_TRY (LAGraph_Vector_Print (all_mappings[0], LAGraph_COMPLETE, stdout, msg)) ;
    /*
    char msg[1024] ;
    LAGraph_Init (msg) ;
    LAGraph_Random_Init (msg) ;
    GrB_Matrix test = NULL , test2 = NULL ;
    GRB_TRY (LAGraph_Random_Matrix (&test, GrB_BOOL, 3, 5, 0.5, 42, msg)) ;
    GRB_TRY (LAGraph_Random_Matrix (&test2, GrB_BOOL, 5, 3, 0.2, 93, msg)) ;
    GRB_TRY (GrB_transpose (test2, NULL, NULL, test, NULL)) ;
    return (GrB_SUCCESS) ;

    3 3 4
    1 2 1.7
    1 3 0.5
    2 1 1.7
    3 1 0.5
    */
}