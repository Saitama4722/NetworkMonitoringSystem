#pragma once

namespace NetworkMonitoringSystem { ref class ProgressForm; }

#include "Models/Device.h"
#include "Models/KeyItem.h"
#include "Services/DeviceService.h"
#include "Services/KeyService.h"
#include "Services/MonitoringService.h"

namespace NetworkMonitoringSystem {

	using namespace System::Windows::Forms;

	public ref class MainForm sealed : public Form
	{
	public:
		MainForm();

	internal:
		void CompleteDeviceCheck(ProgressForm^ progress, System::String^ successStatus, System::Exception^ error);

	private:
		Services::DeviceService^ deviceService_;
		Services::KeyService^ keyService_;
		Services::MonitoringService^ monitoringService_;

		TabControl^ mainTabs_;
		DataGridView^ devicesGrid_;
		Button^ btnAdd_;
		Button^ btnEdit_;
		Button^ btnDelete_;
		Button^ btnRefresh_;
		Button^ btnCheckSelected_;
		Button^ btnCheckAll_;

		DataGridView^ keysGrid_;
		Button^ btnKeyAdd_;
		Button^ btnKeyEdit_;
		Button^ btnKeyDelete_;
		Button^ btnKeyAssign_;
		Button^ btnKeyRelease_;
		Button^ btnKeyRefresh_;

		void BuildLayout();
		void BuildDevicesTab(System::Windows::Forms::TabPage^ page);
		void BuildKeysTab(System::Windows::Forms::TabPage^ page);
		void RefreshDevices();
		void RefreshKeys();
		void OnAddClick(System::Object^ sender, System::EventArgs^ e);
		void OnEditClick(System::Object^ sender, System::EventArgs^ e);
		void OnDeleteClick(System::Object^ sender, System::EventArgs^ e);
		void OnRefreshClick(System::Object^ sender, System::EventArgs^ e);
		void OnCheckSelectedClick(System::Object^ sender, System::EventArgs^ e);
		void OnCheckAllClick(System::Object^ sender, System::EventArgs^ e);
		void OnKeyAddClick(System::Object^ sender, System::EventArgs^ e);
		void OnKeyEditClick(System::Object^ sender, System::EventArgs^ e);
		void OnKeyDeleteClick(System::Object^ sender, System::EventArgs^ e);
		void OnKeyAssignClick(System::Object^ sender, System::EventArgs^ e);
		void OnKeyReleaseClick(System::Object^ sender, System::EventArgs^ e);
		void OnKeyRefreshClick(System::Object^ sender, System::EventArgs^ e);
		void OnLoadForm(System::Object^ sender, System::EventArgs^ e);
		void OnDevicesCellFormatting(System::Object^ sender, System::Windows::Forms::DataGridViewCellFormattingEventArgs^ e);
		void OnKeysCellFormatting(System::Object^ sender, System::Windows::Forms::DataGridViewCellFormattingEventArgs^ e);
		void SetStatusText(System::String^ text);

		StatusStrip^ statusStrip_;
		ToolStripStatusLabel^ statusMain_;

		Models::Device^ TryGetSelectedDevice();
		Models::KeyItem^ TryGetSelectedKey();
		static System::String^ DeviceTypeRu(Models::DeviceTypeEnum t);
		static System::String^ StatusRu(Models::DeviceStatus s);
		static System::String^ KeyStateRu(bool busy);
		void SetDeviceCheckButtonsEnabled(bool enabled);
	};

}
