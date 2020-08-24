/*
 * Job manager for "jobber".
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "jobber.h"
#include "task.h"
#include <fcntl.h>
#include <ctype.h>
//main things
void multi_space_to_single_space(char * s){
    int n = strlen(s);
    if(n==0)return;
    int q=0;
    int prev=1;
    for(int i=1;i<n;i++){
        if(s[i]=='\'')q++;
        if(q%2==0 && isspace(s[i]) && isspace(s[i-1])){

        }else{
            s[prev++]=s[i];
        }
    }
    s[prev]='\0';
}
char* ptr[2];
int split(char * s){
    char * t = s;
    if(strlen(s)==0)return 0;
    int count =0;
    int q=0;
    ptr[0]=s;
    while(*t!='\0'){
        if(isspace(*t) && q%2==0){
            *t='\0';
            if(*(t+1)!='\0'){
                count++;
                if(count<2)
                    ptr[count]= t+1;
            }
            
        }
        else if(*t=='\'')q++;
        t++;
    }
    return count+1;
}
int parse_input(char * s){
    multi_space_to_single_space(s);
    return split(s);
}
int my_stoi(char * s){
    int val = 0;
    while(*s){
        val = val*10 + (*s-'0');
        s++;
    }
    return val;
}
//
struct JOB{
    int exists;
    int status;
    int pgid;
    char * task_str;
    int result;
    TASK * t;
}jobs[MAX_JOBS];

struct RUNNER{
    int exists;
    pid_t pgid;
    int jid;
}runner[MAX_RUNNERS];

int enable;
char ** my_argv;
volatile pid_t mpgid;
volatile sig_atomic_t global_flag = 0;
void set_child_handler_flag(int sig){
    global_flag = 1;
}

int jobs_init(void) {
    if(signal(SIGCHLD, set_child_handler_flag)==SIG_ERR)
        return -1;
    enable=0;
    for(int i=0;i<MAX_JOBS;i++){
        jobs[i].exists = 0;
    }
    for(int i=0;i<MAX_RUNNERS;i++){
        runner[i].exists = 0;
    }
    return 0;
}


int count_words(WORD_LIST * w){
    int count =0;
    while(w){ count++; w = w->rest;}
    return count;
}
void fill_argv(WORD_LIST * w, int n){
    int count =0;
    while(w){
        *(my_argv+count) = w->first;
        count++;
        w = w->rest;
    }
    *(my_argv+count) = NULL;
}
int count_commands_number(COMMAND_LIST * cmd_list){
    int ans =0;
    while(cmd_list){
        cmd_list = cmd_list->rest;
        ans++;
    }
    return ans;
}
int count_pipeline_number(PIPELINE_LIST * pip_list){
    int ans =0;
    while(pip_list){
        pip_list = pip_list->rest;
        ans++;
    }
    return ans;
}
int poll(){
    debug("-------------POLL-------------");
    for(int i=0;i<MAX_JOBS;i++){
        if(jobs[i].exists!=0 && jobs[i].status == WAITING){
            for(int j=0;j<MAX_RUNNERS; j++){
                int assigned= 0;
                if(runner[j].exists==0){
                    assigned = 1;
                    TASK * tsk = jobs[i].t;
                    runner[j].exists = 1;
                    jobs[i].status = RUNNING;
                    runner[j].jid = i;

                    //Signal blocking
                    sigset_t mask_all, prev_mask;
                    sigfillset(&mask_all);
                    sigprocmask(SIG_BLOCK, &mask_all,&prev_mask);

                    mpgid = runner[j].pgid = jobs[i].pgid =fork();

                    if(mpgid==0){
                        // volatile sig_atomic_t done;
                        // int ccount;

                        sigprocmask(SIG_SETMASK,&prev_mask, NULL);
                        setpgid(0,0);
                        mpgid = getpid();
                    
                        debug("mpgid pid %d",mpgid);
                        PIPELINE_LIST * pip_list = tsk->pipelines;

                        debug("PCount %d",count_pipeline_number(pip_list));
                        while(pip_list!=NULL){ 
                            PIPELINE * ppln = pip_list->first;
                            COMMAND_LIST * cmd_list = ppln->commands; 
                            // done=0;
                            int ccount = count_commands_number(cmd_list);
                            debug("CCount %d",ccount);
                            int fd[2];
                            if(pipe(fd)<0){
                                debug("Error cant create pipe \n");
                            }
                            int no_commands=0;
                            while(cmd_list!=NULL){
                                no_commands++;
                                int out =0;
                                int in =0;
                                int pipe_in = 1;
                                int pipe_out = 1;


                                WORD_LIST * wrd_list = cmd_list->first->words;
                                int num_words = count_words(wrd_list);
                                my_argv = (char **)malloc((num_words+1)*sizeof(char *));
                                fill_argv(wrd_list, num_words);

                                if(no_commands==1 && ppln->input_path)in = 1;
                                if(cmd_list->rest==NULL && ppln->output_path)out =1;
                                if(no_commands==1)pipe_in=0;
                                if(cmd_list->rest==NULL) pipe_out=0;


                                debug("WCOUNT %d 0: %s, 1: %s",num_words, my_argv[0], my_argv[1]);
                                pid_t pid;;
                                
                                if((pid=fork())==0) { 
                                    setpgid(0, mpgid);
                                    debug("pid %d pgid %d in %d out %d, pip_in%d pip_out %d\n", \
                                        getpid(),getpgid(getpid()),in,out,pipe_in, pipe_out);
                                    // sf_job_start(cpid, getpgid(cpid));
                                    // sf_job_status_change(cpid, WAITING, RUNNING);
                                    int fd1,fd0;
                                    if(out==1){
                                        fd1 = open(ppln->output_path,O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
                                        if(fd1<0){
                                            debug("Error opening write file");
                                            abort();
                                        }
                                        if(dup2(fd1,1)<0){
                                            debug("Error in dup fd");
                                            abort();
                                        }
                                        close(fd1);
                                    }
                                    if(in==1){
                                        fd0 = open(ppln->input_path,O_RDONLY);
                                        if(fd0<0){
                                            debug("Error opening read file");
                                            abort();
                                        }
                                        if(dup2(fd0,0)<0){
                                             debug("Error in dup fd");
                                             abort();
                                        }
                                        close(fd0);
                                    }
                                    if(pipe_out==1){
                                        debug("pipe out for %s",my_argv[0]);
                                        close(STDOUT_FILENO);
                                        close(fd[0]);
                                        if(dup2(fd[1],1)<0){
                                             debug("Error in dup fd");
                                             abort();
                                        }
                                        close(fd[1]);
                                    }
                                    if(pipe_in==1){
                                        debug("pipe in for %s",my_argv[0]);
                                        close(STDIN_FILENO);
                                        close(fd[1]);
                                        if(dup2(fd[0],0)<0){
                                             debug("Error in dup fd");
                                             abort();
                                        }
                                        close(fd[0]);
                                    }
                                    if (execvp(my_argv[0],my_argv) < 0 ){
                                        debug("Command not found.\n");
                                        abort();
                                    }
                                    //free(my_argv);
                                    exit(0);
                                }else if(pid>0){
                                    runner[j].pgid = mpgid;
                                }else{
                                    debug("FORK ERROR");
                                    abort();
                                }
                                //free(my_argv);
                                cmd_list = cmd_list->rest;
                            }
                            close(fd[0]);
                            close(fd[1]);
                            int child_status;
                            for (int ii = 0; ii < ccount; ii++) {
                                pid_t res = wait(&child_status);

                                if (WIFSIGNALED(child_status) || res==-1){
                                    debug("Child %d terminated abnormally\n", res);
                                    abort();
                                }
                                
                            }
                            pip_list = pip_list->rest;
                            debug("----Pipeline Completed-----");
                        }
                        debug("----JOB Completed-----");
                        exit(0);//job_pid  
                    }else if(mpgid>0){
                        sf_job_start(i, mpgid);
                        sf_job_status_change(i, WAITING,  RUNNING);
                        sigprocmask(SIG_SETMASK,&prev_mask, NULL);
                    }else{
                        return -1;
                    }
                }

                if(assigned)
                    break;
            }
        }
    }
    return 0;
}
int child_handler() {
    //debug("----Child Handler----\n");
    if(global_flag==1){
        pid_t pid;
        int child_status;
        while((pid = waitpid(-1, &child_status, WNOHANG)) != 0) {
            if(pid < 0){
                //perror("wait error");
                return -1;
            }
            int jid=-1;
            for(int i=0;i<MAX_JOBS;i++){
                if(jobs[i].exists!=0 && jobs[i].pgid == pid){
                    jid = i;
                    break;
                }
            }
            if(jid==-1)return -1;
            jobs[jid].result = child_status;
            //make runner free again
            for(int i=0;i<MAX_RUNNERS;i++){
                if(runner[i].exists!=0 && runner[i].pgid == pid){
                    runner[i].exists=0;
                    break;
                }
            }
            if(jid!=-1){
                if (WIFEXITED(child_status)){
                    sf_job_end(jid, pid, child_status);
                    jobs[jid].status = COMPLETED;
                    sf_job_status_change(jid, RUNNING, COMPLETED);

                }
                else if (WIFSIGNALED(child_status)){
                    sf_job_end(jid, pid, child_status);
                    sf_job_status_change(jid, jobs[jid].status, ABORTED);
                    jobs[jid].status = ABORTED;
                    debug("Child terminated abnormally\n");
                }
                else{
                    sf_job_end(jid, pid, child_status);
                    debug("Child %d terminated abnormally\n", pid);

                }
            }

        }
    }
    global_flag = 0;
    if(enable==1)
        poll();
    return 0;
    
}
void print_jobs(){
    for(int id=0;id<MAX_JOBS;id++){
        if(jobs[id].exists!=0){
            if(jobs[id].status==COMPLETED)
                printf("job %d [%s (%d)]: ",id, job_status_names[jobs[id].status],jobs[id].result);
            else
                printf("job %d [%s]: ",id, job_status_names[jobs[id].status]);
            printf("%s\n",jobs[id].task_str);
        }
    }
}

void jobs_fini(void) {
    for(int id=0;id<MAX_JOBS;id++){
        if(jobs[id].exists!=0){
            job_cancel(id);
            int status;
            
            debug("STIL WAITING");
            //sleep(2);
            pid_t x = waitpid(jobs[id].pgid,&status,0);
            debug("PID %d status %d",x, jobs[id].status);
            if(jobs[id].status==CANCELED && x>0){
                sf_job_end(id, x, status);
                if(status==0){
                    sf_job_status_change(id, jobs[id].status, COMPLETED);
                    jobs[id].status = COMPLETED;
                }else{
                    sf_job_status_change(id, jobs[id].status, ABORTED);
                    jobs[id].status = ABORTED;
                }
                
            }
        }
    }
    // while(1){
    //     int flag =1;
    //     for(int id=0;id<MAX_JOBS;id++){
    //         if(jobs[id].exists!=0 && jobs[id].status==CANCELED){
    //             flag=0;
    //         }
    //     }
    //     if(flag)break;
    // }
    for(int id=0;id<MAX_JOBS;id++){
        if(jobs[id].exists!=0){
            job_expunge(id);
        }else{
            free(jobs[id].task_str);
            if(jobs[id].t)
                free_task(jobs[id].t);
        }
    }
}

int jobs_set_enabled(int val) {
    int prev = enable;
    enable = val;
    if(enable==1)
        poll();
    return prev;
}

int jobs_get_enabled() {
    return enable;
}


int job_create(char *command) {
    debug("--------------JOB CREATE------------- ");
    char * copy = (char *)malloc(sizeof(char)*(strlen(command)+1));
    if(copy==NULL)return -1; 
    strcpy(copy, command);
    TASK * task = parse_task(&command);
    if(task==NULL)return -1;
    //free(command);
    int id = -1;
    for(int i=0;i<MAX_JOBS; i++){
        if(jobs[i].exists==0){
            jobs[i].exists=1;
            jobs[i].status = NEW;
            jobs[i].pgid = -1;
            jobs[i].task_str = copy;
            jobs[i].t = task;
            id=i;
            sf_job_create(i);
            break;
        }
    }
    if(id==-1){
        debug("Error None Jobs free: %s",copy);
        return -1;
    }
    jobs[id].status = WAITING;
    sf_job_status_change(id, NEW, WAITING);
    debug("ENABLE BIT %d",enable);
    if(enable==1){
        poll();
    }

    return id;
}

int job_expunge(int jobid) {
    debug("--------------EXPUNGE------------- id -> %d\n",jobid);
    if(jobid<0 || jobid>=MAX_JOBS || jobs[jobid].exists==0 || \
        (jobs[jobid].status!=COMPLETED && jobs[jobid].status!=ABORTED)){
        debug("Error\n");
        return -1;
    }
    free(jobs[jobid].task_str);
    if(jobs[jobid].t)
        free_task(jobs[jobid].t);
    jobs[jobid].exists=0;
    sf_job_expunge(jobid);
    return 0;
}

int job_cancel(int jobid) {
    debug("--------------CancEL------------- id -> %d\n",jobid);
    sigset_t mask_all, prev_mask;
    sigfillset(&mask_all);
    sigprocmask(SIG_BLOCK, &mask_all,&prev_mask);

    if(jobid<0 || jobid >= MAX_JOBS || jobs[jobid].exists==0){
        debug("Job not exists %d\n",jobid);
        sigprocmask(SIG_SETMASK,&prev_mask, NULL);
        return -1;
    }else if(jobs[jobid].status!=PAUSED && jobs[jobid].status!=WAITING && jobs[jobid].status!=RUNNING ){
        debug("Job not paused/waiting/running %d\n",jobid);
        sigprocmask(SIG_SETMASK,&prev_mask, NULL);
        return -1;
    }

    if(jobs[jobid].status == WAITING){
        sf_job_status_change(jobid,  WAITING,  CANCELED);
        jobs[jobid].status = CANCELED;
        sigprocmask(SIG_SETMASK,&prev_mask, NULL);
        return 0;
    }
    pid_t pgid = -1;
    for(int i=0;i<MAX_RUNNERS;i++){
        if(runner[i].exists != 0 && runner[i].jid==jobid){
            pgid = runner[i].pgid;
            break;
        }
    }
    if(pgid==-1){
        sigprocmask(SIG_SETMASK,&prev_mask, NULL);
        return -1;
    }
    
    sigprocmask(SIG_SETMASK,&prev_mask, NULL);
    if(killpg(pgid, SIGKILL) ==0){
        sf_job_status_change(jobid,  jobs[jobid].status,  CANCELED); 
        jobs[jobid].status = CANCELED;   
    }
    else{
       debug("Error in sending KILL signal  %d\n",jobid); 
       sigprocmask(SIG_SETMASK,&prev_mask, NULL);
       return -1;
    }
    
    return 0;
}

int job_pause(int jobid) {
    sigset_t mask_all, prev_mask;
    sigfillset(&mask_all);
    sigprocmask(SIG_BLOCK, &mask_all,&prev_mask);

    debug("--------------PAUSE------------- id -> %d\n",jobid);

    if(jobid<-1 || jobid >= MAX_JOBS || jobs[jobid].exists==0){
        debug("Job not exists %d\n",jobid);
        sigprocmask(SIG_SETMASK,&prev_mask, NULL);
        return -1;
    }else if(jobs[jobid].status!=RUNNING){
        debug("Job not running %d\n",jobid);
        sigprocmask(SIG_SETMASK,&prev_mask, NULL);
        return -1;
    }
    pid_t pgid = -1;
    for(int i=0;i<MAX_RUNNERS;i++){
        if(runner[i].exists != 0 && runner[i].jid==jobid){
            pgid = runner[i].pgid;
            break;
        }
    }
    if(pgid==-1){
        debug("pgid not found");
        sigprocmask(SIG_SETMASK,&prev_mask, NULL);
        return -1;
    }
    
    if(killpg(pgid, SIGSTOP) ==0){
        jobs[jobid].status=PAUSED;
        sf_job_status_change(jobid,  RUNNING,  PAUSED);
        sf_job_pause(jobid, pgid);
    }
    else{
       debug("Error in sending Pause signal  %d\n",jobid); 
       sigprocmask(SIG_SETMASK,&prev_mask, NULL);
       return -1;
    }
    sigprocmask(SIG_SETMASK,&prev_mask, NULL);
    return 0;
}

int job_resume(int jobid) {
    sigset_t mask_all, prev_mask;
    sigfillset(&mask_all);
    sigdelset(&mask_all, SIGCONT);
    sigprocmask(SIG_BLOCK, &mask_all,&prev_mask);

    debug("--------------RESUME------------- id -> %d\n",jobid);

    if(jobid<-1 || jobid >= MAX_JOBS || jobs[jobid].exists==0){
        debug("Job not exists %d\n",jobid);
        sigprocmask(SIG_SETMASK,&prev_mask, NULL);
        return -1;
    }else if(jobs[jobid].status!=PAUSED){
        debug("Job not paused %d\n",jobid);
        sigprocmask(SIG_SETMASK,&prev_mask, NULL);
        return -1;
    }
    pid_t pgid = -1;
    for(int i=0;i<MAX_RUNNERS;i++){
        if(runner[i].exists != 0 && runner[i].jid==jobid){
            pgid = runner[i].pgid;
            break;
        }
    }
    if(pgid==-1){
        sigprocmask(SIG_SETMASK,&prev_mask, NULL);
        return -1;
    }
    
    if(killpg(pgid, SIGCONT) ==0){
        jobs[jobid].status=RUNNING;
        sf_job_status_change(jobid,  PAUSED,  RUNNING);
        sf_job_resume(jobid, pgid); 
    }
    else{
       debug("Error in sending Pause signal  %d\n",jobid); 
       sigprocmask(SIG_SETMASK,&prev_mask, NULL);
       return -1;
    }
    sigprocmask(SIG_SETMASK,&prev_mask, NULL);
    return 0;
}

int job_get_pgid(int jobid) {
    if(jobid<-1 || jobid >= MAX_JOBS || jobs[jobid].exists==0){
        debug("Job not exists %d\n",jobid);
        return -1;
    }
    if(jobs[jobid].status!=PAUSED && jobs[jobid].status!=WAITING && jobs[jobid].status!=RUNNING ){
        debug("Job not paused/waiting/running %d\n",jobid);
        return -1;
    }
    pid_t pgid = -1;
    for(int i=0;i<MAX_RUNNERS;i++){
        if(runner[i].exists != 0 && runner[i].jid==jobid){
            pgid = runner[i].pgid;
            break;
        }
    }
    return pgid;
}

JOB_STATUS job_get_status(int jobid) {
   if(jobid<-1 || jobid >= MAX_JOBS || jobs[jobid].exists==0){
        debug("Job not exists %d\n",jobid);
        return -1;
    }
    return jobs[jobid].status;
}

int job_get_result(int jobid) {
    if(jobid<-1 || jobid >= MAX_JOBS || jobs[jobid].exists==0 || jobs[jobid].status!=COMPLETED){
        debug("Job not exists or not completed%d\n",jobid);
        return -1;
    }
    return jobs[jobid].result;
}

int job_was_canceled(int jobid) {
    if(jobid<-1 || jobid >= MAX_JOBS || jobs[jobid].exists==0){
        debug("Job not exists %d\n",jobid);
        return 0;
    }
    if(jobs[jobid].status == ABORTED)
        return 1;
    else return 0;
}

char *job_get_taskspec(int jobid) {
    if(jobid<-1 || jobid >= MAX_JOBS || jobs[jobid].exists==0){
        debug("Job not exists %d\n",jobid);
        return NULL;
    }
    return jobs[jobid].task_str;
    
}
