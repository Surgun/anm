#ifndef ANM_H
#define ANM_H

#include <stdexcept>
#include <sstream>
#include <fstream>
#include <iostream>

#include "..\\FSFlibs.h"

//-------------------------------------------------------------------------------------------
#define RKMODE_EVALf     0x00000001
#define RKMODE_EVAL      0x00000002
#define RKMODE_NORMAL    0x00000004
#define RKMODE_HAS_CURT  0x00000008

// ����� �������� � ����� ����������� ���������� (��� ����, 0-��� ���������� � ������ ����� ���������� ����� ����������� ����������) "anm_new_var"
#define ANMMODE_NEW_VAR  0x00000010
// ��� �������� "�������" �������
#define ANMMODE_EVULATE_SQUARE 0x00000020

class ANM_solutions;
class result;
class sym_table;

//---- ����� ������� ����������:
//---- ����� ����� ��� ����������� ���������� � �������� ���������� ����� �����:
//		1. ������������ R[i] ������������� �������
//		2. ��� ������ ������� ���������� ��� ������������� ������� - ������� ��������������� �������� ��� ������������� �������
//		2�. ��� ������ ������� ���������� ��� ���������� ������� - �����������, � ������� ���� ���������� 2.
//		3. Sign - ���� ������ ������������� �������
class solution;
class Phi{
public:
	Phi();
	~Phi();

	void	Init(solution* aOwner, numeric aSign);
	numeric	evalf();								// ���������� �������� X � ������� ������ �������
	numeric	evalfPhi();								// ���������� �������� Fi � ������� ������ �������
	numeric get_step(int, int tmpNE);				// ������ ����
	int		get_max_r_ind(int &m, int &i, int);		// ���������� ����� ����� ������������� ���� ��������� return value 1 - maxR = max(R[i]) 2 - maxR = max(R[i]/i!) 3 - maxR = max(R[i]*tau^i) tau = pow(R[m]/R[i], i-m)
	numeric	R(int);								//	R[i+1]/(i+1)
	void	set_I(int);								//	������� ��������������� �������� ������� (�� ������ ��������� ������ ���������� ���������� ��������� �����������)

	solution* owner;
	string name;									//	��� ����������

	numeric SupLocError, LocalD;
	int xI;											// ������� ��������������� ��������
	int MaxRInd;									// ����� ��������� ����� �������� �� ������ ���� ����

	int TypeOfMaxR;									// ����� ����� ������������� ���� ��������
	numeric q;										// ���� ��� ������������ 3-� ��������, �� ���������� ��������� ��� ����� ��� ������ ����
	numeric Sign;									// 0 => ������������ ������� -1 => ������ �������; 1 - �������
};

//-------------------------------------------------------------------------------------------
//--------------- unknown_sol class -----------------------
// ����� ������������ ������� ������� ���������. �� ������������� � ������� �������������
// sptr - ��������� �� �������
// n - ����� ����������� (��� �������������� �������������)

class unknown_sol : public basic{
	GINAC_DECLARE_REGISTERED_CLASS(unknown_sol, basic)
	friend class sub_val;
public:
	unknown_sol(solution *aSptr, int aN = 0);
public:
// ������� GiNaC
    ex evalf(int level = 0) const;
	ex coeff(const ex & s, int n = 1) const;
	void print(const print_context &c, unsigned level = 0) const;
	int degree(const ex &s) const;
	ex derivative(const symbol &s) const;
protected:
	solution* sptr;
	int n;
};

//-------------------------------------------------------------------------------------------
//--------------- solution class -----------------------
// ����� ������������� ������� ������� ���������.

