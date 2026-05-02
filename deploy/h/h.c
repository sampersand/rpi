#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>

#ifndef HAS_GETOPT_LONG
# if defined(_GNU_SOURCE) || defined(__GNU_LIBRARY__) || defined(__GLIBC__) /* glibc (Linux) */ \
  || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__) /* BSDs */ \
  || defined(__APPLE__)/* macOS */
# define HAS_GETOPT_LONG
# endif
#elif HAS_GETOPT_LONG == 0
# undef HAS_GETOPT_LONG
#endif

/**************************************************************************************************
 *                             Global Variables and Helper Functions                              *
 **************************************************************************************************/

const char *inline_elements[] = {
    "a", "abbr", "acronym", "b", "bdi", "bdo", "big", "button",
    "cite", "code", "em", "i", "kbd", "label", "mark", "q",
    "s", "samp", "select", "small", "span", "strong", "sub", "sup",
    "textarea", "time", "u", "var", NULL
};

// Returns whether an html element represented by `str` is a text node, and thus
// needs spaces when outputting in compact mode
bool is_text_node(const char *str) {
	if (str == NULL) return false;
	for (const char *p = inline_elements[0]; *p; ++p)
		if (!strcmp(str, p)) return true;
	return false;
}

// Controlled by the `COMPACT` env var. If set, no extraneous whitespace is printed
int inline_mode;
bool is_very_first_element = true;

// The `argc` and `argv` from `main()` (set here so others can use them)
int argc;
char *const *argv;

// Aborts with a usage message
#define die(...) do { fprintf(stderr, "%s: ", argv[0]), \
				  fprintf(stderr, __VA_ARGS__), \
				  fputc('\n', stderr), \
				  exit(EXIT_FAILURE); } while (0)

// Prints a file to stdout, aborting if there's a problem
void cat_FILE(FILE *f, bool chomp_last) {
    char buf[4096];
    size_t n;
    int prev = -1;  // last byte of previous chunk, or -1 if none

    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        if (prev != -1) fwrite(&(char){prev}, 1, 1, stdout);
        prev = (unsigned char)buf[n - 1];
        fwrite(buf, 1, n - 1, stdout);
    }

    // now prev holds the very last byte
    if (prev != -1 && !(chomp_last && prev == '\n'))
        fwrite(&(char){prev}, 1, 1, stdout);
}

void cat_file(const char *file) {
    FILE *f = fopen(file, "r");
    if (!f)
    	die("cannot cat %s: %s", file, strerror(errno));
    cat_FILE(f, false);
    fclose(f);
}

