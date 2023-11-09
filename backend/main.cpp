#include "gui.h"

#include <thread>

int __stdcall wWinMain(
	HINSTANCE instance,
	HINSTANCE previousInstance,
	PWSTR arguments,
	int commandShow)
{
	// creating gui
	gui::CreateHWindow("Fork bets calculator", "fork calculator class");
	gui::CreateDevice();
	gui::CreateImGui();

	while (gui::isRunning) {
		gui::BeginRender();
		gui::Render();
		gui::EndRender();


		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	gui::DestroyImGui();
	gui::DestroyDevice();
	gui::DestroyHWindow();

	return EXIT_SUCCESS;
}