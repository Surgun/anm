#include "..\\rows\\rows.h"
#include "..\\parser\\parser.h"
#include "..\\chart\\chart.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include "..\\trans.h"

extern string num_to_string(const numeric& x, int MaxN = 0);
extern void normalize_pts(vector <POINT3D> &pts, vector <POINT3D> &new_pts, const RECT &br);
extern void add_two_pts(const POINT3D &p1, const POINT3D &p2, const RECT &br, vector <POINT3D> &new_pts);
extern const POINT3D get_pt(const POINT3D &p1, const POINT3D &p2, const RECT &br);
extern bool bound(const POINT3D &p, const RECT &br);
extern void DrawName(HDC &hdc, unsigned long color, ANM_chart_window *prop, const string &name);
extern int to_integer(const numeric &x);
extern void SetGLColor(long lColor);


//-------------- класс результатов ---------------------------------------
result::result()
{
	bDraw = false;
}

result::result(const result &other)
{
	name = other.name;
	color = other.color;
	bSquare = other.bSquare;
	bDraw = other.bDraw;
	Rk.assign(other.Rk.begin(), other.Rk.end());
	PhiRk.assign(other.PhiRk.begin(), other.PhiRk.end());
	PhiPlusRk.assign(other.PhiPlusRk.begin(), other.PhiPlusRk.end());
	PhiMinusRk.assign(other.PhiMinusRk.begin(), other.PhiMinusRk.end());
	others.assign(other.others.begin(), other.others.end());
	other_rows.assign(other.other_rows.begin(), other.other_rows.end());
}

const result result::operator =(const result &other)
{
	name = other.name;
	bSquare = other.bSquare;
	color = other.color;
	bDraw = other.bDraw;
	Rk.assign(other.Rk.begin(), other.Rk.end());
	PhiRk.assign(other.PhiRk.begin(), other.PhiRk.end());
	PhiPlusRk.assign(other.PhiPlusRk.begin(), other.PhiPlusRk.end());
	PhiMinusRk.assign(other.PhiMinusRk.begin(), other.PhiMinusRk.end());
	others.assign(other.others.begin(), other.others.end());
	other_rows.assign(other.other_rows.begin(), other.other_rows.end());
	return *this;
}

void result::append(const result &other)
{
	Rk.insert(Rk.end(), other.Rk.begin(), other.Rk.end());
	PhiRk.insert(PhiRk.end(), other.PhiRk.begin(), other.PhiRk.end());
	PhiPlusRk.insert(PhiPlusRk.end(), other.PhiPlusRk.begin(), other.PhiPlusRk.end());
	PhiMinusRk.insert(PhiMinusRk.end(), other.PhiMinusRk.begin(), other.PhiMinusRk.end());
	//DEBUG_OUT("others.size() = "<< others.size())
	//DEBUG_OUT("other.others.size() = "<< other.others.size())
	if(others.size() == other.others.size())
	{
		for(int i = 0; i < others.size(); i++)
		{
			if(others[i].name == other.others[i].name)
				others[i].insert(others[i].end(), other.others[i].begin(), other.others[i].end());
			else
				ERROR_13
		}
	}
	else
		ERROR_14
	if(other_rows.size() == other.other_rows.size())
	{
		for(int i = 0; i < other_rows.size(); i++)
		{
			if(other_rows[i].name == other.other_rows[i].name)
				other_rows[i].append(other.other_rows[i]);
			else
				ERROR_13
		}
	}
	else
		ERROR_14
}
int result::size(){
	return int(other_rows.size()) + int(others.size()) + 1;
}

void result::SetPoints(ANM_chart_window *prop, defx_array &defx_pts, vector <result> &cur_results)
{
	if(bDraw)
    {
	    defx_array::iterator dtIter;
        dtIter = defx_pts.begin();
        values.clear();
		for(dtIter = defx_pts.begin(); dtIter != defx_pts.end(); dtIter++)
        {
            //DEBUG_OUT("value = " << Rk[dtIter->i].Value(dtIter->dx))
            //DEBUG_OUT("dx = " << dtIter->dx)
            values.push_back(Rk[dtIter->i].Value(dtIter->dx));
        }
		if(prop->bDrawDomen)
		{
		}
    }
    for(int i = 0; i < other_rows.size(); i++)
		other_rows[i].SetPoints(prop, defx_pts, cur_results);
}

