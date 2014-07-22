#ifndef __ANM_CHART_H__
#define __ANM_CHART_H__

#include "..\\result\\result.h"
#include <float.h>

extern symbol GetSymbol(const string&);
extern ex GetExpression(const ex&);
extern ex ExpandWithSymTab(ex&);
template< class T > string ToString(const T & t);

// Структура для хранения 3х-мерной точки. Флаг Empty - если точка пустая и ее выводить не нужно (например, в этой точке разыв)
typedef struct tagPOINT3D
{
    LONG  x;
    LONG  y;
    LONG  z;
	bool Empty;
} POINT3D;

//string num_to_string(const numeric& x, int MaxN = 0);

//------------------------------------------------------------------------------------------
//const unsigned long Colors[] = {RGB(255,128,128), RGB(255,0,0), RGB(128,64,64), RGB(128,0,0), RGB(64,0,0), RGB(255,255,0), RGB(255,128,64), RGB(128,128,0), RGB(0,128,0), RGB(0,64,0), RGB(0,255,255), RGB(0,0,128), RGB(0,0,255), RGB(128,128,255), RGB(64,0,64), RGB(128,0,255), RGB(255,0,255)};
const unsigned long Colors[] = {RGB(128,64,64), //0
								RGB(255,128,64),//1
								RGB(128,0,0),   //2
								RGB(128,255,0), //3
								RGB(128,128,0), //4
								RGB(0,128,128), //5
								RGB(0,64,128),  //6
								RGB(128,128,255), //7
								RGB(128,64,0), //8
								RGB(255,0,255), //9
								RGB(255,128,192), //10
								RGB(0,64,64), //11
								RGB(0,64,0), //12
								RGB(0,0,128), //13
								RGB(64,0,64), //14
								RGB(128,128,128), //15
								RGB(128,128,0),//16
								RGB(128,0,128), //17
								RGB(0,0,160),//18
								RGB(0,128,0),//19
								RGB(64,0,0),//20
								RGB(0,255,0),//21
								0};

extern int ColorInd;
class chart{
public:
	friend class all_charts;
	chart(const ex &aFunc, const ex &aVar, const ex &aVar2 = 0)
	{
		//DEBUG_OUT("func = " << aFunc << "(" << aVar << ", " << aVar2 << ")")
		bDraw = true;
		func = aFunc;
		var = aVar;
		var2 = aVar2;
		color = Colors[ColorInd++];
		if(Colors[ColorInd] == 0)
			ColorInd = 0;
	}
	chart(const chart &other)
	{
		color = other.color;
		func = other.func;
		var = other.var;
		var2 = other.var2;
		bDraw = other.bDraw;
	}
	const chart operator=(const chart &other)
	{
		color = other.color;
		func = other.func;
		var = other.var;
		var2 = other.var2;
		bDraw = other.bDraw;
	}

	const range GetValueRange(const range &rX, const RECT &br, ANM_chart_window *);
	void DrawPlot(HDC &hdc, ANM_chart_window*);
	void DrawPhases(HDC &hdc, ANM_chart_window*, defx_array &);
	void SaveToCHB(ofstream &os);
	void NamesToCVS(ofstream &os);
	void DataToCVS(ofstream &os, const numeric &);
	void LoadFromCHB(ifstream &is);
protected:
	ex func;
	ex var;
	ex var2;
	unsigned long color;
	bool bDraw;
};

//------------------------------------------------------------------------------------------
class all_charts{
public:
	all_charts(){}

	void push_back(const ex &func, const ex &var, const ex &var2 = 0)
	{
		plots.push_back(chart(func, var, var2));
	}
	ANM_charts* push_back(const ANM_charts &var);
	int size(){
		int res = int(plots.size());
		for(int i = 0; i < ANMs.size(); i++)
			res += ANMs[i].size();
		return res;
	}
	const string GetStrData();
	const string GetStrDataToSave();
	// возвращает диапазон изменения времени
	const range GetXRange();
	const range GetValueRange(const range &ParamsRange, const RECT &br, ANM_chart_window *);
	const range GetValueRange(const range &ParamsRange, const RECT &br, const result *phase, ANM_chart_window *);
	void DrawPlots(HDC &hdc, ANM_chart_window *);
	void SetData(const string &name, unsigned long clr, bool aDraw);
	void SaveToCHB(ofstream &os);
	void SaveToCVS(ofstream &os, ANM_chart_window *); // сохранение в текстовом формате с разделителями (потом можно открыть в Excele как табличку);
	void LoadFromCHB(ifstream &is);
    result* GetResultPtr(const string &name);
	vector <ANM_charts> ANMs;
protected:
	vector <chart> plots;
};

