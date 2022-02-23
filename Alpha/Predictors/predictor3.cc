/* Author: Mark Faust;   
 * Description: This file defines the two required functions for the branch predictor.
 * Modified by: Kamali, Michael, and Ram
 * ECE 586, Winter 2020
 * Portland State University
*/

#include "predictor.h"

#define LHTADDRMASK 0x3FF // Selects bits 0-9
#define GPTADDRMASK 0xFFF // Selects bits 0-12
#define LHTHEIGHT 1024
#define LHTWIDTH 10
#define LPTHEIGHT 1024
#define LPTWIDTH 3
#define LPTVAL 0x7
#define GPTHEIGHT 4096
#define GPTWIDTH 2
#define GPTVAL 0x3
#define CPHEIGHT 4096
#define CPWIDTH 2
#define CPVAL 0x3
#define LHTVALMASK 0x3FF    ///local history table's 10 bits
#define PATHHISTVAL 0xFFF
#define GLOBAL_PREDICTION 1
#define LOCAL_PREDICTION 0
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

// Data Structures
static unsigned int local_history_table[LHTHEIGHT] = {0};
static unsigned int local_prediction_table[LPTHEIGHT] = {0};
static unsigned int global_prediction_table[GPTHEIGHT] = {0};
static unsigned int choice_prediction_table[CPHEIGHT] = {0};

/* Variables	*/
static unsigned int local_history_table_val = 0;
static unsigned int local_prediction_table_val = 0;
static unsigned int global_prediction_table_val = 0;
static unsigned int choice_prediction_table_val = 0;
static unsigned int b_index = 0;
static unsigned int path_history = 0;
long unsigned int t = 0;


/* Function Prototypes	*/
void debug(unsigned int val, int tag);

bool PREDICTOR::get_prediction(const branch_record_c* br, const op_state_c* os){

	/* Index from PC	*/
	b_index = ( LHTADDRMASK & (br->instruction_addr >>2) );

	/* Local Prediction logic	*/
	// local_history_table index		=	( LHTADDRMASK & (br->instruction_addr >>2) )
	// local_prediction_table index		=	local_history_table[( LHTADDRMASK & (br->instruction_addr >>2) )]
	// local_prediction					= 	( 0x1 & ( local_prediction_table[local_history_table[( LHTADDRMASK & (br->instruction_addr >>2) )]] >> (LPTWIDTH - 1) ) )

	local_history_table_val = local_history_table[b_index];

	local_prediction_table_val = ( 0x1 & local_prediction_table[local_history_table_val] >> (LPTWIDTH -1 )); 

	/* Global Prediction Logic	*/
	// global_prediction_table index	=	path_history
	// global_prediction				= 	( 0x1 & ( global_prediction_table[path_history] >> (GPTWIDTH - 1) ) )

	global_prediction_table_val = ( 0x1 & (global_prediction_table[path_history] >> (GPTWIDTH - 1)));

	/* Choice Prediction Logic	*/
	// choice_prediction_table index	=	path_history
	// choice_prediction				=	( 0x1 & ( choice_prediction_table[path_history] >> (CPWIDTH - 1) ) )

	choice_prediction_table_val = ( 0x1 & (choice_prediction_table[path_history] >> (CPWIDTH - 1)));

	/* Debug	*/
	t++;
	if(t == 1) debug (0, HEAD);
	debug( 0, NEWLINE);
	debug( 0, LINE);
	debug( ( 0x1 & ( choice_prediction_table[path_history] >> (CPWIDTH - 1) ) ), PGL);
	debug( ( 0x1 & ( global_prediction_table[path_history] >> (GPTWIDTH - 1) ) ), PGP);
	debug( ( 0x1 & ( local_prediction_table[local_history_table[( LHTADDRMASK & (br->instruction_addr >>2) )]] >> (LPTWIDTH - 1) ) ), PLP);
	debug( local_history_table[( LHTADDRMASK & (br->instruction_addr >>2) )], PLH );


	/* Multiplexer */

	if( br->branch_target < br->instruction_addr) return 1;

	if( !br->is_conditional || br->is_call || br->is_return) return 1;

	return	choice_prediction_table_val	?	// GLOBAL_PREDICTION or LOCAL_PREDICTION ? 
			global_prediction_table_val:	// GLOBAL_PREDICTION
			local_prediction_table_val; 	// LOCAL_PREDICTION

}
    
