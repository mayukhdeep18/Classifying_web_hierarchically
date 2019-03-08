#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "fastXML.h"

using namespace std;

int K;


void exit_with_help()
{
	printf(
	"Usage: predict [options] predict_file [model_folder] [score_file_name]\n"
	"predict_file expected in sparse libsvm format\n"
	"model_folder (default: 'model')\n"
	"score_file_name (default: 'scores.txt)\n"
	"options:\n"
	"-p K: output precisions at top 1..K (default K=5)\n"
	);
	exit(1);
}

void parse_command_line(int argc,char** argv,string& predict_file_name,string& model_folder_name,string& score_file_name)
{
	K = 5;

	int i;
	// parse options
	for(i=1;i<argc;i++)
	{
		if(argv[i][0] != '-') break;
		switch(argv[i][1])
		{
			case 'p':
				K = atoi(argv[i+1]);
				i++;
				break;

			default:
				fprintf(stderr,"unknown option: -%c\n", argv[i][1]);
				exit_with_help();
				break;
		}
	}

	// determine filenames
	if(i>=argc)
		exit_with_help();

	predict_file_name = string(argv[i]);

	i++;

	if(i<argc)
		model_folder_name = string(argv[i]);
	else
	{
		model_folder_name = "model";
	}

	i++;

	if(i==argc-1)
	{
		score_file_name = string(argv[i]);
	}
	else
	{
		score_file_name = "scores.txt";
	}
}

int MAX_LEN;
char* line = NULL;
char* read_line(FILE *input)
{
	// reads next line from 'input'. Exits with message on failure

	int len;

	fgets(line,MAX_LEN,input);
	if(ferror(input))
	{
		char mesg[100];
		sprintf(mesg,"error while reading training data file\n");
		printf("%s\n",mesg);
		exit(1);
	}
	if(feof(input))
		return NULL;

	while(strrchr(line,'\n') == NULL)
	{
		MAX_LEN *= 2;
		Realloc(line,char,MAX_LEN);
		len = (int) strlen(line);
		if(fgets(line+len,MAX_LEN-len,input) == NULL)
			break;
	}

	char * cptr = strrchr(line,'\n');
	*cptr = '\0';

	return line;
}

void parse_line_lbls_fts(vector<int> & labels, vector<pairIF> & features)
{
	int ctr;
	char * tok;
	int id;
	float val;

	char * lbls = line;
	char * lbl_delim = strchr(lbls,' ');
	*lbl_delim = '\0';
	tok = strtok(lbl_delim+1,": ");  

	while(tok)
	{
		id = strtol(tok,NULL,10);
		tok = strtok(NULL,": ");
		val = strtod(tok,NULL);
		features.push_back(make_pair(id,val));
		tok = strtok(NULL,": ");
	}

	tok = strtok(lbls,",");
	while(tok != NULL)
	{
		id = strtol(tok,NULL,10);
		labels.push_back(id);
		tok = strtok(NULL,",");
	}

	return;
}

void read_file(string data_file_name,Mat*& ft_mat,Mat*& lbl_mat)
{
	int ctr;
	MAX_LEN = 1000;
	Malloc(line,char,MAX_LEN);
	vector<pairIF> features;
	vector<int> labels;

	int num_inst=0,num_ft=0,num_lbl=0;

	FILE * data_file = fopen(data_file_name.c_str(),"r");

	vector<pairIF> fts;
	vector<int> lbls;

	pairIF** ft_data;
	pairIF** lbl_data;
	int MAX_INST = 1000;
	Malloc(ft_data,pairIF*,MAX_INST);
	Malloc(lbl_data,pairIF*,MAX_INST);

	ctr = 0;
	while(true)
	{
		fts.clear();
		lbls.clear();
		read_line(data_file);
		if(feof(data_file))
			break;

		parse_line_lbls_fts(lbls,fts);

		Malloc(ft_data[num_inst],pairIF,fts.size()+1);
		for(int i=0; i<fts.size(); i++)
		{
			ft_data[num_inst][i].first = fts[i].first;
			ft_data[num_inst][i].second = fts[i].second;
			if(fts[i].first>=num_ft)
				num_ft = fts[i].first+1;
		}
		ft_data[num_inst][fts.size()].first = -1;

		Malloc(lbl_data[num_inst],pairIF,lbls.size()+1);
		for(int i=0; i<lbls.size(); i++)
		{
			lbl_data[num_inst][i].first = lbls[i];
			lbl_data[num_inst][i].second = 1.0;
			if(lbls[i]>=num_lbl)
				num_lbl = lbls[i]+1;
		}
		lbl_data[num_inst][lbls.size()].first = -1;

		ctr++;
		if(num_inst+10 >= MAX_INST)
		{
			MAX_INST *= 2;
			Realloc(ft_data,pairIF*,MAX_INST);
			Realloc(lbl_data,pairIF*,MAX_INST);
		}

		num_inst++;
	}
	Realloc(ft_data,pairIF*,num_inst);
	Realloc(lbl_data,pairIF*,num_inst);

	ft_mat = new Mat;
	ft_mat->num_row = num_inst;
	ft_mat->num_col = num_ft;
	ft_mat->data = ft_data;

	lbl_mat = new Mat;
	lbl_mat->num_row = num_inst;
	lbl_mat->num_col = num_lbl;
	lbl_mat->data = lbl_data;

	free(line);
}

void write_score_mat(Mat* tst_score_mat,string score_file_name)
{
	FILE* fout = fopen(score_file_name.c_str(),"w");

	int num_tst = tst_score_mat->num_row;
	pairIF** data = tst_score_mat->data;
	for(int i=0; i<num_tst; i++)
	{
		pairIF* score_vec = data[i];
		int ctr = 0;
		while(score_vec[ctr].first!=-1)
		{
			int id = score_vec[ctr].first;
			float val = score_vec[ctr].second;
			if(ctr==0)
			{
				fprintf(fout,"%d:%.2f",id,val);
			}
			else
			{
				fprintf(fout," %d:%.2f",id,val);
			}
			ctr++;
		}
		fprintf(fout,"\n");
	}

	fclose(fout);
}

int main(int argc,char** argv)
{
	string predict_file_name,model_folder_name,score_file_name;
	parse_command_line(argc,argv,predict_file_name,model_folder_name,score_file_name);

	Mat* tst_ft_mat;
	Mat* tst_lbl_mat;
	Mat* tst_score_mat;
	read_file(predict_file_name,tst_ft_mat,tst_lbl_mat);

	int num_ft,num_lbl;
	FastXML_Param fastXML_param = read_param(model_folder_name,num_ft,num_lbl);
	tst_ft_mat->num_col = num_ft;
	tst_lbl_mat->num_col = num_lbl;

	float test_time,test_balance_mean,test_balance_std,model_size;
	fastXML_test(tst_ft_mat,model_folder_name,tst_score_mat,test_time,model_size,test_balance_mean,test_balance_std);

	write_score_mat(tst_score_mat,score_file_name);

	vector<float> precs_vec = output_precision_at_K(tst_score_mat,tst_lbl_mat,K);
	for(int i=0; i<precs_vec.size(); i++)
	{
		printf("p%d: %f\n",i+1,precs_vec[i]);
	}
	
	free_mat(tst_ft_mat);
	free_mat(tst_lbl_mat);
	free_mat(tst_score_mat);

	printf("predict time: %f s\n",test_time);
	printf("tree predict balance: %f +/- %f\n",test_balance_mean,test_balance_std);
	printf("model size: %f MB\n",model_size/1000000.0);

	return 0;
}
