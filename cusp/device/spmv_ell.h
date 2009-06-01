/*
 *  Copyright 2008-2009 NVIDIA Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */



#pragma once

#include <cusp/ell_matrix.h>
#include <cusp/memory.h>
#include <cusp/device/utils.h>
#include <cusp/device/texture.h>

// SpMV kernel for the ELLPACK/ITPACK matrix format.

namespace cusp
{

namespace device
{

template <typename IndexType, typename ValueType, bool UseCache>
__global__ void
spmv_ell_kernel(const IndexType num_rows, 
                const IndexType num_cols, 
                const IndexType num_cols_per_row,
                const IndexType stride,
                const IndexType * Aj,
                const ValueType * Ax, 
                const ValueType * x, 
                      ValueType * y)
{
    const IndexType row = large_grid_thread_id();

    if(row >= num_rows){ return; }

    ValueType sum = y[row];

    Aj += row;
    Ax += row;
    
    const IndexType invalid_index = cusp::ell_matrix<IndexType, ValueType, cusp::device_memory>::invalid_index;

    for(IndexType n = 0; n < num_cols_per_row; n++){
        const IndexType col = *Aj;

        if (col != invalid_index){
            const ValueType A_ij = *Ax;
            sum += A_ij * fetch_x<UseCache>(col, x);
        }

        Aj += stride;
        Ax += stride;
    }

    y[row] = sum;
}

template <typename IndexType, typename ValueType>
void spmv(const cusp::ell_matrix<IndexType,ValueType,cusp::device_memory>& ell, 
          const ValueType * x, 
                ValueType * y)
{
    const unsigned int BLOCK_SIZE = 256;
    const dim3 grid = make_large_grid(ell.num_rows, BLOCK_SIZE);
   
    spmv_ell_kernel<IndexType,ValueType,false> <<<grid, BLOCK_SIZE>>>
        (ell.num_rows, ell.num_cols, ell.num_entries_per_row, ell.stride,
         ell.column_indices, ell.values,
         x, y);
}

template <typename IndexType, typename ValueType>
void spmv_tex(const cusp::ell_matrix<IndexType,ValueType,cusp::device_memory>& ell, 
              const ValueType * x, 
                    ValueType * y)
{
    const unsigned int BLOCK_SIZE = 256;
    const dim3 grid = make_large_grid(ell.num_rows, BLOCK_SIZE);
  
    bind_x(x);

    spmv_ell_kernel<IndexType,ValueType,true> <<<grid, BLOCK_SIZE>>>
        (ell.num_rows, ell.num_cols, ell.num_entries_per_row, ell.stride,
         ell.column_indices, ell.values,
         x, y);

    unbind_x(x);
}


} // end namespace device

} // end namespace cusp

