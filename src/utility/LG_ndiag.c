//------------------------------------------------------------------------------
// LG_ndiag: count the # of diagonal entries in a matrix
//------------------------------------------------------------------------------

// LAGraph, (c) 2021 by The LAGraph Contributors, All Rights Reserved.
// SPDX-License-Identifier: BSD-2-Clause
// Contributed by Tim Davis, Texas A&M University.
// See additional acknowledgments in the LICENSE file,
// or contact permission@sei.cmu.edu for the full terms.

//------------------------------------------------------------------------------

#define LG_FREE_ALL         \
{                           \
    GrB_free (&M) ;         \
    GrB_free (&D) ;         \
    GrB_free (&d) ;         \
}

#include "LG_internal.h"

int LG_ndiag
(
    // output:
    int64_t *ndiag,         // # of entries 
    // input:
    GrB_Matrix A,           // matrix to count
    char *msg               // error message
)
{

    //--------------------------------------------------------------------------
    // extract the diagonal and count its entries
    //--------------------------------------------------------------------------

    GrB_Matrix D = NULL, M = NULL ;
    GrB_Vector d = NULL ;
    LG_ASSERT (ndiag != NULL, GrB_NULL_POINTER) ;
    (*ndiag) = LAGRAPH_UNKNOWN ;

    GrB_Index nrows, ncols ;
    GrB_TRY (GrB_Matrix_nrows (&nrows, A)) ;
    GrB_TRY (GrB_Matrix_ncols (&ncols, A)) ;
    GrB_Index n = LAGraph_MIN (nrows, ncols) ;

    GrB_Type atype ;
    char atype_name [LAGRAPH_MAX_NAME_LEN] ;
    LG_TRY (LAGraph_Matrix_TypeName (atype_name, A, msg)) ;
    LG_TRY (LAGraph_TypeFromName (&atype, atype_name, msg)) ;

    #if LG_SUITESPARSE

        //----------------------------------------------------------------------
        // SuiteSparse:GraphBLAS v5.0.2: use GxB_Vector_diag
        //----------------------------------------------------------------------

        GrB_TRY (GrB_Vector_new (&d, atype, n)) ;
        GrB_TRY (GxB_Vector_diag (d, A, 0, NULL)) ;
        GrB_TRY (GrB_Vector_nvals ((GrB_Index *) ndiag, d)) ;

    #else

        //----------------------------------------------------------------------
        // pure GrB version with no GxB extensions
        //----------------------------------------------------------------------

        GrB_TRY (GrB_Matrix_new (&M, GrB_BOOL, nrows, ncols)) ;
        GrB_TRY (GrB_Matrix_new (&D, atype, nrows, ncols)) ;
        for (int64_t i = 0 ; i < n ; i++)
        {
            // M (i,i) = true
            GrB_TRY (GrB_Matrix_setElement (M, (bool) true, i, i)) ;
        }

        // D<M,struct> = A
        GrB_TRY (GrB_assign (D, M, NULL, A, GrB_ALL, nrows, GrB_ALL, ncols,
            GrB_DESC_S)) ;
        GrB_TRY (GrB_Matrix_nvals ((GrB_Index *) ndiag, D)) ;

    #endif

    LG_FREE_ALL ;
    return (GrB_SUCCESS) ;
}

