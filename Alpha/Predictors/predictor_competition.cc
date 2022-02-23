/* Author: Mark Faust;   
 * Description: This file defines the two required functions for the branch predictor.
 * Modified by: Kamali, Michael, and Ram
 * ECE 586, Winter 2020
 * Portland State University
 * Competition Predictor
*/

#include "predictor.h"

////////////////////////Tournament Predictor //////////////////////////////////////
/*
#define LHTADDRMASK 0x3FF // Selects bits 0-9
#define GPTADDRMASK 0xFFF // Selects bits 0-12
#define LHTHEIGHT 1024
#define LHTWIDTH 10
#define LPTHEIGHT 1024
#define LPTWIDTH 3
#define GPTHEIGHT 4096
#define GPTWIDTH 2
#define CPHEIGHT 4096
#define CPWIDTH 2
#define LHTVALMASK 0x3FF    ///local history table's 10 bits
#define PATHHISTVAL 0xFFF
#define SETLSB 0x1
#define CLRLSB 0xFFE

// Debug
//#define BS_VERBOSE
#define PGL 1
#define PGP 2
#define PLP 3
#define PLH 4
#define TAKEN 5
#define ULP 6
#define ULH 7
#define UGP 8
#define UGL 9
#define LINE 10
#define HEAD 11
#define NEWLINE 12
#define BINDEX	13
#define	PATHH	14

// Data Structures
static unsigned int local_history_table[LHTHEIGHT] = {0};
static unsigned int local_prediction_table[LPTHEIGHT] = {0};
static unsigned int global_prediction_table[GPTHEIGHT] = {0};
static unsigned int choice_prediction_table[CPHEIGHT] = {0};
*/
/* Variables	*/
/*
static unsigned int local_history_table_val = 0;
static unsigned int local_prediction_table_val = 0;
static unsigned int global_prediction_table_val = 0;
static unsigned int choice_prediction_table_val = 0;
static unsigned int b_index = 0;
static unsigned int path_history = 0;
long unsigned int t = 0;
*/
/* Function Prototypes	*/
/*
void debug(unsigned int val, int tag);
void IndexFromPC(const branch_record_c* br);
void LocalPrediction(void);
void GlobalPrediction(void);
void ChoicePrediction(void);
bool Multiplexer(const branch_record_c* br);

void LocalHistoryUpdate(bool taken);
void LocalPredictionUpdate(bool taken);
void GlobalPredictionUpdate(bool taken);
void PredictionUpdate(bool taken);
void PathHistoryUpdate(bool taken);
*/

/*Variables- gshare*/

#define BHMASK 0x3FF	//10 bits for branch history
#define BAMASK 0x3FF	//10 bits for branch Address
#define XORMASK 0x3FF	//10 bits for XOR Address
#define PREDICT_SIZE 1024	//Prediction table of size 1024
#define TAGMASK 0xFFFFF		//20 bits for TAG Address
#define SETLSB 0x1
#define PATHHISTVAL 0xFFF

unsigned int branch_history_g = 0;
unsigned int xor_output_g = 0;
unsigned int path_history = 0;
unsigned int branch_address_g = 0;
unsigned int predict_output = 0;
unsigned int predict_output_val = 0;
unsigned int predict_tag_g = 0;
unsigned int predict_set_g = 0;
/*Data Structures- gshare*/
 unsigned int predict_table_g[PREDICT_SIZE];
 unsigned int predict_table_set_g[PREDICT_SIZE];




/////////////////////////////////Gshare Predictor//////////////////////////////////
/*function Prototypes- gshare*/
void get_global_branch_history_g();
void get_branch_address(const branch_record_c*);
void get_xor_operation();
unsigned int get_predict(const branch_record_c*);
void update_global_branch_history(bool);
void update_predict(bool);



bool PREDICTOR::get_prediction(const branch_record_c* br  , const op_state_c* os ){
	//Gshare

	//Get Global Branch History values
	get_global_branch_history_g();

	//Get Branch Address
	get_branch_address(br);


	//XOR Operation of Global and Branch
	get_xor_operation();

	//Get Prediction table values
	return get_predict(br);

}


