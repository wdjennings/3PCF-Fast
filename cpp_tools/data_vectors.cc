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


/*--------------------------------------------------
	 SCALAR OPERATORs on vector<T>&
----------------------------------------------------*/

template <typename T>
vector<T>& operator-=(vector<T>& v, double s){
    int imax = v.size();
#ifdef _OMPTHREAD_
#pragma omp parallel for
#endif
    for (int i=0; i<imax; i++) { v.at(i) = v.at(i) - s;  }
    return v;
}

template <typename T>
vector<T>& operator+=(vector<T>& v, double s){
    int imax = v.size();
#ifdef _OMPTHREAD_
#pragma omp parallel for
#endif
    for (int i=0; i<imax; i++) { v.at(i) = v.at(i) + s;  }
    return v;
}

template <typename T>
vector<T>& operator/=(vector<T>& v, double s){
    int imax = v.size();
#ifdef _OMPTHREAD_
#pragma omp parallel for
#endif
    for (int i=0; i<imax; i++) { v.at(i) = v.at(i) / s;  }
    return v;
}

template <typename T>
vector<T>& operator*=(vector<T>& v, double s){
    int imax = v.size();
#ifdef _OMPTHREAD_
#pragma omp parallel for
#endif
    for (int i=0; i<imax; i++) { v.at(i) = v.at(i) * s;  }
    return v;
}


/*-----------------------------------------------------
	 Get the mean value, and set the range values
-------------------------------------------------------*/

template <typename T>
double meanAndRange(vector<T>* v, double& lowest, double& highest){

    double datasum = 0.0;
    #ifdef _OMPTHREAD_
    #pragma omp parallel
    #endif
    {
        double privateDataSum = 0.0;
        #ifdef _OMPTHREAD_
        #pragma omp for
        #endif
        for (size_t i=0; i<v->size(); i++) { 
            double dv = (double)(v->at(i)); 
            privateDataSum += dv; 
            lowest = fmin(lowest,dv); 
            highest = fmax(highest,dv); 
        }
        #ifdef _OMPTHREAD_
        #pragma omp critical
        #endif
            { datasum += privateDataSum; }
    }
    return datasum / ((double)(v->size()));
}



/*-----------------------------------------------------
	 Get the mean value
-------------------------------------------------------*/

template <typename T>
double mean(vector<T>* v){

    double datasum = 0.0;
    #ifdef _OMPTHREAD_
    #pragma omp parallel
    #endif
     {
        double privateDataSum = 0.0;
        #ifdef _OMPTHREAD_
        #pragma omp for
        #endif
        for (size_t i=0; i<v->size(); i++) { 
            double dv = (double)(v->at(i)); 
            privateDataSum += dv; 
        }
        #ifdef _OMPTHREAD_
        #pragma omp critical
        #endif
        { datasum += privateDataSum; }
    }
    return datasum / ((double)(v->size()));
}



/*-----------------------------------------------------
	 Discretise data into binary format = {0,1}
-------------------------------------------------------*/

template <typename T>
void fillBinaryBox(vector<T>* v, vector<bool>* binary, const std::pair<T,T> range){
    int imax = v->size();
    T low = range.first, high = range.second;
    #ifdef _OMPTHREAD_
    #pragma omp parallel for
    #endif
    for (int i=0; i<imax; i++) { T vv = v->at(i); binary->at(i) = ((vv>=low) && (vv<high));  }
}



/*---------------------------------------------------------------
    Data Binning functions
        = a rough estimate of a histogram of the data values
        (i.e. 10% lie below 0K, 10% between 0 and 0.005mK etc)
-----------------------------------------------------------------*/


#include <algorithm>
using std::sort;

static bool sort_using_less_than(double u, double v){ return u < v; }

int mini(int a, int b){ return (a<b ? a : b); }

void outputDiscreteBins(vector<double>* data, size_t numberOfBins){
    
    // Make a random subset of the data that is smalle
    // Up to 10^6 values long
    int dataLength = (int)data->size();
    int maxPixels = ((int)1E+7);
    
    vector<double> smallerData;
    for (int i=0; i<mini(dataLength, maxPixels); i++){ smallerData.push_back(data->at(rand()%dataLength)); }
    
    // Sort the data in the COPIED array
    printf("SORTING...");
    std::sort(smallerData.begin(),smallerData.end(),sort_using_less_than);
    printf("done.\n");

    // split into N segments
    double smallerDataSize = (double)smallerData.size();
    double segmentSize = ((double)smallerDataSize)/numberOfBins;

    for (size_t rangeBin = 0; rangeBin < numberOfBins; rangeBin++){
        int index_low = (int)(rangeBin * segmentSize);
        int index_high = (int)((rangeBin+1) * segmentSize);
        index_high = mini(index_high, ((int)smallerDataSize-1) );
        
        double data_low = smallerData.at(index_low);
        double data_high = smallerData.at(index_high);
        double actualPercent = (100.0*(1 + index_high - index_low)) / smallerDataSize;

        int lowLog = ((int)(1 - ceil(log(data_low*1.0E+3))));
        int highLog = ((int)(1-ceil(log(data_high*1.0E+3))));
        
        printf(" DATA RANGE BIN %ld: from %.*f to %.*f (%.01f %% of data)\n",rangeBin,lowLog,data_low,highLog,data_high,actualPercent);
    }
}


#endif
