#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>


// PREPROCESSOR DEFINITIONS
#define DEBUG_ON

#define MAX_DEVICES 						4
#define MAX_DEVICE_NAME_LENGTH	20
#define MAX_PROCESSES						50

#define MAX_EVENTS_PER_PROCESS	100

#define TIME_CONTEXT_SWITCH			5
#define TIME_ACQUIRE_BUS				5

#define MAX_WORD_LENGTH					20

#define CHAR_COMMENT						'#'

// FLAGS
enum m_process_status {
		READY,
		REQUEST_DATA_BUS,
		BLOCKED,
		RUNNING,
		NEW,
		PROCESS_COMPLETE
};

enum m_process_type {
	IO			= 0,
	REBOOT	= 1,
	EXIT		= 2
};

// DATA STRUCTURES
struct device {
	char device_type[MAX_DEVICE_NAME_LENGTH];
	int device_speed;
	bool blocked;
	int priority;
};

struct event {
	enum m_process_type process_type;
	int event_start_time;
	struct device process_device;
	int bytes_transferred;
	int event_execution_length;
	bool complete;
	int parent_id;
};

struct process {
	int process_id;
	int process_start_time;
	struct event events[MAX_EVENTS_PER_PROCESS];
	int event_count;
	int current_event;
	int process_execution_time;
	enum m_process_status process_status;

	int execution_begin;
};

struct device devices[MAX_DEVICES];
struct process processes[MAX_PROCESSES];

struct event event_queue[MAX_DEVICES * MAX_PROCESSES * MAX_EVENTS_PER_PROCESS];
int event_count_per_device[MAX_DEVICES];
int event_offset_per_device[MAX_DEVICES];

int device_count = 0;
int process_count = 0;

int process_finished_count = 0;

int system_time = 0;

bool finished = false;

struct device get_device_for_name(char * name) {
		for (int i = 0; i < MAX_DEVICES; i++) {
			if (strcmp(devices[i].device_type, name) == 0) {
				return devices[i];
			}
		}
		printf("Device doesn't exist. Aborting\n");
		exit(EXIT_FAILURE);
}

int get_priority(char * name) {
	struct device d = get_device_for_name(name);
	return d.priority;
}

void set_device_priorities() {
	int priorities[device_count];

	for (int i = 0; i < device_count; i++) {
		priorities[i] = devices[i].device_speed;
	}

	for (int j = 0; j < device_count; j++) {
		for (int i = 1; i < device_count; i++) {
			if (priorities[i] > priorities[i - 1]) {
				int tmp = priorities[i - 1];

				priorities[i - 1] = priorities[i];
				priorities[i] = tmp;
			}
		}
	}

	for (int i = 0; i < device_count; i++) {
		for (int j = 0; j < device_count; j++) {
			if (devices[i].device_speed == priorities[j]) {
				devices[i].priority = j;
			}
		}
	}
}

float calc_event_time(struct event ev) {
	// time in seconds
	struct device d = get_device_for_name(ev.process_device.device_type);

	if (d.device_speed == 0) {
		return 0;
	}
	float delta = (float)ev.bytes_transferred / (float)d.device_speed;

	// multiply by 1000000000 to get time in microseconds
	delta = delta * 1000000000;
	ev.event_execution_length = delta;

	return delta;
}

#ifdef DEBUG_ON
// USED FOR DEBUGGING PURPOSES TO ENSURE DATA LOADED CORRECTLY
void print_devices() {
	for (int i = 0; i < MAX_DEVICES; i++) {
		printf("Device %i: {\n\ttype:\t%s \n\tspeed:\t%i bytes/sec\n\tpriority:\t%i\n }\n\n", i, devices[i].device_type, devices[i].device_speed, devices[i].priority);
	}
}

void print_processes() {
	for (int i = 0; i < process_count; i++) {
		printf("Process %i : @%i {\n", processes[i].process_id, processes[i].process_start_time);

		for (int s = 0; s < processes[i].event_count; s++) {
			struct event ev = processes[i].events[s];
			if (ev.process_type != EXIT) {
				printf("\t%i\t%i\t%s\t%i\n", ev.process_type, ev.event_start_time, ev.process_device.device_type, ev.bytes_transferred);
				printf("Time: %f\n", calc_event_time(ev));
			}	else {
				printf("\t%i\t%i\n", ev.process_type, ev.event_start_time);
			}
		}

		printf("}\n\n");
	}

}

