// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "convolution1.h"
#include "log/klog.h"
#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/aie_adf.hpp>
#include <aie_api/utils.hpp>
#include <stdio.h>

#define input_width 3
#define input_height 224
#define filter_size_1 3
#define in_channels_1 3
#define out_channels_1 7

int32_t ifm_matrix[WGT_MAX_FRAME_NUM * IFM_SIZE];
int32_t wgt_matrix[WGT_MAX_FRAME_NUM * IFM_SIZE];
int32_t compute_data[WGT_MAX_FRAME_NUM * WGT_MAX_FRAME_NUM];

int32_t prev_out[8 * 8];

int32_t idatatmp[IFM_SIZE];
int wcur_count = 0;
#define VECTOR_LENGTH 16
///*
void vector_cpy(int32_t *dst, int32_t *src, int32_t src_len) {
    int ilen = 0;
    for (; ilen + VECTOR_LENGTH < src_len; ilen += VECTOR_LENGTH)
        chess_prepare_for_pipelining {
            aie::vector<int32_t, VECTOR_LENGTH> temp = aie::load_unaligned_v<VECTOR_LENGTH>(src + ilen);
            aie::store_unaligned_v<VECTOR_LENGTH>(dst + ilen, temp);
        }

    for (int i = ilen; i < src_len; i++) {
        dst[i] = src[i];
    }
    return;
}
int32_t vector_dot_product(int32_t *a, int32_t *b, int src_len) {
    // return 0;
    int32_t result = 0, ilen = 0;
    for (; ilen + VECTOR_LENGTH < src_len; ilen += VECTOR_LENGTH)
        chess_prepare_for_pipelining {
            aie::vector<int32_t, VECTOR_LENGTH> va = aie::load_v<VECTOR_LENGTH>(a + ilen);
            aie::vector<int32_t, VECTOR_LENGTH> vb = aie::load_v<VECTOR_LENGTH>(b + ilen);
            aie::accum<acc64, VECTOR_LENGTH> vm = aie::mul(va, vb);
            aie::vector<int32_t, VECTOR_LENGTH> result_vector = vm.to_vector<int32_t>(0);
            result += aie::reduce_add(result_vector);
        }

    for (int i = ilen; i < src_len; i++) {
        result += a[i] * b[i];
    }
    return result;
}
void conv2d(adf::input_async_buffer<int32> &__restrict iData, adf::input_async_buffer<int32> &__restrict wData,
            adf::output_async_buffer<int32> &__restrict outdata, int weight_round, int *wgt_count, int row, int col) {
    int32_t otmp[WGT_MAX_FRAME_NUM];

    // wlog("row ", row);
    // wlog("data1 ", *ifm);
    // wlog("data2 ", *wgt);
    printf("cur = %d max = %d row %d col %d\n", wcur_count, weight_round, row, col);
    if (*wgt_count < weight_round) {
        // aie::vector<int32, IFM_SIZE> v = aie::load_v<IFM_SIZE>(ifm);
        iData.acquire();
        int32_t *ifm = iData.data(), *ifm_array_data = &ifm_matrix[*wgt_count * IFM_SIZE];
        printf("cur = %d max = %d row %d col %d ifm = 0x%x\n", wcur_count, weight_round, row, col, ifm);
        if (1) { // use vector load
            vector_cpy(ifm_array_data, ifm, IFM_SIZE);
        } else {
            memcpy(&ifm_matrix[*wgt_count * IFM_SIZE], ifm, IFM_SIZE * sizeof(int32_t));
        }
        //---------
        for (int i = 0; i < 10; i++) {
            printf("ifm[%d]=%d ", i, ifm[i]);
        }
        printf("\n");
        //--------------
        iData.release();

        wData.acquire();
        int32_t *wgt = wData.data();
        printf("cur = %d max = %d row %d col %d wgt = 0x%x\n", wcur_count, weight_round, row, col, wgt);
        // the last ifm of first 8 or weight_round, it do not need storage
        if (1) {
            vector_cpy(&wgt_matrix[*wgt_count * IFM_SIZE], wgt, IFM_SIZE);
        } else {
            memcpy(&wgt_matrix[*wgt_count * IFM_SIZE], wgt, IFM_SIZE * sizeof(int32_t));
        }
        //------------
        for (int i = 0; i < 10; i++) {
            printf("wgt[%d]=%d ", i, wgt[i]);
        }
        printf("\n");
        //----------
        wData.release();

        // int32_t* ifm = &ifm_matrix[(*wgt_count-1) * IFM_SIZE];
        for (int i = 0; i <= *wgt_count; i++) {
            // can not use ifm and wgt anymore as after release the dma will fill it
            int32_t *last_ifm = &ifm_matrix[*wgt_count * IFM_SIZE];
            int32_t *last_wgt = &wgt_matrix[*wgt_count * IFM_SIZE];
            int32_t *wdata = &wgt_matrix[i * IFM_SIZE];
            int cur_ifm_idx = *wgt_count;
            int var = 0;
            // last IFM compute with each previous wgt
            if (1) {
                var = vector_dot_product(wdata, last_ifm, IFM_SIZE);
            } else {
                for (int k = 0; k < IFM_SIZE; k++) {
                    var += wdata[k] * last_ifm[k];
                }
            }
            // the current ifm , multiply with each exist wgt
            prev_out[cur_ifm_idx * weight_round + i] = var;
            printf(" -- pre -- prev_out[%d]=0x%x \n", i, var); // + cur_ifm_idx * 1000/*add offset of idx*/);

            // the current wgt multiply with each previous ifm
            if (i < *wgt_count) {
                int32_t *idata = &ifm_matrix[i * IFM_SIZE];
                int cur_wgt_idx = *wgt_count;
                int var = 0;
                if (1) {
                    var = vector_dot_product(idata, last_wgt, IFM_SIZE);
                } else {
                    for (int k = 0; k < IFM_SIZE; k++) {
                        printf("var = %d , idata[%d] = %d wgt[%d]=%d\n", var, k, idata[k], k, wgt[k]);
                        var += idata[k] * last_wgt[k];
                    }
                }
                prev_out[i * weight_round + cur_wgt_idx] = var;
                printf(" prev_out[%d]=0x%x \n", i, var); //+ cur_wgt_idx * 1000/*add offset of idx*/);
            }
        }
        if (*wgt_count == weight_round - 1) {
            for (int i = 0; i < weight_round; i++) {
                outdata.acquire();
                int32_t *out = outdata.data();
                if (1) { // weight_round should be 8
                    aie::vector<int32_t, 8> temp = aie::load_unaligned_v<8>(&prev_out[i * weight_round]);
                    aie::store_unaligned_v<8>(out, temp);
                } else {
                    for (int j = 0; j < weight_round; j++) {
                        out[j] = prev_out[i * weight_round + j]; //+ (i+1) * 10000 + (j + 1)*1000;/*add offset of idx*/
                        // if (j == 0) out[j] =1;//for test debug purpose//remove me
                        printf("round %d out[%d]=0x%x \n", i, j, out[j]);
                    }
                }
                outdata.release();
            }
        }

    } else {
        iData.acquire();
        int32_t *ifm = iData.data();
        if (1) {
            vector_cpy(idatatmp, ifm, IFM_SIZE);
        } else {
            memcpy(idatatmp, ifm, IFM_SIZE * sizeof(int32_t));
        }
        iData.release();

        for (int i = 0; i < weight_round; i++) {
            int32_t *wdata = &wgt_matrix[i * IFM_SIZE];
            int var = 0;
            if (1) {
                var = vector_dot_product(wdata, idatatmp, IFM_SIZE);
            } else {
                for (int k = 0; k < IFM_SIZE; k++) {
                    var += wdata[k] * idatatmp[k];
                }
            }
            otmp[i] = var;
            // printf("otmp[%d]=0x%x \n", i, otmp[i]);
        }

        outdata.acquire();
        int32_t *out = outdata.data();
        if (1) {
            aie::vector<int32_t, 8> temp = aie::load_unaligned_v<8>(otmp);
            aie::store_unaligned_v<8>(out, temp);
        } else {
            memcpy(out, otmp, weight_round * sizeof(int32_t));
        }
        outdata.release();
    }
    (*wgt_count)++;
    return;
}
//*/

