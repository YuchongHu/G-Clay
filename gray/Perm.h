#include <algorithm>
#include <iostream>
#include <random>
#include <chrono>
#include <assert.h>
#include <set>
using namespace std;

ostream& prt_vector(const vector<int>& v) {
    cout<<v[0];
    for (int i=1; i<v.size(); i++) cout<<","<<v[i];
    return cout;
}

static int pow_int(int a, int x) {
  int power = 1;
  while (x) {
    if (x & 1) power *= a;
    x /= 2;
    a *= a;
  }
  return power;
}

class Perm {
public:
    int k, m;
    int q, t, a; 
    std::vector<int> seq_order; 
    std::vector<int> order;
    std::vector<int> z_vec;

    Perm(int k, int m) : k(k), m(m) {
        assert((k+m)%m == 0);
        q = m;
        t = (k+m)/m;
        a=1;
        for (int i=0; i<t; i++) a*=q;
        for (int i=0; i<a; i++) seq_order.push_back(i);
        z_vec.resize(t);
    }

    void get_plane_vector(int z) {
        for (int i = 0; i < t; i++) {
            z_vec[t-1-i] = z % q;
            z = (z - z_vec[t-1-i]) / q;
        }
    }
    bool check_z(int z, int node) {
        int x = node%q;
        int y = node/q;
        get_plane_vector(z);
        return x==z_vec[y];
    }
    int calc_node_io(const vector<int>& v, int node) {
        int node_io=0;
        int i=0;
        while (i<v.size()) {
            if (check_z(v[i], node)) {
                node_io++;
                int j=i+1;
                while (j<v.size() && check_z(v[j], node)) j++;
                i=j;
            } else {
                i++;
            }
        }
        return node_io;
    }

    int calc_allnode_io(const vector<int>& v) {
        int allnode_io=0;
        for (int node=0; node<k+m; node++) {
            allnode_io += calc_node_io(v, node);
        }
        return allnode_io;
    }

    void show_seq_io() {
        printf("------------ %-15s ------------\n", __func__);
        cout<<"Seq-Order: ";
        prt_vector(seq_order)<<endl;
        int allnode_io=0;
        for (int node=0; node<k+m; node++) {
            int x=node%q;
            int y=node/q;
            printf("%2d (%d,%d)  z:", node, x, y);
            for (int& z : seq_order) {
                if (check_z(z, node)) printf("%5d", z);
            }
            int node_io = calc_node_io(seq_order, node);
            allnode_io += node_io;
            cout<<", IO: "<<node_io<<endl;
        }
        cout<<"All-IO: "<<allnode_io<<endl;
    }

    void show_diff_io(const vector<int>& order) {
        printf("------------ %-15s ------------\n", __func__);
        // cout<<"Seq-Order: ";
        // prt_vector(seq_order)<<endl;
        cout<<"Order: ";
        prt_vector(order)<<endl;
        int allnode_io_seq=0, allnode_io=0;
        int node_io_seq, node_io;
        for (int node=0; node<k+m; node++) {
            int x=node%q;
            int y=node/q;
            printf("%2d (%d,%d)  ", node, x, y);
            node_io_seq = calc_node_io(seq_order, node);
            allnode_io_seq += node_io_seq;
            node_io = calc_node_io(order, node);
            allnode_io += node_io;
            printf("IO: %4d -> %-4d\n", node_io_seq, node_io);
        }
        printf("All-IO: %d -> %d, reduce ratio: %.3f%%\n", allnode_io_seq, allnode_io, double(allnode_io_seq-allnode_io)/allnode_io_seq*100);
    }

    void find_minio() {
        printf("------------ %-15s ------------\n", __func__);
        vector<int> order = seq_order;
        int min_allnode_io=(k+m)*a;
        int allperms=0, minperms=0;
        do {
            min_allnode_io = min(min_allnode_io, calc_allnode_io(order));
        } while (next_permutation(order.begin(), order.end())); 
        do {
            if (calc_allnode_io(order)==min_allnode_io) {
                prt_vector(order)<<' ';
                printf("%d+%d IO: ", k, m);
                for (int node=0; node<k+m; node++) {
                    printf("%d ", calc_node_io(order, node));
                }
                printf("\n");
                minperms++;
            }
            allperms++;
        } while (next_permutation(order.begin(), order.end()));
        printf(">>> allperms: %d, minperms: %d, min-All-IO: %d\n", allperms, minperms, min_allnode_io);
    }

    void try_minio(int iterations) {
        printf("------------ %-15s ------------\n", __func__);
        vector<int> order = seq_order;
        random_device rd;
        default_random_engine en(rd());

        auto start = chrono::steady_clock::now(); 
        int min_allnode_io = a*2-2; 
        int min_cnt = 0;
        vector<int> vmin;
        printf("> k=%d, m=%d, a=%d, seq_io=%d, Iteration times: %d\n", k, m, a, min_allnode_io, iterations);
        while (iterations--) {
            shuffle(order.begin(), order.end(), en);
            int allnode_io = calc_allnode_io(order);
            if (allnode_io < min_allnode_io) {
                min_allnode_io = allnode_io;
                min_cnt = 1;
                vmin = order;
                printf("Found current min: %d\n", min_allnode_io);
            } else if (allnode_io == min_allnode_io) {
                min_cnt++;
                printf("Again %d for %dth\n", min_allnode_io, min_cnt);
            }
        }
    }

    void show_couple_m2() {
        auto pow = [](int a, int x) -> int {
            int out=1;
            while (x--) out*=a;
            return out;
        };
        for (int node=0; node<k+m; node++) {
            printf("--- %d ---\n", node);
            int x=node%q, y=node/q;
            for (int z=0; z<a; z++) {
                get_plane_vector(z);
                printf("%2d: ", z);
                if (x==z_vec[y]) {
                    // no output
                } else {
                    int node2=y*q+z_vec[y];
                    int z2 = z + (x - z_vec[y])*pow(q,t-1-y);
                    printf("pair=%d, z=%d", node2, z2);
                }
                printf("\n");
            }
        }
    }

    void get_repair_subchunks(const int &lost_node, vector<pair<int, int>> &repair_sub_chunks_ind, string mymode)
    {
        const int y_lost = lost_node / q;
        const int x_lost = lost_node % q;

        const int seq_sc_count = pow_int(q, t-1-y_lost);
        const int num_seq = pow_int(q, y_lost);

        int index = x_lost * seq_sc_count;
        for (int ind_seq = 0; ind_seq < num_seq; ind_seq++) {
            repair_sub_chunks_ind.push_back(make_pair(index, seq_sc_count));
            index += q * seq_sc_count;
        }
        
        if (mymode == "myclay") {
            vector<pair<int, int>> out;
            set<int> to_read;
            for (auto& p:repair_sub_chunks_ind) {
                for (int i=0; i<p.second; i++) to_read.insert(p.first+i);
            }
            size_t i=0, j;
            while (i<order.size()) {
                if (to_read.count(order[i])) {
                    j=i+1;
                    while (j<order.size() && to_read.count(order[j])) j++;
                    out.push_back({i,j-i});
                    i=j;
                } else {
                    i++;
                }
            }
            repair_sub_chunks_ind.assign(out.begin(), out.end());
        }
    }
};