#ifndef __ANM_RESULT_H__
#define __ANM_RESULT_H__
/*
	���������� ���������� ���������� ������� � �������� �� �����.
*/

//------------------------------------------- ������������ ��� ���������� ����������� ������� -------------------------
//---------------------------------------------------------------------------------------------------------------------
#include "..\\system\\anm.h"
#include <iostream>
#include <vector>
//#include <math.h>
#include <windows.h>

typedef vector<numeric> numArray;
//--------------------------------------------------------------------
// ����� defx ������ ��� ��������� ������� ����������
// �� ����, ������� ����������� ������� ��������, �� ������� ����� �������� ������, � ����� �������� ������
class defx{
public:
    defx(int aI, const numeric &aDx, const numeric &aCurx, const numeric &aDx_div_step):i(aI),dx(aDx),curx(aCurx),dx_div_step(aDx_div_step){}
    defx(const defx &other):i(other.i),dx(other.dx),curx(other.curx),dx_div_step(other.dx_div_step){}

    int i;  // ����� ����
    numeric dx; // ���������� �� ������� t[n] �� ������� �����
    numeric curx; // t[n] + dx
    numeric dx_div_step;    // dx/step - ��� ��������� ���������� �����������
};

typedef vector<defx> defx_array;
//------------------------------------------------------------------------------------------
//����� range - ������ �������� �������� [rmin;rmax]
class range{
public:
	range(){}
	range(const range &other)
	{
		rmin = other.rmin;
		rmax = other.rmax;
	}
	range(const numeric &aRmin, const numeric &aRmax)
	{
		rmin = aRmin;
		rmax = aRmax;
	}
	const range operator = (const range &other)
	{
		rmin = other.rmin;
		rmax = other.rmax;
		return *this;
	}
	const range max_range(const range &other) const
	{
		range ret;
		if(other.rmin > other.rmax)
			return *this;
		if(rmin > rmax)
			return other;
		ret.rmin = min(rmin, other.rmin);
		ret.rmax = max(rmax, other.rmax);
		return ret;
	}
	const numeric get() const
	{
		return rmax - rmin;
	}

	bool bound(const numeric &val) const
	{
		if(val <= rmax && val >= rmin)
			return true;
		return false;
	}
	numeric rmin;
	numeric rmax;
};

//---------------------------------------------------------------------------------------------------------------------
// ����� �������-"�������", ������ ������ ������� ���������
// � ���� ������� ������������ �������� �������
// �������� ������� �������, � ������� ����������
// ��������� �������� � ��������������� ����� ������� ������� ������������ ��������
class taylor_spline_element : public basic{
	GINAC_DECLARE_REGISTERED_CLASS(taylor_spline_element, basic)
	friend class result;
public:
	//taylor_spline_element();
	taylor_spline_element(numArray &aRk, numeric aGlobalD, numeric aPreGlobalD, bool aInv = false);
	taylor_spline_element(const taylor_spline_element& other);
	taylor_spline_element operator=(const taylor_spline_element& other);
	ex coeff(const ex & s, int n = 1) const;
	numeric Value(const numeric &at);
// ��������� ������� ������� ���������� ������ ������� � ���������� [t0;t1] ��� ���� ���������, ��� � ������ ���� ����������� ���������� (��� ������� ���� = 0)
	const numeric DomenSquareMin(const numeric &t0, const numeric &t1);
// ��������� ������� ������� ���������� ������ ������� � ���������� [t0;t1] ��� ���� ���������, ��� � ������ ���� ����������� ����������� (��� ������� ���� <> 0)
	const numeric DomenSquareMax(const numeric &t0, const numeric &t1);
// ��������� �������� (�������) �� ������������� ������� � ���������� [t0;t1]
	const numeric SolutionSquare(const numeric &t0, const numeric &t1);
// ��������� �������� (�������) �� (������������� �������)^2 � ���������� [t0;t1]
	const numeric Solution_x2_Square(const numeric &t0, const numeric &t1);

	void SaveToCHB(ofstream &os);
	void LoadFromCHB(ifstream &is);
protected:
	numArray Rk;		// ������������ ��������
	numeric GlobalD;	// ���������� �����������, ����������� � ����� ����
	numeric preGlobalD;	// ���������� �����������, ����������� � ����������� (��� �������� ����������� ������������� �������)
	ex x2;				// ��� ���������� ������� �� ������������� �������� � SolutionSquare ����� x^2. ��� ������������� ������ ���������� ���������� ���������� x2 �������� x^2
	bool inversed;		// ���� ����, ��� � Rk ��������� ������������ ��� 1/Value. ��� ���� ����������� ��� Value
};

