#include "../../src/benchmark/LAGraph_demo.h"
#include "LG_internal.h"
#include "LAGraphX.h"

/*
Given an adjacency matrix of an undirected graph, produces the corresponding
incidence matrix. The adjacency matrix may be weighted or unweighted.
*/
int A_to_E(GrB_Matrix *result, const GrB_Matrix A, char *msg) {
    GrB_Matrix E = NULL ;
    GrB_Index *row_indices = NULL ;
    GrB_Index *col_indices = NULL ;
    GrB_Index *values = NULL ;
    GrB_Index *E_row_indices = NULL ;
    GrB_Index *E_col_indices = NULL ;
    GrB_Index *E_values = NULL ;

    GrB_Index nvals ;

    GRB_TRY (GrB_Matrix_nvals (&nvals, A)) ;
    row_indices = (GrB_Index*) malloc(sizeof(GrB_Index) * nvals) ;
    col_indices = (GrB_Index*) malloc(sizeof(GrB_Index) * nvals) ;
    // TODO: need to change values to match type of A (bool, uint64, fp32, etc)
    // for now, assuming A has uint64 entries
    values = (GrB_Index*) malloc(sizeof(GrB_UINT64) * nvals) ;

    GRB_TRY (GrB_Matrix_extractTuples (row_indices, col_indices, values, &nvals, A)) ;

    // number of entries in E should be 2 * n_edges
    // n_edges is nvals / 2 (don't count duplicates). So, number of entries in E is just nvals.
    E_row_indices = (GrB_Index*) malloc(sizeof(GrB_Index) * nvals) ;
    E_col_indices = (GrB_Index*) malloc(sizeof(GrB_Index) * nvals) ;
    E_values = (GrB_Index*) malloc(sizeof(GrB_UINT64) * nvals) ;

    // current index in E_* arrays
    GrB_Index pos = 0;

    for (int i = 0; i < nvals; i++) {
        // only consider edges if row < col (prevent duplicates)
        GrB_Index row = row_indices[i] ;
        GrB_Index col = col_indices[i] ;
        GrB_Index value = values[i] ;

        if (row < col) {
            // first put only row values (1st endpoint)
            E_col_indices[pos] = pos ;
            E_row_indices[pos] = row ;
            E_values[pos] = value ;
            pos++ ;
        }
    }

    for (int i = 0; i < nvals; i++){
        // in this pass, put the col values (2nd endpoint)
        GrB_Index row = row_indices[i] ;
        GrB_Index col = col_indices[i] ;
        GrB_Index value = values[i] ;
        if (row < col) {
            E_col_indices[pos] = pos - nvals ;
            E_row_indices[pos] = col ;
            E_values[pos] = value;
            pos++ ;
        }
    }

    GRB_TRY (GrB_Matrix_build (E, E_row_indices, E_col_indices, E_values, nvals, GrB_SECOND_UINT64)) ;

    (*result) = E ;
    return (GrB_SUCCESS) ;
}

int main (int argc, char** argv)
{
    char msg [LAGRAPH_MSG_LEN] ;

    bool burble = false;
    demo_init (burble) ;

    char *matrix_name = (argc > 1) ? argv[1] : "stdin" ;



    return (GrB_SUCCESS) ;
}