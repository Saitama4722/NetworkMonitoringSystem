#include "MainForm.h"
#include "ProgressForm.h"
#include "DeviceEditForm.h"
#include "KeyAssignForm.h"
#include "KeyEditForm.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Drawing;
using namespace System::Threading::Tasks;
using namespace System::Windows::Forms;
using namespace NetworkMonitoringSystem::Models;
using namespace NetworkMonitoringSystem::Services;

namespace NetworkMonitoringSystem {

	namespace {

		ref class CheckAllJob sealed
		{
		public:
			MainForm^ host;
			ProgressForm^ progress;
			DeviceService^ devSvc;
			MonitoringService^ monSvc;

			void Run()
			{
				Exception^ caught = nullptr;
				try
				{
					List<Device^>^ list = devSvc->GetAllDevices();
					int total = list->Count;
					progress->SetProgress(0,
						L"\u041f\u0440\u043e\u0432\u0435\u0440\u043a\u0430 \u0443\u0441\u0442\u0440\u043e\u0439\u0441\u0442\u0432...");
					for (int idx = 0; idx < total; idx++)
					{
						Device^ d = list[idx];
						String^ ip = d->IpAddress != nullptr ? d->IpAddress->Trim() : String::Empty;
						String^ display = (!String::IsNullOrWhiteSpace(d->Name)) ? d->Name->Trim() : ip;
						if (String::IsNullOrWhiteSpace(display))
							display = L"\u2014";
						int pctStart = total > 0 ? (idx * 100 / total) : 0;
						progress->SetProgress(pctStart,
							String::Format(
								L"\u041f\u0440\u043e\u0432\u0435\u0440\u043a\u0430 \u0443\u0441\u0442\u0440\u043e\u0439\u0441\u0442\u0432\u0430: {0}",
								display));

						DeviceStatus st;
						if (String::IsNullOrWhiteSpace(ip) || !monSvc->IsValidIpAddress(ip))
							st = DeviceStatus::Offline;
						else
							st = monSvc->ProbeReachability(ip);

						try
						{
							d->Status = st;
							devSvc->UpdateDevice(d);
						}
						catch (Exception^ ex)
						{
							caught = ex;
							break;
						}

						int pctDone = (idx + 1) * 100 / total;
						progress->SetProgress(pctDone,
							String::Format(
								L"\u041f\u0440\u043e\u0432\u0435\u0440\u043a\u0430 \u0443\u0441\u0442\u0440\u043e\u0439\u0441\u0442\u0432\u0430: {0}",
								display));
					}
				}
				catch (Exception^ ex)
				{
					caught = ex;
				}

				String^ okMsg = L"\u041f\u0440\u043e\u0432\u0435\u0440\u043a\u0430 \u0437\u0430\u0432\u0435\u0440\u0448\u0435\u043d\u0430";
				host->BeginInvoke(
					gcnew Action<ProgressForm^, String^, Exception^>(host, &MainForm::CompleteDeviceCheck),
					progress,
					okMsg,
					caught);
			}
		};

		ref class CheckSelectedJob sealed
		{
		public:
			MainForm^ host;
			ProgressForm^ progress;
			DeviceService^ devSvc;
			MonitoringService^ monSvc;
			Device^ device;
			String^ displayIp;

			void Run()
			{
				Exception^ caught = nullptr;
				try
				{
					progress->SetProgress(0,
						String::Format(
							L"\u041f\u0440\u043e\u0432\u0435\u0440\u043a\u0430 \u0443\u0441\u0442\u0440\u043e\u0439\u0441\u0442\u0432\u0430: {0}",
							displayIp));
					DeviceStatus st = monSvc->ProbeReachability(displayIp);
					device->Status = st;
					devSvc->UpdateDevice(device);
					progress->SetProgress(100,
						String::Format(
							L"\u041f\u0440\u043e\u0432\u0435\u0440\u043a\u0430 \u0443\u0441\u0442\u0440\u043e\u0439\u0441\u0442\u0432\u0430: {0}",
							displayIp));
				}
				catch (Exception^ ex)
				{
					caught = ex;
				}

				String^ okMsg =
					L"\u041f\u0440\u043e\u0432\u0435\u0440\u043a\u0430 \u0432\u044b\u0431\u0440\u0430\u043d\u043d\u043e\u0433\u043e \u0443\u0441\u0442\u0440\u043e\u0439\u0441\u0442\u0432\u0430 \u0432\u044b\u043f\u043e\u043b\u043d\u0435\u043d\u0430";
				host->BeginInvoke(
					gcnew Action<ProgressForm^, String^, Exception^>(host, &MainForm::CompleteDeviceCheck),
					progress,
					okMsg,
					caught);
			}
		};

		void StyleActionButton(Button^ b)
		{
			b->AutoSize = false;
			b->MinimumSize = Drawing::Size(118, 30);
			b->Size = Drawing::Size(118, 30);
			b->Margin = System::Windows::Forms::Padding(0, 0, 10, 0);
		}

		void StyleCheckButton(Button^ b)
		{
			b->AutoSize = true;
			b->MinimumSize = Drawing::Size(1, 30);
			b->Margin = System::Windows::Forms::Padding(0, 0, 10, 8);
		}

		void StyleMainDataGrid(DataGridView^ g)
		{
			g->ReadOnly = true;
			g->AllowUserToAddRows = false;
			g->AllowUserToDeleteRows = false;
			g->AllowUserToResizeRows = false;
			g->SelectionMode = DataGridViewSelectionMode::FullRowSelect;
			g->MultiSelect = false;
			g->RowHeadersVisible = false;
			g->AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode::Fill;
			g->BackgroundColor = Color::White;
			g->BorderStyle = BorderStyle::Fixed3D;
			g->ColumnHeadersHeightSizeMode = DataGridViewColumnHeadersHeightSizeMode::AutoSize;
			g->EnableHeadersVisualStyles = true;
		}

	}

	String^ MainForm::DeviceTypeRu(DeviceTypeEnum t)
	{
		switch (t)
		{
		case DeviceTypeEnum::Workstation:
			return L"\u0420\u0430\u0431\u043e\u0447\u0430\u044f \u0441\u0442\u0430\u043d\u0446\u0438\u044f";
		case DeviceTypeEnum::Server:
			return L"\u0421\u0435\u0440\u0432\u0435\u0440";
		case DeviceTypeEnum::Router:
			return L"\u041c\u0430\u0440\u0448\u0440\u0443\u0442\u0438\u0437\u0430\u0442\u043e\u0440";
		case DeviceTypeEnum::Media:
			return L"\u041c\u0435\u0434\u0438\u0430";
		case DeviceTypeEnum::Other:
			return L"\u041f\u0440\u043e\u0447\u0435\u0435";
		default:
			return L"\u041d\u0435\u0438\u0437\u0432\u0435\u0441\u0442\u043d\u043e";
		}
	}

