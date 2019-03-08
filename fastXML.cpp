#include "fastXML.h"

using namespace std;

char mesg[10000];

void print_string_stdout()
{
	printf("%s",mesg);
	fflush(stdout);
}

void (*mesg_flush) () = print_string_stdout;

void set_mesg_flush(void (*mesg_flush_function) ())
{
	mesg_flush = mesg_flush_function;
}

vector<float> output_precision_at_K(Mat* tst_score_mat,Mat* tst_lbl_mat,int K)
{
	vector<float> precs_vec;

	int num_tst = tst_lbl_mat->num_row;
	int num_lbl = tst_lbl_mat->num_col;

	pairIF** lbl_data = tst_lbl_mat->data;
	pairIF** score_data = tst_score_mat->data;

	float* precs;
	Calloc(precs,float,K);

	bool* lbl_mask;
	Calloc(lbl_mask,bool,num_lbl);

	for(int i=0; i<num_tst; i++)
	{
		pairIF* lbl_vec = lbl_data[i];
		int ctr = 0;
		while(lbl_vec[ctr].first!=-1)
		{
			lbl_mask[lbl_vec[ctr].first] = true;
			ctr++;
		}

		pairIF* score_vec = score_data[i];
		ctr = 0;
		while(score_vec[ctr].first!=-1)
			ctr++;

		sort(score_vec,score_vec+ctr,comp_pairIF_by_second_desc);

		for(int j=0; j<ctr && j<K; j++)
		{
			if(lbl_mask[score_vec[j].first])
			{
				for(int k=j; k<K; k++)
					precs[k]++;
			}
		}

		ctr = 0;
		while(lbl_vec[ctr].first!=-1)
		{
			lbl_mask[lbl_vec[ctr].first] = false;
			ctr++;
		}
	}

	for(int i=0; i<K; i++)
	{
		precs_vec.push_back(precs[i]/(num_tst*(i+1)));
	}

	free(precs);
	free(lbl_mask);

	return precs_vec;
}

void free_mat(Mat* mat)
{
	int num_row = mat->num_row;
	for(int i=0; i<num_row; i++)
		free(mat->data[i]);
	free(mat->data);
	delete mat;
}

struct Node
{
  int num_inst;      // number of instances in the node
  int* inst_index;   // instance ids of instances in the node
  pairIF* w;         // separator hyperplane
  int pos_child;     // positive child index
  int neg_child;     // negative child index 
  bool is_leaf;      // indicates if the node is leaf or internal node
  pairIF* leaf_dist; // stores leaf label probability distribution, maximum 'lbl_per_leaf' label scores are retained
};

struct Tree
{
  int num_nodes;     // number of nodes in the tree
  Node** nodes;
};

void free_tree(Tree* tree)
{
	int num_nodes = tree->num_nodes;
	Node** nodes = tree->nodes;
	for(int i=0; i<num_nodes; i++)
	{
		// freeing node i
		Node* node = nodes[i];
		
		if(node->is_leaf)
			free(node->leaf_dist);
		
		if(!node->is_leaf)
			free(node->w);
		
		free(node->inst_index);
		delete node;
	}
	free(nodes);
	delete tree;
}

FastXML_Param read_param(string model_folder_name,int& num_ft,int& num_lbl)
{
	char param_file_name[NAME_LEN];
	sprintf(param_file_name,"%s/params.txt",model_folder_name.c_str());
	FILE * fin = fopen(param_file_name,"r");

	char str[100];
	float val;
	FastXML_Param fastXML_param;

	while(!feof(fin))
	{
		fscanf(fin,"%s %f",&str,&val);
		if(strcmp(str,"num_ft")==0)
			num_ft = (int)val;
		if(strcmp(str,"num_lbl")==0)
			num_lbl = (int)val;
		if(strcmp(str,"log_loss_coeff")==0)
			fastXML_param.log_loss_coeff = val;
		if(strcmp(str,"lbl_per_leaf")==0)
			fastXML_param.lbl_per_leaf = (int)val;
		if(strcmp(str,"max_leaf")==0)
			fastXML_param.max_leaf = (int)val;
		if(strcmp(str,"start_tree")==0)
			fastXML_param.start_tree = (int)val;
		if(strcmp(str,"num_trees")==0)
			fastXML_param.num_trees = (int)val;
		if(strcmp(str,"bias")==0)
			fastXML_param.bias = val;
	}
	fclose(fin);

	return fastXML_param;
}

void write_param(string model_folder_name,FastXML_Param fastXML_param,int num_ft,int num_lbl)
{
	string param_file_name = model_folder_name + "/params.txt";
	FILE* param_file = fopen(param_file_name.c_str(),"w");

	fprintf(param_file,"%s\t%d\n","num_ft",num_ft);
	fprintf(param_file,"%s\t%d\n","num_lbl",num_lbl);
	fprintf(param_file,"%s\t%f\n","log_loss_coeff",fastXML_param.log_loss_coeff);
	fprintf(param_file,"%s\t%d\n","lbl_per_leaf",fastXML_param.lbl_per_leaf);
	fprintf(param_file,"%s\t%d\n","max_leaf",fastXML_param.max_leaf);
	fprintf(param_file,"%s\t%d\n","start_tree",fastXML_param.start_tree);
	fprintf(param_file,"%s\t%d\n","num_trees",fastXML_param.num_trees);
	fprintf(param_file,"%s\t%f\n","bias",fastXML_param.bias);

	fclose(param_file);
}

class reindex_lbl_mat_class
{
	public:
		bool* lbl_mask;
		int* old_to_new;
		int* new_to_old;

		reindex_lbl_mat_class(Mat* trn_lbl_mat)
		{
			int num_lbl = trn_lbl_mat->num_col;
			Calloc(lbl_mask,bool,num_lbl);
			Calloc(old_to_new,int,num_lbl);
			Calloc(new_to_old,int,num_lbl);
		}

		~reindex_lbl_mat_class()
		{
			free(lbl_mask);
			free(old_to_new);
			free(new_to_old);
		}

};

