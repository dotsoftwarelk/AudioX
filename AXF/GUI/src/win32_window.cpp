// Windows-only GUI backend: Win32 window + Direct2D/DirectWrite painting.
#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "axf/gui/window.hpp"

#include <d2d1.h>
#include <dwrite.h>
#include <windows.h>
#include <wrl/client.h>

#include <algorithm>
#include <string>
#include <utility>

namespace axf::gui
{
using Microsoft::WRL::ComPtr;

namespace
{
std::wstring utf8ToWide(const std::string& value)
{
    const int size = ::MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
    if (size <= 0)
    {
        return {};
    }
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, result.data(), size);
    result.resize(result.size() - 1);
    return result;
}

D2D1_RECT_F toD2d(Rect rect) noexcept
{
    return D2D1::RectF(rect.x, rect.y, rect.x + rect.width, rect.y + rect.height);
}

D2D1_COLOR_F toD2d(Rgba color) noexcept
{
    return D2D1::ColorF(color.r, color.g, color.b, color.a);
}

class Direct2DCanvas final : public ICanvas
{
  public:
    Direct2DCanvas(ID2D1HwndRenderTarget* target, ID2D1SolidColorBrush* brush,
                   IDWriteFactory* writeFactory) noexcept
        : target_(target), brush_(brush), writeFactory_(writeFactory)
    {
    }

    void clear(Rgba color) noexcept override
    {
        if (target_ != nullptr)
        {
            target_->Clear(toD2d(color));
        }
    }

    void fillRect(Rect bounds, Rgba color) noexcept override
    {
        if (target_ == nullptr || brush_ == nullptr || bounds.width <= 0.0F ||
            bounds.height <= 0.0F)
        {
            return;
        }
        brush_->SetColor(toD2d(color));
        target_->FillRectangle(toD2d(bounds), brush_);
    }

    void drawText(Rect bounds, std::string_view text, Rgba color, float fontSize) noexcept override
    {
        if (target_ == nullptr || brush_ == nullptr || writeFactory_ == nullptr || text.empty() ||
            bounds.width <= 0.0F || bounds.height <= 0.0F)
        {
            return;
        }
        ComPtr<IDWriteTextFormat> format;
        ComPtr<IDWriteTextLayout> layout;
        const std::wstring wide = utf8ToWide(std::string(text));
        const float size = std::clamp(fontSize, 6.0F, 96.0F);
        if (FAILED(writeFactory_->CreateTextFormat(
                L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL, size, L"en-US", format.GetAddressOf())))
        {
            return;
        }
        if (FAILED(writeFactory_->CreateTextLayout(wide.c_str(), static_cast<UINT32>(wide.size()),
                                                   format.Get(), bounds.width, bounds.height,
                                                   layout.GetAddressOf())))
        {
            return;
        }
        brush_->SetColor(toD2d(color));
        target_->DrawTextLayout(D2D1::Point2F(bounds.x, bounds.y), layout.Get(), brush_);
    }

    void present() noexcept override
    {
    }

  private:
    ID2D1HwndRenderTarget* target_{nullptr};
    ID2D1SolidColorBrush* brush_{nullptr};
    IDWriteFactory* writeFactory_{nullptr};
};

class Win32Window final : public IWindow
{
  public:
    Win32Window(std::wstring title, Rect bounds, Theme theme)
        : title_(std::move(title)), bounds_(bounds), theme_(theme)
    {
    }

    ~Win32Window() override
    {
        releaseResources();
    }

    bool create() override
    {
        WNDCLASSEXW windowClass{};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = &Win32Window::windowProc;
        windowClass.hInstance = reinterpret_cast<HINSTANCE>(::GetModuleHandle(nullptr));
        windowClass.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
        windowClass.lpszClassName = className();
        ::RegisterClassExW(&windowClass);

        const auto width = static_cast<int>(bounds_.width);
        const auto height = static_cast<int>(bounds_.height);
        hwnd_ = ::CreateWindowExW(0, className(), title_.c_str(), WS_OVERLAPPEDWINDOW,
                                  CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr,
                                  windowClass.hInstance, this);
        if (hwnd_ == nullptr)
        {
            return false;
        }
        if (!createRenderTarget())
        {
            return false;
        }
        ::ShowWindow(hwnd_, SW_SHOW);
        ::UpdateWindow(hwnd_);
        return true;
    }

