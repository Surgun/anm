#include "math.h"
#include "..\\parser\\parser.h"
#include "chart.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include "..\\trans.h"

int ColorInd(0);
extern string num_to_string(const numeric& x, int MaxN = 0);
extern void normalize_pts(vector <POINT3D> &pts, vector <POINT3D> &new_pts, const RECT &br);
extern void add_two_pts(const POINT3D &p1, const POINT3D &p2, const RECT &br, vector <POINT3D> &new_pts);
extern const POINT3D get_pt(const POINT3D &p1, const POINT3D &p2, const RECT &br);
extern bool bound(const POINT3D &p, const RECT &br);
extern void DrawName(HDC &hdc, unsigned long color, ANM_chart_window *prop, const string &name);
extern int to_integer(const numeric &x);
extern void SetGLColor(long lColor);
extern long GetPointColor(POINT3D p, numeric dy, range rY, int bottom);

bool bIsOpenGL;
bool bLeftBtn;
int iBottom;
int iWidth;

void MoveTo3D(HDC hdc, int x, int y, int z = 0)
{
	if(bIsOpenGL)
	{
		glVertex3f(x,iBottom - y,z);
	}
	else
	{
		MoveToEx(hdc, x, y, (LPPOINT)NULL);
	}
}

void LineTo3D(HDC hdc, int x, int y, int z = 0)
{
	if(bIsOpenGL)
	{
		glVertex3f(x,iBottom - y,z);
	}
	else
	{
		LineTo(hdc, x, y);
	}
}

void TextOutGL(int x, int y, int z, const char* str)
{
	SetGLColor(0);
	glRasterPos3f(x,y,z);
	glListBase(1000);
	glCallLists(strlen(str), GL_UNSIGNED_BYTE, (GLubyte*)str);
}

void TextOut3D(HDC hdc, int x, int y, const char* str, int length, int z = 0)
{
	if(bIsOpenGL)
	{
		UINT uiTAlign = GetTextAlign(hdc);
		//DEBUG_OUT("uiTAlign = " << uiTAlign)
		//DEBUG_OUT("uiTAlign & TA_CENTER = " << (uiTAlign & TA_CENTER))
		//DEBUG_OUT("TA_RIGHT = " << (uiTAlign & TA_RIGHT))
		//DEBUG_OUT("uiTAlign | ~TA_TOP = " << (uiTAlign | ~TA_TOP))
		//DEBUG_OUT("(uiTAlign | ~TA_TOP)&1 = " << ((uiTAlign | ~TA_TOP)&1))
		if(uiTAlign != 0)
			y += int(iWidth*1.5)-1;
		if (uiTAlign == TA_CENTER)
			x -= (iWidth*length)/2;
		else if (uiTAlign == TA_RIGHT)
			x -= (iWidth*length);
		TextOutGL(x,iBottom - y,z, str);
//		TextOutGL(x - iWidth*length,iBottom - y + iWidth/2,z, str);
	}
	else
	{
		TextOut(hdc, x, y, str, length);
	}
}

//-----------------------------------------------------------------------------------------
const range
chart::GetValueRange(const range &rX0, const RECT &br, ANM_chart_window *prop)
{
	range ret(1,-1);
	if(!bDraw)
		return range(1,-1);
	range rX = prop->Xranges;
	range rY = prop->Yranges;
	range rZ = prop->Zranges;

	numeric dx = (rX.get())/numeric(br.right - br.left);
	numeric dy = (rY.get())/numeric(br.bottom - br.top);
	numeric dz = (rZ.get())/numeric(br.right - br.left);
	numeric curx = rX.rmin;
	numeric curz = rZ.rmin;
	ex y = 0;
	try{
		y = func.subs(lst(var,var2), lst(curx, curz)).evalf();
	} catch (const exception &e) {
		cout << "Error: GetValueRange " << func << "(" << curx << "): " << e.what() << "\x0D\x0A" << flush;
	}
	if(!is_a<numeric>(y))
		return ret;
	numeric cury = ex_to<numeric>(y);
	ret.rmax = ret.rmin = cury;
	while(curz <= rZ.rmax)
	{
		curx = rX.rmin;
		while(curx <= rX.rmax)
		{
			try{
				cury = ex_to<numeric>(func.subs(lst(var,var2), lst(curx, curz)).evalf());
				ret.rmax = max(ret.rmax, cury);
				ret.rmin = min(ret.rmin, cury);
			} catch (const exception &e) {
				cout << "Error: GetValueRange " << func << "(" << curx << "): " << e.what() << "\x0D\x0A" << flush;
			}
			curx += dx*prop->dt;
			if (curx > rX.rmax && curx - dx*prop->dt < rX.rmax)
				curx = rX.rmax;
		}
		curz += dz*prop->dt;
		if (curz > rZ.rmax && curz - dz*prop->dt < rZ.rmax)
			curz = rZ.rmax;
		if ((dz.is_zero())||!(prop->CurPlot)||!(prop->CurPlot->Is3D))
			curz = rZ.rmax+1;
	}
	//DEBUG_OUT("cury = " << cury)
	//DEBUG_OUT(" chart::GetValueRange::ret = (" << ret.rmin << ", " << ret.rmax << ")")
	return ret;
}
//-----------------------------------------------------------------------------------------
const range all_charts::GetValueRange(const range &ParamsRange, const RECT &br, const result *phase, ANM_chart_window *prop)
{
	range ret(1,-1);
	if(br.right <= br.left || br.top >= br.bottom)
		return range(1,-1);
	for(int i = 0; i < ANMs.size(); i++)
		ret = ret.max_range(ANMs[i].GetValueRange(ParamsRange, br, phase, prop));
	return ret;
}

const range all_charts::GetXRange()
{
	range ret(1,-1);
	for(int i = 0; i < ANMs.size(); i++)
		ret = ret.max_range(ANMs[i].GetXRange());
	return ret;
}

const range all_charts::GetValueRange(const range &ParamsRange, const RECT &br, ANM_chart_window *prop)
{
	range ret(1,-1);
	if(br.right <= br.left || br.top >= br.bottom)
		return range(1,-1);
	for(int i = 0; i < plots.size(); i++)
	{
		//DEBUG_OUT(" GetValueRange::i = " << i)
		ret = ret.max_range(plots[i].GetValueRange(ParamsRange, br, prop));
		//DEBUG_OUT(" all_charts::GetValueRange::ret = (" << ret.rmin << ", " << ret.rmax)
	}
	for(int i = 0; i < ANMs.size(); i++)
		ret = ret.max_range(ANMs[i].GetValueRange(ParamsRange, br, prop));
	//DEBUG_OUT(" all_charts::GetValueRange::ret = (" << ret.rmin << ", " << ret.rmax)
	return ret;
}

//-----------------------------------------------------------------------------------------
ANM_chart_window::ANM_chart_window(HWND aMFCWnd, ANM_system *aODEsystem, Plot *aPlot)
{
	MFCWnd = aMFCWnd;
	chartDC = NULL;
	bOpenGL = true;
	bIsDrawing = false;
	glListInd = 1;
	ODEsystem = aODEsystem;
	CurPlot = aPlot;
	unvisible = 0;
	bPause = false;
	bDrawSteps = true;		// обновление графиков пошагово
	bLeftBtn = true;
	bDrawStep = (GetVersion() < 0x80000000);	// W2K, NT, XP
	bLegend = true;
	bDrawDomen = true;
	bkColor = RGB(255,255,255);
	LineColor = RGB(0,0,0);
	GridColor = RGB(100, 150, 100);
	GridType = PS_DOT;
	Rop2Mode = R2_COPYPEN;
	dt = numeric(10);
	fontName = "Courier New Cyr";
	fontHeight = 16;
	nWeight = FW_NORMAL;
	LineWidth = 1;
	OthersMode = 0;
	m = 8;
	n = 10;
    // чтобы по умолчанию все рисовалось относительно независимой переменной расчета (например, t)
    phase_t = 0;
    // чтобы по умолчанию все рисовалось в пространстве (графики располагаются по оси z)
    phase_z = 0;
// Если указатель ненулевой, то инициализировать сохранение результатов расчета
// Для этого нужно добавить новый пустой результат расчета (мы ведь не начинали ничего считать)
// и определить указатель на то место, где будут храниться результаты (push_back его возвращает)
	Xranges.rmax = 10;
	Xranges.rmin = -10;
	Yranges.rmax = 10;
	Yranges.rmin = -10;
	Zranges.rmax = 10;
	Zranges.rmin = -10;
	//====== Initial image turn
	m_AngleX = 0.f;
	m_AngleY = 0.f;
	m_ZoomZ = 0.f;
	m_Trans = 0;
	defXr = Xranges;
	bSensitivityChart = false;
	//DEBUG_OUT("ODEsystem = " << ODEsystem)
	//DEBUG_OUT("ODEsystem->Result = " << ODEsystem->Result)
	if(ODEsystem)
	{
		// если имеем дело с параметрической чувствительностью, то
		// если Result = NULL, значит это окно рисования основного графика (по времени)
		// иначе - окно рисования по параметру
		if(is_a<ANM_sens_system>(*ODEsystem))
		{
			if (ODEsystem->Result != NULL)
			{
				((ANM_sens_system*)(ODEsystem))->AllResults = &AllCharts;
//				((ANM_sens_system*)(ODEsystem))->SensChart = this;
				bSensitivityChart = true;
				OwnerTimeRange = ((ANM_sens_system*)(ODEsystem))->PlotsTimeRange;
				return;
			}
			else
				((ANM_sens_system*)(ODEsystem))->PlotsTimeRange = &Xranges;
		}
		for(int i = 0; i < ODEsystem->AdditionalFuncs.nops(); i++)
			AllCharts.push_back(ODEsystem->AdditionalFuncs.op(i), s_t);
		Xranges.rmax = max(ODEsystem->SupT, ODEsystem->InfT);
		Xranges.rmin = min(ODEsystem->SupT, ODEsystem->InfT);
		ODEsystem->Result = AllCharts.push_back(ODEsystem->GetEmptyResult());
		unvisible = ODEsystem->Result->size() - ODEsystem->Solutions.size();
		defXr = Xranges;
		LineWidth = ODEsystem->LineWidth;
		fontHeight = ODEsystem->FontSize;
		nWeight = ODEsystem->nWeight;
	}
	if(CurPlot)
	{
		for(int i = 0; i < CurPlot->Funcs.nops(); i++)
			AllCharts.push_back(CurPlot->Funcs.op(i), CurPlot->x, CurPlot->y);
		Xranges = range(CurPlot->Xmin, CurPlot->Xmax);
		Yranges = range(CurPlot->Ymin, CurPlot->Ymax);
		Zranges = range(CurPlot->Zmin, CurPlot->Zmax);
		defXr = Xranges;
		LineWidth = CurPlot->LineWidth;
		fontHeight = CurPlot->FontSize;
		nWeight = CurPlot->nWeight;
	}

	//DEBUG_OUT("ODEsystem->Result = " << ODEsystem->Result)
	//DEBUG_OUT("bSensitivityChart = " << bSensitivityChart)
}

string ANM_chart_window::GetStrDataToSave()
{
	string OutStr;
	string tmpStr;

	OutStr += "Chart;";	// chart properties
	OutStr += AllCharts.GetStrDataToSave();
	OutStr += ToString(bDrawDomen) + ";";
	OutStr += ToString(bkColor) + ";";
	OutStr += ToString(bLegend) + ";";
	OutStr += ToString(defXr.rmin)+";";
	OutStr += ToString(defXr.rmax)+";";
	OutStr += ToString(dt) + ";";
	OutStr += fontName + ";";
	OutStr += ToString(fontHeight) + ";";
	OutStr += ToString(GridColor) + ";";
	OutStr += ToString(GridType) + ";";
	OutStr += ToString(LineColor) + ";";
	OutStr += ToString(m) + ";";
	OutStr += ToString(n) + ";";
	OutStr += ToString(Xranges.rmin.evalf())+";";
	OutStr += ToString(Xranges.rmax.evalf())+";";
	OutStr += ToString(Yranges.rmin.evalf())+";";
	OutStr += ToString(Yranges.rmax.evalf())+";";
	OutStr += ToString(1)+";";
	OutStr += ToString(1)+";";
    if(phase_t)
        OutStr += phase_t->name + ";";
    else
        OutStr += "t;";
	return OutStr;
}

string ANM_chart_window::GetStrData()
{
	string OutStr;
	string tmpStr;

	OutStr += "Chart;";	// chart properties
	OutStr += AllCharts.GetStrData();
	OutStr += ToString(bDrawDomen) + ";";
	OutStr += ToString(bkColor) + ";";
	OutStr += ToString(bLegend) + ";";
	OutStr += ToString(defXr.rmin)+";";
	OutStr += ToString(defXr.rmax)+";";
	OutStr += ToString(dt) + ";";
	OutStr += fontName + ";";
	OutStr += ToString(fontHeight) + ";";
	OutStr += ToString(GridColor) + ";";
	OutStr += ToString(GridType) + ";";
	OutStr += ToString(LineColor) + ";";
	OutStr += ToString(m) + ";";
	OutStr += ToString(n) + ";";
	OutStr += ToString(Xranges.rmin.evalf())+";";
	OutStr += ToString(Xranges.rmax.evalf())+";";
	OutStr += ToString(Yranges.rmin.evalf())+";";
	OutStr += ToString(Yranges.rmax.evalf())+";";
	OutStr += ToString(1)+";";
	OutStr += ToString(1)+";";
    if(phase_t)
        OutStr += phase_t->name + ";";
    else
        OutStr += "t;";
	return OutStr;
}

