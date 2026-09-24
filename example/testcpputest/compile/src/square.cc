#include <stdio.h>
#include <adf.h>
#define DATA_LENGTH 128
void square(input_window_int32 * win,
              output_window_int32 *out) {


  for (unsigned i = 0 ; i < DATA_LENGTH ; i++) {
    int j = window_readincr(win);
    window_writeincr(out,j*j + 0  );
  }

}