#pragma once
#include "PluginInternal.h"
#include "ysfx.h"
#include <array>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
namespace cydsp::detail {
class JsfxEditor {
    ysfx_t* effect_;
    HWND window_ = nullptr;
    std::vector<std::uint8_t> pixels_;
    PluginEditorSize size_{320, 200};
    double scale_ = 1;
    std::uint32_t buttons_ = 0;
    static LRESULT CALLBACK proc(HWND w, UINT m, WPARAM a, LPARAM b) {
        auto* self = reinterpret_cast<JsfxEditor*>(GetWindowLongPtrW(w, GWLP_USERDATA));
        if (m == WM_NCCREATE) {
            self = static_cast<JsfxEditor*>(reinterpret_cast<CREATESTRUCTW*>(b)->lpCreateParams);
            SetWindowLongPtrW(w, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (!self)
            return DefWindowProcW(w, m, a, b);
        switch (m) {
        case WM_PAINT: {
            PAINTSTRUCT p;
            auto dc = BeginPaint(w, &p);
            BITMAPINFO info{};
            info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            info.bmiHeader.biWidth = LONG(self->size_.width);
            info.bmiHeader.biHeight = -LONG(self->size_.height);
            info.bmiHeader.biPlanes = 1;
            info.bmiHeader.biBitCount = 32;
            info.bmiHeader.biCompression = BI_RGB;
            StretchDIBits(dc, 0, 0, int(self->size_.width), int(self->size_.height), 0, 0,
                          int(self->size_.width), int(self->size_.height), self->pixels_.data(),
                          &info, DIB_RGB_COLORS, SRCCOPY);
            EndPaint(w, &p);
            return 0;
        }
        case WM_LBUTTONDOWN:
            self->buttons_ |= ysfx_button_left;
            SetCapture(w);
            break;
        case WM_LBUTTONUP:
            self->buttons_ &= ~ysfx_button_left;
            ReleaseCapture();
            break;
        case WM_MBUTTONDOWN:
            self->buttons_ |= ysfx_button_middle;
            SetCapture(w);
            break;
        case WM_MBUTTONUP:
            self->buttons_ &= ~ysfx_button_middle;
            ReleaseCapture();
            break;
        case WM_RBUTTONDOWN:
            self->buttons_ |= ysfx_button_right;
            SetCapture(w);
            break;
        case WM_RBUTTONUP:
            self->buttons_ &= ~ysfx_button_right;
            ReleaseCapture();
            break;
        case WM_CHAR:
            ysfx_gfx_add_key(self->effect_, mods(), std::uint32_t(a), true);
            return 0;
        case WM_ERASEBKGND:
            return 1;
        default:
            break;
        }
        if (m == WM_MOUSEMOVE || m == WM_LBUTTONDOWN || m == WM_LBUTTONUP || m == WM_RBUTTONDOWN ||
            m == WM_RBUTTONUP || m == WM_MBUTTONDOWN || m == WM_MBUTTONUP || m == WM_MOUSEWHEEL ||
            m == WM_MOUSEHWHEEL) {
            POINT pt{SHORT(LOWORD(b)), SHORT(HIWORD(b))};
            auto wheel = m == WM_MOUSEWHEEL ? double(SHORT(HIWORD(a))) / 120 : 0.;
            auto horizontal = m == WM_MOUSEHWHEEL ? double(SHORT(HIWORD(a))) / 120 : 0.;
            if (m == WM_MOUSEWHEEL || m == WM_MOUSEHWHEEL)
                ScreenToClient(w, &pt);
            ysfx_gfx_update_mouse(self->effect_, mods(), pt.x, pt.y, self->buttons_, wheel,
                                  horizontal);
            return 0;
        }
        return DefWindowProcW(w, m, a, b);
    }
    static std::uint32_t mods() noexcept {
        return (GetKeyState(VK_SHIFT) < 0 ? ysfx_mod_shift : 0u) |
               (GetKeyState(VK_CONTROL) < 0 ? ysfx_mod_ctrl : 0u) |
               (GetKeyState(VK_MENU) < 0 ? ysfx_mod_alt : 0u) |
               ((GetKeyState(VK_LWIN) < 0 || GetKeyState(VK_RWIN) < 0) ? ysfx_mod_super : 0u);
    }
    static std::int32_t menu(void* p, const char* text, std::int32_t x, std::int32_t y) {
        auto& self = *static_cast<JsfxEditor*>(p);
        auto* parsed = ysfx_parse_menu(text);
        if (!parsed)
            return 0;
        struct Cleanup {
            ysfx_menu_t* p;
            ~Cleanup() {
                ysfx_menu_free(p);
            }
        } cleanup{parsed};
        if (parsed->insn_count > 4096)
            return 0;
        HMENU root = CreatePopupMenu();
        if (!root)
            return 0;
        std::array<HMENU, 64> stack{};
        stack[0] = root;
        std::size_t depth = 0;
        for (std::size_t i = 0; i < parsed->insn_count; ++i) {
            auto& op = parsed->insns[i];
            std::wstring label;
            if (op.name) {
                int n = MultiByteToWideChar(CP_UTF8, 0, op.name, -1, nullptr, 0);
                if (n > 0) {
                    label.resize(std::size_t(n));
                    MultiByteToWideChar(CP_UTF8, 0, op.name, -1, label.data(), n);
                }
            }
            UINT flags = (op.item_flags & ysfx_menu_item_disabled ? MF_GRAYED : 0u) |
                         (op.item_flags & ysfx_menu_item_checked ? MF_CHECKED : 0u);
            if (op.opcode == ysfx_menu_item)
                AppendMenuW(stack[depth], flags, op.id, label.c_str());
            else if (op.opcode == ysfx_menu_separator)
                AppendMenuW(stack[depth], MF_SEPARATOR, 0, nullptr);
            else if (op.opcode == ysfx_menu_sub && depth + 1 < stack.size()) {
                auto child = CreatePopupMenu();
                if (child) {
                    AppendMenuW(stack[depth], MF_POPUP | flags, reinterpret_cast<UINT_PTR>(child),
                                label.c_str());
                    stack[++depth] = child;
                }
            } else if (op.opcode == ysfx_menu_endsub && depth)
                --depth;
        }
        POINT pt{x, y};
        ClientToScreen(self.window_, &pt);
        int selected = TrackPopupMenu(root, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0,
                                      self.window_, nullptr);
        DestroyMenu(root);
        return selected;
    }
    void configure() {
        ysfx_gfx_config_t config{};
        config.user_data = this;
        config.pixel_width = size_.width;
        config.pixel_height = size_.height;
        config.pixels = pixels_.data();
        config.scale_factor = scale_;
        config.show_menu = menu;
        ysfx_gfx_setup(effect_, &config);
    }

  public:
    explicit JsfxEditor(ysfx_t* effect) : effect_(effect) {}
    ~JsfxEditor() {
        close();
    }
    Status open(const PluginWindow& w, PluginEditorSize& size) {
        if (w.api != PluginWindowApi::win32 || w.floating)
            return Status::unsupported_backend;
        std::uint32_t dim[2]{};
        ysfx_get_gfx_dim(effect_, dim);
        size_ = {dim[0] ? dim[0] : 320, dim[1] ? dim[1] : 200};
        scale_ = w.scale;
        if (size_.width > 4096 || size_.height > 4096)
            return Status::invalid_spec;
        pixels_.resize(std::size_t(size_.width) * size_.height * 4);
        configure();
        WNDCLASSW cls{};
        cls.hInstance = GetModuleHandleW(nullptr);
        cls.lpfnWndProc = proc;
        cls.lpszClassName = L"CyDSP.JSFX.Canvas.1";
        cls.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        if (!RegisterClassW(&cls) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return Status::io_error;
        window_ = CreateWindowExW(0, cls.lpszClassName, L"", WS_CHILD | WS_VISIBLE, 0, 0,
                                  int(size_.width), int(size_.height),
                                  reinterpret_cast<HWND>(w.handle), nullptr, cls.hInstance, this);
        if (!window_)
            return Status::io_error;
        size = size_;
        pump();
        return Status::ok;
    }
    Status resize(PluginEditorSize& size) {
        if (!window_)
            return Status::not_prepared;
        if (size.width > 4096 || size.height > 4096)
            return Status::invalid_spec;
        std::vector<std::uint8_t> next(std::size_t(size.width) * size.height * 4);
        pixels_.swap(next);
        size_ = size;
        configure();
        MoveWindow(window_, 0, 0, int(size.width), int(size.height), TRUE);
        return Status::ok;
    }
    Status scale(double scale) {
        scale_ = scale;
        configure();
        return Status::ok;
    }
    Status show(bool show) {
        if (!window_)
            return Status::not_prepared;
        ShowWindow(window_, show ? SW_SHOW : SW_HIDE);
        return Status::ok;
    }
    Status focus(bool on) {
        if (on)
            SetFocus(window_);
        else if (GetFocus() == window_)
            SetFocus(GetParent(window_));
        return Status::ok;
    }
    Status key(const PluginEditorKey& key) {
        std::uint32_t k = key.character;
        if (key.key >= PluginEditorKeyCode::f1 && key.key <= PluginEditorKeyCode::f12)
            k = ysfx_key_f1 + std::uint32_t(key.key) - std::uint32_t(PluginEditorKeyCode::f1);
        else if (key.key != PluginEditorKeyCode::none) {
            switch (key.key) {
            case PluginEditorKeyCode::backspace:
                k = ysfx_key_backspace;
                break;
            case PluginEditorKeyCode::escape:
                k = ysfx_key_escape;
                break;
            case PluginEditorKeyCode::deleteKey:
                k = ysfx_key_delete;
                break;
            case PluginEditorKeyCode::left:
                k = ysfx_key_left;
                break;
            case PluginEditorKeyCode::right:
                k = ysfx_key_right;
                break;
            case PluginEditorKeyCode::up:
                k = ysfx_key_up;
                break;
            case PluginEditorKeyCode::down:
                k = ysfx_key_down;
                break;
            case PluginEditorKeyCode::pageUp:
                k = ysfx_key_page_up;
                break;
            case PluginEditorKeyCode::pageDown:
                k = ysfx_key_page_down;
                break;
            case PluginEditorKeyCode::home:
                k = ysfx_key_home;
                break;
            case PluginEditorKeyCode::end:
                k = ysfx_key_end;
                break;
            case PluginEditorKeyCode::insert:
                k = ysfx_key_insert;
                break;
            case PluginEditorKeyCode::enter:
                k = 13;
                break;
            case PluginEditorKeyCode::tab:
                k = 9;
                break;
            case PluginEditorKeyCode::space:
                k = 32;
                break;
            default:
                return Status::unsupported_format;
            }
        }
        std::uint32_t m = (key.modifiers & editorShift ? ysfx_mod_shift : 0u) |
                          (key.modifiers & editorControl ? ysfx_mod_ctrl : 0u) |
                          (key.modifiers & editorAlt ? ysfx_mod_alt : 0u) |
                          (key.modifiers & editorSuper ? ysfx_mod_super : 0u);
        ysfx_gfx_add_key(effect_, m, k, key.down);
        return Status::ok;
    }
    Status wheel(float value) {
        POINT pt{};
        GetCursorPos(&pt);
        ScreenToClient(window_, &pt);
        ysfx_gfx_update_mouse(effect_, mods(), pt.x, pt.y, buttons_, value, 0);
        return Status::ok;
    }
    void pump() {
        if (window_ && ysfx_gfx_run(effect_))
            InvalidateRect(window_, nullptr, FALSE);
    }
    void close() noexcept {
        if (window_) {
            DestroyWindow(window_);
            window_ = nullptr;
        }
    }
};
} // namespace cydsp::detail
#endif
