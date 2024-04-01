#include <iostream>
#include <vector>
#include <algorithm>
#include "Perm.h"
using namespace std;

uint64_t fact(int a) {
    uint64_t r=1;
    for (int i=1; i<=a; i++) r*=i;
    return r;
}

vector<vector<int>> get_next_solutions_m2(const Perm& from, const vector<vector<int>>& solutions_from);

void trim_solutions(int a, vector<vector<int>>& solutions, int trim_nz);

int main() {
    Perm perm_2(2,2);
    vector<int> order_2 = perm_2.seq_order;
    vector<vector<int>> st_2;
    do {
        if (perm_2.calc_allnode_io(order_2) == 5) {
            st_2.push_back(order_2);
        }
    } while (next_permutation(order_2.begin(), order_2.end()));
    // solution_2: 0,1,3,2 - 0,2,3,1 - 1,0,2,3 - 1,3,2,0 - 2,0,1,3 - 2,3,1,0 - 3,1,0,2 - 3,2,0,1

    Perm perm_4(4,2), perm_6(6,2), perm_8(8,2), perm_10(10,2), perm_12(12,2), perm_14(14,2), perm_16(16,2);
    vector<vector<int>> st_4, st_6, st_8, st_10, st_12, st_14, st_16;

    // (2,2) a=4 size=8
    printf("k=%-2d m=%d, All-IO: %4d -> %-4d\n", perm_2.k, perm_2.m, perm_2.calc_allnode_io(perm_2.seq_order), perm_2.calc_allnode_io(st_2[0]));

    st_4 = get_next_solutions_m2(perm_2, st_2); // (4,2) a=8 size=32
    printf("k=%-2d m=%d, All-IO: %4d -> %-4d\n", perm_4.k, perm_4.m, perm_4.calc_allnode_io(perm_4.seq_order), perm_4.calc_allnode_io(st_4[0]));

    st_6 = get_next_solutions_m2(perm_4, st_4); // (6,2) a=16 size=256
    printf("k=%-2d m=%d, All-IO: %4d -> %-4d\n", perm_6.k, perm_6.m, perm_6.calc_allnode_io(perm_6.seq_order), perm_6.calc_allnode_io(st_6[0]));

    const int TRIM_NZ = 32;
    st_8 = get_next_solutions_m2(perm_6, st_6); // (8,2) a=32 size=8192, size=1024(trimmed)
    printf("k=%-2d m=%d, All-IO: %4d -> %-4d\n", perm_8.k, perm_8.m, perm_8.calc_allnode_io(perm_8.seq_order), perm_8.calc_allnode_io(st_8[0]));
    trim_solutions(perm_8.a, st_8, TRIM_NZ);
    
    st_10 = get_next_solutions_m2(perm_8, st_8); // (10,2) a=64 size=2048(trimmed)
    printf("k=%-2d m=%d, All-IO: %4d -> %-4d\n", perm_10.k, perm_10.m, perm_10.calc_allnode_io(perm_10.seq_order), perm_10.calc_allnode_io(st_10[0]));
    trim_solutions(perm_10.a, st_10, TRIM_NZ);

    st_12 = get_next_solutions_m2(perm_10, st_10); // (12,2) a=128 size=4096(trimmed)
    printf("k=%-2d m=%d, All-IO: %4d -> %-4d\n", perm_12.k, perm_12.m, perm_12.calc_allnode_io(perm_12.seq_order), perm_12.calc_allnode_io(st_12[0]));
    trim_solutions(perm_12.a, st_12, TRIM_NZ);

    st_14 = get_next_solutions_m2(perm_12, st_12); // (14,2) a=256 size=8192(trimmed)
    printf("k=%-2d m=%d, All-IO: %4d -> %-4d\n", perm_14.k, perm_14.m, perm_14.calc_allnode_io(perm_14.seq_order), perm_14.calc_allnode_io(st_14[0]));
    trim_solutions(perm_14.a, st_14, TRIM_NZ);

    st_16 = get_next_solutions_m2(perm_14, st_14); // (16,2) a=512 size=524288
    printf("k=%-2d m=%d, All-IO: %4d -> %-4d\n", perm_16.k, perm_16.m, perm_16.calc_allnode_io(perm_16.seq_order), perm_16.calc_allnode_io(st_16[0]));

    printf("--------- st_16 trimmed size: %ld ----------\n", st_16.size());
    perm_16.show_diff_io(st_16[42]);

    /*
Order: 0,1,3,2,6,4,5,7,15,13,12,14,10,8,9,11,27,25,24,26,30,28,29,31,23,21,20,22,18,16,17,19,51,49,48,50,54,52,53,55,63,61,60,62,58,56,57,59,43,41,40,42,46,44,45,47,39,37,36,38,34,32,33,35,99,98,96,97,101,103,102,100,108,110,111,109,105,104,106,107,123,121,120,122,126,124,125,127,119,117,116,118,114,112,113,115,83,82,80,81,85,87,86,84,92,94,95,93,89,88,90,91,75,73,72,74,78,76,77,79,71,69,68,70,66,64,65,67,195,193,192,194,198,196,197,199,207,205,204,206,202,203,201,200,216,218,219,217,221,223,222,220,212,214,215,213,209,211,210,208,240,241,243,242,246,244,245,247,255,253,252,254,250,248,249,251,235,233,232,234,238,236,237,239,231,229,228,230,226,224,225,227,163,162,160,161,165,167,166,164,172,174,175,173,169,171,170,168,184,186,187,185,189,191,190,188,180,182,183,181,177,179,178,176,144,146,147,145,149,151,150,148,156,158,159,157,153,152,154,155,139,137,136,138,142,140,141,143,135,133,132,134,130,131,129,128,384,385,387,386,390,388,389,391,399,397,396,398,394,392,393,395,411,409,408,410,414,412,413,415,407,405,404,406,402,400,401,403,435,433,432,434,438,436,437,439,447,445,444,446,442,443,441,440,424,426,427,425,429,431,430,428,420,422,423,421,417,419,418,416,480,482,483,481,485,487,486,484,492,494,495,493,489,488,490,491,507,505,504,506,510,508,509,511,503,501,500,502,498,496,497,499,467,466,464,465,469,471,470,468,476,478,479,477,473,472,474,475,459,457,456,458,462,460,461,463,455,453,452,454,450,448,449,451,323,321,320,322,326,324,325,327,335,333,332,334,330,331,329,328,344,346,347,345,349,351,350,348,340,342,343,341,337,339,338,336,368,369,371,370,374,372,373,375,383,381,380,382,378,379,377,376,360,362,363,361,365,367,366,364,356,358,359,357,353,355,354,352,288,290,291,289,293,295,294,292,300,302,303,301,297,299,298,296,312,314,315,313,317,319,318,316,308,310,311,309,305,307,306,304,272,274,275,273,277,279,278,276,284,286,287,285,281,280,282,283,267,266,264,265,269,268,270,271,263,262,260,261,257,256,258,259
 0 (0,0)  IO:    1 -> 1   
 1 (1,0)  IO:    1 -> 1   
 2 (0,1)  IO:    2 -> 2   
 3 (1,1)  IO:    2 -> 1   
 4 (0,2)  IO:    4 -> 3   
 5 (1,2)  IO:    4 -> 2   
 6 (0,3)  IO:    8 -> 5   
 7 (1,3)  IO:    8 -> 4   
 8 (0,4)  IO:   16 -> 9   
 9 (1,4)  IO:   16 -> 8   
10 (0,5)  IO:   32 -> 17  
11 (1,5)  IO:   32 -> 16  
12 (0,6)  IO:   64 -> 33  
13 (1,6)  IO:   64 -> 32  
14 (0,7)  IO:  128 -> 117 
15 (1,7)  IO:  128 -> 117 
16 (0,8)  IO:  256 -> 76  
17 (1,8)  IO:  256 -> 76  
All-IO: 1022 -> 520
    */
}



