#include "MainForm.h"
#include "Data/DatabaseManager.h"

using namespace System;
using namespace System::Windows::Forms;

[STAThreadAttribute]
int main(array<String^>^ /*args*/)
{
	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false);

	String^ dbError;
	if (!NetworkMonitoringSystem::Data::DatabaseManager::TryInitialize(dbError))
	{
		MessageBox::Show(
			dbError != nullptr ? dbError : L"Неизвестная ошибка базы данных.",
			L"Ошибка базы данных",
			MessageBoxButtons::OK,
			MessageBoxIcon::Error);
		return 1;
	}

	Application::Run(gcnew NetworkMonitoringSystem::MainForm());
	return 0;
}
