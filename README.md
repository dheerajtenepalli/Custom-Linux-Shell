# Homework 4 Job Spooler - CSE 320 - Fall 2019
#### Professor Eugene Stark

### **Due Date: Friday 11/15/2018 @ 11:59pm**

## Introduction

The goal of this assignment is to become familiar with low-level Unix/POSIX system
calls related to processes, signal handling, files, and I/O redirection.
You will implement a job spooler program, called `jobber`, that accepts user
requests to spool tasks for execution, cancel jobs, pause and resume jobs,
show the status of jobs, and expunge terminated jobs from the system.

### Takeaways

After completing this assignment, you should:

* Understand process execution: forking, executing, and reaping.
* Understand signal handling.
* Understand the use of "dup" to perform I/O redirection.
* Have a more advanced understanding of Unix commands and the command line.
* Have gained experience with C libraries and system calls.
* Have enhanced your C programming abilities.

## Hints and Tips

* We **strongly recommend** that you check the return codes of **all** system calls
  and library functions.  This will help you catch errors.
* **BEAT UP YOUR OWN CODE!** Use a "monkey at a typewriter" approach to testing it
  and make sure that no sequence of operations, no matter how ridiculous it may
  seem, can crash the program.
* Your code should **NEVER** crash, and we will deduct points every time your
  program crashes during grading.  Especially make sure that you have avoided
  race conditions involving process termination and reaping that might result
  in "flaky" behavior.  If you notice odd behavior you don't understand:
  **INVESTIGATE**.
* You should use the `debug` macro provided to you in the base code.
  That way, when your program is compiled without `-DDEBUG`, all of your debugging
  output will vanish, preventing you from losing points due to superfluous output.

> :nerd: When writing your program, try to comment as much as possible and stay
> consistent with code formatting.  Keep your code organized, and don't be afraid
> to introduce new source files if/when appropriate.

### Reading Man Pages

This assignment will involve the use of many system calls and library functions
that you probably haven't used before.
As such, it is imperative that you become comfortable looking up function
specifications using the `man` command.

The `man` command stands for "manual" and takes the name of a function or command
(programs) as an argument.
For example, if I didn't know how the `fork(2)` system call worked, I would type
`man fork` into my terminal.
This would bring up the manual for the `fork(2)` system call.

