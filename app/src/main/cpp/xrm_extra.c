// Xrm resource-database functions that SDL3 requires from libX11 (BASEXLIB).
// Without them SDL_X11_LoadSymbols() fails and SDL_Init reports "x11 not available".
// Plain C, no X11 headers: types are opaque pointers.
#include <stddef.h>

typedef void *XrmDatabase_;
typedef struct { unsigned int size; void *addr; } XrmValue_;

XrmDatabase_ XrmGetStringDatabase(const char *data) { (void)data; return NULL; }
void XrmDestroyDatabase(XrmDatabase_ db) { (void)db; }
int XrmGetResource(XrmDatabase_ db, const char *name, const char *cls, char **type, XrmValue_ *value)
{ (void)db; (void)name; (void)cls; (void)type; (void)value; return 0; }