Mat* reindex_lbl_mat(int num_inst,int* inst_index,Mat* trn_lbl_mat,int*& lbl_map)
{
	// reindexes instance ids and label ids to contiguous ranges
	// example: if only labels 0,4,6,7 are present in the node, 0->0,4->1,6->2,7->3
	// helps to process label matrix efficiently
	// lbl_map stores new->original index mapping

	static reindex_lbl_mat_class local_data(trn_lbl_mat); // large structures are packed into local_data, and malloc'd just once
	bool* lbl_mask = local_data.lbl_mask;
	int* old_to_new = local_data.old_to_new;
	int* new_to_old = local_data.new_to_old;

	int num_lbl = 0;
	pairIF** data = trn_lbl_mat->data;
	pairIF** new_data;
	Malloc(new_data,pairIF*,num_inst);
	for(int i=0; i<num_inst; i++)
	{
		int id = inst_index[i];
		pairIF* lbl_vec = data[id];
		int ctr = 0;
		while(lbl_vec[ctr].first!=-1)
		{
			int lbl_id = lbl_vec[ctr].first;
			if(!lbl_mask[lbl_id])
			{
				new_to_old[num_lbl++] = lbl_id;
				lbl_mask[lbl_id] = true;
			}
			ctr++;
		}
		Malloc(new_data[i],pairIF,ctr+1);
	}

	sort(new_to_old,new_to_old+num_lbl);
	for(int i=0; i<num_lbl; i++)
	{
		old_to_new[new_to_old[i]] = i;
	}

	for(int i=0; i<num_inst; i++)
	{
		int id = inst_index[i];
		pairIF* lbl_vec = data[id];
		int ctr = 0;
		while(lbl_vec[ctr].first!=-1)
		{
			int lbl_id = lbl_vec[ctr].first;
			float lbl_val = lbl_vec[ctr].second;

			new_data[i][ctr].first = old_to_new[lbl_id];
			new_data[i][ctr].second = lbl_val;

			ctr++;
		}
		new_data[i][ctr].first = -1;
	}

	Malloc(lbl_map,int,num_lbl);
	for(int i=0; i<num_lbl; i++)
	{
		lbl_map[i] = new_to_old[i];
		lbl_mask[new_to_old[i]] = false;
		old_to_new[new_to_old[i]] = 0;
		new_to_old[i] = 0;
	}

	Mat* new_lbl_mat = new Mat;
	new_lbl_mat->num_row = num_inst;
	new_lbl_mat->num_col = num_lbl;
	new_lbl_mat->data = new_data;

	return new_lbl_mat;
}

void calc_leaf_prob(Node* node,Mat* trn_lbl_mat,int lbl_per_leaf)
{
	int num_inst = node->num_inst;
	int* inst_index = node->inst_index;

	int* lbl_map;
	Mat* new_lbl_mat = reindex_lbl_mat(num_inst,inst_index,trn_lbl_mat,lbl_map);

	int num_lbl = new_lbl_mat->num_col;
	pairIF** data = new_lbl_mat->data;

	pairIF* leaf_dist;
	Malloc(leaf_dist,pairIF,num_lbl);  // stores leaf distributions. label index required for sorting purpose
	// initializing leaf_dist
	for(int i=0; i<num_lbl; i++)
	{
		leaf_dist[i].first = lbl_map[i];
		leaf_dist[i].second = 0;
	}

	// calculate frequency of each label among instances in the leaf
	for(int i=0; i<num_inst; i++)
	{
		pairIF* lbl_vec = data[i];
		int ctr = 0;
		while(lbl_vec[ctr].first!=-1)
		{
			leaf_dist[lbl_vec[ctr].first].second += lbl_vec[ctr].second;
			ctr++;
		}
	}

	// divide by num_inst to convert into probability scores
	for(int i=0; i<num_lbl; i++)
		leaf_dist[i].second /= num_inst;

	// order labels by decreasing probabilities
	sort(leaf_dist,leaf_dist+num_lbl,comp_pairIF_by_second_desc);

	// retain only top MIN(num_lbl,lbl_per_leaf) labels
	int lpl = MIN(num_lbl,lbl_per_leaf);
	Realloc(leaf_dist,pairIF,lpl+1);
	leaf_dist[lpl].first = -1;	
	
	sort(leaf_dist,leaf_dist+lpl,comp_pairIF_by_first);

	node->leaf_dist = leaf_dist;

	free(lbl_map);
	free_mat(new_lbl_mat);
}

inline pairII get_pos_neg_count(int num_inst,int* pos_or_neg)
{
	int num_pos=0,num_neg=0;

	for(int i=0; i<num_inst; i++)
		if(pos_or_neg[i]==+1)
			num_pos++;
		else
			num_neg++;

	return make_pair(num_pos,num_neg);
}


class reindex_transpose_ft_mat_class
{
	public:
		int* ft_count;
		int* old_to_new;
		int* new_to_old;

		reindex_transpose_ft_mat_class(Mat* trn_ft_mat)
		{
			int num_ft = trn_ft_mat->num_col;
			Calloc(ft_count,int,num_ft);
			Calloc(old_to_new,int,num_ft);
			Calloc(new_to_old,int,num_ft);
		}

		~reindex_transpose_ft_mat_class()
		{
			free(ft_count);
			free(old_to_new);
			free(new_to_old);
		}
};