void result::DrawPlots(HDC &hdc, ANM_chart_window *prop, defx_array &defx_pts)
{
    //DEBUG_OUT("result::DrawPlots name = " << name)
	int i;
	if(bDraw)
	{
		//prop->DrawState("Рисуем " + ToString(name));
		//DEBUG_OUT("bDraw = true for name: " << name)
		range rX = prop->Xranges;
		range rY = prop->Yranges;
		RECT br = prop->cRect;
		RECT clipBox;
		GetClipBox(hdc, &clipBox);
		numeric dx = (rX.get())/numeric(br.right - br.left);
		numeric dy = (rY.get())/numeric(br.bottom - br.top);
		POINT3D p, old_p;
		vector <POINT3D> sup_pts, ns_pts;	//верхняя граница
		vector <POINT3D> mid_pts, nm_pts;	//приближенное решение
		vector <POINT3D> inf_pts, ni_pts;	//нижняя граница
		vector<POINT3D>::pointer mpts;

		DrawName(hdc, color, prop, name);

		HPEN pen, pOldPen;
		pen = CreatePen(PS_SOLID, (int)prop->LineWidth, color);
		pOldPen = (HPEN)SelectObject(hdc, pen);

		SetROP2(hdc, prop->Rop2Mode);

	    numArray::iterator ptsIter;
        defx_array::iterator defxIter;
        defxIter = defx_pts.begin();
        //DEBUG_OUT("for(ptsIter = values.begin()")
		for(ptsIter = values.begin(); ptsIter != values.end(); ptsIter++)
		{
           //DEBUG_OUT("value = " << *ptsIter)
           //DEBUG_OUT("curx = " << defxIter->curx)
			p.x = to_integer((defxIter->curx - rX.rmin)/dx) + br.left;
			p.y = br.bottom - to_integer((*ptsIter - rY.rmin)/dy);
			p.z = int(prop->m_Trans);
            if(!mid_pts.size() || p.x != old_p.x || old_p.y != p.y)
			    mid_pts.push_back(p);
            defxIter++;
            old_p.x = p.x;
            old_p.y = p.y;
		}
        //DEBUG_OUT("normalize_pts")
		if(!prop->bOpenGL)
	    	normalize_pts(mid_pts, nm_pts, clipBox);

		if(prop->bDrawDomen)
		{
			LOGBRUSH logBrush;
			logBrush.lbColor = color;
			logBrush.lbStyle = BS_SOLID;
			HBRUSH brush;
			brush = CreateBrushIndirect(&logBrush);

		    numeric preGlobD;
		    numeric GlobD;
            defxIter = defx_pts.begin();
            POINT3D old_sup_p, old_inf_p;
		    for(ptsIter = values.begin(); ptsIter != values.end(); ptsIter++)
		    {
                preGlobD = Rk[defxIter->i].preGlobalD;
        		GlobD = preGlobD + (Rk[defxIter->i].GlobalD - preGlobD)*defxIter->dx_div_step;
			    p.x = to_integer((defxIter->curx - rX.rmin)/dx) + br.left;
			    p.y = br.bottom - to_integer(((*ptsIter) + GlobD - rY.rmin)/dy);
				p.z = int(prop->m_Trans);
                if(!sup_pts.size() || p.x != old_sup_p.x || p.y != old_sup_p.y)
    		        sup_pts.push_back(p);
                old_sup_p.x = p.x;
                old_sup_p.y = p.y;
			    p.y = br.bottom - to_integer(((*ptsIter) - GlobD - rY.rmin)/dy);
                if(!inf_pts.size() || p.x != old_inf_p.x || p.y != old_inf_p.y)
			        inf_pts.push_back(p);
                defxIter++;
                old_inf_p.x = p.x;
                old_inf_p.y = p.y;
		    }
			if(!prop->bOpenGL)
			{
				sup_pts.insert(sup_pts.end(), inf_pts.rbegin(), inf_pts.rend());
				normalize_pts(sup_pts, ns_pts, clipBox);
			}
			if(ns_pts.size()||(prop->bOpenGL && sup_pts.size()))
            {
				if(prop->bOpenGL)
				{
					vector<POINT3D>::iterator supIter, infIter;
					int poly[sup_pts.size()*2][3];
					int k = 0;
					infIter = inf_pts.begin();
				    for(supIter = sup_pts.begin(); supIter != sup_pts.end(); supIter++)
					{
						poly[k][0] = infIter->x;
						poly[k][1] = clipBox.bottom - infIter->y;
						poly[k][2] = infIter->z;
						k++;
						poly[k][0] = supIter->x;
						poly[k][1] = clipBox.bottom - supIter->y;
						poly[k][2] = supIter->z;
						k++;
						infIter++;
					}
					glNewList(prop->glListInd++, GL_COMPILE);
					SetGLColor(color);
					glVertexPointer(3,GL_INT,0,poly);
					glEnableClientState(GL_VERTEX_ARRAY);
					glDrawArrays(GL_QUAD_STRIP,0, sup_pts.size()*2);
					glDisableClientState(GL_VERTEX_ARRAY);
					glEndList();
				}
				else
				{
					//vector<POINT3D>::pointer pts = &ns_pts[0];
					//HRGN rgn = CreatePolygonRgn(pts, ns_pts.size(), ALTERNATE);
					//FillRgn(hdc, rgn, brush);
    				//DeleteObject(rgn);
				}
            }
			DeleteObject(brush);
		}
        //DEBUG_OUT("if(nm_pts.size())")
		if(prop->bOpenGL && mid_pts.size())
		{
			vector<POINT3D>::iterator midIter;
			int poly[mid_pts.size()][3];
			int k = 0;
			for(midIter = mid_pts.begin(); midIter != mid_pts.end(); midIter++)
			{
				poly[k][0] = midIter->x;
				poly[k][1] = clipBox.bottom - midIter->y;
				poly[k][2] = midIter->x;
				k++;
			}
			prop->m_Trans = prop->m_Trans + 50;
			glNewList(prop->glListInd++, GL_COMPILE);
			SetGLColor(color);
			glVertexPointer(3,GL_INT,0,poly);
			glEnableClientState(GL_VERTEX_ARRAY);
			glDrawArrays(GL_LINE_STRIP,0, mid_pts.size());
			glDisableClientState(GL_VERTEX_ARRAY);
			glEndList();
		}
        if(nm_pts.size())
        {
		    mpts = &nm_pts[0];
		    //Polyline(hdc, mpts, nm_pts.size());
        }
		SetROP2(hdc, R2_COPYPEN);
		DeleteObject(pen);
		SelectObject(hdc, pOldPen);
        //DEBUG_OUT("DeleteObject(pen);")
    }
    //DEBUG_OUT("for(i = 0; i < others.size(); i++)")
    for(i = 0; i < others.size(); i++)
		others[i].DrawPlot(hdc, prop, defx_pts);
    for(i = 0; i < other_rows.size(); i++)
		other_rows[i].DrawPlots(hdc, prop, defx_pts);
    //DEBUG_OUT("others.size() = " << others.size())
    //DEBUG_OUT("end - DrawPlots " << bDraw << " " << bkColor << " " << color << " " << name);
}

