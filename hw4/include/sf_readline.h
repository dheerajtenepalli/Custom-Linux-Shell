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
 * The following function prints a prompt on stdout and collects an arbitrarily
 * long line from stdin, similarly to GNU readline.  The string that is returned does
 * not contain the line terminator '\n', so that inputting a blank line returns the
 * empty string.  An EOF condition on stdin is treated as if it were a newline,
 * except in case the input line is empty, in which case NULL is returned.
 * The string that is returned is allocated with malloc, and it is the client's
 * responsibility to free it.
 */
char *sf_readline(char *prompt);

/*
 * The following function can be used to install a pointer to a function
 * to be called (with signals masked) just before blocking for user input.
 * This permits the client to install safe signal handlers that just set flags,
 * knowing that there will be a chance to perform the real work of handling
 * the signals in a prompt fashion, outside of an actual signal handler context.
 *
 * (NOTE: I would have thought that the similar variable rl_signal_event_hook that
 * was recently added to GNU readline was intended for a similar purpose,
 * but examination of the GNU readline source code shows that it is broken
 * due to inherent races.)
 */
typedef int signal_hook_func_t (void);
void sf_set_readline_signal_hook(signal_hook_func_t func);
