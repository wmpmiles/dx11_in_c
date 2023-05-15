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

#include <stdio.h>
#include <stdbool.h>

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
    RawF3 pos;
    RawF3 color;
} VertPosColor;
_Static_assert(sizeof(VertPosColor) == 24, "");

typedef struct RenderTarget {
    ID3D11Texture2D         *pBackBuffer;
    ID3D11RenderTargetView  *pBackBufferView;
    D3D11_TEXTURE2D_DESC     descBackBuffer;
    ID3D11Texture2D         *pDepthStencil;
    ID3D11DepthStencilState *pDepthStencilState;
    ID3D11DepthStencilView  *pDepthStencilView;
} RenderTarget;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void releaseRenderTarget(RenderTarget *rt);
HRESULT createRenderTarget(RenderTarget *rt, IDXGISwapChain *pSwapChain, ID3D11Device *pD3D11Device, ID3D11DeviceContext *pContext);

MatF4x4 calculatePerspectiveTransform(float vfov, float aspect_ratio, float near_, float far_);
MatF4x4 calculateViewTransform(VecF4 p_eye, VecF4 p_at, VecF4 v_up);

const wchar_t CLASS_NAME[] = L"Sample Window Class";
const wchar_t WINDOW_TITLE[] = L"Cube11";

const D3D_FEATURE_LEVEL levels[] = {
    D3D_FEATURE_LEVEL_11_0,
};

const RawF4 TEAL = { 0.098f, 0.439f, 0.439f, 1.000f };

const RawF3 TRIANGLE_VERTS[] = {
    {-0.5f, -0.5f, 0.5f},
    { 0.5f, -0.5f, 0.5f},
    { 0.0f,  0.5f, 0.5f},
};

const unsigned short TRIANGLE_INDICES[] = {
    0, 2, 1,
};

const VertPosColor CUBE_VERTS[] = {
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 0.0f}},
    {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}},
};

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

