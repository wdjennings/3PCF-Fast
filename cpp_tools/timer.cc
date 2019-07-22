/* Timer methods for feedback */

#include "timer.hpp"

// Globals for start and end time
struct timeval start_time, end_time;

// Get current date/time, format is "MM-DD at HH:mm:ss"
const string pretty_time(double delta_time) {
    time_t     now = time(0) + delta_time;
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%m-%d at %X", &tstruct);
    return buf;
}

// Start timing now
void startTimer(){ 
	gettimeofday(&start_time, NULL); 
}

// Get current time since start_time as a string
string currentTimeTaken(){
    gettimeofday(&end_time, NULL);
    double delta = ((end_time.tv_sec  - start_time.tv_sec) * 1000000u + end_time.tv_usec - start_time.tv_usec) / 1.e6;    
    char buff[100];
    sprintf(buff,"%.02fs",delta);
    string ans = buff;
    return ans;
}