void ANM_chart_window::SetRanges(const range rX, const range rY, bool brX, bool brY)
{
	HDC hdc;
	hdc = GetDC(MFCWnd);
    RECT br;
	GetClipBox(hdc, &br);
	ReleaseDC(MFCWnd, hdc);
	defXr = AllCharts.GetXRange();
	//DEBUG_OUT(" if (defXr.rmax < defXr.rmin) ")
	if (defXr.rmax < defXr.rmin)
		defXr = Xranges;
	if(brX && rX.get() > 0)
		Xranges = rX;
	else
        if(phase_t)
	        Xranges = AllCharts.GetValueRange(defXr, br, phase_t, this);
        else
		{
			Xranges = defXr;
			m_AngleX = m_AngleY = m_ZoomZ = 0;
		}
	//DEBUG_OUT(" if(brY && rY.get() > 0) ")
	if(brY && rY.get() > 0)
		Yranges = rY;
	else
        if(phase_t)
    		Yranges = AllCharts.GetValueRange(Tranges, br, this);
        else
    		Yranges = AllCharts.GetValueRange(Xranges, br, this);
	//DEBUG_OUT(" if(Xranges.get() <= 0) ")
    if(Xranges.get() <= 0)
        Xranges = defXr;
    if(Yranges.get() <= 0)
        Yranges = range(-10, 10);
	if(glListInd > 1)
		glDeleteLists(1, glListInd);
	glListInd = 1;
}

// индекс текущего результата (для sub_res - others и other_rows)
int curResInd = -1;
void *curANMCharts = 0;
void ANM_chart_window::SetData(const string &inStr)
{
	//DEBUG_OUT("inStr = " << inStr)
	curResInd = -1;
	curANMCharts = 0;
	char *buf, *token;
	buf = new char[inStr.length()+10];
	strcpy(buf, inStr.c_str());
	token = strtok(buf, ";");
	if(!strcmp(token, "Chart"))
	{
		string tok;
		tok = strtok( NULL, ";");
		if(tok == "_Begin")
		{
			tok = strtok( NULL, ";");
			//DEBUG_OUT("tok0 = " << tok)
			unvisible = 0;
			while(tok != "_End")
			{
				//DEBUG_OUT("tok1 = " << tok)
				unsigned long c = (unsigned long)(atof(strtok( NULL, ";")));
				bool bDraw = (bool)(atoi(strtok( NULL, ";")));
				//DEBUG_OUT("bool bDraw = (bool)")
				AllCharts.SetData(tok, c, bDraw);
				//DEBUG_OUT("AllCharts.SetData(tok, c, bDraw);")
				if(!bDraw)
					unvisible++;
				//DEBUG_OUT("bDraw <> false for " << tok)
				tok = strtok(NULL, ";");
			}
		}
		bDrawDomen = atoi(strtok( NULL, ";"));
		bkColor = (unsigned long)(atof(strtok( NULL, ";")));
		bLegend = atoi(strtok( NULL, ";"));
		string defXmin, defXmax;
		defXmin = strtok( NULL, ";");
		tok = strtok(NULL, ";");
		defXmax = tok;
		defXr = range(numeric(defXmin.c_str()), numeric(defXmax.c_str()));
		dt = strtok( NULL, ";");
		if(dt <= numeric(0))
			dt = 1;
		fontName = (strtok( NULL, ";"));
		fontHeight = atoi(strtok( NULL, ";"));
		GridColor = (unsigned long)(atof(strtok( NULL, ";")));
		GridType = (unsigned long)(atof(strtok( NULL, ";")));
		LineColor = (unsigned long)(atof(strtok( NULL, ";")));
		m = atoi(strtok( NULL, ";"));
		if(m <= 0)
			m = 4;
		n = atoi(strtok( NULL, ";"));
		if(n <= 0)
			n = 4;
		range Xr, Yr;
		Xr.rmin = (strtok( NULL, ";"));
		Xr.rmax = (strtok( NULL, ";"));
		Yr.rmin = (strtok( NULL, ";"));
		Yr.rmax = (strtok( NULL, ";"));
		bool Xcheck, Ycheck;
		Xcheck = atoi(strtok( NULL, ";"));
		Ycheck = atoi(strtok( NULL, ";"));
        tok = strtok( NULL, ";");
		// если до установки настроек не было фазовых траекторий, то диапазон по времени = диапазону по оси абсцисс
		if(!phase_t)
			Tranges = Xranges;
        if("t" == tok)
            phase_t = NULL;
        else
            phase_t = AllCharts.GetResultPtr(tok);
        SetRanges(Xr, Yr, Xcheck, Ycheck);
	}
	//DEBUG_OUT("buf = " << buf);
	delete buf;
    CreateCompDC();
//	DrawAllPlots();
}

#define ASSERT(x) if(!(x)) return;

void ANM_chart_window::get_lim_coords(numeric coord1, numeric coord2, numeric touch, // in
                         numeric &start_coord, numeric &end_coord) // out
{
//	if(p >= Digits - 1)
//		throw runtime_error("Слишком крупный масштаб для выбранной точности вычислений (Digits)");
   ASSERT(coord1 < coord2);
   start_coord = touch * to_integer((coord1 / touch));
   while (start_coord > coord1)
      start_coord -= touch;

   end_coord = touch * to_integer((coord2 / touch));
   while (end_coord < coord2)
      end_coord += touch;

   ASSERT((start_coord < coord1 || (start_coord == coord1)) && (coord1 - start_coord < touch));
   ASSERT((end_coord > coord2 || (end_coord == coord2)) && (end_coord - coord2 < touch));
}

// получить промежуточные метки при заданном шаге
void ANM_chart_window::get_touch_arr(numeric coord1, numeric coord2, numeric touch, numArray& coords)
{
   numeric start_coord = -1, end_coord = -1;

   get_lim_coords(coord1, coord2, touch, start_coord, end_coord);

   ASSERT(!(coords.size()));

   while (start_coord < end_coord || (start_coord == end_coord))
   {
      coords.push_back(start_coord);
      start_coord += touch;
   }
}

// получить количество меток при заданном шаге
int ANM_chart_window::test_touch_arr(numeric coord1, numeric coord2, numeric touch)
{
   numeric start_coord = -1, end_coord = -1;

   get_lim_coords(coord1, coord2, touch, start_coord, end_coord);

    //DEBUG_OUT(((end_coord - start_coord) / touch).to_double() << " = " << ((end_coord - start_coord) / touch).evalf() << "\x0D\x0A";
   return to_integer((end_coord - start_coord) / touch) + 1;
}

// возвращает заполненный вектор с промежуточными значениями
void ANM_chart_window::get_touchs(numeric coord1, numeric coord2, const int max_touch_count, numArray& touchs)
{
	if(coord2 - coord1 < 1e-6)
	{
		for(int k = 0; k < max_touch_count; k++)
			touchs.push_back(coord1 + (coord2 - coord1)/numeric(max_touch_count)*numeric(k));
		return;
	}
   numeric diff = coord2 - coord1;
   // определить приблизительный порядок
   int p = to_integer(log(diff)/log(numeric(10)));
   // начинать можно с любого touch, просто такой подход позволяет быстрее достичь результата
   numeric touch = pow(numeric(10), numeric(p));

   // флаг окончания поиска решения
   bool is_cont = true;

   const int DENOM_COUNT = 3;
   const int denom[] = {2, 5, 10};

   // здесь будут сохранены подходящие результаты
   numeric res_touch_count = -1;
   numeric res_touch = -1;

   numeric saved_touch = touch + (1); // используется на каждой конкретной итерации
   // "+ 1" - чтобы не было равны при первой проверке

   // по идее сразу же может быть найден ответ. Но этот ответ может быть не оптимальным с точки зрения
   // приближенности к границам... поэтому надо пробовать найти более приближенный ответ...
   // если это дело не получится, тогда уже надо брать этот ответ!
   while (is_cont)
   {
      // если такое будет, значит, вошли в зацикливание!
      ASSERT(saved_touch != touch);

      saved_touch = touch;

      for (int k = 0; k < DENOM_COUNT; ++k)
      {
         // получить число промежуточных меток при заданном шаге
         const int touch_count = test_touch_arr(coord1, coord2, touch);

         if (res_touch_count != -1)
         {
            // сравнить результаты предыдущей и текущей итераций:
            // если раньше результаты устраивали, а сейчас уже нет, то пора завязывать
            if (res_touch_count <= max_touch_count && touch_count > max_touch_count)
            {
               is_cont = false;
               break;
            }
         }

         if (touch_count <= max_touch_count)
         {
            // сохранить подходящие результаты
            res_touch_count = touch_count;
            res_touch = touch;
            // пытаемся уменьшить шаг
            touch = saved_touch / (denom[k]);
         }
         else
         {
            // пытаемся увеличить шаг
            touch = saved_touch * (denom[k]);
         }
      }
   }

   get_touch_arr(coord1, coord2, res_touch, touchs);
   ASSERT(touchs.size() > 1);
}

void ANM_chart_window::CreateBMP(int width, int height)
{
	if (!bOpenGL)
	{
		HDC hdc;
		hdc = GetDC(MFCWnd);
		chartBitmap = CreateCompatibleBitmap(hdc, width, height);
		ReleaseDC(MFCWnd, hdc);
	}
	else
	{
		void *pBitsDIB(NULL);            // содержимое битмапа
		int cxDIB(width); int cyDIB(height);  // его размеры (например для окна 200х300)
		BITMAPINFOHEADER BIH;            // и заголовок

		// …
		// создаем DIB section
		// создаем структуру BITMAPINFOHEADER, описывающую наш DIB
		int iSize = sizeof(BITMAPINFOHEADER);  // размер
		memset(&BIH, 0, iSize);

		BIH.biSize = iSize;        // размер структуры
		BIH.biWidth = cxDIB;       // геометрия
		BIH.biHeight = cyDIB;      // битмапа
		BIH.biPlanes = 1;          // один план
		BIH.biBitCount = 24;       // 24 bits per pixel
		BIH.biCompression = BI_RGB;// без сжатия

		// создаем DIB-секцию
		chartBitmap = CreateDIBSection(
		chartDC,                  // контекст устройства
		(BITMAPINFO*)&BIH,       // информация о битмапе
		DIB_RGB_COLORS,          // параметры цвета
		&pBitsDIB,               // местоположение буфера (память выделяет система)
		NULL,                    // не привязываемся к отображаемым в память файлам
		0);

	}
}

void ANM_chart_window::CreateCompDC()
{
	RECT br;
	HDC hdc;
	hdc = GetDC(MFCWnd);
	GetClipBox(hdc, &br);
	DeleteObject(chartDC);
	chartDC = CreateCompatibleDC(hdc);
	FW_NORMAL;
	font = CreateFont(
		fontHeight,                // nHeight
		0,                         // nWidth
		0,                         // nEscapement
		0,                         // nOrientation
		nWeight,                 // nWeight FW_NORMAL FW_BOLD
		FALSE,                     // bItalic
		FALSE,                     // bUnderline
		0,                         // cStrikeOut
		ANSI_CHARSET,              // nCharSet
		OUT_DEFAULT_PRECIS,        // nOutPrecision
		CLIP_DEFAULT_PRECIS,       // nClipPrecision
		DEFAULT_QUALITY,           // nQuality
		DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
		fontName.c_str());
	CreateBMP(br.right - br.left, br.bottom-br.top);
	SelectObject(chartDC, chartBitmap);
	SelectObject(chartDC, font);
	bOpenGL = true;
	if(bOpenGL)
	{
	// Инициализация OpenGL
		PIXELFORMATDESCRIPTOR pfd =	// Structure used to describe the format
		{
			sizeof(PIXELFORMATDESCRIPTOR),
			1,			// Version
			PFD_DRAW_TO_BITMAP |	//
			PFD_SUPPORT_OPENGL,// |	// Supports GDI
	//		PFD_DOUBLEBUFFER,	// Use double buffering (more efficient drawing)
			PFD_TYPE_RGBA,		// No pallettes
			24, 			// Number of color planes
		 				// in each color buffer
			24,	0,		// for Red-component
			24,	0,		// for Green-component
			24,	0,		// for Blue-component
			24,	0,		// for Alpha-component
			0,			// Number of planes
						// of Accumulation buffer
			0,			// for Red-component
			0,			// for Green-component
			0,			// for Blue-component
			0,			// for Alpha-component
			32, 			// Depth of Z-buffer
			0,			// Depth of Stencil-buffer
			0,			// Depth of Auxiliary-buffer
			0,			// Now is ignored
			0,			// Number of planes
			0,			// Now is ignored
			0,			// Color of transparent mask
			0			// Now is ignored
		};

		//====== Ask to find the nearest compatible pixel-format
		int iD = ChoosePixelFormat(chartDC, &pfd);
		if ( !iD )
		{
			bOpenGL = false;
			DEBUG_OUT("ChoosePixelFormat::Error");
		}

		//====== Try to set this format
		if ( !SetPixelFormat (chartDC, iD, &pfd) )
		{
			DEBUG_OUT("SetPixelFormat::Error");
			bOpenGL = false;
		}

		//====== Try to create the OpenGL rendering context
		if ( !(m_hRC = wglCreateContext (chartDC)))
		{
			DEBUG_OUT("wglCreateContext::Error");
			bOpenGL = false;
		}

		//====== Try to put it in action
		if ( !wglMakeCurrent (chartDC, m_hRC))
		{
			DEBUG_OUT("wglMakeCurrent::Error");
			bOpenGL = false;
		}
		int m_FontListBase = 1000;
		if ( !wglUseFontBitmaps (chartDC, 0, 255, m_FontListBase))
		{
			DEBUG_OUT("wglUseFontBitmaps::Error");
			bOpenGL = false;
		}
		glEnable(GL_DEPTH_TEST);
		wglMakeCurrent (NULL, NULL);
		if(!bOpenGL)
			CreateCompDC();
	}
	bIsOpenGL = bOpenGL;
	iBottom = br.bottom;
	ReleaseDC(MFCWnd, hdc);
	GetCharWidth(chartDC, 'W', 'W', &cWidth);
	iWidth = cWidth;
	//DEBUG_OUT("iWidth = " << iWidth)
	TEXTMETRIC tm;
	GetTextMetrics(chartDC, &tm);
	cHeight = tm.tmHeight + tm.tmExternalLeading;
	//DEBUG_OUT("cHeight = " << cHeight)
	pRect.bottom = 2*cHeight;
	if(bLegend)
		pRect.bottom += (cHeight*6*(AllCharts.size() - unvisible))/5;
	pRect.left = cWidth*(10/*Digits*/);
	pRect.right = cWidth*10/*Digits*//2;
	pRect.top = cHeight + 40*bDrawStep;
// определение области построения
	cRect.left = br.left + pRect.left;
	cRect.top = br.top + pRect.top;
	cRect.right = br.right - pRect.right;
	cRect.bottom = br.bottom - pRect.bottom;
    Clear();
	Invalidate();
}