bool resize = false;

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
    ID3D11VertexShader      *pVertexShader      = NULL;
    ID3D11InputLayout       *pInputLayout       = NULL;
    ID3D11PixelShader       *pPixelShader       = NULL;
    ID3D11Buffer            *pConstantBuffer    = NULL;
    ID3D11Buffer            *pVertexBuffer      = NULL;
    ID3D11Buffer            *pIndexBuffer       = NULL;

    RenderTarget renderTarget = { 0 };

    ConstantBuffer constant_buffer = { 0 };
    int frame_count = 0;

    // For debug output
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE* fp = NULL;
        if (freopen_s(&fp, "CONOUT$", "w", stdout)) {
            setvbuf(stdout, NULL, _IONBF, 0);
        }
    }
    printf("\n");

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

    hr = createRenderTarget(&renderTarget, pSwapChain, pD3D11Device, pContext);
    CHECK_HR;

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
            /*
            {
                .SemanticName = "COLOR",
                .SemanticIndex = 0,
                .Format = DXGI_FORMAT_R32G32B32_FLOAT,
                .InputSlot = 0,
                .AlignedByteOffset = 12,
                .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0
            },
            */
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

    /*
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
    */

    //
    // === CUBE SETUP ===
    // 

    { // Create vertex buffer
        D3D11_BUFFER_DESC v_desc = {
            .ByteWidth = sizeof(TRIANGLE_VERTS),
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
        };

        D3D11_SUBRESOURCE_DATA v_data = {
            .pSysMem = TRIANGLE_VERTS,
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
        D3D11_BUFFER_DESC i_desc = {
            .ByteWidth = sizeof(TRIANGLE_INDICES),
            .BindFlags = D3D11_BIND_INDEX_BUFFER,
        };

        D3D11_SUBRESOURCE_DATA i_data = {
            .pSysMem = TRIANGLE_INDICES,
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

    /*
    { // Calculate camera / view transform
        VecF4 p_eye = vf4_from_rf4((RawF4){ 0.0f,  0.7f, 1.5f, 1.0f });
        VecF4 p_at  = vf4_from_rf4((RawF4){ 0.0f, -0.1f, 0.0f, 1.0f });
        VecF4 v_up  = vf4_from_rf4((RawF4){ 0.0f,  1.0f, 0.0f, 0.0f });

        constant_buffer.view = rf16_from_mf4x4(calculateViewTransform(p_eye, p_at, v_up));
    }
    
    { // Calculate the perspective transform
        float aspect_ratio = (float)renderTarget.descBackBuffer.Width / (float)renderTarget.descBackBuffer.Height;
        MatF4x4 transform = calculatePerspectiveTransform(90, aspect_ratio, 0.01f, 100.0f);
        constant_buffer.projection = rf16_from_mf4x4(transform);
    }
    */

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
                if (resize) { // Call resizeBuffers and recreate the back buffer
                    resize = false;
                    releaseRenderTarget(&renderTarget);
                    hr = pSwapChain->lpVtbl->ResizeBuffers(pSwapChain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
                    CHECK_HR;
                    hr = createRenderTarget(&renderTarget, pSwapChain, pD3D11Device, pContext);
                    CHECK_HR;
                    /*
                    float aspect_ratio = (float)renderTarget.descBackBuffer.Width / (float)renderTarget.descBackBuffer.Height;
                    MatF4x4 transform = calculatePerspectiveTransform(90, aspect_ratio, 0.01f, 100.0f);
                    constant_buffer.projection = rf16_from_mf4x4(transform);
                    */
                }
                { // Update
                    /*
                    constant_buffer.world = rf16_from_mf4x4(mf4x4_drotation_y((float)frame_count));
                    */
                    frame_count = (frame_count == 359 ? 0 : frame_count + 1);
                }
                { // render
                    // pContext->lpVtbl->UpdateSubresource(pContext, (ID3D11Resource *)pConstantBuffer, 0, NULL, &constant_buffer, 0, 0);

                    // Clear the render target and z-buffer.
                    float color[4] = {(float)frame_count/359.0f, 1.0f, 1.0f - (float)frame_count/359.0f, 1.0f};
                    pContext->lpVtbl->ClearRenderTargetView(pContext, renderTarget.pBackBufferView, color);
                    pContext->lpVtbl->ClearDepthStencilView(pContext, renderTarget.pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

                    // Bind the depth stencil state
                    //pContext->lpVtbl->OMSetDepthStencilState(pContext, renderTarget.pDepthStencilState, 1);

                    // Set the render target.
                    pContext->lpVtbl->OMSetRenderTargets(pContext, 1, &renderTarget.pBackBufferView, renderTarget.pDepthStencilView);

                    // Set up the IA stage by setting the input topology and layout.
                    UINT stride = sizeof(RawF3);
                    UINT offset = 0;
                    pContext->lpVtbl->IASetVertexBuffers(pContext, 0, 1, &pVertexBuffer, &stride, &offset);
                    pContext->lpVtbl->IASetIndexBuffer(pContext, pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
                    pContext->lpVtbl->IASetPrimitiveTopology(pContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                    pContext->lpVtbl->IASetInputLayout(pContext, pInputLayout);

                    // Set up the vertex shader stage.
                    pContext->lpVtbl->VSSetShader(pContext, pVertexShader, NULL, 0);
                    //pContext->lpVtbl->VSSetConstantBuffers(pContext, 0, 1, &pConstantBuffer);

                    // Set up the pixel shader stage.
                    pContext->lpVtbl->PSSetShader(pContext, pPixelShader, NULL, 0);

                    // Caling Draw tells Direct3D to start sending commands to the graphics device.
                    pContext->lpVtbl->DrawIndexed(pContext, ARRAYSIZE(TRIANGLE_INDICES), 0, 0);
                }
                // present
                pSwapChain->lpVtbl->Present(pSwapChain, 1, 0);
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
    releaseRenderTarget(&renderTarget);
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
    case WM_SIZE:
        resize = true;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

void releaseRenderTarget(RenderTarget *rt) {
#define RELEASE(p) { if (p) p->lpVtbl->Release(p); p = NULL; }
    RELEASE(rt->pBackBuffer);
    RELEASE(rt->pBackBufferView);
    RELEASE(rt->pDepthStencil);
    RELEASE(rt->pDepthStencilState);
    RELEASE(rt->pDepthStencilView);
#undef RELEASE
}

HRESULT createRenderTarget(RenderTarget *rt, IDXGISwapChain *pSwapChain, ID3D11Device *pD3D11Device, ID3D11DeviceContext *pContext) {
    HRESULT hr = S_OK;
    #define CHECK_HR { if (FAILED(hr)) goto exit; }

    { // Get the back buffer and create a view of it
        hr = pSwapChain->lpVtbl->GetBuffer(pSwapChain, 0, &IID_ID3D11Texture2D, &rt->pBackBuffer);
        CHECK_HR;
        hr = pD3D11Device->lpVtbl->CreateRenderTargetView(pD3D11Device, (ID3D11Resource *)rt->pBackBuffer, NULL, &rt->pBackBufferView);
        CHECK_HR;
    }

    { // Create depth stencil buffer, view, and state
        rt->pBackBuffer->lpVtbl->GetDesc(rt->pBackBuffer, &rt->descBackBuffer);

        D3D11_TEXTURE2D_DESC descDepthStencil = {
            .Width = rt->descBackBuffer.Width,
            .Height = rt->descBackBuffer.Height,
            .MipLevels = 1,
            .ArraySize = 1,
            .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
            .SampleDesc = { .Count = 1, .Quality = 0 },
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = D3D11_BIND_DEPTH_STENCIL,
            .CPUAccessFlags = 0,
            .MiscFlags = 0
        };
        hr = pD3D11Device->lpVtbl->CreateTexture2D(pD3D11Device, &descDepthStencil, NULL, &rt->pDepthStencil);
        CHECK_HR;

        D3D11_DEPTH_STENCILOP_DESC descDepthStencilOp = {
            .StencilFunc = D3D11_COMPARISON_ALWAYS,
            .StencilDepthFailOp = D3D11_STENCIL_OP_KEEP,
            .StencilPassOp = D3D11_STENCIL_OP_KEEP,
            .StencilFailOp = D3D11_STENCIL_OP_KEEP
        };
        D3D11_DEPTH_STENCIL_DESC descDepthStencilState = {
            .DepthEnable = TRUE,
            .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
            .DepthFunc = D3D11_COMPARISON_GREATER,
            .StencilEnable = FALSE,
            .StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK,
            .StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK,
            .FrontFace = descDepthStencilOp,
            .BackFace = descDepthStencilOp
        };
        hr = pD3D11Device->lpVtbl->CreateDepthStencilState(pD3D11Device, &descDepthStencilState, &rt->pDepthStencilState);
        CHECK_HR;

        D3D11_DEPTH_STENCIL_VIEW_DESC descDepthStencilView = {
            .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
        };
        hr = pD3D11Device->lpVtbl->CreateDepthStencilView(pD3D11Device, (ID3D11Resource *)rt->pDepthStencil, &descDepthStencilView, &rt->pDepthStencilView);
        CHECK_HR;
    }

    { // Set viewport
        D3D11_VIEWPORT viewport = {
            .Width = (float)rt->descBackBuffer.Width,
            .Height = (float)rt->descBackBuffer.Height,
            .MinDepth = 0,
            .MaxDepth = 0,
        };
        pContext->lpVtbl->RSSetViewports(pContext, 1, &viewport);
    }

#undef CHECK_HR
exit:
    return hr;
}

MatF4x4 calculateViewTransform(VecF4 p_eye, VecF4 p_at, VecF4 v_up) {
    VecF4 u_z = vf4_normalize(vf4_sub(p_eye, p_at));
    VecF4 u_x = vf4_normalize(vf4_cross(v_up, u_z));
    VecF4 u_y = vf4_normalize(vf4_cross(u_z, u_x));

    // Output matrix is ready for element_dot, so we don't have to transpose it
    return mf4x4_inv_orthonormal_point(u_x, u_y, u_z, p_eye);
}

MatF4x4 calculatePerspectiveTransform(float vfov, float aspect_ratio, float near_, float far_) {
    const float half_vfov = vfov / 2.0f;

    // pre-computations
    float rcp_tan_half_vfov = 1 / vf4_ss_get(vf4_tand(vf4_ss_set(half_vfov)));
    float rcp_f_sub_n = 1 / (far_ - near_);
    float rcp_aspect_ratio = 1 / aspect_ratio;

    // coefficients
    float c_xx = rcp_aspect_ratio * rcp_tan_half_vfov;
    float c_yy = rcp_tan_half_vfov;
    float c_zz = near_ * rcp_f_sub_n;
    float c_zw = -far_ * c_zz;

    // move into vectors for matrix composition
    VecF4 x = VF4_FROM(c_xx,    0,    0,    0);
    VecF4 y = VF4_FROM(   0, c_yy,    0,    0);
    VecF4 z = VF4_FROM(   0,    0, c_zz, c_zw);
    VecF4 w = VF4_FROM(   0,    0,    1,    0);

    // Matrix is ready for element_dot, so we don't have to transpose it
    return mf4x4_from_vf4(x, y, z, w);
}
