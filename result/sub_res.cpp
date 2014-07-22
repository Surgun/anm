#include "..\\rows\\rows.h"
#include "..\\parser\\parser.h"
#include "..\\chart\\chart.h"
#include <GL/gl.h>


extern void normalize_pts(vector <POINT3D> &pts, vector <POINT3D> &new_pts, const RECT &br);
extern void add_two_pts(const POINT3D &p1, const POINT3D &p2, const RECT &br, vector <POINT3D> &new_pts);
extern const POINT3D get_pt(const POINT3D &p1, const POINT3D &p2, const RECT &br);
extern bool bound(const POINT3D &p, const RECT &br);
extern void DrawName(HDC &hdc, unsigned long color, ANM_chart_window *prop, const string &name);
extern void SetGLColor(long lColor);

//-----------------------------------------------------------------------------------------
const range sub_res::GetValueRange(const range &ParamsRange, const time_array &dTime, const numArray &step)
{
	range ret;
	if(!bDraw)
		return range(1,-1);
	for(int i = dTime.Begin; i <= dTime.End; i++)
	{
        if(i == dTime.Begin)
		    ret.rmin = ret.rmax = (*this)[i];
        else
        {
		    ret.rmax = max(ret.rmax, (*this)[i]);
		    ret.rmin = min(ret.rmin, (*this)[i]);
        }
	}
	return ret;
}

void sub_res::DrawPlot(HDC &hdc, ANM_chart_window *prop, defx_array &defx_pts)
{
	if(!bDraw)
		return;
	range rX = prop->Xranges;
	range rY = prop->Yranges;
	RECT br = prop->cRect;
	RECT clipBox;
	GetClipBox(hdc, &clipBox);
	numeric dx = (rX.get())/numeric(br.right - br.left);
	numeric dy = (rY.get())/numeric(br.bottom - br.top);
	numeric curx;
	numeric cury;
//	numeric x;

	HPEN pen, pOldPen;
	pen = CreatePen(PS_SOLID, (int)prop->LineWidth, color);
	pOldPen = (HPEN)SelectObject(hdc, pen);

	SetROP2(hdc, prop->Rop2Mode);

	DrawName(hdc, color, prop, name);

	POINT3D p;
	vector <POINT3D> pts;
    defx_array::iterator defxIter;
    int i, pre_i = -1;
	for(defxIter = defx_pts.begin(); defxIter != defx_pts.end(); defxIter++)
	{
        i = defxIter->i;
        if(i == pre_i)
            continue;
        pre_i = i;
		cury = (*this)[i];
		curx = defxIter->curx;
		p.x = (int)((curx - rX.rmin)/dx).to_double() + br.left;
		p.y = br.bottom - (int)((cury - rY.rmin)/dy).to_double();
		p.z = int(prop->m_Trans);
		if(prop->bOpenGL || prop->OthersMode >= 1)
			pts.push_back(p); // Линии
		else if(prop->OthersMode == 0 || prop->OthersMode == 2)	// Окружности
			Ellipse(hdc, p.x-1, p.y+1, p.x+1, p.y-1);
	}
	if(prop->bOpenGL)
	{
		vector<POINT3D>::iterator pIter;
		int poly[pts.size()][3];
		int k = 0;
		for(pIter = pts.begin(); pIter != pts.end(); pIter++)
		{
			poly[k][0] = pIter->x;
			poly[k][1] = clipBox.bottom - pIter->y;
			poly[k][2] = pIter->z;
			k++;
		}
		prop->m_Trans += 50;
		glNewList(prop->glListInd++, GL_COMPILE);
		glPointSize(3);
		SetGLColor(color);
		glVertexPointer(3,GL_INT,0,poly);
		glEnableClientState(GL_VERTEX_ARRAY);
		if(prop->OthersMode >= 1)
			glDrawArrays(GL_LINE_STRIP,0, pts.size());
		if(prop->OthersMode == 0 || prop->OthersMode == 2)
			glDrawArrays(GL_POINTS,0, pts.size());
		glDisableClientState(GL_VERTEX_ARRAY);
		glEndList();
	}
	else if(prop->OthersMode >= 1)	// Линии
	{
    	vector <POINT3D> n_pts;
		normalize_pts(pts, n_pts, clipBox);
        if(n_pts.size())
        {
		    vector<POINT3D>::pointer mpts = &n_pts[0];
		    //Polyline(hdc, mpts, n_pts.size());
        }
	}
	SetROP2(hdc, R2_COPYPEN);
	SelectObject(hdc, pOldPen);
	DeleteObject(pen);
}

