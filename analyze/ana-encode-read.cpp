#include <iostream>
#include <vector>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <regex>
#include "util.h"
using namespace std;
/*
analyze encode and read time
*/

#define REPAIR_NO 10
struct Info {
    double d1, d2;
};
Info infos[99];

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
    cout<<"--------------- encode ---------------"<<endl;
    for (int i=1; i<argc; i++) {
        printf("%.3f - Encode of %25s\n", infos[i].d1, argv[i]);
    }
    cout<<"---------------  read  ---------------"<<endl;
    for (int i=1; i<argc; i++) {
        printf("%.3f - Read of   %25s\n", infos[i].d2, argv[i]);
    }
    cout<<"----------------- Sum -----------------"<<endl;
    for (int i=1; i<argc; i++) {
        double sum = infos[i].d1 + infos[i].d2;
        printf("%.3f - Sum of    %25s\n", sum, argv[i]);
    }
    
    if (argc == 2) {   
        cout<<"-----------------"<<endl;
        printf("%.3f,%.3f\n", infos[1].d1, infos[1].d2);
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

    // stage 1
    pattern = timepattern + string(".*encode_chunks invoked");
    regex_search(begin, end, m, regex(pattern));
    ts1 = m[1];
    pattern = timepattern + string(".*encode_chunks done");
    begin = text.cbegin() + text.rfind("encode_chunks invoked");
    regex_search(begin, end, m, regex(pattern));
    ts2 = m[1];
    infos[idx].d1 = timediff(ts2, ts1);

    // stage 2
    pattern = timepattern + string(".*store->read");
    regex_search(begin, end, m, regex(pattern));
    ts1 = m[1];
    pattern = timepattern + string(".*full-object read done");
    begin = text.cbegin() + text.rfind("store->read");
    regex_search(begin, end, m, regex(pattern));
    ts2 = m[1];
    infos[idx].d2 = timediff(ts2, ts1);
}