void ANM_chart_window::ReSize()
{
	if(!MFCWnd || !chartDC)
		return;
	RECT br;
	HDC hdc;
	hdc = GetDC(MFCWnd);
	GetClipBox(hdc, &br);
	if(cRect.left == br.left + pRect.left && cRect.top == br.top + pRect.top && cRect.right == br.right - pRect.right && cRect.bottom == br.bottom - pRect.bottom)
    {
    	ReleaseDC(MFCWnd, hdc);
		return;
        /*Sleep(10);
	    hdc = GetDC(MFCWnd);
	    GetClipBox(hdc, &br);
    	if(cRect.left == br.left + pRect.left && cRect.top == br.top + pRect.top && cRect.right == br.right - pRect.right && cRect.bottom == br.bottom - pRect.bottom)
        {
        	ReleaseDC(MFCWnd, hdc);
            return;
        }*/
    }
	DeleteObject(chartBitmap);
	CreateBMP(br.right - br.left, br.bottom-br.top);
//	chartBitmap = CreateCompatibleBitmap(hdc, br.right - br.left, br.bottom - br.top);
	SelectObject(chartDC, chartBitmap);
	ReleaseDC(MFCWnd, hdc);
	if(bOpenGL)
	{
		wglMakeCurrent (chartDC, m_hRC);
		glViewport(0,0,br.right,br.bottom);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		wglMakeCurrent (NULL, NULL);
		iBottom = br.bottom;
	}
// определение области построения
	cRect.left = br.left + pRect.left;
	cRect.top = br.top + pRect.top;
	cRect.right = br.right - pRect.right;
	cRect.bottom = br.bottom - pRect.bottom;
// clearing
	Clear();
	Invalidate();
	DrawAllPlots();
	Invalidate();
}
void ANM_chart_window::DrawState(const string &sState)
{
	return;
	if(!MFCWnd || !chartDC || !bOpenGL)
		return;
	RECT br;
	HDC hdc;
	hdc = GetDC(MFCWnd);
	GetClipBox(hdc, &br);
   	ReleaseDC(MFCWnd, hdc);
	glBegin(GL_QUADS);
	glColor3ub(0,128,0);
	glVertex3f(br.left, 0, 0);
	glVertex3f(br.right, 0, 0);
	glVertex3f(br.right, iWidth*2, 0);
	glVertex3f(br.left, iWidth*2, 0);
	glEnd();
	SetTextAlign(chartDC, TA_CENTER);
	TextOut3D(chartDC, (br.right-br.left)/2, iBottom - (iWidth*5)/2 + 4, sState.c_str(), sState.length());
	glFlush();
}

void ANM_chart_window::DrawPlots()
{
	if(!MFCWnd || !chartDC)
		return;
	RECT br;
	GetClipBox(chartDC, &br);
// определение области построения
	cRect.left = br.left + pRect.left;
	cRect.top = br.top + pRect.top;
	cRect.right = br.right - pRect.right;
	cRect.bottom = br.bottom - pRect.bottom;
// если область построения слишком маленькая, то...
	if(cRect.right - cRect.left <= 0 || cRect.bottom - cRect.top <= 0)
		return;
// drawing
	//DEBUG_OUT(" drawing ")
	// Определение диапазонов
    SetRanges(range(1,-1), range(1,-1), false, false);
// clearing
	//DEBUG_OUT(" clearing ")
	Clear();
	//DEBUG_OUT(" DrawAllPlots ")
	DrawAllPlots();
	//DEBUG_OUT(" Invalidate ")
	Invalidate();
/*	if(ODEsystem)
		if(bPause && !(ODEsystem->CanEvulate()))
		{
		}
*/
}

// рисование осей
void ANM_chart_window::DrawAxis()
{
	RECT r;
	HPEN pen, pOldPen;
	int i;
	GetClipBox(chartDC, &r);
	pen = CreatePen(PS_SOLID, (int)LineWidth, (COLORREF)LineColor);
	pOldPen = (HPEN)SelectObject(chartDC, pen);

	SetGLColor(LineColor);
	numeric dt = numeric(cRect.right - cRect.left)/(Xranges.get());
	numeric dV = numeric(cRect.bottom - cRect.top)/(Yranges.get());
	numeric dz = numeric(cRect.right - cRect.left)/(Zranges.get()*Zranges.get()/Xranges.get());
	m_Trans = to_integer(dz*Zranges.get());
	numArray Touchs;
//	Вывод осей абсцисс и ординат
	glBegin(GL_LINE_STRIP);
	MoveTo3D(chartDC, cRect.left, cRect.top);
	LineTo3D(chartDC, cRect.left, cRect.bottom);
	LineTo3D(chartDC, cRect.right, cRect.bottom);
	glEnd();
//	Вывод оси аппликат
	glBegin(GL_LINE_STRIP);
	LineTo3D(chartDC, cRect.left, cRect.bottom);
	LineTo3D(chartDC, cRect.left, cRect.bottom, int(m_Trans+1));
	glEnd();
//	Вывод оси ординат
	//DEBUG_OUT("SetTextAlign(chartDC, TA_RIGHT );")
	SetTextAlign(chartDC, TA_RIGHT );
	Touchs.clear();
	get_touchs(Yranges.rmin, Yranges.rmax, m, Touchs);
	for(i = 0; i < Touchs.size(); i++)
	{
		if(Touchs[i] < Yranges.rmin || Touchs[i] > Yranges.rmax)
			continue;
		int y0 = cRect.bottom - to_integer((Touchs[i] - Yranges.rmin)*dV);
		glBegin(GL_LINE_STRIP);
		MoveTo3D(chartDC, cRect.left - 3, y0);
		LineTo3D(chartDC, cRect.left + 3, y0);
		glEnd();
		TextOut3D(chartDC, cRect.left - 5, y0 - cHeight/2, num_to_string(Touchs[i]).c_str(), num_to_string(Touchs[i]).length());

	}
	glBegin(GL_LINE_STRIP);
	MoveTo3D(chartDC, cRect.left, cRect.top);
	LineTo3D(chartDC, cRect.left, cRect.top - 15);
	LineTo3D(chartDC, cRect.left-4, cRect.top - 5);
	glEnd();
	glBegin(GL_LINE_STRIP);
	MoveTo3D(chartDC, cRect.left, cRect.top - 15);
	LineTo3D(chartDC, cRect.left+4, cRect.top - 5);
	glEnd();
//	Вывод оси абсцисс
	SetTextAlign(chartDC, TA_CENTER | TA_TOP );
	//DEBUG_OUT("SetTextAlign(chartDC, TA_CENTER | TA_TOP );")
	Touchs.clear();
	get_touchs(Xranges.rmin, Xranges.rmax, n, Touchs);
	for(i = 0; i < Touchs.size(); i++)
	{
		if(Touchs[i] < Xranges.rmin || Touchs[i] > Xranges.rmax)
			continue;
		int x0 = cRect.left + to_integer((Touchs[i] - Xranges.rmin)*dt);
		glBegin(GL_LINE_STRIP);
		MoveTo3D(chartDC, x0, cRect.bottom - 3);
		LineTo3D(chartDC, x0, cRect.bottom + 3);
		glEnd();
		TextOut3D(chartDC, x0, cRect.bottom + 5, num_to_string(Touchs[i]).c_str(), num_to_string(Touchs[i]).length());
	}
	glBegin(GL_LINE_STRIP);
	MoveTo3D(chartDC, cRect.right, cRect.bottom);
	LineTo3D(chartDC, cRect.right+20, cRect.bottom);
	LineTo3D(chartDC, cRect.right+10, cRect.bottom - 4);
	glEnd();
	glBegin(GL_LINE_STRIP);
	MoveTo3D(chartDC, cRect.right+20, cRect.bottom);
	LineTo3D(chartDC, cRect.right+10, cRect.bottom + 4);
	glEnd();
    if(phase_t)
		TextOut3D(chartDC, cRect.right+20, cRect.bottom + cHeight+5-cHeight/2, phase_t->name.c_str(), phase_t->name.length());
    else
		if(CurPlot)
			TextOut3D(chartDC, cRect.right+20, cRect.bottom + cHeight+5-cHeight/2, ToString(CurPlot->x).c_str(), ToString(CurPlot->x).length());
		else
			TextOut3D(chartDC, cRect.right+20, cRect.bottom + cHeight+5-cHeight/2, "t", 1);
//	Вывод оси аппликат
	//DEBUG_OUT("Вывод оси аппликат")
	if(m_AngleX != 0 || m_AngleY != 0)
	{
		Touchs.clear();
		//DEBUG_OUT("m_Trans = " << m_Trans)
		get_touchs(Zranges.rmin, Zranges.rmax, n, Touchs);
		for(i = 0; i < Touchs.size(); i++)
		{
			if(Touchs[i] < Zranges.rmin || Touchs[i] > Zranges.rmax)
				continue;
			int z0 = to_integer((Touchs[i] - Zranges.rmin)*dz);
			//DEBUG_OUT("z0 = " << z0)
			glBegin(GL_LINE_STRIP);
			MoveTo3D(chartDC, cRect.left, cRect.bottom - 3, z0);
			LineTo3D(chartDC, cRect.left, cRect.bottom + 3, z0);
			glEnd();
			TextOut3D(chartDC, cRect.left, cRect.bottom + 5, num_to_string(Touchs[i]).c_str(), num_to_string(Touchs[i]).length(), z0);
		}
		glBegin(GL_LINE_STRIP);
		MoveTo3D(chartDC, cRect.left, cRect.bottom, int(m_Trans));
		LineTo3D(chartDC, cRect.left, cRect.bottom, int(m_Trans) + 20);
		LineTo3D(chartDC, cRect.left, cRect.bottom - 4, int(m_Trans) + 10);
		glEnd();
		glBegin(GL_LINE_STRIP);
		MoveTo3D(chartDC, cRect.left, cRect.bottom, int(m_Trans) + 20);
		LineTo3D(chartDC, cRect.left, cRect.bottom + 4, int(m_Trans) + 10);
		glEnd();
		if(CurPlot)
			TextOut3D(chartDC, cRect.left, cRect.bottom + cHeight+5-cHeight/2, ToString(CurPlot->y).c_str(), ToString(CurPlot->y).length(), to_integer(Zranges.get()*dz));
	}
	SelectObject(chartDC, pOldPen);
	DeleteObject(pen);
}


// рисование Сетки
void ANM_chart_window::DrawGrid()
{
	if(!GridType)
		return;
	RECT r;
	HPEN pen, pOldPen;
	int i;
	GetClipBox(chartDC, &r);
	pen = CreatePen((int)GridType, (int)LineWidth, (COLORREF)GridColor);

	pOldPen = (HPEN)SelectObject(chartDC, pen);
	numeric dt = numeric(cRect.right - cRect.left)/(Xranges.get());
	numeric dV = numeric(cRect.bottom - cRect.top)/(Yranges.get());
	if(bOpenGL)
	{
		SetGLColor(GridColor);
		glLineStipple(2,0xAAAA);
		glEnable(GL_LINE_STIPPLE);
	}
	numArray Touchs;
//	Вывод горизонтальных линий
	Touchs.clear();
	get_touchs(Yranges.rmin, Yranges.rmax, m, Touchs);
	for(i = 0; i < Touchs.size(); i++)
	{
		if(Touchs[i] <= Yranges.rmin || Touchs[i] > Yranges.rmax)
			continue;
		int y0 = cRect.bottom - to_integer((Touchs[i] - Yranges.rmin)*dV);
		glBegin(GL_LINE_STRIP);
		MoveTo3D(chartDC, cRect.left, y0);
		LineTo3D(chartDC, cRect.right, y0);
		glEnd();

	}
//	Вывод вертикальных линий
	Touchs.clear();
	get_touchs(Xranges.rmin, Xranges.rmax, n, Touchs);
	for(i = 0; i < Touchs.size(); i++)
	{
		if(Touchs[i] <= Xranges.rmin || Touchs[i] > Xranges.rmax)
			continue;
		int x0 = cRect.left + to_integer((Touchs[i] - Xranges.rmin)*dt);
		glBegin(GL_LINE_STRIP);
		MoveTo3D(chartDC, x0, cRect.bottom);
		LineTo3D(chartDC, x0, cRect.top);
		glEnd();
	}
	if(bOpenGL)
		glDisable(GL_LINE_STIPPLE);

	SelectObject(chartDC, pOldPen);
	DeleteObject(pen);
}