void sub_res::DrawPhase(HDC &hdc, ANM_chart_window *prop, defx_array &defx_pts, result *phase)
{
	if(!bDraw)
		return;
	range rX = prop->Xranges;
	range rY = prop->Yranges;
	RECT br = prop->cRect;
	RECT clipBox;
	GetClipBox(hdc, &clipBox);
	numeric dx = (rX.get())/numeric(br.right - br.left);
	numeric dy = (rY.get())/numeric(br.bottom - br.top);
	numeric curx;
	numeric cury;
//	numeric x;

	HPEN pen, pOldPen;
	pen = CreatePen(PS_SOLID, (int)prop->LineWidth, color);
	pOldPen = (HPEN)SelectObject(hdc, pen);

	SetROP2(hdc, prop->Rop2Mode);

	DrawName(hdc, color, prop, name);

	POINT3D p;
	vector <POINT3D> pts;
    defx_array::iterator defxIter;
    numArray::iterator valIter;
    int i, pre_i = -1;
    valIter = phase->values.begin();
	for(defxIter = defx_pts.begin(); defxIter != defx_pts.end(); defxIter++)
	{
        i = defxIter->i;
        if(i == pre_i)
        {
            valIter++;
            continue;
        }
        pre_i = i;
		cury = (*this)[i];
		curx = *valIter;
        valIter++;
		p.x = (int)((curx - rX.rmin)/dx).to_double() + br.left;
		p.y = br.bottom - (int)((cury - rY.rmin)/dy).to_double();
		p.z = int(prop->m_Trans);
		pts.push_back(p);
/*		if(prop->OthersMode == 0 || prop->OthersMode == 2)	// Окружности
			Ellipse(hdc, p.x-1, p.y+1, p.x+1, p.y-1);
		if(prop->OthersMode >= 1)	// Линии
			pts.push_back(p);
*/
	}
	if(prop->bOpenGL)
	{
		vector<POINT3D>::iterator pIter;
		int poly[pts.size()][3];
		int k = 0;
		for(pIter = pts.begin(); pIter != pts.end(); pIter++)
		{
			poly[k][0] = pIter->x;
			poly[k][1] = clipBox.bottom - pIter->y;
			poly[k][2] = pIter->z;
			k++;
		}
		prop->m_Trans += 50;
		glNewList(prop->glListInd++, GL_COMPILE);
		glPointSize(3);
		SetGLColor(color);
		glVertexPointer(3,GL_INT,0,poly);
		glEnableClientState(GL_VERTEX_ARRAY);
		if(prop->OthersMode >= 1)
			glDrawArrays(GL_LINE_STRIP,0, pts.size());
		if(prop->OthersMode == 0 || prop->OthersMode == 2)
			glDrawArrays(GL_POINTS,0, pts.size());
		glDisableClientState(GL_VERTEX_ARRAY);
		glEndList();
	}
	else if(prop->OthersMode >= 1)	// Линии
	{
    	vector <POINT3D> n_pts;
		normalize_pts(pts, n_pts, clipBox);
        if(n_pts.size())
        {
		    vector<POINT3D>::pointer mpts = &n_pts[0];
		    //Polyline(hdc, mpts, n_pts.size());
        }
	}

	SetROP2(hdc, R2_COPYPEN);
	SelectObject(hdc, pOldPen);
	DeleteObject(pen);
}

void sub_res::SaveToCHB(ofstream &os)
{
	os << " " << bDraw;
	os << " " << color;
	os << " " << name;
	os << " " << size();
	for(int i = 0; i < size(); i++)
		os << " " << (*this)[i];
}

void sub_res::LoadFromCHB(ifstream &is)
{
	string tmpStr;
	is >> bDraw;
	is >> color;
	is >> name;
	size_t sz;
	is >> sz;
	clear();
	for(int i = 0; i < sz; i++)
	{
		is >> tmpStr;
		push_back(numeric(tmpStr.c_str()));
	}
}
