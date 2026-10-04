#define NOMINMAX
#include <windows.h>
#include <gdiplus.h>
#include <windowsx.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "cgui.hpp"





using namespace Gdiplus;

namespace
{
    constexpr int DEFAULT_WIDTH = 800;
    constexpr int DEFAULT_HEIGHT = 600;
    constexpr int DEFAULT_FONT_SIZE = 24;

    std::wstring toWide(const std::string& text)
    {
        if (text.empty())
            return {};

        const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
        if (size <= 0)
            return {};

        std::wstring result(static_cast<size_t>(size), L'\0');
        if (MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, result.data(), size) <= 0)
            return {};
        result.resize(static_cast<size_t>(size - 1));
        return result;
    }

    int clampColor(int value)
    {
        return std::clamp(value, 0, 255);
    }

    Color makeColor(int r, int g, int b, int a = 255)
    {
        return Color(
            static_cast<BYTE>(clampColor(a)),
            static_cast<BYTE>(clampColor(r)),
            static_cast<BYTE>(clampColor(g)),
            static_cast<BYTE>(clampColor(b)));
    }

    bool fileExists(const std::string& path)
    {
        const std::wstring widePath = toWide(path);
        if (widePath.empty())
            return false;

        const DWORD attributes = GetFileAttributesW(widePath.c_str());
        return attributes != INVALID_FILE_ATTRIBUTES &&
               (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
    }

    std::string upper(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        return value;
    }

    int virtualKeyFromString(const std::string& key)
    {
        const std::string k = upper(key);

        if (k.size() == 1)
        {
            const unsigned char c = static_cast<unsigned char>(k[0]);
            if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
                return c;
        }

        if (k == "SPACE") return VK_SPACE;
        if (k == "ENTER" || k == "RETURN") return VK_RETURN;
        if (k == "ESC" || k == "ESCAPE") return VK_ESCAPE;
        if (k == "TAB") return VK_TAB;
        if (k == "BACKSPACE") return VK_BACK;
        if (k == "SHIFT") return VK_SHIFT;
        if (k == "CTRL" || k == "CONTROL") return VK_CONTROL;
        if (k == "ALT") return VK_MENU;
        if (k == "LEFT") return VK_LEFT;
        if (k == "RIGHT") return VK_RIGHT;
        if (k == "UP") return VK_UP;
        if (k == "DOWN") return VK_DOWN;
        if (k == "INSERT") return VK_INSERT;
        if (k == "DELETE") return VK_DELETE;
        if (k == "HOME") return VK_HOME;
        if (k == "END") return VK_END;
        if (k == "PAGEUP") return VK_PRIOR;
        if (k == "PAGEDOWN") return VK_NEXT;
        if (k == "CAPSLOCK") return VK_CAPITAL;
        if (k == "LWIN" || k == "WIN") return VK_LWIN;
        if (k == "RWIN") return VK_RWIN;
        if (k == "NUM0") return VK_NUMPAD0;
        if (k == "NUM1") return VK_NUMPAD1;
        if (k == "NUM2") return VK_NUMPAD2;
        if (k == "NUM3") return VK_NUMPAD3;
        if (k == "NUM4") return VK_NUMPAD4;
        if (k == "NUM5") return VK_NUMPAD5;
        if (k == "NUM6") return VK_NUMPAD6;
        if (k == "NUM7") return VK_NUMPAD7;
        if (k == "NUM8") return VK_NUMPAD8;
        if (k == "NUM9") return VK_NUMPAD9;

        if (k.size() >= 2 && k[0] == 'F')
        {
            try
            {
                const int number = std::stoi(k.substr(1));
                if (number >= 1 && number <= 24)
                    return VK_F1 + (number - 1);
            }
            catch (...) {}
        }

        return 0;
    }

    bool isMouseLeft(const std::string& name)
    {
        const std::string value = upper(name);
        return value == "LEFT" || value == "LMB" || value == "MOUSE1";
    }

    bool isMouseRight(const std::string& name)
    {
        const std::string value = upper(name);
        return value == "RIGHT" || value == "RMB" || value == "MOUSE2";
    }
}

