#include "trace_runtime.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

#if !defined(__x86_64__)
#error "Este runtime didatico suporta apenas Linux x86_64."
#endif

static void fill_event_from_regs(pid_t pid, int entering, const struct user_regs_struct *regs, struct syscall_event *ev) {
    (void)regs; 
    memset(ev, 0, sizeof(*ev));
    ev->pid = pid;
    ev->entering = entering;
}

static pid_t launch_tracee(char *const argv[]) {
   pid_t pid = fork();
    if (pid < 0) {
        perror("erro ao executar o fork");
        return -1;
    }
    if (pid == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL); 
        raise(SIGSTOP); 
        execvp(argv[0], argv); 
        perror("erro ao executar o execvp");
        _exit(1);
    }
    return pid; 
}

static int wait_for_initial_stop(pid_t child) {
   int status;
    if (waitpid(child, &status, 0) < 0) {
        perror("erro no waitpid inicial");
        return -1;
    }
    if (WIFSTOPPED(status)) {
        return 0;
    }
    return -1;
}

static int configure_trace_options(pid_t child) {
    (void)child;
    fprintf(stderr, "erro: TODO Semana 3: implementar configure_trace_options()\n");
    return -1;
}

static int resume_until_next_syscall(pid_t child, int signal_to_deliver) {
    (void)child; (void)signal_to_deliver;
    fprintf(stderr, "erro: TODO Semana 3: implementar resume_until_next_syscall()\n");
    return -1;
}

static int wait_for_syscall_stop(pid_t child, int *status) {
    (void)child; (void)status;
    fprintf(stderr, "erro: TODO Semana 3: implementar wait_for_syscall_stop()\n");
    return -1;
}

int trace_program(char *const argv[], trace_observer_fn observer, void *userdata) {
    pid_t child;
    int status = 0;
    int entering = 1;
    if (argv == NULL || argv[0] == NULL) {
        fprintf(stderr, "erro: programa alvo ausente\n");
        return -1;
    }
    child = launch_tracee(argv);
    if (child < 0) return -1;
    if (wait_for_initial_stop(child) < 0) return -1;
    if (configure_trace_options(child) < 0) return -1;
    if (resume_until_next_syscall(child, 0) < 0) return -1;
    while (1) {
        struct user_regs_struct regs;
        struct syscall_event ev;
        int stop_kind = wait_for_syscall_stop(child, &status);
        if (stop_kind < 0) return -1;
        if (stop_kind == 0) {
            if (WIFEXITED(status)) return WEXITSTATUS(status);
            if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
            return 0;
        }
        memset(&regs, 0, sizeof(regs));
        fill_event_from_regs(child, entering, &regs, &ev);
        if (observer != NULL) observer(&ev, userdata);
        entering = !entering;
        if (resume_until_next_syscall(child, 0) < 0) return -1;
    }
}