	String^ MainForm::StatusRu(DeviceStatus s)
	{
		switch (s)
		{
		case DeviceStatus::Online:
			return L"\u0412 \u0441\u0435\u0442\u0438";
		case DeviceStatus::Offline:
			return L"\u041d\u0435 \u0432 \u0441\u0435\u0442\u0438";
		default:
			return L"\u041d\u0435\u0438\u0437\u0432\u0435\u0441\u0442\u043d\u043e";
		}
	}

	String^ MainForm::KeyStateRu(bool busy)
	{
		return busy
			? L"\u0417\u0430\u043d\u044f\u0442"
			: L"\u0421\u0432\u043e\u0431\u043e\u0434\u0435\u043d";
	}

	void MainForm::SetStatusText(String^ text)
	{
		if (statusMain_ == nullptr)
			return;
		statusMain_->Text = text != nullptr ? text : String::Empty;
	}

	void MainForm::SetDeviceCheckButtonsEnabled(bool enabled)
	{
		if (btnCheckSelected_ != nullptr)
			btnCheckSelected_->Enabled = enabled;
		if (btnCheckAll_ != nullptr)
			btnCheckAll_->Enabled = enabled;
	}

	void MainForm::CompleteDeviceCheck(ProgressForm^ prog, String^ successStatus, Exception^ error)
	{
		try
		{
			if (prog != nullptr && !prog->IsDisposed)
				prog->Close();
		}
		catch (Exception^)
		{
		}
		RefreshDevices();
		SetDeviceCheckButtonsEnabled(true);
		if (error != nullptr)
		{
			SetStatusText(L"\u041e\u0448\u0438\u0431\u043a\u0430 \u043f\u0440\u0438 \u043f\u0440\u043e\u0432\u0435\u0440\u043a\u0435");
			MessageBox::Show(
				error->Message,
				L"\u041e\u0448\u0438\u0431\u043a\u0430",
				MessageBoxButtons::OK,
				MessageBoxIcon::Error);
		}
		else
		{
			SetStatusText(successStatus != nullptr ? successStatus : String::Empty);
		}
	}

	void MainForm::OnDevicesCellFormatting(Object^ /*sender*/, DataGridViewCellFormattingEventArgs^ e)
	{
		if (devicesGrid_ == nullptr || e->RowIndex < 0 || e->ColumnIndex < 0)
			return;
		DataGridViewColumn^ col = devicesGrid_->Columns[e->ColumnIndex];
		if (col == nullptr || col->Name != L"Status")
			return;
		DataGridViewRow^ row = devicesGrid_->Rows[e->RowIndex];
		if (row->IsNewRow)
			return;
		Device^ d = dynamic_cast<Device^>(row->Tag);
		if (d == nullptr)
			return;
		switch (d->Status)
		{
		case DeviceStatus::Online:
			e->CellStyle->BackColor = Color::FromArgb(220, 245, 220);
			e->CellStyle->ForeColor = Color::FromArgb(18, 90, 28);
			e->CellStyle->SelectionBackColor = Color::FromArgb(180, 220, 185);
			e->CellStyle->SelectionForeColor = Color::FromArgb(18, 90, 28);
			break;
		case DeviceStatus::Offline:
			e->CellStyle->BackColor = Color::FromArgb(255, 228, 228);
			e->CellStyle->ForeColor = Color::FromArgb(110, 25, 25);
			e->CellStyle->SelectionBackColor = Color::FromArgb(235, 195, 195);
			e->CellStyle->SelectionForeColor = Color::FromArgb(110, 25, 25);
			break;
		default:
			e->CellStyle->BackColor = Color::FromArgb(236, 236, 236);
			e->CellStyle->ForeColor = Color::FromArgb(55, 55, 55);
			e->CellStyle->SelectionBackColor = Color::FromArgb(200, 200, 200);
			e->CellStyle->SelectionForeColor = Color::FromArgb(55, 55, 55);
			break;
		}
	}

	void MainForm::OnKeysCellFormatting(Object^ /*sender*/, DataGridViewCellFormattingEventArgs^ e)
	{
		if (keysGrid_ == nullptr || e->RowIndex < 0 || e->ColumnIndex < 0)
			return;
		DataGridViewColumn^ col = keysGrid_->Columns[e->ColumnIndex];
		if (col == nullptr || col->Name != L"KeyState")
			return;
		DataGridViewRow^ row = keysGrid_->Rows[e->RowIndex];
		if (row->IsNewRow)
			return;
		KeyItem^ k = dynamic_cast<KeyItem^>(row->Tag);
		if (k == nullptr)
			return;
		if (k->IsBusy)
		{
			e->CellStyle->BackColor = Color::FromArgb(255, 240, 210);
			e->CellStyle->ForeColor = Color::FromArgb(125, 80, 10);
			e->CellStyle->SelectionBackColor = Color::FromArgb(240, 215, 175);
			e->CellStyle->SelectionForeColor = Color::FromArgb(125, 80, 10);
		}
		else
		{
			e->CellStyle->BackColor = Color::FromArgb(218, 240, 255);
			e->CellStyle->ForeColor = Color::FromArgb(12, 75, 105);
			e->CellStyle->SelectionBackColor = Color::FromArgb(185, 220, 240);
			e->CellStyle->SelectionForeColor = Color::FromArgb(12, 75, 105);
		}
	}

	MainForm::MainForm()
	{
		deviceService_ = gcnew DeviceService();
		keyService_ = gcnew KeyService();
		monitoringService_ = gcnew MonitoringService();
		SuspendLayout();
		Text = L"\u0421\u0438\u0441\u0442\u0435\u043c\u0430 \u043c\u043e\u043d\u0438\u0442\u043e\u0440\u0438\u043d\u0433\u0430 \u041b\u0412\u0421";
		ClientSize = Drawing::Size(960, 680);
		MinimumSize = Drawing::Size(780, 560);
		StartPosition = FormStartPosition::CenterScreen;
		Load += gcnew EventHandler(this, &MainForm::OnLoadForm);
		BuildLayout();
		ResumeLayout(false);
	}

