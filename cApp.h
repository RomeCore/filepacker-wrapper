#pragma once
#include <wx/wx.h>
#include "MainFrame.h"
class cApp : public wxApp
{
	MainFrame* MainWindow;
public:
	cApp();
	~cApp();
	virtual bool OnInit();
};