#!/bin/sh
if [ ! -e /tmp/hello_c ] || [ /tmp/hello_c -ot "${EXECUTABLE:?}" ]; then cc -xc - <<EOS -o /tmp/hello_c || exit; fi; exec /tmp/hello_c "$@"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(){
	if (strcmp(getenv("METHOD"), "GET")) {
		system("status 405 true");
		return( 1 );
	}

	char *place;
	if (!(place = getenv("place"))) place = "world";

	system("header Content-Type text/html");

	puts("<!DOCTYPE html>");
	puts("<html lang=en>");
	printf("<title> Hello %s! </title>\n", place);
	puts("</head>");
	puts("<body>");
	printf("<h1> Hello, %s! </h1>\n", place);
	printf("<p> The time on the server is currently "), fflush(stdout), system("date"), printf(".</p>\n");
	puts("</body>");
	puts("</html>");
}