	void MainForm::BuildLayout()
	{
		statusStrip_ = gcnew StatusStrip();
		statusStrip_->SizingGrip = false;
		statusMain_ = gcnew ToolStripStatusLabel();
		statusMain_->Spring = true;
		statusMain_->TextAlign = ContentAlignment::MiddleLeft;
		statusMain_->Margin = System::Windows::Forms::Padding(6, 2, 6, 2);
		statusMain_->Text = L"\u0413\u043e\u0442\u043e\u0432\u043e";
		statusStrip_->Items->Add(statusMain_);
		statusStrip_->Dock = DockStyle::Bottom;

		Panel^ fill = gcnew Panel();
		fill->Dock = DockStyle::Fill;
		fill->Padding = System::Windows::Forms::Padding(0);
		fill->Margin = System::Windows::Forms::Padding(0);

		TableLayoutPanel^ root = gcnew TableLayoutPanel();
		root->Dock = DockStyle::Fill;
		root->ColumnCount = 1;
		root->RowCount = 3;
		root->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100.0F));
		root->RowStyles->Add(gcnew RowStyle(SizeType::AutoSize));
		root->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 120.0F));
		root->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100.0F));
		root->Padding = System::Windows::Forms::Padding(12, 12, 12, 8);
		root->Margin = System::Windows::Forms::Padding(0);

		Label^ titleLabel = gcnew Label();
		titleLabel->Text =
			L"\u041c\u043e\u043d\u0438\u0442\u043e\u0440\u0438\u043d\u0433 \u0443\u0441\u0442\u0440\u043e\u0439\u0441\u0442\u0432 \u041b\u0412\u0421 \u0438 \u0443\u0447\u0451\u0442 \u043a\u043b\u044e\u0447\u0435\u0439 \u043f\u043e\u0434\u043a\u043b\u044e\u0447\u0435\u043d\u0438\u044f";
		titleLabel->AutoSize = true;
		titleLabel->Dock = DockStyle::Fill;
		titleLabel->TextAlign = ContentAlignment::MiddleLeft;
		titleLabel->Margin = System::Windows::Forms::Padding(0, 0, 0, 8);
		titleLabel->Font = gcnew System::Drawing::Font(
			titleLabel->Font, System::Drawing::FontStyle::Bold);
		root->Controls->Add(titleLabel, 0, 0);

		GroupBox^ aboutBox = gcnew GroupBox();
		aboutBox->Text = L"\u041e \u043f\u0440\u043e\u0433\u0440\u0430\u043c\u043c\u0435";
		aboutBox->AutoSize = false;
		aboutBox->Dock = DockStyle::Fill;
		aboutBox->Margin = System::Windows::Forms::Padding(0, 0, 0, 10);
		aboutBox->Padding = System::Windows::Forms::Padding(10, 8, 10, 10);
		Label^ aboutText = gcnew Label();
		aboutText->Dock = DockStyle::Fill;
		aboutText->AutoSize = false;
		aboutText->TextAlign = ContentAlignment::TopLeft;
		aboutText->Font = gcnew System::Drawing::Font(
			aboutText->Font->FontFamily, 9.0F, System::Drawing::FontStyle::Regular);
		aboutText->Text =
			L"\u041f\u0440\u0438\u043b\u043e\u0436\u0435\u043d\u0438\u0435 \u0432\u0435\u0434\u0451\u0442 \u0443\u0447\u0451\u0442 \u0441\u0435\u0442\u0435\u0432\u044b\u0445 \u0443\u0441\u0442\u0440\u043e\u0439\u0441\u0442\u0432, "
			L"\u043f\u043e\u0437\u0432\u043e\u043b\u044f\u0435\u0442 \u043f\u0440\u043e\u0432\u0435\u0440\u044f\u0442\u044c \u0434\u043e\u0441\u0442\u0443\u043f\u043d\u043e\u0441\u0442\u044c \u043f\u043e IP-\u0430\u0434\u0440\u0435\u0441\u0443.\r\n"
			L"\u0423\u043f\u0440\u0430\u0432\u043b\u0435\u043d\u0438\u0435 \u043a\u043b\u044e\u0447\u0430\u043c\u0438 \u043f\u043e\u0434\u043a\u043b\u044e\u0447\u0435\u043d\u0438\u044f; \u0441\u0432\u0435\u0434\u0435\u043d\u0438\u044f \u0445\u0440\u0430\u043d\u044f\u0442\u0441\u044f "
			L"\u0432 \u043b\u043e\u043a\u0430\u043b\u044c\u043d\u043e\u0439 \u0431\u0430\u0437\u0435 \u0434\u0430\u043d\u043d\u044b\u0445 SQLite.";
		aboutBox->Controls->Add(aboutText);
		root->Controls->Add(aboutBox, 0, 1);

		mainTabs_ = gcnew TabControl();
		mainTabs_->Dock = DockStyle::Fill;
		mainTabs_->Margin = System::Windows::Forms::Padding(0);

		TabPage^ tabDevices = gcnew TabPage();
		tabDevices->Text = L"\u0421\u0435\u0442\u0435\u0432\u044b\u0435 \u0443\u0441\u0442\u0440\u043e\u0439\u0441\u0442\u0432\u0430";
		tabDevices->Padding = System::Windows::Forms::Padding(10);
		BuildDevicesTab(tabDevices);

		TabPage^ tabKeys = gcnew TabPage();
		tabKeys->Text = L"\u041a\u043b\u044e\u0447\u0438 \u043f\u043e\u0434\u043a\u043b\u044e\u0447\u0435\u043d\u0438\u044f";
		tabKeys->Padding = System::Windows::Forms::Padding(10);
		BuildKeysTab(tabKeys);

		mainTabs_->TabPages->Add(tabDevices);
		mainTabs_->TabPages->Add(tabKeys);

		root->Controls->Add(mainTabs_, 0, 2);
		fill->Controls->Add(root);
		Controls->Add(fill);
		Controls->Add(statusStrip_);
	}

	void MainForm::BuildDevicesTab(TabPage^ page)
	{
		Panel^ host = gcnew Panel();
		host->Dock = DockStyle::Fill;
		host->Padding = System::Windows::Forms::Padding(0);

		FlowLayoutPanel^ toolbar = gcnew FlowLayoutPanel();
		toolbar->Dock = DockStyle::Top;
		toolbar->AutoSize = true;
		toolbar->Padding = System::Windows::Forms::Padding(0, 8, 0, 8);
		toolbar->FlowDirection = FlowDirection::LeftToRight;
		toolbar->WrapContents = true;

		btnAdd_ = gcnew Button();
		btnAdd_->Text = L"\u0414\u043e\u0431\u0430\u0432\u0438\u0442\u044c";
		btnAdd_->Click += gcnew EventHandler(this, &MainForm::OnAddClick);
		StyleActionButton(btnAdd_);

		btnEdit_ = gcnew Button();
		btnEdit_->Text = L"\u0418\u0437\u043c\u0435\u043d\u0438\u0442\u044c";
		btnEdit_->Click += gcnew EventHandler(this, &MainForm::OnEditClick);
		StyleActionButton(btnEdit_);

		btnDelete_ = gcnew Button();
		btnDelete_->Text = L"\u0423\u0434\u0430\u043b\u0438\u0442\u044c";
		btnDelete_->Click += gcnew EventHandler(this, &MainForm::OnDeleteClick);
		StyleActionButton(btnDelete_);

		btnRefresh_ = gcnew Button();
		btnRefresh_->Text = L"\u041e\u0431\u043d\u043e\u0432\u0438\u0442\u044c";
		btnRefresh_->Click += gcnew EventHandler(this, &MainForm::OnRefreshClick);
		StyleActionButton(btnRefresh_);

		btnCheckSelected_ = gcnew Button();
		btnCheckSelected_->Text =
			L"\u041f\u0440\u043e\u0432\u0435\u0440\u0438\u0442\u044c \u0432\u044b\u0431\u0440\u0430\u043d\u043d\u043e\u0435";
		btnCheckSelected_->Click += gcnew EventHandler(this, &MainForm::OnCheckSelectedClick);
		StyleCheckButton(btnCheckSelected_);

		btnCheckAll_ = gcnew Button();
		btnCheckAll_->Text =
			L"\u041f\u0440\u043e\u0432\u0435\u0440\u0438\u0442\u044c \u0432\u0441\u0435";
		btnCheckAll_->Click += gcnew EventHandler(this, &MainForm::OnCheckAllClick);
		StyleCheckButton(btnCheckAll_);

		toolbar->Controls->Add(btnAdd_);
		toolbar->Controls->Add(btnEdit_);
		toolbar->Controls->Add(btnDelete_);
		toolbar->Controls->Add(btnRefresh_);
		toolbar->Controls->Add(btnCheckSelected_);
		toolbar->Controls->Add(btnCheckAll_);

		devicesGrid_ = gcnew DataGridView();
		devicesGrid_->Dock = DockStyle::Fill;
		devicesGrid_->Margin = System::Windows::Forms::Padding(0);
		StyleMainDataGrid(devicesGrid_);

		DataGridViewTextBoxColumn^ colId = gcnew DataGridViewTextBoxColumn();
		colId->Name = L"Id";
		colId->HeaderText = L"\u0418\u0434";
		colId->DataPropertyName = L"Id";
		colId->FillWeight = 50;

		DataGridViewTextBoxColumn^ colName = gcnew DataGridViewTextBoxColumn();
		colName->Name = L"Name";
		colName->HeaderText = L"\u041d\u0430\u0437\u0432\u0430\u043d\u0438\u0435";
		colName->DataPropertyName = L"Name";
		colName->FillWeight = 120;

		DataGridViewTextBoxColumn^ colIp = gcnew DataGridViewTextBoxColumn();
		colIp->Name = L"IpAddress";
		colIp->HeaderText = L"IP-\u0430\u0434\u0440\u0435\u0441";
		colIp->DataPropertyName = L"IpAddress";
		colIp->FillWeight = 100;

		DataGridViewTextBoxColumn^ colType = gcnew DataGridViewTextBoxColumn();
		colType->Name = L"DeviceType";
		colType->HeaderText = L"\u0422\u0438\u043f \u0443\u0441\u0442\u0440\u043e\u0439\u0441\u0442\u0432\u0430";
		colType->FillWeight = 100;

		DataGridViewTextBoxColumn^ colStatus = gcnew DataGridViewTextBoxColumn();
		colStatus->Name = L"Status";
		colStatus->HeaderText = L"\u0421\u0442\u0430\u0442\u0443\u0441 \u0434\u043e\u0441\u0442\u0443\u043f\u043d\u043e\u0441\u0442\u0438";
		colStatus->FillWeight = 80;

		devicesGrid_->Columns->Add(colId);
		devicesGrid_->Columns->Add(colName);
		devicesGrid_->Columns->Add(colIp);
		devicesGrid_->Columns->Add(colType);
		devicesGrid_->Columns->Add(colStatus);
		devicesGrid_->CellFormatting += gcnew DataGridViewCellFormattingEventHandler(
			this, &MainForm::OnDevicesCellFormatting);

		host->Controls->Add(devicesGrid_);
		host->Controls->Add(toolbar);
		page->Controls->Add(host);
	}

	void MainForm::BuildKeysTab(TabPage^ page)
	{
		Panel^ host = gcnew Panel();
		host->Dock = DockStyle::Fill;
		host->Padding = System::Windows::Forms::Padding(0);

		FlowLayoutPanel^ toolbar = gcnew FlowLayoutPanel();
		toolbar->Dock = DockStyle::Top;
		toolbar->AutoSize = true;
		toolbar->Padding = System::Windows::Forms::Padding(0, 8, 0, 8);
		toolbar->FlowDirection = FlowDirection::LeftToRight;
		toolbar->WrapContents = true;

		btnKeyAdd_ = gcnew Button();
		btnKeyAdd_->Text = L"\u0414\u043e\u0431\u0430\u0432\u0438\u0442\u044c";
		btnKeyAdd_->Click += gcnew EventHandler(this, &MainForm::OnKeyAddClick);
		StyleActionButton(btnKeyAdd_);

		btnKeyEdit_ = gcnew Button();
		btnKeyEdit_->Text = L"\u0418\u0437\u043c\u0435\u043d\u0438\u0442\u044c";
		btnKeyEdit_->Click += gcnew EventHandler(this, &MainForm::OnKeyEditClick);
		StyleActionButton(btnKeyEdit_);

		btnKeyDelete_ = gcnew Button();
		btnKeyDelete_->Text = L"\u0423\u0434\u0430\u043b\u0438\u0442\u044c";
		btnKeyDelete_->Click += gcnew EventHandler(this, &MainForm::OnKeyDeleteClick);
		StyleActionButton(btnKeyDelete_);

		btnKeyAssign_ = gcnew Button();
		btnKeyAssign_->Text =
			L"\u041d\u0430\u0437\u043d\u0430\u0447\u0438\u0442\u044c \u043a\u043b\u044e\u0447";
		btnKeyAssign_->Click += gcnew EventHandler(this, &MainForm::OnKeyAssignClick);
		btnKeyAssign_->AutoSize = false;
		btnKeyAssign_->MinimumSize = Drawing::Size(168, 30);
		btnKeyAssign_->Size = Drawing::Size(168, 30);
		btnKeyAssign_->Margin = System::Windows::Forms::Padding(0, 0, 10, 0);

		btnKeyRelease_ = gcnew Button();
		btnKeyRelease_->Text =
			L"\u041e\u0441\u0432\u043e\u0431\u043e\u0434\u0438\u0442\u044c \u043a\u043b\u044e\u0447";
		btnKeyRelease_->Click += gcnew EventHandler(this, &MainForm::OnKeyReleaseClick);
		btnKeyRelease_->AutoSize = false;
		btnKeyRelease_->MinimumSize = Drawing::Size(168, 30);
		btnKeyRelease_->Size = Drawing::Size(168, 30);
		btnKeyRelease_->Margin = System::Windows::Forms::Padding(0, 0, 10, 0);

		btnKeyRefresh_ = gcnew Button();
		btnKeyRefresh_->Text = L"\u041e\u0431\u043d\u043e\u0432\u0438\u0442\u044c";
		btnKeyRefresh_->Click += gcnew EventHandler(this, &MainForm::OnKeyRefreshClick);
		StyleActionButton(btnKeyRefresh_);

		toolbar->Controls->Add(btnKeyAdd_);
		toolbar->Controls->Add(btnKeyEdit_);
		toolbar->Controls->Add(btnKeyDelete_);
		toolbar->Controls->Add(btnKeyAssign_);
		toolbar->Controls->Add(btnKeyRelease_);
		toolbar->Controls->Add(btnKeyRefresh_);

		keysGrid_ = gcnew DataGridView();
		keysGrid_->Dock = DockStyle::Fill;
		keysGrid_->Margin = System::Windows::Forms::Padding(0);
		StyleMainDataGrid(keysGrid_);

		DataGridViewTextBoxColumn^ cId = gcnew DataGridViewTextBoxColumn();
		cId->Name = L"Id";
		cId->HeaderText = L"\u0418\u0434";
		cId->FillWeight = 45;

		DataGridViewTextBoxColumn^ cName = gcnew DataGridViewTextBoxColumn();
		cName->Name = L"KeyName";
		cName->HeaderText = L"\u0418\u043c\u044f \u043a\u043b\u044e\u0447\u0430";
		cName->FillWeight = 120;

		DataGridViewTextBoxColumn^ cBusy = gcnew DataGridViewTextBoxColumn();
		cBusy->Name = L"KeyState";
		cBusy->HeaderText = L"\u0421\u043e\u0441\u0442\u043e\u044f\u043d\u0438\u0435 \u043a\u043b\u044e\u0447\u0430";
		cBusy->FillWeight = 90;

		DataGridViewTextBoxColumn^ cAsg = gcnew DataGridViewTextBoxColumn();
		cAsg->Name = L"AssignedTo";
		cAsg->HeaderText = L"\u041d\u0430\u0437\u043d\u0430\u0447\u0435\u043d\u043e";
		cAsg->FillWeight = 120;

		DataGridViewTextBoxColumn^ cNote = gcnew DataGridViewTextBoxColumn();
		cNote->Name = L"Note";
		cNote->HeaderText = L"\u041f\u0440\u0438\u043c\u0435\u0447\u0430\u043d\u0438\u0435";
		cNote->FillWeight = 150;

		keysGrid_->Columns->Add(cId);
		keysGrid_->Columns->Add(cName);
		keysGrid_->Columns->Add(cBusy);
		keysGrid_->Columns->Add(cAsg);
		keysGrid_->Columns->Add(cNote);
		keysGrid_->CellFormatting += gcnew DataGridViewCellFormattingEventHandler(
			this, &MainForm::OnKeysCellFormatting);

		host->Controls->Add(keysGrid_);
		host->Controls->Add(toolbar);
		page->Controls->Add(host);
	}

	void MainForm::OnLoadForm(Object^ /*sender*/, EventArgs^ /*e*/)
	{
		RefreshDevices();
		RefreshKeys();
		SetStatusText(L"\u0413\u043e\u0442\u043e\u0432\u043e");
	}

	Device^ MainForm::TryGetSelectedDevice()
	{
		if (devicesGrid_->SelectedRows->Count < 1)
			return nullptr;
		DataGridViewRow^ row = devicesGrid_->SelectedRows[0];
		if (row->IsNewRow)
			return nullptr;
		return dynamic_cast<Device^>(row->Tag);
	}

	KeyItem^ MainForm::TryGetSelectedKey()
	{
		if (keysGrid_->SelectedRows->Count < 1)
			return nullptr;
		DataGridViewRow^ row = keysGrid_->SelectedRows[0];
		if (row->IsNewRow)
			return nullptr;
		return dynamic_cast<KeyItem^>(row->Tag);
	}

	void MainForm::RefreshDevices()
	{
		try
		{
			List<Device^>^ list = deviceService_->GetAllDevices();
			devicesGrid_->Rows->Clear();
			for each (Device^ d in list)
			{
				int i = devicesGrid_->Rows->Add();
				DataGridViewRow^ row = devicesGrid_->Rows[i];
				row->Cells[L"Id"]->Value = d->Id;
				row->Cells[L"Name"]->Value = d->Name;
				row->Cells[L"IpAddress"]->Value = d->IpAddress;
				row->Cells[L"DeviceType"]->Value = DeviceTypeRu(d->DeviceType);
				row->Cells[L"Status"]->Value = StatusRu(d->Status);
				row->Tag = d;
			}
			SetStatusText(L"\u0421\u043f\u0438\u0441\u043e\u043a \u0443\u0441\u0442\u0440\u043e\u0439\u0441\u0442\u0432 \u043e\u0431\u043d\u043e\u0432\u043b\u0451\u043d");
		}
		catch (Exception^ ex)
		{
			SetStatusText(L"\u041e\u0448\u0438\u0431\u043a\u0430 \u0437\u0430\u0433\u0440\u0443\u0437\u043a\u0438 \u0443\u0441\u0442\u0440\u043e\u0439\u0441\u0442\u0432");
			MessageBox::Show(
				ex->Message,
				L"\u041e\u0448\u0438\u0431\u043a\u0430 \u0437\u0430\u0433\u0440\u0443\u0437\u043a\u0438",
				MessageBoxButtons::OK,
				MessageBoxIcon::Error);
		}
	}

	void MainForm::RefreshKeys()
	{
		try
		{
			List<KeyItem^>^ list = keyService_->GetAllKeys();
			keysGrid_->Rows->Clear();
			for each (KeyItem^ k in list)
			{
				int i = keysGrid_->Rows->Add();
				DataGridViewRow^ row = keysGrid_->Rows[i];
				row->Cells[L"Id"]->Value = k->Id;
				row->Cells[L"KeyName"]->Value = k->KeyName;
				row->Cells[L"KeyState"]->Value = KeyStateRu(k->IsBusy);
				row->Cells[L"AssignedTo"]->Value = k->AssignedTo != nullptr ? k->AssignedTo : String::Empty;
				row->Cells[L"Note"]->Value = k->Note != nullptr ? k->Note : String::Empty;
				row->Tag = k;
			}
			SetStatusText(L"\u0421\u043f\u0438\u0441\u043e\u043a \u043a\u043b\u044e\u0447\u0435\u0439 \u043e\u0431\u043d\u043e\u0432\u043b\u0451\u043d");
		}
		catch (Exception^ ex)
		{
			SetStatusText(L"\u041e\u0448\u0438\u0431\u043a\u0430 \u0437\u0430\u0433\u0440\u0443\u0437\u043a\u0438 \u043a\u043b\u044e\u0447\u0435\u0439");
			MessageBox::Show(
				ex->Message,
				L"\u041e\u0448\u0438\u0431\u043a\u0430 \u0437\u0430\u0433\u0440\u0443\u0437\u043a\u0438 \u043a\u043b\u044e\u0447\u0435\u0439",
				MessageBoxButtons::OK,
				MessageBoxIcon::Error);
		}
	}

	void MainForm::OnAddClick(Object^ /*sender*/, EventArgs^ /*e*/)
	{
		DeviceEditForm^ dlg = gcnew DeviceEditForm(nullptr);
		if (dlg->ShowDialog(this) != System::Windows::Forms::DialogResult::OK)
			return;
		Device^ d = dlg->ResultDevice;
		if (d == nullptr)
			return;
		try
		{
			deviceService_->AddDevice(d);
			RefreshDevices();
			SetStatusText(L"\u0423\u0441\u0442\u0440\u043e\u0439\u0441\u0442\u0432\u043e \u0434\u043e\u0431\u0430\u0432\u043b\u0435\u043d\u043e");
		}
		catch (Exception^ ex)
		{
			SetStatusText(L"\u041e\u0448\u0438\u0431\u043a\u0430 \u0441\u043e\u0445\u0440\u0430\u043d\u0435\u043d\u0438\u044f");
			MessageBox::Show(
				ex->Message,
				L"\u041e\u0448\u0438\u0431\u043a\u0430",
				MessageBoxButtons::OK,
				MessageBoxIcon::Error);
		}
	}

	void MainForm::OnEditClick(Object^ /*sender*/, EventArgs^ /*e*/)
	{
		Device^ sel = TryGetSelectedDevice();
		if (sel == nullptr)
		{
			MessageBox::Show(
				L"\u0412\u044b\u0431\u0435\u0440\u0438\u0442\u0435 \u0441\u0442\u0440\u043e\u043a\u0443 \u0441 \u0443\u0441\u0442\u0440\u043e\u0439\u0441\u0442\u0432\u043e\u043c.",
				L"\u0418\u0437\u043c\u0435\u043d\u0435\u043d\u0438\u0435",
				MessageBoxButtons::OK,
				MessageBoxIcon::Information);
			return;
		}
		Device^ fresh = deviceService_->GetDeviceById(sel->Id);
		if (fresh == nullptr)
		{
			MessageBox::Show(
				L"\u0423\u0441\u0442\u0440\u043e\u0439\u0441\u0442\u0432\u043e \u043d\u0435 \u043d\u0430\u0439\u0434\u0435\u043d\u043e.",
				L"\u0418\u0437\u043c\u0435\u043d\u0435\u043d\u0438\u0435",
				MessageBoxButtons::OK,
				MessageBoxIcon::Warning);
			RefreshDevices();
			return;
		}
		DeviceEditForm^ dlg = gcnew DeviceEditForm(fresh);
		if (dlg->ShowDialog(this) != System::Windows::Forms::DialogResult::OK)
			return;
		Device^ d = dlg->ResultDevice;
		if (d == nullptr)
			return;
		try
		{
			deviceService_->UpdateDevice(d);
			RefreshDevices();
			SetStatusText(L"\u0414\u0430\u043d\u043d\u044b\u0435 \u0443\u0441\u0442\u0440\u043e\u0439\u0441\u0442\u0432\u0430 \u0441\u043e\u0445\u0440\u0430\u043d\u0435\u043d\u044b");
		}
		catch (Exception^ ex)
		{
			SetStatusText(L"\u041e\u0448\u0438\u0431\u043a\u0430 \u0441\u043e\u0445\u0440\u0430\u043d\u0435\u043d\u0438\u044f");
			MessageBox::Show(
				ex->Message,
				L"\u041e\u0448\u0438\u0431\u043a\u0430",
				MessageBoxButtons::OK,
				MessageBoxIcon::Error);
		}
	}

	void MainForm::OnDeleteClick(Object^ /*sender*/, EventArgs^ /*e*/)
	{
		Device^ sel = TryGetSelectedDevice();
		if (sel == nullptr)
		{
			MessageBox::Show(
				L"\u0412\u044b\u0431\u0435\u0440\u0438\u0442\u0435 \u0441\u0442\u0440\u043e\u043a\u0443 \u0441 \u0443\u0441\u0442\u0440\u043e\u0439\u0441\u0442\u0432\u043e\u043c.",
				L"\u0423\u0434\u0430\u043b\u0435\u043d\u0438\u0435",
				MessageBoxButtons::OK,
				MessageBoxIcon::Information);
			return;
		}
		System::Windows::Forms::DialogResult dr = MessageBox::Show(
			L"\u0423\u0434\u0430\u043b\u0438\u0442\u044c \u0432\u044b\u0431\u0440\u0430\u043d\u043d\u043e\u0435 \u0443\u0441\u0442\u0440\u043e\u0439\u0441\u0442\u0432\u043e?",
			L"\u041f\u043e\u0434\u0442\u0432\u0435\u0440\u0436\u0434\u0435\u043d\u0438\u0435",
			MessageBoxButtons::YesNo,
			MessageBoxIcon::Question);
		if (dr != System::Windows::Forms::DialogResult::Yes)
			return;
		try
		{
			deviceService_->DeleteDevice(sel->Id);
			RefreshDevices();
			SetStatusText(L"\u0423\u0441\u0442\u0440\u043e\u0439\u0441\u0442\u0432\u043e \u0443\u0434\u0430\u043b\u0435\u043d\u043e");
		}
		catch (Exception^ ex)
		{
			SetStatusText(L"\u041e\u0448\u0438\u0431\u043a\u0430 \u0443\u0434\u0430\u043b\u0435\u043d\u0438\u044f");
			MessageBox::Show(
				ex->Message,
				L"\u041e\u0448\u0438\u0431\u043a\u0430",
				MessageBoxButtons::OK,
				MessageBoxIcon::Error);
		}
	}

	void MainForm::OnRefreshClick(Object^ /*sender*/, EventArgs^ /*e*/)
	{
		RefreshDevices();
	}

	void MainForm::OnCheckSelectedClick(Object^ /*sender*/, EventArgs^ /*e*/)
	{
		Device^ sel = TryGetSelectedDevice();
		if (sel == nullptr)
		{
			MessageBox::Show(
				L"\u0412\u044b\u0431\u0435\u0440\u0438\u0442\u0435 \u0441\u0442\u0440\u043e\u043a\u0443 \u0441 \u0443\u0441\u0442\u0440\u043e\u0439\u0441\u0442\u0432\u043e\u043c.",
				L"\u041f\u0440\u043e\u0432\u0435\u0440\u043a\u0430 \u0434\u043e\u0441\u0442\u0443\u043f\u043d\u043e\u0441\u0442\u0438",
				MessageBoxButtons::OK,
				MessageBoxIcon::Information);
			return;
		}
		Device^ fresh = deviceService_->GetDeviceById(sel->Id);
		if (fresh == nullptr)
		{
			MessageBox::Show(
				L"\u0423\u0441\u0442\u0440\u043e\u0439\u0441\u0442\u0432\u043e \u043d\u0435 \u043d\u0430\u0439\u0434\u0435\u043d\u043e.",
				L"\u041f\u0440\u043e\u0432\u0435\u0440\u043a\u0430 \u0434\u043e\u0441\u0442\u0443\u043f\u043d\u043e\u0441\u0442\u0438",
				MessageBoxButtons::OK,
				MessageBoxIcon::Warning);
			RefreshDevices();
			return;
		}
		String^ ip = fresh->IpAddress != nullptr ? fresh->IpAddress->Trim() : String::Empty;
		if (String::IsNullOrWhiteSpace(ip) || !monitoringService_->IsValidIpAddress(ip))
		{
			MessageBox::Show(
				L"\u0423 \u0443\u0441\u0442\u0440\u043e\u0439\u0441\u0442\u0432\u0430 \u043d\u0435\u0442 \u043a\u043e\u0440\u0440\u0435\u043a\u0442\u043d\u043e\u0433\u043e IP-\u0430\u0434\u0440\u0435\u0441\u0430 \u0434\u043b\u044f \u043f\u0440\u043e\u0432\u0435\u0440\u043a\u0438.",
				L"\u041f\u0440\u043e\u0432\u0435\u0440\u043a\u0430 \u0434\u043e\u0441\u0442\u0443\u043f\u043d\u043e\u0441\u0442\u0438",
				MessageBoxButtons::OK,
				MessageBoxIcon::Warning);
			return;
		}

		ProgressForm^ prog = gcnew ProgressForm();
		prog->Owner = this;
		prog->Show(this);
		prog->BringToFront();
		SetDeviceCheckButtonsEnabled(false);

		CheckSelectedJob^ job = gcnew CheckSelectedJob();
		job->host = this;
		job->progress = prog;
		job->devSvc = deviceService_;
		job->monSvc = monitoringService_;
		job->device = fresh;
		job->displayIp = ip;

		Task::Run(gcnew Action(job, &CheckSelectedJob::Run));
	}

	void MainForm::OnCheckAllClick(Object^ /*sender*/, EventArgs^ /*e*/)
	{
		List<Device^>^ list = deviceService_->GetAllDevices();
		if (list == nullptr || list->Count == 0)
		{
			RefreshDevices();
			SetStatusText(L"\u041f\u0440\u043e\u0432\u0435\u0440\u043a\u0430 \u0437\u0430\u0432\u0435\u0440\u0448\u0435\u043d\u0430");
			return;
		}

		ProgressForm^ prog = gcnew ProgressForm();
		prog->Owner = this;
		prog->Show(this);
		prog->BringToFront();
		SetDeviceCheckButtonsEnabled(false);

		CheckAllJob^ job = gcnew CheckAllJob();
		job->host = this;
		job->progress = prog;
		job->devSvc = deviceService_;
		job->monSvc = monitoringService_;

		Task::Run(gcnew Action(job, &CheckAllJob::Run));
	}

	void MainForm::OnKeyAddClick(Object^ /*sender*/, EventArgs^ /*e*/)
	{
		KeyEditForm^ dlg = gcnew KeyEditForm(nullptr);
		if (dlg->ShowDialog(this) != System::Windows::Forms::DialogResult::OK)
			return;
		KeyItem^ k = dlg->ResultKey;
		if (k == nullptr)
			return;
		try
		{
			keyService_->AddKey(k);
			RefreshKeys();
			SetStatusText(L"\u041a\u043b\u044e\u0447 \u0434\u043e\u0431\u0430\u0432\u043b\u0435\u043d");
		}
		catch (Exception^ ex)
		{
			SetStatusText(L"\u041e\u0448\u0438\u0431\u043a\u0430 \u0441\u043e\u0445\u0440\u0430\u043d\u0435\u043d\u0438\u044f");
			MessageBox::Show(
				ex->Message,
				L"\u041e\u0448\u0438\u0431\u043a\u0430",
				MessageBoxButtons::OK,
				MessageBoxIcon::Error);
		}
	}

	void MainForm::OnKeyEditClick(Object^ /*sender*/, EventArgs^ /*e*/)
	{
		KeyItem^ sel = TryGetSelectedKey();
		if (sel == nullptr)
		{
			MessageBox::Show(
				L"\u0412\u044b\u0431\u0435\u0440\u0438\u0442\u0435 \u0441\u0442\u0440\u043e\u043a\u0443 \u0441 \u043a\u043b\u044e\u0447\u043e\u043c.",
				L"\u0418\u0437\u043c\u0435\u043d\u0435\u043d\u0438\u0435",
				MessageBoxButtons::OK,
				MessageBoxIcon::Information);
			return;
		}
		KeyItem^ fresh = keyService_->GetKeyById(sel->Id);
		if (fresh == nullptr)
		{
			MessageBox::Show(
				L"\u041a\u043b\u044e\u0447 \u043d\u0435 \u043d\u0430\u0439\u0434\u0435\u043d.",
				L"\u0418\u0437\u043c\u0435\u043d\u0435\u043d\u0438\u0435",
				MessageBoxButtons::OK,
				MessageBoxIcon::Warning);
			RefreshKeys();
			return;
		}
		KeyEditForm^ dlg = gcnew KeyEditForm(fresh);
		if (dlg->ShowDialog(this) != System::Windows::Forms::DialogResult::OK)
			return;
		KeyItem^ k = dlg->ResultKey;
		if (k == nullptr)
			return;
		try
		{
			keyService_->UpdateKey(k);
			RefreshKeys();
			SetStatusText(L"\u0414\u0430\u043d\u043d\u044b\u0435 \u043a\u043b\u044e\u0447\u0430 \u0441\u043e\u0445\u0440\u0430\u043d\u0435\u043d\u044b");
		}
		catch (Exception^ ex)
		{
			SetStatusText(L"\u041e\u0448\u0438\u0431\u043a\u0430 \u0441\u043e\u0445\u0440\u0430\u043d\u0435\u043d\u0438\u044f");
			MessageBox::Show(
				ex->Message,
				L"\u041e\u0448\u0438\u0431\u043a\u0430",
				MessageBoxButtons::OK,
				MessageBoxIcon::Error);
		}
	}

	void MainForm::OnKeyAssignClick(Object^ /*sender*/, EventArgs^ /*e*/)
	{
		KeyItem^ sel = TryGetSelectedKey();
		if (sel == nullptr)
		{
			MessageBox::Show(
				L"\u0412\u044b\u0431\u0435\u0440\u0438\u0442\u0435 \u0441\u0442\u0440\u043e\u043a\u0443 \u0441 \u043a\u043b\u044e\u0447\u043e\u043c.",
				L"\u041d\u0430\u0437\u043d\u0430\u0447\u0435\u043d\u0438\u0435 \u043a\u043b\u044e\u0447\u0430",
				MessageBoxButtons::OK,
				MessageBoxIcon::Information);
			return;
		}
		KeyItem^ fresh = keyService_->GetKeyById(sel->Id);
		if (fresh == nullptr)
		{
			MessageBox::Show(
				L"\u041a\u043b\u044e\u0447 \u043d\u0435 \u043d\u0430\u0439\u0434\u0435\u043d.",
				L"\u041d\u0430\u0437\u043d\u0430\u0447\u0435\u043d\u0438\u0435 \u043a\u043b\u044e\u0447\u0430",
				MessageBoxButtons::OK,
				MessageBoxIcon::Warning);
			RefreshKeys();
			return;
		}
		if (fresh->IsBusy)
		{
			MessageBox::Show(
				L"\u041a\u043b\u044e\u0447 \u0443\u0436\u0435 \u0437\u0430\u043d\u044f\u0442. \u0421\u043d\u0430\u0447\u0430\u043b\u0430 \u043e\u0441\u0432\u043e\u0431\u043e\u0434\u0438\u0442\u0435 \u0435\u0433\u043e.",
				L"\u041d\u0430\u0437\u043d\u0430\u0447\u0435\u043d\u0438\u0435 \u043a\u043b\u044e\u0447\u0430",
				MessageBoxButtons::OK,
				MessageBoxIcon::Information);
			return;
		}
		List<Device^>^ devs = deviceService_->GetAllDevices();
		KeyAssignForm^ dlg = gcnew KeyAssignForm(devs);
		if (dlg->ShowDialog(this) != System::Windows::Forms::DialogResult::OK)
			return;
		String^ target = dlg->ResultTargetName;
		if (String::IsNullOrWhiteSpace(target))
			return;
		try
		{
			keyService_->AssignKeyToTarget(fresh->Id, target);
			RefreshKeys();
			SetStatusText(L"\u041a\u043b\u044e\u0447 \u043d\u0430\u0437\u043d\u0430\u0447\u0435\u043d");
		}
		catch (Exception^ ex)
		{
			SetStatusText(L"\u041e\u0448\u0438\u0431\u043a\u0430 \u043d\u0430\u0437\u043d\u0430\u0447\u0435\u043d\u0438\u044f");
			MessageBox::Show(
				ex->Message,
				L"\u041e\u0448\u0438\u0431\u043a\u0430",
				MessageBoxButtons::OK,
				MessageBoxIcon::Error);
		}
	}

	void MainForm::OnKeyReleaseClick(Object^ /*sender*/, EventArgs^ /*e*/)
	{
		KeyItem^ sel = TryGetSelectedKey();
		if (sel == nullptr)
		{
			MessageBox::Show(
				L"\u0412\u044b\u0431\u0435\u0440\u0438\u0442\u0435 \u0441\u0442\u0440\u043e\u043a\u0443 \u0441 \u043a\u043b\u044e\u0447\u043e\u043c.",
				L"\u041e\u0441\u0432\u043e\u0431\u043e\u0436\u0434\u0435\u043d\u0438\u0435 \u043a\u043b\u044e\u0447\u0430",
				MessageBoxButtons::OK,
				MessageBoxIcon::Information);
			return;
		}
		KeyItem^ fresh = keyService_->GetKeyById(sel->Id);
		if (fresh == nullptr)
		{
			MessageBox::Show(
				L"\u041a\u043b\u044e\u0447 \u043d\u0435 \u043d\u0430\u0439\u0434\u0435\u043d.",
				L"\u041e\u0441\u0432\u043e\u0431\u043e\u0436\u0434\u0435\u043d\u0438\u0435 \u043a\u043b\u044e\u0447\u0430",
				MessageBoxButtons::OK,
				MessageBoxIcon::Warning);
			RefreshKeys();
			return;
		}
		if (!fresh->IsBusy && String::IsNullOrWhiteSpace(fresh->AssignedTo))
		{
			MessageBox::Show(
				L"\u041a\u043b\u044e\u0447 \u0443\u0436\u0435 \u0441\u0432\u043e\u0431\u043e\u0434\u0435\u043d.",
				L"\u041e\u0441\u0432\u043e\u0431\u043e\u0436\u0434\u0435\u043d\u0438\u0435 \u043a\u043b\u044e\u0447\u0430",
				MessageBoxButtons::OK,
				MessageBoxIcon::Information);
			return;
		}
		System::Windows::Forms::DialogResult dr = MessageBox::Show(
			L"\u041e\u0441\u0432\u043e\u0431\u043e\u0434\u0438\u0442\u044c \u0432\u044b\u0431\u0440\u0430\u043d\u043d\u044b\u0439 \u043a\u043b\u044e\u0447? \u041d\u0430\u0437\u043d\u0430\u0447\u0435\u043d\u0438\u0435 \u0431\u0443\u0434\u0435\u0442 \u0441\u0431\u0440\u043e\u0448\u0435\u043d\u043e.",
			L"\u041f\u043e\u0434\u0442\u0432\u0435\u0440\u0436\u0434\u0435\u043d\u0438\u0435",
			MessageBoxButtons::YesNo,
			MessageBoxIcon::Question);
		if (dr != System::Windows::Forms::DialogResult::Yes)
			return;
		try
		{
			keyService_->ReleaseKey(fresh->Id);
			RefreshKeys();
			SetStatusText(L"\u041a\u043b\u044e\u0447 \u043e\u0441\u0432\u043e\u0431\u043e\u0436\u0434\u0451\u043d");
		}
		catch (Exception^ ex)
		{
			SetStatusText(L"\u041e\u0448\u0438\u0431\u043a\u0430 \u043e\u0441\u0432\u043e\u0431\u043e\u0436\u0434\u0435\u043d\u0438\u044f");
			MessageBox::Show(
				ex->Message,
				L"\u041e\u0448\u0438\u0431\u043a\u0430",
				MessageBoxButtons::OK,
				MessageBoxIcon::Error);
		}
	}

	void MainForm::OnKeyDeleteClick(Object^ /*sender*/, EventArgs^ /*e*/)
	{
		KeyItem^ sel = TryGetSelectedKey();
		if (sel == nullptr)
		{
			MessageBox::Show(
				L"\u0412\u044b\u0431\u0435\u0440\u0438\u0442\u0435 \u0441\u0442\u0440\u043e\u043a\u0443 \u0441 \u043a\u043b\u044e\u0447\u043e\u043c.",
				L"\u0423\u0434\u0430\u043b\u0435\u043d\u0438\u0435",
				MessageBoxButtons::OK,
				MessageBoxIcon::Information);
			return;
		}
		System::Windows::Forms::DialogResult dr = MessageBox::Show(
			L"\u0423\u0434\u0430\u043b\u0438\u0442\u044c \u0432\u044b\u0431\u0440\u0430\u043d\u043d\u044b\u0439 \u043a\u043b\u044e\u0447? \u0421\u0432\u044f\u0437\u0430\u043d\u043d\u044b\u0435 \u0437\u0430\u043f\u0438\u0441\u0438 \u043d\u0430\u0437\u043d\u0430\u0447\u0435\u043d\u0438\u0439 \u0442\u043e\u0436\u0435 \u0431\u0443\u0434\u0443\u0442 \u0443\u0434\u0430\u043b\u0435\u043d\u044b.",
			L"\u041f\u043e\u0434\u0442\u0432\u0435\u0440\u0436\u0434\u0435\u043d\u0438\u0435",
			MessageBoxButtons::YesNo,
			MessageBoxIcon::Question);
		if (dr != System::Windows::Forms::DialogResult::Yes)
			return;
		try
		{
			keyService_->DeleteKey(sel->Id);
			RefreshKeys();
			SetStatusText(L"\u041a\u043b\u044e\u0447 \u0443\u0434\u0430\u043b\u0451\u043d");
		}
		catch (Exception^ ex)
		{
			SetStatusText(L"\u041e\u0448\u0438\u0431\u043a\u0430 \u0443\u0434\u0430\u043b\u0435\u043d\u0438\u044f");
			MessageBox::Show(
				ex->Message,
				L"\u041e\u0448\u0438\u0431\u043a\u0430",
				MessageBoxButtons::OK,
				MessageBoxIcon::Error);
		}
	}

	void MainForm::OnKeyRefreshClick(Object^ /*sender*/, EventArgs^ /*e*/)
	{
		RefreshKeys();
	}

}