#endif

int last_exit = 0;

// LOADS A TRACEFILE WITH NAME tracefile AND PARSES DATA INTO THE DATA STRUCTURES WE DEFINED EARLIER
void parse_tracefile(char program[], char tracefile[]) {

	FILE *fp = fopen(tracefile, "r");

// Make sure this file exists
	if (fp == NULL) {
		printf("%s: Unable to open '%s'\n", program, tracefile);
		exit(EXIT_FAILURE);
	}

	char line[BUFSIZ];
	int line_count = 0;

	struct process p_current;

	while (fgets(line, sizeof(line), fp) != NULL) {

		line_count++;

		if (line[0] == CHAR_COMMENT) {
			continue;
		}

		char word0[MAX_WORD_LENGTH];
		char word1[MAX_WORD_LENGTH];
		char word2[MAX_WORD_LENGTH];
		char word3[MAX_WORD_LENGTH];

		// Read the line into our 4 character arrays, splitting at spaces
		int n_words = sscanf(line, "%s %s %s %s", word0, word1, word2, word3);

/*
#ifdef DEBUG_ON
		printf("%i = %s", n_words, line);
#endif
*/

// We haven't reached EOF, but there was no data on this line
		if (n_words <= 0) {
			continue;
		}

		if ((n_words == 4) && (strcmp(word0, "device") == 0)) {
			// DEVICE DEFINITION

			struct device d;

			int len = strlen(word1);
			strncpy(d.device_type, word1, len);

			// null byte not appending properly add it to the end
			d.device_type[len] = 0;

			d.device_speed = atoi(word2);

			d.blocked = false;

			devices[device_count++] = d;

		}

		else if ((n_words == 1) && (strcmp(word0, "reboot") == 0)) {
			// end of device definitions, sort the devices into priority based on transfer speed
			set_device_priorities();
		}

		else if ((n_words == 4) && (strcmp(word0, "process") == 0)) {
			// START OF PROCESS EVENTS
			// Parse all the data into our data structure and initialise variables
			// we set a current process in upper scope, as the events within it are processed over multiple passes
				p_current.process_id = atoi(word1);
				p_current.process_start_time = atoi(word2);
				p_current.event_count = 0;
				p_current.current_event = 0;
				p_current.process_execution_time = 0;
				p_current.process_status = NEW;
		}

		else if ((n_words == 4) && (strcmp(word0, "i/o") == 0)) {
			// I/O EVENT FOR CURRENT PROCESS
			struct event p;

			p.process_type = IO;
			p.event_start_time = atoi(word1);

			int len = strlen(word2);
			strncpy(p.process_device.device_type, word2, strlen(word2));

			// null byte not appending properly, add to end
			p.process_device.device_type[len] = 0;

			p.bytes_transferred = atoi(word3);

			p.parent_id = p_current.process_id;

			// add this event to the event list of the current process we are loading
			p_current.events[p_current.event_count++] = p;
		}

		else if ((n_words == 2) && (strcmp(word0, "exit") == 0)) {
			// LAST EVENT WE SEE FOR THIS PROCESS
			struct event p;

			p.process_type = EXIT;
			p.event_start_time = atoi(word1);

			if (p.event_start_time > last_exit) {
				last_exit = p.event_start_time;
			}

			strncpy(p.process_device.device_type, "", strlen(""));

			p.bytes_transferred = 0;

			p_current.process_execution_time = atoi(word1);

			p.parent_id = p_current.process_id;

			p_current.events[p_current.event_count++] = p;
		}

		else if ((n_words == 1) && (strcmp(word0, "}") == 0)) {
			// END OF CURRENT PROCESS
			// This is the end of a process definition, push the current loading process to the list of processes
			processes[process_count++] = p_current;
		}

		else {
			printf("Unrecognised input in file %s\n\t<line: %i>\t'%s'",
				program, line_count, tracefile);

			exit(EXIT_FAILURE);
		}
	}

	fclose(fp);

#ifdef DEBUG_ON

	print_devices();
	print_processes();

#endif

}