void appendv(vector<int>& v, const vector<int>& add) {
    for (auto i:add) v.push_back(i);
}
vector<int> plusv(const vector<int>& v, int plus) {
    vector<int> tmp(v);
    for (auto& i:tmp) i+=plus;
    return tmp;
}
vector<int> symmetryv(const vector<int>& v, int a) {
    vector<int> tmp(v);
    for (auto& i:tmp) i=a-i;
    return tmp;
}

vector<vector<int>> get_next_solutions_m2(const Perm& from, const vector<vector<int>>& solutions_from) {
    vector<vector<int>> solutions_next;
    int a = from.a;
    int ns = solutions_from.size();
    int nz = ns / from.a;
    for (const vector<int>& order : solutions_from) {
        vector<int> order_next;
        appendv(order_next, order);
        for (int i=order.back()*nz, j=0; j<nz; i++, j++) {
            appendv(order_next, plusv(solutions_from[i], a));
            solutions_next.push_back(order_next);
            order_next.resize(a);
        }
    }
    int sz = solutions_next.size();
    for (int i=sz-1; i>=0; i--) {
        solutions_next.push_back(symmetryv(solutions_next[i], 2*a-1));
    }
    return solutions_next;
}

void trim_solutions(int a, vector<vector<int>>& solutions, int trim_nz) {
    int nz = solutions.size() / a;
    for (int i=0; i<a; i++) {
        auto begin = solutions.begin()+i*trim_nz+trim_nz;
        solutions.erase(begin, begin+nz-trim_nz);
    }
}