class new_var;
class diff_sol;
class phase;
class row_res;
class solution{
	friend class ANM_solutions;
	friend class ANM_system;
	friend class ANM_sens_system;
	friend class ANM_sensitivity;
	friend class Phi;
	friend class sym_rk;
	friend class sym_IC;
	friend class sub_val;
	friend class new_var;
	friend class diff_sol;
    friend class discont;
    friend class phase;
	friend class unknown_sol;
public:
	solution();
	solution(const ex& exB, const ex& exA, const ex aName);
	solution(const solution& other);
	~solution();
	void copy(const solution& other);
	result get_empty_result();
	result get_current_result(bool EvulateGlobal);
// ������� ��� ������ ���
//	������������� �����
	// R[i] - i-� ����������� ���� ������� ��� ����� ������� � ���������� ����
	ex R(int i);
	// R[n][i] - i-� ����������� ���� ������� ��� n-� ������������ ����� ������� � ���������� ����
	ex Rn(int n, int i);
	// R[n][i] - i-� ����������� ���� ������� ��� n-� ������������ ����� ������� � ���������� ���� (������ n ����������� ��� ������)
	ex Rn(int i, ex lambda);
	// i-� ������������� ������� x(0), ���� i = 0, ����� diff(x(t),t,i)(0)
	ex IC(int i);
	void set_ic(const lst &ICs);
//	��������� �����
	virtual void set_ic(const numeric &h);
	// R[i] - i-� ����������� ���� ������� ��� ����� ������� � ��������� ����
	virtual numeric numR(int i);
	numeric numIC(int i);
	virtual numeric get_step(int Forh, bool EvulateGlobal, const numeric &min_step);		// ���������� ���. ��� �� ��������������� q � ������� set_max_r_ind
	void set_I(const numeric& h, const numeric& aEpsilon, bool EvulateGlobal);
	void set_global_delta(const numeric& h, bool EvulateGlobal);
// ��� ���������� �����������
	numeric evalf();

public:
// ������� GiNaC
protected:
// �������, ������������ ������ (��� �������).
	int set_max_r_ind(int &m, int &i, int Forh);
	vector <ex> Rk;				// Rk � ���������� �������
	vector <numeric> numRk[3];	// Rk �����
    vector <diff_sol> ICk;      // ��� ��������� ��������� ������� ����� ������� ����������� �� x
	int TypeOfMaxR;
	int MaxRInd, xI;
	int CurSymR;				// ������ �������� ������������ � ���������� ���� Rk
	int CurNumR;				// ������ �������� ������������ �������� Rk
	int J;
	numeric q;

	ex A;
	ex B;
	ex name;
	numeric preGlobalD, GlobalD, LocalD;	// ������ �����������, ����������� � (n-1)-�� � n-�� ���� � ��������� �����������
	numeric mu_max, mu_min;		// ������������ � ����������� �������� x(tk, mu). ������������ ��� ��������������� ����������������
	numeric kappa;
	Phi phi[3];		//������� ����������
	numeric *step;
	numeric *curt;
	int sign;								// +/- �����������.
	bool needsign;							// ����� �� �������� ���� ���� ������������ ����������� (��. �-� numR)
	int iHold;								// ����� ������������ ��� ���������� sym_rk. ���� iHold < ind �� � ������� sym_rk.eval() ������������ hold, ����� R(i)
	int *NE;// + x -        0 - "+"  1 - x  2 - "-"
//    bool *bMinus;                           // ���� ����, ��� R[i] ����� ������� ��� �������������� ����
	new_var *newVar;						// ��������� ��� ... (����������� �������� ������� ����)

	unsigned long dwMode;
	int ZeroRk;
	vector <sub_val> others;
	vector <row_res> other_rows;
};


//-------------------------------------------------------------------------------------------
//
// ����������� ����� ��� �������� � ����� ����������.
// ����� ���� ��� ������� ������ ��� ���������� ����� �������
// diff_sol - ��� ����������� n-�� ���������� �������, n = 0..inf
// discont  - 1/������� -> ��� ����������� �������� ������� ����
// ?? - �������/(����� ����������� ����������) -> ��� �������� � ����� ����������� ����������
// ?? - 1/diff(�������, t) -> ��� ����������� �������� ������� ���� ����� ����������� (���� �� ������� ������������ ������ 1/�������)
//
// sol - �������, ������� ������������ ��������������
// Rk_ex - ������ ����� Rk - �������� ������������� � ������������
// A � B - ����� �� ���� �������� � A � B �� ������������� ������ ��� = 1 � ������������
//
class new_var : public solution{
public:
	// aSol - �������;
	new_var(solution *aSol);
    new_var(const new_var &other);
    ~new_var(){};
public:
	// ���������� �������� R[i] ��� ���������������� �������
	virtual numeric numR(int i);
    // �� ����, ������ �� ICk ��� new_var ����� ������������ ICs ��� ����������� solution-�������
    // ������� set_ic(h) ������ ���������� ����� ����
  	virtual void set_ICs(vector<diff_sol> &ICs);

public:
	solution* sol;
	ex Rk_ex;
	ex A;
	ex B;
};

