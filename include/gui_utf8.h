#ifndef GUI_UTF8_H
#define GUI_UTF8_H

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

HWND gui_create_edit_utf8(HINSTANCE instance,
                          HWND parent,
                          int id,
                          const char *initial_text,
                          DWORD ex_style,
                          DWORD style,
                          int x,
                          int y,
                          int w,
                          int h);

int gui_get_control_text_utf8(HWND parent, int id, char *out, int len);

int gui_set_control_text_utf8(HWND parent, int id, const char *text);

#ifdef __cplusplus
}
#endif

#endif

#endif
