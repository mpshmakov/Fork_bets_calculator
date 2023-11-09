#include "gui.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"
#include <string>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND window, UINT message, WPARAM wideParameter, LPARAM longParameter);

long __stdcall WindowProcess(HWND window, UINT message, WPARAM wideParameter, LPARAM longParameter) {
	if (ImGui_ImplWin32_WndProcHandler( window, message, wideParameter, longParameter))
		return true;

	switch (message) {
	case WM_SIZE: {
		if (gui::device && wideParameter != SIZE_MINIMIZED) {
			gui::presentParameters.BackBufferWidth = LOWORD(longParameter);
			gui::presentParameters.BackBufferHeight = HIWORD(longParameter);
			gui::ResetDevice();
		}

	}return 0;

	case WM_SYSCOMMAND: {
		if ((wideParameter & 0xfff0) == SC_KEYMENU)
			return 0;
	}break;


	case WM_DESTROY: {
		PostQuitMessage(0);
	}return 0;


	case WM_LBUTTONDOWN: {
		gui::position = MAKEPOINTS(longParameter);
	}return 0;

	case WM_MOUSEMOVE: {
		if (wideParameter == MK_LBUTTON)
		{
			const auto points = MAKEPOINTS(longParameter);
			auto rect = RECT{ };

			GetWindowRect(gui::window, &rect);

			rect.left += points.x - gui::position.x;
			rect.top += points.y - gui::position.y;

			if (gui::position.x >= 0 &&
				gui::position.x <= gui::WIDTH &&
				gui::position.y >= 0 && gui::position.y <= 19)
				SetWindowPos(
					gui::window,
					HWND_TOPMOST,
					rect.left,
					rect.top,
					0, 0,
					SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER
				);
		}

	}return 0;

	}

	return DefWindowProcW(window, message, wideParameter, longParameter); 
}

void gui::CreateHWindow(const char* windowName, const char* className) noexcept {
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_CLASSDC;
	windowClass.lpfnWndProc = WindowProcess;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandleA(0);
	windowClass.hIcon = 0;
	windowClass.hCursor = 0;
	windowClass.hbrBackground = 0;
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = "class001";
	windowClass.hIconSm = 0;

	RegisterClassEx(&windowClass);

	window = CreateWindowEx(
		0,
		"class001",
		windowName,
		WS_POPUP,
		100,
		100,
		WIDTH,
		HEIGHT,
		0,
		0,
		windowClass.hInstance,
		0
	);

	ShowWindow(window, SW_SHOWDEFAULT);
	UpdateWindow(window);
}

void gui::DestroyHWindow() noexcept {
	DestroyWindow(window);
	UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
}


bool gui::CreateDevice() noexcept {
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	if (!d3d)
		return false;

	ZeroMemory(&presentParameters, sizeof(presentParameters));

	presentParameters.Windowed = TRUE;
	presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
	presentParameters.EnableAutoDepthStencil = TRUE;
	presentParameters.AutoDepthStencilFormat = D3DFMT_D16;
	presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	if (d3d->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		window,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&presentParameters,
		&device) < 0)
		return false;

	return true;
}

void gui::ResetDevice() noexcept {
	ImGui_ImplDX9_InvalidateDeviceObjects();

	const auto result = device->Reset(&presentParameters);

	if (result == D3DERR_INVALIDCALL)
		IM_ASSERT(0);

	ImGui_ImplDX9_CreateDeviceObjects();
}

void gui::DestroyDevice() noexcept {
	if (device)
	{
		device->Release();
		device = nullptr;
	}

	if (d3d)
	{
		d3d->Release();
		d3d = nullptr;
	}
}


void gui::CreateImGui() noexcept {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ::ImGui::GetIO();

	io.IniFilename = NULL;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(device);
}

void gui::DestroyImGui() noexcept {
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}



void gui::BeginRender() noexcept {
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);

		if (message.message == WM_QUIT)
		{
			isRunning = !isRunning;
			return;
		}
	}

	// Start the Dear ImGui frame
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void gui::EndRender() noexcept {
	ImGui::EndFrame();

	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);

	if (device->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		device->EndScene();
	}

	const auto result = device->Present(0, 0, 0, 0);

	// Handle loss of D3D9 device
	if (result == D3DERR_DEVICELOST && device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		ResetDevice();
}

