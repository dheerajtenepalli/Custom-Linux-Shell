/**
 * === DO NOT MODIFY THIS FILE ===
 * If you need some other prototypes or constants in a header, please put them
 * in another header file.
 *
 * When we grade, we will be replacing this file with our own copy.
 * You have been warned.
 * === DO NOT MODIFY THIS FILE ===
 */

/*
 * This is a very basic implementation of a "readline" function.
 * We would use GNU readline, except for the fact that it is impossible to
 * use GNU readline together with properly written signal handling code.
 * The version defined here arranges for race-free callbacks to a
 * client-specified function before blocking for user input.
 * This permits the actual signal handlers to be written in a safe manner
 * in which they just set flags to indicate that signals have arrived,
 * and the actual work of dealing with the signals to postponed to the
 * callback function where there are no constraints on what functions can
 * be safely used.
 *
 * Author: E. Stark, 10/2019.
 */
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>

#include "../include/sf_readline.h"
#include "../include/debug.h"

static signal_hook_func_t *signal_hook_func;

void sf_set_readline_signal_hook(signal_hook_func_t func) {
    signal_hook_func = func;
}

#define BUFSIZE 32  // Initial buffer size.
char *sf_readline(char *prompt) {
    int size = BUFSIZE;
    char *buf = malloc(size);
    if(buf == NULL) {
	debug("malloc failed");
	return NULL;
    }
    char *bp = buf;
    fprintf(stdout, "%s", prompt);
    fflush(stdout);
    bp = buf;
    while(1) {
	char c;
	// Here we mask all signals, call a client-specified handler function (if any)
	// to deal with any signals that have already been noticed, then atomically
	// unmask signals and block until stdin is ready for reading.
	sigset_t mask, omask;
	sigfillset(&mask);
	sigprocmask(SIG_BLOCK, &mask, &omask);
	if(signal_hook_func)
	    signal_hook_func();
	// Use pselect() to atomically release signals and wait for input to become
	// available.
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(0, &readfds);
	int n = pselect(1, &readfds, NULL, NULL, NULL, &omask);
	int e = errno;
	// OK to unmask signals now that we know there is input to read and we
	// won't block trying to get it.
	sigprocmask(SIG_SETMASK, &omask, NULL);
	if(n < 0) {
	    if(e == EINTR)
		continue;  // pselect() was interrupted by a signal, restart
	    debug("select failed (errno = %d)", e);
	    return NULL;
	} else if(n == 0) {
	    if(bp - buf > 0) {
		debug("EOF on input -- treating as newline");
		break;
	    } else {
		debug("EOF on input");
		free(buf);
		return NULL;
	    }
	} else {
	    // Read one character.  That's all we can do without a risk of blocking.
	    if(read(0, &c, 1) != 1) {
		if(bp - buf > 0) {
		    debug("read returned <= 0 -- treating as newline");
		    break;
		} else {
		    debug("read returned <= 0 -- treating as EOF");
		    free(buf);
		    return NULL;
		}
	    }
	    if(c == '\n')
		break;
	    if(bp - buf >= size -1) {
		size <<= 1;
		debug("realloc: %d", size);
		char *nbuf = realloc(buf, size);
		if(nbuf == NULL) {
		    debug("realloc failed");
		    break;
		} else {
		    bp = nbuf + (bp - buf);
		    buf = nbuf;
		}
	    }
	    *bp++ = c;
	}
    }
    *bp = '\0';
    return buf;
}
