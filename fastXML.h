#ifndef FASTXML_H
#define FASTXML_H

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <cfloat>
#include <ctime>

using namespace std;

#define DEBUG printf("%d\n",__LINE__);fflush(stdout);
#define Malloc(ptr,type,n) ptr = (type*)malloc((n)*sizeof(type))
#define Realloc(ptr,type,n) ptr = (type*)realloc((ptr),(n)*sizeof(type))
#define Calloc(ptr,type,n) ptr = (type*)calloc((n),sizeof(type))

#define pairII pair<int,int>
#define pairIF pair<int,float>
#define NAME_LEN 1000
#define SQ(x) ((x)*(x))
#define LOG2(x) (log((float)x)/log((float)2.0))
#define INF FLT_MAX
#define NEG_INF FLT_MIN
#define EPS 1e-6
#define TIC {tic = clock();}
#define TOC {toc = clock();t_time+=(float)(toc-tic)/CLOCKS_PER_SEC;}

extern char mesg[10000];  // stores the messages to be flushed to output stream (either to standard console or to matlab console)

extern void (*mesg_flush) ();

template <class T> static inline void SWAP(T& x, T& y) { T t=x; x=y; y=t; }
template <class T> static inline T MIN(T x,T y) { return (x<y)?x:y; }
template <class T> static inline T MAX(T x,T y) { return (x>y)?x:y; }

inline bool comp_pairIF_by_second_desc(pairIF a,pairIF b)
{
	return a.second>b.second;
}

inline bool comp_pairIF_by_first(pairIF a,pairIF b)
{
	return a.first<b.first;
}

struct Mat // a sparse matrix format
{
	int num_row;
	int num_col;
	pairIF** data;
};

void free_mat(Mat* mat);

// store parameters for the algorithm
struct FastXML_Param
{
  float log_loss_coeff;   // cost factor for the logistic loss term
  int max_leaf;      // Maximum instances per leaf node. Nodes containing less than 'n' instances will become leaf nodes. Default 50
  int lbl_per_leaf;          // Maximum labels to retain in leaf node. Probability scores will be saved for only top 'l' most frequent labels. Default 5
  int start_tree;           // Starting index for the trees. Default 0
  int num_trees;            // Number of trees to be grown. Default 50
  float bias;              // Bias term. if bias >= 0, feature vector x becomes [x; bias]; if < 0, no bias term added. Default 1
};

FastXML_Param read_param(string model_folder_name,int& num_ft,int& num_lbl);

void write_param(string model_folder_name,FastXML_Param fastXML_param,int num_ft,int num_lbl);

void fastXML_train(Mat* trn_ft_mat,Mat* trn_lbl_mat,FastXML_Param fastXML_param,string model_folder_name,float& train_time,float& train_balance_mean,float& train_balance_std);

void fastXML_test(Mat* tst_ft_mat,string model_folder_name,Mat*& tst_score_mat,float& test_time,float& model_size,float& test_balance_mean,float& test_balance_std);

vector<float> output_precision_at_K(Mat* tst_score_mat,Mat* tst_lbl_mat,int K);

void set_mesg_flush(void (*mesg_flush_function) ());

#endif
