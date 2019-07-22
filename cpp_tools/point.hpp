/*  Point struct for (x,y,z) values
    And point arithemtic (equality, add, mag, dist)
*/

#ifndef __POINT_HPP__
#define __POINT_HPP__

#include <iostream>
using std::ostream;

#include <stdio.h>

#include <cmath>
using std::pow;
using std::sqrt;

// Main struct
struct point{ 
    int x, y, z;
    point() : x(0), y(0), z(0){}
    point(int a, int b, int c) : x(a), y(b), z(c) {}
};

// Equality of two points
bool operator==(point &A, point &B);

// Convert point to pretty string, with optional delim
std::string point_string(point& a, std::string delim);

// Output points to stream
ostream& operator<<(ostream& os, point& p);

// Magnitude of point value
float mag(point &a);

// Square distance between two point
double distSq(point &a, point &b);

// Convert x,y,z into index of 3d array
signed long int pixel_index(int x, int y, int z, int Nres);

#endif