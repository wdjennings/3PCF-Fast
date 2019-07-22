#ifndef DIR_EXT_HPP
#define DIR_EXT_HPP

#include <sys/types.h>
#include <sys/stat.h> 
#include <cstdio>
#include <string>
using namespace std;
#include <fstream>

bool dir_exists(string pathname);
std::ifstream::pos_type filesize(const char* filename);

#endif