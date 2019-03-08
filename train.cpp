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

char c;

void exit_with_help()
{
	printf(
	"Usage: train [options] train_file [model_folder]\n"
	"train_file expected in sparse libsvm format\n"
	"model_folder (default: 'model')\n"
	"options:\n"
	"-s start_tree : starting index for the trees (default 0)\n"
	"-t num_trees : number of trees to grow (default 50)\n"
	"-c cost : co-efficient of log loss (default 1.0)\n"
	"-m max_leaf : maximum training instances a leaf can hold, nodes larger than this get split (default 10)\n"
	"-l lbl_per_leaf : maximum number of label scores to retain in a leaf (default 10)\n"
	"-b bias : feature vector x becomes [x; bias] (default 1.0)\n"
	);
	exit(1);
}

FastXML_Param parse_command_line(int argc,char** argv,string& train_file_name,string& model_folder_name)
{
	FastXML_Param param;
	// default values
	param.log_loss_coeff = 1.0;
	param.lbl_per_leaf = 10;
	param.max_leaf = 10;
	param.start_tree = 0;
	param.num_trees = 50;
	param.bias = 1.0;

	int i;
	// parse options
	for(i=1;i<argc;i++)
	{
		if(argv[i][0] != '-') break;
		if(++i>=argc)
			exit_with_help();
		switch(argv[i-1][1])
		{
			case 's':
				param.start_tree = atoi(argv[i]);
				break;

			case 't':
				param.num_trees = atoi(argv[i]);
				break;

			case 'c':
				param.log_loss_coeff = atof(argv[i]);
				break;

			case 'm':
				param.max_leaf = atoi(argv[i]);
				break;

			case 'l':
				param.lbl_per_leaf = atoi(argv[i]);
				break;

			case 'b':
				param.bias = atof(argv[i]);
				break;

			default:
				fprintf(stderr,"unknown option: -%c\n", argv[i-1][1]);
				exit_with_help();
				break;
		}
	}

	// determine filenames
	if(i>=argc)
		exit_with_help();

	train_file_name = string(argv[i]);
	i++;

	if(i<argc)
		model_folder_name = string(argv[i]);
	else
	{
		model_folder_name = "model";
	}

	return param;
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

int main(int argc,char** argv)
{
	string train_file_name,model_folder_name;
	FastXML_Param fastXML_param = parse_command_line(argc,argv,train_file_name,model_folder_name);

	Mat* trn_ft_mat;
	Mat* trn_lbl_mat;
	read_file(train_file_name,trn_ft_mat,trn_lbl_mat);

	system(("mkdir "+model_folder_name).c_str());

	float train_time,train_balance_mean,train_balance_std;
	fastXML_train(trn_ft_mat,trn_lbl_mat,fastXML_param,model_folder_name,train_time,train_balance_mean,train_balance_std);

	free_mat(trn_ft_mat);
	free_mat(trn_lbl_mat);

	printf("training time: %f s\n",train_time);
	printf("tree balance over training data: %f +/- %f\n",train_balance_mean,train_balance_std);

	return 0;
}