struct CGUI::Impl
{
    struct SpriteState
    {
        std::unique_ptr<Image> image;
        std::string path;
        int x = 0;
        int y = 0;
        float scaleX = 1.0f;
        float scaleY = 1.0f;
        float rotation = 0.0f;
        bool visible = true;
        bool flipX = false;
        bool flipY = false;
        Color tint = Color(255, 255, 255, 255);
    };

    struct ButtonState
    {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
    };

    struct WindowState
    {
        std::string name;
        HWND hwnd = nullptr;
        int width = DEFAULT_WIDTH;
        int height = DEFAULT_HEIGHT;
        bool resizable = true;
        Color background = Color(255, 30, 30, 30);
        std::unique_ptr<Bitmap> backBuffer;
        std::unordered_map<std::string, SpriteState> sprites;
        std::unordered_map<std::string, ButtonState> buttons;
        int mouseX = 0;
        int mouseY = 0;
    };

    ULONG_PTR gdiplusToken = 0;
    bool appRunning = true;
    bool frameOpen = false;

    // Key state is intentionally tiny and frame based.
    bool keysDown[256]{};
    bool keysPressed[256]{};
    bool mouseDownLeft = false;
    bool mouseDownRight = false;
    bool mousePressedLeft = false;
    bool mousePressedRight = false;
    HWND clickWindow = nullptr;
    POINT clickPoint{};

    std::unordered_map<std::string, std::unique_ptr<WindowState>> windows;
    WindowState* current = nullptr;

    std::string currentFontName = "Arial";
    std::string currentFontPath;
    int currentFontSize = DEFAULT_FONT_SIZE;
    std::unique_ptr<PrivateFontCollection> privateFonts;

    std::once_flag classRegistration;

    Impl()
    {
        GdiplusStartupInput input;
        if (GdiplusStartup(&gdiplusToken, &input, nullptr) != Ok)
        {
            gdiplusToken = 0;
            std::cerr << "CGUI+ ERROR: GDI+ could not be started.\n";
        }
    }

