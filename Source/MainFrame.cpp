#include "MainFrame.h"
#include <wx/menu.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/dirdlg.h>
#include <wx/sizer.h>
#include <wx/treectrl.h>
#include <wx/imaglist.h>
#include <wx/image.h>
#include <wx/imagpng.h>
#include <wx/dirctrl.h>
#include <wx/busyinfo.h>
#include <wx/dir.h>
#include <wx/progdlg.h>
#include <wx/button.h>
#include <wx/translation.h>
#include <filesystem>
#include <initializer_list>
#include <wx/toolbar.h>
#include <sstream>
#include <iostream>
#include <list>

#undef DeleteFile

enum class EVENT
{
	ID_M_CreateFile = 1,
	ID_M_OpenFile,
	ID_M_SaveFile,
	ID_M_Unpack,
	ID_B_AddFiles,
	ID_B_AddDirectory,
	ID_B_DeleteFiles
};

int TotalFilesProcessedWhenAdd;
wxGenericProgressDialog* ProgressDlgAdd = nullptr;
int TotalFilesProcessedWhenUnpack;
wxGenericProgressDialog* ProgressDlgUnpack = nullptr;
bool MainFrame::AddDirectoryToTree(wxTreeItemId Parent, const wxArrayString& Files, const wxString& Directory)
{
	for (auto& FilePath : Files)
	{
		if (wxDirExists(FilePath))
		{
			wxArrayString DirFiles;
			for (const auto& entry : std::filesystem::directory_iterator(FilePath.c_str().AsChar()))
				DirFiles.push_back(entry.path().string());

			wxString ResultDirectory;
			wxString FileName = FilePath.substr(FilePath.find_last_of("\\") + 1);
			if (Directory.empty())
				ResultDirectory = FileName;
			else
				ResultDirectory = Directory + "\\" + FileName;
			auto Node = FileList->AppendItem(Parent, FileName, wxTheFileIconsTable->folder);
			if (!AddDirectoryToTree(Node, DirFiles, ResultDirectory))
				return false;
		}
		else if (wxFileExists(FilePath))
			if (!AddFileToTree(Parent, FilePath, Directory))
				return false;
	}
	return true;
}

bool MainFrame::AddFileToTree(wxTreeItemId Parent, const wxString& FilePath, const wxString& Directory)
{
	std::ifstream FileStream(FilePath.c_str().AsChar(), std::ios::binary);
	if (!FileStream.is_open())
	{
		wxMessageBox(wxString(_("File not opened at directory :\"")) + FilePath + "\"", _("File not opened!"), wxOK | wxICON_INFORMATION);
		return true;
	}
	wxString FileName = FilePath.substr(FilePath.find_last_of("\\") + 1);
	wxString PathAtPackage;
	if (Directory.empty())
		PathAtPackage = FileName;
	else
		PathAtPackage = Directory + "\\" + FileName;
	ProgressDlgAdd->Update(TotalFilesProcessedWhenAdd, PathAtPackage);

	Package.AddFile(FileStream, PathAtPackage.c_str().AsChar());
	FileStream.close();
	FileList->AppendItem(Parent, FileName, wxTheFileIconsTable->GetIconID(FileName.substr(FileName.find_last_of(".") + 1), ""));
	TotalFilesProcessedWhenAdd++;
	return !ProgressDlgAdd->WasCancelled();
}

bool MainFrame::AddFileToTree(const wxString& FilePath)
{
	std::istringstream Stream(FilePath.ToStdString());
	std::string Part;
	wxTreeItemId CurrentParent = FileList->GetRootItem();
	
	while (std::getline(Stream, Part, '\\')) {
		if (FilePath.EndsWith(Part))
		{
			FileList->AppendItem(CurrentParent, Part, wxTheFileIconsTable->GetIconID(Part.substr(Part.find_last_of(".") + 1), ""));
			return false;
		}
		else
		{
			if (FileList->HasChildren(CurrentParent))
			{
				wxTreeItemIdValue Cookie;
				wxTreeItemId NewParent = FileList->GetFirstChild(CurrentParent, Cookie);

				bool Found = false;
				while (NewParent.IsOk())
				{
					if (FileList->GetItemText(NewParent) == Part)
					{
						CurrentParent = NewParent;
						Found = true;
						break;
					}
					NewParent = FileList->GetNextSibling(NewParent);
				}
				if (!Found)
					CurrentParent = FileList->AppendItem(CurrentParent, Part, wxTheFileIconsTable->folder);
			}
			else
			{
				CurrentParent = FileList->AppendItem(CurrentParent, Part, wxTheFileIconsTable->folder);
			}
		}
	}
	return false;
}
wxString MainFrame::GetItemPath(wxTreeItemId Item)
{
	wxString Result = FileList->GetItemText(Item);
	wxTreeItemId Current = FileList->GetItemParent(Item);
	while (Current != 0)
	{
		Result = FileList->GetItemText(Current) + "\\" + Result;
		Current = FileList->GetItemParent(Current);
	}

	return Result;
}

