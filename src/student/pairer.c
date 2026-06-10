#include "student_api.h"

int student_pair_syscall(struct syscall_pairer *pairer,
                         const struct syscall_event *ev,
                         struct syscall_event *out)
{
    if (pairer == NULL || ev == NULL || out == NULL) {
        return -1;
    }

    if (ev->entering) {
        if (pairer->has_entry) {
            return -1;
        }

        pairer->entry = *ev;
        pairer->has_entry = 1;
        return 0;
    }

    if (!pairer->has_entry) {
        return -1;
    }

    if (pairer->entry.pid != ev->pid ||
        pairer->entry.syscall_no != ev->syscall_no) {
        pairer->has_entry = 0;
        return -1;
    }

    *out = pairer->entry;
    out->entering = 0;
    out->ret = ev->ret;

    pairer->has_entry = 0;
    return 1;
}