void get_global_branch_history_g(void)
{
	branch_history_g = path_history & BHMASK;
}

void get_branch_address(const branch_record_c* br)
{
	branch_address_g = (BAMASK & (br->instruction_addr >> 2));
}

void get_xor_operation(void)
{
	xor_output_g = branch_history_g ^ branch_address_g;
}
unsigned int get_predict(const branch_record_c* br)
{
	predict_tag_g = ( TAGMASK & (br->instruction_addr)>>12);
	predict_set_g = predict_table_set_g[predict_tag_g];
	predict_output = (0x1 & (predict_table_g[predict_set_g] >> 1));
	return predict_output;
}
    
// Update the predictor after a prediction has been made.  This should accept
// the branch record (br) and architectural state (os), as well as a third
// argument (taken) indicating whether or not the branch was taken.
void PREDICTOR::update_predictor(const branch_record_c* br, const op_state_c* os, bool taken){

	//Update path history values
	update_global_branch_history(taken);


	//update predict table
	update_predict_table(taken);

}

void update_global_branch_history(bool taken)
{
	if(taken)
		path_history = PATHHISTVAL & ((path_history << 1) | SETLSB);
	else
		path_history = PATHHISTVAL & (path_history << 1);
}

void update_predict_table(bool taken)
{
		if(predict_output == taken)
			if(predict_set_g < 0x3)
			 	predict_table_set_g[predict_tag_g]++;
		else if(predict_set_g > 0x1)
				predict_table_set_g[predict_tag_g]--;
}


void debug(unsigned int val, int tag){
	#ifdef	BS_VERBOSE
	FILE * fh = NULL;
	fh = fopen("log.txt", "a");
	
	char binary[13] = {0};
	
	if(tag != LINE){
		for(char i=12; i > 0 ; i--){
			if((val >> (i - 1)) & 0x1) binary[11 - (i-1)] = '1';
			else binary[11 - (i -1)] = '0';
		}
	}


	switch(tag){
		case PGL:	fprintf(fh, "%s\t", binary);
			break;
		case PGP:	fprintf(fh, "%s\t", binary);
			break;
		case PLP:	fprintf(fh, "%s\t", binary);
			break;
		case PLH:	fprintf(fh, "%s\t", binary);
			break;
		case TAKEN:	fprintf(fh, "%-12u\t", val);
			break;
		case ULP:	fprintf(fh, "%s\t", binary);
			break;
		case ULH:	fprintf(fh, "%s\t", binary);
			break;
		case UGP:	fprintf(fh, "%s\t", binary);
			break;
		case UGL:	fprintf(fh, "%s\t", binary);
			break;
		case PATHH:	fprintf(fh, "%-12s\t", binary);
			break;
		case NEWLINE:fprintf(fh, "\r\n");
			break;
		case LINE:	fprintf(fh, "%-12lu\t", t);
			break;
		case BINDEX:	fprintf(fh, "%s\t", binary);
			break;
		case HEAD:
					fprintf(fh,"%-12s\t",			 "LINE");
					fprintf(fh,"%-12s\t",			 "BINDEX");
					fprintf(fh,"%-12s\t",			 "P-Choice");
					fprintf(fh,"%-12s\t",			 "P-Global P");
					fprintf(fh,"%-12s\t",			 "P-Local P");
					fprintf(fh,"%-12s\t",			 "P-LocalHist");
					fprintf(fh,"%-12s\t",			 "P-Path Hist");
					fprintf(fh,"%-12s\t",			 "taken");
					fprintf(fh,"%-12s\t",			 "U-G|L");
					fprintf(fh,"%-12s\t",			 "U-LocalHist");
					fprintf(fh,"%-12s\t",			 "U-LocalPred");
					fprintf(fh,"%-12s\t",			 "U-GlobalPred");
					fprintf(fh,"%-12s",			 "U-Path Hist");
			break;
	} 
	fclose(fh);
	#endif
}