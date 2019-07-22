/* Interface for loading bin files */

#ifndef __BINS_HPP__
#define __BINS_HPP__

#include <cstdlib>

#include <vector>
using std::vector;

#include <iostream>
using std::cout;

#include "../../cpp_tools/point.hpp"

// Set of triangles
// = configurations for ONE primary point
//    with all secondary points which match the requested bins
struct triangle_set{
	point ptB;
	vector<point> ptsC;
	triangle_set(point _ptB) : ptB(_ptB) {}
    size_t size(){ return ptsC.size(); }
};

// One bin has a list of triangle_sets
// One set for each primary point
struct triangle_configs{
	float rmin, rmax;
	float r1avg, r2avg, r3avg;
	vector <triangle_set> sets;

    size_t size(){ 
    	size_t total_size = 0;
    	for (size_t index=0; index<sets.size(); index++){
    		total_size += sets.at(index).size();
    	}
    	return total_size;
    }

    triangle_set& at(size_t index){ 
    	return sets.at(index); 
    }

    void add_set(triangle_set _set){
    	sets.push_back(_set);
    }
};


// Method to load all triangle vertices from store .verts file
vector<triangle_configs>* load_triangle_configs(const char *vertsfilename, float cell_size);

// Print number of configuations and likely run time
double summary_and_time_per_file(vector<triangle_configs>* selectionFunction, int Nres3, bool verbose);


#endif