void MainFrame::Clear()
{
	PackageName = DefaultPName;
	Package.ClearFiles();
	FileList->DeleteAllItems();
	RootItem = FileList->AppendItem(0, DefaultPName, wxTheFileIconsTable->folder);
}

wxButton* AddFilesButton;
wxButton* DeleteFilesButton;

MainFrame::MainFrame() : wxFrame(nullptr, wxID_ANY, "File packer")
{
	wxMenu* menuFile = new wxMenu;
	menuFile->Append((int)EVENT::ID_M_CreateFile, _("&Create\tCtrl-Q"), _("Create a new package"));
	menuFile->Append((int)EVENT::ID_M_OpenFile, _("&Open...\tCtrl-W"), _("Open package from (.pack) file"));
	menuFile->Append((int)EVENT::ID_M_SaveFile, _("&Save...\tCtrl-E"), _("Save package to (.pack) file"));
	menuFile->AppendSeparator();
	menuFile->Append((int)EVENT::ID_M_Unpack, _("&Unpack...\tCtrl-R"), _("Unpack files from package"));
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT);
	wxMenu* menuHelp = new wxMenu;
	menuHelp->Append(wxID_ABOUT);
	wxMenuBar* menuBar = new wxMenuBar;

	menuBar->Append(menuFile, _("&File"));
	menuBar->Append(menuHelp, _("&Help"));

	SetMenuBar(menuBar);

	CreateStatusBar();
	SetStatusText(_("Welcome to file packer!"));

	FileList = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_MULTIPLE | wxTR_DEFAULT_STYLE);
	RootItem = FileList->AppendItem(0, DefaultPName, wxTheFileIconsTable->folder);
	FileList->DragAcceptFiles(true);
	FileList->Connect(wxEVT_DROP_FILES, wxDropFilesEventHandler(MainFrame::OnDropFiles), NULL, this);
	FileList->SetImageList(wxTheFileIconsTable->GetSmallImageList());

	wxBoxSizer* AllSizer = new wxBoxSizer(wxHORIZONTAL);
	AllSizer->Add(FileList, 1, wxEXPAND);
	wxBoxSizer* ToolSizer = new wxBoxSizer(wxVERTICAL);
	AllSizer->Add(ToolSizer, 1);

	wxImage::AddHandler(new wxPNGHandler);
	wxBitmap AddFiles("Icons/AddFiles.png", wxBITMAP_TYPE_PNG);
	wxBitmap AddDirectory("Icons/AddDirectory.png", wxBITMAP_TYPE_PNG);
	wxBitmap DeleteFiles("Icons/Delete.png", wxBITMAP_TYPE_PNG);

	wxToolBar* toolbar = CreateToolBar();
	toolbar->AddTool((int)EVENT::ID_B_AddFiles, _("Add multiple files"), AddFiles, _("Add multiple files"));
	toolbar->AddTool((int)EVENT::ID_B_AddDirectory, _("Add directory"), AddDirectory, _("Add directory"));
	toolbar->AddTool((int)EVENT::ID_B_DeleteFiles, _("Delete files or directories"), DeleteFiles, _("Delete files or directories"));
	toolbar->Realize();

	Bind(wxEVT_MENU, &MainFrame::OnFileCreate, this, (int)EVENT::ID_M_CreateFile);
	Bind(wxEVT_MENU, &MainFrame::OnFileOpen, this, (int)EVENT::ID_M_OpenFile);
	Bind(wxEVT_MENU, &MainFrame::OnFileSave, this, (int)EVENT::ID_M_SaveFile);
	Bind(wxEVT_MENU, &MainFrame::OnUnpack, this, (int)EVENT::ID_M_Unpack);
	Bind(wxEVT_TOOL, &MainFrame::OnAddFiles, this, (int)EVENT::ID_B_AddFiles);
	Bind(wxEVT_TOOL, &MainFrame::OnAddDirectory, this, (int)EVENT::ID_B_AddDirectory);
	Bind(wxEVT_TOOL, &MainFrame::OnDeleteFiles, this, (int)EVENT::ID_B_DeleteFiles);
	Bind(wxEVT_MENU, &MainFrame::OnAbout, this, wxID_ABOUT);
	Bind(wxEVT_MENU, &MainFrame::OnExit, this, wxID_EXIT);

	SetSizerAndFit(AllSizer);
}

