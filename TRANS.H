#ifndef __ANM_TRANS_H__
#define __ANM_TRANS_H__

#include <windows.h>
#include <vector>

#define HWND_MESSAGE 0
#define ANM_TRANS 1
#define ANM_STOP_EVULATING 2
#define ANM_NEWCHART 3
#define ANM_STRPIPE 4
#define ANM_OPENCHART 5
#define ANM_RIGHTSIDE 6
#define ANM_MINSTEP 7
#define ANM_DATANEEDEDBYMFC 8
#define ANM_DATASENDEDBYMFC 9
#define ANM_LOADFROMFILE 10
#define ANM_SAVETOFILE 11
#define ANM_DESTROYWND 12
#define ANM_SAVETOCVSFILE 13
#define ANM_TRANS_FIRSTLINE 14
#define ANM_TRANS_LASTLINE 15
#define ANM_TRANS_NEXTLINE 16
#define ANM_NEWSENSCHART 17
#define ANM_START_INTERFACE 18
#define ANM_ALLDRAW 19


#define MAX_TRANS_CHAR 3024

using namespace std;
class anm_trans{
public:
	anm_trans(HWND HWnd = NULL, const char *Str_data = "")
	{
		hWnd = HWnd;
		strcpy(str_data,"");strcpy(str_data1,"");strcpy(str_data2,"");strcpy(str_data3,"");strcpy(str_data4,"");
		strcpy(str_data5,"");strcpy(str_data6,"");strcpy(str_data7,"");strcpy(str_data8,"");
		strcpy(str_data9,"");strcpy(str_data10,"");
		if(string(Str_data).length()>11*(MAX_TRANS_CHAR-1))
			return;
		strncpy(str_data, Str_data, MAX_TRANS_CHAR - 1);
		if(string(Str_data).length()>MAX_TRANS_CHAR-1)
			strncpy(str_data1, Str_data + MAX_TRANS_CHAR - 1, MAX_TRANS_CHAR - 1);
		if(string(Str_data).length()>2*(MAX_TRANS_CHAR-1))
			strncpy(str_data2, Str_data + 2*(MAX_TRANS_CHAR - 1), MAX_TRANS_CHAR - 1);
		if(string(Str_data).length()>3*(MAX_TRANS_CHAR-1))
			strncpy(str_data3, Str_data + 3*(MAX_TRANS_CHAR - 1), MAX_TRANS_CHAR - 1);
		if(string(Str_data).length()>4*(MAX_TRANS_CHAR-1))
			strncpy(str_data4, Str_data + 4*(MAX_TRANS_CHAR - 1), MAX_TRANS_CHAR - 1);
		if(string(Str_data).length()>5*(MAX_TRANS_CHAR-1))
			strncpy(str_data5, Str_data + 5*(MAX_TRANS_CHAR - 1), MAX_TRANS_CHAR - 1);
		if(string(Str_data).length()>6*(MAX_TRANS_CHAR-1))
			strncpy(str_data6, Str_data + 6*(MAX_TRANS_CHAR - 1), MAX_TRANS_CHAR - 1);
		if(string(Str_data).length()>7*(MAX_TRANS_CHAR-1))
			strncpy(str_data7, Str_data + 7*(MAX_TRANS_CHAR - 1), MAX_TRANS_CHAR - 1);
		if(string(Str_data).length()>8*(MAX_TRANS_CHAR-1))
			strncpy(str_data8, Str_data + 8*(MAX_TRANS_CHAR - 1), MAX_TRANS_CHAR - 1);
		if(string(Str_data).length()>9*(MAX_TRANS_CHAR-1))
			strncpy(str_data9, Str_data + 9*(MAX_TRANS_CHAR - 1), MAX_TRANS_CHAR - 1);
		if(string(Str_data).length()>10*(MAX_TRANS_CHAR-1))
			strncpy(str_data10, Str_data + 10*(MAX_TRANS_CHAR - 1), MAX_TRANS_CHAR - 1);
//		res = -1;
	}

	HWND hWnd;
//  Инентификатор потока, в котором обрабатываются сообщения для графика
	DWORD ChartThread;
//  Инентификатор потока, в котором обрабатываются сообщения для графиков чувствительности
	DWORD SensChartThread;
	char str_data[MAX_TRANS_CHAR];
	char str_data1[MAX_TRANS_CHAR];
	char str_data2[MAX_TRANS_CHAR];
	char str_data3[MAX_TRANS_CHAR];
	char str_data4[MAX_TRANS_CHAR];
	char str_data5[MAX_TRANS_CHAR];
	char str_data6[MAX_TRANS_CHAR];
	char str_data7[MAX_TRANS_CHAR];
	char str_data8[MAX_TRANS_CHAR];
	char str_data9[MAX_TRANS_CHAR];
	char str_data10[MAX_TRANS_CHAR];
//	int res;
};

#endif // ndef __ANM_TRANS_H__
