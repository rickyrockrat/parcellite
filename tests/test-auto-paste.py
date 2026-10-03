#!/usr/bin/env python3
"""Exercise the real paste function with synthetic text and a recording xdotool."""
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile


source = (Path(__file__).resolve().parents[1] / 'src/main.c').read_text()
match = re.search(r'^void set_clipboard_text\([^;]*?\n\{.*?^\}', source, re.M | re.S)
assert match, 'set_clipboard_text'

fixture = r'''
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define DTRACE(x)
#define H_MODE_LIST 0
#define g_signal_emit_by_name(...) ((void)0)
struct history_info { GList *delete_list; void *menu; };
struct history_item { char *text; };
static int use_copy = 1, use_primary = 1;
static void *clipboard, *primary;
static int mode, finished;
static void *find_h_item(GList *a, void *b, GList *c) { return NULL; }
static char *p_strdup(const char *s) { return g_strdup(s); }
static void update_clipboard(void *a, char *b, int c) {}
static int get_pref_int32(const char *name) {
    if(!strcmp(name, "automatic_paste")) return mode != 0;
    if(!strcmp(name, "auto_mouse")) return mode == 2;
    if(!strcmp(name, "auto_key")) return mode == 1;
    if(!strcmp(name, "key_input")) return mode == 3;
    return 0;
}
static void action_exit(GPid pid, gint status, gpointer data) {
    finished = 1;
    g_spawn_close_pid(pid);
}
static gboolean test_spawn(const gchar *cwd, gchar **argv, gchar **env,
                           GSpawnFlags flags, GSpawnChildSetupFunc setup,
                           gpointer data, GPid *pid, GError **error) {
    if(g_getenv("TEST_SPAWN_FAIL")) {
        g_set_error_literal(error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED, "Synthetic launch failure");
        return FALSE;
    }
    return g_spawn_async(cwd, argv, env, flags, setup, data, pid, error);
}
#define g_spawn_async test_spawn
'''
fixture += match.group()
fixture += r'''
int main(int argc, char **argv) {
    struct history_item item;
    struct history_info history = {0};
    if(!g_file_get_contents(argv[1], &item.text, NULL, NULL)) return 2;
    mode = atoi(argv[2]);
    if(argc > 3) use_copy = use_primary = 0;
    GList *list = g_list_append(NULL, &item);
    set_clipboard_text(&history, list);
    gint64 end = g_get_monotonic_time() + 3 * G_USEC_PER_SEC;
    while(mode && !finished && g_get_monotonic_time() < end) {
        g_main_context_iteration(NULL, FALSE);
        g_usleep(5000);
    }
    g_list_free(list);
    g_free(item.text);
    return mode && !finished ? 3 : 0;
}
'''

with tempfile.TemporaryDirectory(prefix='parcellite-paste-test-') as directory:
    root = Path(directory)
    (root / 'fixture.c').write_text(fixture)
    flags = subprocess.check_output(['pkg-config', '--cflags', '--libs', 'glib-2.0'], text=True).split()
    subprocess.run(['cc', str(root / 'fixture.c'), '-o', str(root / 'fixture'), *flags], check=True)
    recorder = root / 'xdotool'
    recorder.write_text('#!/usr/bin/env python3\nimport json,os,sys\n'
                        'open(os.environ["RECORD"],"w").write(json.dumps(sys.argv[1:]))\n')
    recorder.chmod(0o700)
    env = dict(os.environ, PATH=str(root) + ':' + os.environ['PATH'], RECORD=str(root / 'args'),
               PARCELLITE_TEXT='stale inherited value')
    payload = root / 'payload'

    def run(mode, text):
        (root / 'args').unlink(missing_ok=True)
        payload.write_text(text)
        subprocess.run([str(root / 'fixture'), str(payload), str(mode)], env=env, check=True, timeout=5)
        return json.loads((root / 'args').read_text()) if (root / 'args').exists() else None

    assert run(0, 'disabled') is None
    assert run(1, 'clipboard paste') == ['key', '--clearmodifiers', 'ctrl+v']
    assert run(2, 'primary paste') == ['click', '--clearmodifiers', '2']
    for mode in (1, 2):
        (root / 'args').unlink(missing_ok=True)
        payload.write_text('must not paste stale clipboard text')
        blocked = subprocess.run([str(root / 'fixture'), str(payload), str(mode), 'disabled'],
                                 env=env, text=True, capture_output=True, timeout=5)
        assert blocked.returncode == 3 and 'matching clipboard selection' in blocked.stderr
        assert not (root / 'args').exists()
    for text in ('plain', 'He said "hello"; it\'s literal.', '--help',
                 'line one\nline two\tend', '\u00f1\u4e2d\U0001f642',
                 '$(touch SHOULD_NOT_EXIST) `echo untouched` $HOME \\'):
        args = run(3, text)
        assert args[:3] == ['type', '--clearmodifiers', '--'], args
        assert args[-1] == text, (text, args)
    (root / 'args').unlink()
    failed = subprocess.run([str(root / 'fixture'), str(payload), '3'],
                            env=dict(env, TEST_SPAWN_FAIL='1'), text=True,
                            capture_output=True, timeout=5)
    assert failed.returncode == 3 and 'Cannot start auto-paste' in failed.stderr, failed.stderr
    assert 'CRITICAL' not in failed.stderr and not (root / 'args').exists()
    print('PASS: disabled, key/mouse routes, exact typed text, and launch-failure handling')