void result::DrawPhases(HDC &hdc, ANM_chart_window *prop, result *phase, defx_array &defx_pts)
{
    //DEBUG_OUT("result::DrawPhases")
	if(bDraw /*&& this != phase*/ && phase)
	{
		range rX = prop->Xranges;
		range rY = prop->Yranges;
		RECT br = prop->cRect;
		RECT clipBox;
		GetClipBox(hdc, &clipBox);
		numeric dx = (rX.get())/numeric(br.right - br.left);
		numeric dy = (rY.get())/numeric(br.bottom - br.top);
		POINT3D p;
		vector <POINT3D> sup_pts;	//верхняя граница
		vector <POINT3D> mid_pts, nm_pts;	//приближенное решение
		vector <POINT3D> inf_pts;	//нижняя граница
		vector<POINT3D>::pointer mpts;

		HPEN pen, pOldPen;
		pen = CreatePen(PS_SOLID, (int)prop->LineWidth, color);
		pOldPen = (HPEN)SelectObject(hdc, pen);

		SetROP2(hdc, prop->Rop2Mode);

	    numArray::iterator ptsIter;
        numArray::iterator phaseIter;
        //DEBUG_OUT("phaseIter = phase->values.begin();")
        phaseIter = phase->values.begin();
        //DEBUG_OUT("after phaseIter = phase->values.begin();")
		for(ptsIter = values.begin(); ptsIter != values.end(); ptsIter++)
		{
            //DEBUG_OUT("p.x = to_integer")
			p.x = to_integer((*phaseIter - rX.rmin)/dx) + br.left;
			p.y = br.bottom - to_integer((*ptsIter - rY.rmin)/dy);
			p.z = int(prop->m_Trans);
			mid_pts.push_back(p);
            phaseIter++;
		}
        //DEBUG_OUT("mpts = &mid_pts[0];")
		if(!prop->bOpenGL)
	    	normalize_pts(mid_pts, nm_pts, clipBox);
		if(nm_pts.size()||(prop->bOpenGL && mid_pts.size()))
		{
			if(prop->bOpenGL)
			{
				vector<POINT3D>::iterator pIter;
				int poly[mid_pts.size()][3];
				int k = 0;
				for(pIter = mid_pts.begin(); pIter != mid_pts.end(); pIter++)
				{
					poly[k][0] = pIter->x;
					poly[k][1] = clipBox.bottom - pIter->y;
					poly[k][2] = pIter->z;
					k++;
				}
				glNewList(prop->glListInd++, GL_COMPILE);
				SetGLColor(color);
				glVertexPointer(3,GL_INT,0,poly);
				glEnableClientState(GL_VERTEX_ARRAY);
				glDrawArrays(GL_LINE_STRIP,0, mid_pts.size());
				glDisableClientState(GL_VERTEX_ARRAY);
				glEndList();
			}
			else
			{
				vector<POINT3D>::pointer mpts = &nm_pts[0];
				//DEBUG_OUT("Polyline");
				//Polyline(hdc, mpts, nm_pts.size());
			}
		}
    }
    //DEBUG_OUT("end - result::DrawPhases")

    for(int i = 0; i < others.size(); i++)
		others[i].DrawPhase(hdc, prop, defx_pts, phase);
    for(int i = 0; i < other_rows.size(); i++)
		other_rows[i].DrawPhases(hdc, prop, phase, defx_pts);
}