Mat* reindex_transpose_ft_mat(int num_inst,int* inst_index,Mat* trn_ft_mat,float bias,int*& ft_map)
{
	// reindexes instance ids and feature ids to contiguous ranges
	// example: if only features 0,4,6,7 are present in the node, 0->0,4->1,6->2,7->3
	// helps to process feature matrix efficiently
	// returns transposed feature matrix, as required by optimize_log_loss function (modified from liblinear)
	// ft_map stores new->original index mapping


	static reindex_transpose_ft_mat_class local_data(trn_ft_mat);  // large structures are packed into local_data, and malloc'd just once

	int* ft_count = local_data.ft_count;
	int* old_to_new = local_data.old_to_new;
	int* new_to_old = local_data.new_to_old;

	pairIF** data = trn_ft_mat->data;

	int num_ft = 0;
	for(int i=0; i<num_inst; i++)
	{
		int id = inst_index[i];
		pairIF* ft_vec = data[id];
		int ctr = 0;
		while(ft_vec[ctr].first != -1)
		{
			int ft_ind = ft_vec[ctr].first;
			if(ft_count[ft_ind]==0)
			{
				new_to_old[num_ft++] = ft_ind;
			}
			ft_count[ft_ind]++;
			ctr++;
		}
	}

	sort(new_to_old,new_to_old+num_ft);

	pairIF** trans_data;
	Malloc(trans_data,pairIF*,num_ft+1);
	for(int i=0; i<num_ft; i++)
	{
		Malloc(trans_data[i],pairIF,ft_count[new_to_old[i]]+1);
		old_to_new[new_to_old[i]] = i;
		ft_count[new_to_old[i]] = 0;
	}
	Malloc(trans_data[num_ft],pairIF,num_inst+1);

	for(int i=0; i<num_inst; i++)
	{
		int id = inst_index[i];
		pairIF* ft_vec = data[id];
		int ctr = 0;
		while(ft_vec[ctr].first != -1)
		{
			int ft_id = ft_vec[ctr].first;
			float ft_val = ft_vec[ctr].second;
			trans_data[old_to_new[ft_id]][ft_count[ft_id]].first = i;
			trans_data[old_to_new[ft_id]][ft_count[ft_id]].second = ft_val;
			ft_count[ft_id]++;
			ctr++;
		}
	}

	for(int i=0; i<num_ft; i++)
		trans_data[i][ft_count[new_to_old[i]]].first = -1;
	int i;
	for(i=0; i<num_inst; i++)
	{
		trans_data[num_ft][i].first = i;
		trans_data[num_ft][i].second = bias;
	}
	trans_data[num_ft][i].first = -1;

	Malloc(ft_map,int,num_ft+1);

	for(int i=0; i<num_ft; i++)
	{
		ft_map[i] = new_to_old[i];
		ft_count[new_to_old[i]] = 0;
		old_to_new[new_to_old[i]] = 0;
		new_to_old[i] = 0;
	}
	ft_map[num_ft] = trn_ft_mat->num_col;

	Mat* trans_mat = new Mat;
	trans_mat->num_row = num_ft+1;
	trans_mat->num_col = num_inst;
	trans_mat->data = trans_data;

	return trans_mat;
}

