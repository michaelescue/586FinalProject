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

// Data Structures
static unsigned int local_history_table[LHTHEIGHT] = {0};
static unsigned int local_prediction_table[LPTHEIGHT] = {0};
static unsigned int global_prediction_table[GPTHEIGHT] = {0};
static unsigned int choice_prediction_table[CPHEIGHT] = {0};
static unsigned int path_history = 0;
long unsigned int t = 0;


/* Function Prototypes	*/
void debug(unsigned int val, int tag);

bool PREDICTOR::get_prediction(const branch_record_c* br, const op_state_c* os){

	/* Local Prediction logic	*/
	// local_history_table index		=	( LHTADDRMASK & (br->instruction_addr >>2) )
	// local_prediction_table index		=	local_history_table[( LHTADDRMASK & (br->instruction_addr >>2) )]
	// local_prediction					= 	( 0x1 & ( local_prediction_table[local_history_table[( LHTADDRMASK & (br->instruction_addr >>2) )]] >> (LPTWIDTH - 1) ) )

	/* Global Prediction Logic	*/
	// global_prediction_table index	=	path_history
	// global_prediction				= 	( 0x1 & ( global_prediction_table[path_history] >> (GPTWIDTH - 1) ) )

	/* Choice Prediction Logic	*/
	// choice_prediction_table index	=	path_history
	// choice_prediction				=	( 0x1 & ( choice_prediction_table[path_history] >> (CPWIDTH - 1) ) )

	#ifdef BS_VERBOSE
	t++;
	if(t == 1) debug (0, HEAD);
	debug( 0, LINE);
	debug( ( 0x1 & ( choice_prediction_table[path_history] >> (CPWIDTH - 1) ) ), PGL);
	debug( ( 0x1 & ( global_prediction_table[path_history] >> (GPTWIDTH - 1) ) ), PGP);
	debug( ( 0x1 & ( local_prediction_table[local_history_table[( LHTADDRMASK & (br->instruction_addr >>2) )]] >> (LPTWIDTH - 1) ) ), PLP);
	debug( local_history_table[( LHTADDRMASK & (br->instruction_addr >>2) )], PLH );
	#endif

	/* Multiplexer */
	if( !br->is_conditional || br->is_call || br->is_return) return 1;
	return	( 0x1 & ( choice_prediction_table[path_history] >> (CPWIDTH - 1) ) ) 		?	// GLOBAL_PREDICTION or LOCAL_PREDICTION ? 
			( 0x1 & ( global_prediction_table[path_history] >> (GPTWIDTH - 1) ) )		:	// GLOBAL_PREDICTION
			( 0x1 & ( local_prediction_table[local_history_table[( LHTADDRMASK & (br->instruction_addr >>2) )]] >> (LPTWIDTH - 1) ) ); // LOCAL_PREDICTION

}
    
