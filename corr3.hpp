/*************************************************************
  Interface for 3-point correlation
*************************************************************/

#ifndef __CORR3_HPP__
#define __CORR3_HPP__

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include <cmath>
using std::ceil;
using std::isnan;

#ifdef _OMPTHREAD_
#include <omp.h>
#endif

#include <cstdlib>

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <vector>
using std::vector;

#include <iostream>
using std::ios_base;
using std::ostream;
using std::cout;

#include <string>
using std::string;

#include <assert.h>
#include <cstdlib>

#include <fstream>
using std::ifstream;
using std::ofstream;

#include <algorithm>    // std::min_element, std::max_element

#include "cpp_tools/point.hpp"     
#include "cpp_tools/timer.hpp"
#include "bins.hpp"        
#include "cpp_tools/filecommands.hpp"
#include "cpp_tools/data_vectors.hpp"

#include "cpp_tools/argparse.hpp"
#include "cpp_tools/string_ext.hpp"
#include "cpp_tools/dir_ext.hpp"
#include "cpp_tools/loop_data.hpp"

// corr3 statistics struct (DDD, DDR, DRR, RRR)
// And also with jackknifing vector
struct statistics;
struct statistics_with_jk;

// Operators for corr3 statistics
// ostream& operator<<(ofstream& os, statistics& s);
statistics& operator+=(statistics& sa, statistics& sb);
statistics operator+(statistics& sa, statistics& sb);
statistics operator-(statistics& sa, statistics& sb);

// Estimator Functions
typedef double (*estimatorFunctionType)(statistics&); 
double estimatorLS(statistics& s);
double estimatorPlain(statistics& s);

// Main correlation method
vector<statistics_with_jk>* 
run_correlation(const float* box1, const float* box2, const float* box3, 
				vector< triangle_configs > *selectionFunction, 
				int Nres);

// Save results to file
void save(vector<statistics_with_jk> *results, estimatorFunctionType estimator, vector< triangle_configs > *selectionFunction, const char *binfilename);

#endif