void gui::Render() noexcept
{
	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ WIDTH, HEIGHT });
	ImGui::Begin(
		"Fork bets calculator",
		&isRunning,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove
	);
	
	int zerosSum = 0;

	static auto firstMoney = 0.f;
	static auto firstMulti = 0.f;

	static auto tmpFirstMoney = firstMoney;
	static auto tmpFirstMulti = firstMulti;

	static auto secondMoney = 0.f;
	static auto secondMulti = 0.f;

	static auto tmpSecondMoney = secondMoney;
	static auto tmpSecondMulti = secondMulti;

	static auto totalMoney = 0.f;
	static auto tmpTotalMoney = totalMoney;


	static auto profitMoney = 0.f;
	static auto profitPercentage = 0.f;

	

	

	ImGui::PushItemWidth(100);
	ImGui::InputFloat("total money (optional)", &totalMoney);
	ImGui::PopItemWidth();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Text("fill in any of the 3 boxes below");
	ImGui::Spacing();
	ImGui::Text("Team 1");

	ImGui::PushItemWidth(100);
	ImGui::InputFloat("money1", &firstMoney);
	ImGui::PopItemWidth();
	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
	ImGui::PushItemWidth(100);
	ImGui::InputFloat("multiplier1", &firstMulti);
	ImGui::PopItemWidth();

	ImGui::Spacing();

	ImGui::Text("Team 2");

	ImGui::PushItemWidth(100);
	ImGui::InputFloat("money2", &secondMoney);
	ImGui::PopItemWidth();
	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
	ImGui::PushItemWidth(100);
	ImGui::InputFloat("multiplier2", &secondMulti);
	ImGui::PopItemWidth();

	ImGui::Spacing();

	if ((firstMoney != tmpFirstMoney) || (firstMulti != tmpFirstMulti) || (secondMoney != tmpSecondMoney) || (secondMulti != tmpSecondMulti) || (totalMoney != tmpTotalMoney)) {
		if (firstMoney == 0)
			zerosSum++;
		if (firstMulti == 0)
			zerosSum++;
		if (secondMoney == 0)
			zerosSum++;
		if (secondMulti == 0)
			zerosSum++;
		if (totalMoney == 0)
			zerosSum++;
		if (zerosSum > 2)
			ImGui::TextColored({ 255,0,0,255 }, "error: minimum 3 boxes must be filled (not == 0)");
		else {
			if (totalMoney == 0) {
				if (firstMoney == 0) 
					firstMoney = (secondMoney*secondMulti)/firstMulti;
				if (secondMoney == 0)
					secondMoney = (firstMoney * firstMulti) / secondMulti;
				if(firstMulti == 0)
					firstMulti = (secondMoney * secondMulti) / firstMoney;
				if(secondMulti == 0)
					secondMulti = (firstMoney * firstMulti) / secondMoney;
				totalMoney = firstMoney + secondMoney;
			}
			else {
				firstMoney = (secondMulti * totalMoney) / (firstMulti + secondMulti);
				secondMoney = totalMoney - firstMoney;
			}

			profitMoney = (firstMoney * firstMulti)-totalMoney;
			profitPercentage = ((100 * (profitMoney+totalMoney)) / totalMoney)-100;

		}

		

		

		zerosSum = 0;

	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::PushItemWidth(100);
	ImGui::Text(("total profit = " + std::to_string(profitMoney)).c_str());
	ImGui::PopItemWidth();
	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
	ImGui::PushItemWidth(100);
	ImGui::Text(("percentage = " + std::to_string(profitPercentage)).c_str());
	ImGui::PopItemWidth();

	ImGui::Spacing();

	if(ImGui::Button("reset all")) {
		firstMoney = 0;
		firstMulti = 0;
		secondMoney =0;
		secondMulti =0;
		profitMoney = 0;
		profitPercentage = 0;

	}
	
	

	ImGui::End();
}

