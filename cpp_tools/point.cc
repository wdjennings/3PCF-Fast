#include "point.hpp"

// Equality of two points
bool operator==(point &A, point &B){
    return A.x==B.x && A.y==B.y && A.z==B.z;
}

// Convert point to pretty string, with optional delim
std::string point_string(point& a, std::string delim){
    char ans[100];
    sprintf(ans,"%d%s%d%s%d", a.x, delim.c_str(), a.y, delim.c_str(), a.z);
    return ans;
}

// Output points to stream
ostream& operator<<(ostream& os, point& p){
    os << "{" << p.x << ", " << p.y << ", " << p.z << "}";
    return os;
}

// Magnitude of point value
float mag(point &a){
    return sqrt( pow(a.x,2.0) + pow(a.y,2.0) + pow(a.z,2.0) );
}

// Square distance between two points
double distSq(point &a, point &b){ 
    double dx = double(a.x)-double(b.x);
    double dy = double(a.y)-double(b.y);
    double dz = double(a.z)-double(b.z);
    return pow(dx,2.0)+pow(dy,2.0)+pow(dz,2.0); 
}

// Convert x,y,z into index of 3d array
signed long int pixel_index(int x, int y, int z, int Nres){
    return (x*Nres*Nres) + y*(Nres) + z; 
}