#undef MAX_WORD_LENGTH

#undef CHAR_COMMENT

int simulate_job_mix(int time_quantum) {
	printf("Running simulate_job_mix(time_quantum = %i usecs)\n", time_quantum);

	system_time = 0;
	finished = false;

	bool processor_free = true;
	bool bus_free = true;

	struct event e_current;
	e_current.complete = false;

	int complete_process_count = 0;

	// Loop indefinitely until we have completed all jobs
	while (!finished) {

	// Set any new processes to ready if system_time has passed their start time
		for (int i = 0; i < process_count; i++) {
			if (processes[i].process_start_time <= system_time && processes[i].process_status == NEW) {
				processes[i].process_status = READY;

				printf("@%d\t:process %d \t\tNEW->READY\n", system_time, i);
			}
		}
		// we're using the data bus
		if (!bus_free) {
			printf("fff\n");
			if (!e_current.complete) {
				e_current.event_execution_length--;

				if (e_current.event_execution_length <= 0) {
					e_current.complete = true;
					bus_free = true;
					processor_free = true;

					processes[e_current.parent_id].process_status = READY;

					printf("@%d\t:process %d\t\tBLOCKED->READY\n", system_time, e_current.parent_id);
				}
			}
		} else if (bus_free) {
			printf("nnn\n");
			for (int a = 0; a < device_count; a++) {
				for (int b = event_offset_per_device[a]; b < event_count_per_device[a]; b++) {
					if (system_time >= event_queue[a][b].event_start_time) {
						e_current = event_queue[a][b];
						event_offset_per_device[a]++;
						bus_free = false;

						printf("@%d\t:process %d IO\t\n", system_time, e_current.parent_id);
						break;
					}
				}
			}
		}

		for (int i = 0; i < process_count; i++) {
			if (processes[i].process_status == RUNNING) {
				// if a running process has been executing longer than the time_quantum
				// halt the process and return its status to ready so we can execute the next one
				if (system_time - processes[i].execution_begin >= time_quantum) {
					processes[i].process_status = READY;

					processor_free = true;

					printf("@%d\t:process %d EXPIRED\tRUNNING->READY\n", system_time, i);

					i = (i + 1) % process_count;
				} else {
					struct event e = processes[i].events[processes[i].current_event++];
					if (e.process_type == EXIT) {
						processes[i].process_status = PROCESS_COMPLETE;
						complete_process_count++;
						continue;
					}
					int device_priority = get_priority(e.process_device.device_type);
					event_queue[device_priority][event_count_per_device[device_priority]] = e;
					processes[i].process_status = BLOCKED;
					system_time += 5;
					printf("@%d\t:process %d\t\tRUNNING->BLOCKED\n", system_time, i);
				}
			}

			if (processor_free) {
				// If we're not currently executing a process, find the next ready process and begin execution
				if (processes[i].process_status == READY) {
					processes[i].process_status = RUNNING;

					// processor occupied
					processor_free = false;

					printf("@%d\t:process %d \t\tREADY->RUNNING\n", system_time, i);

					system_time += 5;

					processes[i].execution_begin = system_time;

					break;
				}
			}
		}

		if (complete_process_count >= process_count) {
			finished = true;
		}

		system_time++;

	}

	return 0;
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

	// SIMULATION HAPPENS HERE

	// Set to large number
	int current_best = 0xFFFFFFFF;

	for (int t = TQ0; t < TQfinal; t += TQinc) {
		for (int j = 0; j < process_count; j++) {
			processes[j].process_status = NEW;
		}

		int i = simulate_job_mix(t);

		// If we found a better time, store it as the best time
		if (i < current_best) {
			i = current_best;
		}
	}

	// print best here
	// printf("Best TQ: %i \nTotal time: %i\n", time_optimal, time_total);

	exit(EXIT_SUCCESS);

}