#define GETI(i) (y[i]+1)
typedef signed char schar;
bool optimize_log_loss(int num_inst,int* inst_index,Mat* trn_ft_mat,float log_loss_coeff,float bias,int* pos_or_neg,pairIF*& sparse_w)
{
	// input : num_inst: number of training instances
	//         num_ft:   number of features
	//         ft_mat:   training feature matrix
	//         side:     training label vector
	//         C:        svm hinge-loss coefficient

	// output: w:        svm hyperplane
	//         side:     training label predictions (overwrites the ground truth with predicted labels

	//float frac_pos = 0;  // fraction of the training points that go left and right
	//float frac_neg = 0;  // used for svm balancing purposes

	int* ft_map;
	Mat* trans_ft_mat = reindex_transpose_ft_mat(num_inst,inst_index,trn_ft_mat,bias,ft_map);
	pairIF** trans_data = trans_ft_mat->data;
	int num_ft = trans_ft_mat->num_row;

	pairII num_pos_neg = get_pos_neg_count(num_inst,pos_or_neg);
	float frac_pos = (float)num_pos_neg.first/(num_pos_neg.first+num_pos_neg.second);
	float frac_neg = (float)num_pos_neg.second/(num_pos_neg.first+num_pos_neg.second);
	double Cp = log_loss_coeff/frac_pos;
	double Cn = log_loss_coeff/frac_neg;  // unequal Cp,Cn improves the balancing in some data sets


	double* w;
	Malloc(w,double,num_ft);
	double eps = 0.01;

	int l = trans_ft_mat->num_col;
	int w_size = trans_ft_mat->num_row;
	int j, s, newton_iter=0, iter=0;
	int max_newton_iter = 10;
	int max_iter = 10;
	int max_num_linesearch = 20;
	int active_size;
	int QP_active_size;

	double nu = 1e-12;
	double inner_eps = 1;
	double sigma = 0.01;
	double w_norm, w_norm_new;
	double z, G, H;
	double Gnorm1_init;
	double Gmax_old = INF;
	double Gmax_new, Gnorm1_new;
	double QP_Gmax_old = INF;
	double QP_Gmax_new, QP_Gnorm1_new;
	double delta, negsum_xTd, cond;

	int *index = new int[w_size];
	schar *y = new schar[l];
	double *Hdiag = new double[w_size];
	double *Grad = new double[w_size];
	double *wpd = new double[w_size];
	double *xjneg_sum = new double[w_size];
	double *xTd = new double[l];
	double *exp_wTx = new double[l];
	double *exp_wTx_new = new double[l];
	double *tau = new double[l];
	double *D = new double[l];
	pairIF *x;

	double C[3] = {Cn,0,Cp};
	// Initial w can be set here.
	for(j=0; j<w_size; j++)
		w[j] = 0;

	for(j=0; j<l; j++)
	{
		if(pos_or_neg[j] > 0)
			y[j] = 1;
		else
			y[j] = -1;

		exp_wTx[j] = 0;
	}

	w_norm = 0;
	for(j=0; j<w_size; j++)
	{	  
		w_norm += fabs(w[j]);
		wpd[j] = w[j];
		index[j] = j;
		xjneg_sum[j] = 0;
		x = trans_data[j];
		while(x->first != -1)
		{
			int ind = x->first;
			double val = x->second;
			exp_wTx[ind] += w[j]*val;
			if(y[ind] == -1)
				xjneg_sum[j] += C[GETI(ind)]*val;
			x++;
		}
	}

	for(j=0; j<l; j++)
	{
		exp_wTx[j] = exp(exp_wTx[j]);
		double tau_tmp = 1/(1+exp_wTx[j]);
		tau[j] = C[GETI(j)]*tau_tmp;
		D[j] = C[GETI(j)]*exp_wTx[j]*tau_tmp*tau_tmp;
	}

	while(newton_iter < max_newton_iter)
	{

		Gmax_new = 0;
		Gnorm1_new = 0;
		active_size = w_size;

		for(s=0; s<active_size; s++)
		{
			j = index[s];
			Hdiag[j] = nu;
			Grad[j] = 0;

			double tmp = 0;
			x = trans_data[j];
			while(x->first != -1)
			{
				int ind = x->first;
				Hdiag[j] += x->second*x->second*D[ind];
				tmp += x->second*tau[ind];
				x++;
			}
			Grad[j] = -tmp + xjneg_sum[j];

			double Gp = Grad[j]+1;
			double Gn = Grad[j]-1;
			double violation = 0;
			if(w[j] == 0)
			{
				if(Gp < 0)
					violation = -Gp;
				else if(Gn > 0)
					violation = Gn;
				//outer-level shrinking
				else if(Gp>Gmax_old/l && Gn<-Gmax_old/l)
				{
					active_size--;
					swap(index[s], index[active_size]);
					s--;
					continue;
				}
			}
			else if(w[j] > 0)
				violation = fabs(Gp);
			else
				violation = fabs(Gn);

			Gmax_new = max(Gmax_new, violation);
			Gnorm1_new += violation;
		}

		if(newton_iter == 0)
			Gnorm1_init = Gnorm1_new;

		if(Gnorm1_new <= eps*Gnorm1_init)
			break;

		iter = 0;
		QP_Gmax_old = INF;
		QP_active_size = active_size;

		for(int i=0; i<l; i++)
			xTd[i] = 0;

		// optimize QP over wpd
		while(iter < max_iter)
		{
			QP_Gmax_new = 0;
			QP_Gnorm1_new = 0;

			for(j=0; j<QP_active_size; j++)
			{
				int i = j+rand()%(QP_active_size-j);
				swap(index[i], index[j]);
			}

			for(s=0; s<QP_active_size; s++)
			{
				j = index[s];
				H = Hdiag[j];

				x = trans_data[j];
				G = Grad[j] + (wpd[j]-w[j])*nu;
				while(x->first != -1)
				{
					int ind = x->first;
					G += x->second*D[ind]*xTd[ind];
					x++;
				}

				double Gp = G+1;
				double Gn = G-1;
				double violation = 0;
				if(wpd[j] == 0)
				{
					if(Gp < 0)
						violation = -Gp;
					else if(Gn > 0)
						violation = Gn;
					//inner-level shrinking
					else if(Gp>QP_Gmax_old/l && Gn<-QP_Gmax_old/l)
					{
						QP_active_size--;
						swap(index[s], index[QP_active_size]);
						s--;
						continue;
					}
				}
				else if(wpd[j] > 0)
					violation = fabs(Gp);
				else
					violation = fabs(Gn);

				QP_Gmax_new = max(QP_Gmax_new, violation);
				QP_Gnorm1_new += violation;

				// obtain solution of one-variable problem
				if(Gp < H*wpd[j])
					z = -Gp/H;
				else if(Gn > H*wpd[j])
					z = -Gn/H;
				else
					z = -wpd[j];

				if(fabs(z) < 1.0e-12)
					continue;
				z = min(max(z,-10.0),10.0);

				wpd[j] += z;

				x = trans_data[j];
				while(x->first != -1)
				{
					int ind = x->first;
					xTd[ind] += x->second*z;
					x++;
				}
			}

			iter++;

			if(QP_Gnorm1_new <= inner_eps*Gnorm1_init)
			{
				//inner stopping
				if(QP_active_size == active_size)
					break;
				//active set reactivation
				else
				{
					QP_active_size = active_size;
					QP_Gmax_old = INF;
					continue;
				}
			}

			QP_Gmax_old = QP_Gmax_new;
		}

		delta = 0;
		w_norm_new = 0;
		for(j=0; j<w_size; j++)
		{
			delta += Grad[j]*(wpd[j]-w[j]);
			if(wpd[j] != 0)
				w_norm_new += fabs(wpd[j]);
		}
		delta += (w_norm_new-w_norm);

		negsum_xTd = 0;
		for(int i=0; i<l; i++)
			if(y[i] == -1)
				negsum_xTd += C[GETI(i)]*xTd[i];

		int num_linesearch;
		for(num_linesearch=0; num_linesearch < max_num_linesearch; num_linesearch++)
		{
			cond = w_norm_new - w_norm + negsum_xTd - sigma*delta;

			for(int i=0; i<l; i++)
			{
				double exp_xTd = exp(xTd[i]);
				exp_wTx_new[i] = exp_wTx[i]*exp_xTd;
				cond += C[GETI(i)]*log((1+exp_wTx_new[i])/(exp_xTd+exp_wTx_new[i]));
			}

			if(cond <= 0)
			{
				w_norm = w_norm_new;
				for(j=0; j<w_size; j++)
					w[j] = wpd[j];
				for(int i=0; i<l; i++)
				{
					exp_wTx[i] = exp_wTx_new[i];
					double tau_tmp = 1/(1+exp_wTx[i]);
					tau[i] = C[GETI(i)]*tau_tmp;
					D[i] = C[GETI(i)]*exp_wTx[i]*tau_tmp*tau_tmp;
				}
				break;
			}
			else
			{
				w_norm_new = 0;
				for(j=0; j<w_size; j++)
				{
					wpd[j] = (w[j]+wpd[j])*0.5;
					if(wpd[j] != 0)
						w_norm_new += fabs(wpd[j]);
				}
				delta *= 0.5;
				negsum_xTd *= 0.5;
				for(int i=0; i<l; i++)
					xTd[i] *= 0.5;
			}
		}

		// Recompute some info due to too many line search steps
		if(num_linesearch >= max_num_linesearch)
		{
			for(int i=0; i<l; i++)
				exp_wTx[i] = 0;

			for(int i=0; i<w_size; i++)
			{
				if(w[i]==0) continue;
				x = trans_data[i];
				while(x->first != -1)
				{
					exp_wTx[x->first] += w[i]*x->second;
					x++;
				}
			}

			for(int i=0; i<l; i++)
				exp_wTx[i] = exp(exp_wTx[i]);
		}

		if(iter == 1)
			inner_eps *= 0.25;

		newton_iter++;
		Gmax_old = Gmax_new;
	}

	// calculate objective value

	double v = 0;
	int nnz = 0;
	for(j=0; j<w_size; j++)
		if(w[j] != 0)
		{
			v += fabs(w[j]);
			nnz++;
		}

	for(j=0; j<l; j++)
	{
		if(y[j] == 1)
		{
			v += C[GETI(j)]*log(1+1/exp_wTx[j]);
		}
		else
		{
			v += C[GETI(j)]*log(1+exp_wTx[j]);
		}
	}

	for(int i=0; i<l; i++)
		exp_wTx[i] = 0;

	for(int i=0; i<w_size; i++)
	{
		if(w[i]==0) continue;
		x = trans_data[i];
		while(x->first != -1)
		{
			exp_wTx[x->first] += w[i]*x->second;
			x++;
		}
	}

	for(int i=0; i<l; i++)
	{
		if(exp_wTx[i]>=0)
			pos_or_neg[i] = +1;
		else if(exp_wTx[i]<0)
			pos_or_neg[i] = -1;
		else
		{
			int r = rand()%2;
			if(r==1)
				pos_or_neg[i] = +1;
			else
				pos_or_neg[i] = -1;
		}
	}

	delete [] index;
	delete [] y;
	delete [] Hdiag;
	delete [] Grad;
	delete [] wpd;
	delete [] xjneg_sum;
	delete [] xTd;
	delete [] exp_wTx;
	delete [] exp_wTx_new;
	delete [] tau;
	delete [] D;

	float th = 1e-16;
	Malloc(sparse_w,pairIF,num_ft);
	int ctr = 0;
	for(int i=0; i<num_ft; i++)
	{
		if(fabs(w[i])>th)
		{
			sparse_w[ctr].first = ft_map[i]; // remap from new to original feature index
			sparse_w[ctr].second = -w[i];
			ctr++;
		}
	}
	Realloc(sparse_w,pairIF,ctr+1);
	sparse_w[ctr].first = -1;

	// clean up
	free(w);
	free(ft_map);
	free_mat(trans_ft_mat);

	num_pos_neg = get_pos_neg_count(num_inst,pos_or_neg);
	if(num_pos_neg.first==0 || num_pos_neg.second==0)
	{
		free(pos_or_neg);
		free(sparse_w);
		return false;
	}

	return true;
}

