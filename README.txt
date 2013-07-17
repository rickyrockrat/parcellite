This release addresses most of the bugs in the bug list (see change log below).

It adds fifo_cmd, special characters for spaces/newlines/etc in menu, and added keyboard input
(much like mouse input).

This should fix the problem with it not running in Unity for Raring, though I suspect Unity is a 
moving target.


V1.1.5 Wednesday, July 17 2013
 +Added ppa script.
 +Updated German Translation from patch by Christoph Wickert.
 +Fixed bug 79, reverse order doesn't work.
 +Added fifo_cmd, and stop_all, run_all commands.
 +Fixed bug 82, Parcellite sometimes eats about twenty last characters from a an entry in history.
 +Fixed bug where item length reverts to 50 if > 75. Now uses DEF_ITEM_LENGTH_MAX.
 +Added Frame creation to add_section.
 +Tentative Fix for bug 81, Cannot run for  multiple users, on multiple X sessions.
 +Fixed bug 83, Ctrl+c/v does not work for blender.
 +Added automake rule to set svn version each time svn version is built.
 +Fixed svnversion to use local copy.
 +Added Clipboard standard usage notes to main.c.
 +Fixed Bug 85, Garbage in clipboard when app is closed.
 +Fixed Bug 86, Parcellite not putting history text in clipboard.
 +Fixed Bug 87,Error converting selection from UTF8_STRING fills X log.
 +Improved documentation for Actions.
 +Fix bug 88, Parcellite does not work with appindicator in Raring.
 +Fix bug 92, Ctrl-Shift/Right-Click followed by Clear produces SegFault/Double Free.
 +Added Keyboard Input feature.
 +Fixed bug #94, spam stdout with Looking for 'indicator-menu'.
 +Fixed bug 97, inkscape doesn't work (other objects besides text overwritten from clip).
 +Added Restore Empty option in preferences, and added -v/--version option.
 +Fixed bug 89, odd behaviour with gnome-terminal search. 
 +Fixed bug 91, gnome-terminal and deselect bug (again).
 +Fixed bug 96, entries with lots of tabs too wide/feature-request, alternate non-printing display.
 +Added debug_update preference (but currently disabled via DEBUG_UPDATE in parcellite.h.
 +Fixed GTK warning about GTK_IS_WIDGET/GTK_IS_WINDOW when history window closes.
 +Changed mulituser logic to look at owner of PID dir in /proc (added pid_to_uid).
 +Added XDG_SESSION_COOKIE check to see if we are in the same X session for multi-users.
 +Added gpg key for deb build (does not effect application).
 