// Update the predictor after a prediction has been made.  This should accept
// the branch record (br) and architectural state (os), as well as a third
// argument (taken) indicating whether or not the branch was taken.
void PREDICTOR::update_predictor(const branch_record_c* br, const op_state_c* os, bool taken){
	
	/* Debug	*/
	debug(taken, TAKEN);
	
	/* Local Prediction Update logic	*/
	// local_history_table index		=	( LHTADDRMASK & (br->instruction_addr >>2) )
	// local_prediction_table index		=	local_history_table[( LHTADDRMASK & (br->instruction_addr >>2) )]
	// local_prediction value			= 	local_prediction_table[local_history_table[( LHTADDRMASK & (br->instruction_addr >>2) )]]
	
	if(taken && ( local_prediction_table_val >= 0x7 )) local_prediction_table[local_history_table_val] = 0x7;	
	else if(taken) local_prediction_table[local_history_table_val]++;
	else if(!taken && ( local_prediction_table_val == 0x0 )) local_prediction_table[local_history_table_val] = 0x0;	
	else local_prediction_table[local_history_table_val]--;
	
	/*	Debug	*/
	debug( local_prediction_table[local_history_table_val], ULP);
	
	/* Local History Update logic	*/
	// local_history_table index		=	( LHTADDRMASK & (br->instruction_addr >>2) )
	// local_history_table value		=	local_history_table[( LHTADDRMASK & (br->instruction_addr >>2) )]

	local_history_table[b_index] = taken ?
	(LHTVALMASK & (( local_history_table_val << 1 ) | SETLSB )):		// Shift in 1
	(LHTVALMASK & (( local_history_table_val << 1 ) & CLRLSB ));		// Shift in 0 

	/*	Debug	*/
	debug( local_history_table[b_index], ULH );

	/* Global Prediction Update	*/
	// global_prediction_table index	=	path_history
	// global_prediction_table value	=	global_prediction_table[path_history]
	
	if(taken && ( global_prediction_table[path_history] >= 0x3 )) global_prediction_table[path_history] = 0x3;
	else if(taken) global_prediction_table[path_history]++;
	else if(!taken && ( global_prediction_table[path_history] == 0x0 ))	global_prediction_table[path_history] = 0x0;
	else global_prediction_table[path_history]--;
	
	/*	Debug	*/
	debug( ( global_prediction_table[path_history] ), UGP);

	/* Choice Prediction Update	*/
	// choice_prediction_table index	=	path_history
	// choice_prediction_table value	=	choice_prediction_table[path_history]
	
	if((global_prediction_table_val == taken) && (local_prediction_table_val != taken)){	// 1/0
		if(choice_prediction_table[path_history] == 0x3)
			choice_prediction_table[path_history] = choice_prediction_table[path_history]; 
		else choice_prediction_table[path_history]++;			// Move to higher state.
	}
	if((global_prediction_table_val != taken) && (local_prediction_table_val == taken)){ 	// 0/1
		if(choice_prediction_table[path_history] == 0x0)
			choice_prediction_table[path_history] = choice_prediction_table[path_history]; 
		else choice_prediction_table[path_history]--;			// Move to lower state.
	}

	/*	Debug	*/
	debug( choice_prediction_table[path_history] , UGL);
	debug(0, NEWLINE);

	/* Path History Update	*/
	
	path_history = taken ?
	( PATHHISTVAL & (( path_history << 1 ) | SETLSB )):		// Shift in 1
	( PATHHISTVAL & (( path_history << 1 ) | CLRLSB ));		// Shift in 0 

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
		case UGL:	fprintf(fh, "%s", binary);
			break;
		case NEWLINE:fprintf(fh, "\r\n");
			break;
		case LINE:	fprintf(fh, "%-12lu\t", t);
			break;
		case HEAD:	fprintf(fh,"%-12s\t%-12s\t%-12s\t%-12s\t%-12s\t%-12s\t%-12s\t%-12s\t%-12s\t%-12s",
								"LINE","P-G|L","P-GP","P-LP","P-LH","taken","U-LP","U-LH","U-GP","U-G|L");
			break;
	}
	fclose(fh);
	#endif
}