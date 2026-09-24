#include "square.h"
#include <adf.h>


using namespace adf;
#define WIN_SIZE 32
#define DATA_LENGTH 128
struct myGraph : public adf::graph {
public :
adf::kernel sq;

  adf::input_gmio in;
  adf::output_gmio out;




  myGraph() {
    in = input_gmio::create("gmio1", 64, 1);
    out = output_gmio::create("gmio2", 64, 1);
    sq = adf::kernel::create( square ) ;
    adf::source(sq) = "src/square.cc";
    runtime<ratio>(sq) = 0.6;
		adf::location<adf::kernel>(sq) = adf::tile(4, 1);

    adf::connect<window<WIN_SIZE * sizeof(int32)>>(in.out[0], async(sq.in[0]));
    adf::connect<window<WIN_SIZE * sizeof(int32)>>(async(sq.out[0]), out.in[0]);

  }
};