//-------------------------------------------------------------------
// ����� diff_sol ��� ����������� n-�� ���������� �������, n = 0..inf
// ���� � ���, ��� ��� ����������� ����������� n-�� ���������� ������� ����� ��������� ����������� ��� diff(x(t),t,n)
class diff_sol : public new_var{
public:
	diff_sol(solution *aSol, int aN);
    diff_sol(const diff_sol &other);
    ~diff_sol(){};
public:
	// ���������� �������� R[i] ��� ���������������� �������
	numeric numR(int i);
    // ���������� nIC +- GlobalD
    numeric numIC();
    void set_ic();
	void set_ic(const numeric &h);
public:
    numeric nIC;// �������� ���������� �������
    int n;      // ������� �����������
};

//-------------------------------------------------------------------
// ����� 1/������� -> ��� ����������� �������� ������� ����
class discont : public new_var{
public:
	discont(solution *aSol);
    // ������� set_ic(h) ������ ���������� ����� ����
    discont(const discont &other);
    ~discont(){};
public:
	// ���������� �������� R[i] ��� ���������������� �������
	numeric numR(int i);
  	void set_ICs(vector<diff_sol> &ICs);
};

//-------------------------------------------------------------------
// ����� �������/(����� ����������� ����������) -> ��� �������� � ����� ����������� ����������
class phase : public new_var{
public:
	phase(solution *aSol, solution *aVal);
    phase(const phase &other);
    ~phase(){};
public:
	// ���������� �������� R[i] ��� ���������������� �������
	numeric numR(int i);
    // ������� set_ic(h) ������ ���������� ����� ����
  	void set_ICs(vector<diff_sol> &ICs);
};

//-------------------------------------------------------------------------------------------
// ����� sub_val "���"-��������.
// ��� ������������� ����� ���� x(t).GlobalD ��� x(t).R[0] � �.�.(������ ������ � ������� evalf())
class sub_val : public basic
{
	GINAC_DECLARE_REGISTERED_CLASS(sub_val, basic)
	friend class solution;
public:
	sub_val(const ex& aName, const ex& aVal, const ex& aPrintName = 0)
	{
		name = aName;
		val = aVal;
		if(aPrintName.is_equal(0))
			printName = name;
		else
			printName = aPrintName;
	}

public:
	void print(const print_context &c, unsigned level = 0) const;
	ex evalf(int level = 0) const;
	size_t nops() const;
	ex op(size_t i) const;
	ex& let_op(size_t i);
	private:
	ex name;
	ex val;
	ex printName;
};

//-------------------------------------------------------------------------------------------
class row_power_z;
class sym_tk : public basic
{
	GINAC_DECLARE_REGISTERED_CLASS(sym_tk, basic)
public:
	sym_tk(row_power_z *aName, const int &aInd);
public:
	void print(const print_context &c, unsigned level = 0) const;
	ex evalf(int level = 0) const;
private:
	row_power_z *name;
	int ind;
};

//-------------------------------------------------------------------------------------------
// ����� - ������ tk, ��� ������������ ��������������� ����������������
class cur_tk : public basic
{
	GINAC_DECLARE_REGISTERED_CLASS(cur_tk, basic)
	friend class ANM_sensitivity;
public:
	cur_tk(numeric *aCur_t);
public:
	void print(const print_context &c, unsigned level = 0) const;
	ex evalf(int level = 0) const;
private:
	numeric *cur_t;
};

//-------------------------------------------------------------------------------------------

class sym_IC;
class sym_rk : public basic
{
	friend class sym_IC;
	GINAC_DECLARE_REGISTERED_CLASS(sym_rk, basic)
public:
	sym_rk(solution* aSptr, int aInd)
	{
		sptr = aSptr;
		ind = aInd;
	}

public:
	void print(const print_context &c, unsigned level = 0) const;
    ex evalf(int level = 0) const;
    ex eval(int level = 0) const;
private:
	solution* sptr;
	int ind;
};

//---------------------------------------------------------------
// ����� sym_IC - ���������� ����������� ���������� ������� x(0)

class sym_IC : public sym_rk{
	GINAC_DECLARE_REGISTERED_CLASS(sym_IC, sym_rk)
public:
	sym_IC(solution *aSptr, int aInd):sym_rk(aSptr, aInd)
	{
	}

	void print(const print_context &c, unsigned level = 0) const
	{
		if(ind)
			c.s << "IC(" << sptr->name << ", " << ind << ")";
		else
			c.s << sptr->name.op(0) << "(0)";
	}

