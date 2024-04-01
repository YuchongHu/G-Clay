// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2018 Indian Institute of Science <office.ece@iisc.ac.in>
 *
 * Author: Myna Vajha <mynaramana@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 */

#include <errno.h>
#include <algorithm>
#include <sstream>

#include "ErasureCodeMyClay.h"

#include "common/debug.h"
#include "erasure-code/ErasureCodePlugin.h"
#include "include/ceph_assert.h"
#include "include/str_map.h"
#include "include/stringify.h"
#include "osd/osd_types.h"

#include <time.h>
#include <stdlib.h>


#define dout_context g_ceph_context
#define dout_subsys ceph_subsys_osd
#undef dout_prefix
#define dout_prefix _prefix(_dout)

#define LARGEST_VECTOR_WORDSIZE 16
#define talloc(type, num) (type *) malloc(sizeof(type)*(num))

#define MYCLAY_DEBUG_CHUNK 0

using namespace std;
using namespace ceph;

static ostream& _prefix(std::ostream* _dout)
{
  return *_dout << "ErasureCodeMyClay: ";
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

string myclay_str_to_string(const char* str, int len, bool hexmode);

ErasureCodeMyClay::~ErasureCodeMyClay()
{
  for (int i = 0; i < q*t; i++) {
    if (U_buf[i].length() != 0) U_buf[i].clear();
  }
}

int ErasureCodeMyClay::init(ErasureCodeProfile &profile,
			  ostream *ss)
{
  dout(0)<<"--- INIT ---"<<dendl;
  int r;
  r = parse(profile, ss); 
  if (r)
    return r;

  // clay seq order
  for (int i=0; i<sub_chunk_no; i++) order.push_back(i);
  if (mymode=="myclay") {
    if (k==6 && m==2) order = std::vector<int>{1,5,7,3,2,6,4,0,8,10,11,15,14,12,13,9};
    if (k==16 && m==2) order = std::vector<int>{0,1,3,2,6,4,5,7,15,13,12,14,10,8,9,11,27,25,24,26,30,28,29,31,23,21,20,22,18,16,17,19,51,49,48,50,54,52,53,55,63,61,60,62,58,56,57,59,43,41,40,42,46,44,45,47,39,37,36,38,34,32,33,35,99,98,96,97,101,103,102,100,108,110,111,109,105,104,106,107,123,121,120,122,126,124,125,127,119,117,116,118,114,112,113,115,83,82,80,81,85,87,86,84,92,94,95,93,89,88,90,91,75,73,72,74,78,76,77,79,71,69,68,70,66,64,65,67,195,193,192,194,198,196,197,199,207,205,204,206,202,203,201,200,216,218,219,217,221,223,222,220,212,214,215,213,209,211,210,208,240,241,243,242,246,244,245,247,255,253,252,254,250,248,249,251,235,233,232,234,238,236,237,239,231,229,228,230,226,224,225,227,163,162,160,161,165,167,166,164,172,174,175,173,169,171,170,168,184,186,187,185,189,191,190,188,180,182,183,181,177,179,178,176,144,146,147,145,149,151,150,148,156,158,159,157,153,152,154,155,139,137,136,138,142,140,141,143,135,133,132,134,130,131,129,128,384,385,387,386,390,388,389,391,399,397,396,398,394,392,393,395,411,409,408,410,414,412,413,415,407,405,404,406,402,400,401,403,435,433,432,434,438,436,437,439,447,445,444,446,442,443,441,440,424,426,427,425,429,431,430,428,420,422,423,421,417,419,418,416,480,482,483,481,485,487,486,484,492,494,495,493,489,488,490,491,507,505,504,506,510,508,509,511,503,501,500,502,498,496,497,499,467,466,464,465,469,471,470,468,476,478,479,477,473,472,474,475,459,457,456,458,462,460,461,463,455,453,452,454,450,448,449,451,323,321,320,322,326,324,325,327,335,333,332,334,330,331,329,328,344,346,347,345,349,351,350,348,340,342,343,341,337,339,338,336,368,369,371,370,374,372,373,375,383,381,380,382,378,379,377,376,360,362,363,361,365,367,366,364,356,358,359,357,353,355,354,352,288,290,291,289,293,295,294,292,300,302,303,301,297,299,298,296,312,314,315,313,317,319,318,316,308,310,311,309,305,307,306,304,272,274,275,273,277,279,278,276,284,286,287,285,281,280,282,283,267,266,264,265,269,268,270,271,263,262,260,261,257,256,258,259};
  }
  order_ind.resize(sub_chunk_no, 0);
  for (int i=0; i<sub_chunk_no; i++) {
    order_ind[order[i]] = i; 
  }

  dout(0)<<"mymode: "<<mymode<<", io_thre: "<<io_thre<<dendl;
  string s0;
  for (int i:order) s0.append(std::to_string(i)+" ");
  dout(0)<<"order: "<<s0<<dendl;
  dout(0)<<"MYCLAY_DEBUG_CHUNK: "<<MYCLAY_DEBUG_CHUNK<<dendl;
  

  r = ErasureCode::init(profile, ss);
  if (r)
    return r;
  ErasureCodePluginRegistry &registry = ErasureCodePluginRegistry::instance();
  r = registry.factory(mds.profile["plugin"],
		       directory,
		       mds.profile,
		       &mds.erasure_code,
		       ss);
  if (r)
    return r;
  r = registry.factory(pft.profile["plugin"],
		       directory,
		       pft.profile,
		       &pft.erasure_code,
		       ss);
  
  return r;
}

unsigned int ErasureCodeMyClay::get_chunk_size(unsigned int object_size) const
{
  unsigned int alignment_scalar_code = pft.erasure_code->get_chunk_size(1);
  unsigned int alignment = sub_chunk_no * k * alignment_scalar_code;
  
  return round_up_to(object_size, alignment) / k;
}

int ErasureCodeMyClay::minimum_to_decode(const set<int> &want_to_read,
				       const set<int> &available,
				       map<int, vector<pair<int, int>>> *minimum)
{
  // log
  string s1, s2;
  for (int a:want_to_read) s1.append(std::to_string(a)+",");
  for (int a:available) s2.append(std::to_string(a)+",");
  dout(0)<<"minimum_to_decode invoked, want_to_read: " <<s1<<" available: "<<s2<<dendl;

  int r = 0;
  if (is_repair(want_to_read, available)) {
    r = minimum_to_repair(want_to_read, available, minimum);
  } else {
    r = ErasureCode::minimum_to_decode(want_to_read, available, minimum);
  }

  return r;
}

int ErasureCodeMyClay::decode(const set<int> &want_to_read,
			    const map<int, bufferlist> &chunks,
			    map<int, bufferlist> *decoded, int chunk_size)
{
  // log
  dout(0)<<"decode invoked"<<dendl;
  set<int> avail;
  for ([[maybe_unused]] auto& [node, bl] : chunks) {
    avail.insert(node);
    (void)bl;  // silence -Wunused-variable
  }

  if (MYCLAY_DEBUG_CHUNK) {
    dout(0)<<"Before decode:"<<dendl;
    string s1;
    for (int a:want_to_read) s1.append(std::to_string(a)+",");
    dout(0)<<"want_to_read: " <<s1<<dendl;
    dout(0)<<"chunk_size: "<<chunk_size<<", chunks[0] size: "<<(chunks.begin()->second.length())<<dendl;
    for (auto& p : chunks) {
      dout(0)<<"chunks["<<p.first<<"]: "<<myclay_str_to_string(p.second.to_str().c_str(), p.second.length(), p.first>=k)<<dendl;
    }
    if (decoded->empty()) {
      dout(0)<<"decoded[] empty"<<dendl;
    } else {
      for (auto& p : *decoded) {
        dout(0)<<"decoded["<<p.first<<"]: "<<myclay_str_to_string(p.second.to_str().c_str(), p.second.length(), p.first>=k)<<dendl;
      }
    }
  }

  int ret=0;
  if (is_repair(want_to_read, avail) && 
         ((unsigned int)chunk_size > chunks.begin()->second.length())) {
    dout(0)<<" now to repair()"<<dendl;
    ret = repair(want_to_read, chunks, decoded, chunk_size);
  } else {
    dout(0)<<" now to ErasureCode::_decode"<<dendl;
    ret = ErasureCode::_decode(want_to_read, chunks, decoded);
  }

  if (MYCLAY_DEBUG_CHUNK) {
    dout(0)<<"After decode:"<<dendl;
    for (auto& p : *decoded) {
      dout(0)<<"decoded["<<p.first<<"], "<<"size "<<p.second.length()<<": "<<myclay_str_to_string(p.second.to_str().c_str(), p.second.length(), p.first>=k)<<dendl;
    }
  }

  // log
  dout(0)<<"decode done"<<dendl;

  return ret;
}

void p(const set<int> &s) { cerr << s; } // for gdb

string myclay_str_to_string(const char* str, int len, bool hexmode) {
  stringstream ss;
  if (hexmode) {
    char hexchar[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    string s("00");
    for (int i=0; i<len; i++) {
      uint8_t a = str[i];
      s[0]=hexchar[a/16];
      s[1]=hexchar[a%16];
      ss<<s<<' ';
    }
  } else {
    for (int i=0; i<len; i++) {
      ss<<str[i]<<' ';
    }
  }
  return ss.str();
}

int ErasureCodeMyClay::encode_chunks(const set<int> &want_to_encode,
				   map<int, bufferlist> *encoded)
{
  // log
  dout(0) << "encode_chunks invoked" << dendl;

  if (MYCLAY_DEBUG_CHUNK) {
    dout(0) << "Before encode:"<<dendl;
    for (int i=0; i<k; i++) {
      dout(0)<<i<<": "<<myclay_str_to_string((*encoded)[i].c_str(), (*encoded)[0].length(), false)<<dendl;
    }
  }

  map<int, bufferlist> chunks;
  set<int> parity_chunks;
  int chunk_size = (*encoded)[0].length();

  for (int i = 0; i < k + m; i++) {
    if (i < k) {
      chunks[i] = (*encoded)[i];
    } else {
      chunks[i+nu] = (*encoded)[i];
      parity_chunks.insert(i+nu);
    }
  }

  for (int i = k; i < k + nu; i++) {
    bufferptr buf(buffer::create_aligned(chunk_size, SIMD_ALIGN));
    buf.zero();
    chunks[i].push_back(std::move(buf));
  }

  if (mymode == "myclay") {
    int sub_chunk_size = chunk_size / sub_chunk_no;
    char* tmp_buf = (char*)malloc(chunk_size);
    for (int chk=0; chk<k; chk++) {
      char* orig_buf = (*encoded)[chk].c_str();
      memcpy(tmp_buf, orig_buf, chunk_size);
      for (int i=0; i<sub_chunk_no; i++) {
        int z=order[i];
        memcpy(orig_buf+i*sub_chunk_size, tmp_buf+z*sub_chunk_size, sub_chunk_size);
      }
    }
    free(tmp_buf);
    // after reorder, print
    if (MYCLAY_DEBUG_CHUNK) {
      dout(0) << "After reorder:"<<dendl;
      for (int i=0; i<k; i++) {
        dout(0)<<i<<": "<<myclay_str_to_string((*encoded)[i].c_str(), (*encoded)[0].length(), false)<<dendl;
      }
    }
  }

  int res = decode_layered(parity_chunks, &chunks);
  for (int i = k ; i < k + nu; i++) {
    // need to clean some of the intermediate chunks here!!
    chunks[i].clear();
  }
  
  if (MYCLAY_DEBUG_CHUNK) {
    dout(0) << "After encode:"<<dendl;
    for (int i=k; i<k+m; i++) {
      dout(0)<<i<<": "<<myclay_str_to_string((*encoded)[i].c_str(), (*encoded)[0].length(), true)<<dendl;
    }
  }

  dout(0) << "encode_chunks done" << dendl;

  return res;
}

int ErasureCodeMyClay::decode_chunks(const set<int> &want_to_read,
				   const map<int, bufferlist> &chunks,
				   map<int, bufferlist> *decoded)
{
  set<int> erasures;
  map<int, bufferlist> coded_chunks;

  for (int i = 0; i < k + m; i++) {
    if (chunks.count(i) == 0) {
      erasures.insert(i < k ? i : i+nu);
    }
    ceph_assert(decoded->count(i) > 0);
    coded_chunks[i < k ? i : i+nu] = (*decoded)[i];
  }
  int chunk_size = coded_chunks[0].length();

  for (int i = k; i < k+nu; i++) {
    bufferptr buf(buffer::create_aligned(chunk_size, SIMD_ALIGN));
    buf.zero();
    coded_chunks[i].push_back(std::move(buf));
  }
  int res = decode_layered(erasures, &coded_chunks);
  for (int i = k; i < k+nu; i++) {
    coded_chunks[i].clear();
  }
  return res;
}

int ErasureCodeMyClay::parse(ErasureCodeProfile &profile,
			   ostream *ss)
{
  int err = 0;
  err = ErasureCode::parse(profile, ss);
  err |= to_int("k", profile, &k, DEFAULT_K, ss);
  err |= to_int("m", profile, &m, DEFAULT_M, ss);

  err |= sanity_check_k_m(k, m, ss);

  err |= to_int("d", profile, &d, std::to_string(k+m-1), ss);

  mds.profile["jlog"] = "false";
  pft.profile["jlog"] = "false";
  err |= to_string("mymode", profile, &mymode, "clay", ss);
  err |= to_int("io_thre", profile, &io_thre, "256", ss);

  // check for scalar_mds in profile input
  if (profile.find("scalar_mds") == profile.end() ||
      profile.find("scalar_mds")->second.empty()) {
    mds.profile["plugin"] = "jerasure";
    pft.profile["plugin"] = "jerasure";
  } else {
    std::string p = profile.find("scalar_mds")->second;
    if ((p == "jerasure") || (p == "isa") || (p == "shec")) {
      mds.profile["plugin"] = p;
      pft.profile["plugin"] = p;
    } else {
        *ss << "scalar_mds " << mds.profile["plugin"] <<
               "is not currently supported, use one of 'jerasure',"<<
               " 'isa', 'shec'" << std::endl;
        err = -EINVAL;
        return err;
    }
  }

  if (profile.find("technique") == profile.end() ||
      profile.find("technique")->second.empty()) {
    if ((mds.profile["plugin"]=="jerasure") || (mds.profile["plugin"]=="isa") ) {
      mds.profile["technique"] = "reed_sol_van";
      pft.profile["technique"] = "reed_sol_van";
    } else {
      mds.profile["technique"] = "single";
      pft.profile["technique"] = "single";
    }
  } else {
    std::string p = profile.find("technique")->second;
    if (mds.profile["plugin"] == "jerasure") {
      if ( (p == "reed_sol_van") || (p == "reed_sol_r6_op") || (p == "cauchy_orig")
           || (p == "cauchy_good") || (p == "liber8tion")) {
        mds.profile["technique"] = p;
        pft.profile["technique"] = p;
      } else {
        *ss << "technique " << p << "is not currently supported, use one of "
	    << "reed_sol_van', 'reed_sol_r6_op','cauchy_orig',"
	    << "'cauchy_good','liber8tion'"<< std::endl;
        err = -EINVAL;
        return err;
      }
    } else if (mds.profile["plugin"] == "isa") {
      if ( (p == "reed_sol_van") || (p == "cauchy")) {
        mds.profile["technique"] = p;
        pft.profile["technique"] = p;
      } else {
        *ss << "technique " << p << "is not currently supported, use one of"
	    << "'reed_sol_van','cauchy'"<< std::endl;
        err = -EINVAL;
        return err;
      }
    } else {
      if ( (p == "single") || (p == "multiple")) {
        mds.profile["technique"] = p;
        pft.profile["technique"] = p;
      } else {
        *ss << "technique " << p << "is not currently supported, use one of"<<
               "'single','multiple'"<< std::endl;
        err = -EINVAL;
        return err;
      }
    }
  }
  if ((d < k) || (d > k + m - 1)) {
    *ss << "value of d " << d
        << " must be within [ " << k << "," << k+m-1 << "]" << std::endl;
    err = -EINVAL;
    return err;
  }

  q = d - k + 1;
  if ((k + m) % q) {
    nu = q - (k + m) % q;
  } else {
    nu = 0;
  }

  if (k+m+nu > 254) {
    err = -EINVAL;
    return err;
  }

  if (mds.profile["plugin"] == "shec") {
    mds.profile["c"] = '2';
    pft.profile["c"] = '2';
  }
  mds.profile["k"] = std::to_string(k+nu);
  mds.profile["m"] = std::to_string(m);
  mds.profile["w"] = '8';

  pft.profile["k"] = '2';
  pft.profile["m"] = '2';
  pft.profile["w"] = '8';

  t = (k + m + nu) / q;
  sub_chunk_no = pow_int(q, t);

  dout(0) << __func__
	   << " (q,t,nu)=(" << q << "," << t << "," << nu <<")" << dendl; 

  return err;
}

int ErasureCodeMyClay::is_repair(const set<int> &want_to_read,
			       const set<int> &available_chunks) {

  if (includes(available_chunks.begin(), available_chunks.end(),
               want_to_read.begin(), want_to_read.end())) return 0;
  if (want_to_read.size() > 1) return 0;

  int i = *want_to_read.begin();
  int lost_node_id = (i < k) ? i: i+nu;
  for (int x = 0; x < q; x++) {
    int node = (lost_node_id/q)*q+x;
    node = (node < k) ? node : node-nu;
    if (node != i) { // node in the same group other than erased node
      if (available_chunks.count(node) == 0) return 0;
    }
  }

  if (available_chunks.size() < (unsigned)d) return 0;
  return 1;
}

int ErasureCodeMyClay::minimum_to_repair(const set<int> &want_to_read,
				       const set<int> &available_chunks,
				       map<int, vector<pair<int, int>>> *minimum)
{
  int i = *want_to_read.begin();
  int lost_node_index = (i < k) ? i : i+nu;
  int rep_node_index = 0;

  // add all the nodes in lost node's y column.
  vector<pair<int, int>> sub_chunk_ind;
  get_repair_subchunks(lost_node_index, sub_chunk_ind);
  if ((available_chunks.size() >= (unsigned)d)) {
    for (int j = 0; j < q; j++) {
      if (j != lost_node_index%q) {
        rep_node_index = (lost_node_index/q)*q+j;
        if (rep_node_index < k) {
          minimum->insert(make_pair(rep_node_index, sub_chunk_ind));
        } else if (rep_node_index >= k+nu) {
          minimum->insert(make_pair(rep_node_index-nu, sub_chunk_ind));
        }
      }
    }
    for (auto chunk : available_chunks) {
      if (minimum->size() >= (unsigned)d) {
	break;
      }
      if (!minimum->count(chunk)) {
	minimum->emplace(chunk, sub_chunk_ind);
      }
    }
  } else {
    dout(0) << "minimum_to_repair: shouldn't have come here" << dendl;
    ceph_assert(0);
  }
  ceph_assert(minimum->size() == (unsigned)d);
  return 0;
}

void ErasureCodeMyClay::get_repair_subchunks(const int &lost_node,
					   vector<pair<int, int>> &repair_sub_chunks_ind)
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

int ErasureCodeMyClay::get_repair_sub_chunk_count(const set<int> &want_to_read)
{
  int weight_vector[t];
  std::fill(weight_vector, weight_vector + t, 0);
  for (auto to_read : want_to_read) {
    weight_vector[to_read / q]++;
  }

  int repair_subchunks_count = 1;
  for (int y = 0; y < t; y++) {
    repair_subchunks_count = repair_subchunks_count*(q-weight_vector[y]);
  }

  return sub_chunk_no - repair_subchunks_count;
}

int ErasureCodeMyClay::repair(const set<int> &want_to_read,
			    const map<int, bufferlist> &chunks,
			    map<int, bufferlist> *repaired, int chunk_size)
{
  ceph_assert((want_to_read.size() == 1) && (chunks.size() == (unsigned)d));

  int repair_sub_chunk_no = get_repair_sub_chunk_count(want_to_read);
  vector<pair<int, int>> repair_sub_chunks_ind;

  unsigned repair_blocksize = chunks.begin()->second.length();
  assert(repair_blocksize%repair_sub_chunk_no == 0);

  unsigned sub_chunksize = repair_blocksize/repair_sub_chunk_no;
  unsigned chunksize = sub_chunk_no*sub_chunksize;

  ceph_assert(chunksize == (unsigned)chunk_size);

  map<int, bufferlist> recovered_data;
  map<int, bufferlist> helper_data;
  set<int> aloof_nodes;

  for (int i =  0; i < k + m; i++) {
    // included helper data only for d+nu nodes.
    if (auto found = chunks.find(i); found != chunks.end()) { // i is a helper
      if (i<k) {
	helper_data[i] = found->second;
      } else {
	helper_data[i+nu] = found->second;
      }
    } else {
      if (i != *want_to_read.begin()) { // aloof node case.
        int aloof_node_id = (i < k) ? i: i+nu;
        aloof_nodes.insert(aloof_node_id);
      } else {
        bufferptr ptr(buffer::create_aligned(chunksize, SIMD_ALIGN));
	ptr.zero();
        int lost_node_id = (i < k) ? i : i+nu;
        (*repaired)[i].push_back(ptr);
        recovered_data[lost_node_id] = (*repaired)[i];
        get_repair_subchunks(lost_node_id, repair_sub_chunks_ind);
      }
    }
  }

  // this is for shortened codes i.e., when nu > 0
  for (int i=k; i < k+nu; i++) {
    bufferptr ptr(buffer::create_aligned(repair_blocksize, SIMD_ALIGN));
    ptr.zero();
    helper_data[i].push_back(ptr);
  }

  ceph_assert(helper_data.size()+aloof_nodes.size()+recovered_data.size() ==
	      (unsigned) q*t);
  
  int r = repair_one_lost_chunk(recovered_data, aloof_nodes,
				helper_data, repair_blocksize,
				repair_sub_chunks_ind);

  // clear buffers created for the purpose of shortening
  for (int i = k; i < k+nu; i++) {
    helper_data[i].clear();
  }

  return r;
}

int ErasureCodeMyClay::repair_one_lost_chunk(map<int, bufferlist> &recovered_data,
					   set<int> &aloof_nodes,
					   map<int, bufferlist> &helper_data,
					   int repair_blocksize,
					   vector<pair<int,int>> &repair_sub_chunks_ind)
{
  unsigned repair_subchunks = (unsigned)sub_chunk_no / q;
  unsigned sub_chunksize = repair_blocksize / repair_subchunks;

  int z_vec[t];
  map<int, set<int> > ordered_planes;
  map<int, int> repair_plane_to_ind;
  int count_retrieved_sub_chunks = 0;
  int plane_ind = 0;

  bufferptr buf(buffer::create_aligned(sub_chunksize, SIMD_ALIGN));
  bufferlist temp_buf;
  temp_buf.push_back(buf);
  for (auto [index,count] : repair_sub_chunks_ind) {
    for (int j = index; j < index + count; j++) {
      int j2 = order[j]; 

      get_plane_vector(j2, z_vec);
      int order_i = 0;
      // check across all erasures and aloof nodes
      for ([[maybe_unused]] auto& [node, bl] : recovered_data) {
        if (node % q == z_vec[node / q]) order_i++;
        (void)bl;  // silence -Wunused-variable
      }
      for (auto node : aloof_nodes) {
        if (node % q == z_vec[node / q]) order_i++;
      }
      ceph_assert(order_i > 0); 
      ordered_planes[order_i].insert(j2);
      // to keep track of a sub chunk within helper buffer recieved
      repair_plane_to_ind[j2] = plane_ind;
      plane_ind++;
    }
  }
  assert((unsigned)plane_ind == repair_subchunks);

  int plane_count = 0;
  for (int i = 0; i < q*t; i++) {
    if (U_buf[i].length() == 0) {
      bufferptr buf(buffer::create_aligned(sub_chunk_no*sub_chunksize, SIMD_ALIGN));
      buf.zero();
      U_buf[i].push_back(std::move(buf));
    }
  }

  int lost_chunk;
  int count = 0;
  for ([[maybe_unused]] auto& [node, bl] : recovered_data) {
    lost_chunk = node;
    count++;
    (void)bl;  // silence -Wunused-variable
  }
  ceph_assert(count == 1);

  set<int> erasures;
  for (int i = 0; i < q; i++) {
    erasures.insert(lost_chunk - lost_chunk % q + i);
  }
  for (auto node : aloof_nodes) {
    erasures.insert(node);
  }

  for (int order_i = 1; ;order_i++) {
    if (ordered_planes.count(order_i) == 0) {
      break;
    }
    plane_count += ordered_planes[order_i].size();
    for (auto z : ordered_planes[order_i]) {
      get_plane_vector(z, z_vec);

      for (int y = 0; y < t; y++) {
	for (int x = 0; x < q; x++) {
	  int node_xy = y*q + x;
	  map<int, bufferlist> known_subchunks;
	  map<int, bufferlist> pftsubchunks;
	  set<int> pft_erasures;
	  if (erasures.count(node_xy) == 0) {
	    assert(helper_data.count(node_xy) > 0);
	    int z_sw = z + (x - z_vec[y])*pow_int(q,t-1-y);
	    int node_sw = y*q + z_vec[y];
	    int i0 = 0, i1 = 1, i2 = 2, i3 = 3;
	    if (z_vec[y] > x) {
	      i0 = 1;
	      i1 = 0;
	      i2 = 3;
	      i3 = 2;
	    }
	    if (aloof_nodes.count(node_sw) > 0) {
	      assert(repair_plane_to_ind.count(z) > 0);
	      assert(repair_plane_to_ind.count(z_sw) > 0);
	      pft_erasures.insert(i2);
	      known_subchunks[i0].substr_of(helper_data[node_xy], repair_plane_to_ind[z]*sub_chunksize, sub_chunksize);
	      known_subchunks[i3].substr_of(U_buf[node_sw], z_sw*sub_chunksize, sub_chunksize);
	      pftsubchunks[i0] = known_subchunks[i0];
	      pftsubchunks[i1] = temp_buf;
	      pftsubchunks[i2].substr_of(U_buf[node_xy], z*sub_chunksize, sub_chunksize);
	      pftsubchunks[i3] = known_subchunks[i3];
	      for (int i=0; i<3; i++) {
		pftsubchunks[i].rebuild_aligned(SIMD_ALIGN);
	      }
	      pft.erasure_code->decode_chunks(pft_erasures, known_subchunks, &pftsubchunks);
	    } else {
	      ceph_assert(helper_data.count(node_sw) > 0);
	      ceph_assert(repair_plane_to_ind.count(z) > 0);
	      if (z_vec[y] != x){
		pft_erasures.insert(i2); 
		ceph_assert(repair_plane_to_ind.count(z_sw) > 0);
		known_subchunks[i0].substr_of(helper_data[node_xy], repair_plane_to_ind[z]*sub_chunksize, sub_chunksize);
		known_subchunks[i1].substr_of(helper_data[node_sw], repair_plane_to_ind[z_sw]*sub_chunksize, sub_chunksize);
		pftsubchunks[i0] = known_subchunks[i0];
		pftsubchunks[i1] = known_subchunks[i1];
		pftsubchunks[i2].substr_of(U_buf[node_xy], z*sub_chunksize, sub_chunksize);
		pftsubchunks[i3].substr_of(temp_buf, 0, sub_chunksize);
		for (int i=0; i<3; i++) {
		  pftsubchunks[i].rebuild_aligned(SIMD_ALIGN);
		}
		pft.erasure_code->decode_chunks(pft_erasures, known_subchunks, &pftsubchunks);
	      } else {
		char* uncoupled_chunk = U_buf[node_xy].c_str();
		char* coupled_chunk = helper_data[node_xy].c_str();
		memcpy(&uncoupled_chunk[z*sub_chunksize],
		       &coupled_chunk[repair_plane_to_ind[z]*sub_chunksize],
		       sub_chunksize);
	      }
	    }
	  }
	} // x
      } // y
      ceph_assert(erasures.size() <= (unsigned)m);
      decode_uncoupled(erasures, z, sub_chunksize);

      for (auto i : erasures) {
	int x = i % q;
	int y = i / q;
	int node_sw = y*q+z_vec[y];
	int z_sw = z + (x - z_vec[y]) * pow_int(q,t-1-y);
	set<int> pft_erasures;
	map<int, bufferlist> known_subchunks;
	map<int, bufferlist> pftsubchunks;
	int i0 = 0, i1 = 1, i2 = 2, i3 = 3;
	if (z_vec[y] > x) {
	  i0 = 1;
	  i1 = 0;
	  i2 = 3;
	  i3 = 2;
	}
	// make sure it is not an aloof node before you retrieve repaired_data
	if (aloof_nodes.count(i) == 0) {
	  if (x == z_vec[y]) { 
	    char* coupled_chunk = recovered_data[i].c_str();
	    char* uncoupled_chunk = U_buf[i].c_str();
	    // memcpy(&coupled_chunk[z*sub_chunksize],
		  //  &uncoupled_chunk[z*sub_chunksize],
		  //  sub_chunksize);
      memcpy(&coupled_chunk[order_ind[z]*sub_chunksize],
		   &uncoupled_chunk[z*sub_chunksize],
		   sub_chunksize);
	    count_retrieved_sub_chunks++;
	  } else {
	    ceph_assert(y == lost_chunk / q);
	    ceph_assert(node_sw == lost_chunk);
	    ceph_assert(helper_data.count(i) > 0);
	    pft_erasures.insert(i1);
	    known_subchunks[i0].substr_of(helper_data[i], repair_plane_to_ind[z]*sub_chunksize, sub_chunksize);
	    known_subchunks[i2].substr_of(U_buf[i], z*sub_chunksize, sub_chunksize);

	    pftsubchunks[i0] = known_subchunks[i0];
	    // pftsubchunks[i1].substr_of(recovered_data[node_sw], z_sw*sub_chunksize, sub_chunksize);
      pftsubchunks[i1].substr_of(recovered_data[node_sw], order_ind[z_sw]*sub_chunksize, sub_chunksize);
	    pftsubchunks[i2] = known_subchunks[i2];
	    pftsubchunks[i3] = temp_buf;
	    for (int i=0; i<3; i++) {
	      pftsubchunks[i].rebuild_aligned(SIMD_ALIGN);
	    }
	    pft.erasure_code->decode_chunks(pft_erasures, known_subchunks, &pftsubchunks);
	  }
	}
      } // recover all erasures
    } // planes of particular order
  } // order

  return 0;
}


int ErasureCodeMyClay::decode_layered(set<int> &erased_chunks,
                                    map<int, bufferlist> *chunks)
{
  // dout(0)<<"decode_layered invoked."<<dendl;
  // dout(0)<<" erased_chunks: "; for (auto i:erased_chunks) {dout(0)<<i<<' ';} dout(0)<<dendl;
  // dout(0)<<" chunks: "; for (auto& p:*chunks) {dout(0)<<p.first<<": "<<p.second.length()<<", ";} dout(0)<<dendl;
  int num_erasures = erased_chunks.size();

  int size = (*chunks)[0].length(); 
  ceph_assert(size%sub_chunk_no == 0);
  int sc_size = size / sub_chunk_no;

  ceph_assert(num_erasures > 0);

  for (int i = k+nu; (num_erasures < m) && (i < q*t); i++) {
    if ([[maybe_unused]] auto [it, added] = erased_chunks.emplace(i); added) {
      num_erasures++;
      (void)it;  // silence -Wunused-variable
    }
  }
  ceph_assert(num_erasures == m);

  int max_iscore = get_max_iscore(erased_chunks); // dout(0)<<" max_iscore: "<<max_iscore<<dendl;
  int order[sub_chunk_no];
  int z_vec[t];
  for (int i = 0; i < q*t; i++) {
    if (U_buf[i].length() == 0) {
      bufferptr buf(buffer::create_aligned(size, SIMD_ALIGN));
      buf.zero();
      U_buf[i].push_back(std::move(buf));
    }
  }

  set_planes_sequential_decoding_order(order, erased_chunks); // printf(" order: "); for (auto i:order) {printf("%d ", i);} printf("\n");

  for (int iscore = 0; iscore <= max_iscore; iscore++) {
    // printf(" iscore: %d\n", iscore);
   for (int z = 0; z < sub_chunk_no; z++) {
      if (order[z] == iscore) {
        // printf("  decode_erasures z=%d\n", z);
        decode_erasures(erased_chunks, z, chunks, sc_size);
      }
    }

    // printf("  now get C of each above z\n");
    for (int z = 0; z < sub_chunk_no; z++) {
      if (order[z] == iscore) {
	get_plane_vector(z, z_vec);
        for (auto node_xy : erased_chunks) {
          int x = node_xy % q;
          int y = node_xy / q;
	  int node_sw = y*q+z_vec[y];
          if (z_vec[y] != x) {
            if (erased_chunks.count(node_sw) == 0) {
	      recover_type1_erasure(chunks, x, y, z, z_vec, sc_size);
	    } else if (z_vec[y] < x){
              ceph_assert(erased_chunks.count(node_sw) > 0);
              ceph_assert(z_vec[y] != x);
              get_coupled_from_uncoupled(chunks, x, y, z, z_vec, sc_size);
	    }
	  } else {
	    char* C = (*chunks)[node_xy].c_str();
            char* U = U_buf[node_xy].c_str();
            memcpy(&C[order_ind[z]*sc_size], &U[z*sc_size], sc_size);
          }
        }
      }
    } // plane
  } // iscore, order

  return 0;
}


int ErasureCodeMyClay::decode_erasures(const set<int>& erased_chunks, int z,
				     map<int, bufferlist>* chunks, int sc_size)
{
  int z_vec[t];

  get_plane_vector(z, z_vec);

  for (int x = 0; x < q; x++) {
    for (int y = 0; y < t; y++) {
      int node_xy = q*y+x;
      int node_sw = q*y+z_vec[y];
      // printf("    node_xy: %d\n", node_xy);
      if (erased_chunks.count(node_xy) == 0) {
	      if (z_vec[y] < x) {
          // printf("    get_uncoupled_from_coupled\n");
	        get_uncoupled_from_coupled(chunks, x, y, z, z_vec, sc_size);
	      } else if (z_vec[y] == x) {
          // printf("    memcpy\n");
	          char* uncoupled_chunk = U_buf[node_xy].c_str();
	          char* coupled_chunk = (*chunks)[node_xy].c_str();
            // memcpy(&uncoupled_chunk[z*sc_size], &coupled_chunk[z*sc_size], sc_size);
            memcpy(&uncoupled_chunk[z*sc_size], &coupled_chunk[order_ind[z]*sc_size], sc_size);
        } else {
            if (erased_chunks.count(node_sw) > 0) {
              // printf("    3\n");
              get_uncoupled_from_coupled(chunks, x, y, z, z_vec, sc_size);
            }
        }
      }
    }
  }
  return decode_uncoupled(erased_chunks, z, sc_size);
}
int ErasureCodeMyClay::decode_uncoupled(const set<int>& erased_chunks, int z, int sc_size)
{
  map<int, bufferlist> known_subchunks;
  map<int, bufferlist> all_subchunks;

  for (int i = 0; i < q*t; i++) {
    if (erased_chunks.count(i) == 0) {
      known_subchunks[i].substr_of(U_buf[i], z*sc_size, sc_size);
      all_subchunks[i] = known_subchunks[i];
    } else {
      all_subchunks[i].substr_of(U_buf[i], z*sc_size, sc_size);
    }
    all_subchunks[i].rebuild_aligned_size_and_memory(sc_size, SIMD_ALIGN);
    assert(all_subchunks[i].is_contiguous());
  }

  mds.erasure_code->decode_chunks(erased_chunks, known_subchunks, &all_subchunks);
  return 0;
}


void ErasureCodeMyClay::set_planes_sequential_decoding_order(int* order, set<int>& erasures) {
  int z_vec[t];
  for (int z = 0; z < sub_chunk_no; z++) {
    get_plane_vector(z,z_vec);
    order[z] = 0;
    for (auto i : erasures) {
      if (i % q == z_vec[i / q]) { // x==zy, uncoupled
	order[z] = order[z] + 1;
      }
    }
  }
}

void ErasureCodeMyClay::recover_type1_erasure(map<int, bufferlist>* chunks,
					    int x, int y, int z,
					    int* z_vec, int sc_size)
{
  set<int> erased_chunks;

  int node_xy = y*q+x;
  int node_sw = y*q+z_vec[y];
  int z_sw = z + (x - z_vec[y]) * pow_int(q,t-1-y);

  map<int, bufferlist> known_subchunks;
  map<int, bufferlist> pftsubchunks;
  bufferptr ptr(buffer::create_aligned(sc_size, SIMD_ALIGN));
  ptr.zero();

  int i0 = 0, i1 = 1, i2 = 2, i3 = 3;
  if (z_vec[y] > x) {
    i0 = 1;
    i1 = 0;
    i2 = 3;
    i3 = 2;
  }

  erased_chunks.insert(i0);
  pftsubchunks[i0].substr_of((*chunks)[node_xy], order_ind[z] * sc_size, sc_size);
  known_subchunks[i1].substr_of((*chunks)[node_sw], order_ind[z_sw] * sc_size, sc_size);
  known_subchunks[i2].substr_of(U_buf[node_xy], z * sc_size, sc_size);
  pftsubchunks[i1] = known_subchunks[i1];
  pftsubchunks[i2] = known_subchunks[i2];
  pftsubchunks[i3].push_back(ptr);

  for (int i=0; i<3; i++) {
    pftsubchunks[i].rebuild_aligned_size_and_memory(sc_size, SIMD_ALIGN);
  }

  pft.erasure_code->decode_chunks(erased_chunks, known_subchunks, &pftsubchunks);
}


void ErasureCodeMyClay::get_coupled_from_uncoupled(map<int, bufferlist>* chunks,
						 int x, int y, int z,
						 int* z_vec, int sc_size)
{
  set<int> erased_chunks = {0, 1};

  int node_xy = y*q+x;
  int node_sw = y*q+z_vec[y];
  int z_sw = z + (x - z_vec[y]) * pow_int(q,t-1-y);

  ceph_assert(z_vec[y] < x);
  map<int, bufferlist> uncoupled_subchunks;
  uncoupled_subchunks[2].substr_of(U_buf[node_xy], z * sc_size, sc_size);
  uncoupled_subchunks[3].substr_of(U_buf[node_sw], z_sw * sc_size, sc_size);

  map<int, bufferlist> pftsubchunks;
  pftsubchunks[0].substr_of((*chunks)[node_xy], order_ind[z] * sc_size, sc_size);
  pftsubchunks[1].substr_of((*chunks)[node_sw], order_ind[z_sw] * sc_size, sc_size);
  pftsubchunks[2] = uncoupled_subchunks[2];
  pftsubchunks[3] = uncoupled_subchunks[3];

  for (int i=0; i<3; i++) {
    pftsubchunks[i].rebuild_aligned_size_and_memory(sc_size, SIMD_ALIGN);
  }
  pft.erasure_code->decode_chunks(erased_chunks, uncoupled_subchunks, &pftsubchunks);
}


void ErasureCodeMyClay::get_uncoupled_from_coupled(map<int, bufferlist>* chunks,
						 int x, int y, int z,
						 int* z_vec, int sc_size)
{
  set<int> erased_chunks = {2, 3};

  // (x,y,z) --pair-> (z_vec[y],y,z')
  int node_xy = y*q+x;
  int node_sw = y*q+z_vec[y]; // pair (x,y)
  int z_sw = z + (x - z_vec[y]) * pow_int(q,t-1-y); // pair z

  int i0 = 0, i1 = 1, i2 = 2, i3 = 3;
  if (z_vec[y] > x) {
    i0 = 1;
    i1 = 0;
    i2 = 3;
    i3 = 2;
  }
  map<int, bufferlist> coupled_subchunks;
  // coupled_subchunks[i0].substr_of((*chunks)[node_xy], z * sc_size, sc_size);
  // coupled_subchunks[i1].substr_of((*chunks)[node_sw], z_sw * sc_size, sc_size);
  coupled_subchunks[i0].substr_of((*chunks)[node_xy], order_ind[z] * sc_size, sc_size);
  coupled_subchunks[i1].substr_of((*chunks)[node_sw], order_ind[z_sw] * sc_size, sc_size);

  map<int, bufferlist> pftsubchunks;
  pftsubchunks[0] = coupled_subchunks[0];
  pftsubchunks[1] = coupled_subchunks[1];
  pftsubchunks[i2].substr_of(U_buf[node_xy], z * sc_size, sc_size);
  pftsubchunks[i3].substr_of(U_buf[node_sw], z_sw * sc_size, sc_size);
  for (int i=0; i<3; i++) {
    pftsubchunks[i].rebuild_aligned_size_and_memory(sc_size, SIMD_ALIGN);
  }
  pft.erasure_code->decode_chunks(erased_chunks, coupled_subchunks, &pftsubchunks);
}



int ErasureCodeMyClay::get_max_iscore(set<int>& erased_chunks)
{
  int weight_vec[t];
  int iscore = 0;
  memset(weight_vec, 0, sizeof(int)*t);

  for (auto i : erased_chunks) {
    if (weight_vec[i / q] == 0) {
      weight_vec[i / q] = 1;
      iscore++;
    }
  }
  return iscore;
}


void ErasureCodeMyClay::get_plane_vector(int z, int* z_vec)
{
  for (int i = 0; i < t; i++) {
    z_vec[t-1-i] = z % q;
    z = (z - z_vec[t-1-i]) / q;
  }
}