void result::SaveToCHB(ofstream &os)
{
    //DEBUG_OUT("Save " << bDraw << " " << bkColor << " " << color << " " << name << "\x0D\x0A";
	os << " " << bDraw;
	os << " " << bkColor;
	os << " " << color;
	os << " " << name;
	os << " " << others.size();
	for(int i = 0; i < others.size(); i++)
		others[i].SaveToCHB(os);
	os << " " << Rk.size();
	for(int i = 0; i < Rk.size(); i++)
		Rk[i].SaveToCHB(os);
	os << " " << PhiRk.size();
	for(int i = 0; i < PhiRk.size(); i++)
		PhiRk[i].SaveToCHB(os);
	os << " " << PhiPlusRk.size();
	for(int i = 0; i < PhiPlusRk.size(); i++)
		PhiPlusRk[i].SaveToCHB(os);
	os << " " << PhiMinusRk.size();
	for(int i = 0; i < PhiMinusRk.size(); i++)
		PhiMinusRk[i].SaveToCHB(os);
}

const numeric result::GetGlobD(int i, const numeric &dx_div_step)
{
	numeric CurGlobD = Rk[i].preGlobalD + (Rk[i].GlobalD - Rk[i].preGlobalD)*dx_div_step;
	if(Rk[i].inversed)
	{
		numeric CurVal = numeric(1)/Rk[i].Value(0);
		numeric tmpGlob1 = CurGlobD/(CurVal*(CurVal+CurGlobD));
		numeric tmpGlob2 = CurGlobD/(CurVal*(CurVal-CurGlobD));
		//DEBUG_OUT("inversed GlobD = " << CurGlobD)
		//DEBUG_OUT("inversed Value = " << Rk[i].Value(0))
		return max(tmpGlob1, tmpGlob2);
	}
    return CurGlobD;
}