  	ex derivative(const symbol &s) const;
	ex evalf(int level = 0) const;
};
//-------------------------------------------------------------------------------------------
class myprocedure;
class ANM_system;
class ANM_solutions : public vector <solution> {
friend class ANM_system;
public:
	ANM_solutions(){}
public:
	vector<result> get_current_result(bool EvulateGlobal);
	bool is_a_ANM_Solution(const myprocedure&);
	bool is_a_ANM_IC(const myprocedure&);
	solution* get_sptr(const myprocedure&);
public:
    void set_bMinus(bool aBMinus = true);
    numeric get_time_step(const numeric &h);
    numeric get_newVar(const numeric &tN);
    void set_XNames(ANM_system& owner);
	void set(const matrix&, ANM_system&);
	void setICs(const lst& ICs){
		for(int i=0; i<size();i++)
			(*this)[i].set_ic(ICs);
	}
	void setICs(const numeric& h);
	void null();
	numeric get_step(int Forh, bool EvulateGlobal, const numeric &min_step);
	void set_global_delta(const numeric &h, const numeric &aEpsilon, bool EvulateGlobal){
		for(int i=0; i<size();i++)
			(*this)[i].set_I(h, aEpsilon, EvulateGlobal);
		for(int i=0; i<size();i++)
			(*this)[i].set_global_delta(h, EvulateGlobal);
	}

public:
    bool bMinus; // ���� ����, ��� ��� �������������
	// �� ��������� ����������� ���������� - TheTime. ��� �������� � ����� ����������� ���������� TheTime ���������� ��������� ������ �������: ����� ��� dt = 1/diff(����� ����������� ����������)
//	solution *TheTime;
};

//--------------- ANM_system class -----------------------
// �����, ����������� ������������� � ��������� ����� ������. ���� ������ ��� �������.
//
class ANM_charts;
class ANM_system : public basic{
	GINAC_DECLARE_REGISTERED_CLASS(ANM_system, basic)
friend class ANM_solutions;
friend class ANM_chart_window;

public:
	ANM_system(const ex&);
	ANM_system(const lst&);
	virtual void Analitic(const ex&);							// ������������ ����� ������� ��� �������
	void print(const print_context &c, unsigned level = 0) const;
	size_t nops() const{
		return 1;
	}
	ex op(size_t) const{
		return InputData;
	}
	virtual ANM_charts GetEmptyResult();
protected:
	bool CanEvulate();
	virtual void Evulate();								// ���������� �������� �� �����
	// ������������� ������ ���
	void Init_A();
	void Init_G();
	void Init_H();
	void Init_Q();
	// ����������� � �������������� ������� ����� (����� ����� ������������������� ��������� ��������� � ������������ ������� ������� �����)
	void SupSelecting();
	// ����������� �������� �����, ���� ��� ������������
	void SupSelectLinearPart();
	// ����������� �������� �����, ��� i-� ������ � ��� l-� ����������. m_H - ������� ������������ �����������
	void SupSelectLinearPart(int i, int l, int m_H);
	// ���������� ������������ ����������� � ��������� Expl ��� IdName
	int GetMaxDiffLevel(const ex& Expl, const ex& IdName);
	// ���������� ����������� ��� ������������ ����������� m_H � ��������� Expl ��� IdName
	ex GetMaxDiffCoef(const ex& Expl, const ex& IdName, int m_H);
	ex R(int l, int i);
	ex getX(int l);
	// ���������� �������� ����� ODEi ������������ IdName
	virtual ex GetLinearCoeff(const ex& ODEi, const ex& IdName);
	// ���������� ���������� ����� All
	virtual ex GetNotLinearPart(const ex& All);
	// �������� x(t)->R(k) �� x(t).R[k]. �� ���� sub_val �� sym_rk
	virtual ex SubVal2Rk(const ex& ODEi);
	int Is_XorF(const ex& Expl);
	ex GetQElement(const ex& mxElem, const ex& IdName);
protected:
	sym_table analysis_table; // � ���� ������� ������ ��������� ��� ��������������, ������� ������������ ������������� ��� ������� ����������� ���������
	ex InputData;	// ������� ������. ������� 'analysis', ��������� �������������. ����� ��� ������� ������������ ������� �������� sym_tab
	matrix A;
	matrix X;
	matrix SymbX;	// ��� solve
	matrix G;
	matrix F;
	matrix H;
	matrix Q;
	matrix C;
	ANM_solutions Solutions;

	numeric InfT, SupT, tN;					// ������� ��������� ������������ [InfT;SupT] � ������� ������ ������� tN