void ANM_chart_window::Invalidate()
{
	if(!MFCWnd || !chartDC)
		return;
	RECT br;
	HDC hdc;
	hdc = GetDC(MFCWnd);
	GetClipBox(hdc, &br);
	BitBlt(hdc, 0, 0, br.right - br.left, br.bottom - br.top, chartDC, 0, 0, SRCCOPY);
	ReleaseDC(MFCWnd, hdc);

	anm_trans tmpTrans;
	COPYDATASTRUCT cd;
	cd.dwData = ANM_ALLDRAW; // Все отрисовали, сообщаем об этом окну Infinity
	cd.lpData = &tmpTrans;
	cd.cbData = sizeof(tmpTrans);
	SendMessage(MFCWnd, WM_COPYDATA, 0, (LPARAM) &cd);
	bIsDrawing = false;
}

void ANM_chart_window::DrawAllPlots()
{
	if(!MFCWnd || !chartDC)
		return;
	if(cRect.right - cRect.left <= 0 || cRect.bottom - cRect.top <= 0)
		return;
	if(bOpenGL)
	{
		wglMakeCurrent (chartDC, m_hRC);
		glLineWidth (LineWidth);
	}
	// вывод текста "построение графика" в прогресс баре
//	SendMessage(ProgressWnd, WM_KEYDOWN, 1, 0);
	// Сетка
    //DEBUG_OUT("DrawGrid")
	DrawGrid();
	// рисование всех внесенных в список результатов
	CurPlotInd = 1;
	MaxNameLength = 0;
	bIsDrawing = true;
    //DEBUG_OUT("DrawPlots")
	if(!bOpenGL || glListInd == 1)
	{
		m_Trans = 0;
		AllCharts.DrawPlots(chartDC, this);
	}
	// рисование осей абсцисс(X), ординат(Y) и, если m_AngleX и m_AngleY != 0, аппликат(Z)
    //DEBUG_OUT("DrawAxis")
	if(	m_Trans == 0)
		m_Trans = 100;
	if(!CurPlot)
		Zranges.rmax = m_Trans;
	DrawAxis();
	if(bOpenGL)
	{
		//TextOutGL(10,100,0,"GLTest");
		for(int i = 1; i < glListInd; i++)
			glCallList(i);
		glFinish();
		glFlush();
		wglMakeCurrent(NULL,NULL);
	}
	if(ProgressWnd && ODEsystem)
		SendMessage(ProgressWnd, WM_KEYDOWN, 0, int(((ODEsystem->tN - ODEsystem->InfT)/(ODEsystem->SupT - ODEsystem->InfT)*numeric(100)).to_double()));
}

void ANM_chart_window::Clear()
{
	if(!MFCWnd || !chartDC)
		return;
	RECT br;
	GetClipBox(chartDC, &br);
	if(bOpenGL)
	{
		wglMakeCurrent (chartDC, m_hRC);
		// очищаем OpenGL экран
		glClearColor(GetRValue(bkColor)/255.0,GetGValue(bkColor)/255.0,GetBValue(bkColor)/255.0,1.0f );
		glClearDepth(1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(br.left,br.right,br.top,br.bottom,-(br.right-br.left) - labs(long(m_ZoomZ)),(br.right-br.left) + m_Trans +  labs(long(m_ZoomZ)));
		if(!CurPlot)
			Zranges.rmin = 0;
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef((br.right-br.left)/2,(br.bottom - br.top)/2,m_Trans/2);
		glRotatef (m_AngleX, 1.0f, 0.0f, 0.0f );	// and to rotate
		glRotatef (m_AngleY, 0.0f, 1.0f, 0.0f );
		glScalef(1.0-m_ZoomZ/(br.right-br.left),1-m_ZoomZ/(br.right-br.left),1-m_ZoomZ/(br.right-br.left));
		glTranslatef(-(br.right-br.left)/2,-(br.bottom - br.top)/2,-m_Trans/2);
		//DEBUG_OUT("m_ZoomZ = " << m_ZoomZ)

//		gluLookAt(m_AngleY, m_AngleX,0,0,0,-1,0,1,0);
//		gluLookAt(0, 0, 1000, 0, 0, -1000, 0,1,0);
//		gluLookAt(m_AngleX, m_AngleY, (br.right-br.left), 2*(br.right - br.left), 2*(br.bottom - br.top), 0, 0,-1,0);
		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) ;
		//glRotatef(m_AngleX, 1.0, 0.0, 0.0);
		//glRotatef(m_AngleY, 0.0, 1.0, 0.0);
		glFlush();
		wglMakeCurrent (NULL, NULL);
	}
	else
	{
	// clearing
		LOGBRUSH logBrush;
		logBrush.lbColor = bkColor;
		logBrush.lbStyle = BS_SOLID;
		HBRUSH brush;
		brush = CreateBrushIndirect(&logBrush);
		FillRect(chartDC, &br, brush);
		DeleteObject(brush);
	}
}

void ANM_chart_window::OnSaveToCHB(string fname)
{
	ofstream fstr;
	fstr.open(fname.c_str(), ios_base::out | ios_base::trunc);
	fstr.clear();
	fstr << "ANM_chart_window version 1.2.6";
	AllCharts.SaveToCHB(fstr);
	string tmpStr = GetStrDataToSave();
	//DEBUG_OUT("tmpStr = " << tmpStr)
	fstr << " " << tmpStr;
	fstr.close();
}

//сохранение в виде текстового файла с разделителями
void ANM_chart_window::OnSaveToCVS(string fname)
{
	ofstream fstr;
	fstr.open(fname.c_str(), ios_base::out | ios_base::trunc);
	fstr.clear();
	AllCharts.SaveToCVS(fstr, this);
	fstr.close();
}

void ANM_chart_window::OnOpenFromCHB(string fname)
{
	ifstream fstr;
	string tmpStr;

	string fname0 = fname;
	fstr.open(fname0.c_str(), ios_base::in | ios_base::binary);
	fstr >> tmpStr;
	if(tmpStr == "ANM_chart_window")
	{
		fstr >> tmpStr;
		if(tmpStr == "version")
		{
			fstr >> tmpStr;
			if(tmpStr == "1.2.6")
			{
				//DEBUG_OUT("if(tmpStr == 1.2.6)")
				AllCharts.LoadFromCHB(fstr);
				string strData;
				//DEBUG_OUT("string strData;")
				while(!fstr.eof())
				{
					fstr >> tmpStr;
					strData += tmpStr;
					if(!fstr.eof())
						strData += " ";
				}
				//DEBUG_OUT("while(!fstr.eof())")
				SetData(strData);
				//DEBUG_OUT("SetData(strData);")
			}
			else
			if(tmpStr == "1.2.1")
			{
				AllCharts.LoadFromCHB(fstr);
				string strData;
				while(!fstr.eof())
				{
					fstr >> tmpStr;
					strData += tmpStr;
					if(!fstr.eof())
						strData += " ";
				}
                strData +="t;";
				SetData(strData);
			}
			else
				cout << "\'" << fname0 << "\' file open error: wrong version of Infinity Plot - " << tmpStr << "\x0D\x0A" << flush;
		}
		else
			cout << "\'" << fname0 << "\' file open error: not Infinity Plot version" << "\x0D\x0A" << flush;
	}
	else
		cout << "\'" << fname0 << "\' file open error: not Infinity Plot" << "\x0D\x0A" << flush;
	fstr.close();
}


void ANM_chart_window::OnHandMove(int dx, int dy)
{
	//DEBUG_OUT("Limit angels")
	if(bIsDrawing)
		return;
	bIsDrawing = true;
	while (m_AngleX < -360.f)
		m_AngleX += 360.f;
	while (m_AngleX > 360.f)
		m_AngleX -= 360.f;
	while (m_AngleY < -360.f)
		m_AngleY += 360.f;
	while (m_AngleY > 360.f)
		m_AngleY -= 360.f;
	//DEBUG_OUT("end - Limit angels")
	//DEBUG_OUT("Set angels")
	double a = fabs(m_AngleX);
	if (90. < a && a < 270. && bLeftBtn)
		dx = -dx;
	if(bLeftBtn)
	{
		m_AngleX += dy/2;
		m_AngleY += dx/2;
	}
	else
	{
		RECT br;
		GetClipBox(chartDC, &br);
		m_ZoomZ += (1.0-m_ZoomZ/(br.right-br.left))*(dx + dy)*5;
	}
	//DEBUG_OUT("end - Set angels")
	Clear();
// если область построения слишком маленькая, то...
	if(cRect.right - cRect.left <= 0 || cRect.bottom - cRect.top <= 0)
		return;
/*	numeric ndx = (Xranges.get())/numeric(cRect.right - cRect.left)*numeric(dx);
	numeric ndy = (Yranges.get())/numeric(cRect.bottom - cRect.top)*numeric(dy);
	Xranges.rmax -= ndx;
	Xranges.rmin -= ndx;
	Yranges.rmax += ndy;
	Yranges.rmin += ndy;
*/

	DrawAllPlots();
	Invalidate();
}

void ANM_chart_window::OnZoom(const RECT& zoomr)
{
	Clear();
// если область построения слишком маленькая, то...
	if(cRect.right - cRect.left <= 0 || cRect.bottom - cRect.top <= 0)
		return;
	range oldX(Xranges), oldY(Yranges);
	numeric dx = (Xranges.get())/numeric(cRect.right - cRect.left);
	numeric dy = (Yranges.get())/numeric(cRect.bottom - cRect.top);
	numeric Xmin = dx*(zoomr.left - cRect.left) + Xranges.rmin;
	Xranges.rmax = dx*(zoomr.right - cRect.left) + Xranges.rmin;
	Xranges.rmin = Xmin;
	numeric Ymin = Yranges.rmin + dy*(cRect.bottom - zoomr.bottom);
	Yranges.rmax = Yranges.rmin + dy*(cRect.bottom - zoomr.top);
	Yranges.rmin = Ymin;
	if(glListInd > 1)
		glDeleteLists(1, glListInd);
	glListInd = 1;
    //DEBUG_OUT(Xranges.get() << " = " << (Xranges.get().to_double()) << " = " << (Xranges.get().evalf()) << "\x0D\x0A";
//		throw runtime_error("Слишком крупный масштаб для выбранной точности вычислений (Digits)");
	DrawAllPlots();
	Invalidate();
}
void ANM_chart_window::OnMouseZoom(int dv)
{
	if(bIsDrawing)
		return;
	RECT zoomr;
	zoomr.left = cRect.left - ((cRect.right - cRect.left)*dv)/50;
	zoomr.top = cRect.top - ((cRect.bottom - cRect.top)*dv)/50;
	zoomr.right = cRect.right + ((cRect.right - cRect.left)*dv)/50;
	zoomr.bottom = cRect.bottom + ((cRect.bottom - cRect.top)*dv)/50;
	OnZoom(zoomr);
}

void ANM_chart_window::EvulateSensSystem(int x, int y)
{
	// если имеем дело с параметрической чувствительностью, то
	// рисуем результат для выбранного момента времени.
	if(ODEsystem)
		if(is_a<ANM_sens_system>(*ODEsystem))
		{
			numeric curx = Xranges.get()*numeric(x - cRect.left)/numeric(cRect.right - cRect.left) + Xranges.rmin;
			//numeric cury = Yranges.get()*numeric(cRect.bottom - y)/numeric(cRect.bottom - cRect.top) + Yranges.rmin;
			((ANM_sens_system*)ODEsystem)->EvulateSensSystem(curx);
		}
}

void ANM_chart_window::OnMouseMove(int x, int y)
{
	static int max_Length = 15;
	if(bIsDrawing)
		return;
	RECT br;
	GetClipBox(chartDC, &br);
	HDC hdc;
	hdc = GetDC(MFCWnd);
	GetClipBox(hdc, &br);
	numeric curx = Xranges.get()*numeric(x - cRect.left)/numeric(cRect.right - cRect.left) + Xranges.rmin;
	numeric cury = Yranges.get()*numeric(cRect.bottom - y)/numeric(cRect.bottom - cRect.top) + Yranges.rmin;
	SelectObject(hdc, font);
// evulating
	string xcoord = "X:";
	if(curx >= 0)
		xcoord += " ";
	xcoord += num_to_string(curx);
	string ycoord = "Y:";
	if(cury >= 0)
		ycoord += " ";
	ycoord += num_to_string(cury);
// clearing
	LOGBRUSH logBrush;
	RECT clrR;
	clrR.top = br.bottom - 5*cHeight/2;
	clrR.left = br.right - max_Length*cWidth+5;
	clrR.bottom = br.bottom;
	clrR.right = br.right;
	logBrush.lbColor = bkColor;
	logBrush.lbStyle = BS_SOLID;
	HBRUSH brush;
	brush = CreateBrushIndirect(&logBrush);
//	FillRect(hdc, &clrR, brush);
	DeleteObject(brush);
//*/
// printing
	//max_Length = (xcoord.length() > ycoord.length())?xcoord.length():ycoord.length() + 1;
	bIsOpenGL = false;
	TextOut3D(hdc, br.right - max_Length*cWidth, br.bottom - 5*cHeight/2, xcoord.c_str(), xcoord.length());
	TextOut3D(hdc, br.right - max_Length*cWidth, br.bottom - 3*cHeight/2, ycoord.c_str(), ycoord.length());
	bIsOpenGL = true;
	ReleaseDC(MFCWnd, hdc);
}