MainFrame::~MainFrame()
{

}



void MainFrame::OnDropFiles(wxDropFilesEvent& event)
{
	if (event.GetNumberOfFiles() > 0) {
		wxString* Dropped = event.GetFiles();
		if (!ProgressDlgAdd) ProgressDlgAdd = new wxGenericProgressDialog;
		wxArrayString Files;
		int TotalFilesToProcess = 0;
		for (int i = 0; i < event.GetNumberOfFiles(); i++)
		{
			wxArrayString Dummy;
			if (wxFileExists(Dropped[i]))
				TotalFilesToProcess++;
			else if (wxDirExists(Dropped[i]))
				TotalFilesToProcess += wxDir::GetAllFiles(Dropped[i], &Dummy);
			Files.push_back(Dropped[i]);
		}
		ProgressDlgAdd->Create(_("Adding dropped files..."), _("Starting..."), TotalFilesToProcess, this, wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_CAN_ABORT);
		AddDirectoryToTree(RootItem, Files, "");
		ProgressDlgAdd->Hide();
	}
}

void MainFrame::OnFileCreate(wxCommandEvent& event)
{
	Clear();
}
void MainFrame::OnFileOpen(wxCommandEvent& event)
{
	Clear();
	wxFileDialog OpenChoose(this, _("Choose file to open"), "", "", _("PACK files (*.pack)|*.pack"), wxFD_FILE_MUST_EXIST | wxFD_OPEN);
	if (OpenChoose.ShowModal() == wxID_CANCEL)
		return;
	Package.LoadPackage((OpenChoose.GetDirectory() + "\\" + OpenChoose.GetFilename()).c_str().AsChar());
	for (auto& File : Package.GetFilesData())
	{
		AddFileToTree(File.first);
	}
}
void MainFrame::OnFileSave(wxCommandEvent& event)
{
	wxFileDialog SaveChoose(this, _("Choose name of file to save"), "", PackageName, _("PACK files (*.pack)|*.pack"), wxFD_OVERWRITE_PROMPT | wxFD_SAVE);
	if (SaveChoose.ShowModal() == wxID_CANCEL)
		return;
	Package.SavePackage((SaveChoose.GetDirectory() + "\\" + SaveChoose.GetFilename()).c_str().AsChar(), 5);
	PackageName = SaveChoose.GetFilename();
	FileList->SetItemText(RootItem, PackageName);
}
void MainFrame::OnUnpack(wxCommandEvent& event)
{
	wxDirDialog DirChoose(this, _("Choose directory to unpack file"), "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
	if (DirChoose.ShowModal() == wxID_CANCEL)
		return;
	std::filesystem::path TargetPath = DirChoose.GetPath().c_str().AsChar();
	if (!ProgressDlgUnpack) ProgressDlgUnpack = new wxGenericProgressDialog;
	TotalFilesProcessedWhenUnpack = 0;
	ProgressDlgUnpack->Create(_("Unpacking files..."), _("Starting..."), Package.GetFilesData().size(), this, wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_CAN_ABORT);
	for (auto& File : Package.GetFilesData())
	{
		ProgressDlgUnpack->Update(TotalFilesProcessedWhenUnpack, File.first);
		int Offset;
		if ((Offset = File.first.find_last_of("\\")) == std::string::npos)
			Offset = 0;
		if (Offset != 0)
			std::filesystem::create_directories(TargetPath / File.first.substr(0, Offset));
		std::ofstream FileStream(TargetPath / File.first, std::ios::binary);
		if (FileStream.is_open())
			Package.SaveToFile(File.first, FileStream);
		FileStream.close();
		TotalFilesProcessedWhenUnpack++;
	}
	ProgressDlgUnpack->Hide();

}

void MainFrame::OnAddFiles(wxCommandEvent& event)
{
	wxFileDialog AddChoose(this, _("Choose files to add"), "", "", _("Any files (*.*)|*.*"), wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE | wxFD_OPEN);
	if (AddChoose.ShowModal() == wxID_CANCEL)
		return;
	wxArrayString FileNames;
	AddChoose.GetFilenames(FileNames);
	wxString Directory = AddChoose.GetDirectory();
	if (!ProgressDlgAdd) ProgressDlgAdd = new wxGenericProgressDialog;
	wxArrayString Files;
	int TotalFilesToProcess = 0;
	for (auto& FileName : FileNames)
	{
		wxArrayString Dummy;
		if (wxFileExists(FileName))
			TotalFilesToProcess++;
		else if (wxDirExists(FileName))
			TotalFilesToProcess += wxDir::GetAllFiles(FileName, &Dummy);
		Files.push_back(Directory + "\\" + FileName);
	}
	ProgressDlgAdd->Create(_("Adding files..."), _("Starting..."), TotalFilesToProcess, this, wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_CAN_ABORT);
	AddDirectoryToTree(RootItem, Files, "");
	ProgressDlgAdd->Hide();
}

void MainFrame::OnAddDirectory(wxCommandEvent& event)
{
	wxDirDialog AddChoose(this, _("Choose files to add"), "", wxDD_DIR_MUST_EXIST | wxDD_MULTIPLE);
	if (AddChoose.ShowModal() == wxID_CANCEL)
		return;
	wxArrayString Directories;
	AddChoose.GetPaths(Directories);
	if (!ProgressDlgAdd) ProgressDlgAdd = new wxGenericProgressDialog;
	wxArrayString Files;
	int TotalFilesToProcess = 0;
	for (auto& Directory : Directories)
	{
		wxArrayString Dummy;
		TotalFilesToProcess += wxDir::GetAllFiles(Directory, &Dummy);
	}
	ProgressDlgAdd->Create(_("Adding files..."), _("Starting..."), TotalFilesToProcess, this, wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_CAN_ABORT);
	AddDirectoryToTree(RootItem, Directories, "");
	ProgressDlgAdd->Hide();
}

void MainFrame::OnDeleteFiles(wxCommandEvent& event)
{
	wxArrayTreeItemIds Selected;
	if (FileList->GetSelections(Selected) == 0)
	{
		wxMessageDialog* dial = new wxMessageDialog(NULL, _("There is no selected files to delete"), _("No selected"), wxOK | wxOK_DEFAULT | wxICON_INFORMATION);
		dial->ShowModal();
		return;
	}
	wxMessageDialog* dial = new wxMessageDialog(NULL, _("Are you sure to delete selected directories or files?"), _("Deleteing confirmation"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
	if (dial->ShowModal() == wxID_YES)
	{
		std::function<void(wxTreeItemId)> ProcChilds = [&](wxTreeItemId ToProc)
		{
			if (FileList->HasChildren(ToProc))
			{
				wxTreeItemIdValue Cookie;
				wxTreeItemId Child = FileList->GetFirstChild(ToProc, Cookie);
				while (Child != 0)
				{
					ProcChilds(Child);
					Child = FileList->GetNextSibling(Child);
				}
			}
			else
			{
				Package.DeleteFile(GetItemPath(ToProc).ToStdString());
			}
		};

		for (auto File : Selected)
		{
			if (File == RootItem)
			{
				wxMessageDialog* dial = new wxMessageDialog(NULL, _("Can't delete root package directory!"), _("Can't delete!"), wxOK | wxOK_DEFAULT | wxICON_ERROR);
				dial->ShowModal();
				return;
			}

			ProcChilds(File);
			FileList->DeleteChildren(File);
			FileList->Delete(File);
		}
	}
}

void MainFrame::OnExit(wxCommandEvent& event)
{
	Package.ClearFiles();
	FileList->DeleteAllItems();
	Close(true);
}


void MainFrame::OnAbout(wxCommandEvent& event)
{
	wxMessageBox(_("This is a FilePacker.lib application wrapper!"),
		_("About Packer"), wxOK | wxICON_INFORMATION);
}