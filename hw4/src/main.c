#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "helper.h"

/*
 * "Jobber" job spooler.
 */



int main(int argc, char *argv[])
{
    char * help_menu = "Available commands:\n \
    help (0 args) Print this help message\n \
    quit (0 args) Quit the program\n \
    enable (0 args) Allow jobs to start\n \
    disable (0 args) Prevent jobs from starting\n \
    spool (1 args) Spool a new job\n \
    pause (1 args) Pause a running job\n \
    resume (1 args) Resume a paused job\n \
    cancel (1 args) Cancel an unfinished job\n \
    expunge (1 args) Expunge a finished job\n \
    status (1 args) Print the status of a job\n \
    jobs (0 args) Print the status of all jobs\n";

    jobs_init();
    while(1){
        debug("START");
        char * prompt = "jobber>";
        char *  input =  sf_readline(prompt);
        sf_set_readline_signal_hook(&child_handler);
        if(input==NULL){
            printf("GOT NULL");
            jobs_fini();
            exit(0);
        }

        int n = parse_input(input);
        // debug("N = %d ",n);
        // if(n>0)debug(" ptr[0] = %s",ptr[0]);
        // if(n>1)debug(" ptr[1] = %s",ptr[1]);

        if(n==0){
            continue;
        }

        else if(strcmp(ptr[0],"help")==0){
            printf("%s", help_menu);
        }
        else if(strcmp(ptr[0],"quit")==0){
            jobs_fini();
            free(ptr[0]);
            exit(0);
        }
        else if(strcmp(ptr[0],"jobs")==0){
            if(n!=1)
                printf("Wrong number of args (given: %d, required: 0) for command 'jobs'",n-1);
            else
                print_jobs();
        }
        else if(strcmp(ptr[0],"spool")==0){
            debug("hello");
            if(n!=2)
                printf("Wrong number of args (given: %d, required: 1) for command 'spool'",n-1);
            else{
                int l = strlen(ptr[1]);
                if(ptr[1][0]=='\'' && ptr[1][l-1]=='\''){
                    ptr[1][l-1]='\0';
                    ptr[1]=ptr[1]+1;
                }
                job_create(ptr[1]);
            }
        }
        else if(strcmp(ptr[0],"expunge")==0){
            if(n!=2)
                printf("Wrong number of args (given: %d, required: 1) for command 'expunge'",n-1);
            else
                job_expunge(my_stoi(ptr[1]));;
            
        }
        else if(strcmp(ptr[0],"enable")==0){
            if(n!=1)
                printf("Wrong number of args (given: %d, required: 0) for command 'enable'",n-1);
            else
                jobs_set_enabled(1);
        }
        else if(strcmp(ptr[0],"disable")==0){
            if(n!=1)
                printf("Wrong number of args (given: %d, required: 0) for command 'disable'",n-1);
            else
                jobs_set_enabled(0);
        }
        else if(strcmp(ptr[0],"pause")==0){
            if(n!=2)
                printf("Wrong number of args (given: %d, required: 1) for command 'pause'",n-1);
            else
                job_pause(my_stoi(ptr[1]));;
            
        }
        else if(strcmp(ptr[0],"cancel")==0){
            if(n!=2)
                printf("Wrong number of args (given: %d, required: 1) for command 'cancel'",n-1);
            else
                job_cancel(my_stoi(ptr[1]));;
            
        }
        else if(strcmp(ptr[0],"resume")==0){
            if(n!=2)
                printf("Wrong number of args (given: %d, required: 1) for command 'resume'",n-1);
            else
                job_resume(my_stoi(ptr[1]));;
            
        }
        else if(strcmp(ptr[0],"status")==0){
            if(n!=2)
                printf("Wrong number of args (given: %d, required: 1) for command 'status'",n-1);
            else
                job_get_status(my_stoi(ptr[1]));;
            
        }
        else{
            printf("Unrecognized command: %s",ptr[0]);
        }
        free(ptr[0]);

    }

    exit(EXIT_FAILURE);
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