    void run() override
    {
        MSG message{};
        while (::GetMessageW(&message, nullptr, 0, 0) > 0)
        {
            ::TranslateMessage(&message);
            ::DispatchMessageW(&message);
        }
    }

    void close() noexcept override
    {
        if (hwnd_ != nullptr)
        {
            ::PostMessageW(hwnd_, WM_CLOSE, 0, 0);
        }
    }

    Scene& scene() noexcept override
    {
        return scene_;
    }

  private:
    static constexpr const wchar_t* className() noexcept
    {
        return L"AXFDirect2DWindow";
    }

    bool createRenderTarget() noexcept
    {
        if (FAILED(::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                                       factory_.ReleaseAndGetAddressOf())) ||
            FAILED(::DWriteCreateFactory(
                DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                reinterpret_cast<IUnknown**>(writeFactory_.ReleaseAndGetAddressOf()))))
        {
            return false;
        }
        const D2D1_SIZE_U size =
            D2D1::SizeU(static_cast<UINT32>(bounds_.width), static_cast<UINT32>(bounds_.height));
        if (FAILED(factory_->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
                                                    D2D1::HwndRenderTargetProperties(hwnd_, size),
                                                    target_.ReleaseAndGetAddressOf())))
        {
            return false;
        }
        return SUCCEEDED(target_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White),
                                                        brush_.ReleaseAndGetAddressOf()));
    }

    void releaseResources() noexcept
    {
        brush_.Reset();
        target_.Reset();
        writeFactory_.Reset();
        factory_.Reset();
    }

    void paint() noexcept
    {
        if (target_ == nullptr)
        {
            return;
        }
        Direct2DCanvas canvas(target_.Get(), brush_.Get(), writeFactory_.Get());
        canvas.clear(theme_.background);
        scene_.render(canvas, Rect{0.0F, 0.0F, bounds_.width, bounds_.height});
        canvas.present();
    }

    static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        Win32Window* window = nullptr;
        if (message == WM_NCCREATE)
        {
            const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
            window = static_cast<Win32Window*>(create->lpCreateParams);
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        }
        else
        {
            window = reinterpret_cast<Win32Window*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (window != nullptr)
        {
            if (message == WM_PAINT)
            {
                window->paint();
                ::ValidateRect(hwnd, nullptr);
                return 0;
            }
            if (message == WM_SIZE)
            {
                if (window->target_ != nullptr)
                {
                    const D2D1_SIZE_U size = D2D1::SizeU(static_cast<UINT32>(LOWORD(lParam)),
                                                         static_cast<UINT32>(HIWORD(lParam)));
                    (void)window->target_->Resize(size);
                }
                return 0;
            }
            if (message == WM_DESTROY)
            {
                ::PostQuitMessage(0);
                return 0;
            }
        }
        return ::DefWindowProcW(hwnd, message, wParam, lParam);
    }

    std::wstring title_;
    Rect bounds_{};
    Theme theme_{};
    Scene scene_;
    HWND hwnd_{nullptr};
    ComPtr<ID2D1Factory> factory_;
    ComPtr<ID2D1HwndRenderTarget> target_;
    ComPtr<ID2D1SolidColorBrush> brush_;
    ComPtr<IDWriteFactory> writeFactory_;
};

} // namespace

std::unique_ptr<IWindow> createWindowsWindow(std::string title, Rect bounds, Theme theme)
{
    return std::make_unique<Win32Window>(utf8ToWide(title), bounds, theme);
}

} // namespace axf::gui
#endif
