//----------------------------------------------------------------------------
// LAGraph/src/test/test_SWrite.c: test cases for LAGraph_SWrite and SRead
// ----------------------------------------------------------------------------

// LAGraph, (c) 2021 by The LAGraph Contributors, All Rights Reserved.
// SPDX-License-Identifier: BSD-2-Clause
// See additional acknowledgments in the LICENSE file,
// or contact permission@sei.cmu.edu for the full terms.

//-----------------------------------------------------------------------------

#include <stdio.h>
#include <acutest.h>
#include <LAGraphX.h>
#include <LAGraph_test.h>

char msg [LAGRAPH_MSG_LEN] ;
LAGraph_Graph G = NULL ;
GrB_Matrix A = NULL ;
GrB_Matrix B = NULL ;
GrB_Matrix *S = NULL ;

#define LEN 512
char filename [LEN+1] ;

#define NFILES 51
const char *files [ ] =
{
    "A.mtx",
    "cover.mtx",
    "cover_structure.mtx",
    "jagmesh7.mtx",
    "ldbc-cdlp-directed-example.mtx",
    "ldbc-cdlp-undirected-example.mtx",
    "ldbc-directed-example-bool.mtx",
    "ldbc-directed-example.mtx",
    "ldbc-directed-example-unweighted.mtx",
    "ldbc-undirected-example-bool.mtx",
    "ldbc-undirected-example.mtx",
    "ldbc-undirected-example-unweighted.mtx",
    "ldbc-wcc-example.mtx",
    "LFAT5.mtx",
    "msf1.mtx",
    "msf2.mtx",
    "msf3.mtx",
    "sample2.mtx",
    "sample.mtx",
    "sources_7.mtx",
    "olm1000.mtx",
    "bcsstk13.mtx",
    "cryg2500.mtx",
    "tree-example.mtx",
    "west0067.mtx",
    "lp_afiro.mtx",
    "lp_afiro_structure.mtx",
    "karate.mtx",
    "matrix_bool.mtx",
    "matrix_int8.mtx",
    "matrix_int16.mtx",
    "matrix_int32.mtx",
    "matrix_int64.mtx",
    "matrix_uint8.mtx",
    "matrix_uint16.mtx",
    "matrix_uint32.mtx",
    "matrix_uint64.mtx",
    "matrix_fp32.mtx",
    "matrix_fp32_structure.mtx",
    "matrix_fp64.mtx",
    "west0067_jumbled.mtx",
    "skew_fp32.mtx",
    "skew_fp64.mtx",
    "skew_int8.mtx",
    "skew_int16.mtx",
    "skew_int32.mtx",
    "skew_int64.mtx",
    "structure.mtx",
    "full.mtx",
    "full_symmetric.mtx",
    "empty.mtx",
    "",
} ;

//****************************************************************************
void test_SSaveSet (void)
{
    LAGraph_Init (msg) ;
    GrB_Descriptor desc = NULL ;
    #if LG_SUITESPARSE
    OK (GrB_Descriptor_new (&desc)) ;
    OK (GxB_set (desc, GxB_COMPRESSION, GxB_COMPRESSION_LZ4HC + 9)) ;
    #endif

    // load all matrices into a single set
    GrB_Matrix *Set = LAGraph_Malloc (NFILES, sizeof (GrB_Matrix)) ;
    TEST_CHECK (Set != NULL) ;

    for (int k = 0 ; k < NFILES ; k++)
    {
        // load the matrix as Set [k]
        const char *aname = files [k] ;
        if (strlen (aname) == 0) break;
        TEST_CASE (aname) ;
        snprintf (filename, LEN, LG_DATA_DIR "%s", aname) ;
        FILE *f = fopen (filename, "r") ;
        TEST_CHECK (f != NULL) ;
        OK (LAGraph_MMRead (&(Set [k]), f, msg)) ;
        fclose (f) ;
    }

    // workaround for bug in v6.0.0 to v6.0.2:
    // ensure the matrix is not iso
    #if LG_SUITESPARSE
    #if GxB_IMPLEMENTATION < GxB_VERSION (6,0,3)
    printf ("\nworkaround for bug in SS:GrB v6.0.2 (fixed in v6.0.3)\n") ;
    for (int k = 0 ; k < NFILES ; k++)
    {
        OK (GrB_Matrix_setElement (Set [k], 0, 0, 0)) ;
        OK (GrB_wait (Set [k], GrB_MATERIALIZE)) ;
    }
    #endif
    #endif

    // save the set of matrices in a single file
    OK (LAGraph_SSaveSet ("matrices.lagraph", Set, NFILES, "many test matrices",
        msg)) ;

    // load the matrices back in
    GrB_Matrix *Set2 = NULL ;
    GrB_Index nmatrices = 0 ;
    char *collection = NULL ;
    int r = 
        LAGraph_SLoadSet ("matrices.lagraph", &Set2, &nmatrices, &collection,
        msg) ;
    printf ("nmatrices %g r %d msg %s\n", (double) nmatrices, r, msg) ;
    TEST_CHECK (nmatrices == NFILES) ;
    TEST_CHECK (Set2 != NULL) ;
    TEST_CHECK (strcmp (collection, "many test matrices") == 0) ;

    // check the matrices
    for (int k = 0 ; k < NFILES ; k++)
    {
        // ensure the matrices Set [k] and Set2 [k] are the same
        bool ok ;
        OK (LAGraph_Matrix_IsEqual (&ok, Set [k], Set2 [k], msg)) ;
        TEST_CHECK (ok) ;
    }

    // free all matrices
    LAGraph_SFreeSet (&Set, NFILES) ;
    LAGraph_SFreeSet (&Set2, NFILES) ;
    LAGraph_Free ((void **) &collection) ;

    OK (GrB_free (&desc)) ;
    LAGraph_Finalize (msg) ;
}

//****************************************************************************

TEST_LIST = {
    {"SSaveSet", test_SSaveSet},
    {NULL, NULL}
};