void execute_command(const char *cmd) {
	FILE *p = popen(cmd, "r");
	if (!p)
    	die("cannot execute %s: %s", cmd, strerror(errno));
   	cat_FILE(p, true);

    int status = pclose(p);
    if (status == -1)
        die("pclose failed for '%s': %s", cmd, strerror(errno));
    else if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
        die("'%s' exited with status %d", cmd, WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        die("'%s' killed by signal %d", cmd, WTERMSIG(status));
}


/**************************************************************************************************
 *                                         HTML Elements                                          *
 **************************************************************************************************/

// An HTML element
struct element {
	const char *name;      // The name of the element (eg `h1`, `div`, etc)
	char *attributes;      // Attributes for the element (`style=`, `href=`, ...). malloc'd.
	unsigned short indent; // How deeply indented the element is
	bool compact;          // Whether we emit the smallest required whitespace
	bool is_text_node, prev_is_text_node;
};

// Prints the leading indentation for the element
void print_indent(const struct element *ele) {
	if (inline_mode) {
		inline_mode = false;
		return;
	}

	if (ele->compact) {
		if (ele->prev_is_text_node /*&& (ele->name && !(is_text_node(ele->name)))*/)
			putchar(' ');
	} else {
		if (is_very_first_element)
			is_very_first_element = false;
		else
			putchar('\n');

		for (unsigned i = 0; i < ele->indent; ++i)
			putchar('\t');
	}
}

enum closing { OPENING_ELE, CLOSING_ELE };
enum indent { NO_INDENT, INDENT };
#define print_current_element(...) print_element(&current_element, __VA_ARGS__)
void print_element(struct element *ele, enum closing closing, enum indent indent) {
	assert(ele->name);
	if (indent == INDENT)
		print_indent(ele);

	if (!strcmp(ele->name, "@")) return; // TODO: is this still useful?

	if (closing == CLOSING_ELE) {
		printf("</%s>", ele->name);
	} else if (!strcmp(ele->name, "DOCTYPE")) {
		system("header Content-Type text/html"); // TODO: abort if this fails
		fputs("<!DOCTYPE html>", stdout);
	} else {
		printf("<%s", ele->name);
		if (ele->attributes) printf(" %s", ele->attributes);
		putchar('>');
	}
}

/**************************************************************************************************
 *                                       The Element Stack                                        *
 **************************************************************************************************/

// The element stack. This is used when nesting elements via `[ ... ]`, eg `div [ span [ ... ] ]`
#ifndef ELEMENT_STACK_SIZE
# define ELEMENT_STACK_SIZE 10000
#endif
struct element element_stack[ELEMENT_STACK_SIZE];
unsigned element_stack_len = 0;
#define current_element (element_stack[element_stack_len])

static bool has_current_element(void) {
	return current_element.name != NULL;
}

// Overwrites the current element with a new one
void set_current_element(const char *name) {
	if (current_element.name) {
		current_element.prev_is_text_node = current_element.is_text_node;
	}

	current_element.name = name;
	free(current_element.attributes); // NOTE: this is OK even when `attributes` are null.
	current_element.attributes = NULL;

	current_element.is_text_node = is_text_node(name);
}

// Pushes the current stack, adding a new element on top. Used by `[`.
void push_stack(void) {
	if (element_stack_len++ > ELEMENT_STACK_SIZE) {
		die("too many nested `[` encountered (%d max)", ELEMENT_STACK_SIZE);
	}

	set_current_element(NULL);
	current_element.indent = element_stack[element_stack_len - 1].indent + 1;
	current_element.compact = element_stack[element_stack_len - 1].compact;
	current_element.prev_is_text_node = false;
}

// Clears the current element from the top of the stack. Returns whether the
// popped element was in "compact mode"
bool pop_stack(void) {
	assert(element_stack_len);
	free(current_element.attributes); // make sure we don't have dangling memory

	return element_stack[element_stack_len--].compact;
}

/**************************************************************************************************
 *                                          Program Loop                                          *
 **************************************************************************************************/

_Noreturn void usage(bool is_error) {
	FILE *out = is_error ? stderr : stdout;

	fprintf(out,
		"usage: %s [options | html-elements]\n"
		"summary: prints out an HTML element,possibly with nested subelements.\n"
		"         each new non-flag option introduces a new element to the scope.\n"
		"         children nodes inherit parent node flags, except -I which is incremented.\n"
		"\n"
		"options: \n"
		"   -c, --compact            only print required whitespace (also: $COMPACT)\n"
		"   -i, --inline             suppress whitespace before this element\n"
		"   -I, --indent=LEVEL       sets indentation level (relative: +N or -N) (also: $INDENT)\n"
		"   -a, --attribute=ATTR     add an HTML attribute to the current element\n"
		"   -A, --clear-attributes   clears all attribute for the current element\n"
		"   -t, --text=TEXT          inserts <ele>TEXT</ele> (repeatable)\n"
		"   -T, --inline-text=TEXT   inserts TEXT without an HTML wrapper\n"
		"   -f, --file=FILE          like -t, but read content from FILE\n"
		"   -F, --inline-file=TEXT   like -T, but read content from FILE\n"
		"   -x, --execute=CMD        like -t, but executes CMD\n"
		"   -X, --inline-execute=CMD like -T, but executes CMD\n"
		"\n"
		"nested elements use square brackets: div [ strong -t hello ]\n"
		"\n"
		"note: in compact mode, whitespace is still inserted before inline elements\n"
		"      (strong, span, a, etc.) unless -i is given.\n",
	argv[0]);

	exit(is_error ? EXIT_FAILURE : EXIT_SUCCESS);
}

int getopt_possibly_long(void) {
	#define GETOPT_STRING "ht:T:f:F:a:AcCiI:x:"
#ifdef HAS_GETOPT_LONG
	// TODO
	static struct option longopts[] = {
	    { "help",           no_argument,        NULL,   'h' },
	    { "text",           required_argument,  NULL,   't' },
	    { "inline-text",    required_argument,  NULL,   'T' },
	    { "attribute",      required_argument,  NULL,   'a' },
	    { "clear-attribute",no_argument,        NULL,   'A' },
	    { "compact",        no_argument,        NULL,   'c' },
	    { "no-compact",     no_argument,        NULL,   'C' },
	    { "indent",         required_argument,  NULL,   'I' },
	    { "execute",        required_argument,  NULL,   'x' },
	    { "inline=execute", required_argument,  NULL,   'X' },
	    { "include-file",   required_argument,  NULL,   'f' },
	    { "inline-file",    required_argument,  NULL,   'F' },
	    { "inline",         no_argument,        NULL,   'i' },
	    { NULL,             0,                  NULL,    0  },
	};

	return getopt_long(argc, argv, "+" GETOPT_STRING, longopts, NULL);
#else
	return getopt(argc, argv, GETOPT_STRING);
#endif
}

enum status {
	NO_MORE_OPTIONS,
	END_NESTED_ELEMENT
};

enum status run_program(void) {
	int opt;
	bool was_printed = false;
	char *nonflag_arg;
	enum status status = NO_MORE_OPTIONS;

	// Loop while there's still stuff left to be read.
	while ( optind < argc ) {
		// Fetch the current option.
		//
		// We have a Frankenstein-esque approach to using `getopt` here, where instead of just
		// parsing all options up front, we instead use non-option values as HTML elements, and then
		// flags modify those HTML elements.
		switch ((opt = getopt_possibly_long())) {

		// An option of `-1` means "end of option parsing"--either because we're at the end of
		// argv, or because a non-option arg was encountered
		case -1:
			// Extract the arg, and increment `optind` by one, so that the next time `getopt`
			// runs, we don't see the same option.
			nonflag_arg = argv[optind++];

			// If it's NULL, that means we're at end of argument parsing.
			if (!nonflag_arg) {
				status = NO_MORE_OPTIONS;
				goto done;
			}

			/******************************************************************
			 *                 Nested HTML Element Arguments                  *
			 ******************************************************************/

			// An `[` begins a nested group; we print out an opening tag,
			// then all elements until a matching `]` are printed out, then a
			// closing tag.
			if (!strcmp(nonflag_arg, "[")) {
				if (!has_current_element())
					die("cannot nest when there's no active element");
				print_current_element(OPENING_ELE, INDENT);

				push_stack();
				enum status child_status = run_program();
				enum indent indent = pop_stack() ? NO_INDENT : INDENT;

				if (child_status != END_NESTED_ELEMENT)
					die("missing closing ] for %s", current_element.name);
				print_current_element(CLOSING_ELE, indent);
				was_printed = true;
				break;
			}

			// If we encounter a `]`, that's the end of a nested group.
			if (!strcmp(nonflag_arg, "]")) {
				status = END_NESTED_ELEMENT;
				goto done;
			}

			// An `[]` just prints out an empty body. Short-hand for `[ ]`
			if (!strcmp(nonflag_arg, "[]")) {
				if (!has_current_element())
					die("cannot nest when there's no active element");
				print_current_element(OPENING_ELE, INDENT);
				print_current_element(CLOSING_ELE, INDENT);
				was_printed = true;
				break;
			}

			/******************************************************************
			 *                        Normal HTML Tags                        *
			 ******************************************************************/

			if (has_current_element() && !was_printed) {
				print_current_element(OPENING_ELE, INDENT);
			}

			set_current_element(nonflag_arg);
			was_printed = false;
			break;

		case 'h':
			usage(false);

		case 'i':
			inline_mode = true;
			break;

		case 'c':
			current_element.compact = true;
			break;

		case 'C':
			current_element.compact = false;
			break;

		case 'I':
			// You can specify `-I +10` to increment by 10, or `-I +` to just increase 1
			switch (optarg[0]) {
			case '+':
				current_element.indent += optarg[1] ? atoi(optarg + 1) : 1;
				break;
			case '-':
				current_element.indent -= optarg[1] ? atoi(optarg + 1) : 1;
				break;
			default:
				current_element.indent = atoi(optarg);
			}
			break;

		case 'A':
			free(current_element.attributes); // works even if it is nil
			current_element.attributes = NULL;
			break;

		case 'a':
			if (!current_element.attributes) {
				current_element.attributes = strdup(optarg);
			} else {
				long len = strlen(current_element.attributes) + 2 + strlen(optarg);
				char *tmp = malloc(len);
				snprintf(tmp, len, "%s %s", current_element.attributes, optarg);
				free(current_element.attributes);
				current_element.attributes = tmp;
			}
			break;

		case 'X':
		case 'F':
		case 'T':
			print_indent(&current_element);
			current_element.prev_is_text_node = true;
			if (opt == 'X') fflush(stdout), system(optarg);
			else if (opt == 'F') cat_file(optarg);
			else fputs(optarg, stdout);
			break;

		case 'x':
		case 'f':
		case 't':
			if (!has_current_element())
				die("cannot print embedded text when there is no active element; try -T instead?");

			print_current_element(OPENING_ELE, INDENT);
			if (opt == 'x') execute_command(optarg);
			else if (opt == 'f') cat_file(optarg);
			else fputs(optarg, stdout);

			// TODO: should we have no indent? thats what the shell one did
			print_current_element(CLOSING_ELE, NO_INDENT);
			was_printed = true;
			break;

        case '?':
        	exit(EXIT_FAILURE);

		default:
			die("bug, unknown option %d", opt);
		}
	}

done:

	if ( has_current_element() && ! was_printed ) {
		print_current_element(OPENING_ELE, INDENT);
	}

	return status;
}

int main(int argc_, char *const argv_[]) {
	argc = argc_;
	argv = argv_;

	// Initial environment variable setup
	current_element.compact = getenv("COMPACT") != NULL;
	char *indent = getenv("INDENT");
	current_element.indent = indent ? atoi(indent) : 0;

	if (argc == 1)
		usage(true);

	if (run_program() == END_NESTED_ELEMENT)
		die("stray ] encountered");

	if (!current_element.compact) putchar('\n');
}
