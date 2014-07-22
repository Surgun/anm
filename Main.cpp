#include <stdexcept>
#include <sstream>
#include <fstream>

#include "parser\\parser.h"
#include "system\\anm.h"
#include "chart\\chart.h"
#include "trans.h"
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>

#include "main.h"

extern sym_table sym_tab;
extern void lib_init();

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
extern LRESULT CALLBACK ChartWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

std::ostringstream BigLogString;
HWND hLogEdit;
FILE *logstream;

HANDLE DLL_EXPORT Test(const LPCSTR exprtext)
{
    //MessageBoxA(0,  exprtext, "DLL Message", MB_OK | MB_ICONINFORMATION);
    return EvulateExpression(exprtext);
}

HANDLE DLL_EXPORT EvulateExpression(const LPCSTR exprtext)
{
    //DEBUG_OUT("start EvulateExpression");
    bool bOutput;
    string OutStr;
    bOutput = true;
    try
    {
        DEBUG_OUT("exprtext = " << exprtext);
        string instr(exprtext);
        DEBUG_OUT("instr = " << instr);
        parser pars;
        if (pars.Parse(instr))
        {
            for (size_t i = 0; i < pars.StatementCount(); i++)
            {
                ex InputEx = pars.GetStatement(i);
                if (bOutput)
                {
                    OutStr += ToString(InputEx);
                    if (i < pars.StatementCount() - 1)
                        OutStr += "; ";
                }
                // Если распознана команда analysis* , то начинаем анализ или анализ с параметрической чувствительностью или все остальное
                if (is_a<ANM_system>(InputEx))
                {
                    /*			        DWORD dwThreadId, dwThrdParam;
                    					// а здесь уже важно четко определиться, что мы будем делать дальше
                    					if(is_a<ANM_sens_system>(InputEx))
                    						dwThrdParam = (LPARAM) &(ex_to<ANM_sens_system>(InputEx));
                    					else
                    						dwThrdParam = (LPARAM) &(ex_to<ANM_system>(InputEx));
                    	                // для проверки, нет ли ошибок в функции Analysis (если они есть, будет exception)
                                        ((ANM_system*)dwThrdParam)->Analitic(numeric(-1));
                    			        HANDLE hCreationEvent = CreateEvent(NULL, true, false, "ANM_chartcreation");
                    			        ResetEvent(hCreationEvent);
                    			        CreateThread(NULL, 0, ChartThreadFunc, &dwThrdParam, 0, &dwThreadId);
                    			        if (winHWND != NULL)
                    			        {
                    				        trans_out.ChartThread = dwThreadId;
                    				        WaitForSingleObject(hCreationEvent, INFINITE);
                    						if(is_a<ANM_sens_system>(InputEx))
                    						{
                    					        cd.dwData = ANM_NEWSENSCHART;
                    							CloseHandle(hCreationEvent);
                    							hCreationEvent = CreateEvent(NULL, true, false, "ANM_chartcreation");
                    							ResetEvent(hCreationEvent);
                    							CreateThread(NULL, 0, ChartThreadFunc, &dwThrdParam, 0, &dwThreadId);
                    					        trans_out.SensChartThread = dwThreadId;
                    					        WaitForSingleObject(hCreationEvent, INFINITE);
                    						}
                    						else
                    					        cd.dwData = ANM_NEWCHART;
                    				        cd.lpData = &trans_out;
                    				        cd.cbData = sizeof(trans_out);
                    				        SendMessage(winHWND, WM_COPYDATA, 0, (LPARAM) &cd);
                    			        }
                    			        CloseHandle(hCreationEvent);
                    */		        }
                // Если распознана команда plot* , то строим графики
                if (is_a<Plot>(InputEx))
                {
                    /*			        DWORD dwThreadId, dwThrdParam;
                    					dwThrdParam = (LPARAM) &(ex_to<Plot>(InputEx));
                    			        HANDLE hCreationEvent = CreateEvent(NULL, true, false, "ANM_chartcreation");
                    			        ResetEvent(hCreationEvent);
                    			        CreateThread(NULL, 0, PlotThreadFunc, &dwThrdParam, 0, &dwThreadId);
                    			        if (winHWND != NULL)
                    			        {
                    				        trans_out.ChartThread = dwThreadId;
                    				        WaitForSingleObject(hCreationEvent, INFINITE);
                    				        cd.dwData = ANM_NEWCHART;
                    				        cd.lpData = &trans_out;
                    				        cd.cbData = sizeof(trans_out);
                    				        SendMessage(winHWND, WM_COPYDATA, 0, (LPARAM) &cd);
                    			        }
                    			        CloseHandle(hCreationEvent);
                    */		        }
            }
        }
    }
    catch (const exception &e)
    {
        OutStr += "Error: " + ToString(e.what());
//        need_nline = true;
    }
    HANDLE hbuf;
    char *bufptr;
    hbuf = GlobalAlloc(GMEM_MOVEABLE, OutStr.size()+1);
    try
    {
        bufptr = (char *) GlobalLock(hbuf);
        memmove(bufptr, OutStr.c_str() , OutStr.size()+1);
        GlobalUnlock(hbuf);
    }
    catch (const exception &e)
    {
        GlobalFree(hbuf);
        hbuf = INVALID_HANDLE_VALUE;
    }

    return hbuf;
}

