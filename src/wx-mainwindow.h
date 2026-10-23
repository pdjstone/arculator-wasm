extern "C"
{
	#include "arc.h"
	#include "plat_input.h"
}

class MyPlotEvent;
wxDECLARE_EVENT(myEVT_PLOT, MyPlotEvent);

class MainCanvas: public wxWindow
{
public:
	MainCanvas(wxWindow *parent, const wxString& title, const wxPoint& pos, const wxSize& size)
	: wxWindow(parent, wxID_ANY, pos, size, wxDEFAULT_FRAME_STYLE | wxWANTS_CHARS),
	  bmp(2048, 2048, 24),
	  dx(0), dy(0), last_mouse_pos(0, 0)
	{
	}

	virtual ~MainCanvas() {}

	int dx, dy, b;

private:
	void OnPaint(wxPaintEvent &event);
	void OnMyPlot(MyPlotEvent &event);
	void OnMotion(wxMouseEvent &event);
	void OnKeyDown(wxKeyEvent &event);
	void OnKeyUp(wxKeyEvent &event);

	void OnLeftDown(wxMouseEvent &event)
	{
		if (!mousecapture)
		{
			mouse_capture_enable();
			CaptureMouse();
			SetCursor(wxCursor(wxCURSOR_BLANK));
			mousecapture = 1;
			updatemips = 1;
		}
		b |= 1;
		event.Skip();
	}
	void OnLeftUp(wxMouseEvent &event)
	{
		b &= ~1;
		event.Skip();
	}
	void OnRightDown(wxMouseEvent &event)
	{
		b |= 2;
		event.Skip();
	}
	void OnRightUp(wxMouseEvent &event)
	{
		b &= ~2;
		event.Skip();
	}
	void OnMiddleDown(wxMouseEvent &event)
	{
		b |= 4;
		event.Skip();
	}
	void OnMiddleUp(wxMouseEvent &event)
	{
		b &= ~4;
		event.Skip();
	}

	wxBitmap bmp;
	wxPoint last_mouse_pos;


	wxDECLARE_EVENT_TABLE();
};

class MainFrame: public wxFrame
{
public:
	MainFrame(Frame *parent, const wxString& title, const wxPoint& pos,
			const wxSize& size);
	virtual ~MainFrame();

	MainCanvas *GetCanvas() { return canvas; }
	void UpdateMenu()
	{
		parent->UpdateMenu();
	}

private:
	void OnClose(wxCloseEvent& event);
	void OnMenu(wxCommandEvent& event);

	MainCanvas *canvas;
	Frame *parent;

	wxDECLARE_EVENT_TABLE();
};
