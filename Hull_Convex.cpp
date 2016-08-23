#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/wfstream.h>
#include <wx/dcbuffer.h>
#include <wx/dc.h>
#include <wx/scrolwin.h>

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <exception>
#include <limits>
#include <algorithm>
#include <iomanip>
#include <ctime>

using namespace std;

enum { ID_NEXT = 100 };

double PI = 3.1415926;

struct Point {
	int x, y;
	Point() : x(0), y(0) { }
	Point(int xx, int yy) : x(xx), y(yy) { }

	operator wxPoint() { return wxPoint(x, y); }
};

typedef pair< vector<Point>, vector<Point> > PVV;

bool operator<(const Point& a, const Point& b) 
{ 
	return a.y < b.y;
}

struct Line {
	Point a, b;
	Line(Point aa, Point bb) : a(aa), b(bb) { }
};

ostream& operator<<(ostream& os, const Point& a)
{
	return os << '(' << a.x << ', ' << a.y << ')';
}

double radian_to_degree(double r)
{
	return r * 180 / PI;
}

double inv_radian_to_degree(double r)
{
	return 360 - radian_to_degree(r);
}

double angle(Point centre, Point A, Point B, bool no_inv=true)
{
	double nomin = (A.x - centre.x)*(B.x - centre.x) + (A.y - centre.y)*(B.y - centre.y);
	double denom = sqrt(pow(A.x - centre.x, 2) + pow(A.y - centre.y, 2)) *
		sqrt(pow(B.x - centre.x, 2) + pow(B.y - centre.y, 2));
	if (no_inv) return radian_to_degree(acos(nomin / denom));
	else return inv_radian_to_degree(acos(nomin / denom));
}

int random(int min, int max)
{
	return min + rand() % (max - min + 1);
}

class Hull_Convex : public wxFrame
{
public :
	Hull_Convex();

	void PaintBackground(wxDC& dc);
	void OnPaint(wxPaintEvent&);
	void OnNextRandom(wxCommandEvent&);
	void OnQuit(wxCommandEvent&);

private :
	wxMenuBar* m_menubar;
	wxMenu* m_fichier;
	wxMenuItem* m_quit;
	wxToolBar* m_toolbar;

	vector<Point> rand_points;
	void gen_rand_points();

	vector<Point> envelope;
	void get_envelope();
	/*enum Side { left, right };
	vector<Point> get_most_of_next(Side);*/
	PVV get_most_of_next();

	/*struct Graham_Sort {
		Point pivot;
		Point greatest;
		bool operator()(const Point&, const Point&);
	};*/

	double cross_product(const Point& a, const Point& b, const Point& c);

	wxSpinCtrl* number_points;