    ~Impl()
    {
        appRunning = false;
        frameOpen = false;

        for (auto& [name, windowState] : windows)
        {
            if (windowState->hwnd)
                DestroyWindow(windowState->hwnd);
        }
        windows.clear();
        current = nullptr;
        privateFonts.reset();

        if (gdiplusToken != 0)
            GdiplusShutdown(gdiplusToken);
    }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        Impl* self = reinterpret_cast<Impl*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        if (message == WM_NCCREATE)
        {
            const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = static_cast<Impl*>(create->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }

        if (!self)
            return DefWindowProcW(hwnd, message, wParam, lParam);

        WindowState* windowState = self->findWindow(hwnd);

        switch (message)
        {
            case WM_CLOSE:
                DestroyWindow(hwnd);
                return 0;

            case WM_DESTROY:
                if (self->windows.size() <= 1)
                    self->appRunning = false;
                return 0;

            case WM_ERASEBKGND:
                return 1;

            case WM_PAINT:
            {
                PAINTSTRUCT paint{};
                HDC dc = BeginPaint(hwnd, &paint);
                if (windowState && windowState->backBuffer)
                {
                    Graphics graphics(dc);
                    graphics.DrawImage(windowState->backBuffer.get(), 0, 0,
                                       windowState->width, windowState->height);
                }
                EndPaint(hwnd, &paint);
                return 0;
            }

            case WM_SIZE:
                if (windowState)
                {
                    windowState->width = std::max(1, static_cast<int>(LOWORD(lParam)));
                    windowState->height = std::max(1, static_cast<int>(HIWORD(lParam)));
                    self->createBackBuffer(*windowState);
                }
                return 0;

            case WM_MOUSEMOVE:
                if (windowState)
                {
                    windowState->mouseX = GET_X_LPARAM(lParam);
                    windowState->mouseY = GET_Y_LPARAM(lParam);
                }
                return 0;

            case WM_LBUTTONDOWN:
                self->mouseDownLeft = true;
                self->mousePressedLeft = true;
                self->clickWindow = hwnd;
                self->clickPoint.x = GET_X_LPARAM(lParam);
                self->clickPoint.y = GET_Y_LPARAM(lParam);
                SetCapture(hwnd);
                return 0;

            case WM_LBUTTONUP:
                self->mouseDownLeft = false;
                if (GetCapture() == hwnd)
                    ReleaseCapture();
                return 0;

            case WM_RBUTTONDOWN:
                self->mouseDownRight = true;
                self->mousePressedRight = true;
                SetCapture(hwnd);
                return 0;

            case WM_RBUTTONUP:
                self->mouseDownRight = false;
                if (GetCapture() == hwnd)
                    ReleaseCapture();
                return 0;

            case WM_KEYDOWN:
            {
                const int vk = static_cast<int>(wParam);
                if (vk >= 0 && vk < 256)
                {
                    if ((lParam & (1LL << 30)) == 0)
                        self->keysPressed[vk] = true;
                    self->keysDown[vk] = true;
                }
                return 0;
            }

            case WM_KEYUP:
            {
                const int vk = static_cast<int>(wParam);
                if (vk >= 0 && vk < 256)
                    self->keysDown[vk] = false;
                return 0;
            }

            case WM_KILLFOCUS:
                std::fill(std::begin(self->keysDown), std::end(self->keysDown), false);
                self->mouseDownLeft = false;
                self->mouseDownRight = false;
                if (GetCapture() == hwnd)
                    ReleaseCapture();
                return 0;

            case WM_NCDESTROY:
                if (windowState && self->current == windowState)
                {
                    self->current = nullptr;
                    for (auto& [name, candidate] : self->windows)
                    {
                        if (candidate.get() != windowState && candidate->hwnd)
                        {
                            self->current = candidate.get();
                            break;
                        }
                    }
                }
                if (windowState)
                    windowState->hwnd = nullptr;
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
                return DefWindowProcW(hwnd, message, wParam, lParam);
        }

        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    WindowState* findWindow(HWND hwnd)
    {
        for (auto& [name, windowState] : windows)
        {
            if (windowState->hwnd == hwnd)
                return windowState.get();
        }
        return nullptr;
    }

    WindowState* findWindow(const std::string& name)
    {
        const auto it = windows.find(name);
        if (it == windows.end())
            return nullptr;
        return it->second.get();
    }

    void registerWindowClass()
    {
        std::call_once(classRegistration, [this]()
        {
            HINSTANCE instance = GetModuleHandleW(nullptr);

            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
            wc.lpfnWndProc = WndProc;
            wc.hInstance = instance;
            wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
            wc.lpszClassName = L"CGUIPlusWindowClass";

            if (!RegisterClassExW(&wc))
            {
                if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
                {
                    std::cerr << "CGUI+ ERROR: Could not register the Windows window class.\n";
                }
            }
        });
    }

    void createBackBuffer(WindowState& windowState)
    {
        if (!gdiplusToken || windowState.width <= 0 || windowState.height <= 0)
            return;

        auto buffer = std::make_unique<Bitmap>(
            static_cast<INT>(windowState.width),
            static_cast<INT>(windowState.height),
            PixelFormat32bppPARGB);

        if (buffer->GetLastStatus() != Ok)
            return;

        Graphics graphics(buffer.get());
        graphics.Clear(windowState.background);
        windowState.backBuffer = std::move(buffer);
    }

    void processMessages()
    {
        MSG message{};
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_QUIT)
            {
                appRunning = false;
                continue;
            }

            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }

    void ensureFrameStarted()
    {
        if (!frameOpen)
        {
            processMessages();
            frameOpen = true;
        }
    }

    void presentWindow(WindowState& windowState)
    {
        if (!windowState.hwnd || !windowState.backBuffer)
            return;

        HDC dc = GetDC(windowState.hwnd);
        if (!dc)
            return;

        Graphics graphics(dc);
        graphics.DrawImage(windowState.backBuffer.get(), 0, 0,
                           windowState.width, windowState.height);
        ReleaseDC(windowState.hwnd, dc);
    }

