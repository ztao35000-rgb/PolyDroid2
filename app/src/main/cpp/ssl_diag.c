/* TEMP diagnostic (x86 preload): the game logs "cannot create a client for https://api.luduvo.com",
 * i.e. httplib's SSL_CTX_new fails. Try the same calls ourselves and print the OpenSSL error. */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

static void *diag(void *arg) {
    (void)arg;
    sleep(4);
    char comm[64] = {0};
    FILE *f = fopen("/proc/self/comm", "r");
    if (f) { fgets(comm, sizeof comm, f); fclose(f); }
    if (!strstr(comm, "LuduvoGame")) return NULL;

    unsigned char buf[16];
    long r = syscall(SYS_getrandom, buf, sizeof buf, 0);
    fprintf(stderr, "[ssl_diag] getrandom -> %ld errno=%d\n", r, r < 0 ? errno : 0);
    int fd = open("/dev/urandom", O_RDONLY);
    fprintf(stderr, "[ssl_diag] open(/dev/urandom) -> %d errno=%d\n", fd, fd < 0 ? errno : 0);
    if (fd >= 0) { ssize_t n = read(fd, buf, sizeof buf); fprintf(stderr, "[ssl_diag] read urandom -> %zd\n", n); close(fd); }

    void *ssl = dlopen("libssl.so.3", RTLD_NOW | RTLD_GLOBAL);
    void *cr = dlopen("libcrypto.so.3", RTLD_NOW | RTLD_GLOBAL);
    fprintf(stderr, "[ssl_diag] dlopen libssl=%p libcrypto=%p\n", ssl, cr);
    if (!ssl || !cr) return NULL;
    int (*init_ssl)(unsigned long, const void *) = dlsym(ssl, "OPENSSL_init_ssl");
    const void *(*method)(void) = dlsym(ssl, "TLS_client_method");
    void *(*ctx_new)(const void *) = dlsym(ssl, "SSL_CTX_new");
    void (*ctx_free)(void *) = dlsym(ssl, "SSL_CTX_free");
    unsigned long (*err_get)(void) = dlsym(cr, "ERR_get_error");
    void (*err_str)(unsigned long, char *, size_t) = dlsym(cr, "ERR_error_string_n");
    int (*rand_bytes)(unsigned char *, int) = dlsym(cr, "RAND_bytes");
    fprintf(stderr, "[ssl_diag] syms init=%p method=%p ctx_new=%p err=%p rand=%p\n",
            (void *)init_ssl, (void *)method, (void *)ctx_new, (void *)err_get, (void *)rand_bytes);
    if (!method || !ctx_new) return NULL;
    if (init_ssl) fprintf(stderr, "[ssl_diag] OPENSSL_init_ssl -> %d\n", init_ssl(0, NULL));
    if (rand_bytes) fprintf(stderr, "[ssl_diag] RAND_bytes -> %d\n", rand_bytes(buf, sizeof buf));
    void *ctx = ctx_new(method());
    fprintf(stderr, "[ssl_diag] SSL_CTX_new -> %p\n", ctx);
    unsigned long e;
    int guard = 0;
    while (err_get && (e = err_get()) && guard++ < 16) {
        char s[256] = {0};
        if (err_str) err_str(e, s, sizeof s);
        fprintf(stderr, "[ssl_diag] openssl error 0x%lx: %s\n", e, s);
    }
    if (ctx && ctx_free) ctx_free(ctx);
    return NULL;
}

__attribute__((constructor)) static void ssl_diag_init(void) {
    pthread_t t;
    if (pthread_create(&t, NULL, diag, NULL) == 0) pthread_detach(t);
}
