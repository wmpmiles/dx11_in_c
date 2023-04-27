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

#include "e02_vertex.h"
#include "e02_pixel.h"

typedef float M4x4[4][4];

struct ConstantBuffer {
    M4x4 world;
    M4x4 view;
    M4x4 projection;
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

const wchar_t CLASS_NAME[] = L"Sample Window Class";
const wchar_t WINDOW_TITLE[] = L"Cube11";

const D3D_FEATURE_LEVEL levels[] = {
    D3D_FEATURE_LEVEL_11_0,
};

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    (void)pCmdLine;
    (void)hPrevInstance;

    HRESULT hr = S_OK;
    #define CHECK_HR { if (FAILED(hr)) goto cleanup; }

    HWND hwnd = NULL;

    ID3D11Device           *pD3D11Device      = NULL;
    ID3D11DeviceContext    *pContext          = NULL;
    IDXGIDevice3           *pDXGIDevice       = NULL;
    IDXGIAdapter           *pAdapter          = NULL;
    IDXGIFactory           *pFactory          = NULL;
    IDXGISwapChain         *pSwapChain        = NULL;
    ID3D11Texture2D        *pBackBuffer       = NULL;
    ID3D11RenderTargetView *pBackBufferView   = NULL;
    ID3D11Texture2D        *pDepthStencil     = NULL;
    ID3D11DepthStencilView *pDepthStencilView = NULL;
    ID3D11VertexShader     *pVertexShader     = NULL;
    ID3D11InputLayout      *pInputLayout      = NULL;
    ID3D11PixelShader      *pPixelShader      = NULL;
    ID3D11Buffer           *pConstantBuffer   = NULL;

    D3D11_TEXTURE2D_DESC bb_desc;

    { // Register the window class.
        WNDCLASS wc = {
            .lpfnWndProc   = WindowProc,
            .hInstance     = hInstance,
            .lpszClassName = CLASS_NAME,
        };

        RegisterClass(&wc);
    }

    { // Create the window.
        hwnd = CreateWindowEx(
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
    }

    { // Create D3D11 device
        D3D_FEATURE_LEVEL level;
        hr = D3D11CreateDevice(
            NULL,
            D3D_DRIVER_TYPE_HARDWARE,
            NULL,
            D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_DEBUG,
            levels,
            ARRAYSIZE(levels),
            D3D11_SDK_VERSION,
            &pD3D11Device,
            &level,
            &pContext
        );
        CHECK_HR;
    }

    { // Get IDXGI device interface from D3D11 device
        hr = pD3D11Device->lpVtbl->QueryInterface(pD3D11Device, &IID_IDXGIDevice3, &pDXGIDevice);
        CHECK_HR;
    }

    { // Get IDXGI adapter and factory from the device
        hr = pDXGIDevice->lpVtbl->GetAdapter(pDXGIDevice, &pAdapter);
        CHECK_HR;
        hr = pAdapter->lpVtbl->GetParent(pAdapter, &IID_IDXGIFactory, &pFactory);
        CHECK_HR;
    }

    { // Create the swap chain
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
        hr = pFactory->lpVtbl->CreateSwapChain(pFactory, (IUnknown *)pD3D11Device, &sc_desc, &pSwapChain);
        CHECK_HR;
    }

    { // Get the back buffer and create a view of it
        hr = pSwapChain->lpVtbl->GetBuffer(pSwapChain, 0, &IID_ID3D11Texture2D, &pBackBuffer);
        CHECK_HR;
        hr = pD3D11Device->lpVtbl->CreateRenderTargetView(pD3D11Device, (ID3D11Resource *)pBackBuffer, NULL, &pBackBufferView);
        CHECK_HR;
    }

    { // Create depth stencil buffer and view
        pBackBuffer->lpVtbl->GetDesc(pBackBuffer, &bb_desc);

        D3D11_TEXTURE2D_DESC ds_desc = {
            .Width = bb_desc.Width,
            .Height = bb_desc.Height,
            .MipLevels = 1,
            .ArraySize = 1,
            .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
            .SampleDesc = { .Count = 1, .Quality = 0 },
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = D3D11_BIND_DEPTH_STENCIL,
            .CPUAccessFlags = 0,
            .MiscFlags = 0
        };
        hr = pD3D11Device->lpVtbl->CreateTexture2D(pD3D11Device, &ds_desc, NULL, &pDepthStencil);
        CHECK_HR;

        D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {
            .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
        };
        hr = pD3D11Device->lpVtbl->CreateDepthStencilView(pD3D11Device, (ID3D11Resource *)pDepthStencil, &dsv_desc, &pDepthStencilView);
        CHECK_HR;
    }

    { // Set viewport
        D3D11_VIEWPORT viewport = {
            .Width = (float)bb_desc.Width,
            .Height = (float)bb_desc.Height,
            .MinDepth = 0,
            .MaxDepth = 0,
        };
        pContext->lpVtbl->RSSetViewports(pContext, 1, &viewport);
    }

    { // Create vertex shader
        hr = pD3D11Device->lpVtbl->CreateVertexShader(
            pD3D11Device,
            vertex_shader,
            sizeof(vertex_shader),
            NULL,
            &pVertexShader
        );
        CHECK_HR;
    }

    { // Create input layout
        D3D11_INPUT_ELEMENT_DESC vsi_desc[] = {
            {
                .SemanticName = "POSITION",
                .SemanticIndex = 0,
                .Format = DXGI_FORMAT_R32G32B32_FLOAT,
                .InputSlot = 0,
                .AlignedByteOffset = 0,
                .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0
            },
            {
                .SemanticName = "COLOR",
                .SemanticIndex = 0,
                .Format = DXGI_FORMAT_R32G32B32_FLOAT,
                .InputSlot = 0,
                .AlignedByteOffset = 12,
                .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0
            },
        };

        hr = pD3D11Device->lpVtbl->CreateInputLayout(
            pD3D11Device, 
            vsi_desc,
            ARRAYSIZE(vsi_desc),
            vertex_shader,
            ARRAYSIZE(vertex_shader),
            &pInputLayout
        );
        CHECK_HR;
    }

    { // Create pixel shader
        hr = pD3D11Device->lpVtbl->CreatePixelShader(
            pD3D11Device,
            pixel_shader,
            sizeof(pixel_shader),
            NULL,
            &pPixelShader
        );
        CHECK_HR;
    }

    { // Create constant buffer
        D3D11_BUFFER_DESC cb_desc = {
            .ByteWidth = sizeof(struct ConstantBuffer),
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
            .CPUAccessFlags = 0,
            .MiscFlags = 0,
            .StructureByteStride = 0
        };

        hr = pD3D11Device->lpVtbl->CreateBuffer(
            pD3D11Device,
            &cb_desc,
            NULL,
            &pConstantBuffer
        );
        CHECK_HR;
    }

    { // Show window
        ShowWindow(hwnd, nCmdShow);
    }

    { // Run the message loop.
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
    }

#undef CHECK_HR
cleanup:
#define RELEASE(p) { if (p) p->lpVtbl->Release(p); p = NULL; }
    RELEASE(pConstantBuffer);
    RELEASE(pPixelShader);
    RELEASE(pInputLayout);
    RELEASE(pVertexShader);
    RELEASE(pDepthStencilView);
    RELEASE(pDepthStencil);
    RELEASE(pBackBufferView);
    RELEASE(pBackBuffer);
    RELEASE(pSwapChain);
    RELEASE(pFactory);
    RELEASE(pAdapter);
    RELEASE(pDXGIDevice);
    RELEASE(pContext);
    RELEASE(pD3D11Device);
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