    SpriteState* findSprite(const std::string& name)
    {
        if (!current)
            return nullptr;

        auto it = current->sprites.find(name);
        if (it == current->sprites.end())
            return nullptr;
        return &it->second;
    }

    const ButtonState* findButton(const std::string& name) const
    {
        if (!current)
            return nullptr;

        const auto it = current->buttons.find(name);
        if (it == current->buttons.end())
            return nullptr;
        return &it->second;
    }

    void error(const std::string& message) const
    {
        std::cerr << "CGUI+ ERROR: " << message << '\n';
    }
};

CGUI::CGUI()
    : impl(std::make_unique<Impl>())
{
}

CGUI::~CGUI() = default;

bool CGUI::window(const std::string& name, int width, int height, bool resizable)
{
    if (name.empty())
    {
        impl->error("Window name cannot be empty.");
        return false;
    }

    if (width <= 0 || height <= 0)
    {
        impl->error("Window size must be greater than zero.");
        return false;
    }

    if (impl->findWindow(name))
    {
        impl->current = impl->findWindow(name);
        impl->error("Window '" + name + "' already exists; it is now the active window.");
        return false;
    }

    impl->registerWindowClass();

    auto state = std::make_unique<Impl::WindowState>();
    state->name = name;
    state->width = width;
    state->height = height;
    state->resizable = resizable;
    state->background = Color(255, 30, 30, 30);

    const DWORD style = resizable
        ? WS_OVERLAPPEDWINDOW
        : (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);

    RECT clientRect{0, 0, width, height};
    AdjustWindowRectEx(&clientRect, style, FALSE, 0);

    const std::wstring title = toWide(name);
    HWND hwnd = CreateWindowExW(
        0,
        L"CGUIPlusWindowClass",
        title.c_str(),
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top,
        nullptr,
        nullptr,
        GetModuleHandleW(nullptr),
        impl.get());

    if (!hwnd)
    {
        impl->error("Could not create window '" + name + "'.");
        return false;
    }

    state->hwnd = hwnd;
    impl->windows.emplace(name, std::move(state));
    impl->current = impl->findWindow(name);

    impl->createBackBuffer(*impl->current);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    return true;
}

bool CGUI::window(const CGUIDefaultSettings&)
{
    return window("CGUI+ Window", DEFAULT_WIDTH, DEFAULT_HEIGHT, true);
}

