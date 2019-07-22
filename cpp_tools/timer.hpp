/* Timer methods for feedback */

#ifndef __TIMER_HPP__
#define __TIMER_HPP__

#include <string>
using std::string;

#include <stdio.h>
#include <sys/time.h>

#include <ctime>
#include <iostream>

const string pretty_time(double delta_time=0);
void startTimer();
string currentTimeTaken();

#endif