inline void reset_float_vec(float* vec, int num, float reset_val=0)
{
	for(int i=0; i<num; i++)
	{
		vec[i] = reset_val;
	}
}

inline void reset_pairIF_vec(pairIF* vec, int num)
{
	for(int i=0; i<num; i++)
	{
		vec[i].first = i;
		vec[i].second = 0;
	}
}

inline pairIF* create_pairIF_vec(int num)
{
	pairIF* vec;
	Malloc(vec,pairIF,num);
	for(int i=0; i<num; i++)
	{
		vec[i].first = i;
		vec[i].second = 0;
	}
	return vec;
}


class optimize_ndcg_class
{
	public:
		float* wt_vec;
		float* inv_max_dcg;

		optimize_ndcg_class(Mat* trn_lbl_mat)
		{
			int num_trn = trn_lbl_mat->num_row;
			int num_lbl = trn_lbl_mat->num_col;

			Malloc(wt_vec,float,num_lbl);
			for(int i=0; i<num_lbl; i++)
			{
				wt_vec[i] = 1/LOG2(i+2);
			}
			Malloc(inv_max_dcg,float,num_trn);
			pairIF** data = trn_lbl_mat->data;

			for(int i=0; i<num_trn; i++)
			{
				
				vector<pairIF> vec;
				pairIF* lbl_vec = data[i];
				int ctr = 0;
				while(lbl_vec[ctr].first != -1)
				{
					vec.push_back(lbl_vec[ctr]);	
					ctr++;
				}
				sort(vec.begin(),vec.end(),comp_pairIF_by_second_desc);
				
				float val = 0;
				for(int j=0; j<vec.size(); j++)
					val += vec[j].second*wt_vec[j];

				if(val==0)
					inv_max_dcg[i] = 1.0;
				else
					inv_max_dcg[i] = 1.0/val;
			}

		}

		~optimize_ndcg_class()
		{
			free(wt_vec);
			free(inv_max_dcg);
		}
};

bool optimize_ndcg(int num_inst,int* inst_index,Mat* trn_lbl_mat,int* pos_or_neg)
{

	static optimize_ndcg_class local_data(trn_lbl_mat);// large structures are packed into local_data, and malloc'd just once


	float* wt_vec = local_data.wt_vec;    // stores [1/log(2), 1/log(3),..., 1/log(l+1)] nDCG coefficients
	float* inv_max_dcg = local_data.inv_max_dcg;  // stores 1/ideal-DCG values for each training point. nDCG = DCG/ideal-DCG

	int* lbl_map;
	Mat* new_lbl_mat = reindex_lbl_mat(num_inst,inst_index,trn_lbl_mat,lbl_map);
	int num_lbl = new_lbl_mat->num_col;
	pairIF** data = new_lbl_mat->data;

	pairIF* pos_lbl_sum = create_pairIF_vec(num_lbl);
	pairIF* neg_lbl_sum = create_pairIF_vec(num_lbl);
	float* wt_diff_vec;
	Malloc(wt_diff_vec,float,num_lbl);
	float val = -1;

	while(1)
	{
		// get ready for a new iteration
		reset_pairIF_vec(pos_lbl_sum,num_lbl);
		reset_pairIF_vec(neg_lbl_sum,num_lbl);
		reset_float_vec(wt_diff_vec,num_lbl,0);

		// calculate the label frequencies in both positive and negative cluster
		for(int i=0; i<num_inst; i++)
		{
			pairIF* lbl_vec = data[i];

			int ctr = 0;
			while(lbl_vec[ctr].first!=-1)
			{
				if(pos_or_neg[i]==+1)
					pos_lbl_sum[lbl_vec[ctr].first].second += lbl_vec[ctr].second*inv_max_dcg[inst_index[i]];
				else
					neg_lbl_sum[lbl_vec[ctr].first].second += lbl_vec[ctr].second*inv_max_dcg[inst_index[i]];
				ctr++;
			}
		}

		// find the positive and negative cluster rankings and calculate the total gain over all instances
		sort(pos_lbl_sum,pos_lbl_sum+num_lbl,comp_pairIF_by_second_desc);
		sort(neg_lbl_sum,neg_lbl_sum+num_lbl,comp_pairIF_by_second_desc);	     

		float new_val = 0;
		for(int i=0; i<num_lbl; i++)
		{
			new_val += pos_lbl_sum[i].second*wt_vec[i] + neg_lbl_sum[i].second*wt_vec[i];
			wt_diff_vec[pos_lbl_sum[i].first] += wt_vec[i];
			wt_diff_vec[neg_lbl_sum[i].first] -= wt_vec[i];
		}
		new_val /= num_inst;

		// assign each instance to (positive or negative) cluster whose ranking gives it higher gain
		for(int i=0; i<num_inst; i++)
		{
			float gain_diff = 0;
			pairIF* lbl_vec = data[i];
			int ctr = 0;
			while(lbl_vec[ctr].first!=-1)
			{
				gain_diff += lbl_vec[ctr].second*wt_diff_vec[lbl_vec[ctr].first];
				ctr++;
			}
			if(gain_diff>0)
				pos_or_neg[i] = +1;
			else if(gain_diff<0)
				pos_or_neg[i] = -1;
		}

		// if incremental increase in gain is very small, terminate
		if(fabs(new_val-val)<1e-6)
			break;
		else
			val = new_val;

	}

	// clean-up
	free(pos_lbl_sum);
	free(neg_lbl_sum);
	free(wt_diff_vec);
	free(lbl_map);
	free_mat(new_lbl_mat);

	pairII pos_neg_count = get_pos_neg_count(num_inst,pos_or_neg);

	if(pos_neg_count.first==0 || pos_neg_count.second==0)
	{
		free(pos_or_neg);
		return false;
	}

	return true;
}