bool CGUI::awindow(const std::string& name, int width, int height, bool resizable)
{
    if (width <= 0 || height <= 0)
    {
        impl->error("Window size must be greater than zero.");
        return false;
    }

    Impl::WindowState* state = impl->findWindow(name);
    if (!state)
    {
        impl->error("Window '" + name + "' was not found.");
        return false;
    }

    DWORD style = resizable
        ? WS_OVERLAPPEDWINDOW
        : (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);

    SetWindowLongPtrW(state->hwnd, GWL_STYLE, static_cast<LONG_PTR>(style));

    RECT clientRect{0, 0, width, height};
    AdjustWindowRectEx(&clientRect, style, FALSE, 0);

    SetWindowPos(
        state->hwnd,
        nullptr,
        0,
        0,
        clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top,
        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

    state->resizable = resizable;
    state->width = width;
    state->height = height;
    impl->createBackBuffer(*state);
    impl->current = state;

    return true;
}

bool CGUI::usewindow(const std::string& name)
{
    Impl::WindowState* state = impl->findWindow(name);
    if (!state)
    {
        impl->error("Window '" + name + "' was not found.");
        return false;
    }

    impl->current = state;
    return true;
}

bool CGUI::sprite(const std::string& name, const std::string& path)
{
    if (!impl->current)
    {
        impl->error("Create a window before creating a sprite.");
        return false;
    }

    if (name.empty())
    {
        impl->error("Sprite name cannot be empty.");
        return false;
    }

    if (!fileExists(path))
    {
        impl->error("Image file '" + path + "' was not found.");
        return false;
    }

    const std::wstring widePath = toWide(path);
    std::unique_ptr<Image> image(Image::FromFile(widePath.c_str(), FALSE));

    if (!image || image->GetLastStatus() != Ok)
    {
        impl->error("Could not load image '" + path + "'.");
        return false;
    }

    Impl::SpriteState spriteState;
    spriteState.image = std::move(image);
    spriteState.path = path;

    impl->current->sprites[name] = std::move(spriteState);
    return true;
}

bool CGUI::dsprite(const std::string& name, int x, int y)
{
    if (!impl->current || !impl->current->backBuffer)
    {
        impl->error("Create a window before drawing a sprite.");
        return false;
    }

    Impl::SpriteState* spriteState = impl->findSprite(name);
    if (!spriteState)
    {
        impl->error("Sprite '" + name + "' was not found.");
        return false;
    }

    if (!spriteState->image || !spriteState->visible)
        return true;

    spriteState->x = x;
    spriteState->y = y;

    const int sourceWidth = static_cast<int>(spriteState->image->GetWidth());
    const int sourceHeight = static_cast<int>(spriteState->image->GetHeight());

    const int destinationWidth = std::max(1, static_cast<int>(sourceWidth * std::abs(spriteState->scaleX)));
    const int destinationHeight = std::max(1, static_cast<int>(sourceHeight * std::abs(spriteState->scaleY)));

    Graphics graphics(impl->current->backBuffer.get());
    graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    graphics.SetSmoothingMode(SmoothingModeHighQuality);
    graphics.SetPixelOffsetMode(PixelOffsetModeHighQuality);

    GraphicsState saved = graphics.Save();

    Matrix matrix;
    matrix.Translate(
        static_cast<REAL>(x + destinationWidth / 2.0f),
        static_cast<REAL>(y + destinationHeight / 2.0f),
        MatrixOrderAppend);
    matrix.Rotate(spriteState->rotation, MatrixOrderAppend);
    matrix.Scale(
        spriteState->flipX ? -1.0f : 1.0f,
        spriteState->flipY ? -1.0f : 1.0f,
        MatrixOrderAppend);
    matrix.Translate(
        -destinationWidth / 2.0f,
        -destinationHeight / 2.0f,
        MatrixOrderAppend);

    graphics.SetTransform(&matrix);

    ImageAttributes attributes;
    const bool tinted =
        spriteState->tint.GetRed() != 255 ||
        spriteState->tint.GetGreen() != 255 ||
        spriteState->tint.GetBlue() != 255;

    if (tinted)
    {
        ColorMatrix colorMatrix =
        {
            {
                spriteState->tint.GetRed() / 255.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, spriteState->tint.GetGreen() / 255.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, spriteState->tint.GetBlue() / 255.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f, 1.0f
            }
        };
        attributes.SetColorMatrix(&colorMatrix);
    }

    Rect destination(0, 0, destinationWidth, destinationHeight);
    if (tinted)
    {
        graphics.DrawImage(
            spriteState->image.get(),
            destination,
            0,
            0,
            sourceWidth,
            sourceHeight,
            UnitPixel,
            &attributes,
            nullptr,
            nullptr);
    }
    else
    {
        graphics.DrawImage(
            spriteState->image.get(),
            destination,
            0,
            0,
            sourceWidth,
            sourceHeight,
            UnitPixel);
    }

    graphics.Restore(saved);
    return true;
}

bool CGUI::sposition(const std::string& name, int x, int y)
{
    Impl::SpriteState* spriteState = impl->findSprite(name);
    if (!spriteState)
    {
        impl->error("Sprite '" + name + "' was not found.");
        return false;
    }
    spriteState->x = x;
    spriteState->y = y;
    return true;
}

bool CGUI::sscale(const std::string& name, float x, float y)
{
    Impl::SpriteState* spriteState = impl->findSprite(name);
    if (!spriteState)
    {
        impl->error("Sprite '" + name + "' was not found.");
        return false;
    }
    if (x == 0.0f || y == 0.0f)
    {
        impl->error("Sprite scale cannot be zero.");
        return false;
    }
    spriteState->scaleX = x;
    spriteState->scaleY = y;
    return true;
}

bool CGUI::srotate(const std::string& name, float degrees)
{
    Impl::SpriteState* spriteState = impl->findSprite(name);
    if (!spriteState)
    {
        impl->error("Sprite '" + name + "' was not found.");
        return false;
    }
    spriteState->rotation = degrees;
    return true;
}

bool CGUI::svisible(const std::string& name, bool visible)
{
    Impl::SpriteState* spriteState = impl->findSprite(name);
    if (!spriteState)
    {
        impl->error("Sprite '" + name + "' was not found.");
        return false;
    }
    spriteState->visible = visible;
    return true;
}

bool CGUI::scolor(const std::string& name, int r, int g, int b)
{
    Impl::SpriteState* spriteState = impl->findSprite(name);
    if (!spriteState)
    {
        impl->error("Sprite '" + name + "' was not found.");
        return false;
    }
    spriteState->tint = makeColor(r, g, b);
    return true;
}

bool CGUI::sflip(const std::string& name, bool horizontal, bool vertical)
{
    Impl::SpriteState* spriteState = impl->findSprite(name);
    if (!spriteState)
    {
        impl->error("Sprite '" + name + "' was not found.");
        return false;
    }
    spriteState->flipX = horizontal;
    spriteState->flipY = vertical;
    return true;
}

bool CGUI::sdelete(const std::string& name)
{
    if (!impl->current)
    {
        impl->error("Create a window before deleting a sprite.");
        return false;
    }

    const auto erased = impl->current->sprites.erase(name);
    if (!erased)
    {
        impl->error("Sprite '" + name + "' was not found.");
        return false;
    }
    return true;
}

bool CGUI::font(const std::string& name, const std::string& path, int size)
{
    if (name.empty())
    {
        impl->error("Font name cannot be empty.");
        return false;
    }
    if (size <= 0)
    {
        impl->error("Font size must be greater than zero.");
        return false;
    }

    impl->privateFonts.reset();

    if (!path.empty())
    {
        if (!fileExists(path))
        {
            impl->error("Font file '" + path + "' was not found.");
            return false;
        }

        auto privateCollection = std::make_unique<PrivateFontCollection>();
        const std::wstring widePath = toWide(path);
        if (privateCollection->AddFontFile(widePath.c_str()) != Ok)
        {
            impl->error("Could not load font file '" + path + "'.");
            return false;
        }

        const int count = privateCollection->GetFamilyCount();
        if (count <= 0)
        {
            impl->error("Font file '" + path + "' did not contain a usable font family.");
            return false;
        }

        std::vector<FontFamily> families(static_cast<size_t>(count));
        INT found = 0;
        if (privateCollection->GetFamilies(count, families.data(), &found) != Ok || found <= 0)
        {
            impl->error("Could not read the font family from '" + path + "'.");
            return false;
        }

        WCHAR familyName[LF_FACESIZE]{};
        if (families[0].GetFamilyName(familyName, LANG_NEUTRAL) != Ok)
        {
            impl->error("Could not read the font family name from '" + path + "'.");
            return false;
        }

        impl->currentFontName = std::string(name);
        impl->currentFontPath = path;
        impl->currentFontSize = size;
        impl->privateFonts = std::move(privateCollection);
        // Store the actual family name in the same UTF-8 string slot.
        const int needed = WideCharToMultiByte(CP_UTF8, 0, familyName, -1, nullptr, 0, nullptr, nullptr);
        if (needed > 0)
        {
            std::string actualFamily(static_cast<size_t>(needed), '\0');
            if (WideCharToMultiByte(CP_UTF8, 0, familyName, -1, actualFamily.data(), needed, nullptr, nullptr) > 0)
            {
                actualFamily.resize(static_cast<size_t>(needed - 1));
                impl->currentFontName = actualFamily;
            }
        }
        return true;
    }

    impl->currentFontName = name;
    impl->currentFontPath.clear();
    impl->currentFontSize = size;
    return true;
}

void CGUI::text(const std::string& value, int x, int y, int r, int g, int b)
{
    if (!impl->current || !impl->current->backBuffer)
    {
        impl->error("Create a window before drawing text.");
        return;
    }

    Graphics graphics(impl->current->backBuffer.get());
    graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);

    std::wstring wideValue = toWide(value);
    std::wstring wideFamily = toWide(impl->currentFontName);
    FontFamily family(
        wideFamily.c_str(),
        impl->privateFonts ? static_cast<FontCollection*>(impl->privateFonts.get()) : nullptr);

    if (family.GetLastStatus() != Ok)
    {
        wideFamily = L"Arial";
        FontFamily fallback(wideFamily.c_str());
        Font font(&fallback, static_cast<REAL>(impl->currentFontSize), FontStyleRegular, UnitPixel);
        SolidBrush brush(makeColor(r, g, b));
        graphics.DrawString(wideValue.c_str(), -1, &font,
                            PointF(static_cast<REAL>(x), static_cast<REAL>(y)), &brush);
        return;
    }

    Font drawFont(&family, static_cast<REAL>(impl->currentFontSize), FontStyleRegular, UnitPixel);
    SolidBrush brush(makeColor(r, g, b));
    graphics.DrawString(wideValue.c_str(), -1, &drawFont,
                        PointF(static_cast<REAL>(x), static_cast<REAL>(y)), &brush);
}