const string result::GetStrSquare(const range &rX, const time_array &dTime, const numArray &step)
{
	string res;
	if(!bSquare || !bDraw || dTime.Begin > dTime.End || !dTime.size() || dTime.End == 0)
		return "";
	int i = dTime.Begin;
	numeric curS;
	numeric startTime, endTime;
	//DEBUG_OUT("dTime.Begin = " << dTime.Begin << "; dTime.End = " << dTime.End)
	//DEBUG_OUT("step[dTime.Begin] = " << step[dTime.Begin])
	//DEBUG_OUT("dTime[dTime.Begin] = " << dTime[dTime.Begin])
// Расчет площади области, содержащей точное решение (min)
	if(rX.rmin >= dTime[dTime.Begin])
		startTime = rX.rmin;
	else
		startTime = dTime[dTime.Begin];
	if(rX.rmax <= dTime[dTime.End] + step[dTime.End])
		endTime = rX.rmax;
	else
		endTime = dTime[dTime.End] + step[dTime.End];
	if(endTime <= dTime[i] + step[i])
		curS = Rk[i].DomenSquareMin(startTime - dTime[i], endTime - dTime[i]);
	else
		curS = Rk[i].DomenSquareMin(startTime - dTime[i], step[i]);
	for(i = int(dTime.Begin) + 1; i < int(dTime.End) - 1; i++)
		curS += Rk[i].DomenSquareMin(0, step[i]);
	if(int(dTime.End) - 1 > int(dTime.Begin))
		curS += Rk[i].DomenSquareMin(0, endTime - dTime[i]);
	res = "Squares of (Площади для) " + ToString(name) + ": domen_min (мин. область) = " + ToString(curS);
// Расчет площади области, содержащей точное решение (max)
	if(rX.rmin >= dTime[dTime.Begin])
		startTime = rX.rmin;
	else
		startTime = dTime[dTime.Begin];
	if(rX.rmax <= dTime[dTime.End] + step[dTime.End])
		endTime = rX.rmax;
	else
		endTime = dTime[dTime.End] + step[dTime.End];
	if(endTime <= dTime[i] + step[i])
		curS = Rk[i].DomenSquareMax(startTime - dTime[i], endTime - dTime[i]);
	else
		curS = Rk[i].DomenSquareMax(startTime - dTime[i], step[i]);
	for(i = int(dTime.Begin) + 1; i < int(dTime.End) - 1; i++)
		curS += Rk[i].DomenSquareMax(0, step[i]);
	if(int(dTime.End) - 1 > int(dTime.Begin))
		curS += Rk[i].DomenSquareMax(0, endTime - dTime[i]);
	res = res + "; domen_max (макс. область) = " + ToString(curS);
// Расчет площади под приближенным решением (неправильно считается если в интервале расчета есть пересечение с осью абсцисс)
	i = dTime.Begin;
	if(endTime <= dTime[i] + step[i])
		curS = Rk[i].SolutionSquare(startTime - dTime[i], endTime - dTime[i]);
	else
		curS = Rk[i].SolutionSquare(startTime - dTime[i], step[i]);
	for(i = int(dTime.Begin) + 1; i < int(dTime.End) - 1; i++)
		curS += Rk[i].SolutionSquare(0, step[i]);
	if(int(dTime.End) - 1 > int(dTime.Begin))
		curS += Rk[i].SolutionSquare(0, endTime - dTime[i]);
	res = res + "; sol (приближенное решение) = " + ToString(curS);
// Расчет площади под (приближенным решением)^2
	i = dTime.Begin;
	if(endTime <= dTime[i] + step[i])
		curS = Rk[i].Solution_x2_Square(startTime - dTime[i], endTime - dTime[i]);
	else
		curS = Rk[i].Solution_x2_Square(startTime - dTime[i], step[i]);
	for(i = int(dTime.Begin) + 1; i < int(dTime.End) - 1; i++)
		curS += Rk[i].Solution_x2_Square(0, step[i]);
	if(int(dTime.End) - 1 > int(dTime.Begin))
		curS += Rk[i].Solution_x2_Square(0, endTime - dTime[i]);
	res = res + "; sol^2 (решение^2) = " + ToString(curS);
	return (res + "\x0D\x0A");
}

const range result::GetValueRange(const range &rX, const time_array &dTime, const numArray &step, const RECT &br, ANM_chart_window *prop, vector <result> &cur_results)
{
	range ret;
	if(bDraw)
	{
		numeric dx = (rX.get())/numeric(br.right - br.left);
		numeric cury;
		numeric x;

		int i = dTime.Begin;
		cury = Rk[i].Value(0);
		ret.rmax = ret.rmin = cury;
		for(int i = dTime.Begin; i <= dTime.End; i++)
		{
			for(x = 0; x < abs(step[i]); x += dx*prop->dt)
			{
    			cury = Rk[i].Value((step[i] < 0)?-x:x);
				ret.rmax = max(ret.rmax, cury + ((prop->bDrawDomen)?(Rk[i].GlobalD):(0)));
				ret.rmin = min(ret.rmin, cury - ((prop->bDrawDomen)?(Rk[i].GlobalD):(0)));
			}
			cury = Rk[i].Value((step[i]));
			ret.rmax = max(ret.rmax, cury + ((prop->bDrawDomen)?(Rk[i].GlobalD):(0)));
			ret.rmin = min(ret.rmin, cury - ((prop->bDrawDomen)?(Rk[i].GlobalD):(0)));
		}
	}
	for(int i = 0; i < others.size(); i++)
		if(others[i].bDraw)
			ret = ret.max_range(others[i].GetValueRange(rX, dTime, step));
	for(int i = 0; i < other_rows.size(); i++)
		if(other_rows[i].bDraw)
			ret = ret.max_range(other_rows[i].GetValueRange(rX, dTime, step, br, prop, cur_results));
	return ret;
}

void result::LoadFromCHB(ifstream &is)
{
	string tmpStr;
	is >> bDraw;
	is >> bkColor;
	is >> color;
	is >> name;
	size_t sz;
	is >> sz;
	others.clear();
	//DEBUG_OUT("others.clear();")
	for(int i = 0; i < sz; i++)
		others.push_back(sub_res());
	for(int i = 0; i < sz; i++)
		others[i].LoadFromCHB(is);
	//DEBUG_OUT("for(int i = 0; i < sz; i++)")
	is >> sz;
	Rk.clear();
	for(int i = 0; i < sz; i++)
		Rk.push_back(taylor_spline_element());
	for(int i = 0; i < sz; i++)
		Rk[i].LoadFromCHB(is);
	is >> sz;
	PhiRk.clear();
	for(int i = 0; i < sz; i++)
		PhiRk.push_back(taylor_spline_element());
	for(int i = 0; i < sz; i++)
		PhiRk[i].LoadFromCHB(is);
	is >> sz;
	PhiPlusRk.clear();
	for(int i = 0; i < sz; i++)
		PhiPlusRk.push_back(taylor_spline_element());
	for(int i = 0; i < sz; i++)
		PhiPlusRk[i].LoadFromCHB(is);
	is >> sz;
	PhiMinusRk.clear();
	for(int i = 0; i < sz; i++)
		PhiMinusRk.push_back(taylor_spline_element());
	for(int i = 0; i < sz; i++)
		PhiMinusRk[i].LoadFromCHB(is);
}

