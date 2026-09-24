#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
#include "graph.h"
myGraph gradf;
#else
#include "xil_printf.h"
#include "xil_io.h"
#include "xil_cache.h"
#endif

#include <fstream>
#include <xaiengine.h>
#include "aiebaremetal.h"


AbrGraph gr("gradf");

int main(int argc, char ** argv)
{

#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
 gradf.init();
#endif

	gr.getaiedevinst();
	gr.init();
	gr.run(1);
	gr.wait();
	gr.end();
  return 0;
}