void conv1(adf::input_async_buffer<int32> &__restrict data1,
           // #ifdef WEIGHT
           adf::input_async_buffer<int32> &__restrict data2,
           // #endif
           int weight_round, adf::output_async_buffer<int32> &__restrict out) {
    int row = (get_coreid() & 0xF);
    int col = (get_coreid() & 0x7F0000) >> 16;
    for (int k = 0; k < 112 * 7; k++) {
        conv2d(data1, data2, out, weight_round, &wcur_count, row, col);
        /*
        auto inIter1=aie::begin(data1);
        auto inIter2=aie::begin(data2);
        auto outIter=aie::begin(out);
        auto inIter_temp = inIter1;
        int32 c = 0;
        int32 a = 0;
        int32 b = 0;

        printf("cur = %d max = %d row %d col %d\n", wcur_count,weight_round, row, col);
        //wlog("row ", row);
        //wlog("col ", col);

        //wlog(" c1 wcurcount \n",wcur_count );
        //wlog(" c1 weround \n",weight_round );

        data1.acquire();
        //wlog(" after data1.acquire \n",wcur_count );
        for(int i=0;i< 7*7*3;i++)
        {
            a=*inIter1++;
            //printf("(%d) - %d \n",i, a);

            if (wcur_count < weight_round) {
                //printf("wts (%d) - %d \n",i, *inIter2++);
            }

        }
        //wlog(" after set data before data1 release \n",wcur_count );
        data1.release();
        if (wcur_count < weight_round) {
            data2.acquire();
        }
        //wlog(" after data2.acquire \n",wcur_count );
        //printf("after aquire\n");
        if (wcur_count < weight_round) {
        data2.release();
        }
        wcur_count++;
        //wlog("one round done ",0);
        //printf("one round done \n");

        out.acquire();
        //wlog(" after out.acquire \n",wcur_count );
        for (int i = 0; i < 8; i++) {
            *outIter++=i;
        }
        out.release();

        if (wcur_count < 10 || wcur_count > 200) {
        // wlog("2wcu ", wcur_count);
        }
        */
    }
}