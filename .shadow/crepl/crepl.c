#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#define T_EXPR 0
#define T_FUNC 1

static inline int parse_line(char* line) {
    if (line[0] == 'i' && line[1] == 'n' && line[2] == 't' && line[3] == ' ') {
        return T_FUNC;
    }
    return T_EXPR;
}

int _wrapper();
static inline void init_env() {
    setenv("LD_LIBRARY_PATH", "/tmp", 1);
}
static inline void try_compile() {
    //
}
int main(int argc, char *argv[]) {
    printf("before create file.\n");
    char s_target[32] = "/tmp/targetXXXXXX.c";
    char s_buffer[32] = "/tmp/bufferXXXXXX.c";
    int target = mkstemps(s_target, 2);
    int buffer = mkstemps(s_buffer, 2);
    // first write the line into the buffer and try to compile
    // if succed, add it into target.
    printf("after create file. %s and %s\n", s_buffer, s_target);
    if (target == -1 || buffer == -1) {
        perror("Failed to create file.\n");
        return 1;
    }

    init_env();

    static char line[4096];
    int rc = -1;
    while (1) {
        printf("crepl> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }
        
        // To be implemented.

        int type = parse_line(line);

        if (type == T_FUNC) {
            // a function.
            write(buffer, line, strlen(line) + 1);


        } else {
            // an expression. 
            line[strlen(line) - 1] = '\0';
            char new_line[4096] = "int _wrapper(){return (";
            strcat(new_line, line);
            strcat(new_line, ");}\n");
            write(buffer, new_line, strlen(new_line) + 1);


        }

        // rc = fork();
        // if (rc == 0) {
        //     //child.
            
        // } else {
        //     //parent.
        // }



        // My implementation above.

        printf("Got %zu chars.\n", strlen(line));
    }

    return 0;
}
