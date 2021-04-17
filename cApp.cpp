#include "cApp.h"

wxIMPLEMENT_APP(cApp);

cApp::cApp()
{

}
cApp::~cApp()
{

}
bool cApp::OnInit()
{
	MainWindow = new MainFrame();
	MainWindow->Show(true);
	return true;
}