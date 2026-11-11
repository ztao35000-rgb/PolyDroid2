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

/* SDL3 non-XKB keymap path derefs XGetKeyboardMapping()'s result; the committed libX11 stub returned NULL.
   CI renames that stub export so this definition (reached via DT_NEEDED) wins. */
typedef unsigned long KS_;
static KS_ s_xrm_kc2ks[256] = {
    [  9] = 0xff1b, /* Escape */
    [ 10] = 0x0031, /* 1 */  [ 11] = 0x0032, /* 2 */  [ 12] = 0x0033, /* 3 */
    [ 13] = 0x0034, /* 4 */  [ 14] = 0x0035, /* 5 */  [ 15] = 0x0036, /* 6 */
    [ 16] = 0x0037, /* 7 */  [ 17] = 0x0038, /* 8 */  [ 18] = 0x0039, /* 9 */
    [ 19] = 0x0030, /* 0 */
    [ 20] = 0x002d, /* minus */     [ 21] = 0x003d, /* equal */
    [ 22] = 0xff08, /* BackSpace */ [ 23] = 0xff09, /* Tab */
    [ 24] = 0x0071, /* q */  [ 25] = 0x0077, /* w */  [ 26] = 0x0065, /* e */
    [ 27] = 0x0072, /* r */  [ 28] = 0x0074, /* t */  [ 29] = 0x0079, /* y */
    [ 30] = 0x0075, /* u */  [ 31] = 0x0069, /* i */  [ 32] = 0x006f, /* o */
    [ 33] = 0x0070, /* p */
    [ 34] = 0x005b, /* bracketleft */  [ 35] = 0x005d, /* bracketright */
    [ 36] = 0xff0d, /* Return */
    [ 37] = 0xffe3, /* Control_L */
    [ 38] = 0x0061, /* a */  [ 39] = 0x0073, /* s */  [ 40] = 0x0064, /* d */
    [ 41] = 0x0066, /* f */  [ 42] = 0x0067, /* g */  [ 43] = 0x0068, /* h */
    [ 44] = 0x006a, /* j */  [ 45] = 0x006b, /* k */  [ 46] = 0x006c, /* l */
    [ 47] = 0x003b, /* semicolon */  [ 48] = 0x0027, /* apostrophe */
    [ 49] = 0x0060, /* grave */
    [ 50] = 0xffe1, /* Shift_L */
    [ 51] = 0x005c, /* backslash */
    [ 52] = 0x007a, /* z */  [ 53] = 0x0078, /* x */  [ 54] = 0x0063, /* c */
    [ 55] = 0x0076, /* v */  [ 56] = 0x0062, /* b */  [ 57] = 0x006e, /* n */
    [ 58] = 0x006d, /* m */
    [ 59] = 0x002c, /* comma */  [ 60] = 0x002e, /* period */
    [ 61] = 0x002f, /* slash */
    [ 62] = 0xffe2, /* Shift_R */
    [ 64] = 0xffe9, /* Alt_L */
    [ 65] = 0x0020, /* space */
    [ 66] = 0xffe5, /* Caps_Lock */
    [ 67] = 0xffbe, /* F1 */  [ 68] = 0xffbf, /* F2 */  [ 69] = 0xffc0, /* F3 */
    [ 70] = 0xffc1, /* F4 */  [ 71] = 0xffc2, /* F5 */  [ 72] = 0xffc3, /* F6 */
    [ 73] = 0xffc4, /* F7 */  [ 74] = 0xffc5, /* F8 */  [ 75] = 0xffc6, /* F9 */
    [ 76] = 0xffc7, /* F10 */
    [ 77] = 0xff7f, /* Num_Lock */
    [ 78] = 0xff14, /* Scroll_Lock */
    [ 95] = 0xffc8, /* F11 */  [ 96] = 0xffc9, /* F12 */
    [105] = 0xffe4, /* Control_R */
    [108] = 0xffea, /* Alt_R */
    [110] = 0xff50, /* Home */
    [111] = 0xff52, /* Up */
    [112] = 0xff55, /* Page_Up */
    [113] = 0xff51, /* Left */
    [114] = 0xff53, /* Right */
    [115] = 0xff57, /* End */
    [116] = 0xff54, /* Down */
    [117] = 0xff56, /* Page_Down */
    [118] = 0xff63, /* Insert */
    [119] = 0xffff, /* Delete */
    [133] = 0xffeb, /* Super_L */
    [134] = 0xffec, /* Super_R */
};


#include <stdlib.h>
KS_ *XGetKeyboardMapping(void *dpy, unsigned char first, int count, int *per) {
    const int n = 2;
    (void)dpy;
    if (per) *per = n;
    if (count < 1) count = 1;
    KS_ *r = (KS_ *)calloc((size_t)count * n, sizeof(KS_));
    if (!r) return NULL;
    for (int i = 0; i < count; i++) {
        unsigned int kc = (unsigned int)first + (unsigned int)i;
        r[i * n] = kc < 256 ? s_xrm_kc2ks[kc] : 0;
    }
    return r;
}