bool split_node(Node* node,Mat* trn_ft_mat,Mat* trn_lbl_mat,float log_loss_coeff,float bias,int*& pos_or_neg)
{
	int num_inst = node->num_inst;
	int* inst_index = node->inst_index;

	Malloc(pos_or_neg,int,num_inst);

	// allot each instance to positive or negative cluster at random
	for(int i=0; i<num_inst; i++)
	{
		if(rand()%2)
			pos_or_neg[i] = 1;
		else
			pos_or_neg[i] = -1;
	}

	// one run of ndcg optimization
	bool success;
	success = optimize_ndcg(num_inst,inst_index,trn_lbl_mat,pos_or_neg);	
	if(!success)
	{
		return false;
	}

	// one run of log-loss optimization
	success = optimize_log_loss(num_inst,inst_index,trn_ft_mat,log_loss_coeff,bias,pos_or_neg,node->w);
	if(!success)
	{
		return false;
	}

	return true;
}

inline void set_node(Node* node,vector<int> vec,int max_leaf)
{
	int num_inst = vec.size();
	node->num_inst = num_inst;
	Malloc(node->inst_index,int,num_inst);
	
	for(int i=0; i<num_inst; i++)
		node->inst_index[i] = vec[i];

	if(num_inst <= max_leaf)
		node->is_leaf = true;
	else
		node->is_leaf = false;
}

Tree* grow_tree(Mat* trn_ft_mat,Mat* trn_lbl_mat,FastXML_Param fastXML_param)
{
	int lbl_per_leaf = fastXML_param.lbl_per_leaf;
	int max_leaf = fastXML_param.max_leaf;
	float log_loss_coeff = fastXML_param.log_loss_coeff;
	float bias = fastXML_param.bias;

	int max_nodes = 1000;    // number of nodes node_list can hold
	int curr_node = 0;       // node being grown
	int end_node = 0;        // last node discovered

	int num_trn = trn_ft_mat->num_row;

	Node** nodes;
	Malloc(nodes,Node*,max_nodes);

	nodes[0] = new Node;

	vector<int> vec;
	for(int i=0; i<num_trn; i++)
		vec.push_back(i);
	set_node(nodes[0],vec,max_leaf);

	end_node++;

	// grow internal nodes one by one. For leaves, save leaf probability distributions
	while(curr_node < end_node)
	{
		if(curr_node%1000==0)
		{
			// output progress
			sprintf(mesg,"\tnode %d\n",curr_node);
			mesg_flush();
		}

		/// growing node indexed 'curr_node'

		// if number of nodes discovered is close to number of allocated nodes, double the number of allocated nodes (num_nodes)
		if(end_node>max_nodes-10)
		{
			max_nodes = 2*max_nodes;
			Realloc(nodes,Node*,max_nodes);    // preserves the existing entries while allocating more memory
		}

		Node* node = nodes[curr_node];

		if(node->is_leaf)
		{
			/// if leaf, save leaf label distributions      
			calc_leaf_prob(node,trn_lbl_mat,lbl_per_leaf);
			curr_node++;
		}
		else
		{
			/// if internal node, split the node
			int* pos_or_neg;
			bool success = split_node(node,trn_ft_mat,trn_lbl_mat,log_loss_coeff,bias,pos_or_neg);
			if(success)
			{
				vector<int> pos;
				vector<int> neg;
				int num_inst = node->num_inst;
				int* inst_index = node->inst_index;
				for(int i=0; i<num_inst; i++)
					if(pos_or_neg[i]==+1)
						pos.push_back(inst_index[i]);
					else
						neg.push_back(inst_index[i]);

				nodes[end_node] = new Node;
				set_node(nodes[end_node],pos,max_leaf);
				node->pos_child = end_node;
				end_node++;

				nodes[end_node] = new Node;
				set_node(nodes[end_node],neg,max_leaf);
				node->neg_child = end_node;
				end_node++;

				free(pos_or_neg);
				curr_node++;
			}
			else
			{
				// if node cannot be split, make it a leaf and process it in next iteration
				node->is_leaf = true;
			}
		}
	}

	Realloc(nodes,Node*,end_node);
	Tree* tree = new Tree;
	tree->nodes = nodes;
	tree->num_nodes = end_node;
	return tree;	
}

float calc_tree_balance(Tree* tree)
{
	int num_nodes = tree->num_nodes;
	int * depths;
	Malloc(depths,int,num_nodes);
	Node** nodes = tree->nodes;
	depths[0] = 0;
	float tree_balance = 0;
	int num_leaves = 0;
	int num_inst = 0;

	for(int i=0; i<num_nodes; i++)
	{
		if(nodes[i]->is_leaf)
		{
			tree_balance += (nodes[i]->num_inst)*depths[i];
			num_leaves++;
			num_inst += (nodes[i]->num_inst);
		}
		else
		{
			depths[nodes[i]->pos_child] = depths[i]+1;
			depths[nodes[i]->neg_child] = depths[i]+1;
		}
	}
	tree_balance /= num_inst;
	tree_balance /= LOG2(num_leaves);

	free(depths);

	return tree_balance;
}

void write_tree(Tree* tree,string model_folder_name,int tree_no)
{
	char tree_file_name[NAME_LEN];
	sprintf(tree_file_name,"%s/tree.%d.txt",model_folder_name.c_str(),tree_no);
	FILE * fout = fopen(tree_file_name,"w");
	fprintf(fout,"%d\n",tree->num_nodes);

	for(int i=0; i<tree->num_nodes; i++)
	{
		Node* node = tree->nodes[i];
		bool is_leaf = node->is_leaf;

		fprintf(fout,"%d\n",is_leaf?1:0);

		if(is_leaf)
		{
			int ctr = 0;
			pairIF* lbl_dist = node->leaf_dist;
			while(lbl_dist[ctr].first!=-1)
				ctr++;

			fprintf(fout,"%d",ctr);
			for(int j=0; j<ctr; j++)
			{
				fprintf(fout," %d:%f",lbl_dist[j].first,lbl_dist[j].second);
			}
			fprintf(fout,"\n");
		}
		if(!is_leaf)
		{
			pairIF* w = node->w;
			int size = 0;
			while(w[size].first!=-1)
			{
				size++;
			}
			fprintf(fout,"%d",size);
			for(int j=0; j<size; j++)
			{
				fprintf(fout," %d:%f",w[j].first,w[j].second);
			}
			fprintf(fout,"\n");
		}
		if(!is_leaf)
		{
			int pos_child = node->pos_child;
			int neg_child = node->neg_child;
			fprintf(fout,"%d %d\n",pos_child,neg_child);
		}

	}

	fclose(fout);
}