void ANM_chart_window::Evulate()
{
	if(!bPause && ODEsystem && !bSensitivityChart)
	{
		SendMessage(ProgressWnd, WM_KEYDOWN, 2, 0);
		try{
            //DEBUG_OUT("ODEsystem->Evulate();")
			ODEsystem->Evulate();
            //DEBUG_OUT("after ODEsystem->Evulate();")
		} catch (const exception &e) {
			string OutputString = ToString(e.what());
			OutputString += "\nEvulation stopped.";
			COPYDATASTRUCT cd;
			anm_trans tmpTrans;
			strncpy(tmpTrans.str_data, OutputString.c_str(), MAX_TRANS_CHAR);
			cd.dwData = ANM_TRANS;
			cd.lpData = &tmpTrans;
			cd.cbData = sizeof(tmpTrans);
			SendMessage(MFCWnd, WM_COPYDATA, 0, (LPARAM) &cd);
//			MessageBox(MFCWnd, OutputString.c_str(), "Error!", MB_OK | MB_TASKMODAL | MB_SETFOREGROUND);
			bPause = true;
		}
    //DEBUG_OUT("h = " << ODEsystem->h << "\x0D\x0A";
		// посылаем сообщение прогресс бару на обновление
//		if(ProgressWnd)
		SendMessage(ProgressWnd, WM_KEYDOWN, 0, int(((ODEsystem->tN - ODEsystem->InfT)/(ODEsystem->SupT - ODEsystem->InfT)*numeric(100)).to_double()));
		if(bDrawSteps)
		{
			DrawPlots();
		}
		if(ODEsystem->Solutions.bMinus && ODEsystem->tN <= ODEsystem->SupT || (!ODEsystem->Solutions.bMinus) && ODEsystem->tN >= ODEsystem->SupT)
		{
			DrawPlots();
			if(MFCWnd)
			{
				COPYDATASTRUCT cd;
				int tmpint = 1;
				cd.dwData = ANM_RIGHTSIDE;
				cd.lpData = &tmpint;
				cd.cbData = sizeof(tmpint);
				SendMessage(MFCWnd, WM_COPYDATA, 0, (LPARAM) &cd);
//				MessageBox(MFCWnd, "Достигнута правая граница интервала", "Evulation stopped!", MB_OK | MB_TASKMODAL | MB_SETFOREGROUND);
			}
		}
		if(ODEsystem->h <= ODEsystem->Minh)
		{
			DrawPlots();
			if(MFCWnd)
			{
				COPYDATASTRUCT cd;
				int tmpint = 0;
				cd.dwData = ANM_MINSTEP;
				cd.lpData = &tmpint;
				cd.cbData = sizeof(tmpint);
				SendMessage(MFCWnd, WM_COPYDATA, 0, (LPARAM) &cd);
//				MessageBox(MFCWnd, "Шаг стал меньше заданного:\nh < Minh", "Evulation stopped!", MB_OK | MB_TASKMODAL | MB_SETFOREGROUND);
			}
		}
	}
}
//------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------
const string ANM_charts::get_str_data_tosave()
{
	string outstr = "";
	for(size_t k = 0; k < results.size(); k++)
	{
		outstr += ToString(results[k].name) + ";" + ToString(results[k].color) + ";" + ToString(results[k].bDraw) + ";";
		for(size_t i = 0; i < results[k].others.size(); i++)
			outstr += results[k].others[i].name + ";" + ToString(results[k].others[i].color) + ";" + ToString(results[k].others[i].bDraw) + ";";
		for(size_t i = 0; i < results[k].other_rows.size(); i++)
			outstr += results[k].other_rows[i].name + ";" + ToString(results[k].other_rows[i].color) + ";" + ToString(results[k].other_rows[i].bDraw) + ";";
	}
	return outstr;
}

const string ANM_charts::get_str_data()
{
	string outstr = "";
	for(size_t k = 0; k < results.size(); k++)
	{
		outstr += ToString(results[k].name) + ";" + ToString(results[k].color) + ";" + ToString(results[k].bDraw) + ";";
		if(results[k].others.size()+results[k].other_rows.size() > 0)
			outstr += "_Begin;";
		for(size_t i = 0; i < results[k].others.size(); i++)
			outstr += results[k].others[i].name + ";" + ToString(results[k].others[i].color) + ";" + ToString(results[k].others[i].bDraw) + ";";
		for(size_t i = 0; i < results[k].other_rows.size(); i++)
			outstr += results[k].other_rows[i].name + ";" + ToString(results[k].other_rows[i].color) + ";" + ToString(results[k].other_rows[i].bDraw) + ";";
		if(results[k].others.size()+results[k].other_rows.size() > 0)
			outstr += "_End;";
	}
	return outstr;
}

numeric ANM_charts::GetIC(const string &name, const numeric& curx, int n)
{
	for(int k = 0; k < results.size(); k++)
	{
		if(results[k].name == name)
		{
			for(int i = dt.Begin; i < dt.End-1; i++)
				if(dt[i+1] > curx && dt[i] <= curx)
					return results[k].Rk[i].Value(0);
			return results[k].Rk[dt.End].Value(0);
		}
	}
}

void ANM_charts::set_data(const string &name, unsigned long clr, bool aDraw)
{
	for(int k = 0; k < results.size(); k++)
	{
		if(results[k].name == name)
		{
			results[k].color = clr;
			results[k].bDraw = aDraw;
			//DEBUG_OUT("results[" << k << "].name = " << results[k].name)
			curResInd = k;
			curANMCharts = this;
			return;
		}
		if(k == curResInd && curANMCharts == this)
		{
			//DEBUG_OUT("for name = " << name)
			//DEBUG_OUT("curANMCharts = " << curANMCharts)
			for(int i = 0; i < results[k].others.size(); i++)
			{
				if(results[k].others[i].name == name)
				{
					results[k].others[i].bDraw = aDraw;
					results[k].others[i].color = clr;
					return;
				}
			}
			for(int i = 0; i < results[k].other_rows.size(); i++)
			{
				if(results[k].other_rows[i].name == name)
				{
					results[k].other_rows[i].bDraw = aDraw;
					results[k].other_rows[i].color = clr;
					return;
				}
			}
		}
	}
}

const string all_charts::GetStrData()
{
	string outstr;
	outstr = "_Begin;";
	for(size_t k = 0; k < plots.size(); k++)
		outstr += ToString(plots[k].func) + ";" + ToString(plots[k].color) + ";" + ToString(plots[k].bDraw) + ";";
	for(size_t k = 0; k < ANMs.size(); k++)
		outstr += ANMs[k].get_str_data();
	outstr += "_End;";
	return outstr;
}

const string all_charts::GetStrDataToSave()
{
	string outstr;
	outstr = "_Begin;";
	for(size_t k = 0; k < plots.size(); k++)
		outstr += ToString(plots[k].func) + ";" + ToString(plots[k].color) + ";" + ToString(plots[k].bDraw) + ";";
	for(size_t k = 0; k < ANMs.size(); k++)
		outstr += ANMs[k].get_str_data_tosave();
	outstr += "_End;";
	return outstr;
}

void all_charts::SetData(const string &name, unsigned long clr, bool aDraw)
{
	for(int k = 0; k < plots.size(); k++)
		if(ToString(plots[k].func) == name)
		{
			plots[k].color = clr;
			plots[k].bDraw = aDraw;
		}
	for(int k = 0; k < ANMs.size(); k++)
		ANMs[k].set_data(name, clr, aDraw);
}

result* all_charts::GetResultPtr(const string &name)
{
    result *ret = NULL;
	for(int k = 0; k < ANMs.size(); k++)
		if(ret = ANMs[k].GetResultPtr(name))
            break;
    return ret;
}

void chart::DrawPhases(HDC &hdc, ANM_chart_window *prop, defx_array &defx_pts)
{
	if(!bDraw)
		return;
    //DEBUG_OUT("chart::DrawPhases")
	range rX = prop->Xranges;
	range rY = prop->Yranges;
	RECT br = prop->cRect;
	RECT clipBox;
	GetClipBox(hdc, &clipBox);
	numeric dx = (rX.get())/numeric(br.right - br.left);
	numeric dy = (rY.get())/numeric(br.bottom - br.top);
    defx_array::iterator defxIter;
    //DEBUG_OUT("defxIter = defx_pts.begin();")
    defxIter = defx_pts.begin();
    //DEBUG_OUT("ex y = ")
	ex y = func.subs(lst(var), lst(defxIter->curx)).evalf();
    //DEBUG_OUT("if(!is_a<numeric>(y) || dx.is_zero())")
	if(!is_a<numeric>(y) || dx.is_zero())
		return;
	HPEN pen, pOldPen;
	pen = CreatePen(PS_SOLID, (int)prop->LineWidth, (COLORREF)color);
	pOldPen = (HPEN)SelectObject(hdc, pen);

	SetROP2(hdc, prop->Rop2Mode);
	DrawName(hdc, color, prop, ToString(func));

    //DEBUG_OUT("cury = ")
	numeric cury = ex_to<numeric>(y);
	POINT3D p,p1;
	vector <POINT3D> pts, n_pts;	//приближенное решение и точки, нормализованные для области построения

    //DEBUG_OUT("numArray::iterator phaseIter;")
    numArray::iterator phaseIter;
	for(phaseIter = prop->phase_t->values.begin(); phaseIter != prop->phase_t->values.end(); phaseIter++)
	{
		cury = ex_to<numeric>(func.subs(lst(var), lst(defxIter->curx)).evalf());
		p.x = to_integer((*phaseIter - rX.rmin)/dx) + br.left;
	    p.y = br.bottom - to_integer((cury - rY.rmin)/dy);
		p.z = int(prop->m_Trans);
		pts.push_back(p);
        defxIter++;
	}
    //DEBUG_OUT("normalize_pts")
	if(!prop->bOpenGL)
		normalize_pts(pts, n_pts, clipBox);
    if(n_pts.size()||(prop->bOpenGL && pts.size()))
    {
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
			glNewList(prop->glListInd++, GL_COMPILE);
			SetGLColor(color);
			glVertexPointer(3,GL_INT,0,poly);
			glEnableClientState(GL_VERTEX_ARRAY);
			glDrawArrays(GL_LINE_STRIP,0, pts.size());
			glDisableClientState(GL_VERTEX_ARRAY);
			glEndList();
		}
		else
		{
			vector<POINT3D>::pointer mpts = &n_pts[0];
			//DEBUG_OUT("Polyline");
			//Polyline(hdc, mpts, n_pts.size());
		}
    }
	SetROP2(hdc, R2_COPYPEN);
	SelectObject(hdc, pOldPen);
	DeleteObject(pen);
    //DEBUG_OUT("end - chart::DrawPhases")
}