//------------------------------------------------------------------------------------------
class ANM_chart_window{
//ctors
public:
	ANM_chart_window(HWND aMFCWnd, ANM_system *aODEsystem = NULL, Plot *aPlot = NULL);
//DC functions
public:
	void CreateCompDC();
	void ReSize();
	void DrawAllPlots();	// рисование всех координат относительно времени
	void DrawPlots();		// тут вызывается DrawAllPlots();
	void DrawAllPhases();	// рисование всех координат относительно другой координаты
	void DrawPhases();		// тут вызывается DrawAllPhases();
	void OnSaveToCHB(string fname); // сохранение всех данных в текстовом формате (непонятном для чела), с возможностью восстановления потом в виде графиков
	void OnSaveToCVS(string fname); // сохранение в текстовом формате с разделителями (потом можно открыть в Excele как табличку);
	void OnOpenFromCHB(string fname);
	void OnMouseMove(int x, int y);
	void EvulateSensSystem(int x, int y);
	void OnHandMove(int dx, int dy);
	void OnZoom(const RECT&);
	void OnMouseZoom(int dv);
	void Clear();
	void Invalidate();
	void SetData(const string &inStr);
	void CreateBMP(int width, int height);
	void DrawState(const string &sState);
//ANM_system functions
public:
	void Evulate();
	void StopEvulating(){bPause = true;}
	void ContinueEvulating(){bPause = false;}
	bool IsPaused(){return bPause;}
	void ChangeDrawStepsFlag(){bDrawSteps = !bDrawSteps;}
	void ChangeDrawStepFlag(){bDrawStep = !bDrawStep;CreateCompDC();}
	bool CanEvulate()
	{
		return (!bSensitivityChart && ODEsystem && ODEsystem->CanEvulate());
	}
//Wnd data and functions
public:
	string GetStrData();
	string GetStrDataToSave();
	HWND MFCWnd;
	HWND ProgressWnd;
	HDC chartDC;
	HBITMAP chartBitmap;
	bool bPause;
// область построения
public:
	bool bDrawStep;		// прорисовка динамики шага
	HGLRC	m_hRC;
	bool bOpenGL, bIsDrawing; // флаг использования OpenGL для отрисовки элементов
	float LineWidth; // Толщина линий
	int glListInd; // индекс текущего списка (изменяется при прорисовке графиков)
	HFONT font;
	RECT pRect; // plot Rect
	RECT cRect; // client Rect
	int cWidth;
	int cHeight;
	range Xranges, defXr, Tranges; // Xranges - диапазон по оси абсцисс, defXr - диапазон по умолчанию, Tranges - диапазон по времени (для фазовых траекторий)
	range* OwnerTimeRange; // Для построения графиков - параметрической чувствительности (для определения временных интервалов построения)
	range Yranges; // диапазон по оси ординат
	range Zranges; // диапазон по оси Z
	float	m_AngleX;	// Rotation angle (around X-axis)
	float m_AngleY;	// Rotation angle (around Y-axis)
	float m_Trans;	// Преобразование видовых единиц в GL
	float m_ZoomZ;  // смещение по оси Z
	bool bSensitivityChart; // флаг того, что график строится для исследования чувствительности
	int CurPlotInd;
	int MaxNameLength;
	bool bLegend;			// флаг прорисовки легенды
	unsigned long bkColor;	// цвет фона
	unsigned long LineColor;
	numeric dt;		// шаг дискретизации при построении решений
	unsigned long Rop2Mode;
	bool bDrawDomen;	// флаг рисования области, содержащей точное решение
	int OthersMode;		// Режим вывода дополнительных данных: 0 - окружности, 1 - Линии, 2 - и то и другое
	string fontName;
	int fontHeight;
	int nWeight;

    // для построения фазовых траекторий - указатель на переменную, которая будет откладываться по оси абсцисс (X)
    result *phase_t;
    // для построения фазовых траекторий 3D - указатель на переменную, которая будет откладываться по оси аппликат (Z)
	result *phase_z;
// сетка
public:
	int m, n;
	unsigned long GridType;
	unsigned long GridColor;
	all_charts AllCharts;

// построение графиков
protected:
	void get_touchs(numeric coord1, numeric coord2, const int max_touch_count, numArray& touchs);
	int test_touch_arr(numeric coord1, numeric coord2, numeric touch);
	void get_touch_arr(numeric coord1, numeric coord2, numeric touch, numArray& coords);
	void get_lim_coords(numeric coord1, numeric coord2, numeric touch, numeric &start_coord, numeric &end_coord);
	void DrawAxis();
	void DrawGrid();
    void SetRanges(const range rX, const range rY, bool brX, bool brY);
    POINT RealToDisplay(int i, const numeric &x, const numeric &cury);

public:
	Plot *CurPlot;
//ANM_system data
protected:
	ANM_system *ODEsystem;
	bool bDrawSteps;		// пошаговое обновление графиков
	int unvisible;			// количество невидимых графиков
};

#endif // ndef __ANM_CHART_H__
