#include <iostream>
#include <vector>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <regex>
#include "util.h"
using namespace std;
/*
parse jerasure.log clay.log myclay.log，derive time of each stage
*/

#define REPAIR_NO 10
struct Info {
    double d1, d2, d3;
};
Info infos[99];
string want_to_read[99]; // "6" | "2,6"

void parse(const string& text, int mode);
void usage() {
    cout<<"--------------- analyze ---------------"<<endl;
    cout<<"analyze xx.log [xx.log]..."<<endl;
    cout<<"analyze -h: print help info"<<endl;
}

int main(int argc, char* argv[]) {
    if (argc > 1 && strcmp(argv[1], "-h") == 0) {
        usage();
        return 0;
    }
    for (int i=1; i<argc; i++) {
        string filename(argv[i]);
        ifstream f(filename);
        if (!f) {
            cerr << "file open error" << endl;
            return -1;
        }
        ostringstream ss;
        ss << f.rdbuf();
        const string text = ss.str();
        parse(text, i);
    }

    // output
    cout<<"--------------- stage 1 ---------------"<<endl;
    for (int i=1; i<argc; i++) {
        printf("%.3f - Disk Read of %15s (REPAIR=%s)\n", infos[i].d1, argv[i], want_to_read[i].c_str());
    }
    cout<<"--------------- stage 2 ---------------"<<endl;
    for (int i=1; i<argc; i++) {
        printf("%.3f - Network of   %15s (REPAIR=%s)\n", infos[i].d2, argv[i], want_to_read[i].c_str());
    }
    cout<<"--------------- stage 3 ---------------"<<endl;
    for (int i=1; i<argc; i++) {
        printf("%.3f - Decode of    %15s (REPAIR=%s)\n", infos[i].d3, argv[i], want_to_read[i].c_str());
    }
    cout<<"----------------- Sum -----------------"<<endl;
    for (int i=1; i<argc; i++) {
        double sum = infos[i].d1 + infos[i].d2 + infos[i].d3;
        printf("%.3f - Sum of       %15s (REPAIR=%s)\n", sum, argv[i], want_to_read[i].c_str());
    }
    
    if (argc == 2) {  
        cout<<"-----------------"<<endl;
        printf("%.3f,%.3f,%.3f\n", infos[1].d1, infos[1].d2, infos[1].d3);
    }

    return 0;
}

void parse(const string& text, int idx) {
    smatch m;
    string pattern;
    string ts1, ts2; // time string
    string timepattern = "T([0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]{3})";
    auto begin = text.cbegin(); // must be const
    auto end = text.cend();

    pattern = string("want_to_read: ([0-9]{1,2}|[0-9]{1,2},[0-9]{1,2}), available");
    regex_search(begin, end, m, regex(pattern));
    want_to_read[idx] = m[1];

    // stage 1
    pattern = timepattern + string(".*store->read.*\n.*") + timepattern + string(".*read done(\n)"); // m[1]: time1, m[2]: time2, m[3]: \n
    vector<string> readtime, donetime;
    for (int i=0; i<REPAIR_NO; i++) {
        regex_search(begin, end, m, regex(pattern));
        readtime.push_back(m[1]);
        donetime.push_back(m[2]);
        begin += m.position(3); 
    }
    infos[idx].d1 = timediff(donetime.back(), readtime.front());
    
    // stage 2
    pattern = timepattern + string(".*: decode invoked");
    regex_search(begin, end, m, regex(pattern));
    infos[idx].d2 = timediff(m[1], donetime.back());

    // stage 3
    ts1 = m[1];
    pattern = timepattern + string(".*decode done");
    begin = text.cbegin() + text.rfind(": decode invoked"); // last decode position
    regex_search(begin, end, m, regex(pattern));
    ts2 = m[1];
    infos[idx].d3 = timediff(ts2, ts1);
}
