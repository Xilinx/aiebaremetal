/**
* Copyright (C) 2024-2025 Advanced Micro Devices, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/

#include <adf.h>

struct test {
	void hello() {
	}
};
struct test2 {
	test tt;
	void hello2() {
	}
};

struct fir:public adf::graph {
	adf::input_gmio in[2];
};
struct multout:public adf::graph{
	adf::output_gmio out[8];
};
struct myGraph:public adf::graph {
	adf::input_port inpt[2];
	adf::input_gmio in[3];
	adf::output_gmio singleout;
	adf::output_gmio out[2];
	adf::output_gmio outarray[2][2];
	fir fir24;
	multout firstout;
};

