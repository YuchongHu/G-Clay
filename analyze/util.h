#pragma once
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
using namespace std;

/**
 * s of form: "04:32:31.819"
*/
double time2sec(const string& s) {
    return stoi(s.substr(0,2))*3600+stoi(s.substr(3,2))*60+stoi(s.substr(6,2))+stoi(s.substr(9,3))/1000.0;
}

/**
 * s1 s2 of form: "04:32:31.819"
 * should: s1 > s2
 * @return s1 - s2
*/
double timediff(const string& s1, const string& s2) {
    return time2sec(s1) - time2sec(s2);
}

/**
 * return a string line of statistics of vector v
 * they are: SUM, AVG, MAX, MIN
*/
string statistics_of_vector(const vector<double>& v) {
    char buf[100];
    double sum=0, maxd=INT32_MIN, mind=INT32_MAX;
    for (const double& d:v) {
        sum += d;
        maxd = max(maxd, d);
        mind = min(mind, d);
    }
    sprintf(buf, "SUM: %.3f, AVG: %.3f, MAX: %.3f, MIN: %.3f", sum, sum/v.size(), maxd, mind);
    return string(buf);
}