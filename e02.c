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

#include "3dmath.c"

typedef struct {
    _Alignas(64) RawF16 world;
    _Alignas(64) RawF16 view;
    _Alignas(64) RawF16 projection;
} ConstantBuffer;
_Static_assert(_Alignof(ConstantBuffer) >= 16, "");

typedef struct {
    RawF4 pos;
    RawF4 color;
} VertPosColor;

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

    ID3D11Device            *pD3D11Device       = NULL;
    ID3D11DeviceContext     *pContext           = NULL;
    IDXGIDevice3            *pDXGIDevice        = NULL;
    IDXGIAdapter            *pAdapter           = NULL;
    IDXGIFactory            *pFactory           = NULL;
    IDXGISwapChain          *pSwapChain         = NULL;
    ID3D11Texture2D         *pBackBuffer        = NULL;
    ID3D11RenderTargetView  *pBackBufferView    = NULL;
    ID3D11Texture2D         *pDepthStencil      = NULL;
    ID3D11DepthStencilState *pDepthStencilState = NULL;
    ID3D11DepthStencilView  *pDepthStencilView  = NULL;
    ID3D11VertexShader      *pVertexShader      = NULL;
    ID3D11InputLayout       *pInputLayout       = NULL;
    ID3D11PixelShader       *pPixelShader       = NULL;
    ID3D11Buffer            *pConstantBuffer    = NULL;
    ID3D11Buffer            *pVertexBuffer      = NULL;
    ID3D11Buffer            *pIndexBuffer       = NULL;

    D3D11_TEXTURE2D_DESC bb_desc;

    ConstantBuffer constant_buffer = { 0 };

    //
    // === WINDOW SETUP ===
    //

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

    //
    // === D3D11 / DXGI SETUP ===
    //

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

    { // Create depth stencil buffer, view, and state
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

        D3D11_DEPTH_STENCILOP_DESC ds_op_desc = {
            .StencilFunc = D3D11_COMPARISON_ALWAYS,
            .StencilDepthFailOp = D3D11_STENCIL_OP_KEEP,
            .StencilPassOp = D3D11_STENCIL_OP_KEEP,
            .StencilFailOp = D3D11_STENCIL_OP_KEEP
        };
        D3D11_DEPTH_STENCIL_DESC ds_state_desc = {
            .DepthEnable = TRUE,
            .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
            .DepthFunc = D3D11_COMPARISON_GREATER,
            .StencilEnable = FALSE,
            .StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK,
            .StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK,
            .FrontFace = ds_op_desc,
            .BackFace = ds_op_desc
        };
        hr = pD3D11Device->lpVtbl->CreateDepthStencilState(pD3D11Device, &ds_state_desc, &pDepthStencilState);
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

    //
    // === SHADER SETUP ===
    //

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
            .ByteWidth = sizeof(ConstantBuffer),
            .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
        };

        hr = pD3D11Device->lpVtbl->CreateBuffer(
            pD3D11Device,
            &cb_desc,
            NULL,
            &pConstantBuffer
        );
        CHECK_HR;
    }

    //
    // === CUBE SETUP ===
    // 

    { // Create vertex buffer
        const VertPosColor CUBE_VERTS[] = {
            {{-0.5f, -0.5f, -0.5f, 0.0f}, {0, 0, 0, 0}},
            {{-0.5f, -0.5f,  0.5f, 0.0f}, {0, 0, 1, 0}},
            {{-0.5f,  0.5f, -0.5f, 0.0f}, {0, 1, 0, 0}},
            {{-0.5f,  0.5f,  0.5f, 0.0f}, {0, 1, 1, 0}},
            {{ 0.5f, -0.5f, -0.5f, 0.0f}, {1, 0, 0, 0}},
            {{ 0.5f, -0.5f,  0.5f, 0.0f}, {1, 0, 1, 0}},
            {{ 0.5f,  0.5f, -0.5f, 0.0f}, {1, 1, 0, 0}},
            {{ 0.5f,  0.5f,  0.5f, 0.0f}, {1, 1, 1, 0}},
        };

        D3D11_BUFFER_DESC v_desc = {
            .ByteWidth = sizeof(CUBE_VERTS),
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
        };

        D3D11_SUBRESOURCE_DATA v_data = {
            .pSysMem = CUBE_VERTS,
            .SysMemPitch = 0,
            .SysMemSlicePitch = 0,
        };

        hr = pD3D11Device->lpVtbl->CreateBuffer(
            pD3D11Device,
            &v_desc,
            &v_data,
            &pVertexBuffer
        );
        CHECK_HR;
    }

    { // Create index buffer
        const unsigned short CUBE_INDICES[] = {
            0, 2, 1, // -x
            1, 2, 3,

            4, 5, 6, // +x
            5, 7, 6,

            0, 1, 5, // -y
            0, 5, 4,

            2, 6, 7, // +y
            2, 7, 3,

            0, 4, 6, // -z
            0, 6, 2,

            1, 3, 7, // +z
            1, 7, 5,
        };

        D3D11_BUFFER_DESC i_desc = {
            .ByteWidth = sizeof(CUBE_INDICES),
            .BindFlags = D3D11_BIND_INDEX_BUFFER,
        };

        D3D11_SUBRESOURCE_DATA i_data = {
            .pSysMem = CUBE_INDICES,
            .SysMemPitch = 0,
            .SysMemSlicePitch = 0,
        };

        hr = pD3D11Device->lpVtbl->CreateBuffer(
            pD3D11Device,
            &i_desc,
            &i_data,
            &pIndexBuffer
        );
        CHECK_HR;
    }

    // HLSL will (by default) interpret matrix data as being column-major, so
    // as long as we only pre-multiply in HLSL we should get the same outcome as
    // using element_dot i.e. make sure all of your matrices are ready for 
    // multiplication on the CPU side

    { // Calculate camera / view transform
        VecF4 p_eye = vf4_from_rf4((RawF4){ 0.0f,  0.7f, 1.5f, 1.0f });
        VecF4 p_at  = vf4_from_rf4((RawF4){ 0.0f, -0.1f, 0.0f, 1.0f });
        VecF4 v_up  = vf4_from_rf4((RawF4){ 0.0f,  1.0f, 0.0f, 0.0f });

        VecF4 u_z = vf4_normalize(vf4_sub(p_eye, p_at));
        VecF4 u_x = vf4_normalize(vf4_cross(v_up, u_z));
        VecF4 u_y = vf4_normalize(vf4_cross(u_z, u_x));

        // Output matrix is ready for element_dot, so we don't have to transpose it
        MatF4x4 tf_view = mf4x4_inv_orthonormal_point(u_x, u_y, u_z, p_eye);
        constant_buffer.view = rf16_from_mf4x4(tf_view);
    }
    
    { // Calculate the perspective transform
        // parameters
        const float half_vfov = 45;
        const float aspect_ratio = (float)bb_desc.Width / (float)bb_desc.Height;
        const float n = 0.01f;
        const float f = 100.0f;

        // pre-computations
        const float tan_half_vfov = vf4_ss_get(vf4_tand(vf4_ss_set(half_vfov)));
        const float f_sub_n = f - n;

        // coefficients
        VecF4 numerator = vf4_from_rf4((RawF4){ 1.0f, 1.0f, n, -n * f });
        VecF4 denominator = vf4_from_rf4((RawF4){ 
            aspect_ratio * tan_half_vfov,
            tan_half_vfov,
            f_sub_n,
            f_sub_n
        });
        VecF4 coefficients = vf4_mul(numerator, vf4_reciprocal(denominator));

        // move into vectors for matrix composition
        VecF4 x = VF4_EXTRACT(coefficients, VF4_MASK4(1, 0, 0, 0));
        VecF4 y = VF4_EXTRACT(coefficients, VF4_MASK4(0, 1, 0, 0));
        VecF4 z = VF4_EXTRACT(coefficients, VF4_MASK4(0, 0, 1, 1));
        VecF4 w = vf4_from_rf4((RawF4){ 0.0f, 0.0f, -1.0f, 0.0f });

        // The above are constructed such that no transpose is required
        MatF4x4 tf_perspective = mf4x4_from_vf4(x, y, z, w);
        constant_buffer.projection = rf16_from_mf4x4(tf_perspective);
    }

    //
    // === RUN WINDOW ===
    //

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
    RELEASE(pIndexBuffer);
    RELEASE(pVertexBuffer);
    RELEASE(pConstantBuffer);
    RELEASE(pPixelShader);
    RELEASE(pInputLayout);
    RELEASE(pVertexShader);
    RELEASE(pDepthStencilView);
    RELEASE(pDepthStencilState);
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
