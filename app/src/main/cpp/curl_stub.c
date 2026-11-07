#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define CURLE_FAILED_INIT 2

struct curl_slist {
    char *data;
    struct curl_slist *next;
};

typedef struct {
    int age;
    const char *version;
    unsigned int version_num;
    const char *host;
    int features;
    const char *ssl_version;
    long ssl_version_num;
    const char *libz_version;
    const char *const *protocols;
} curl_version_info_data;

static const char *const no_protocols[] = { NULL };
static curl_version_info_data version_info = {
    0, "7.88.1", 0x075801, "x86_64-pc-linux-gnu", 0, NULL, 0, NULL, no_protocols
};

int curl_global_init(long flags) { (void)flags; return CURLE_FAILED_INIT; }
void curl_global_cleanup(void) {}
void *curl_easy_init(void) { return NULL; }
void curl_easy_cleanup(void *h) { (void)h; }
void curl_easy_reset(void *h) { (void)h; }
int curl_easy_perform(void *h) { (void)h; return CURLE_FAILED_INIT; }
int curl_easy_setopt(void *h, int opt, ...) { (void)h; (void)opt; return CURLE_FAILED_INIT; }
int curl_easy_getinfo(void *h, int info, ...) { (void)h; (void)info; return CURLE_FAILED_INIT; }
const char *curl_easy_strerror(int code) { (void)code; return "curl unavailable"; }
curl_version_info_data *curl_version_info(int age) { (void)age; return &version_info; }

struct curl_slist *curl_slist_append(struct curl_slist *list, const char *s) {
    struct curl_slist *node = calloc(1, sizeof(*node));
    if (!node) return NULL;
    node->data = strdup(s ? s : "");
    if (!node->data) { free(node); return NULL; }
    if (!list) return node;
    struct curl_slist *last = list;
    while (last->next) last = last->next;
    last->next = node;
    return list;
}

void curl_slist_free_all(struct curl_slist *list) {
    while (list) {
        struct curl_slist *next = list->next;
        free(list->data);
        free(list);
        list = next;
    }
}
