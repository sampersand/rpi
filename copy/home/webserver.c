#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>

#define BACKLOG 10
#define OUTPUT_FD 9

#define LOG(...) (fprintf(stderr, "%s:%d:%s ", __FILE__, __LINE__, __FUNCTION__), fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n"), fflush(stderr))

// Read entire fd into a malloc'd buffer, returns number of bytes read
int read_fd(int fd, char **out) {
    LOG("reading from fd %d");
    char *buf = malloc(4096);
    int capacity = 4096, total = 0, bytes;
    while ((bytes = read(fd, buf + total, capacity - total)) > 0) {
        total += bytes;
        if (total == capacity) {
            capacity *= 2;
            buf = realloc(buf, capacity);
        }
    }
    LOG("read %d bytes from from fd %d", total, fd);
    *out = buf;
    return total;
}

char **build_envp(char *query, char *method) {
    extern char **environ;
    int envc = 0;
    while (environ[envc]) envc++;

    char **envp = malloc((envc + 3) * sizeof(char *));
    memcpy(envp, environ, envc * sizeof(char *));

    int query_len = sizeof("QUERY_STRING=") + strlen(query);
    int method_len = sizeof("METHOD=") + strlen(method);

    char *query_env = malloc(query_len);
    snprintf(query_env, query_len, "QUERY_STRING=%s", query);
    char *method_env = malloc(method_len);
    snprintf(method_env, method_len, "METHOD=%s", method);

    envp[envc]     = query_env;
    envp[envc + 1] = method_env;
    envp[envc + 2] = NULL;
    return envp;
}

void free_envp(char **envp) {
    extern char **environ;
    int envc = 0;
    while (environ[envc]) envc++;
    free(envp[envc]);     // query_env
    free(envp[envc + 1]); // method_env
    free(envp);
}

void handle_request(int client_fd) {
    char buf[4096];
    int n = read(client_fd, buf, sizeof(buf) - 1);
    if (n <= 0) return;
    buf[n] = '\0';

    char method[16], path[256];
    sscanf(buf, "%15s %255s", method, path);

    // Split query string from path
    char *query = strchr(path, '?');
    if (query) {
        *query = '\0';
        query++;
    } else {
        query = "";
    }

    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s/webserver%s", getenv("HOME"), path);

    if (access(fullpath, F_OK | X_OK) != 0) {
        char *resp = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nNot Found\n";
        write(client_fd, resp, strlen(resp));
        return;
    }

    // Parse Content-Length for POST body
    int content_length = 0;
    char *cl = strstr(buf, "Content-Length: ");
    if (cl) sscanf(cl, "Content-Length: %d", &content_length);

    char *body = strstr(buf, "\r\n\r\n");
    if (body) body += 4;

    char **envp = build_envp(query, method);

    int outpipe[2];   // stdout: body
    int headerpipe[2]; // fd OUTPUT_FD: headers
    int stdinpipe[2];

    pipe(outpipe);
    pipe(headerpipe);
    pipe(stdinpipe);

    pid_t pid = fork();
    if (pid == 0) {
        close(outpipe[0]);
        close(headerpipe[0]);
        close(stdinpipe[1]);

        dup2(outpipe[1], STDOUT_FILENO);
        dup2(stdinpipe[0], STDIN_FILENO);
        dup2(headerpipe[1], OUTPUT_FD);  // fd OUTPUT_FD for headers

        execle("/usr/bin/run-cgi-bin", "/usr/bin/run-cgi-bin", fullpath, NULL, envp);
        perror("execle failed");
        exit(1);
    }

    free_envp(envp);

    // Write POST body to child's stdin
    close(stdinpipe[0]);
    if (body && content_length > 0)
        write(stdinpipe[1], body, content_length);
    close(stdinpipe[1]);

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

    if (WIFEXITED(status) && WEXITSTATUS(status) > 1) {
        char *resp = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\n\r\nInternal Server Error\n";
        write(client_fd, resp, strlen(resp));
    } else {
    	if (WIFEXITED(status) && WEXITSTATUS(status) == 1) {
	        char *resp = "HTTP/1.1 400 Bad Request\r\n";
	        write(client_fd, resp, strlen(resp));
    	} else {
	        write(client_fd, "HTTP/1.1 200 OK!\r\n", 18);
	    }
	}

    write(client_fd, headers, headers_len);  // headers (including blank line) from fd OUTPUT_FD

    if (output_len > 0) {
    	char output_len_str[65535];
    	snprintf(output_len_str, sizeof(output_len_str), "Content-Length: %d\r\n\r\n", output_len);
    	write(client_fd, output_len_str, strlen(output_len_str));
    	write(client_fd, output, output_len);    // body from stdout
    }

    free(headers);
    free(output);
}

int main(int argc, const char **argv) {
	int port = 8080;
	if ( argc > 1 ) {
		port = atoi(argv[1]);
	}

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

    printf("Listening on port %d\n", port);

    while (1) {
		LOG("waiting for new request");
        int client_fd = accept(sockfd, NULL, NULL);
		LOG("got a request");
        if (fork() == 0) {
            // Child process
            close(sockfd);
            handle_request(client_fd);
            exit(0);
        }
        // Parent: clean up and loop
        close(client_fd);
        waitpid(-1, NULL, WNOHANG);  // reap any finished children

        break; // DELETEME; only run once
    }
}