//----------------------------------------------- row_res class -----------------------------------------
GINAC_IMPLEMENT_REGISTERED_CLASS(row_res, basic)

row_res::row_res() : result()
{
	bDraw = false;
}

row_res::row_res(const ex &aStat, const ex& aEx_stat, int aMode, const ex& aPrintName)
{
	stat = aStat;
	ex_stat = aEx_stat;
	ex_mode = aMode;
	if(aPrintName.is_equal(0))
	{
		switch(ex_mode){
			case 1: // режим вывода игреков
				name = ToString(stat/ex_stat);
				break;
			case 2: // режим вывода корней (при этом aPrintName не должно быть = 0)
				name = ToString(stat);
				break;
		}
	}
	else
		name = ToString(aPrintName);
	bDraw = false;
}

row_res::row_res(const row_res &a_other):result(a_other)
{
	stat = a_other.stat;
	ex_stat = a_other.ex_stat;
	ex_mode = a_other.ex_mode;
	printName = a_other.printName;
	yRk.assign(a_other.yRk.begin(), a_other.yRk.end());
}

const row_res row_res::operator = (const row_res &other)
{
	result::operator =(other);
	stat = other.stat;
	ex_stat = other.ex_stat;
	ex_mode = other.ex_mode;
	printName = other.printName;
	yRk.assign(other.yRk.begin(), other.yRk.end());
	return *this;
}

void row_res::archive(archive_node &n) const
{
	inherited::archive(n);
	n.add_ex("ex", stat);
}

row_res::row_res(const archive_node &n, lst &sym_lst) : inherited(n, sym_lst)
{
}

ex row_res::unarchive(const archive_node &n, lst &sym_lst)
{
	return (new row_res(n, sym_lst))->setflag(status_flags::dynallocated);
}

int row_res::compare_same_type(const basic &other) const
{
	const row_res &o = static_cast<const row_res &>(other);
	return stat.compare(o.stat);
}

void row_res::print(const print_context &c, unsigned level) const
{
	// print_context::s is a reference to an ostream
	if(ToString(name)==ToString(stat))
		c.s << "row(" << stat << ")";
	else
		c.s << (name);
}

const range row_res::GetValueRange(const range &rX, const time_array &dTime, const numArray &step, const RECT &br, ANM_chart_window *prop, vector <result> &cur_results)
{
	if(ex_mode > 2)
		return result::GetValueRange(rX, dTime, step, br, prop, cur_results);
	range ret;
	if(bDraw)
	{
		numeric dx = (rX.get())/numeric(br.right - br.left);
		numeric cury;
		numeric x;

		int i = dTime.Begin;
		if(ex_mode == 2)
		{
			exmap XtoValues;
			for(int ii = 0; ii < ex_stat.nops(); ii++)
				XtoValues.insert(make_pair(ex_stat.op(ii), cur_results[ii].Rk[i].Value(0)));
			if(!is_a<numeric>(stat.subs(XtoValues).evalf()))
				cury = 0;
			else
				cury = ex_to<numeric>(stat.subs(XtoValues).evalf());
		}
		else
		{
			numeric val = yRk[i].Value(0);
			if(val == 0)
				cury = 0;
			else
				cury = (Rk[i].Value(0)/val);
		}
		ret.rmax = ret.rmin = cury;
		for(int i = dTime.Begin; i <= dTime.End; i++)
		{
			for(x = 0; x < abs(step[i]); x += dx*prop->dt)
			{
				if(ex_mode == 2)
				{
					exmap XtoValues;
					for(int ii = 0; ii < ex_stat.nops(); ii++)
						XtoValues.insert(make_pair(ex_stat.op(ii), cur_results[ii].Rk[i].Value((step[i] < 0)?-x:x)));
					if(!is_a<numeric>(stat.subs(XtoValues).evalf()))
						cury = 0;
					else
						cury = ex_to<numeric>(stat.subs(XtoValues).evalf());
				}
				else
				{
					numeric val = yRk[i].Value((step[i] < 0)?-x:x);
					if(val == 0)
						cury = 0;
					else
						cury = (Rk[i].Value((step[i] < 0)?-x:x)/val);
				}
				ret.rmax = max(ret.rmax, cury);
				ret.rmin = min(ret.rmin, cury);
			}
			if(ex_mode == 2)
			{
				exmap XtoValues;
				for(int ii = 0; ii < ex_stat.nops(); ii++)
					XtoValues.insert(make_pair(ex_stat.op(ii), cur_results[ii].Rk[i].Value(step[i])));
				if(!is_a<numeric>(stat.subs(XtoValues).evalf()))
					cury = 0;
				else
					cury = ex_to<numeric>(stat.subs(XtoValues).evalf());
			}
			else
			{
				numeric val = yRk[i].Value(step[i]);
				if(val == 0)
					cury = 0;
				else
					cury = (Rk[i].Value(step[i])/val);
			}
			ret.rmax = max(ret.rmax, cury);
			ret.rmin = min(ret.rmin, cury);
		}
	}
	return ret;
}

