/*
  data_vectors.hpp:
		
    Contains methods & operators that act on Data Vector functions    
        sped up by OMP libraries, if present

    -> subtraction, addition, division, multiplication by scalar
    -> elementwise subtraction, addition, division, multiplication
    -> mean value of data
    -> discretisation: set data to {0, 1} under some condition

*/

#ifndef data_vectors_hpp
#define data_vectors_hpp

#include <vector>
using std::vector;

#include <stdio.h>

#include <iostream>

#include <cmath>
using std::ceil;
using std::log;

#ifdef _OMPTHREAD_
#include <omp.h>
#endif

#include <algorithm>
using std::sort;

template <typename T>
vector<T>& operator-=(vector<T>& v, double s);

template <typename T>
vector<T>& operator+=(vector<T>& v, double s);

template <typename T>
vector<T>& operator/=(vector<T>& v, double s);

template <typename T>
vector<T>& operator*=(vector<T>& v, double s);

template <typename T>
double meanAndRange(vector<T>* v, double& lowest, double& highest);

template <typename T>
double mean(vector<T>* v);

template <typename T>
void fillBinaryBox(vector<T>* v, vector<bool>* binary, const std::pair<T,T> range);

// static bool sort_using_less_than(double u, double v){ return u < v; }

int mini(int a, int b);

void outputDiscreteBins(vector<double>* data, size_t numberOfBins);

#endif