void DLL_EXPORT InitKernel()
{
    // определение файла, в который пишется лог
    char flogname[2049];
    // определение каталога, в котором нужно хранить логи.
    GetModuleFileName(NULL, flogname, 1024);
    flogname[strlen(flogname) - strlen("Infinity.exe")] = 0;
    //
    strcat(flogname, "logs\\");
    CreateDirectory(flogname, NULL);
    strcat(flogname, "kernel.log");
    logstream = freopen(flogname, "w", stdout );
    lib_init();
    extern char *hgrevnum;
    extern char *hgdate;
    cout << "ANM kernel loaded. Revision " << hgrevnum << " date " << hgdate << endl << flush;
//    cout << "ANM kernel loaded. " << "\x0D\x0A" << flush;
// Добавляем в таблицу символов некоторые значения "по умолчанию"
    // Количество знаков после запятой
    sym_tab.append(GetSymbol("Digits"), numeric(Digits));
    // Верхняя граница локальной погрешности
    sym_tab.append(GetSymbol("SupLocError"), numeric("1e-5"));
    // Начальное значение погрешности расчета
    sym_tab.append(GetSymbol("StartError"), numeric("0"));
    // Начальное количество первых коэффициентов ряда Тейлора для определения радиуса сходимости
    sym_tab.append(GetSymbol("Forh"), numeric("25"));
    // Флаг способа расчета полной погрешности
    sym_tab.append(GetSymbol("Complicated"), numeric("0"));
    // Левая граница интервала расчета
    sym_tab.append(GetSymbol("InfT"), numeric("0"));
    // Правая граница интервала расчета
    sym_tab.append(GetSymbol("SupT"), numeric("10"));
    // Минимальное значение шага расчета, после которого считается что достигли особой точки
    sym_tab.append(GetSymbol("Minh"), numeric("1e-7"));
// Устанавливаем флаг того, что ядро успешно запущено
//    HANDLE hKernelEvent = OpenEvent(EVENT_ALL_ACCESS, true, "ANM_kernel_loading");
//	SetEvent(hKernelEvent);
//	CloseHandle(hKernelEvent);
}

void DLL_EXPORT DestroyKernel()
{
    if (logstream != NULL)
        fclose(logstream);
}

class ogBMP{
    public:
        ogBMP(HWND NewWnd, HDC pHdc);
        // размер шрифта
        int fHeight;
        string fName;
        int iWidth, iHeight;
        // Handle окна из интерфейса
        HWND hWnd;
        // HDC после BeginPaint
        HDC hDC;
        // Контекст устройства для рисования OpenGL
        HDC hOGDC;
        // Bitmap для рисования OpenGL
        HBITMAP hBmp;
        // Контекст OpenGL
        HGLRC hRC;
        // Высота и ширина символа
        int cW, cH;
        // Цвет фона
        unsigned long bkColor, LineColor, LineWidth;
        float AngleX;	// Rotation angle (around X-axis)
        float AngleY;	// Rotation angle (around Y-axis)
        float Trans;	// Преобразование видовых единиц в GL
        float ZoomZ;  // смещение по оси Z
        // Функция инициализации OpenGL для выбранного окна. Возвращает true, если все хорошо, false, если были ошибки
        bool InitOpenGL(int iNewWidth, int iNewHeight);
        // Копирование нарисованной картинки в указанную
        void CopyBMP(HWND pH, HDC hdc);
        // создание BMP
        void CreateBMP();
        ~ogBMP();
};

// массив средств для рисования в окна интерфейса
typedef vector<ogBMP> ogBMPs;

