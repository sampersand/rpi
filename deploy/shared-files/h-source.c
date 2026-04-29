#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <assert.h>

bool compact = false;
const char *program_name;

#define die(...) (fprintf(stderr, __VA_ARGS__), exit(1))

// An HTML element
struct element {
	const char *name; // The name of the element (eg `h1`, `div`, etc)
	char *attributes; // Attributes for the element (eg `lang=html`, `style: ...`, etc)
	unsigned indent;  // How deeply indented the element is
	bool no_newline;  // whether a trailing newline should be printed
};

#ifndef ELEMENT_STACK_SIZE
# define ELEMENT_STACK_SIZE 10000
#endif
struct element element_stack[ELEMENT_STACK_SIZE];
unsigned element_stack_len = 0;
#define current_element (element_stack[element_stack_len])

void set_current_element(const char *name) {
	// TODO: free old `attributes`
	current_element.name = name;
	free(current_element.attributes);
	current_element.attributes = NULL;
}

void push_element(void) {
	unsigned old_indent = current_element.indent;
	if (element_stack_len++ > ELEMENT_STACK_SIZE) {
		fprintf(stderr, "%s: too many nested `[` encountered (%d max)", program_name, ELEMENT_STACK_SIZE);
		exit(2);
	}
	set_current_element(NULL);
	current_element.indent = old_indent + 1;
}

void pop_element(void) {
	assert(element_stack_len);
	free(current_element.attributes);
	--element_stack_len;
}

#define print_current_element(...) print_element(&current_element, __VA_ARGS__)

enum closing { OPENING_ELE, CLOSING_ELE };
enum newline { NO_TRAILING_NEWLINE, TRAILING_NEWLINE };
enum indent { NO_INDENT, INDENT };

void print_newline(const struct element *ele) {
	if (compact) return;
	if (!ele->no_newline) putchar('\n');
}

void print_indent(const struct element *ele) {
	if (compact) return;

	for (unsigned i = 0; i < ele->indent; ++i)
		putchar('\t');
}

void print_element(const struct element *ele, enum closing closing, enum newline newline, enum indent indent) {
	if (indent == INDENT) print_indent(ele);

	if (strcmp(ele->name, "@")) {
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

	if (newline == TRAILING_NEWLINE) print_newline(ele);
}

typedef char *const *argv_t;

int run_program(int argc, argv_t argv) {
	int opt;
	bool was_printed = false;

	while ( optind <= argc ) {
		opt = getopt(argc, argv, "t:T:a:AnNx:f:F:");
		// printf("got opt [optind=%d, %d]: %c (%d)\n", optind, argc, opt, opt);

		switch (opt) {
		case -1:
			;
			char *nonflag_arg = argv[optind++];
			if (!nonflag_arg) goto done;

			if (!strcmp(nonflag_arg, "]")) {
				return 0;
			}

			if (!strcmp(nonflag_arg, "[")) {
				print_current_element(OPENING_ELE, TRAILING_NEWLINE, INDENT);
				push_element();
				unsigned len = element_stack_len;
				run_program(argc, argv);
				if ( element_stack_len != len ) {
					die("mismatched `[`s for %s\n", current_element.name);
				}
				pop_element();
				print_current_element(CLOSING_ELE, TRAILING_NEWLINE, INDENT);
				was_printed = true;
				break;
			}

			if (current_element.name && !was_printed) {
				print_current_element(OPENING_ELE, TRAILING_NEWLINE, INDENT);
			}

			set_current_element(nonflag_arg);
			was_printed = false;
			break;

		case 'n':
			current_element.no_newline = true;
			break;

		case 'N':
			current_element.no_newline = false;
			break;

		case 'x':
			current_element.indent = atoi(optarg);
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

		case 'F': die("todo");
		case 'T':
			print_indent(&current_element);
			if (opt == 'F') {
				die("todo");
			} else {
				fputs(optarg, stdout);
			}
			print_newline(&current_element);
				// current_element=@ print_current_element '' no_newline
				// if [ $opt = F ]; then
				// 	cat -- "$OPTARG"
				// else
				// 	printf %s "$OPTARG"
				// fi
				// [ $COMPACT ] || [ ! $newline ] || echo

		case 'f': die("todo");
		case 't':
			print_current_element(OPENING_ELE, NO_TRAILING_NEWLINE, INDENT);
			if (opt == 'f') {
				die("todo");
			} else {
				fputs(optarg, stdout);
			}

			// TODO: should we have no indent? thats what the shell one did
			print_current_element(CLOSING_ELE, TRAILING_NEWLINE, NO_INDENT);
			was_printed = true;
			break;

        case '?':
			die("todo: bad options %c", opt);
		}
	}
done:
	if ( current_element.name && ! was_printed ) {
		print_current_element(OPENING_ELE, TRAILING_NEWLINE, INDENT);
	}

	return 0;
}

int main(int argc, argv_t argv) {
	program_name = argv[0];
	// char * const other_argv[] = { argv[0], "-n", "div", "-t", "foobar", "br", "-Tbaz", "quux", 0 };
	// char * const other_argv[] = { argv[0], "p", "-ax", "-ay", "-ta", "-tb", 0 };
	char * const other_argv[] = { argv[0], "div", "[", "-n", "p", "-tfoo", "-N", "]", 0 };
	if (argc == 1) {
		argc = sizeof(other_argv) / sizeof(char*) - 1; // / sizeof(char *);
		argv = other_argv;
	}
	return run_program(argc, argv);
}
