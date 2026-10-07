#!/usr/bin/env python3
"""Check the actual clipboard helpers with fictional data and no X display."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile


source_path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[1] / 'src/main.c'
source = source_path.read_text()
functions = []
for name in ('_update_clipboard', 'update_clipboard'):
    match = re.search(r'^gchar \*' + name + r'\s*\([^;]*?\n\{.*?^\}', source, re.M | re.S)
    assert match, name
    functions.append(match.group())
defines = '\n'.join('#define ' + name + ' ' + value for name, value in
                    re.findall(r'^#define (H_MODE_\w+)\s+(0x[\da-fA-F]+|\d+)', source, re.M))
fixture = r'''
#include <glib.h>
#include <stdint.h>
#include <string.h>
typedef void GtkClipboard;
typedef unsigned GdkModifierType;
static int primary_token, clipboard_token;
static GtkClipboard *primary = &primary_token, *clipboard = &clipboard_token;
static int ignore_clipboard, use_primary = 1, use_copy = 1, restore_empty;
static int current_on_top, writes, reads, appends, guard;
static char *delivered;
static uintptr_t freed_address;
static GList *history_list;
struct history_item { char *text; };
#define DTRACE(x)
#define GDK_BUTTON1_MASK 1
#define GDK_SHIFT_MASK 2
#define HIST_DEL 16
#define HIST_CHECKDUP 1
#define HIST_KEEP_FLAGS 2
#define CLIP_TYPE_TEXT 1
static void gtk_clipboard_set_text(GtkClipboard *c, const char *text, int size) {
    g_free(delivered);
    delivered = g_strdup(text);
    writes++;
}
static void gdk_window_get_pointer(void *w, void *x, void *y, GdkModifierType *s) { *s = 0; }
static int p_strcmp(const char *a, const char *b) { return g_strcmp0(a, b); }
static void append_item(char *text, int flags, int extra, int type) { appends++; }
static char *is_clipboard_empty(GtkClipboard *c) { reads++; return g_strdup(delivered); }
static int content_exists(GtkClipboard *c) { return delivered != NULL; }
static glong validate_utf8_text(char *text, glong size) { return size; }
static int do_we_set_clipboard(GtkClipboard *c, char *text, char **out, int *mod) {
    *out = text; *mod = 0; return 0;
}
static void checked_free(void *p) {
    if (guard) freed_address = (uintptr_t)p;
    g_free(p);
}
static char *checked_strdup(const char *p) {
    g_assert(!guard || !p || (uintptr_t)p != freed_address);
    return g_strdup(p);
}
#undef g_strdup
#define g_strdup checked_strdup
#define g_free checked_free
'''
fixture += defines + '\n' + '\n'.join(functions)
fixture += r'''
int main(void) {
    char *old = g_strdup("fictional restore payload");
    guard = 1;
    char *updated = _update_clipboard(clipboard, old, &old, 0, 0);
    g_assert(updated == old);
    g_assert_cmpstr(old, ==, "fictional restore payload");
    guard = 0;
    g_free(old);

    const char *text = "fictional selected text\nsecond line\n\n";
    update_clipboard(clipboard, (char *)text, H_MODE_LIST);
    g_assert_cmpstr(delivered, ==, text);
    g_assert_cmpint(appends, ==, 1);
    for (int state = 0; state < 2; state++) {
        g_free(delivered);
        delivered = state ? NULL : g_strdup("");
        int before = writes;
        update_clipboard(clipboard, (char *)text, H_MODE_LIST);
        g_assert_cmpstr(delivered, ==, text);
        g_assert_cmpint(writes, ==, before + 1);
        g_assert_cmpint(appends, ==, 1);
        g_assert_cmpint(reads, ==, 0);
    }
    int before = writes;
    use_copy = 0;
    update_clipboard(clipboard, (char *)text, H_MODE_LIST);
    g_assert_cmpint(writes, ==, before);
    use_primary = 0;
    update_clipboard(primary, (char *)text, H_MODE_LIST);
    g_assert_cmpint(writes, ==, before);
    g_free(delivered);
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix='parcellite-clipboard-test-') as directory:
    root = Path(directory)
    (root / 'fixture.c').write_text(fixture)
    flags = subprocess.check_output(['pkg-config', '--cflags', '--libs', 'glib-2.0'], text=True).split()
    subprocess.run(['cc', str(root / 'fixture.c'), '-o', str(root / 'fixture'), *flags], check=True)
    subprocess.run([str(root / 'fixture')], check=True)
print('Clipboard update checks passed: alias safety, reselection, exact text, disabled selections.')