//---------------------------------------------------------------------------------------------------------------------
// ������ ���������� �������� �������, �� �����. ��� ������ ������� ��������� ����.
class time_array : public numArray{
public:
	time_array(){last_step = 0;}
	void set_bounds(const range &rT);
	numeric last_step;
	size_t Begin;
	size_t End;
};

//---------------------------------------------------------------------------------------------------------------------
// ����� sub_res �������� ��������������� ����������.
// ������ �������������� ����������, ����� ����� �������, ��� ����� ���� ����� ���������
class sub_res : public vector <numeric>{
	friend class result;
	friend class solution;
	friend class ANM_charts;
public:
	sub_res(){bDraw = false;}
	sub_res(const string &aName){name = aName;bDraw = false;}
	sub_res(const sub_res &other)
	{
		color = other.color;
		name = other.name;
		bDraw = other.bDraw;
		assign(other.begin(), other.end());
	}
	const sub_res operator = (const sub_res &other)
	{
		color = other.color;
		name = other.name;
		bDraw = other.bDraw;
		assign(other.begin(), other.end());
		return *this;
	}
	void DrawPlot(HDC &hdc, ANM_chart_window *, defx_array &);
	void DrawPhase(HDC &hdc, ANM_chart_window *, defx_array &, result *phase);
	const range GetValueRange(const range &ParamsRange, const time_array &dTime, const numArray &step);
	void SaveToCHB(ofstream &os);
	void LoadFromCHB(ifstream &is);
protected:
	string name;
	unsigned long color;
	bool bDraw;
};

//---------------------------------------------------------------------------------------------------------------------
// ����� result - ��� �������� ����������� �������. ��� �������� � ���� "������� �������".
// �� ������ ���� ��� ������� ������� ����������� ������-������ ���� X(t) = R[0] + R[1]*t^1 + R[2]*t^2/2! + ... + R[I]*t^I/I!,
// ��� R[i] - ������������ �������, I - ������� ��������������� ��������.
// �����: ��� ������� ���� ������� ���� ������ � ���� ������ �������� ������ �� ����� ���� �������
class row_res;
class result{
	friend class ANM_sensitivity;
	friend class ANM_charts;
    friend class ANM_chart_window;
	friend class solution;
	friend class chart;
	friend class sub_res;
public:
	result();
	result(const result &other);
	const result operator =(const result &other);
	~result(){}
	void append(const result &other);
	int size();

    const numeric GetGlobD(int, const numeric &);
    virtual void SetPoints(ANM_chart_window *, defx_array &, vector <result> &);
	void DrawPlots(HDC &hdc, ANM_chart_window *, defx_array &);
	void DrawPhases(HDC &hdc, ANM_chart_window *, result *phase, defx_array &);
	virtual const range GetValueRange(const range &ParamsRange, const time_array &dTime, const numArray &step, const RECT &br, ANM_chart_window *, vector <result> &);
	void SaveToCHB(ofstream &os);
//	void SaveToCVS(ofstream &os); // ���������� � ��������� ������� � ������������� (����� ����� ������� � Excele ��� ��������);
	void LoadFromCHB(ifstream &is);
// ������� ������� �������, ���������� ������ �������
	const string GetStrSquare(const range &rX, const time_array &dTime, const numArray &step);

	string name;
	vector<taylor_spline_element> Rk;
protected:
// �� ���� �������� ������ ���� ���������� ���������� ���������
// � ��� ���������� ������ ���� ����� ���������� ����� � ������ ANM_charts
	vector<taylor_spline_element> PhiRk;
	vector<taylor_spline_element> PhiPlusRk;
	vector<taylor_spline_element> PhiMinusRk;
	vector<sub_res> others;
	vector<row_res> other_rows;
    numArray values;
	bool bDraw, bSquare;
	unsigned long color;
	unsigned long bkColor;
};

//---------------------------------------------------------------------------------------------------------------------
// ����� row_res �������� ��������������� ����������.
// ������ �������������� ����������, ��� �� ��� � sub_res, �� � ���� "�������� �������"
class row_res : public basic, result{
	GINAC_DECLARE_REGISTERED_CLASS(row_res, basic)
	friend class result;
	friend class solution;
	friend class ANM_charts;
public:
	row_res(const ex& aStat, const ex& aEx_stat = 1, int aMode = 1, const ex& aPrintName = 0);
	row_res(const row_res &a_other);
	const row_res operator = (const row_res &other);
	~row_res(){}

