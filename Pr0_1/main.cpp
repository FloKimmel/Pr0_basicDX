#include <Windows.h>

#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>

/* Function prototypes */
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
HWND InitializeWindow(HINSTANCE hinstance, int cmdshow);
bool InitializeD3D11(HWND hwnd, ID3D11Device** device, ID3D11DeviceContext** dcontext, IDXGISwapChain** swapchain);
bool Setup(ID3D11Device* device, ID3D11DeviceContext* dcontext);
bool Render(ID3D11DeviceContext* dcontext, IDXGISwapChain* swapchain, unsigned int vcount);
void Clear(ID3D11DeviceContext* dcontext);

int WINAPI wWinMain(HINSTANCE hinstance, HINSTANCE prev_instance, PWSTR cmdl_args, int cmdshow) {

	/* Create window and show it */
	HWND hwnd = InitializeWindow(hinstance, cmdshow);

	/* Check for errors */
	if (!hwnd) return -1;

	/* Initialize DX11 and check for errors */
	ID3D11Device* device;
	ID3D11DeviceContext* dcontext;
	IDXGISwapChain* swapchain;
	if(!InitializeD3D11(hwnd, &device, &dcontext, &swapchain)) return -1;

	/* Create model and vertex buffer, load and set shaders etc. and check for errors */
	if (!Setup(device, dcontext)) return -1;

	/* Render until window gets closed */
	MSG msg = { 0 };
	while (msg.message != WM_QUIT) {

		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {

			/* Clear backbuffer, depthbuffer and stencilbuffer */
			Clear(dcontext);
			
			/* Render model and check for errors */
			if(!Render(dcontext, swapchain, 3)) return -1;
		}
	}
	return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {

	switch (message) {
	case(WM_DESTROY):
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

HWND InitializeWindow(HINSTANCE hinstance, int cmdshow) {

	/* Set window class structure */
	WNDCLASSEX wndc = { 0 };
	wndc.cbSize = sizeof(WNDCLASSEX);
	wndc.lpfnWndProc = WndProc;
	wndc.hInstance = hinstance;
	wndc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wndc.lpszClassName = "MyWndClass";

	/* Register window class structure */
	if (!RegisterClassEx(&wndc)) return nullptr;

	/* Create window */
	HWND hwnd = CreateWindowEx(WS_EX_APPWINDOW, "MyWndClass", "Awesome!", WS_OVERLAPPEDWINDOW,
							   200, 200, 800, 600, nullptr, nullptr, hinstance, nullptr);

	/* Check for errors */
	if (!hwnd) return nullptr;

	/* Show window etc. */
	ShowWindow(hwnd, cmdshow);
	SetForegroundWindow(hwnd);
	SetFocus(hwnd);

	return hwnd;
}

bool InitializeD3D11(HWND hwnd, ID3D11Device** device, ID3D11DeviceContext** dcontext, IDXGISwapChain** swapchain) {

	/* Get client area size of window */
	RECT dimensions;
	if(!GetClientRect(hwnd, &dimensions)) return false;
	unsigned int width = dimensions.right - dimensions.left;
	unsigned int height = dimensions.bottom - dimensions.top;

	/* Create and fill out the swapchain description */
	DXGI_SWAP_CHAIN_DESC swapchain_desc;
	ZeroMemory(&swapchain_desc, sizeof(swapchain_desc));
	swapchain_desc.BufferCount = 1;
	swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchain_desc.BufferDesc.Height = height;
	swapchain_desc.BufferDesc.Width = width;
	swapchain_desc.BufferDesc.RefreshRate.Denominator = 1;
	swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
	swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapchain_desc.OutputWindow = hwnd;
	swapchain_desc.SampleDesc.Count = 1;
	swapchain_desc.SampleDesc.Quality = 0;
	swapchain_desc.Flags = 0;
	swapchain_desc.Windowed = true;

	/* Set featurelevel to 11_0 to use DirectX version 11.0 */
	D3D_FEATURE_LEVEL featurelevel = D3D_FEATURE_LEVEL_11_0;
	unsigned int featurelevel_count = 1;

	/* Create device, device context and swapchain */
	HRESULT result;
	result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, 0, &featurelevel,
										   featurelevel_count, D3D11_SDK_VERSION, &swapchain_desc,
										   swapchain, device, nullptr, dcontext);

	/* Check for errors */
	if (FAILED(result)) return false;

	/* Create and set rendertarget_view and check for errors*/
	ID3D11Texture2D* backbuffer_texture;
	ID3D11RenderTargetView* rendertarget_view;
	if(FAILED((*swapchain)->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backbuffer_texture))) return false;
	if(FAILED((*device)->CreateRenderTargetView(backbuffer_texture, 0, &rendertarget_view))) return false;
	backbuffer_texture->Release();

	/* Fill out the depth stancil buffer description */
	D3D11_TEXTURE2D_DESC depth_stencil_desc;
	depth_stencil_desc.Width = width;
	depth_stencil_desc.Height = height;
	depth_stencil_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depth_stencil_desc.MipLevels = 1;
	depth_stencil_desc.ArraySize = 1;
	depth_stencil_desc.SampleDesc.Count = 1;
	depth_stencil_desc.SampleDesc.Quality = 0;
	depth_stencil_desc.Usage = D3D11_USAGE_DEFAULT;
	depth_stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depth_stencil_desc.CPUAccessFlags = 0;
	depth_stencil_desc.MiscFlags = 0;

	/* Create depth stencil buffer and its view and check for errors */
	ID3D11Texture2D* depthbuffer_texture;
	ID3D11DepthStencilView* depth_stencil_buffer;
	if(FAILED((*device)->CreateTexture2D(&depth_stencil_desc, nullptr, &depthbuffer_texture))) return false;
	if(FAILED((*device)->CreateDepthStencilView(depthbuffer_texture, 0, &depth_stencil_buffer))) return false;

	/* Set render target and depth stencil buffer */
	(*dcontext)->OMSetRenderTargets(1, &rendertarget_view, depth_stencil_buffer);

	/* Create and fill out the viewport description */
	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(viewport));
	viewport.Height = static_cast<float>(height);
	viewport.Width = static_cast<float>(width);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;

	/* Set viewport */
	(*dcontext)->RSSetViewports(1, &viewport);

	/* Clear render target and depth stencil buffer */
	Clear(*dcontext);

	/* Show cleared backbuffer and check for errors */
	if(FAILED((*swapchain)->Present(0, 0))) return false;

	return true;
}