ogBMP::~ogBMP()
{
}

ogBMP::ogBMP(HWND NewWnd, HDC pHdc)
{
    hWnd = NewWnd;
    hDC = pHdc;
	fName = "Courier New Cyr";
	fHeight = 16;
	bkColor = RGB(255,255,255);
	LineColor = RGB(0,0,0);
	LineWidth = 1;
	AngleX = 0.f;
	AngleY = 0.f;
	ZoomZ = 0.f;
	Trans = 0;
}

void ogBMP::CreateBMP()
{
    void *pBitsDIB(NULL);            // содержимое битмапа
    int cxDIB(iWidth); int cyDIB(iHeight);  // его размеры (например для окна 200х300)
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
    hBmp = CreateDIBSection(
    hOGDC,                  // контекст устройства
    (BITMAPINFO*)&BIH,       // информация о битмапе
    DIB_RGB_COLORS,          // параметры цвета
    &pBitsDIB,               // местоположение буфера (память выделяет система)
    NULL,                    // не привязываемся к отображаемым в память файлам
    0);
}

bool ogBMP::InitOpenGL(int iNewWidth, int iNewHeight)
{
    //HDC hdc = GetDC(hWnd);
    iWidth = iNewWidth;
    iHeight = iNewHeight;
	if(hOGDC)
        DeleteObject(hOGDC);
	hOGDC = CreateCompatibleDC(0);
	HFONT font = CreateFont(
		fHeight,                // nHeight
		0,                         // nWidth
		0,                         // nEscapement
		0,                         // nOrientation
		FW_NORMAL,                 // nWeight FW_NORMAL FW_BOLD
		FALSE,                     // bItalic
		FALSE,                     // bUnderline
		0,                         // cStrikeOut
		ANSI_CHARSET,              // nCharSet
		OUT_DEFAULT_PRECIS,        // nOutPrecision
		CLIP_DEFAULT_PRECIS,       // nClipPrecision
		DEFAULT_QUALITY,           // nQuality
		DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
		fName.c_str());
	CreateBMP();
	SelectObject(hOGDC, hBmp);
	SelectObject(hOGDC, font);
	// Инициализация OpenGL
    PIXELFORMATDESCRIPTOR pfd =	// Structure used to describe the format
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,			// Version
        //PFD_DRAW_TO_BITMAP |	//
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
    hOGDC = hDC;
    //====== Ask to find the nearest compatible pixel-format
    int iD = ChoosePixelFormat(hOGDC, &pfd);
    if ( !iD )
        DEBUG_OUT("ChoosePixelFormat::Error");

        //DEBUG_OUT("ChoosePixelFormat::ok");
    //====== Try to set this format
    if ( !SetPixelFormat (hOGDC, iD, &pfd) )
        DEBUG_OUT("SetPixelFormat::Error");

    //====== Try to create the OpenGL rendering context
    if ( !(hRC = wglCreateContext (hOGDC)))
        DEBUG_OUT("wglCreateContext::Error");

    //====== Try to put it in action
    if ( !wglMakeCurrent (hOGDC, hRC))
        DEBUG_OUT("wglMakeCurrent::Error");
    int m_FontListBase = 1000;
    if ( !wglUseFontBitmaps (hOGDC, 0, 255, m_FontListBase))
        DEBUG_OUT("wglUseFontBitmaps::Error");
    glEnable(GL_DEPTH_TEST);
		// очищаем OpenGL экран
		//glClearColor(GetRValue(bkColor)/255.0,GetGValue(bkColor)/255.0,GetBValue(bkColor)/255.0,1.0f );
		glClearColor(0,0,0,0);
		glClearDepth(1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0,iWidth,0,iHeight, - iWidth - labs(long(ZoomZ)),iWidth + Trans +  labs(long(ZoomZ)));
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef((iWidth)/2,(iHeight)/2,Trans/2);
		glRotatef (AngleX, 1.0f, 0.0f, 0.0f );	// and to rotate
		glRotatef (AngleY, 0.0f, 1.0f, 0.0f );
		glScalef(1.0-ZoomZ/(iWidth),1-ZoomZ/(iWidth),1-ZoomZ/(iWidth));
		glTranslatef(-(iWidth)/2,-(iHeight)/2,-Trans/2);
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
	//ReleaseDC(hWnd, hdc);
	GetCharWidth(hOGDC, 'W', 'W', &cW);
	//DEBUG_OUT("cW = " << cW)
	TEXTMETRIC tm;
	GetTextMetrics(hOGDC, &tm);
	cH = tm.tmHeight + tm.tmExternalLeading;
	DEBUG_OUT("cH  = " << cH)
}