	wxSize paint_zone;
private :
	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(Hull_Convex, wxFrame)
	EVT_MENU(wxID_EXIT, Hull_Convex::OnQuit)
	EVT_MENU(ID_NEXT, Hull_Convex::OnNextRandom)
	EVT_PAINT(Hull_Convex::OnPaint)
END_EVENT_TABLE()

Hull_Convex::Hull_Convex()
	: wxFrame(NULL, wxID_ANY, wxT("Hull_Convex"), wxDefaultPosition, wxSize(1100, 800)),
	paint_zone(1082, 668), rand_points(25)
{
	wxImage::AddHandler(new wxPNGHandler);
	m_menubar = new wxMenuBar;
	m_fichier = new wxMenu;
	m_quit = new wxMenuItem(m_fichier, wxID_EXIT, wxT("&Quit"));
	m_fichier->Append(m_quit);
	m_menubar->Append(m_fichier, wxT("&Fichier"));
	SetMenuBar(m_menubar);
// ------------------------------------------------------------
	m_toolbar = new wxToolBar(this, wxID_ANY);
	m_toolbar->SetBackgroundColour(*wxBLACK);
	wxBitmap next(wxT("next.png"), wxBITMAP_TYPE_PNG);
	m_toolbar->AddTool(ID_NEXT, wxT("Next"), next, wxT("Next Step"));
	number_points = new wxSpinCtrl(m_toolbar, wxID_ANY, wxT("25"));
	m_toolbar->AddControl(number_points);
	m_toolbar->Realize();

	gen_rand_points();
	get_envelope();

	SetBackgroundColour(*wxWHITE);
	Centre();
}

/*bool Hull_Convex::Graham_Sort::operator()(const Point& a, const Point& b)
{
	double angle1 = angle(pivot, a, Point(pivot.x, 0));
	double angle2 = angle(pivot, b, Point(pivot.x, 0));
	return angle1 < angle2;
}*/

double Hull_Convex::cross_product(const Point& a, const Point& b, const Point& c)
{
	return (b.x - a.x)*(c.y - a.y) - (c.x - a.x)*(b.y - a.y);
}

/*vector<Point> Hull_Convex::get_most_of_next(Hull_Convex::Side s)
{
	vector<Point> res;
	Line p(envelope[0], rand_points[rand_points.size() - 1]);
	for (int v = 0; v < rand_points.size(); ++v) {
		for (int i = 1; i < rand_points.size(); ++i) {
			if ((s == Side::left && cross_product(rand_points[i], p.a, p.b) <= 0) || 
				(s == Side::right && cross_product(rand_points[i], p.a, p.b) > 0))
				p.b = rand_points[i];
		}

		res.push_back(p.b);

		p.a = res[res.size() - 1];
		p.b = rand_points[rand_points.size() - 1];
	}

	return res;
}*/

PVV Hull_Convex::get_most_of_next()
{
	vector<Point> res_left, res_right;
	Line p_left(envelope[0], rand_points[rand_points.size() - 1]);
	Line p_right(envelope[0], rand_points[rand_points.size() - 1]);
	for (int v = 0; v < rand_points.size(); ++v) {
		for (int i = 1; i < rand_points.size(); ++i) {
			if (cross_product(rand_points[i], p_left.a, p_left.b) <= 0)
				p_left.b = rand_points[i];
			if (cross_product(rand_points[i], p_right.a, p_right.b) > 0)
				p_right.b = rand_points[i];
		}

		res_left.push_back(p_left.b);
		p_left.a = res_left[res_left.size() - 1];
		p_left.b = rand_points[rand_points.size() - 1];

		res_right.push_back(p_right.b);
		p_right.a = res_right[res_right.size() - 1];
		p_right.b = rand_points[rand_points.size() - 1];
	}

	return make_pair(res_left, res_right);
}

void Hull_Convex::get_envelope()
{
	envelope.clear();
	envelope.push_back(rand_points[0]);
	PVV res = get_most_of_next();
	envelope.resize(envelope.size() + res.first.size());
	copy(res.first.begin(), res.first.end(), envelope.begin() + 1);
	envelope.resize(envelope.size() + res.second.size());
	reverse(res.second.begin(), res.second.end());
	copy(res.second.begin(), res.second.end(), envelope.begin() + (envelope.size()-res.first.size()));
}

void Hull_Convex::gen_rand_points()
{
	rand_points.clear();
	rand_points.resize(number_points->GetValue());
	for (int i = 0; i < rand_points.size(); ++i)
		rand_points[i] = Point(random(150, 700), random(150, 700));
	sort(rand_points.begin(), rand_points.end());
	/*Graham_Sort s; 
	s.pivot = rand_points[0];
	s.greatest = rand_points[rand_points.size() - 1];
	sort(rand_points.begin(), rand_points.end(), s);*/
}

void Hull_Convex::OnQuit(wxCommandEvent& event)
{
	Close(true);
}

void Hull_Convex::OnNextRandom(wxCommandEvent& event)
{
	gen_rand_points();
	get_envelope();
	Refresh();
}


void Hull_Convex::PaintBackground(wxDC& dc)
{
	wxColour backgroundColour = *wxWHITE/*GetBackgroundColour()*/;
	if (!backgroundColour.IsOk())
		backgroundColour = /*wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE)*/*wxWHITE;
	dc.SetBrush(wxBrush(backgroundColour));
	dc.SetPen(wxPen(backgroundColour, 1));
	wxRect windowRect(wxPoint(0, 0), GetClientSize());
	/*CalcUnscrolledPosition(windowRect.x, windowRect.y,
		&windowRect.x, &windowRect.y);*/
	dc.DrawRectangle(windowRect);
}

void Hull_Convex::OnPaint(wxPaintEvent& event)
{
	wxBufferedPaintDC dc(this);
	PrepareDC(dc);
	PaintBackground(dc);
	dc.SetBrush(*wxWHITE_BRUSH);
	dc.SetPen(wxPen(*wxBLUE, 4, wxPENSTYLE_SOLID));
	dc.DrawRectangle(wxPoint(0, 60), paint_zone);
	dc.SetPen(wxPen(*wxBLUE, 4, wxPENSTYLE_SOLID));
	for (int i = 0; i < rand_points.size(); ++i) {
		dc.DrawCircle(rand_points[i], 3);
		wxString x;
		x << i;
		dc.DrawText(x, rand_points[(i) % rand_points.size()]);
	}
	
	dc.SetPen(wxPen(*wxBLACK, 4, wxPENSTYLE_SOLID));
	dc.DrawCircle(envelope[0], 3);
	dc.SetPen(wxPen(*wxRED, 4, wxPENSTYLE_SOLID));
	dc.DrawCircle(envelope[1], 3);
	dc.SetPen(wxPen(*wxGREEN, 4, wxPENSTYLE_SOLID));
	dc.DrawCircle(rand_points[rand_points.size()-1], 3);
	dc.SetPen(wxPen(*wxBLACK, 4, wxPENSTYLE_SOLID));
	for (int i = 0; i < envelope.size(); ++i) {
		dc.DrawLine(envelope[i], envelope[(i + 1) % envelope.size()]);
	}

	dc.SetPen(wxNullPen);
}

class myapp : public wxApp
{
public:
	virtual bool OnInit();
};

//----------------------------------------------------------

DECLARE_APP(myapp)
IMPLEMENT_APP(myapp)

//----------------------------------------------------------

bool myapp::OnInit()
{
	srand(time(NULL));
	Hull_Convex* frame = new Hull_Convex();
	frame->Show(true);
	return true;
}
