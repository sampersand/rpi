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

bool needs_trailing_whitesapce(const char *str) {
	if (str == NULL) return false;
	for (const char *p = inline_elements[0]; *p; ++p)
		if (!strcmp(str, p)) return true;
	return false;
}

// Controlled by the `COMPACT` env var. If set, no extraneous whitespace is printed
bool compact;
int inline_mode;
bool is_very_first_element = true;

// The `argc` and `argv` from `main()` (set here so others can use them)
int argc;
char *const *argv;

// Aborts with a usage message
#define die(...) do { fprintf(stderr, "%s: ", argv[0]), \
				  fprintf(stderr, __VA_ARGS__), \
				  fputc('\n', stderr), \
				  exit(1); } while (0)

// Prints a file to stdout, aborting if there's a problem
void cat_file(const char *file) {
    FILE *f = fopen(file, "r");
    if (!f)
    	die("cannot cat %s: %s", file, strerror(errno));

    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
        fwrite(buf, 1, n, stdout);

    fclose(f);
}

/**************************************************************************************************
 *                                         HTML Elements                                          *
 **************************************************************************************************/

// An HTML element
struct element {
	const char *name;      // The name of the element (eg `h1`, `div`, etc)
	char *attributes;      // Attributes for the element (`style=`, `href=`, ...). malloc'd.
	unsigned short indent; // How deeply indented the element is
	bool no_newline;       // whether a trailing newline should be printed
	bool previous_needs_whitespace;
};

// Prints the leading indentation for the element
void print_indent(struct element *ele) {
	if (inline_mode) {
		inline_mode = false;
		return;
	}
	if (compact) {
		if (ele->previous_needs_whitespace)
			putchar(' ');
	} else {
		if (is_very_first_element)
			is_very_first_element = false;
		else if (!ele->no_newline)
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
	if (current_element.name)
		current_element.previous_needs_whitespace = needs_trailing_whitesapce(current_element.name);

	current_element.name = name;
	free(current_element.attributes); // NOTE: this is OK even when `attributes` are null.
	current_element.attributes = NULL;
}

// Pushes the current stack, adding a new element on top. Used by `[`.
void push_stack(void) {
	unsigned short old_indent = current_element.indent;
	if (element_stack_len++ > ELEMENT_STACK_SIZE) {
		die("too many nested `[` encountered (%d max)", ELEMENT_STACK_SIZE);
	}

	set_current_element(NULL);
	current_element.indent = old_indent + 1;
	current_element.previous_needs_whitespace = false;
}

// Clears the current element from the top of the stack
void pop_stack(void) {
	assert(element_stack_len);
	free(current_element.attributes); // make sure we don't have dangling memory
	--element_stack_len;
}

/**************************************************************************************************
 *                                          Program Loop                                          *
 **************************************************************************************************/

int getopt_possibly_long(void) {
#ifdef HAS_GETOPT_LONG
	// TODO
	static struct option longopts[] = {
	    { "text",           required_argument,  NULL,   't' },
	    { "inline-text",    required_argument,  NULL,   'T' },
	    { "attribute",      required_argument,  NULL,   'a' },
	    { "clear-attribute",no_argument,        NULL,   'A' },
	    { "no-newline",     no_argument,        NULL,   'n' },
	    { "newline",        no_argument,        NULL,   'N' },
	    { "indent",         required_argument,  NULL,   'x' },
	    { "include-file",   required_argument,  NULL,   'f' },
	    { "inline-file",    required_argument,  NULL,   'F' },
	    { "inline",         no_argument,        NULL,   'i' },
	    { NULL,             0,                  NULL,    0  },
	};

	return getopt_long(argc, argv, "+t:T:a:AnNix:f:F:", longopts, NULL);
#else
	return getopt(argc, argv, "t:T:a:AnNix:f:F:");
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
	// int inline_mode = 0;

	// Loop while there's still stuff left to be read.
	while ( 1 ) {
		inline_mode = false;
	top:
		if (!(optind < argc)) break;
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
				pop_stack();

				if (child_status != END_NESTED_ELEMENT)
					die("missing closing ] for %s", current_element.name);
				print_current_element(CLOSING_ELE, INDENT);
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

		case 'i':
			inline_mode = true;
			goto top;

		case 'n':
			current_element.no_newline = true;
			break;

		case 'N':
			current_element.no_newline = false;
			break;

		case 'x':
			// You can specify `-x +10` to increment by 10, or `-x +` to just increase 1
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

		case 'F':
		case 'T':
			print_indent(&current_element);
			current_element.previous_needs_whitespace = true;
			if (opt == 'F') cat_file(optarg);
			else fputs(optarg, stdout);
			break;

		case 'f':
		case 't':
			if (!has_current_element())
				die("cannot print embedded text when there is no active element; try -T instead?");

			print_current_element(OPENING_ELE, INDENT);
			if (opt == 'f') {
				cat_file(optarg);
			} else {
				fputs(optarg, stdout);
			}

			// TODO: should we have no indent? thats what the shell one did
			print_current_element(CLOSING_ELE, NO_INDENT);
			was_printed = true;
			break;

        case '?':
        	exit(1);

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
	compact = getenv("COMPACT") != NULL;
	char *indent = getenv("INDENT");
	current_element.indent = indent ? atoi(indent) : 0;

	// char * const other_argv[] = { argv[0], "-n", "div", "-t", "foobar", "br", "-Tbaz", "quux", 0 };
	// char * const other_argv[] = { argv[0], "p", "-ax", "-ay", "-ta", "-tb", 0 };
	// char * const other_argv[] = { argv[0], "div", "[]", "p", 0 }; //, "-n", "p", "-tfoo", "-N", "]", 0 };
	char * const other_argv[] = { argv[0], "-Ta", "-Tb", "-iTc", 0 }; //, "-n", "p", "-tfoo", "-N", "]", 0 };
	if (argc == 1) {
		compact=1;
		argc = sizeof(other_argv) / sizeof(char*) - 1; // / sizeof(char *);
		argv = other_argv;
	}

	if (run_program() == END_NESTED_ELEMENT)
		die("stray ] encountered");

	if (! compact) putchar('\n');
}