void ogBMP::CopyBMP(HWND pH, HDC hdc)
{

	if (BitBlt(hdc, 0, 0, iWidth, iHeight, hOGDC, 0, 0, SRCCOPY))
        DEBUG_OUT("BitBlt to " << hdc << " ok")
    else
        DEBUG_OUT("BitBlt to " << hdc << " error")
}

ogBMPs allBMPs;

void DLL_EXPORT DrawOGL(const HWND pHdl, const HDC pHdc, const int Width, const int Height)
{
    ogBMPs::iterator bmps;
    bool f = false;
	for(bmps = allBMPs.begin(); bmps != allBMPs.end(); bmps++)
	{
        f = (bmps->hWnd == pHdl);
	    if(f)
            break;
	    bmps++;
	}
	if(!f)
	{
        allBMPs.push_back(ogBMP(pHdl, pHdc));
        bmps = allBMPs.end()-1;
	}
	bmps->InitOpenGL(Width, Height);
        wglMakeCurrent (bmps->hOGDC, bmps->hRC) ;
        glBegin(GL_LINE_STRIP);
        glVertex3f(10,10,0);
        glVertex3f(10,Height - 10,0);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex3f(10,10,0);
        glVertex3f(Width-10,10,0);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex3f(10,10,0);
        glVertex3f(10,10,(Width+Height-20)/2);
        glEnd();
        glPopMatrix;
        SwapBuffers (bmps->hOGDC);
        wglMakeCurrent (NULL, NULL) ;
}

void DLL_EXPORT CopyBMP(const HWND pHdl, const HDC hdc)
{
    ogBMPs::iterator bmps;
    bool f = false;
	for(bmps = allBMPs.begin(); bmps != allBMPs.end(); bmps++)
	{
	    if(bmps->hWnd = pHdl)
	    {
	        f = true;
            break;
	    }
	    bmps++;
	}
	if(!f)
	{
        allBMPs.push_back(ogBMP(pHdl, hdc));
        bmps = allBMPs.end()-1;
	}
	else
        bmps->CopyBMP(pHdl, hdc);
}

extern DWORD WINAPI ChartThreadFunc( LPVOID lpParam );
extern DWORD WINAPI PlotThreadFunc( LPVOID lpParam );

char *GetOutString(char *tok, char *buf, string &OutStr, HWND &winHWND, bool hasnt_dbl_dot)
{
    try
    {
        anm_trans trans_out;
        COPYDATASTRUCT cd;
        string instr(buf);
        //DEBUG_OUT(instr);
        parser pars;
        if (pars.Parse(instr))
        {
            for (size_t i = 0; i < pars.StatementCount(); i++)
            {
                ex InputEx = pars.GetStatement(i);
                if (hasnt_dbl_dot)
                {
                    OutStr += ToString(InputEx);
                    if (i < pars.StatementCount() - 1)
                        OutStr += '\n';
                }
                // Если распознана команда analysis* , то начинаем анализ или анализ с параметрической чувствительностью или все остальное
                if (is_a<ANM_system>(InputEx))
                {
                    DWORD dwThreadId, dwThrdParam;
                    // а здесь уже важно четко определиться, что мы будем делать дальше
                    if (is_a<ANM_sens_system>(InputEx))
                        dwThrdParam = (LPARAM) &(ex_to<ANM_sens_system>(InputEx));
                    else
                        dwThrdParam = (LPARAM) &(ex_to<ANM_system>(InputEx));
                    // для проверки, нет ли ошибок в функции Analysis (если они есть, будет exception)
                    ((ANM_system*)dwThrdParam)->Analitic(numeric(-1));
                    HANDLE hCreationEvent = CreateEvent(NULL, true, false, "ANM_chartcreation");
                    ResetEvent(hCreationEvent);
                    CreateThread(NULL, 0, ChartThreadFunc, &dwThrdParam, 0, &dwThreadId);
                    if (winHWND != NULL)
                    {
                        trans_out.ChartThread = dwThreadId;
                        WaitForSingleObject(hCreationEvent, INFINITE);
                        if (is_a<ANM_sens_system>(InputEx))
                        {
                            cd.dwData = ANM_NEWSENSCHART;
                            CloseHandle(hCreationEvent);
                            hCreationEvent = CreateEvent(NULL, true, false, "ANM_chartcreation");
                            ResetEvent(hCreationEvent);
                            CreateThread(NULL, 0, ChartThreadFunc, &dwThrdParam, 0, &dwThreadId);
                            trans_out.SensChartThread = dwThreadId;
                            WaitForSingleObject(hCreationEvent, INFINITE);
                        }
                        else
                            cd.dwData = ANM_NEWCHART;
                        cd.lpData = &trans_out;
                        cd.cbData = sizeof(trans_out);
                        SendMessage(winHWND, WM_COPYDATA, 0, (LPARAM) &cd);
                    }
                    CloseHandle(hCreationEvent);
                }
                // Если распознана команда plot* , то строим графики
                if (is_a<Plot>(InputEx))
                {
                    DWORD dwThreadId, dwThrdParam;
                    dwThrdParam = (LPARAM) &(ex_to<Plot>(InputEx));
                    HANDLE hCreationEvent = CreateEvent(NULL, true, false, "ANM_chartcreation");
                    ResetEvent(hCreationEvent);
                    CreateThread(NULL, 0, PlotThreadFunc, &dwThrdParam, 0, &dwThreadId);
                    if (winHWND != NULL)
                    {
                        trans_out.ChartThread = dwThreadId;
                        WaitForSingleObject(hCreationEvent, INFINITE);
                        cd.dwData = ANM_NEWCHART;
                        cd.lpData = &trans_out;
                        cd.cbData = sizeof(trans_out);
                        SendMessage(winHWND, WM_COPYDATA, 0, (LPARAM) &cd);
                    }
                    CloseHandle(hCreationEvent);
                }
            }
        }
    }
    catch (const exception &e)
    {
        OutStr += "Error: " + ToString(e.what());
//        need_nline = true;
    }
    return tok;
}

