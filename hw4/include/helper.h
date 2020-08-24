#include "jobber.h"
#include "task.h"

extern char* ptr[2];
extern void print_jobs();
extern int child_handler(void);
extern void multi_space_to_single_space(char * );
extern int split(char * );
extern int parse_input(char * );
extern int my_stoi(char * );