void chart::DrawPlot(HDC &hdc, ANM_chart_window *prop)
{
    //DEBUG_OUT("chart::DrawPlot");
	if(!bDraw)
		return;
	range rX = prop->Xranges;
	range rY = prop->Yranges;
	range rZ = prop->Zranges;
	RECT br = prop->cRect;
	RECT clipBox;
	GetClipBox(hdc, &clipBox);
	numeric dx = (rX.get())/numeric(br.right - br.left);
	if(dx.is_zero())
		return;
	numeric dy = (rY.get())/numeric(br.bottom - br.top);
	numeric dz = (rZ.get())/numeric(br.right - br.left);
	numeric curx = rX.rmin;
	numeric curz = rZ.rmin;
	ex y = 0;
	try{
		y = func.subs(lst(var,var2), lst(curx, curz)).evalf();
	} catch (const exception &e) {
		cout << "Error: DrawPlot " << func << "(" << curx << "): " << e.what() << "\x0D\x0A" << flush;
	}
	if(!is_a<numeric>(y))
		return;
	HPEN pen, pOldPen;
	pen = CreatePen(PS_SOLID, (int)prop->LineWidth, color);
	pOldPen = (HPEN)SelectObject(hdc, pen);

	SetROP2(hdc, prop->Rop2Mode);
	DrawName(hdc, color, prop, ToString(func));

	numeric cury = ex_to<numeric>(y);
	POINT3D p,p1;
	vector <POINT3D> pts, n_pts;	//приближенное решение и точки, нормализованные для области построения
//    p = prop->RealToDisplay(rX.rmin, cury);
	p.x = br.left;
	p.y = br.bottom - to_integer((cury - rY.rmin)/dy);
	p.z = 0;
	//pts.push_back(p);
	// для рисования 3х-мерных графиков нужно знать сколько точек по оси x и по оси z
	int x_pts = 0, temp_pts = 0, z_pts = 0;
	//DEBUG_OUT("rZ.rmax = " << rZ.rmax)
	//DEBUG_OUT("rZ.rmin = " << rZ.rmin)
	//DEBUG_OUT("dz = " << dz)
	while(curz <= rZ.rmax)
	{
		//DEBUG_OUT("curz = " << curz)
		curx = rX.rmin;
		while(curx <= rX.rmax)
		{
			try{
				cury = ex_to<numeric>(func.subs(lst(var,var2), lst(curx, curz)).evalf());
				p.x = to_integer((curx - rX.rmin)/dx) + br.left;
				p.y = br.bottom - to_integer((cury - rY.rmin)/dy);
				p.z = to_integer((curz - rZ.rmin)/dz);
				//DEBUG_OUT("p.z = " << p.z)
				pts.push_back(p);
			} catch (const exception &e) {
				cout << "Error: DrawPlot " << func << "(" << curx << "): " << e.what() << "\x0D\x0A" << flush;
				p.Empty = true;
				pts.push_back(p);
				p.Empty = false;
			}
			temp_pts++;
			curx += dx*prop->dt;
			if (curx > rX.rmax && curx - dx*prop->dt < rX.rmax)
				curx = rX.rmax;
		}
		if(x_pts==0)
			x_pts = temp_pts;
		curz += dz*prop->dt;
		z_pts++;
		if (curz > rZ.rmax && curz - dz*prop->dt < rZ.rmax)
			curz = rZ.rmax;
		if ((dz.is_zero())||!(prop->CurPlot)||!(prop->CurPlot->Is3D))
			curz = rZ.rmax+1;
	}
	//DEBUG_OUT("x_pts = " << x_pts)
	//DEBUG_OUT("z_pts = " << z_pts)
	//DEBUG_OUT("pts.size = " << pts.size())
//	if(!prop->bOpenGL)
//		normalize_pts(pts, n_pts, clipBox);
	int pk = 0;
    if(n_pts.size()||(prop->bOpenGL && pts.size()))
    {
		if(prop->bOpenGL)
		{
			vector<POINT3D>::iterator pIter;
			int poly[z_pts*x_pts*4][3];
			BYTE poly_color[z_pts*x_pts*4][3];
			int i, j, k, n;
			if(prop->CurPlot && prop->CurPlot->Is3D)
				for(int z = 0,i = 0; z < z_pts-1; z++, i++)
				{
					for(int x = 0; x < x_pts-1; x++, i++)
					{
						j = i + x_pts;
						k = j + 1;
						n = i + 1;
						long CurColor = GetPointColor(pts[i], dy, rY, br.bottom);
						poly_color[pk][0] = GetRValue(CurColor);
						poly_color[pk][1] = GetGValue(CurColor);
						poly_color[pk][2] = GetBValue(CurColor);
						poly[pk][0] = pts[i].x;
						poly[pk][1] = clipBox.bottom - pts[i].y;
						poly[pk][2] = pts[i].z;
						pk++;

						//CurColor = GetPointColor(pts[j], dy, rY, br.bottom);
						CurColor = CurColor + RGB(10,10,10);
						poly_color[pk][0] = GetRValue(CurColor);
						poly_color[pk][1] = GetGValue(CurColor);
						poly_color[pk][2] = GetBValue(CurColor);
						poly[pk][0] = pts[j].x;
						poly[pk][1] = clipBox.bottom - pts[j].y;
						poly[pk][2] = pts[j].z;
						pk++;

						//CurColor = GetPointColor(pts[k], dy, rY, br.bottom);
						CurColor = CurColor - RGB(5,5,5);
						poly_color[pk][0] = GetRValue(CurColor);
						poly_color[pk][1] = GetGValue(CurColor);
						poly_color[pk][2] = GetBValue(CurColor);
						poly[pk][0] = pts[k].x;
						poly[pk][1] = clipBox.bottom - pts[k].y;
						poly[pk][2] = pts[k].z;
						pk++;

						//CurColor = GetPointColor(pts[n], dy, rY, br.bottom);
//						CurColor = CurColor + RGB(50,50,50);
						CurColor = CurColor + RGB(5,5,5);
						poly_color[pk][0] = GetRValue(CurColor);
						poly_color[pk][1] = GetGValue(CurColor);
						poly_color[pk][2] = GetBValue(CurColor);
						poly[pk][0] = pts[n].x;
						poly[pk][1] = clipBox.bottom - pts[n].y;
						poly[pk][2] = pts[n].z;
						pk++;
						//DEBUG_OUT("i = " << i << " j = " << j << " k = " << k << " n = " << n)
					}
				}
			else
				for(pIter = pts.begin(); pIter != pts.end(); pIter++)
				{
					poly[k][0] = pIter->x;
					poly[k][1] = clipBox.bottom - pIter->y;
					poly[k][2] = 0;//pIter->z;
					k++;
				}
			glNewList(prop->glListInd++, GL_COMPILE);
			if (prop->OthersMode == 0)
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			if (prop->OthersMode == 1)
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			if (prop->OthersMode == 2)
				glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			glColorPointer(3,GL_UNSIGNED_BYTE,0,poly_color);
			if(prop->CurPlot && prop->CurPlot->Is3D)
				glEnableClientState(GL_COLOR_ARRAY);
			else
				SetGLColor(color);
			glVertexPointer(3,GL_INT,0,poly);
			glEnableClientState(GL_VERTEX_ARRAY);
			if(prop->CurPlot && prop->CurPlot->Is3D)
				glDrawArrays(GL_QUADS,0, pk);
			else
				glDrawArrays(GL_LINE_STRIP,0, pts.size());
			glDisableClientState(GL_VERTEX_ARRAY);
			glEndList();
		}
		else
		{
			//vector<POINT>::pointer mpts = &n_pts[0];
			//DEBUG_OUT("Polyline");
			//Polyline(hdc, mpts, n_pts.size());
		}
    }
	SetROP2(hdc, R2_COPYPEN);
	SelectObject(hdc, pOldPen);
	DeleteObject(pen);
    DEBUG_OUT("end - chart::DrawPlot");
}

void chart::SaveToCHB(ofstream &os)
{
	os << " " << bDraw;
	os << " " << color;
	os << " " << func;
	os << " " << var;
}

void chart::NamesToCVS(ofstream &os)
{
	os << func << ";";
}

void chart::DataToCVS(ofstream &os, const numeric &curx)
{
	os << ex_to<numeric>(func.subs(lst(var), lst(curx)).evalf()) << ";";
}

void chart::LoadFromCHB(ifstream &is)
{
	string tmpStr;
	parser pars;
	is >> bDraw;
	is >> color;
	is >> tmpStr;
	/* debug version
	if(pars.Parse(tmpStr))
        func = pars.GetStatement(0);
	is >> tmpStr;
	if(pars.Parse(tmpStr))
        var = pars.GetStatement(0);
	*/
	func = symbol(tmpStr);
	is >> tmpStr;
	var = symbol(tmpStr);

}

//---------------------------------------------------------------------------
result* ANM_charts::GetResultPtr(const string &name)
{
    vector<result>::iterator resIter;
	for(resIter = results.begin(); resIter != results.end(); resIter++)
	{
		if(resIter->name == name)
            return &(*resIter);
        vector<row_res>::iterator rowIter;
        for(rowIter = resIter->other_rows.begin(); rowIter != resIter->other_rows.end(); rowIter++)
            if(rowIter->name == name)
	            return &(*rowIter);
	}
/*	for(int k = 0; k < results.size(); k++)
	{
		if(results[k].name == name)
            return &(results[k]);
	}
*/    return NULL;
}

const range ANM_charts::GetValueRange(const range &ParamsRange, const RECT &br, ANM_chart_window *prop)
{
	range ret(1,-1);
	if(prop->bSensitivityChart)
	{
		// если момент времени, в который рассчитывалась вариационно-параметрическая система лежит внутри
		// диапазона построения основного графика (строится по времени), то строим графики. Иначе нет
		//if(!(prop->OwnerTimeRange->bound(SensOwnerTime)))
		//	return ret;
	}
	dt.set_bounds(ParamsRange);
	if(dt.Begin > dt.End)
		return ret;
	for(int i = 0; i < results.size(); i++)
		ret = ret.max_range(results[i].GetValueRange(ParamsRange, dt, step, br, prop, results));
	return ret;
}

const range ANM_charts::GetXRange()
{
	range ret(1,-1);
	if(dt.size() == 0)
		return ret;
	ret.rmin = dt[0];
	ret.rmax = dt[0];
	for(int i=0; i < dt.size(); i++)
	{
		if(ret.rmin > dt[i] + dt.last_step*(dt.last_step < 0))
			ret.rmin = dt[i] + dt.last_step*(dt.last_step < 0);
		if(ret.rmax < dt[i] + dt.last_step*(dt.last_step > 0))
			ret.rmax = dt[i] + dt.last_step*(dt.last_step > 0);
	}
	return ret;
}

const range ANM_charts::GetValueRange(const range &ParamsRange, const RECT &br, const result *phase, ANM_chart_window *prop)
{
	range ret(1,-1);
	dt.set_bounds(ParamsRange);
	if(dt.Begin > dt.End)
		return ret;
    vector<result>::iterator resIter;
	for(resIter = results.begin(); resIter != results.end(); resIter++)
        if(&(*resIter) == phase)
        {
            bool pre_bDraw = resIter->bDraw;
            resIter->bDraw = true;
		    ret = ret.max_range(resIter->GetValueRange(ParamsRange, dt, step, br, prop, results));
            resIter->bDraw = pre_bDraw;
        }
		else
		{
			vector<row_res>::iterator rowIter;
			for(rowIter = resIter->other_rows.begin(); rowIter != resIter->other_rows.end(); rowIter++)
				if(&(*rowIter) == phase)
				{
					bool pre_bDraw = rowIter->bDraw;
					rowIter->bDraw = true;
					ret = ret.max_range(rowIter->GetValueRange(ParamsRange, dt, step, br, prop, results));
					rowIter->bDraw = pre_bDraw;
				}
		}
/*	for(int i = 0; i < results.size(); i++)
        if(&(results[i]) == phase)
        {
            bool pre_bDraw = results[i].bDraw;
            results[i].bDraw = true;
		    ret = ret.max_range(results[i].GetValueRange(ParamsRange, dt, step, br, prop));
            results[i].bDraw = pre_bDraw;
        }
		else
*/	return ret;
}

void ANM_charts::SaveToCHB(ofstream &os)
{
// так как size у всех одинаковый, то его можно сохранять только один раз.
	os << " " << dt.size();
	os << " " << dt.Begin;
	os << " " << dt.End;

	for(int i = 0; i < dt.size(); i++)
	{
		os << " " << dt[i];
		os << " " << step[i];
	}
	os << " " << results.size();
	for(int i = 0; i < results.size(); i++)
		results[i].SaveToCHB(os);
}

const unsigned ANM_charts::NamesToCVS(ofstream &os) // сохранение в текстовом формате имен с разделителями (потом можно открыть в Excele как табличку);
{
    os << "t;";
    vector<result>::iterator resIter;
	for(resIter = results.begin(); resIter != results.end(); resIter++)
	{
        if(resIter->bDraw)
    		os << resIter->name << ";" << resIter->name << ".GlobalD;";
        vector<row_res>::iterator rowIter;
        for(rowIter = resIter->other_rows.begin(); rowIter != resIter->other_rows.end(); rowIter++)
            if(rowIter->bDraw)
                os << rowIter->name << ";";
        vector<sub_res>::iterator subIter;
        for(subIter = resIter->others.begin(); subIter != resIter->others.end(); subIter++)
            if(subIter->bDraw)
                os << subIter->name << ";";
	}
    return defx_pts.size();
}

const numeric ANM_charts::DataToCVS(ofstream &os, const int &s) // сохранение в текстовом формате данных (потом можно открыть в Excele как табличку);
{
    if(s >= defx_pts.size())
    {
        os << " ;";
        for(int i = 0; i < results.size(); i++)
            os << " ;";
        return 0;
    }
    os << defx_pts[s].curx.evalf() << ";";
    vector<result>::iterator resIter;
	for(resIter = results.begin(); resIter != results.end(); resIter++)
	{
        if(resIter->bDraw)
        {
    		os << resIter->values[s] << ";";
            numeric GlobD = resIter->GetGlobD(defx_pts[s].i, defx_pts[s].dx_div_step);
    		os << GlobD << ";";
        }
        vector<row_res>::iterator rowIter;
        for(rowIter = resIter->other_rows.begin(); rowIter != resIter->other_rows.end(); rowIter++)
            if(rowIter->bDraw)
                os << rowIter->values[s] << ";";
        vector<sub_res>::iterator subIter;
        for(subIter = resIter->others.begin(); subIter != resIter->others.end(); subIter++)
            if(subIter->bDraw)
                os << subIter->operator [](defx_pts[s].i) << ";";
	}
    return defx_pts[s].curx;
}

void ANM_charts::LoadFromCHB(ifstream &is)
{
	size_t sz;
	string tmpStr;
	is >> sz;
	is >> dt.Begin;
	is >> dt.End;
	dt.clear();
	for(int i = 0; i < sz; i++)
	{
		is >> tmpStr;
		dt.push_back(numeric(tmpStr.c_str()));
		is >> tmpStr;
		step.push_back(numeric(tmpStr.c_str()));
	}
	dt.last_step = step[sz-1];
	is >> sz;
	for(int i = 0; i < sz; i++)
		results.push_back(result());
	for(int i = 0; i < sz; i++)
		results[i].LoadFromCHB(is);
}