HANDLE hThread;
DWORD WINAPI ThreadFunc( LPVOID lpParam )
{
    DWORD lParam = *((DWORD*)lpParam);
    COPYDATASTRUCT *cdin;
    anm_trans trans_out;
    COPYDATASTRUCT cd;
    HWND winHWND;
    string OutputString;
    cdin = (COPYDATASTRUCT*) lParam;

    string tmpStr = string(((anm_trans*)(cdin->lpData))->str_data);
// Удаляем лишние пробелы, начиная с конца
    string rtmpStr;
    rtmpStr.assign(tmpStr.rbegin(), tmpStr.rend());
    //DEBUG_OUT(rtmpStr);
    while (rtmpStr.length() && (*(rtmpStr.begin()) == ' ' || *(rtmpStr.begin()) == '\t'))
        rtmpStr.erase(0,1);
    tmpStr.assign(rtmpStr.rbegin(), rtmpStr.rend());
    winHWND = ((anm_trans*)(cdin->lpData))->hWnd;
    HANDLE hProcCreationEvent = OpenEvent(EVENT_ALL_ACCESS, true, "ANM_processcreation");
    if (hProcCreationEvent == NULL)
        MessageBox(NULL, "Error Proc event open", "Error", MB_OK);
    SetEvent(hProcCreationEvent);
    CloseHandle(hProcCreationEvent);
    //DEBUG_OUT("Event set")

    char *buf = new char[tmpStr.length()+10], *tok = NULL;
    strcpy(buf, tmpStr.c_str());
    //DEBUG_OUT("buf set")
    GetOutString(tok, buf, OutputString, winHWND, int(tmpStr.find(":")) < 0);
    delete buf;
    if (OutputString.length() == 0)
        OutputString = "\n";
    //DEBUG_OUT("buf deleted")

    if (winHWND != NULL)
    {
        strncpy(trans_out.str_data, OutputString.c_str(), MAX_TRANS_CHAR-1);
        cd.dwData = ANM_TRANS;
        cd.lpData = &trans_out;
        cd.cbData = sizeof(trans_out);
        SendMessage(winHWND, WM_COPYDATA, 0, (LPARAM) &cd);
//		cd.dwData = ANM_TRANS_LASTLINE;
//		SendMessage(winHWND, WM_COPYDATA, 0, (LPARAM) &cd);
        //DEBUG_OUT("data sended" << "\x0D\x0A";
    }
    hThread = NULL;
//	ExitThread(0);
    return 0;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    DWORD dwThreadId, dwThrdParam = lParam;
    COPYDATASTRUCT *cdin;
    anm_trans trans_out;
    COPYDATASTRUCT cd;

    //DEBUG_OUT(message << "\x0D\x0A";
    switch (message)
    {
        //case WM_TIMER:
        //	break;
    case WM_KEYDOWN:
    {
        HANDLE hCreationEvent = CreateEvent(NULL, true, false, "ANM_chartcreation");
        ResetEvent(hCreationEvent);
        //DEBUG_OUT("5C creation" << "\x0D\x0A";
        dwThrdParam = 0;
        CreateThread(NULL, 0, ChartThreadFunc, &dwThrdParam, 0, &dwThreadId);
        if (((anm_trans*)(cdin->lpData))->hWnd != NULL)
        {
            trans_out.ChartThread = dwThreadId;
            cd.dwData = ANM_NEWCHART;
            cd.lpData = &trans_out;
            cd.cbData = sizeof(trans_out);
            WaitForSingleObject(hCreationEvent, INFINITE);
            SendMessage(((anm_trans*)(cdin->lpData))->hWnd, WM_COPYDATA, 0, (LPARAM) &cd);
        }
        CloseHandle(hCreationEvent);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_COPYDATA:
        cdin = (COPYDATASTRUCT*) lParam;
        switch ( cdin->dwData )
        {
        case ANM_TRANS:
        {
            //DEBUG_OUT("1P creation" << "\x0D\x0A";
            HANDLE hProcessEvent = CreateEvent(NULL, true, false, "ANM_processcreation");
            ResetEvent(hProcessEvent);
            hThread = CreateThread(NULL, 0, ThreadFunc, &lParam, 0, &dwThreadId);
            // Check the return value for success.
            if (hThread == NULL)
            {
                if (((anm_trans*)(cdin->lpData))->hWnd != NULL)
                {
                    strncpy(trans_out.str_data, "Internal error 2001: send message to developer", MAX_TRANS_CHAR - 1);
                    cd.dwData = ANM_TRANS;
                    cd.lpData = &trans_out;
                    cd.cbData = sizeof(trans_out);
                    SendMessage(((anm_trans*)(cdin->lpData))->hWnd, WM_COPYDATA, 0, (LPARAM) &cd);
                }
            }
            else
                WaitForSingleObject(hProcessEvent, INFINITE);
            CloseHandle(hProcessEvent);
        }
        break;
        case ANM_OPENCHART:
        {
            //DEBUG_OUT("Chart opening" << "\x0D\x0A";
            HANDLE hCreationEvent = CreateEvent(NULL, true, false, "ANM_chartcreation");
            ResetEvent(hCreationEvent);
            dwThrdParam = 0;
            CreateThread(NULL, 0, ChartThreadFunc, &dwThrdParam, 0, &dwThreadId);
            if (((anm_trans*)(cdin->lpData))->hWnd != NULL)
            {
                trans_out.ChartThread = dwThreadId;
                cd.dwData = ANM_NEWCHART;
                cd.lpData = &trans_out;
                cd.cbData = sizeof(trans_out);
                WaitForSingleObject(hCreationEvent, INFINITE);
                SendMessage(((anm_trans*)(cdin->lpData))->hWnd, WM_COPYDATA, 0, (LPARAM) &cd);
            }
            CloseHandle(hCreationEvent);
        }
        break;
        case ANM_STOP_EVULATING:
            if (hThread != NULL)
                if (TerminateThread( hThread, 1 ))
                    if (((anm_trans*)(cdin->lpData))->hWnd != NULL)
                    {
                        strncpy(trans_out.str_data, "Process interrupted", MAX_TRANS_CHAR - 1);
                        cd.dwData = ANM_TRANS;
                        cd.lpData = &trans_out;
                        cd.cbData = sizeof(trans_out);
                        SendMessage(((anm_trans*)(cdin->lpData))->hWnd, WM_COPYDATA, 0, (LPARAM) &cd);
                    }
            break;
        case ANM_START_INTERFACE:
            if (((anm_trans*)(cdin->lpData))->hWnd != NULL)
                hLogEdit = ((anm_trans*)(cdin->lpData))->hWnd;
            //DEBUG_OUT("Test of run");
            break;
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        // attach to process
        // return FALSE to fail DLL load
        break;

    case DLL_PROCESS_DETACH:
        // detach from process
        break;

    case DLL_THREAD_ATTACH:
        // attach to thread
        break;

    case DLL_THREAD_DETACH:
        // detach from thread
        break;
    }
    return TRUE; // succesful
}