void CGUI::background(int r, int g, int b)
{
    if (!impl->current)
    {
        impl->error("Create a window before setting its background.");
        return;
    }

    impl->current->background = makeColor(r, g, b);
}

void CGUI::rect(int x, int y, int width, int height, int r, int g, int b)
{
    if (!impl->current || !impl->current->backBuffer)
    {
        impl->error("Create a window before drawing.");
        return;
    }

    Graphics graphics(impl->current->backBuffer.get());
    SolidBrush brush(makeColor(r, g, b));
    graphics.FillRectangle(&brush, x, y, width, height);
}

void CGUI::circle(int x, int y, int radius, int r, int g, int b)
{
    if (!impl->current || !impl->current->backBuffer)
    {
        impl->error("Create a window before drawing.");
        return;
    }

    if (radius <= 0)
        return;

    Graphics graphics(impl->current->backBuffer.get());
    SolidBrush brush(makeColor(r, g, b));
    graphics.FillEllipse(&brush, x - radius, y - radius, radius * 2, radius * 2);
}

void CGUI::line(int x1, int y1, int x2, int y2, int r, int g, int b)
{
    if (!impl->current || !impl->current->backBuffer)
    {
        impl->error("Create a window before drawing.");
        return;
    }

    Graphics graphics(impl->current->backBuffer.get());
    Pen pen(makeColor(r, g, b), 1.0f);
    graphics.DrawLine(&pen, x1, y1, x2, y2);
}