void ANM_charts::SetPoints(ANM_chart_window *prop)
{
    //DEBUG_OUT("ANM_charts::SetPoints")
    defx_pts.clear();
	range rX;
    if(prop->phase_t)
		rX = prop->Tranges;
//        rX = prop->defXr;
    else
        rX = prop->Xranges;
	RECT br = prop->cRect;
    dt.set_bounds(rX);
	if(dt.Begin > dt.End)
		return;
	if(prop->bSensitivityChart)
	{
		// если момент времени, в который рассчитывалась вариационно-параметрическая система лежит внутри
		// диапазона построения основного графика (строится по времени), то строим графики. Иначе нет
		//if(!(prop->OwnerTimeRange->bound(SensOwnerTime)))
		//	return;
	}
	numeric dx = (rX.get())/numeric(br.right - br.left);
    numeric dcurx = dx*prop->dt;
    if(dcurx.is_zero())
        return;

	time_array::iterator dtIter, stepIter;

// сначала сформируем точки для времени
    //DEBUG_OUT("i = dt.Begin")
    int i = dt.Begin;
    dtIter = dt.begin() + i;
    stepIter = step.begin() + i;
    numeric x, curx;
    if(*dtIter < rX.rmin)
    {
        x = rX.rmin - *dtIter;
        curx = rX.rmin;
    }
    else
    {
        x = 0;
        curx = *dtIter;
    }
    for(i = dt.Begin; i <= dt.End; i++)
    {
        for(; x < abs(*stepIter); x += dcurx)
        {
            if(*stepIter < 0)
                defx_pts.push_back(defx(i, -x, -x + *dtIter, -x/(*stepIter)));
            else
                defx_pts.push_back(defx(i, x, x + *dtIter, x/(*stepIter)));
            if(x + *dtIter > rX.rmax)
	            break;
        }
        if(*dtIter + *stepIter > rX.rmax)
            break;
		///*setup*/ при разрывах первого рода следующую строчку раскоментировать
        //defx_pts.push_back(defx(i, *stepIter, *dtIter + *stepIter, 1));
        x = 0;
        dtIter++;
        stepIter++;
    }
// а теперь для результатов
    //DEBUG_OUT("results[i].SetPoints")
	for(i = 0; i < results.size(); i++)
        if(&(results[i]) == prop->phase_t)
        {
            bool pre_bDraw = results[i].bDraw;
            results[i].bDraw = true;
		    results[i].SetPoints(prop, defx_pts, results);
            results[i].bDraw = pre_bDraw;
        }
        else
		{
			for(int k = 0; k < results[i].other_rows.size(); k++)
				if(&(results[i].other_rows[k]) == prop->phase_t)
				{
					bool pre_bDraw = results[i].other_rows[k].bDraw;
					results[i].other_rows[k].bDraw = true;
					results[i].other_rows[k].SetPoints(prop, defx_pts, results);
					results[i].other_rows[k].bDraw = pre_bDraw;
				}
		    results[i].SetPoints(prop, defx_pts, results);
		}
    //DEBUG_OUT("end - ANM_charts::SetPoints")
}

void ANM_charts::SetOtherPoints(ANM_chart_window *)
{
}

void ANM_charts::DrawPlots(HDC &hdc, ANM_chart_window *prop)
{
    //DEBUG_OUT("ANM_charts::DrawPlots")
	bool bAllDraw = true;
	if(prop->bSensitivityChart)
	{
		// если момент времени, в который рассчитывалась вариационно-параметрическая система лежит внутри
		// диапазона построения основного графика (строится по времени), то строим графики. Иначе нет
		//if(!(prop->OwnerTimeRange->bound(SensOwnerTime)))
		//	bAllDraw = false;
	}
	range rX = prop->Xranges;
	range rY = prop->Yranges;
	RECT br = prop->cRect;
	RECT clipBox;
	GetClipBox(hdc, &clipBox);
	dt.set_bounds(rX);
	if(dt.Begin > dt.End)
		return;
	numeric dx = (rX.get())/numeric(br.right - br.left);
	numeric dy = (rY.get())/numeric(br.bottom - br.top);
    if(dx.is_zero() || dy.is_zero())
        return;
// прорисовка легенды и определение: нужно ли рисовать хотя бы один из результатов решения текущей системы
	bool bNeedDraw = false;
	for(int i = 0; i < results.size(); i++)
        if(results[i].bDraw)
		{
			if(!bAllDraw)
			{
				results[i].bDraw = false;
				i--;
				continue;
			}
			bNeedDraw = true;
			cout << results[i].GetStrSquare(prop->Xranges, dt, step) << flush;
			//DrawName(hdc, results[i].color, prop, results[i].name);
		}
		else
		{
			for(int k = 0; k < results[i].others.size(); k++)
				if(results[i].others[k].bDraw)
				{
					if(!bAllDraw)
					{
						results[i].others[k].bDraw = false;
						k--;
						continue;
					}
					bNeedDraw = true;
				}
			for(int k = 0; k < results[i].other_rows.size(); k++)
				if(results[i].other_rows[k].bDraw)
				{
					if(!bAllDraw)
					{
						results[i].other_rows[k].bDraw = false;
						k--;
						continue;
					}
					bNeedDraw = true;
				}
		}
// Если для текущей системы рисовать ничего не нужно, то не нужно рисовать даже серые линии вверху экрана
	if(!bNeedDraw)
		return;
// рисование серых линий вверху экрана
	if(prop->bDrawStep)
	{
		POINT3D p;
		p.y = -1;
		int i = dt.Begin;
		int Tone = 210;
		HPEN pen, pOldPen;
		pen = CreatePen(PS_SOLID, (int)prop->LineWidth, RGB(Tone,Tone,Tone));
		pOldPen = (HPEN)SelectObject(hdc, pen);
		SetGLColor(RGB(Tone,Tone,Tone));
		while(i < dt.size()-1 && dt[i] < rX.rmin)
			i++;
		int rTone;
		for(; i <= dt.End; i++)
		{
			rTone = Tone;
			p.x = to_integer((dt[i] - rX.rmin)/dx) + br.left;
			if(p.x != p.y)
				Tone = 210;
			else if(Tone > 0)
				Tone -= 10;
			if(Tone != rTone)
			{
				DeleteObject(pen);
				pen = CreatePen(PS_SOLID, (int)prop->LineWidth, RGB(Tone,Tone,Tone));
				SelectObject(hdc, pen);
				SetGLColor(RGB(Tone,Tone,Tone));
			}
			glBegin(GL_LINE_STRIP);
			MoveTo3D(hdc, p.x, 0);
			LineTo3D(hdc, p.x, 36);
			glEnd();
			p.y = p.x;
		}
		if(i - 1 >= 0)
		{
			rTone = Tone;
			p.x = to_integer((dt[i-1]+step[i-1] - rX.rmin)/dx) + br.left;
			if(p.x != p.y)
				Tone = 210;
			else if(Tone > 0)
				Tone -= 10;
			if(Tone != rTone)
			{
				DeleteObject(pen);
				pen = CreatePen(PS_SOLID, (int)prop->LineWidth, RGB(Tone,Tone,Tone));
				SelectObject(hdc, pen);
				SetGLColor(RGB(Tone,Tone,Tone));
			}
			glBegin(GL_LINE_STRIP);
			MoveTo3D(hdc, p.x, 0);
			LineTo3D(hdc, p.x, 36);
			glEnd();
		}
		SelectObject(hdc, pOldPen);
		DeleteObject(pen);
	}
    //DEBUG_OUT("results[i].DrawPlots")
	for(int i = 0; i < results.size(); i++)
		results[i].DrawPlots(hdc, prop, defx_pts);
    //DEBUG_OUT("end - ANM_charts::DrawPlots")
}

void ANM_charts::DrawPhases(HDC &hdc, ANM_chart_window *prop, result *phase)
{
	for(int i = 0; i < results.size(); i++)
		results[i].DrawPhases(hdc, prop, phase, defx_pts);
	for(int i = 0; i < results.size(); i++)
        if(results[i].bDraw)
    		DrawName(hdc, results[i].color, prop, results[i].name);
}

//---------------------------------------------------------------------------
void all_charts::SaveToCHB(ofstream &os)
{
	os << " " << ANMs.size();
	for(int i = 0; i < ANMs.size(); i++)
		ANMs[i].SaveToCHB(os);
	os << " " << plots.size();
	for(int i = 0; i < plots.size(); i++)
		plots[i].SaveToCHB(os);
}

void all_charts::SaveToCVS(ofstream &os, ANM_chart_window *prop)
{
    unsigned strCount = 0;
	// записываем имена в первую строчку выходного файла
    // сначала для результатов расчета... (здесь же определяем сколько строчек будет в файле)
    for(int i = 0; i < ANMs.size(); i++)
    {
    // в этой функции определяем набор точек по той переменной, по которой шел расчет (они хранятся в массиве dt)
    // там же формируем точки для всех остальных графиков. Это не обязательно те точки, по которым будут строиться графики. Это просто значения x(t), а не x1(x2)
		ANMs[i].SetPoints(prop);
        unsigned strs = ANMs[i].NamesToCVS(os); // функция возвращает кол-во точек, по которым все строилось...
		strCount = max(strs, strCount);
    }
    // теперь для дополнительных функций
	for(int i = 0; i < plots.size(); i++)
		plots[i].NamesToCVS(os);

    os << "\x0D\x0A";
    // построчно записываем данные в файл
    for(int s = 0; s < strCount; s++)
    {
        numeric curt;
        for(int i = 0; i < ANMs.size(); i++)
            curt = ANMs[i].DataToCVS(os, s); // функция возвращает текущий момент времени для каждой из рассматриваемых систем
	    for(int i = 0; i < plots.size(); i++)
    		plots[i].DataToCVS(os, curt);
        os << "\x0D\x0A";
    }
    ANMs[0].DataToCVS(os, strCount);
    os << "\x0D\x0A";
}

void all_charts::LoadFromCHB(ifstream &is)
{
	size_t sz;
	is >> sz;
	for(int i = 0; i < sz; i++)
		ANMs.push_back(ANM_charts());
	for(int i = 0; i < sz; i++)
		ANMs[i].LoadFromCHB(is);
	is >> sz;
	for(int i = 0; i < sz; i++)
		plots.push_back(chart(0,0));
	for(int i = 0; i < sz; i++)
		plots[i].LoadFromCHB(is);
}

ANM_charts* all_charts::push_back(const ANM_charts &var)
{
	ANMs.push_back(var);
	return &(ANMs[ANMs.size() - 1]);
}

void all_charts::DrawPlots(HDC &hdc, ANM_chart_window* prop)
{
	//DEBUG_OUT("all_charts::DrawPlots")
	for(int i = 0; i < ANMs.size(); i++)
    {
        // в этой функции определяем набор точек по той переменной, по которой шел расчет (они хранятся в массиве dt)
        // там же формируем точки для всех остальных графиков. Это не обязательно те точки, по которым будут строиться графики. Это просто значения x(t), а не x1(x2)
        //DEBUG_OUT("ANMs[" << i << "].SetPoints(prop);")
		ANMs[i].SetPoints(prop);
        // Далее, учитывая какая переменная откладывается по оси абсцисс формируем массивы точек для рисования и рисуем это дело
        if(prop->phase_t)
		    ANMs[i].DrawPhases(hdc, prop, prop->phase_t);
        else
   		    ANMs[i].DrawPlots(hdc, prop);
        //DEBUG_OUT("after ANMs[" << i << "].DrawPlots")
    }
    //DEBUG_OUT("plots[i].DrawPlot(hdc, prop);")
	for(int i = 0; i < plots.size(); i++)
        if(prop->phase_t && ANMs.size())
    		plots[i].DrawPhases(hdc, prop, ANMs.begin()->defx_pts);
        else
    		plots[i].DrawPlot(hdc, prop);
	//DEBUG_OUT("end - all_charts::DrawPlots")
}

//---------------------------------------------------------------------------
// WM_EVULATE - сообщение для начала расчета следующего шага
#define WM_EVULATE WM_USER + 101
// WM_CHART_CREATION - чтобы присвоить переменной chart нужное значение
#define WM_CHART_CREATION WM_USER + 102

