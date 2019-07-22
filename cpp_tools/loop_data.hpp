#ifndef LOOP_DATA_HPP
#define LOOP_DATA_HPP

#include <dirent.h>
#include <fstream>
#include <cmath>

#include "dir_ext.hpp"
#include "string_ext.hpp"
#include "argparse.hpp"

const static int _SUCCESS = 0;
const static int _FAILURE = -1;

vector< pair<string,string> >* get_loop_filenames(ArgumentParser parser, string output_folder, string output_prefix);

int get_filename_N_L(string filename, int &N, float &L);

float* load_float_data(string inputfilename, int N);
double* load_double_data(string filename, int N);

#endif