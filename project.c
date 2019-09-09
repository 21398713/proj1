#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define DEBUG_ON


#define MAX_DEVICES 		4
#define MAX_DEVICE_NAME_LENGTH	20
#define MAX_PROCESSES		50

#define MAX_EVENTS_PER_PROCESS	100

#define TIME_CONTEXT_SWITCH	5
#define TIME_ACQUIRE_BUS	5

#define MAX_WORD_LENGTH		20

#define CHAR_COMMENT		'#'

void parse_tracefile(char program[], char tracefile[]) {
	
	FILE *fp = fopen(tracefile, "r");
	
	if (fp == NULL) {
		printf("%s: Unable to open '%s'\n", program, tracefile);
		exit(EXIT_FAILURE);	
	}
	
	char line[BUFSIZ];
	int line_count = 0;

	while (fgets(line, sizeof(line), fp) != NULL) {

		line_count++;

		if (line[0] == CHAR_COMMENT) {
			continue;
		}	
		
		char word0[MAX_WORD_LENGTH];
		char word1[MAX_WORD_LENGTH];
		char word2[MAX_WORD_LENGTH];
		char word3[MAX_WORD_LENGTH];

		int n_words = sscanf(line, "%s %s %s %s", word0, word1, word2, word3);

#ifdef DEBUG_ON
		printf("%i = %s", n_words, line);
#endif

		if (n_words <= 0) {
			continue;
		}
		
		if ((n_words == 4) && (strcmp(word0, "device") == 0)) {
			// DEVICE DEFINITION

					
		}
		

		else if ((n_words == 1) && (strcmp(word0, "reboot") == 0)) {
			// NOTHING REQUIRED, DEFINITIONS FINISHED
	
		}	

		else if ((n_words == 4) && (strcmp(word0, "process") == 0)) {
			// START OF PROCESS EVENTS

		
		}
		
		else if ((n_words == 4) && (strcmp(word0, "i/o") == 0)) {
			// I/O EVENT FOR CURRENT PROCESS
			
		} 

		else if ((n_words == 2) && (strcmp(word0, "exit") == 0)) {
			// LAST EVENT WE SEE FOR THIS PROCESS
	
		}

		else if ((n_words == 1) && (strcmp(word0, "}") == 0)) {
			// END OF CURRENT PROCESS
		}

		else {			
			printf("Unrecognised input in file %s\n\t<line: %i>\t'%s'",
				program, line_count, tracefile);

			exit(EXIT_FAILURE);
		}
		
	}
	
	fclose(fp);

}

void simulate_job_mix(int time_quantum) {
	printf("Running simulate_job_mix(time_quantum = %i usecs)\n", time_quantum);

}

void print_usage(char program[]) {
	printf("Usage: %s tracefile TQ-first [TQ-final TQ-increment]\n", program);
	exit(EXIT_FAILURE);

}

int main(int argc, char * argv[]) {

	int TQ0 = 0;
	int TQfinal = 0;
	int TQinc = 0;

	// Tracefile + 3 values
	if (argc == 5) {
		TQ0 = atoi(argv[2]);
		TQfinal = atoi(argv[3]);
		TQinc = atoi(argv[4]);

		if (TQ0 < 1 || TQfinal < TQ0 || TQinc < 1) {
			print_usage(argv[0]);
		}
	}
	
	// Tracefile + 1 time value

	else if (argc == 3) {
		TQ0 = atoi(argv[2]);
		
		if (TQ0 < 1) {
			print_usage(argv[0]);
		}

		TQfinal = TQ0;
		TQinc = 1;

	}

	// Incorrect usage

	else {
		print_usage(argv[0]);
	}
	
	parse_tracefile(argv[0], argv[1]);

	// SUIMULATION HAPPENS HERE

	for (int t = TQ0; t < TQfinal; t += TQinc) {
		simulate_job_mix(t);
	}	
	
	// print best here
	// printf("Best TQ: %i \nTotal time: %i\n", time_optimal, time_total);

	exit(EXIT_SUCCESS); 

}

