#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "dxguid")
#pragma comment(lib, "uuid")

#ifndef UNICODE
#define UNICODE
#endif

#pragma warning ( push )
#pragma warning ( disable: 4668 )
#include <windows.h>
#pragma warning ( pop )

#pragma warning ( push )
#pragma warning ( disable: 4820 4201 )
#include <d3d11.h>
#pragma warning ( pop )

#pragma warning ( push )
#pragma warning ( disable: 4820 )
#include <dxgi1_3.h>
#pragma warning ( pop )

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

const wchar_t CLASS_NAME[] = L"Sample Window Class";
const wchar_t WINDOW_TITLE[] = L"Cube11";

// No 11.1 yet, as it requires special handling when creating the D3D device.
const D3D_FEATURE_LEVEL levels[] = {
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0,
    D3D_FEATURE_LEVEL_9_3,
    D3D_FEATURE_LEVEL_9_2,
    D3D_FEATURE_LEVEL_9_1,
};

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    (void)pCmdLine;
    (void)hPrevInstance;

    ID3D11Device           *pDevice           = NULL;
    ID3D11DeviceContext    *pContext          = NULL;
    IDXGIDevice3           *pDXGIDevice       = NULL;
    IDXGIAdapter           *pAdapter          = NULL;
    IDXGIFactory           *pFactory          = NULL;
    IDXGISwapChain         *pSwapChain        = NULL;
    ID3D11Texture2D        *pBackBuffer       = NULL;
    ID3D11RenderTargetView *pRenderTarget     = NULL;
    ID3D11Texture2D        *pDepthStencil     = NULL;
    ID3D11DepthStencilView *pDepthStencilView = NULL;

    HRESULT hr = S_OK;
    #define CHECK_HR { if (FAILED(hr)) goto cleanup; }

    // Register the window class.
    WNDCLASS wc = {
        .lpfnWndProc   = WindowProc,
        .hInstance     = hInstance,
        .lpszClassName = CLASS_NAME,
    };

    RegisterClass(&wc);

    // Create the window.
    HWND hwnd = CreateWindowEx(
        0,                    // Optional window styles.
        CLASS_NAME,           // Window class
        WINDOW_TITLE,         // Window text
        WS_OVERLAPPEDWINDOW,  // Window style
        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,      // Parent window
        NULL,      // Menu
        hInstance, // Instance handle
        NULL       // Additional application data
    );

    if (hwnd == NULL) {
        goto cleanup;
    }

    // Create D3D11 device
    D3D_FEATURE_LEVEL level;
    hr = D3D11CreateDevice(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_DEBUG,
        levels,
        ARRAYSIZE(levels),
        D3D11_SDK_VERSION,
        &pDevice,
        &level,
        &pContext
    );
    CHECK_HR;

    // Get IDXGI device interface from D3D11 device
    hr = pDevice->lpVtbl->QueryInterface(pDevice, &IID_IDXGIDevice3, &pDXGIDevice);
    CHECK_HR;

    // Get IDXGI adapter and factory from the device
    hr = pDXGIDevice->lpVtbl->GetAdapter(pDXGIDevice, &pAdapter);
    CHECK_HR;
    hr = pAdapter->lpVtbl->GetParent(pAdapter, &IID_IDXGIFactory, &pFactory);
    CHECK_HR;

    // Create the swap chain
    DXGI_SWAP_CHAIN_DESC sc_desc = {
        .Windowed = TRUE,
        .BufferCount = 2,
        .BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .SampleDesc.Count = 1,
        .SampleDesc.Quality = 0,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
        .OutputWindow = hwnd,
    };
    hr = pFactory->lpVtbl->CreateSwapChain(pFactory, (IUnknown *)pDevice, &sc_desc, &pSwapChain);
    CHECK_HR;

    // Get the back buffer and create a view of it
    hr = pSwapChain->lpVtbl->GetBuffer(pSwapChain, 0, &IID_ID3D11Texture2D, &pBackBuffer);
    CHECK_HR;
    hr = pDevice->lpVtbl->CreateRenderTargetView(pDevice, (ID3D11Resource *)pBackBuffer, NULL, &pRenderTarget);
    CHECK_HR;

    // Create depth stencil buffer and view
    D3D11_TEXTURE2D_DESC t2d_desc;
    pBackBuffer->lpVtbl->GetDesc(pBackBuffer, &t2d_desc);

    D3D11_TEXTURE2D_DESC ds_desc = {
        .Width = t2d_desc.Width,
        .Height = t2d_desc.Height,
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
        .SampleDesc = { .Count = 1, .Quality = 0 },
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_DEPTH_STENCIL,
        .CPUAccessFlags = 0,
        .MiscFlags = 0
    };
    hr = pDevice->lpVtbl->CreateTexture2D(pDevice, &ds_desc, NULL, &pDepthStencil);
    CHECK_HR;

    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {
        .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
    };
    hr = pDevice->lpVtbl->CreateDepthStencilView(pDevice, (ID3D11Resource *)pDepthStencil, &dsv_desc, &pDepthStencilView);
    CHECK_HR;

    // Set viewport
    D3D11_VIEWPORT viewport = {
        .Width = (float)t2d_desc.Width,
        .Height = (float)t2d_desc.Height,
        .MinDepth = 0,
        .MaxDepth = 0,
    };
    pContext->lpVtbl->RSSetViewports(pContext, 1, &viewport);

    // Show window
    ShowWindow(hwnd, nCmdShow);

    // Run the message loop.
    MSG msg = { .message = WM_NULL };
    PeekMessage(&msg, NULL, 0u, 0u, PM_NOREMOVE);

    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0u, 0u, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            // update
            // render
            // present
        }
    }

#undef CHECK_HR
cleanup:
#define RELEASE(p) { if (p) p->lpVtbl->Release(p); p = NULL; }
    RELEASE(pDepthStencilView);
    RELEASE(pDepthStencil);
    RELEASE(pRenderTarget);
    RELEASE(pBackBuffer);
    RELEASE(pSwapChain);
    RELEASE(pFactory);
    RELEASE(pAdapter);
    RELEASE(pDXGIDevice);
    RELEASE(pContext);
    RELEASE(pDevice);
#undef RELEASE

    return hr;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}