bool Setup(ID3D11Device* device, ID3D11DeviceContext* dcontext) {

	/* Vertex we want to use */
	struct Vertex {
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT4 color;
	};

	/* Model we want to render */
	Vertex vertices[3];
	vertices[0].position = DirectX::XMFLOAT3(0, 0.5, 0.5);			// upper mid
	vertices[0].color = DirectX::XMFLOAT4(1, 0, 0, 1);				// red
	vertices[1].position = DirectX::XMFLOAT3(0.5, -0.5, 0.5);		// righthand bottom
	vertices[1].color = DirectX::XMFLOAT4(0, 1, 0, 1);				// green
	vertices[2].position = DirectX::XMFLOAT3(-0.5, -0.5, 0.5);		// lefthand bottom
	vertices[2].color = DirectX::XMFLOAT4(0, 0, 1, 1);				// blue

	/* Create and set vertex buffer description */
	D3D11_BUFFER_DESC vbuffer_desc;
	vbuffer_desc.ByteWidth = sizeof(Vertex) * 3;
	vbuffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbuffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
	vbuffer_desc.CPUAccessFlags = 0;
	vbuffer_desc.MiscFlags = 0;
	vbuffer_desc.StructureByteStride = 0;

	/* Specify data the vertex buffer gets filled with */
	D3D11_SUBRESOURCE_DATA init;
	init.pSysMem = vertices;

	/* Create vertex buffer */
	ID3D11Buffer* vbuffer;
	if(FAILED(device->CreateBuffer(&vbuffer_desc, &init, &vbuffer))) return false;

	/* Set vertex buffer */
	unsigned int stride = sizeof(Vertex);
	unsigned int offset = 0;
	dcontext->IASetVertexBuffers(0, 1, &vbuffer, &stride, &offset);

	/* Load and compile hlsl for vertex shader */
	ID3DBlob* vertexShaderBlob;
	if(FAILED(D3DCompileFromFile(L"vertexshader.hlsl", nullptr, nullptr, "myVertexShader",
								 "vs_5_0", 0, 0, &vertexShaderBlob, nullptr))) return false;

	/* Create actual vertex shader */
	ID3D11VertexShader* vertexShader;
	if (FAILED(device->CreateVertexShader(vertexShaderBlob->GetBufferPointer(),
										  vertexShaderBlob->GetBufferSize(),
										  nullptr, &vertexShader))) return false;

	/* Load and compile hlsl for pixel shader */
	ID3DBlob* pixelShaderBlob;
	if (FAILED(D3DCompileFromFile(L"pixelshader.hlsl", nullptr, nullptr, "myPixelShader",
								  "ps_5_0", 0, 0, &pixelShaderBlob, nullptr))) return false;

	/* Create actual pixel shader */
	ID3D11PixelShader* pixelShader;
	if (FAILED(device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(),
										 pixelShaderBlob->GetBufferSize(),
										 nullptr, &pixelShader))) return false;

	/* Create input layout description array */
	D3D11_INPUT_ELEMENT_DESC inputdesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	/* Create actual input layout */
	ID3D11InputLayout* inputLayout;
	if (FAILED(device->CreateInputLayout(inputdesc, ARRAYSIZE(inputdesc), vertexShaderBlob->GetBufferPointer(),
										 vertexShaderBlob->GetBufferSize(), &inputLayout))) return false;

	/* Set topology, input layout, vertex shader and pixel shader */
	dcontext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dcontext->IASetInputLayout(inputLayout);
	dcontext->VSSetShader(vertexShader, nullptr, 0);
	dcontext->PSSetShader(pixelShader, nullptr, 0);

	return true;
}

bool Render(ID3D11DeviceContext* dcontext, IDXGISwapChain* swapchain, unsigned int vcount) {

	dcontext->Draw(vcount, 0);
	if(FAILED(swapchain->Present(0, 0))) return false;

	return true;
}

void Clear(ID3D11DeviceContext* dcontext) {

	/* color the backbuffer should be cleared with in RGBA */
	float color[] = { 0.08627451f, 0.08627451f, 0.09411765f, 1.0f };

	/* Get backbuffer, depthbuffer and stencilbuffer */
	ID3D11RenderTargetView* rendertarget_view;
	ID3D11DepthStencilView* depthStencilView;
	dcontext->OMGetRenderTargets(1, &rendertarget_view, &depthStencilView);

	/* Clear backbuffer with color */
	dcontext->ClearRenderTargetView(rendertarget_view, color);

	/* Clear depthbuffer with 1.0 and stencilbuffer with 0 */
	dcontext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}