inline void calc_mean_std(vector<float> values,float& mean_value,float& std_value)
{
	mean_value = std_value = 0;
	int size = values.size();
	for(int i=0; i<size; i++)
	{
		mean_value += values[i];
		std_value += SQ(values[i]);
	}
	mean_value /= size;
	std_value = sqrt(std_value/size - SQ(mean_value));
}

void fastXML_train(Mat* trn_ft_mat,Mat* trn_lbl_mat,FastXML_Param fastXML_param,string model_folder_name,float& train_time,float& train_balance_mean,float& train_balance_std)
{
	int ctr;
	clock_t tic,toc;
	float t_time = 0;

	int num_ft = trn_ft_mat->num_col;
	int num_lbl = trn_lbl_mat->num_col;
	write_param(model_folder_name,fastXML_param,num_ft,num_lbl);

	vector<float> tree_balances;
	int start_tree = fastXML_param.start_tree;
	int num_trees = fastXML_param.num_trees;
	for(int i=start_tree; i<start_tree+num_trees; i++)
	{
		TIC
		srand(i+1);    // ensures each tree is grown differently
		Tree* tree = grow_tree(trn_ft_mat,trn_lbl_mat,fastXML_param);
		TOC
		float tree_balance = calc_tree_balance(tree);
		tree_balances.push_back(tree_balance);
		write_tree(tree,model_folder_name,i);
		TIC
		free_tree(tree);
		sprintf(mesg,"Tree %d grown\n",i);
		mesg_flush();
		TOC
	}
	calc_mean_std(tree_balances,train_balance_mean,train_balance_std);

	train_time = t_time;
}

Tree* read_tree(string model_folder_name, int tree_no, float& model_size)
{
	model_size = 0;

	char tree_file_name[NAME_LEN];
	sprintf(tree_file_name,"%s/tree.%d.txt",model_folder_name.c_str(),tree_no);
	FILE * fin = fopen(tree_file_name,"r");

	Tree* tree = new Tree;
	model_size += sizeof(tree);

	fscanf(fin,"%d",&(tree->num_nodes));
	Malloc(tree->nodes,Node*,tree->num_nodes);
	model_size += sizeof(Node*)*(tree->num_nodes);
	int num_nodes = tree->num_nodes;
	Node** nodes = tree->nodes;

	for(int j=0; j<num_nodes; j++)
	{
		nodes[j] = new Node;
		model_size += sizeof(Node);

		Node* node = nodes[j];

		int is_leaf;
		fscanf(fin,"%d",&is_leaf);
		node->is_leaf = is_leaf>0;

		if(is_leaf)
		{
			int num_leaf_lbl;
			fscanf(fin,"%d",&num_leaf_lbl);
			pairIF * leaf_dist;
			Malloc(leaf_dist,pairIF,num_leaf_lbl+1);
			model_size += sizeof(pairIF)*(num_leaf_lbl+1);
			node->is_leaf = true;

			for(int k=0; k<num_leaf_lbl; k++)
			{
				int ind;
				char c;
				float val;
				fscanf(fin,"%d %c %f",&ind,&c,&val);
				leaf_dist[k].first = ind;
				leaf_dist[k].second = val;
			}
			leaf_dist[num_leaf_lbl].first = -1;
			node->leaf_dist = leaf_dist;
		}
		if(!is_leaf)
		{
			int w_size;
			fscanf(fin,"%d",&w_size);
			pairIF* w;
			Malloc(w,pairIF,w_size+1);
			model_size += sizeof(pairIF)*(w_size+1);
			for(int k=0; k<w_size; k++)
			{
				int ind;
				char c;
				float val;
				fscanf(fin,"%d %c %f",&ind,&c,&val);
				w[k].first = ind;
				w[k].second = val;
			}
			w[w_size].first = -1;
			node->w = w;
		}
		if(!is_leaf)
		{
			int pos_child, neg_child;
			fscanf(fin,"%d %d",&pos_child,&neg_child);
			if(pos_child!=-1 && neg_child!=-1)
			{
				node->pos_child = pos_child;
				node->neg_child = neg_child;
			}
		}
	}

	fclose(fin);

	return tree;
}

class test_leaf_class
{
	public:
		int* score_count;

		test_leaf_class(Mat* tst_score_mat)
		{
			int num_tst = tst_score_mat->num_row;
			Calloc(score_count,int,num_tst);
		}

		~test_leaf_class()
		{
			free(score_count);
		}
};
   
void test_leaf(Node* node,Mat* tst_score_mat)
{
	static test_leaf_class local_data(tst_score_mat);// large structures are packed into local_data, and malloc'd just once


	int* score_count = local_data.score_count;
	pairIF* leaf_dist = node->leaf_dist;
	int num_inst= node->num_inst;
	int* inst_index = node->inst_index;
	pairIF** data = tst_score_mat->data;


	for(int i=0; i<num_inst; i++)
	{
		int id = inst_index[i];
		int ctr = 0;
		while(leaf_dist[ctr].first!=-1)
		{
			data[id][score_count[id]++] = leaf_dist[ctr];
			ctr++;
		}
		data[id][score_count[id]].first = -1;
	}
}

class test_internal_class
{
	public:
		float* ft_val;

		test_internal_class(int num_ft)
		{
			Calloc(ft_val,float,num_ft);
		}

		~test_internal_class()
		{
			free(ft_val);
		}

};

