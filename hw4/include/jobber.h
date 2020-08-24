/**
 * === DO NOT MODIFY THIS FILE ===
 * If you need some other prototypes or constants in a header, please put them
 * in another header file.
 *
 * When we grade, we will be replacing this file with our own copy.
 * You have been warned.
 * === DO NOT MODIFY THIS FILE ===
 */

#include "sf_readline.h"
#include "debug.h"

/* Maximum number of jobs that can exist at one time. */
#define MAX_JOBS 8

/*
 * Maximum number of runner processes that are allowed to exist at one time.
 */
#define MAX_RUNNERS 4

/*
 * The following are the states that a job can be in.
 * A job starts out in the NEW state.
 * Once it has been fully initialized, it transitions to the WAITING state.
 * A job that is WAITING may be assigned a runner process and set to the
 *   RUNNING state if the current number of runners is less than MAX_RUNNERS.
 * The user may choose to pause a RUNNING job, in which case the job status
 *   is set to PAUSED and the runner's process group is sent a SIGSTOP signal.
 * The user may choose to resume a PAUSED job, in which case the job status
 *   is set to RUNNING and the runner's process group is sent a SIGCONT signal.
 * The user may choose to cancel a job that is in any state other than
 *   CANCELED, COMPLETED, or ABORTED.  In this case, the job status is set
 *   to CANCELED and the runner's process group is sent a SIGKILL signal.
 * A job whose task exits normally is set to the COMPLETED state.
 * A job whose task terminates as a result of a signal is set to the
 *   ABORTED state.
 */
typedef enum {
    NEW,        // Newly created, not runnable
    WAITING,    // Runnable, waiting for an available runner
    RUNNING,    // Started, actively running
    PAUSED,     // Started, waiting to be resumed
    CANCELED,   // Canceled, pending termination
    COMPLETED,  // Terminated normally, exit status available
    ABORTED     // Terminated abnormally by a signal
} JOB_STATUS;

/*
 * The following array (indexed by JOB_STATUS) contains string names for the
 * various job statuses.
 */
extern char *job_status_names[];

/*======================================================================================*/

/*
 * IMPORTANT: The following functions are provided for you as a library that
 * will be linked with your program.  Your program must call these library
 * functions at the specified times.  The default implementations of these
 * functions produce printout on stderr when they are called.
 * When we grade your assignment, we will replace the implementations of these
 * functions by different versions that allow us to check automatically
 * whether your program is functioning properly.
 */

/*
 * The following function must be called when a new job is created.
 *
 * @param jobid  The job ID of the job that has been created.
 */
void sf_job_create(int jobid);

/*
 * The following function must be called when a job is expunged.
 *
 * @param jobid  The job ID of the job that has been expunged.
 */
void sf_job_expunge(int jobid);

/*
 * The following function must be called any time a job changes its status
 * (other than the initial setting of the status to NEW).
 *
 * @param jobid  The job ID of the job that is changing status.
 * @param old  The status of the job before the change.
 * @param new  The status of the job after the change.
 */
void sf_job_status_change(int jobid, JOB_STATUS old, JOB_STATUS new);

/*
 * The following function must be called when a runner process is created
 * for a job.
 *
 * @param jobid  The job ID of the job for which the runner process is being
 * created.
 * @param pgid  The process group ID of the runner process.
 */
void sf_job_start(int jobid, int pgid);

/*
 * The following function must be called when a runner process is reaped
 * upon termination.
 *
 * @param jobid  The job ID of the job that the runner process was running.
 * @param pgid  The process group ID of the runner process.
 * @param result  The exit status of the runner process, as returned by waitpid().
 */
void sf_job_end(int jobid, int pgid, int result);

/*
 * The following function must be called when receipt of a SIGCHLD signal
 * indicates that a runner process has become stopped.
 *
 * @param jobid  The job ID of the job that the runner process was running.
 * @param pgid  The process group ID of the runner process.
 */
void sf_job_pause(int jobid, int pgid);

/*
 * The following function must be called when receipt of a SIGCHLD signal
 * indicates that a previously stopped runner process has continued.
 *
 * @param jobid  The job ID of the job that the runner process was running.
 * @param pgid  The process group ID of the runner process.
 */
void sf_job_resume(int jobid, int pgid);

/*
 * If the following variable is set to 0, then the normal printout produced
 * by the above functions is suppressed.  The default value is nonzero, so that
 * the printout is produced
 */
int sf_suppress_chatter;

/*======================================================================================*/

/*
 * The functions specified below are to be implemented by you.
 * We might choose to test these functions individually, so they must conform
 * exactly to the indicated specifications.
 */

/*
 * @brief  Initialize the job spooler.
 * @details  This function must be called once before any of the other job-related
 * functions are called.  It performs any initialization required to prepare to
 * process jobs.
 *
 * @return 0 if initialization is successful, -1 if it fails.
 */
int jobs_init(void);

/*
 * @brief  Finalize the job spooler.
 * @details  This function must be called once when job processing is to be terminated.
 * It cancels any remaining jobs, waits for them to terminate, expunges all jobs,
 * and frees any other memory or resources before returning.
 */
void jobs_fini(void);

/*
 * @brief  Set whether jobs can be started.
 * @param enabled  If nonzero, then jobs can be started, otherwise jobs cannot
 * be started.
 * @return  The previous setting of this flag.
 */
int jobs_set_enabled(int enabled);

