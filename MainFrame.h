#pragma once

#include <wx/frame.h>
#include <wx/treectrl.h>
#include <wx/arrstr.h>
#include "FilePackage.h"

class MainFrame : public wxFrame
{
    bool AddDirectoryToTree(wxTreeItemId Parent, const wxArrayString& Files, const wxString& Directory);
    bool AddFileToTree(wxTreeItemId Parent, const wxString& FilePath, const wxString& Directory);
    bool AddFileToTree(const wxString& FilePath);
    void Clear();
    wxString GetItemPath(wxTreeItemId Item);
    const wxString DefaultPName = "Unnamed package";
    wxArrayString AllFolders;
    wxTreeCtrl* FileList;
    FilePackage Package;
    wxTreeItemId RootItem;
    wxString PackageName;
public:
	MainFrame();
	~MainFrame();
    void OnDropFiles(wxDropFilesEvent& event);
    void OnFileCreate(wxCommandEvent& event);
    void OnFileOpen(wxCommandEvent& event);
    void OnFileSave(wxCommandEvent& event);
    void OnUnpack(wxCommandEvent& event);
    void OnAddFiles(wxCommandEvent& event);
    void OnAddDirectory(wxCommandEvent& event);
    void OnDeleteFiles(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
};