LRESULT CALLBACK ChartWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    ANM_chart_window *chart = (ANM_chart_window *)GetWindowLong(hWnd,0);
	bool bInvalidate(false);
   	RECT zoomr;
    HANDLE hEvent;
    char buf[200];
    sprintf(buf, "%x", hWnd);
    hEvent = OpenEvent(EVENT_ALL_ACCESS, true, buf);
    ResetEvent(hEvent);
	anm_trans* t;
	switch (message)
	{
		case WM_CHART_CREATION:
			// в параметре передается указатель ANM_chart_window *chart
			SetWindowLong(hWnd, 0, wParam);
            break;
		case WM_COPYDATA:
            {
	            COPYDATASTRUCT *cdin;
	            anm_trans trans_out;
	            COPYDATASTRUCT cd;
                string data;
			    cdin = (COPYDATASTRUCT*) lParam;
			    switch( cdin->dwData )
			    {
				    case ANM_LOADFROMFILE: // OnLoadFromFile
					    data = string(((anm_trans*)(cdin->lpData))->str_data);
    					cout << "Loading from file: " << data << "\x0D\x0A" << flush;
    					chart->OnOpenFromCHB(data);
//				        chart->CreateCompDC();
//				        chart->DrawAllPlots();
   					    break;
				    case ANM_SAVETOFILE: // OnSaveToFile
					    data = string(((anm_trans*)(cdin->lpData))->str_data);
    					cout << "Saving to file: " << data << "\x0D\x0A" << flush;
	    				chart->OnSaveToCHB(data);
   					    break;
				    case ANM_SAVETOCVSFILE: // OnSaveToCVS
					    data = string(((anm_trans*)(cdin->lpData))->str_data);
    					cout << "Saving to csv file: " << data << "\x0D\x0A" << flush;
	    				chart->OnSaveToCVS(data);
   					    break;
				    case ANM_DATANEEDEDBYMFC:	// Data neded by MFC
					    data = chart->GetStrData();
						//DEBUG_OUT("data = " << data)
						if (((anm_trans*)(cdin->lpData))->hWnd != NULL)
						{
							if(data.length()>11*(MAX_TRANS_CHAR-1))
								data = "";
							strncpy(trans_out.str_data, data.c_str(), MAX_TRANS_CHAR - 1);
							if(data.length()>MAX_TRANS_CHAR-1)
								strncpy(trans_out.str_data1, data.c_str() + MAX_TRANS_CHAR - 1, MAX_TRANS_CHAR - 1);
							if(data.length()>2*(MAX_TRANS_CHAR-1))
								strncpy(trans_out.str_data2, data.c_str() + 2*(MAX_TRANS_CHAR - 1), MAX_TRANS_CHAR - 1);
							if(data.length()>3*(MAX_TRANS_CHAR-1))
								strncpy(trans_out.str_data3, data.c_str() + 3*(MAX_TRANS_CHAR - 1), MAX_TRANS_CHAR - 1);
							if(data.length()>4*(MAX_TRANS_CHAR-1))
								strncpy(trans_out.str_data4, data.c_str() + 4*(MAX_TRANS_CHAR - 1), MAX_TRANS_CHAR - 1);
							if(data.length()>5*(MAX_TRANS_CHAR-1))
								strncpy(trans_out.str_data5, data.c_str() + 5*(MAX_TRANS_CHAR - 1), MAX_TRANS_CHAR - 1);
							if(data.length()>6*(MAX_TRANS_CHAR-1))
								strncpy(trans_out.str_data6, data.c_str() + 6*(MAX_TRANS_CHAR - 1), MAX_TRANS_CHAR - 1);
							if(data.length()>7*(MAX_TRANS_CHAR-1))
								strncpy(trans_out.str_data7, data.c_str() + 7*(MAX_TRANS_CHAR - 1), MAX_TRANS_CHAR - 1);
							if(data.length()>8*(MAX_TRANS_CHAR-1))
								strncpy(trans_out.str_data8, data.c_str() + 8*(MAX_TRANS_CHAR - 1), MAX_TRANS_CHAR - 1);
							if(data.length()>9*(MAX_TRANS_CHAR-1))
								strncpy(trans_out.str_data9, data.c_str() + 9*(MAX_TRANS_CHAR - 1), MAX_TRANS_CHAR - 1);
							if(data.length()>10*(MAX_TRANS_CHAR-1))
								strncpy(trans_out.str_data10, data.c_str() + 10*(MAX_TRANS_CHAR - 1), MAX_TRANS_CHAR - 1);
							//DEBUG_OUT("data = " << data)
							//DEBUG_OUT("trans_out.str_data = " << trans_out.str_data)
							//DEBUG_OUT("trans_out.str_data1 = " << trans_out.str_data1)
							//DEBUG_OUT("trans_out.str_data2 = " << trans_out.str_data2)
							cd.dwData = ANM_DATANEEDEDBYMFC;
							cd.lpData = &trans_out;
							cd.cbData = sizeof(trans_out);
							SendMessage(((anm_trans*)(cdin->lpData))->hWnd, WM_COPYDATA, 0, (LPARAM) &cd);
						}
					    break;
			        case ANM_DATASENDEDBYMFC:	// Data sended from MFC
					    data = string(((anm_trans*)(cdin->lpData))->str_data);
						t = (anm_trans*)(cdin->lpData);
						data = t->str_data; data += t->str_data1; data += t->str_data2;
						data += t->str_data3; data += t->str_data4; data += t->str_data5;
						data += t->str_data6; data += t->str_data7; data += t->str_data8;
						data += t->str_data9; data += t->str_data10;
						if(data == "")
							break;
						//DEBUG_OUT("Recieved: " << data);
				        chart->SetData(data);
    //DEBUG_OUT("after SetData" << "\x0D\x0A";
				        chart->DrawAllPlots();
    //DEBUG_OUT("after SetData" << "\x0D\x0A";
				        break;
			    }
			break;
            }
		case WM_PAINT:
    //DEBUG_OUT("message0" << "\x0D\x0A";
			switch(lParam){
			case 14:
				chart->ChangeDrawStepFlag();
				break;
			case 12:
				chart->ChangeDrawStepsFlag();
				break;
			case 11:
				chart->StopEvulating();
				ResetEvent(hEvent);
				break;
			case 10:
				chart->ContinueEvulating();
				if(chart->CanEvulate())
                {
    //DEBUG_OUT("SetEvent 0" << "\x0D\x0A";
					SetEvent(hEvent);
                }
				break;
			case 9:
				chart->OthersMode = wParam;
				break;
			case 8:
				chart->bLegend = !chart->bLegend;
				chart->CreateCompDC();
				break;
			case 7:
				chart->bDrawDomen = !chart->bDrawDomen;
				break;
			case 6:
				chart->Rop2Mode = wParam;
				break;
			case 5:
				if(!chart->GridType)
					chart->GridType = PS_DOT;
				else if(chart->GridType == PS_DOT)
					chart->GridType = PS_SOLID;
				else if(chart->GridType == PS_DOT)
					chart->GridType = 0;
				chart->Clear();
				chart->DrawAllPlots();
				break;
			case 4:
				break;
			case 3:
				bInvalidate = true;
				break;
			case 2:
				chart->ReSize();
				break;
			case 1:
				break;
			};
			if(!bInvalidate)
			{
				    //DEBUG_OUT("Clear" << "\x0D\x0A";
				chart->Clear();
				    //DEBUG_OUT("DrawAllPlots" << "\x0D\x0A";
				chart->DrawAllPlots();
			}
    //DEBUG_OUT("Invalidate" << "\x0D\x0A";
			chart->Invalidate();
			break;
		case WM_KEYDOWN:
            {
                //DEBUG_OUT("message1")
				chart->MFCWnd = (HWND) lParam;
				chart->ContinueEvulating();
                //DEBUG_OUT("chart->CreateCompDC")
				chart->CreateCompDC();
				chart->ProgressWnd = (HWND) wParam;
				chart->DrawPlots();
				SetEvent(hEvent);
                //DEBUG_OUT("after SetEvent(hEvent)")
				break;
            }
		case WM_MOUSEMOVE:
    //DEBUG_OUT("message2" << "\x0D\x0A";
			if(!chart->MFCWnd)
				break;
			switch(wParam){
			case 0:
				bLeftBtn = true;
				chart->OnMouseMove(LOWORD(lParam), HIWORD(lParam));
				break;
			case 1:
				chart->OnHandMove(LOWORD(lParam), HIWORD(lParam));
				break;
			case 2:
				chart->OnHandMove(-LOWORD(lParam), HIWORD(lParam));
				break;
			case 3:
				chart->OnHandMove(-LOWORD(lParam), -HIWORD(lParam));
				break;
			case 4:
				chart->OnHandMove(LOWORD(lParam), -HIWORD(lParam));
				break;
			case 5:
				if(LOWORD(lParam))
					chart->OnMouseZoom(-HIWORD(lParam));
				else
					chart->OnMouseZoom(HIWORD(lParam));
				break;
			case 6:
				chart->EvulateSensSystem(LOWORD(lParam), HIWORD(lParam));
				break;
			case 7:
				// двигаем графики вдоль оси Z
				bLeftBtn = false;
				break;
			}
			break;
		case WM_LBUTTONUP:
    //DEBUG_OUT("message3" << "\x0D\x0A";
			zoomr.left = LOWORD(wParam);
			zoomr.right = LOWORD(lParam);
			zoomr.top = HIWORD(wParam);
			zoomr.bottom = HIWORD(lParam);
			if(zoomr.left >= zoomr.right || zoomr.top >= zoomr.bottom)
				chart->DrawPlots();
			else
				chart->OnZoom(zoomr);
			break;
		case WM_EVULATE:
            //DEBUG_OUT("message5")
            ResetEvent(hEvent);
			if(chart->CanEvulate())
            {
                //DEBUG_OUT("chart->Evulate();")
				chart->Evulate();
            }
            //DEBUG_OUT("if(chart->CanEvulate() && ")
			if(chart->CanEvulate() && !chart->IsPaused())
            {
				SetEvent(hEvent);
            }
            else
                ResetEvent(hEvent);
            //DEBUG_OUT("after if(chart->CanEvulate() && ")
			break;
		case WM_DESTROY:
    //DEBUG_OUT("chart WM_DESTROY" << "\x0D\x0A";
			PostQuitMessage(0);
			break;
		default:
            CloseHandle(hEvent);
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
    CloseHandle(hEvent);
	return 0;
}

DWORD WINAPI EvulateMsgThread( LPVOID lpParam )
{
    HANDLE hEvent;
	HWND hWnd = *((HWND*)(*((DWORD*)lpParam)));
    char buf[200];
    sprintf(buf, "%x", hWnd);
    hEvent = OpenEvent(EVENT_ALL_ACCESS, true, buf);
	while(1)
	{
        ResetEvent(hEvent);
//		while(!PostMessage(hWnd, WM_EVULATE, 0, 0));
		SendMessage(hWnd, WM_EVULATE, 0, 0);
        WaitForSingleObject(hEvent, INFINITE);
	}
    CloseHandle(hEvent);
	return 0;
}

DWORD WINAPI ChartThreadFunc( LPVOID lpParam )
{
    string Exception;
    Exception.erase();
    HANDLE hEvent;
    HANDLE hWinCreationEvent;
	ANM_system* ODEsystem;
	DWORD lParam0 = *((DWORD*)lpParam);
	if(lParam0 != 0)
    {
		if(is_a<ANM_sens_system>(*(ANM_sens_system*)lParam0))
		{
			ODEsystem = (ANM_sens_system*)lParam0;
		}
		else
		{
			ODEsystem = (ANM_system*)lParam0;
		}
		try{
			ODEsystem->Analitic(0);
		} catch (const exception &e) {
            Exception = ToString(e.what());
            cout << "Error: ChartThreadFunc " << e.what() << "\x0D\x0A" << flush;
		}
    }
    else
        Exception = "no ODEsystem";

	ANM_chart_window chart(0, (Exception.empty())?(ODEsystem):(0));
	chart.StopEvulating();

	char buf[200];
	DWORD dwThreadId;
    dwThreadId = GetCurrentThreadId();
    sprintf(buf, "ANM_chart%ld", dwThreadId);
	HWND hWnd;
	hWnd = CreateWindow("ANM_CHART", buf, 0,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_MESSAGE, NULL, NULL, &chart);
    sprintf(buf, "%x", hWnd);
    hEvent = CreateEvent(NULL, false, false, buf);
	if (!hWnd)
		return FALSE;
    //DEBUG_OUT("4W created" << "\x0D\x0A";
//  Посылаем вновь созданному окну сообщение, чтобы установить WinData (Data = &chart)
    ::SendMessage(hWnd, WM_CHART_CREATION, (WPARAM) &chart, 0);
	hWinCreationEvent = OpenEvent(EVENT_ALL_ACCESS, true, "ANM_chartcreation");
	if(hWinCreationEvent == NULL)
		MessageBox(NULL, "Error event open", "Error", MB_OK);
	SetEvent(hWinCreationEvent);
	CloseHandle(hWinCreationEvent);
    //DEBUG_OUT("\"5W created\" event set" << "\x0D\x0A";
    DWORD dwThrdParam = (DWORD)(&hWnd);
	HANDLE hThread;
	hThread = CreateThread(NULL, 0, EvulateMsgThread, &dwThrdParam, 0, &dwThreadId);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
    //DEBUG_OUT(msg.message << " -> " << WM_DESTROY << "\x0D\x0A";
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	CloseHandle(hEvent);
    //DEBUG_OUT("thread end" << "\x0D\x0A";
	return msg.wParam;
}

DWORD WINAPI PlotThreadFunc( LPVOID lpParam )
{
    string Exception;
    Exception.erase();
    HANDLE hEvent;
    HANDLE hWinCreationEvent;
	Plot* aPlot;
	DWORD lParam0 = *((DWORD*)lpParam);
	if(lParam0 != 0)
    {
		aPlot = (Plot*)lParam0;
		try{
			aPlot->Check();
			aPlot->Generate();
		} catch (const exception &e) {
            Exception = ToString(e.what());
            cout << "Error: PlotThreadFunc " << e.what() << "\x0D\x0A" << flush;
		}
    }
    else
        Exception = "no Plot";

	ANM_chart_window PlotWnd(0, 0, (Exception.empty())?(aPlot):(0));
	char buf[200];
	DWORD dwThreadId;
    dwThreadId = GetCurrentThreadId();
    sprintf(buf, "ANM_chart%ld", dwThreadId);
	HWND hWnd;
	hWnd = CreateWindow("ANM_CHART", buf, 0,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_MESSAGE, NULL, NULL, &PlotWnd);
    sprintf(buf, "%x", hWnd);
    hEvent = CreateEvent(NULL, false, false, buf);
	if (!hWnd)
		return FALSE;
    //DEBUG_OUT("4W created" << "\x0D\x0A";
//  Посылаем вновь созданному окну сообщение, чтобы установить WinData (Data = &chart)
    ::SendMessage(hWnd, WM_CHART_CREATION, (WPARAM) &PlotWnd, 0);
	hWinCreationEvent = OpenEvent(EVENT_ALL_ACCESS, true, "ANM_chartcreation");
	if(hWinCreationEvent == NULL)
		MessageBox(NULL, "Error event open", "Error", MB_OK);
	SetEvent(hWinCreationEvent);
	CloseHandle(hWinCreationEvent);
    //DEBUG_OUT("\"5W created\" event set" << "\x0D\x0A";
    DWORD dwThrdParam = (DWORD)(&hWnd);
	HANDLE hThread;
	hThread = CreateThread(NULL, 0, EvulateMsgThread, &dwThrdParam, 0, &dwThreadId);
//    ::SendMessage(hWnd, WM_LBUTTONUP, 0, 0);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
    //DEBUG_OUT(msg.message << " -> " << WM_DESTROY << "\x0D\x0A";
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	CloseHandle(hEvent);
    //DEBUG_OUT("thread end" << "\x0D\x0A";
	return msg.wParam;
}
