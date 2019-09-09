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
	

	}



}