/*
 * @brief  Get whether jobs can be started.
 * @return  Nonzero if jobs can be started, zero if not.
 */
int jobs_get_enabled(void);

/*
 * @brief  Create a new job.
 * @details  This function creates a new job to run the task given as a parameter.
 * It parses the task, validating that it is syntactically well-formed.
 * It allocates an entry in the job table for the new job and sets the status
 * of the entry to NEW.  Once the job has been fully initialized, the status of the
 * job transitions to WAITING.
 *
 * @param task  A string that describes the task to be executed by the job runner process.
 * @return  The job ID (an integer in the range [0, MAX_JOBS) if the job was successfully
 * created, and -1 otherwise.  Job creation should fail if any required resources
 * (e.g. memory) could not be allocated, if the job table is full, or if the command
 * string is not syntactically well-formed.
 */
int job_create(char *task);

/*
 * @brief  Expunge a terminated job.
 * @details  This function takes a job ID as a parameter and, if that job is in the
 * COMPLETED or ABORTED state, then it is removed from the job table and all resources
 * (e.g. memory) held by that job are freed.
 *
 * @param jobid  An integer in the range [0, MAX_JOBS).
 * @return 0  If the job exists, was in the COMPLETED or ABORTED state, and was
 * successfully expunged, and -1 otherwise.
 */
int job_expunge(int jobid);

/*
 * @brief  Attempt to cancel a job.
 * @details  This function takes a job ID as a parameter and, if that job is in the
 * WAITING, RUNNING, or PAUSED state, then it is set to the CANCELED state.
 * In addition, if the job was in the RUNNING or PAUSED state, then a SIGKILL signal
 * is sent to the process group of the job's runner process in order to forcibly
 * terminate it.  If the job was in the WAITING state then it does not have a runner,
 * so no signal is sent in that case.
 *
 * Note that this function must appropriately mask signals in order to avoid a race
 * between the querying of the job status and the subsequent setting of the status
 * to CANCELED and the sending of the SIGKILL signal.
 *
 * @param jobid  An integer in the range [0, MAX_JOBS).
 * @return 0  If the job exists, was in the WAITING, RUNNING, or PAUSED state,
 * and, if the job was in the RUNNING or PAUSED state, then the SIGKILL signal was
 * successfully sent.  Otherwise, -1 is returned.
 */
int job_cancel(int jobid);

/*
 * @brief  Pause a running job.
 * @details  This function takes a job ID as a parameter and, if that job is in the
 * RUNNING state, then it is set to the PAUSED state and a SIGSTOP signal is sent
 * to the process group of the job's runner process in order to cause the job to
 * pause execution.
 *
 * Note that this function must appropriately mask signals in order to avoid a race
 * between the querying of the job status and the subsequent setting of the status
 * to PAUSED and the sending of the SIGSTOP signal.
 *
 * @param jobid  An integer in the range [0, MAX_JOBS).
 * @return 0  If the job exists, was in the RUNNING state, and the SIGSTOP signal
 * was successfully sent.  Otherwise, -1 is returned.
 */
int job_pause(int jobid);

/*
 * @brief  Resume a paused job.
 * @details  This function takes a job ID as a parameter and, if that job is in the
 * PAUSED state, then it is set to the RUNNING state and a SIGCONT signal is sent
 * to the process group of the job's runner process in order to cause the job to
 * continue execution.
 *
 * Note that this function must appropriately mask signals in order to avoid a race
 * between the querying of the job status and the subsequent setting of the status
 * to RUNNING and the sending of the SIGCONT signal.
 *
 * @param jobid  An integer in the range [0, MAX_JOBS).
 * @return 0  If the job exists, was in the PAUSED state, and the SIGCONT signal
 * was successfully sent.  Otherwise, -1 is returned.
 */
int job_resume(int jobid);

/*
 * @brief  Get the current status of a job.
 * @return  If the specified job ID is the ID of an existing job, then its current
 * status is returned, otherwise -1 is returned.
 */
JOB_STATUS job_get_status(int jobid);

/*
 * @brief  Get the process group ID of a job's runner process.
 * @return  If the specified job ID is the ID of an existing job that is in the
 * RUNNING, PAUSED, or CANCELED state, then the process group ID of the job's
 * runner process is returned, otherwise -1 is returned.  Note that as the runner
 * process might terminate at any time, there is no guarantee that the returned
 * process group ID will actually be valid for any length of time.
 */
int job_get_pgid(int jobid);

/*
 * @brief  Get the result (i.e. the exit status) of a job.
 * @return  If the specified job ID is the ID of an existing job that is in the
 * COMPLETED state, then the exit status of the job's runner process is returned,
 * otherwise -1 is returned.  The exit status should be exactly as would be
 * returned by the waitpid() function.
 */
int job_get_result(int jobid);

/*
 * @brief  Determine if a job was successfully canceled.
 * @return If the specified job ID is the ID of an existing job that is in the
 * ABORTED state, the job was explicitly canceled by the user, and the job was
 * terminated by a SIGKILL signal at some point after having been set to the
 * CANCELED state, then 1 is returned, otherwise 0 is returned.
 */
int job_was_canceled(int jobid);

/*
 * @brief  Get the task specification of a job.
 * @return  If the specified job ID is the ID of an existing job, then the task
 * specification string that was passed when the job was created is returned,
 * otherwise NULL is returned.  The specification string will remain valid until
 * the job has been expunged, after which the string should no longer be used.
 */
char *job_get_taskspec(int jobid);