void CGUI::button(const std::string& name, int x, int y, int width, int height)
{
    if (!impl->current || !impl->current->backBuffer)
    {
        impl->error("Create a window before drawing a button.");
        return;
    }

    Impl::ButtonState buttonState;
    buttonState.x = x;
    buttonState.y = y;
    buttonState.width = width;
    buttonState.height = height;
    impl->current->buttons[name] = buttonState;

    const bool hovered =
        impl->current->mouseX >= x &&
        impl->current->mouseX < x + width &&
        impl->current->mouseY >= y &&
        impl->current->mouseY < y + height;

    const int shade = hovered ? 90 : 65;

    Graphics graphics(impl->current->backBuffer.get());
    SolidBrush fill(makeColor(shade, shade, shade));
    Pen outline(makeColor(220, 220, 220), 1.0f);
    graphics.FillRectangle(&fill, x, y, width, height);
    graphics.DrawRectangle(&outline, x, y, width, height);

    std::wstring wideValue = toWide(name);
    std::wstring wideFamily = toWide(impl->currentFontName);

    SolidBrush textBrush(makeColor(255, 255, 255));
    RectF bounds(static_cast<REAL>(x), static_cast<REAL>(y),
                 static_cast<REAL>(width), static_cast<REAL>(height));
    StringFormat format;
    format.SetAlignment(StringAlignmentCenter);
    format.SetLineAlignment(StringAlignmentCenter);

    if (impl->privateFonts)
    {
        FontFamily family(wideFamily.c_str(), impl->privateFonts.get());
        if (family.GetLastStatus() == Ok)
        {
            Font drawFont(&family, static_cast<REAL>(impl->currentFontSize),
                          FontStyleRegular, UnitPixel);
            graphics.DrawString(wideValue.c_str(), -1, &drawFont, bounds, &format, &textBrush);
            return;
        }
    }

    FontFamily systemFamily(wideFamily.c_str());
    if (systemFamily.GetLastStatus() == Ok)
    {
        Font drawFont(&systemFamily, static_cast<REAL>(impl->currentFontSize),
                      FontStyleRegular, UnitPixel);
        graphics.DrawString(wideValue.c_str(), -1, &drawFont, bounds, &format, &textBrush);
        return;
    }

    FontFamily fallbackFamily(L"Arial");
    Font drawFont(&fallbackFamily, static_cast<REAL>(impl->currentFontSize),
                  FontStyleRegular, UnitPixel);
    graphics.DrawString(wideValue.c_str(), -1, &drawFont, bounds, &format, &textBrush);
}