ex row_res::evalf(int level) const
{
	return stat.evalf(level);
}

void row_res::append(const row_res &other)
{
	//DEBUG_OUT("other.yRk.size = " << other.yRk.size())
	yRk.insert(yRk.end(), other.yRk.begin(), other.yRk.end());
	result::append(other);
}

void row_res::SetPoints(ANM_chart_window *prop, defx_array &defx_pts, vector <result> &cur_results)
{
	if(ex_mode > 2)
	{
		result::SetPoints(prop, defx_pts, cur_results);
		return;
	}
	if(bDraw)
    {
	    defx_array::iterator dtIter;
        dtIter = defx_pts.begin();
        values.clear();
		for(dtIter = defx_pts.begin(); dtIter != defx_pts.end(); dtIter++)
		{
			//DEBUG_OUT("Rk.size() = " << Rk.size())
			//DEBUG_OUT("yRk.size() = " << yRk.size())
			//DEBUG_OUT("dtIter->i = " << dtIter->i)
			if(ex_mode == 2)
			{
				exmap XtoValues;
				for(int i = 0; i < ex_stat.nops(); i++)
					XtoValues.insert(make_pair(ex_stat.op(i), cur_results[i].Rk[dtIter->i].Value(dtIter->dx)));
				//DEBUG_OUT("stat.subs(XtoValues) = " << stat.subs(XtoValues))
				if(!is_a<numeric>(stat.subs(XtoValues).evalf()))
					values.push_back(0);
				else
					values.push_back(ex_to<numeric>(stat.subs(XtoValues).evalf()));
			}
			else
			{
				numeric val = yRk[dtIter->i].Value(dtIter->dx);
				if(val == 0)
					values.push_back(0);
				else
					values.push_back(Rk[dtIter->i].Value(dtIter->dx)/val);
			}
		}
    }
}

//-----------------------------------------------------------------------------------------
void time_array::set_bounds(const range &rT)
{
    if( size() && (*this)[0] < 0)
	{
		Begin = 0;
		End = size()-1;
		return;
	}
	if(!size() || front() > rT.rmax)
	{
		Begin = 1;
		End = 0;
		return;
	}
	Begin = 0;
	while(Begin < size() && (*this)[Begin] < rT.rmin)
	{
		if((*this)[Begin] < 0)
		{
			Begin = 0;
			End = size()-1;
			return;
		}
		Begin++;
	}
	if(Begin < size())
	{
		if(Begin)
			Begin--;
		End = Begin+1;
		while(End < size() && (*this)[End] < rT.rmax)
		{
			if((*this)[End] < 0)
			{
				Begin = 0;
				End = size()-1;
				return;
			}
			End++;
		}
		if(End >= size())
			End--;
	}
	else
	{
		if((*this)[Begin-1]+last_step >= rT.rmin)
		{
			Begin--;
			End = Begin;
		}
		else
		{
			Begin = 1;
			End = 0;
		}
    //DEBUG_OUT(Begin << " " << End << " " << last_step << "\x0D\x0A";
	}
}

//---------------------------------------------------------------------------
//--------------- myprocedure class-----------------------
GINAC_IMPLEMENT_REGISTERED_CLASS(taylor_spline_element, basic)
taylor_spline_element::taylor_spline_element()
{
	inversed = false;
}

taylor_spline_element::taylor_spline_element(const archive_node &n, lst &sym_lst) : inherited(n, sym_lst)
{
}

void taylor_spline_element::archive(archive_node &n) const
{
	inherited::archive(n);
}

ex taylor_spline_element::unarchive(const archive_node &n, lst &sym_lst)
{
	return (new taylor_spline_element(n, sym_lst))->setflag(status_flags::dynallocated);
}

