// Copyright (C) 2026 Advanced Micro Devices, Inc.

#if defined(__AEG_METADATA_ONLY__)
#include "graph.h"
myGraph EG;
int main(int argc, char **argv) {
    EG.init();
    return 0;
}
#else
#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
#include "graph.h"
myGraph EG;
#else
#include "xil_cache.h"
#include "xil_io.h"
#include "xil_printf.h"
#if __AIE_ARCH__ == 22
#include "xiltimer.h"
#else
#include "xtime_l.h"
#endif
#endif

#include "aiebaremetal.h"
#include "config.h"
#include <fstream>
#include <xaiengine.h>

AEGGraph<aeg::EG> gr;

#define input_width 3
#define input_height 3
#define filter_size_1 3
#define in_channels_1 3
#define out_channels_1 2
#define filter_size_2 3
#define in_channels_2 2
#define out_channels_2 1

#define NITERATIONS 1
#define InSizePerIteration input_width *input_height *in_channels_1
#define OutSizePerIteration input_width *input_height *out_channels_2

using namespace std;

const int InSize = NITERATIONS * InSizePerIteration;
const int OutSize = NITERATIONS * OutSizePerIteration;

int main(int argc, char **argv) {

#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
    EG.init();
#else
#ifdef _RPU_
    // RPU need to disable data cache, without cache diable app will stuck
    Xil_DCacheDisable();
#endif
#endif

    int32_t *input_data = (int32_t *)gr.malloc(InSize * sizeof(int32_t));
    int32_t *weights1_data =
        (int32_t *)gr.malloc(in_channels_1 * out_channels_1 * filter_size_1 * filter_size_1 * sizeof(int32_t));
    int32_t *weights2_data =
        (int32_t *)gr.malloc(in_channels_2 * out_channels_2 * filter_size_1 * filter_size_2 * sizeof(int32_t));

    int32_t *output_data = (int32_t *)gr.malloc(OutSize * sizeof(int32_t));

    for (int i = 0; i < in_channels_1; i++) {
        for (int j = 0; j < input_width * input_height; j++) {
            input_data[i * input_width * input_height + j] = i + 1;
        }
    }

    // First convolution weight
    for (int k = 0; k < out_channels_1; k++) {
        for (int i = 0; i < in_channels_1; i++) {
            for (int j = 0; j < filter_size_1 * filter_size_1; j++) {
                weights1_data[k * filter_size_1 * filter_size_1 * in_channels_1 + i * filter_size_1 * filter_size_1 +
                              j] = 1;
            }
        }
    }

    // second convolution weight
    for (int k = 0; k < out_channels_2; k++) {
        for (int i = 0; i < in_channels_2; i++) {
            for (int j = 0; j < filter_size_2 * filter_size_2; j++) {
                weights2_data[k * filter_size_2 * filter_size_2 * in_channels_2 + i * filter_size_2 * filter_size_2 +
                              j] = 1;
            }
        }
    }

    cout << " Input data" << endl;

    for (int k = 0; k < in_channels_1; k++) {
        for (int i = 0; i < input_height; i++) {
            for (int j = 0; j < input_width; j++)

            {
                cout << input_data[k * input_width * input_height + i * input_width + j] << " ";
            }
            cout << endl;
        }
        cout << endl;
    }
    cout << "weight_1 data" << endl;

    for (int l = 0; l < out_channels_1; l++) {
        for (int i = 0; i < in_channels_1; i++) {
            for (int j = 0; j < filter_size_1; j++) {
                for (int k = 0; k < filter_size_1; k++)

                {
                    cout << weights1_data[l * filter_size_1 * filter_size_1 * in_channels_1 +
                                          i * filter_size_1 * filter_size_1 + j * filter_size_1 + k]
                         << " ";
                }
                cout << endl;
            }
            cout << endl << endl;
        }
        cout << "**************************" << endl;
    }
    cout << "weight_2 data" << endl;

    for (int l = 0; l < out_channels_2; l++) {
        for (int i = 0; i < in_channels_2; i++) {
            for (int j = 0; j < filter_size_2; j++) {
                for (int k = 0; k < filter_size_2; k++)

                {
                    cout << weights2_data[l * filter_size_2 * filter_size_2 * in_channels_2 +
                                          i * filter_size_2 * filter_size_2 + j * filter_size_2 + k]
                         << " ";
                }
                cout << endl;
            }
            cout << endl << endl;
        }
        cout << "**************************" << endl;
    }

    for (uint32_t i = 0; i < OutSize; i++)
        output_data[i] = 99999;

    gr.init();

    for (int kiter = 0; kiter < NITERATIONS; kiter++) {
        cout << "Iteration: " << kiter << endl;

        gr.ddrin.setAddress(input_data + kiter * InSizePerIteration);
        gr.weights_1.setAddress(weights1_data + kiter * out_channels_1 * in_channels_1 * filter_size_1 * filter_size_1);
        gr.weights_2.setAddress(weights2_data + kiter * out_channels_2 * in_channels_2 * filter_size_2 * filter_size_2);
        gr.ddrout.setAddress(output_data + kiter * OutSizePerIteration);
        cout << "\tAddresses set. " << endl;

        gr.ddrin.out[0].gm2aie_nb();
        gr.weights_1.out[0].gm2aie_nb();
        gr.weights_2.out[0].gm2aie_nb();
        gr.ddrout.in[0].aie2gm_nb();
        cout << "\tTransactions sent. " << endl;

        gr.run(1);

        gr.ddrin.out[0].wait();
        gr.weights_1.out[0].wait();
        gr.weights_2.out[0].wait();
        gr.ddrout.in[0].wait();
        gr.wait();
    }

    cout << "OFM Interface DMA transfer done, ready for file I/O!!" << endl;
    cout << " output data" << endl;
    for (int k = 0; k < out_channels_2; k++) {
        for (int i = 0; i < input_height; i++) {
            for (int j = 0; j < input_width; j++) {
                cout << output_data[k * input_width * input_height + i * input_width + j] << " ";
            }
            cout << endl;
        }
    }

    cout << "\n=== Verifying Results ===" << endl;
    bool test_passed = true;
    int error_count = 0;

    for (int i = 0; i < OutSize; i++) {
        if (output_data[i] == 99999) {
            cout << "ERROR: Output data[" << i << "] was not updated (still 99999)" << endl;
            test_passed = false;
            error_count++;
            if (error_count >= 10) {
                cout << "... (stopping after 10 errors)" << endl;
                break;
            }
        }
    }

    int32_t min_val = output_data[0];
    int32_t max_val = output_data[0];
    int32_t sum_val = 0;

    for (int i = 0; i < OutSize; i++) {
        if (output_data[i] < min_val)
            min_val = output_data[i];
        if (output_data[i] > max_val)
            max_val = output_data[i];
        sum_val += output_data[i];
    }

    cout << "Output statistics:" << endl;
    cout << "  Min value: " << min_val << endl;
    cout << "  Max value: " << max_val << endl;
    cout << "  Average value: " << (float)sum_val / OutSize << endl;

    if (min_val < 0 || max_val > 10000) {
        cout << "WARNING: Output values are outside expected range [0, 10000]" << endl;
        test_passed = false;
    }

    bool all_same = true;
    for (int i = 1; i < OutSize; i++) {
        if (output_data[i] != output_data[0]) {
            all_same = false;
            break;
        }
    }

    if (all_same && OutSize > 1) {
        cout << "INFO: All output values are the same (" << output_data[0] << ")" << endl;
    }

    if (test_passed) {
        cout << "\n*** Test PASSED ***" << endl;
    } else {
        cout << "\n*** Test FAILED ***" << endl;
        gr.end();
        gr.free(input_data);
        gr.free(output_data);
        gr.free(weights1_data);
        gr.free(weights2_data);
        return -1;
    }

    gr.end();
    cout << "Test Passed!\n " << endl;
    gr.free(input_data);
    gr.free(output_data);
    gr.free(weights1_data);
    gr.free(weights2_data);

    return 0;
}
#endif /* !__AEG_METADATA_ONLY__ */