bool CGUI::clicked(const std::string& name) const
{
    const Impl::ButtonState* buttonState = impl->findButton(name);
    if (!buttonState || !impl->current)
        return false;

    if (!impl->mousePressedLeft || impl->clickWindow != impl->current->hwnd)
        return false;

    return
        impl->clickPoint.x >= buttonState->x &&
        impl->clickPoint.x < buttonState->x + buttonState->width &&
        impl->clickPoint.y >= buttonState->y &&
        impl->clickPoint.y < buttonState->y + buttonState->height;
}

int CGUI::mousex() const
{
    if (!impl->current || !impl->current->hwnd)
        return 0;

    POINT point{};
    if (!GetCursorPos(&point))
        return 0;
    ScreenToClient(impl->current->hwnd, &point);
    return point.x;
}

int CGUI::mousey() const
{
    if (!impl->current || !impl->current->hwnd)
        return 0;

    POINT point{};
    if (!GetCursorPos(&point))
        return 0;
    ScreenToClient(impl->current->hwnd, &point);
    return point.y;
}

bool CGUI::mousedown(const std::string& button) const
{
    if (isMouseLeft(button))
        return impl->mouseDownLeft;
    if (isMouseRight(button))
        return impl->mouseDownRight;
    return false;
}

bool CGUI::mousepressed(const std::string& button) const
{
    if (isMouseLeft(button))
        return impl->mousePressedLeft;
    if (isMouseRight(button))
        return impl->mousePressedRight;
    return false;
}

bool CGUI::keypressed(const std::string& key) const
{
    const int vk = virtualKeyFromString(key);
    if (vk <= 0 || vk >= 256)
        return false;
    return impl->keysPressed[vk];
}

bool CGUI::keydown(const std::string& key) const
{
    const int vk = virtualKeyFromString(key);
    if (vk <= 0 || vk >= 256)
        return false;
    return impl->keysDown[vk];
}

bool CGUI::running() const
{
    return impl->appRunning && impl->current != nullptr;
}

void CGUI::clear()
{
    if (!impl->current || !impl->current->backBuffer)
    {
        impl->error("Create a window before calling clear().");
        return;
    }

    impl->ensureFrameStarted();

    if (!impl->appRunning)
        return;

    Graphics graphics(impl->current->backBuffer.get());
    graphics.Clear(impl->current->background);
}

void CGUI::update()
{
    if (!impl->current)
    {
        impl->error("Create a window before calling update().");
        return;
    }

    impl->ensureFrameStarted();

    for (auto& [name, state] : impl->windows)
    {
        if (state->hwnd && state->backBuffer)
            impl->presentWindow(*state);
    }

    InvalidateRect(impl->current->hwnd, nullptr, FALSE);

    std::fill(std::begin(impl->keysPressed), std::end(impl->keysPressed), false);
    impl->mousePressedLeft = false;
    impl->mousePressedRight = false;
    impl->clickWindow = nullptr;
    impl->frameOpen = false;
}

CGUI cg;