int taylor_spline_element::compare_same_type(const basic &other) const
{
//	const taylor_spline_element &o = static_cast<const taylor_spline_element &>(other);
	return 0;
}

taylor_spline_element::taylor_spline_element(numArray &aRk, numeric aGlobalD, numeric aPreGlobalD, bool aInv)
{
	Rk.assign(aRk.begin(), aRk.end());
	GlobalD = aGlobalD;
	preGlobalD = aPreGlobalD;
	inversed = aInv;
}
taylor_spline_element::taylor_spline_element(const taylor_spline_element& other)
{
	Rk.assign(other.Rk.begin(), other.Rk.end());
	GlobalD = other.GlobalD;
	preGlobalD = other.preGlobalD;
	inversed = other.inversed;
}

taylor_spline_element taylor_spline_element::operator=(const taylor_spline_element& other)
{
	Rk.assign(other.Rk.begin(), other.Rk.end());
	GlobalD = other.GlobalD;
	preGlobalD = other.preGlobalD;
	inversed = other.inversed;
	return *this;
}

ex taylor_spline_element::coeff(const ex & s, int n) const
{
	if(s.is_equal(s_taylor))
	{
		if(n < 0 || n >= Rk.size())
			return 0;
		return Rk[n];
	}
	else if(s.is_equal(s_p))
		return coeff(s_taylor, -(n+1));
}

void taylor_spline_element::SaveToCHB(ofstream &os)
{
	os << " " << GlobalD;
	os << " " << preGlobalD;
	os << " " << Rk.size();
	for(int i = 0; i < Rk.size(); i++)
		os << " " << Rk[i];

}

void taylor_spline_element::LoadFromCHB(ifstream &is)
{
	string tmpStr;
	is >> tmpStr;
	GlobalD = numeric(tmpStr.c_str());
	is >> tmpStr;
	preGlobalD = numeric(tmpStr.c_str());
	size_t sz;
	is >> sz;
	Rk.clear();
	for(int i = 0; i < sz; i++)
	{
		is >> tmpStr;
		Rk.push_back(numeric(tmpStr.c_str()));
	}
}

numeric taylor_spline_element::Value(const numeric &at)
{
	if(Rk.size()==0)
		return 0;
	numeric x = 0;
	if(!at.is_equal(0))
		for(int i = Rk.size() - 1; i > 0; i--)
		{
			x += Rk[i];
			x *= at/numeric(i);
		}
	x += Rk[0];
	if(inversed && !x.is_equal(0))
		return numeric(1)/x;
	return x;
}

const numeric taylor_spline_element::DomenSquareMin(const numeric &t0, const numeric &t1)
{
	numeric resS = 2*preGlobalD*(t1 - t0);
	return resS;
}

const numeric taylor_spline_element::DomenSquareMax(const numeric &t0, const numeric &t1)
{
	numeric resS = 2*GlobalD*(t1 - t0);
	return resS;
}

const numeric taylor_spline_element::SolutionSquare(const numeric &t0, const numeric &t1)
{
	numeric x = 0, curt0, curt1, fact;
	curt0 = t0; curt1 = t1; fact = 1;
	for(int i = 0; i < Rk.size(); i++)
	{
		fact = fact*(i+1);
		x += (curt1-curt0)*Rk[i]/numeric(fact);
		//DEBUG_OUT("x.R[" << i << "] = " << curt1 - curt0 << "*" << Rk[i] << "/" << fact << " = " << (curt1-curt0)*Rk[i]/numeric(fact))
		//DEBUG_OUT("curt0 = " << curt0 << "; " << "curt1 = " << curt1)
		curt0 = curt0*t0;
		curt1 = curt1*t1;
	}
	return abs(numeric(x));
}

const numeric taylor_spline_element::Solution_x2_Square(const numeric &t0, const numeric &t1)
{
	numeric x = 0, curt0, curt1, fact;
	x2 = row_power(*this, 2);
	curt0 = t0; curt1 = t1; fact = 1;
	for(int i = 0; i < Rk.size(); i++)
	{
		fact = fact*(i+1);
		x += (curt1-curt0)*ex_to<numeric>(x2.coeff(s_taylor, i)/numeric(fact).evalf());
		//x += (curt1-curt0)*Rk[i]/numeric(fact);
		//DEBUG_OUT("x.R[" << i << "] = " << curt1 - curt0 << "*" << Rk[i] << "/" << fact << " = " << (curt1-curt0)*Rk[i]/numeric(fact))
		//DEBUG_OUT("curt0 = " << curt0 << "; " << "curt1 = " << curt1)
		curt0 = curt0*t0;
		curt1 = curt1*t1;
	}
	return abs(numeric(x));
}


