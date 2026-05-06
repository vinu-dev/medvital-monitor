#include "gui_utf8.h"

#if defined(_WIN32)

static int gui_utf8_or_ansi_to_wide(const char *src, WCHAR *dst, int dst_len)
{
    int written;

    if (dst == NULL || dst_len <= 0) {
        return 0;
    }

    dst[0] = L'\0';
    if (src == NULL) {
        return 1;
    }

    written = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                  src, -1, dst, dst_len);
    if (written > 0) {
        return 1;
    }

    written = MultiByteToWideChar(CP_ACP, 0,
                                  src, -1, dst, dst_len);
    return written > 0;
}

HWND gui_create_edit_utf8(HINSTANCE instance,
                          HWND parent,
                          int id,
                          const char *initial_text,
                          DWORD ex_style,
                          DWORD style,
                          int x,
                          int y,
                          int w,
                          int h)
{
    WCHAR wide_text[256];

    if (!gui_utf8_or_ansi_to_wide(initial_text, wide_text,
                                  (int)(sizeof(wide_text) / sizeof(wide_text[0])))) {
        return NULL;
    }

    return CreateWindowExW(ex_style, L"EDIT", wide_text,
                           style, x, y, w, h,
                           parent, (HMENU)(INT_PTR)id, instance, NULL);
}

int gui_get_control_text_utf8(HWND parent, int id, char *out, int len)
{
    HWND ctrl = GetDlgItem(parent, id);
    WCHAR wide_text[256];
    int wide_len;
    int count;

    if (out == NULL || len <= 0) {
        return 0;
    }

    out[0] = '\0';
    if (ctrl == NULL) {
        return 0;
    }

    wide_len = GetWindowTextW(ctrl, wide_text,
                              (int)(sizeof(wide_text) / sizeof(wide_text[0])));
    if (wide_len <= 0) {
        return 0;
    }

    for (count = wide_len; count > 0; --count) {
        int written = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                          wide_text, count,
                                          out, len - 1,
                                          NULL, NULL);
        if (written > 0) {
            out[written] = '\0';
            return written;
        }
    }

    return 0;
}

int gui_set_control_text_utf8(HWND parent, int id, const char *text)
{
    HWND ctrl = GetDlgItem(parent, id);
    WCHAR wide_text[256];

    if (ctrl == NULL) {
        return 0;
    }

    if (!gui_utf8_or_ansi_to_wide(text, wide_text,
                                  (int)(sizeof(wide_text) / sizeof(wide_text[0])))) {
        return 0;
    }

    return SetWindowTextW(ctrl, wide_text) != 0;
}

#endif