	void append(const row_res &other);
	virtual const range GetValueRange(const range &ParamsRange, const time_array &dTime, const numArray &step, const RECT &br, ANM_chart_window *, vector <result> &);
	virtual void SetPoints(ANM_chart_window *, defx_array &, vector <result> &);
	//void DrawPlot(HDC &hdc, ANM_chart_window *, defx_array &);
	//void DrawPhase(HDC &hdc, ANM_chart_window *, defx_array &, result *phase);
	//const range GetValueRange(const range &ParamsRange, const time_array &dTime, const numArray &step);

	void print(const print_context &c, unsigned level = 0) const;
	size_t nops() const{
		return 1;
	}
	ex op(size_t i) const{
		return stat;
	}
	ex evalf(int level = 0) const;
protected:
	// ���������
	ex stat;
	// ������������� ����������� ��������� (���������� ��� ������� � ������ 1)
	ex ex_stat;
	// ����� ������������� ex_stat - 1 ��� �������
	int ex_mode;
	// ��� ������ �� ������
	ex printName;
	vector<taylor_spline_element> yRk;
};

//---------------------------------------------------------------------------------------------------------------------
// ������ ����������� ������� ����� ������� ���������.
// ���������� ���, ��� ��� ���� ������� ���� ������ dt ���������� �������� ������� - �� ����� ��� �������.
class ANM_charts{
	friend class ANM_system;
	friend class ANM_sensitivity;
	friend class chart;
	friend class all_charts;
public:
	ANM_charts(){}
	~ANM_charts(){}
	ANM_charts(const ANM_charts &other)
	{
		dt.assign(other.dt.begin(), other.dt.end());
		dt.last_step = other.dt.last_step;
		step.assign(other.step.begin(), other.step.end());
		results.assign(other.results.begin(), other.results.end());
	}

	const ANM_charts operator =(const ANM_charts &other)
	{
		dt.assign(other.dt.begin(), other.dt.end());
		dt.last_step = other.dt.last_step;
		step.assign(other.step.begin(), other.step.end());
		results.assign(other.results.begin(), other.results.end());
		return *this;
	}

	void AddResult(const vector<result> &newrs, const numeric &newt, const numeric &newstep)
	{
		bool fappend = false;
		for(int k = 0; k < newrs.size(); k++)
		{
			fappend = false;
			for(int i = 0; i < results.size(); i++)
			{
				if(results[i].name == newrs[k].name)
				{
					results[i].append(newrs[k]);
					fappend = true;
					break;
				}
			}
			if(!fappend && newrs[k].Rk.size() == dt.size()+1)
				results.push_back(newrs[k]);
		}
		dt.push_back(newt);
		dt.last_step = newstep;
		step.push_back(newstep);
	}
	int size()
	{
		int ret = 0;
		for(int i = 0; i < results.size(); i++)
			ret += results[i].size();
		return ret;
	}
	void DrawPlots(HDC &hdc, ANM_chart_window *);
	void DrawPhases(HDC &hdc, ANM_chart_window *, result *phase);
	void SaveToCHB(ofstream &os);
	const unsigned NamesToCVS(ofstream &os); // ���������� � ��������� ������� ���� � ������������� (����� ����� ������� � Excele ��� ��������);
	const numeric DataToCVS(ofstream &os, const int &s); // ���������� � ��������� ������� ������ (����� ����� ������� � Excele ��� ��������);
    void SetPoints(ANM_chart_window *);
    void SetOtherPoints(ANM_chart_window *);
	void LoadFromCHB(ifstream &is);
    result* GetResultPtr(const string &name);
	// ���������� �������� ��������� �������
	const range GetXRange();
	const range GetValueRange(const range &ParamsRange, const RECT &br, ANM_chart_window *);
	const range GetValueRange(const range &ParamsRange, const RECT &br, const result *phase, ANM_chart_window *);
	const string get_str_data();
	const string get_str_data_tosave();
	void set_data(const string &name, unsigned long clr, bool aDraw);
	numeric GetIC(const string &name, const numeric& curx, int n);

	vector <result> results;
protected:
	time_array dt;
	numeric SensOwnerTime; // ��� ���������������� - ������ �������, � ������� �������������� �������
	numArray step;
    defx_array defx_pts;
};

//------------------------------------------------------------------------------------
#endif // ndef __ANM_RESULT_H__