// Update the predictor after a prediction has been made.  This should accept
// the branch record (br) and architectural state (os), as well as a third
// argument (taken) indicating whether or not the branch was taken.
void PREDICTOR::update_predictor(const branch_record_c* br, const op_state_c* os, bool taken){
	
	#ifdef BS_VERBOSE	
	debug(taken, TAKEN);
	#endif

	/* Local Prediction Update logic	*/
	// local_history_table index		=	( LHTADDRMASK & (br->instruction_addr >>2) )
	// local_prediction_table index		=	local_history_table[( LHTADDRMASK & (br->instruction_addr >>2) )]
	// local_prediction value			= 	local_prediction_table[local_history_table[( LHTADDRMASK & (br->instruction_addr >>2) )]]
	
	if(taken && ( local_prediction_table[local_history_table[( LHTADDRMASK & (br->instruction_addr >> 2) )]] == 0x7 ))
		local_prediction_table[local_history_table[( LHTADDRMASK & (br->instruction_addr >> 2) )]] &= LPTVAL;
	else if(taken) local_prediction_table[local_history_table[( LHTADDRMASK & (br->instruction_addr >> 2) )]]++;
	else if(!taken && ( local_prediction_table[local_history_table[( LHTADDRMASK & (br->instruction_addr >> 2) )]] == 0x0 ))
		local_prediction_table[local_history_table[( LHTADDRMASK & (br->instruction_addr >> 2) )]] &= LPTVAL;
	else local_prediction_table[local_history_table[( LHTADDRMASK & (br->instruction_addr >> 2) )]]--;
	
	#ifdef BS_VERBOSE
	debug( ( 0x1 & ( local_prediction_table[local_history_table[( LHTADDRMASK & (br->instruction_addr >>2) )]] >> (LPTWIDTH - 1) ) ), ULP);
	#endif

	/* Local History Update logic	*/
	// local_history_table index		=	( LHTADDRMASK & (br->instruction_addr >>2) )
	// local_history_table value		=	local_history_table[( LHTADDRMASK & (br->instruction_addr >>2) )]

	local_history_table[( LHTADDRMASK & ( br->instruction_addr >> 2 ))] = taken ?
	(LHTVALMASK & (( local_history_table[( LHTADDRMASK & ( br->instruction_addr >> 2 ))] << 1 ) | SETLSB )):		// Shift in 1
	(LHTVALMASK & (( local_history_table[( LHTADDRMASK & ( br->instruction_addr >> 2 ))] << 1 ) & CLRLSB ));		// Shift in 0 

	#ifdef BS_VERBOSE
	debug( local_history_table[( LHTADDRMASK & (br->instruction_addr >>2) )], ULH );
	#endif

	/* Global Prediction Update	*/
	// global_prediction_table index	=	path_history
	// global_prediction_table value	=	global_prediction_table[path_history]
	
	if(taken && ( global_prediction_table[path_history] == 0x3 )) global_prediction_table[path_history] &= GPTVAL;
	else if(taken) global_prediction_table[path_history]++;
	else if(!taken && ( global_prediction_table[path_history] == 0x0 ))	global_prediction_table[path_history] &= GPTVAL;
	else global_prediction_table[path_history]--;
	
	#ifdef BS_VERBOSE
	debug( ( 0x1 & ( global_prediction_table[path_history] >> (GPTWIDTH - 1) ) ), UGP);
	#endif

	/* Choice Prediction Update	*/
	// choice_prediction_table index	=	path_history
	// choice_prediction_table value	=	choice_prediction_table[path_history]
	
	if(taken && ( choice_prediction_table[path_history] == 0x3 )) choice_prediction_table[path_history] &= CPVAL;
	else if(taken) choice_prediction_table[path_history]++;
	else if(!taken && ( choice_prediction_table[path_history] == 0x0 ))	choice_prediction_table[path_history] &= CPVAL;
	else choice_prediction_table[path_history]--;
	
	#ifdef BS_VERBOSE
	debug( ( 0x1 & ( choice_prediction_table[path_history] >> (CPWIDTH - 1) ) ), UGL);
	#endif

	/* Path History Update	*/
	
	path_history = taken ?
	( PATHHISTVAL & (( path_history << 1 ) | SETLSB )):		// Shift in 1
	( PATHHISTVAL & (( path_history << 1 ) | CLRLSB ));		// Shift in 0 

}

void debug(unsigned int val, int tag){
		FILE * fh = NULL;
		fh = fopen("log.txt", "a");

		switch(tag){
			case PGL:	fprintf(fh, "%u\t", val);
				break;
			case PGP:	fprintf(fh, "%u\t", val);
				break;
			case PLP:	fprintf(fh, "%u\t", val);
				break;
			case PLH:	fprintf(fh, "%u\t", val);
				break;
			case TAKEN:	fprintf(fh, "%u\t", val);
				break;
			case ULP:	fprintf(fh, "%u\t", val);
				break;
			case ULH:	fprintf(fh, "%u\t", val);
				break;
			case UGP:	fprintf(fh, "%u\t", val);
				break;
			case UGL:	fprintf(fh, "%u\n", val);
				break;
			case LINE:	fprintf(fh, "%lu\t", t);
				break;
			case HEAD:	fprintf(fh, "LINE\tP-G|L\tP-GP\tP-LP\tP-LH\ttaken\tU-LP\tU-LH\tU-GP\tU-G|L\n");
				break;
		}
		fclose(fh);
}