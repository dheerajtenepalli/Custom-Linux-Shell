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
 * Syntax of a task:
 * A task consists of a sequence of one or more "pipelines", separated by semicolons ;.
 * A pipeline consists of a sequence of one or more "commands", separated by vertical bars |
 *   and optionally followed by one or both of an "input redirection" and an "output redirection",
 *   either order permitted.
 * A command consists of a sequence of "words", separated by whitespace characters.
 *   The first word of a command is interpreted as the name of an executable program and the
 *   remaining words are interpreted as arguments to be passed.
 * An input redirection consists of the character '<' followed by a word, which is interpreted
 *   as the name of a file from which input is to be taken.
 * An output redirection consists of the character '>' followed by a word, which is interpreted
 *   as the name of a file to which output is to be sent.
 * A word is a sequence of consecutive non-delimiter characters, where delimiter characters
 *   comprise whitespace, semicolons ;, vertical bars |, less-than <, and greater-than >.
 * Optional whitespace is permitted around the delimiters ; | < and >.
 *
 * The following definitions describe a linked-list representation of commands, pipelines,
 * and tasks.
 */

typedef struct WORD_LIST {
  char *first;
  struct WORD_LIST *rest;  // NULL indicates the end of the list.
} WORD_LIST;

typedef struct COMMAND {
  WORD_LIST *words;
} COMMAND;

typedef struct COMMAND_LIST {
  COMMAND *first;
  struct COMMAND_LIST *rest;   // NULL indicates the end of the list.
} COMMAND_LIST;

typedef struct PIPELINE {
  COMMAND_LIST *commands;
  char *input_path;   // Either a pathname or NULL.
  char *output_path;  // Either a pathname or NULL.
} PIPELINE;

typedef struct PIPELINE_LIST {
    PIPELINE *first;
    struct PIPELINE_LIST *rest;  // NULL indicates the end of the list.
} PIPELINE_LIST;

typedef struct TASK {
    PIPELINE_LIST *pipelines;
} TASK;

/*
 * @brief  Parse a string, obtaining a task.
 * @details  This function takes a pointer to a variable that points to a string
 * to be parsed as a task and returns a pointer to the parsed task.
 * @param linep  A pointer to a variable that points to the string to be parsed.
 * Both the pointer itself and the string that it points to will in general be modified
 * by this function.
 * @return  A pointer to the parsed task, if parsing is successful, otherwise NULL.
 */
TASK *parse_task(char **linep);

/*
 * @brief  Print a string representation of a parsed task to the specified stream.
 * @details  This function takes a pointer to a parsed task and an output stream
 * and it prints a string representation of the task on the output stream in the format
 * that is understood by parse_task.
 * @param tp  The parsed task whose representation is to be printed.
 * @param out  The output stream on which to print.
 */
void unparse_task(TASK *tp, FILE *out);

/*
 * @brief  Frees a parsed task.
 * @param tp  The parsed task to be freed.
 */
void free_task(TASK *tp);