	int			L;					// ���������� �������
	int			Lf;					// ���������� �����������
	int			Forh;				// ����������� ���������� ������ ������������ ����� ��� ����������� ����
	bool		Speciality;			// ���� ������� ���������� �����������
	int			NE;					// NowEvulating: 0 - ������������ �������; 1 - ������ �������; 2 - ������� �������;

	int			N;				// ������������ ������� p � �������� mainD
	float LineWidth;			// ������� ����� ���������� ��������
	int FontSize;				// ������ ������
	int nWeight;				// �������� ������ FW_NORMAL, FW_BOLD...
	ANM_charts* Result;			// ��������� �� ��������� �����������

	numeric h;				// ��� �������
	numeric SupLocError;	// ������� ������� ��������� ����������� - �������� �������������
	numeric StartError;		// ��������� �������� ����������� ������� (�� ��������� 0) - �������� �������������
	numeric Maxh;			// ����������� ��� �� ����� ��������� ����...(��� ������� �������, ����� �� ������� ���������� ��������)
	numeric Minh;			// ��� ������ ��� ���������� ������ ��� Minh ���������� ������������ (���� Minh ���������� ����, �� ��� ������� � ����������� � ������� 2-�� ����)
	lst SubResults;
	lst ODEs;
	lst ICs;
	lst AdditionalFuncs;
	bool bfInitialaized;
};

//--------------- ANM_sensitivity class -----------------------
// �����, ����������� �������� ������������ ��������������� ���������������� ��� �������
// ���.
class all_charts;
class ANM_sensitivity : public ANM_system{
	GINAC_DECLARE_REGISTERED_CLASS(ANM_sensitivity, ANM_system)
friend class ANM_solutions;
friend class ANM_sens_system;

public:
	ANM_sensitivity(const ex& aInputData, const ex& aMu, numeric *aTime);
	void Calculate(const lst& aICs, all_charts*); // ���������� ������, � ������� ����� �������
	void Analitic();							// ������������ ����� ������� ��� �������
//	void copy(const ANM_sensitivity &other);
protected:
	void Evulate();								// ���������� �������� �� �����
//	ex GetLinearCoeff(const ex& ODEi, const ex& IdName);
//	ex GetNotLinearPart(const ex& All);
protected:
	ex mu; // ��������, ������������ �������� ���������� ������
	cur_tk Time_k;
	bool bSaveResult;
	int StepCntr;
};

//--------------- ANM_sens_system class -----------------------
// �����, ����������� ������ ��� � ������ ������������ ��������������� ����������������
//
class range;
class ANM_sens_system : public ANM_system{
	GINAC_DECLARE_REGISTERED_CLASS(ANM_sens_system, ANM_system)
friend class ANM_solutions;
friend class ANM_chart_window;
public:
	ANM_sens_system(const ex&);
	ANM_sens_system(const lst&);
	ANM_charts GetEmptyResult();
	void Analitic(const ex& aFlags);
protected:
	void Evulate();								// ���������� �������� �� �����
	void EvulateSensSystem(const numeric& curx);
protected:
	ANM_sensitivity sens;
//	numeric delta_mu;
	all_charts* AllResults;			// ��������� �� ��������� �����������
	range* PlotsTimeRange;			// ��������� �� XRange ��������
};

//--------------- Plot class -----------------------
// ����� ��� ��������� ��������. ��� 2D ��� � 3D
//
class Plot : public basic{
	GINAC_DECLARE_REGISTERED_CLASS(Plot, basic)
friend class ANM_chart_window;

public:
	Plot(const ex&);
	Plot(const lst&);
	void print(const print_context &c, unsigned level = 0) const;
	size_t nops() const{
		return 1;
	}
	ex op(size_t) const{
		return InputData;
	}
	void Check();
	void Generate();
public:
	float LineWidth;			// ������� ����� ���������� ��������
	int FontSize;				// ������ ������
	int nWeight;				// �������� ������ FW_NORMAL, FW_BOLD...
	bool Is3D;
protected:
	ex InputData;	// ������� ������. ������� 'plot', ��������� �������������.
	lst Funcs; // ������� - �� ����� ���� ��������� � ���� {sin(t), cos(t)} ��� ���� � ���� sin(t)
	ex x, y; // ����������, ������� ���������� x = rX.min..rX.max; y = rY.min..rY.max;
	numeric Xmin;
	numeric Xmax;
	numeric Ymin;
	numeric Ymax;
	numeric Zmin;
	numeric Zmax;
};

#endif // ndef ANM_H
