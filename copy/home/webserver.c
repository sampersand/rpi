#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <string.h>

#define BACKLOG 10
#define HEADERS_FD 9

#define DEBUG_LOG(...) do { if (debug) { fprintf(stderr, "%s:%d:%s ", __FILE__, __LINE__, __FUNCTION__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); fflush(stderr); } } while(0)
#define VERBOSE(...) printf(__VA_ARGS__), printf("\n")

int run_once = 0;
int port = 8080;
char *cgi_bin =
	#ifndef CGI_BIN_LOCATION
	"/usr/bin/run-cgi-bin"
	#else
	CGI_BIN_LOCATION
	#endif
	;
int debug = 0;



// Read entire fd into a malloc'd buffer, returns number of bytes read
int read_fd(int fd, char **out) {
	DEBUG_LOG("reading from fd %d", fd);
	char *buf = malloc(4096);
	int capacity = 4096, total = 0, bytes;
	while ((bytes = read(fd, buf + total, capacity - total)) > 0) {
		total += bytes;
		if (total == capacity) {
			capacity *= 2;
			buf = realloc(buf, capacity);
		}
	}
	DEBUG_LOG("read %d bytes from from fd %d", total, fd);
	*out = buf;
	return total;
}


void handle_request(int client_fd) {
	int outpipe[2];    // stdout: body
	int headerpipe[2]; // fd HEADERS_FD: response headers

	pipe(outpipe);
	pipe(headerpipe);

	pid_t pid = fork();
	if (pid == 0) {
		close(outpipe[0]);
		close(headerpipe[0]);

		dup2(client_fd, STDIN_FILENO);   // raw HTTP request readable from stdin
		dup2(outpipe[1], STDOUT_FILENO);
		dup2(headerpipe[1], HEADERS_FD); // fd HEADERS_FD for response headers

		// Close originals now that they're dup'd
		close(outpipe[1]);
		close(headerpipe[1]);
		close(client_fd);
		DEBUG_LOG("starting execution of: %s", cgi_bin);

		execl(cgi_bin, cgi_bin, NULL);
		perror("execl failed");
		exit(1);
	}

	// Close write ends so reads will EOF when child exits
	close(outpipe[1]);
	close(headerpipe[1]);

	char *headers = NULL;
	char *output = NULL;
	int headers_len = read_fd(headerpipe[0], &headers);
	int output_len  = read_fd(outpipe[0], &output);
	close(headerpipe[0]);
	close(outpipe[0]);

	int status;
	waitpid(pid, &status, 0);

	if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
		char *resp = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nContent-Length: 22\r\n\r\nInternal Server Error\n";
		write(client_fd, resp, strlen(resp));
		free(headers); free(output);
		return;
	}

	// Send headers (written by script to HEADERS_FD) then body (stdout) ,as well as a content-length
	if (headers_len > 0)
		write(client_fd, headers, headers_len);
	if (output_len > 0) {
		char cl[64];
		int cl_len = snprintf(cl, sizeof(cl), "Content-Length: %d\r\n\r\n", output_len);
		write(client_fd, cl, cl_len);
		write(client_fd, output, output_len);
	}

	free(headers);
	free(output);
}

int parse_options(int argc, char *argv[]) {
	int opt;
	while ((opt = getopt(argc, argv, "p:b:doh")) != -1) {
		switch (opt) {

		case 'p': {
			char *endptr;
			long val = strtol(optarg, &endptr, 10);

			if (*endptr != '\0' || val <= 0 || val > 65535) {
				fprintf(stderr, "Error: -p requires a valid port number (1-65535), got: %s\n", optarg);
				return 1;
			}

			port = (int)val;
			break;
		}

		case 'b': {
			if (optarg[0] != '/') {
				fprintf(stderr, "Error: -b requires an absolute path, got: %s\n", optarg);
				return 1;
			}

			if (access(optarg, X_OK) != 0) {
				fprintf(stderr, "Error: CGI bin path is not executable: %s\n", optarg);
				return 1;
			}

			cgi_bin = optarg;
			break;
		}

		case 'd':
			debug = 1;
			break;

		case 'o':
			run_once = 1;
			break;

		case 'h':
		case '?':
		default:
			fprintf(stderr, "Usage: %s [-p PORT] [-b CGI_BIN] [-d] [-o]\n", argv[0]);
			return 1;
		}
	}

	return 0;
}

int main(int argc, char **argv) {
	if (parse_options(argc, argv)) return 1;

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	int opt = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr.s_addr = INADDR_ANY,
	};
	bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
	listen(sockfd, BACKLOG);

	VERBOSE("Listening on port %d", port);

	while (1) {
		DEBUG_LOG("waiting for new request");
		int client_fd = accept(sockfd, NULL, NULL);
		DEBUG_LOG("got a request");
		if (fork() == 0) {
			// Child process
			close(sockfd);
			handle_request(client_fd);
			exit(0);
		}
		// Parent: clean up and loop
		close(client_fd);
		waitpid(-1, NULL, WNOHANG);  // reap any finished children

		if (run_once) break;
	}
}
