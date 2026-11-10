/*
 * POSIX semaphore shim (x86_64 side, preloaded under box64).
 * Luduvo's logger (quill) aborts with "Failed to create semaphore, errno 38"
 * because the sem_* calls come back ENOSYS here. Futex based replacements.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>

#ifndef FUTEX_WAIT
#define FUTEX_WAIT 0
#define FUTEX_WAKE 1
#endif
#define FUTEX_PRIVATE 128

/* layout inside sem_t: [0]=count (uint32) [1]=waiters (uint32) */
static inline uint32_t *cnt(sem_t *s) { return (uint32_t *)s; }
static inline uint32_t *wtr(sem_t *s) { return ((uint32_t *)s) + 1; }

static long futex(uint32_t *addr, int op, uint32_t val, const struct timespec *to) {
    return syscall(SYS_futex, addr, op, val, to, NULL, 0);
}

int sem_init(sem_t *s, int pshared, unsigned int value) {
    if (!s || value > SEM_VALUE_MAX) { errno = EINVAL; return -1; }
    memset(s, 0, sizeof(*s));
    __atomic_store_n(cnt(s), value, __ATOMIC_SEQ_CST);
    (void)pshared;
    return 0;
}

int sem_destroy(sem_t *s) { (void)s; return 0; }

int sem_post(sem_t *s) {
    uint32_t c = __atomic_load_n(cnt(s), __ATOMIC_SEQ_CST);
    for (;;) {
        if (c == SEM_VALUE_MAX) { errno = EOVERFLOW; return -1; }
        if (__atomic_compare_exchange_n(cnt(s), &c, c + 1, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) break;
    }
    if (__atomic_load_n(wtr(s), __ATOMIC_SEQ_CST) > 0)
        futex(cnt(s), FUTEX_WAKE, 1, NULL);
    return 0;
}

int sem_trywait(sem_t *s) {
    uint32_t c = __atomic_load_n(cnt(s), __ATOMIC_SEQ_CST);
    while (c > 0) {
        if (__atomic_compare_exchange_n(cnt(s), &c, c - 1, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) return 0;
    }
    errno = EAGAIN;
    return -1;
}

static int wait_common(sem_t *s, const struct timespec *abs, clockid_t clk) {
    for (;;) {
        uint32_t c = __atomic_load_n(cnt(s), __ATOMIC_SEQ_CST);
        while (c > 0) {
            if (__atomic_compare_exchange_n(cnt(s), &c, c - 1, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) return 0;
        }
        struct timespec rel, *prel = NULL;
        if (abs) {
            struct timespec now;
            clock_gettime(clk, &now);
            rel.tv_sec = abs->tv_sec - now.tv_sec;
            rel.tv_nsec = abs->tv_nsec - now.tv_nsec;
            if (rel.tv_nsec < 0) { rel.tv_sec--; rel.tv_nsec += 1000000000L; }
            if (rel.tv_sec < 0) { errno = ETIMEDOUT; return -1; }
            prel = &rel;
        }
        __atomic_add_fetch(wtr(s), 1, __ATOMIC_SEQ_CST);
        long r = futex(cnt(s), FUTEX_WAIT, 0, prel);
        int e = errno;
        __atomic_sub_fetch(wtr(s), 1, __ATOMIC_SEQ_CST);
        if (r != 0 && e == ETIMEDOUT) { errno = ETIMEDOUT; return -1; }
        if (r != 0 && e == EINTR) { errno = EINTR; return -1; }
    }
}

int sem_wait(sem_t *s) {
    for (;;) {
        int r = wait_common(s, NULL, CLOCK_MONOTONIC);
        if (r == 0 || errno != EINTR) return r;
    }
}
int sem_timedwait(sem_t *s, const struct timespec *abs) { return wait_common(s, abs, CLOCK_REALTIME); }
int sem_clockwait(sem_t *s, clockid_t clk, const struct timespec *abs) { return wait_common(s, abs, clk); }

int sem_getvalue(sem_t *s, int *out) {
    *out = (int)__atomic_load_n(cnt(s), __ATOMIC_SEQ_CST);
    return 0;
}

/* named semaphores: process-local only */
struct named { char name[128]; sem_t *sem; struct named *next; };
static struct named *g_named;
static pthread_mutex_t g_named_lock = PTHREAD_MUTEX_INITIALIZER;

sem_t *sem_open(const char *name, int oflag, ...) {
    unsigned int value = 0;
    if (oflag & O_CREAT) {
        va_list ap; va_start(ap, oflag);
        (void)va_arg(ap, unsigned int);
        value = va_arg(ap, unsigned int);
        va_end(ap);
    }
    pthread_mutex_lock(&g_named_lock);
    for (struct named *n = g_named; n; n = n->next) {
        if (strncmp(n->name, name, sizeof(n->name)) == 0) {
            if ((oflag & O_CREAT) && (oflag & O_EXCL)) { pthread_mutex_unlock(&g_named_lock); errno = EEXIST; return SEM_FAILED; }
            pthread_mutex_unlock(&g_named_lock);
            return n->sem;
        }
    }
    if (!(oflag & O_CREAT)) { pthread_mutex_unlock(&g_named_lock); errno = ENOENT; return SEM_FAILED; }
    struct named *n = calloc(1, sizeof(*n));
    sem_t *s = calloc(1, sizeof(sem_t));
    if (!n || !s) { free(n); free(s); pthread_mutex_unlock(&g_named_lock); errno = ENOMEM; return SEM_FAILED; }
    strncpy(n->name, name, sizeof(n->name) - 1);
    n->sem = s;
    sem_init(s, 1, value);
    n->next = g_named; g_named = n;
    pthread_mutex_unlock(&g_named_lock);
    return s;
}
int sem_close(sem_t *s) { (void)s; return 0; }
int sem_unlink(const char *name) {
    pthread_mutex_lock(&g_named_lock);
    struct named **pp = &g_named;
    while (*pp) {
        if (strncmp((*pp)->name, name, sizeof((*pp)->name)) == 0) { *pp = (*pp)->next; pthread_mutex_unlock(&g_named_lock); return 0; }
        pp = &(*pp)->next;
    }
    pthread_mutex_unlock(&g_named_lock);
    errno = ENOENT;
    return -1;
}
