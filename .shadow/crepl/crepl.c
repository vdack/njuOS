#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <dlfcn.h>
#define T_EXPR 0
#define T_FUNC 1

static inline int parse_line(char* line) {
    if (line[0] == 'i' && line[1] == 'n' && line[2] == 't' && line[3] == ' ') {
        return T_FUNC;
    }
    return T_EXPR;
}

static char* buffer_args[10];
static char* target_args[9];

static inline void init_env(char* buffer_name, char* target_name) {
    setenv("LD_LIBRARY_PATH", "/tmp/", 1);

    buffer_args[0] = strdup("gcc");
    buffer_args[1] = strdup("-fPIC");
    buffer_args[2] = strdup("-shared");
    buffer_args[3] = strdup("-o");
    buffer_args[4] = strdup("/tmp/libmybuffer.so");
    buffer_args[5] = strdup(buffer_name);
    buffer_args[6] = strdup("-L/tmp");
    buffer_args[7] = strdup("-lmytarget");
#ifdef __x86_64__
    buffer_args[8] = strdup("-m64");
#else 
    buffer_args[8] = strdup("-m32");
#endif 
    buffer_args[9] = strdup("-w");

    target_args[0] = strdup("gcc");
    target_args[1] = strdup("-fPIC");
    target_args[2] = strdup("-shared");
    target_args[3] = strdup("-o");
    target_args[4] = strdup("/tmp/libmytarget.so");
    target_args[5] = strdup(target_name);
#ifdef __x86_64__
    target_args[6] = strdup("-m64");
#else 
    target_args[6] = strdup("-m32");
#endif 
    target_args[7] = strdup("-w");
    target_args[8] = NULL;

}

static inline void compile_buffer() {
    execvp(buffer_args[0], buffer_args);
}
static inline void compile_target() {
    execvp(target_args[0], target_args);
}

int main(int argc, char *argv[]) {

#ifdef __x86_64__
    printf("on 64 machine.\n");
#else 
    printf("on 32 machine.\n");
#endif

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
    close(target);
    close(buffer);

    init_env(s_buffer, s_target);
    int rc = fork();
    if (rc == 0) {
        compile_target();
    }
    wait(NULL);
    rc = fork();
    if (rc == 0) {
        compile_buffer();
    }
    wait(NULL);


    static char line[4096];

    while (1) {
        printf("crepl> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }
        
        // To be implemented.

        int type = parse_line(line);
        buffer = open(s_buffer, O_WRONLY | O_TRUNC);
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
        close(buffer);

        rc = fork();
        if (rc == 0) {
            //child.
            compile_buffer();   
        }

        //parent.
        int status;
        wait(&status);
        int exit_status = WEXITSTATUS(status);
        printf("get return value: %d\n", exit_status);

        if (exit_status != 0) {
            printf("COMPILE ERROR!\n");
            continue;
        }

        if(type == T_FUNC) {
            target = open(s_target, O_WRONLY | O_APPEND);
            write(target, line, strlen(line) + 1);
            printf("Added: %s \n", line);
            close(target);
            rc = fork();
            if (rc == 0) {
                compile_target();
            }
            wait(NULL);
        } else {
            int (*fc)(void);
            void* handle;
            char* error;

            handle = dlopen("/tmp/libmybuffer.so", RTLD_LAZY);
            if (!handle) {
                fprintf(stderr, "%s\n", dlerror());
                return 1;
            }
            dlerror();
            printf("before open the lib.\n");
            *(int **) (&fc) = dlsym(handle, "_wrapper");
            if ((error = dlerror()) != NULL)  {
                fprintf(stderr, "%s\n", error);
                dlclose(handle);
                return 1;
            }
            printf("before run the function.\n");
            printf("(%s) == %d\n", line, fc());
            dlclose(handle);
        }
    
    }

    return 0;
}
