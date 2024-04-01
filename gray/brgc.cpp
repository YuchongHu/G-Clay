#include <iostream>
#include <bits/stdc++.h>
#include "Perm.h"
using namespace std;
/*
2^9, 0 1 -> 00 01 11 10 -> 000 001 011 010 110 111 101 100
3^5, 0 1 2 -> 00 01 02 12 11 10 20 21 22
4^5, 0 1 2 3 -> 00 01 02 03 13 12 11 10 20 21 22 23 33 32 31 30


3^5, k=12, m=3
 0 (0,0)  IO:    1 -> 1   
 1 (1,0)  IO:    1 -> 1   
 2 (2,0)  IO:    1 -> 1   
 3 (0,1)  IO:    3 -> 2   
 4 (1,1)  IO:    3 -> 3   
 5 (2,1)  IO:    3 -> 2   
 6 (0,2)  IO:    9 -> 5   
 7 (1,2)  IO:    9 -> 9   
 8 (2,2)  IO:    9 -> 5   
 9 (0,3)  IO:   27 -> 14  
10 (1,3)  IO:   27 -> 27  
11 (2,3)  IO:   27 -> 14  
12 (0,4)  IO:   81 -> 41  
13 (1,4)  IO:   81 -> 81  
14 (2,4)  IO:   81 -> 41  
All-IO: 363 -> 247, reduce ratio: 31.956%

4^5, k=16, m=4
 0 (0,0)  IO:    1 -> 1   
 1 (1,0)  IO:    1 -> 1   
 2 (2,0)  IO:    1 -> 1   
 3 (3,0)  IO:    1 -> 1   
 4 (0,1)  IO:    4 -> 3   
 5 (1,1)  IO:    4 -> 4   
 6 (2,1)  IO:    4 -> 4   
 7 (3,1)  IO:    4 -> 2   
 8 (0,2)  IO:   16 -> 9   
 9 (1,2)  IO:   16 -> 16  
10 (2,2)  IO:   16 -> 16  
11 (3,2)  IO:   16 -> 8   
12 (0,3)  IO:   64 -> 33  
13 (1,3)  IO:   64 -> 64  
14 (2,3)  IO:   64 -> 64  
15 (3,3)  IO:   64 -> 32  
16 (0,4)  IO:  256 -> 129 
17 (1,4)  IO:  256 -> 256 
18 (2,4)  IO:  256 -> 256 
19 (3,4)  IO:  256 -> 128 
All-IO: 1364 -> 1028, reduce ratio: 24.633%
*/

vector<int> to_plain(int value);
bool check_gray(vector<int>& order);
const int BASE = 3;
const int DIGITS = 4;

int main() {
    vector<int> order;
    for (int i=0; i<BASE; i++) {
        order.push_back(i);
    }
    int a = 1, cur = 1;
    while (cur<DIGITS) {
        a *= BASE;
        vector<int> next;
        bool flag = true;
        for (int i=0; i<BASE; i++) {
            if (flag) {
                for (int j=0; j<order.size(); j++) {
                    next.push_back(i*a + order[j]);
                }
            } else {
                for (int j=order.size()-1; j>=0; j--) {
                    next.push_back(i*a + order[j]);
                }
            }
            flag = !flag;
        }
        order = next;
        cur++;
    }

    if (check_gray(order)) {
        printf("Is gray code.\n");
    } else {
        perror("gray code error");
        exit(1);
    }    

    int m=BASE, k=BASE*DIGITS-m;
    Perm perm(k,m);
    perm.show_seq_io();
    perm.show_diff_io(order);

    vector<pair<int,int>> ind;
    perm.order = order;
    cout<<"validate ind-----------"<<endl;
    for (int i=0; i<k+m; i++) {
        ind.clear();
        perm.get_repair_subchunks(i, ind, "clay");
        printf("repair %d: ", i);
        for (auto p : ind) {
            printf("<%d,%d> ", p.first, p.second);
        }
        printf("\n");
    }

}



vector<int> to_plain(int value) {
    vector<int> v(DIGITS);
    int i=0;
    while (value) {
        v[i] = value%BASE;
        i++;
        value /= BASE;
    }
    return v;
}
bool check_gray(vector<int>& order) {   // not cyclic
    vector<int> v = to_plain(order[0]);
    for (int i=1; i<order.size(); i++) {
        vector<int> v2 = to_plain(order[i]);
        int t=0;
        for (int j=0; j<DIGITS; j++) {
            if (v[j] != v2[j]) t++;
        }
        if (t>1) return false;
        v = v2;
    }
    return true;
}