> :nerd: Navigating through a man page once it is open can be weird if you're not
> familiar with these types of applications.
> To scroll up and down, you simply use the **up arrow key** and **down arrow key**
> or **j** and **k**, respectively.
> To exit the page, simply type **q**.
> That having been said, long `man` pages may look like a wall of text.
> So it's useful to be able to search through a page.
> This can be done by typing the **/** key, followed by your search phrase,
> and then hitting **enter**.
> Note that man pages are displayed with a program known as `less`.
> For more information about navigating the `man` pages with `less`,
> run `man less` in your terminal.

Now, you may have noticed the `2` in `fork(2)`.
This indicates the section in which the `man` page for `fork(2)` resides.
Here is a list of the `man` page sections and what they are for.

| Section          | Contents                                |
| ----------------:|:--------------------------------------- |
| 1                | User Commands (Programs)                |
| 2                | System Calls                            |
| 3                | C Library Functions                     |
| 4                | Devices and Special Files               |
| 5                | File Formats and Conventions            |
| 6                | Games, et al                            |
| 7                | Miscellanea                             |
| 8                | System Administration Tools and Daemons |

From the table above, we can see that `fork(2)` belongs to the system call section
of the `man` pages.
This is important because there are functions like `printf` which have multiple
entries in different sections of the `man` pages.
If you type `man printf` into your terminal, the `man` program will start looking
for that name starting from section 1.
If it can't find it, it'll go to section 2, then section 3 and so on.
However, there is actually a Bash user command called `printf`, so instead of getting
the `man` page for the `printf(3)` function which is located in `stdio.h`,
we get the `man` page for the Bash user command `printf(1)`.
If you specifically wanted the function from section 3 of the `man` pages,
you would enter `man 3 printf` into your terminal.

> :scream: Remember this: **`man` pages are your bread and butter**.
> Without them, you will have a very difficult time with this assignment.

## Getting Started

Fetch and merge the base code for `hw4` as described in `hw1`.
You can find it at this link: https://gitlab02.cs.stonybrook.edu/cse320/hw4

Here is the structure of the base code:
<pre>
├── .gitlab-ci.yml
└── hw4
    ├── demo
    │   └── jobber
    ├── include
    │   ├── debug.h
    │   ├── jobber.h
    │   ├── sf_readline.h
    │   └── task.h
    ├── lib
    │   └── sf_event.o
    ├── Makefile
    ├── src
    │   ├── jobs.c
    │   ├── main.c
    │   ├── sf_readline.c
    │   └── task.c
    └── tests
        └── hw4_tests.c
</pre>

If you run `make`, the code should compile correctly, resulting in an
executable `bin/jobber`.  If you run this program, it doesn't do very
much, because there is very little code -- you have to write it!

## `Jobber`: Functional Specification

### Overview

The `jobber` program implements a facility for spooling, executing, and managing *jobs*.
Each job has a *task* to run, once the job has been assigned a *runner* process.
There is a limit on the maximum number of runners that can be in existence at a given time,
so a job will only begin running if the current number of runners is less than the maximum.
Once a job has been started, it proceeds to execute its task until the task either completes
or aborts.  A running job can be *paused*; this causes execution of the job to be suspended
until the job has later been *resumed*.

A *task* is a description of work to be performed.  Tasks are specified using a syntax
that is similar to that understood by a command shell such as `bash`.
The following is an example of a task specification:

```
echo start ; cat /etc/passwd | grep bash > out ; echo done
```

A task consists of a sequence of one or more *pipelines* separated by semicolons (`;`).
In this example there are three pipelines:

* `echo start`
* `cat /etc/passwd | grep bash > out`
* `echo done`

Each pipeline consists of a sequence of one or more *commands* separated by vertical
bars (`|`), with optional *input redirection* and *output redirection* specifications.
In the example task the first and the third pipeline each consist of a single command
(`echo start` and `echo done`) with no redirection specifications, whereas the second
pipeline consists of two commands: `cat /etc/passwd` and `grep bash`, with the
output redirection `> out`.
Each command consists of a sequence of one or more *words*, where the first word in
a command is interpreted as the name of an executable program to be run and the
remaining words are interpreted as arguments.
A word is any consecutive sequence of non-delimiter characters, where for our purposes
delimiter characters are space characters, semicolons (`;`), vertical bars (`|`),
less-than signs (`<`), and greater-than signs (`>`).
An input redirection consists of a less-than sign (`<`) followed by a word, which is
interpreted as the pathname of a file from which the first command in the pipeline is
to take its input.
An output redirection consists of a greater-than sign (`>`) followed by a word, which is
interpreted as the pathname of a file to which output from the last command in the
pipeline is to be sent.

* Execution of a task is performed by executing each of its pipelines *sequentially*.
	  If a pipeline *completes*, then execution proceeds to the next pipeline in the task.
      If a pipeline *aborts*, then the entire task aborts without proceeding to any subsequent
      pipeline.

* Execution of a pipeline is performed by executing each of its commands *concurrently*,
	  with the standard input and output of each command *redirected* so that the output
      from one command becomes the input of the next command.  In addition, if the pipeline
      has an input redirection, then the specified file becomes the standard input of the
      first command in the pipeline and if the pipeline has an output redirection, then
      the specified file becomes the standard output of the last command in the pipeline.
      A pipeline *aborts* if any command in the pipeline terminates with a signal.
      Otherwise, the pipeline *completes*.

* Execution of a command is performed by using the first word of the command as the
	  name of an executable program, invoking that program, and passing to it an
	  argument vector consisting of the words that make up the command.
      A command *aborts* if it terminates with a signal, otherwise it *completes*.
      A command that completes yields an *exit status* which is propagated
      according to the following rules: the exit status of the last command in a pipeline
      becomes the exit status of the pipeline, and the exit status of the last pipeline
      in a task becomes the exit status of the task.

`Jobber` maintains a *jobs table* that keeps track of the existing jobs and their
current status.  When a job is initially spooled, it receives an entry in the jobs table.
Once the job has been started and has subsequently terminated, its final disposition
(completed or aborted, plus its exit status in case it completed) is recorded in
the jobs table.  At this point, it becomes possible to *expunge* the job from the
jobs table.  Expunging a job is a user-initiated operation causes a terminated job
to be removed from the jobs table.  The jobs table has a limited number of entries,
so expunging terminated jobs makes the entries the expunged jobs occupied available
for use by new jobs.

### Command-Line Interface

When started, `jobber` should present the user with a command-line
interface with the following prompt

```sh
jobber>
```

This prompt is to be printed to `stdout`, as is any other output
(job ID's, error messages, job status lines, *etc.*) specified below.

Typing a blank line after the prompt should should simply cause the prompt
to be repeated, without any other printout or action by the program.
Non-blank lines are interpreted as commands to be executed.
`Jobber` commands have a simple syntax, in which each command consists
of a sequence of "words" (which, except for the effect of single quotes --
see below) are sequences of non-whitespace characters,
separated by sequences of one or more whitespace characters.
The first word of each command is a keyword that names the command.
Any remaining words are the arguments to the command.
Your implementation of `jobber` should understand the following commands,
with arguments as indicated.

  * Miscellaneous commands
    * `help`
    * `quit`

  * Informational commands
    * `status` *job_number*
	* `jobs`

  * System control commands
    * `enable`
	* `disable`

  * Spooling commands
    * `spool` *task*
	* `pause` *job_number*
	* `resume` *job_number*
    * `cancel` *job_number*
    * `expunge` *job_number*

The `help` command takes no arguments, and it responds by printing a message
that lists all of the types of commands understood by the program, similar to
the following:

```
jobber> help
Available commands:
help (0 args) Print this help message
quit (0 args) Quit the program
enable (0 args) Allow jobs to start
disable (0 args) Prevent jobs from starting
spool (1 args) Spool a new job
pause (1 args) Pause a running job
resume (1 args) Resume a paused job
cancel (1 args) Cancel an unfinished job
expunge (1 args) Expunge a finished job
status (1 args) Print the status of a job
jobs (0 args) Print the status of all jobs
jobber> 
```

The `quit` command takes no arguments and causes execution to terminate.
Before termination, the program should cancel all jobs that have not terminated.
It should wait for all jobs to terminate, and then it should expunge all jobs
from the jobs table in order to free any memory that they were using before
finally exiting with status `EXIT_SUCCESS`.

The `enable` command sets a flag that allows spooled jobs to be started and immediately
starts any spooled jobs that have not been started, up to the limitation on the maximum
number of jobs that can be active at once.  Note that initially, starting jobs
is **not** enabled.

The `disable` command clears the flag that allows spooled jobs to be started.
No jobs will subsequently be started until the `enable` command has been given.

The `spool` command spools a new job.
For example:
```
spool 'echo start ; cat /etc/passwd | grep bash > out ; echo done'
```
Here single quote characters (`'`) are used to treat as a single word what would otherwise
be interpreted as multiple words.  Thus, the above command consists only of two words:
`spool` and `echo start; cat /etc/passwd | grep bash > out ; echo done`.
Note that the quote characters themselves do not become part of a word.
The second word in the `spool` command specifies the task that the job is to perform.
The `spool` command assigns a slot in the job table to the new job and it responds to the
user by printing a single line that contains the *job ID* of the newly spooled job.
The job ID is the integer index of the slot in the job table that was assigned to the job,
and it is used to identify the job in other commands.
If there are no available job slots, the job is not created, and an error is reported
to the user.

The `pause` command causes a job (specified by its job ID) that has started running to be suspended.
Suspending a job is achieved by sending a `SIGSTOP` signal to the job runner's process group.
If the specified job ID is not the ID of an existing job, or the job is not currently running,
then an error is reported to the user.

The `resume` command causes a paused job (specified by its job ID) to continue execution.
Resuming a job is achieved by sending a `SIGCONT` signal to the job runner's process group.
If the specified job ID is not the ID of an existing job, or the job is not currently paused,
then an error is reported to the user.

The `cancel` command marks a job (specified by its job ID) as "canceled" and attempts to
forcibly terminate it.
If the job has not yet been started, it is simply no longer eligible to be started.
If the job has already been started, it is killed by sending a `SIGKILL` signal
to the job's process group.
If the specified job ID is not the ID of an existing job, or the job has already terminated,
then the job cannot be canceled, and an error is reported to the user.

The `expunge` command removes a job (specified by its job ID) that has terminated from the
jobs table.
If the specified job ID is not the ID of an existing job, or the job has not yet terminated,
then an error is reported to the user.

The `status` command prints the current status of a job (specified by its job ID);
for example:
```
jobber> status 3
job 3 [waiting]: echo start ; cat /etc/passwd | grep bash > out ; echo done
jobber>
```
Here the output shows the job number of the job, its current status
(more on job statuses a bit later) and the job's task specification.
If the job has completed, then its exit status will also be shown; for example:
```
jobber> status 3
job 3 [completed (0)]: echo start ; cat /etc/passwd | grep bash > out ; echo done
jobber>
```
If the specified job ID is not the ID of an existing job,
then an error is reported to the user.

The `jobs` command prints a report on the current status of *all* existing jobs in the jobs table;
for example:
```
job 1 [aborted]: foo
job 2 [completed (0)]: sleep 30
job 3 [waiting]: echo start; cat /etc/passwd | grep bash > out ; echo done
```

### Job Status

The status of a job will be one of the following:
`NEW`, `WAITING`, `RUNNING`, `PAUSED`, `CANCELED`, `COMPLETED`, `ABORTED`.
Transitions between these states occur according to the following rules:

* A newly created job starts out in the `NEW` state.
* Once a new job has been fully initialized, it transitions to the `WAITING` state.
* A job that is `WAITING` may be assigned a runner process and set to the
    `RUNNING` state if the current number of runners is less than `MAX_RUNNERS`.
* The user may choose to pause a `RUNNING` job, in which case the job status
    is set to `PAUSED` and the runner's process group is sent a `SIGSTOP` signal.
* The user may choose to resume a `PAUSED` job, in which case the job status
    is set to `RUNNING` and the runner's process group is sent a `SIGCONT` signal.
* A job whose task exits normally is set to the `COMPLETED` state.
* A job whose task terminates as a result of a signal is set to the `ABORTED` state.
* The user may choose to cancel a job that is in any state other than
    `CANCELED`, `COMPLETED`, or `ABORTED`.
	If the job has not already started, then its status is simply set to `ABORTED`.
	If the job has already started, then the job status is set to `CANCELED` and
	the runner's process group (if the job has already started) is sent a `SIGKILL` signal.
    When the job subsequently terminates, then its state is changed from `CANCELED`
    to `COMPLETED` or `ABORTED`, according to how termination occured.
    Note that it is still possible for a job that has been canceled to end up in
    the `COMPLETED` state, if the job completed just after the job state was set
    to `CANCELED` but before the `SIGKILL` was delivered to the job runner's process group.

Note that a job may only be expunged if it is in the `COMPLETED` or `ABORTED` state.

### Signal Handling

The `jobber` program must install a `SIGCHLD` handler so that it can be notified
immediately upon termination of a job runner process.  The handler must appropriately
update the job status information and start any further jobs in the jobs table that
can now be processed due to the fact that an existing runner has terminated.

  > :nerd: Note that you will need to use `sigprocmask()` to block signals at
  > appropriate times, to avoid races between the handler and the main program,
  > the occurrence of which which will result in indeterminate behavior.

The handler you install must be async-signal-safe.  Some technicalities regarding
the proper way to implement your handler is discussed below.

### Reading Input

When a process reads user input, it will normally have to *block* (wait) until
input becomes available.  If a signal arrives while the process is blocked, then
the `read()` system call will be interrupted, any handler that has been installed
for that signal will run, and then the `read()` call will return with a "short count".
This is all fine, as long as the signal handler can take care of everything that
needs to be done regarding the signal that has arrived.  However, as it is not safe
to call many useful functions inside a signal handler, it is often not possible for
a handler to do more than to simply set a flag indicating that a signal has arrived,
and to arrange for a separate function to be called to check the flag and handle
the signal later from the context of the main program once the actual handler has
returned and there are no restrictions on the functions that can be called safely.
This creates a problem, though, of how to and when to arrange for this separate function
to be called.  We would want to call this function before blocking for user input,
otherwise it is possible that handling of a signal could be delayed for an arbitrarily
long time until input has been provided by the user.  On the other hand, if we simply
call this separate function immediately before blocking for user input, there will always
be a short window of time during which the arrival of a signal can come too late
to be noticed before blocking occurs.  This race condition could be avoided if we were
to mask signals, call the separate function to handle signals that have already arrived,
and then block for user input, but we can't exactly do that because then signals
would be masked for the entire time that the process is waiting for user input,
again making the program unable to respond to signals in a prompt fashion.
What we need to be able to do is to mask signals, handle signals that have already
arrived, and then *atomically* unmask signals and block for input.
The system call `pselect()` ("POSIX select") provides this capability.
However, even using `pselect()`, to write a correct, race-free function that can
perform user input and yet respond promptly to signals is still somewhat tricky,
so I have provided you with such a function, called `sf_readline()`.

You **must** use the provided function `sf_readline()` as the method of
reading input from the user.  The interface that this function provides
is essentially that of a simplified version of the GNU `readline()` function,
but you **must not use GNU** `readline()` because the implementation of that function
does not properly support race-free, safe, prompt signal handling.
The `sf_readline()` function will call back to a handler function that you specify
in order to handle signals just before blocking to await user input.
The handler function should be specified during program initialization by calling the
function `sf_set_readline_signal_hook()` and passing the handler function as a parameter.
Blocking within `sf_readline()` is done using `pselect()` as described above to eliminate
the short window during which a signal could arrive but not be processed until after
user input has been provided.
Your actual signal handlers (installed using `signal()` or `sigaction()`) should
be async-signal-safe functions that simply set a flag indicating that a signal
has arrived.  The actual handling of the signals should be done in the separate
handler function that you implement, which will be called back from `sf_readline()`.

### Parsing Task Specifications

One of the functions that `jobber` needs to perform is to parse task specifications
input by the user, into a form that can be used by the runner process to execute
the task.  Although it is an important skill to be able to write simple parsers like
the one required here, it is not the main point of the assignment and from previous
experience I have found that students tend to get waylaid by it and don't get
very much done on what is the important part of the assignment; namely, process handling.
For this reason, as well as for reasons of uniformity among the assignment submissions,
I have provided a function `parse_task()` which you **must** use to parse task
specifications input by the user.
The `parse_task()` function is a simple recursive-descent parser that takes as a
parameter a pointer to a variable containing a `(char *)` pointer to the specification
string to be parsed.  It attempts to interpret the string as the specification of
a task and it returns a pointer to a `TASK` structure that represents the task.
If it cannot interpret the string as a `TASK`, then `NULL` is returned.
Note that `parse_task()` will modify both the variable that is passed, as well as the
specification string that it points to, so copies must be made if unmodified versions
are required after the call to `parse_task()`.
The `TASK` object that is returned by `parse_task()` must be freed by the caller using
the provided function `free_task()` when the object is no longer needed.

### Executing a Job

To execute a job, you should first create a *runner* process for that job.
As discussed above, there is a limit to the number of runner processes that
are permitted to exist at one time, so any given job might have to spend some time
in the `WAITING` state until the number of existing runners has dropped below the limit.
The objective of the runner process is to execute the job's *task*,
by arranging for the pipelines that make up that task to be executed in sequence,
noticing (using `wait(2)` or `waitpid(2)`) whether each pipeline completes or aborts,
and then itself either exiting (using `exit(3)`) or aborting (using `abort(3)`);
in the former case using the exit status of the last pipeline as its own exit status.
The runner process should use `setpgid(2)` to create a new process group,
with a process group ID equal to its process ID.
This will later enable `killpg(2)` to send signals to the process group to
cause the task to be stopped, continued, or killed.
The main program should make use of `SIGCHLD` signals to notice the termination of runner
processes and use `waitpid(2)` to reap them.

In order to execute a pipeline, the runner process should create a
*pipeline master* process for that pipeline.
The pipeline master process will then create one child process for each command
in the pipeline,
arranging as it does so for the output of each command to be directed
via a *pipe* (see `pipe(2)` and `dup(2)`) to the input of the next command in the pipeline.
Each command process should use `execvp(3)` to execute its corresponding command.
In addition, the pipeline master will arrange for the first command in the
pipeline to take its input from any file that has been specified with an
*input redirection* (see `open(2)` and `dup(2)`),
and it will arrange for the last command in the pipeline
to send its output to any file that has been specified with an
*output redirection*.
If any command process in the pipeline aborts (terminates with a signal),
then the entire pipeline is determined to have aborted, and the pipeline master
will itself abort (again using `abort(3)`).
Any error in setting up the pipeline itself will also be considered reason for
the pipeline master to abort.
If all command processes in the pipeline complete (exit normally), then
the entire pipeline is determined to have completed, and the exit status of the
last command in the pipeline will be used as the exit status (using `exit(3)`)
of the pipeline master.

  > **Important:**  You **must** create the processes in a task using
  > calls to `fork()` and `execvp()`.  You **must not** use the `system()` function,
  > nor use any form of shell in order to create the pipeline, as the purpose of
  > the assignment is to giving you experience with using the system calls involved
  > in doing this.

### Batch Mode

The normal mode of operation of `jobber` is as an interactive, command-line application.
However, it should also work correctly in batch mode, in which standard input
has been redirected from a command file, or even from a pipe.  You should check that
your program does indeed function correctly in this way, because we will
use this to either feed your program a predefined command sequence or even
to run your program in parallel with a driver program that issues commands
and tracks what is happening using the event functions described above.

### Functions You Must Implement

As part of your implementation of `jobber`, you **must** provide implementations for
the following functions, for which more detailed specifications are given in `jobber.h`:

* `int jobs_init(void)` -- Initialize the job spooler.
* `void jobs_fini(void)` -- Finalize the job spooler.
* `int jobs_set_enabled(int enabled)` -- Set whether jobs can be started.
* `int jobs_get_enabled(void)` -- Get whether jobs can be started.
* `int job_create(char *task)` -- Create a new job.
* `int job_expunge(int jobid)` -- Expunge a job.
* `int job_cancel(int jobid)` -- Attempt to cancel a job.
* `int job_pause(int jobid)` -- Pause a running job.
* `int job_resume(int jobid)` -- Resume a paused job.
* `JOB_STATUS job_get_status(int jobid)` -- Get the current status of a job.
* `int job_get_pgid(int jobid)` -- Get the process group ID of a job's runner process.
* `int job_get_result(int jobid)` -- Get the result (*i.e.* the exit status) of a job.
* `int job_was_canceled(int jobid)` -- Determine if a job was successfully canceled.
* `char *job_get_taskspec(int jobid)` -- Get the task specification of a job.

It is important that you adhere exactly to the specifications given, because
some of the tests we use in grading will likely be unit tests that call these functions
directly.

### Event Functions You Must Call

In order to make it possible for us to trace the actions of your program,
we have provided several functions that you **must** call in particular situations.
These are:

* `void sf_job_create(int jobid);`
        You **must** call this function when a new job is created, before the status
	    of the job has changed from `NEW`.  The argument is the job ID of the new job.
 * `void sf_job_expunge(int jobid);`
        You **must** call this function right after a job has been expunged.
	    The argument is the job ID of the job that has been expunged.
* `void sf_job_status_change(int jobid, JOB_STATUS old, JOB_STATUS new);`
	    You **must** call this function any time the status of a job changes.
        Arguments are the job ID of the job that is changing status, the old status
	    of the job and the new status.
* `void sf_job_start(int jobid, int pgid);`
	    You **must** call this function when a job begins execution.
		Arguments are the job ID of the job that is being started and the
		process group ID of the runner assigned to the job.
* `void sf_job_end(int jobid, int pgid, int result);`
	    You **must** call this function at the time a runner process is reaped
		upon its termination.
		Arguments are the job ID of the job that has terminated,
        the process group ID of the runner assigned to the job,
		and the exit status of the runner process, as returned by `waitpid()`.
* `sf_job_pause(int jobid, int pgid);`
	    You **must** call this function when receipt of a `SIGCHLD` signal
		indicates that a job has been paused.
		Arguments are the job ID of the job that is being started and the
		process group ID of the runner assigned to the job.
* `sf_job_resume(int jobid, int pgid);`
	    You **must** call this function when receipt of a `SIGCHLD` signal
		indicates that a job has been resumed.
		Arguments are the job ID of the job that is being resumed and the
		process group ID of the runner assigned to the job.

The above functions are provided to you as an object file that will be linked with
your program.  As implemented in the basecode, these functions will announce
on `stderr` that they have been called, so that you can verify that you are
calling them properly.  Should you desire not to see this printout, you can
set the global variable `sf_suppress_chatter` to a nonzero value.
When we grade your program, we will replace the basecode implementations of
these functions with different versions that will allow us to test whether
events are actually occurring in the order they are supposed to be.

## Provided Components

### The `jobber.h` Header File

The `jobber.h` header file that we have provided defines the limits on the number
of jobs and runners that can exist at one time, defines the possible states that
a job can be in, gives function prototypes for the functions you are to use to
report the occurrence of job status changes and other events, and gives function
prototypes for the functions that you are to implement.

  > :scream: **Do not make any changes to `jobber.h`.  It will be replaced
  > during grading, and if you change it, you will get a zero!**

### The `sf_readline.h` Header File and `sf_readline.c` Source File

The `sf_readline.h` header file gives function prototypes for the function
`sf_readline()`, which you are to use to read user input, and the function
`sf_set_readline_signal_hook()`, which you are to use to inform `sf_readline()`
about the signal handling function that you write.  In addition, a related
function type `signal_hook_func_t` is defined.
The `sf_readline.c` source file contains implementations of the functions
whose prototypes are given in `sf_readline.h`.

  > :scream: **Do not make any changes to `sf_readline.h` or `sf_readline.c`.
  > They will be replaced during grading, and if you change them, you risk
  > getting a zero!**

### The `task.h` Header File and `task.c` Source File

The `task.h` header file gives function prototypes for the functions
`parse_task()`, which you should use to parse task descriptions entered
by the user, `unparse_task()`, which you may use to print out a task
specification, and a function `free_task()` which you must use to free
a `TASK` object returned by `parse_task()` when it is no longer required.
The `task.h` header file also contains type definitions for types
`TASK`, `PIPELINE`, `PIPELINE_LIST`, `COMMAND`, `COMMAND_LIST`, and
`WORD_LIST`, which are used to represent the parsed tasks returned by
`parse_task()`.  You will need to refer to these type definitions in order
to traverse a task and execute it.

  > :scream: **Do not make any changes to `task.h` or `task.c`.
  > They will be replaced during grading, and if you change them, you risk
  > getting a zero!**

### The `sf_event.o` Object File

The `sf_event.o` object file contains implementations of the functions that
you are to call upon occurrence of various events (see *Event Functions* above).
This file will be linked with your program.  As indicated above, the basecode
implementation simply prints a message on `stderr` each time one of the functions
is called.  During grading, we will replace this implementation with a different
one, which will help us to automatically track the actions of your program.

### The `demo/jobber` Executable

To help answer questions about what you are expected to implement, I have
included a demonstration version of the program as `demo/jobber`.
You should be able to run it simply by typing `demo/jobber`.
If `git` does not preserve the file permissions, it might be necessary to
set execute permission on it using the command `chmod u+x demo/jobber`.

  > :scream:  The behavior of the demo program should be regarded as an example
  > implementation only, not a specification.  If there should be any discrepancy
  > between the behavior of the demo program and what it says either in this document
  > or in the specifications in the header files, the latter should be regarded
  > as authoritative.

## Hand-in instructions
As usual, make sure your homework compiles before submitting.
Test it carefully to be sure that doesn't crash or exhibit "flaky" behavior
due to race conditions.
Use `valgrind` to check for memory errors and leaks.
Besides `--leak-check=full`, also use the option `--track-fds=yes`
to check whether your program is leaking file descriptors because
they haven't been properly closed.
You might also want to look into the `valgrind` `--trace-children` and related
options.

Submit your work using `git submit` as usual.
This homework's tag is: `hw4`.