void test_internal(Node* node,Node* pos_child,Node* neg_child,Mat* tst_ft_mat,float bias)
{
	int num_tst = tst_ft_mat->num_row;
	int num_ft = tst_ft_mat->num_col;
	static test_internal_class local_data(num_ft);// large structures are packed into local_data, and malloc'd just once


	int num_inst = node->num_inst;
	int* inst_index = node->inst_index;

	float* ft_val = local_data.ft_val;
	pairIF* w = node->w;
	float bias_coeff;
	int ctr = 0;
	while(w[ctr].first != -1)
	{
		if(w[ctr].first == num_ft)
			bias_coeff = w[ctr].second;
		else
			ft_val[w[ctr].first] = w[ctr].second;
		ctr++;
	}

	pairIF** data = tst_ft_mat->data;
	int* pos_or_neg;
	Malloc(pos_or_neg,int,num_inst);
	int num_pos=0,num_neg=0;
	for(int i=0; i<num_inst; i++)
	{
		pairIF* ft_vec = data[inst_index[i]];
		int ctr = 0;
		float value = 0;    // holds w'*x

		while(ft_vec[ctr].first != -1)
		{
			value += ft_val[ft_vec[ctr].first]*ft_vec[ctr].second;
			ctr++;
		}
		value = value+bias_coeff*bias;

		if(value<=0) // test point belongs to positive side of the hyperplane
		{
			pos_or_neg[i] = +1;
			num_pos++;
		}
		else if(value>0) // test point belongs to negative side of the hyperplane
		{
			pos_or_neg[i] = -1;
			num_neg++;
		}
	}

	ctr = 0;
	while(w[ctr].first != -1)
	{
		if(w[ctr].first != num_ft)
		{	
			ft_val[w[ctr].first] = 0;
		}
		ctr++;
	}

	int* pos_index;
	if(num_pos>0)
		Malloc(pos_index,int,num_pos);
	else
		pos_index = NULL;

	int* neg_index;
	if(num_neg>0)
		Malloc(neg_index,int,num_neg);
	else
		neg_index = NULL;

	int pctr=0,nctr=0;
	for(int i=0; i<num_inst; i++)
	{
		if(pos_or_neg[i]==+1)
			pos_index[pctr++] = inst_index[i];
		else
			neg_index[nctr++] = inst_index[i];
	}
	pos_child->num_inst = num_pos;
	pos_child->inst_index = pos_index;
	neg_child->num_inst = num_neg;
	neg_child->inst_index = neg_index;

	free(pos_or_neg);

}

void test_tree(Tree* tree,Mat* tst_ft_mat,Mat*& tst_score_mat,FastXML_Param fastXML_param,int num_lbl)
{
	int num_tst = tst_ft_mat->num_row;
	int num_ft = tst_ft_mat->num_col;
	pairIF** data = tst_ft_mat->data;

	int num_trees = fastXML_param.num_trees;
	int lbl_per_leaf = fastXML_param.lbl_per_leaf;
	float bias = fastXML_param.bias;

	if(tst_score_mat==NULL)  // initialize tst_score_mat once
	{
		tst_score_mat = new Mat;
		Malloc(tst_score_mat->data,pairIF*,num_tst);
		for(int i=0; i<num_tst; i++)
		{
			Malloc(tst_score_mat->data[i],pairIF,lbl_per_leaf*num_trees+1);
			tst_score_mat->data[i][0].first = -1;
		}
		tst_score_mat->num_row = num_tst;
		tst_score_mat->num_col = num_lbl;
	}
	
	int num_nodes = tree->num_nodes;
	Node** nodes = tree->nodes;

	nodes[0]->num_inst = num_tst;
	Malloc(nodes[0]->inst_index,int,num_tst);
	for(int i=0; i<num_tst; i++)
		nodes[0]->inst_index[i] = i;

	for(int i=0; i<num_nodes; i++)
	{
		Node* node = nodes[i];
		if(node->is_leaf)
		{
			test_leaf(node,tst_score_mat);
		}
		else
		{
			test_internal(node,nodes[node->pos_child],nodes[node->neg_child],tst_ft_mat,bias);
		}
	}

}

void fastXML_test(Mat* tst_ft_mat,string model_folder_name,Mat*& tst_score_mat,float& test_time,float& model_size,float& test_balance_mean,float& test_balance_std)
{
	clock_t tic,toc;
	float t_time;
	
	TIC
	model_size = 0;
	test_time = 0;
	float tree_model_size;
	vector<float> tree_balances;

	int num_ft;
	int num_lbl;
	FastXML_Param fastXML_param = read_param(model_folder_name,num_ft,num_lbl);
	int num_tst = tst_ft_mat->num_row;

	int lbl_per_leaf = fastXML_param.lbl_per_leaf;
	int start_tree = fastXML_param.start_tree;
	int num_trees = fastXML_param.num_trees;
	tst_score_mat = NULL;
	TOC

	// do prediction over trees, one at a time
	for(int i=start_tree; i<start_tree+num_trees; i++)
	{
		Tree* tree = read_tree(model_folder_name,i,tree_model_size);

		model_size += tree_model_size;
		TIC
		test_tree(tree,tst_ft_mat,tst_score_mat,fastXML_param,num_lbl);
		TOC

		float tree_balance = calc_tree_balance(tree);

		tree_balances.push_back(tree_balance);
		TIC
		free_tree(tree);
		TOC

		sprintf(mesg,"Tree %d tested\n",i);
		mesg_flush();
	}	
	
	pairIF** score_data = tst_score_mat->data;

	// tst_score_mat contains label scores for each test instance
	// score predictions from different trees,for a given instance, are initially stored consecutively in an array
	// here, the multiple tree predictions are merged, and stored back into tst_score_mat
	for(int i=0; i<num_tst; i++)
	{
		pairIF* score_vec = score_data[i];
		int ctr = 0;
		while(score_vec[ctr].first!=-1)
			ctr++;

		sort(score_vec,score_vec+ctr,comp_pairIF_by_first);

		ctr = 0;
		int curr_id = -1;
		float curr_val = 0;
		int ctr1 = 0;
		while(score_vec[ctr].first!=-1)
		{
			int id = score_vec[ctr].first;
			float val = score_vec[ctr].second;
			
			if(id != curr_id)
			{
				if(ctr!=0)
				{
					score_vec[ctr1].first = curr_id;
					score_vec[ctr1++].second = curr_val/num_trees;
				}
				curr_id = id;
				curr_val = val;
			}
			else
			{
				curr_val += val;
			}

			ctr++;
		}
		score_vec[ctr1].first = curr_id;
		score_vec[ctr1++].second = curr_val/num_trees;

		Realloc(score_data[i],pairIF,ctr1+1);
		score_data[i][ctr1].first = -1;
	}

	calc_mean_std(tree_balances,test_balance_mean,test_balance_std);  
	test_time = t_time;
}

