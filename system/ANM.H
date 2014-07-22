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

// режим перехода к новой независимой переменной (при этом, 0-вая переменная в списке иксов становится новой независимой переменной) "anm_new_var"
#define ANMMODE_NEW_VAR  0x00000010
// для подсчета "средней" площади
#define ANMMODE_EVULATE_SQUARE 0x00000020

class ANM_solutions;
class result;
class sym_table;

//---- Класс функции приращения:
//---- Чтобы найти всю необходимую информацию о функциях приращения нужно знать:
//		1. Коэффициенты R[i] приближенного решения
//		2. Для поиска функции приращения для приближенного решения - Порядок ограничивающего полинома для приближенного решения
//		2а. Для поиска функции приращения для сдвинутого решения - Погрешность, с которой была подсчитана 2.
//		3. Sign - знак сдвига предначальных условий
class solution;
class Phi{
public:
	Phi();
	~Phi();

	void	Init(solution* aOwner, numeric aSign);
	numeric	evalf();								// Мгновенное значение X в текущий момент времени
	numeric	evalfPhi();								// Мгновенное значение Fi в текущий момент времени
	numeric get_step(int, int tmpNE);				// расчет шага
	int		get_max_r_ind(int &m, int &i, int);		// Определить среди каких коэффициентов ести максимумы return value 1 - maxR = max(R[i]) 2 - maxR = max(R[i]/i!) 3 - maxR = max(R[i]*tau^i) tau = pow(R[m]/R[i], i-m)
	numeric	R(int);								//	R[i+1]/(i+1)
	void	set_I(int);								//	степень ограничивающего полинома Тейлора (на основе заданного уровня предельной абсолютной локальной погрешности)

	solution* owner;
	string name;									//	для интерфейса

	numeric SupLocError, LocalD;
	int xI;											// порядок ограничивающего полинома
	int MaxRInd;									// номер максимума среди мажорант на каждом шаге своя

	int TypeOfMaxR;									// среди каких коэффициентов есть максимум
	numeric q;										// если шаг определяется 3-м способом, то необходимо учитывать это число при выборе шага
	numeric Sign;									// 0 => приближенное решение -1 => нижняя граница; 1 - верхняя
};

//-------------------------------------------------------------------------------------------
//--------------- unknown_sol class -----------------------
// Класс неизвестного решения системы уравнений. Он подставляется в матрицу нелинейностей
// sptr - указатель на решение
// n - номер производной (для функциональных нелинейностей)

class unknown_sol : public basic{
	GINAC_DECLARE_REGISTERED_CLASS(unknown_sol, basic)
	friend class sub_val;
public:
	unknown_sol(solution *aSptr, int aN = 0);
public:
// функции GiNaC
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
// Класс приближенного решения системы уравнений.

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
// Функции для работы АЧМ
//	Аналитическая часть
	// R[i] - i-й коэффициент ряда Тейлора для этого решения в символьном виде
	ex R(int i);
	// R[n][i] - i-й коэффициент ряда Тейлора для n-й составляющей этого решения в символьном виде
	ex Rn(int n, int i);
	// R[n][i] - i-й коэффициент ряда Тейлора для n-й составляющей этого решения в символьном виде (вместо n указывается сам корень)
	ex Rn(int i, ex lambda);
	// i-е предначальное условие x(0), если i = 0, иначе diff(x(t),t,i)(0)
	ex IC(int i);
	void set_ic(const lst &ICs);
//	Численная часть
	virtual void set_ic(const numeric &h);
	// R[i] - i-й коэффициент ряда Тейлора для этого решения в численном виде
	virtual numeric numR(int i);
	numeric numIC(int i);
	virtual numeric get_step(int Forh, bool EvulateGlobal, const numeric &min_step);		// возвращает шаг. Там же устанавливается q в функции set_max_r_ind
	void set_I(const numeric& h, const numeric& aEpsilon, bool EvulateGlobal);
	void set_global_delta(const numeric& h, bool EvulateGlobal);
// для сохранения результатов
	numeric evalf();

public:
// функции GiNaC
protected:
// Функции, используемые внутри (при расчете).
	int set_max_r_ind(int &m, int &i, int Forh);
	vector <ex> Rk;				// Rk в символьном формате
	vector <numeric> numRk[3];	// Rk числа
    vector <diff_sol> ICk;      // для остальных начальных условий нужно считать производные от x
	int TypeOfMaxR;
	int MaxRInd, xI;
	int CurSymR;				// индекс текущего вычисляемого в символьном виде Rk
	int CurNumR;				// индекс текущего вычисляемого численно Rk
	int J;
	numeric q;

	ex A;
	ex B;
	ex name;
	numeric preGlobalD, GlobalD, LocalD;	// полная погрешность, накопленная к (n-1)-му и n-му шагу и локальная погрешность
	numeric mu_max, mu_min;		// максимальное и минимальное значения x(tk, mu). Используется при параметрической чувствительности
	numeric kappa;
	Phi phi[3];		//функции приращения
	numeric *step;
	numeric *curt;
	int sign;								// +/- детерминант.
	bool needsign;							// нужно ли выбирать знак если используется детерминант (см. ф-ю numR)
	int iHold;								// номер коэффициента при вычислении sym_rk. Если iHold < ind то в функции sym_rk.eval() возвращается hold, иначе R(i)
	int *NE;// + x -        0 - "+"  1 - x  2 - "-"
//    bool *bMinus;                           // флаг того, что R[i] нужно считать для отрицательного шага
	new_var *newVar;						// экземпляр для ... (преодоления разрывов второго рода)

	unsigned long dwMode;
	int ZeroRk;
	vector <sub_val> others;
	vector <row_res> other_rows;
};


//-------------------------------------------------------------------------------------------
//
// Виртуальный класс для перехода к новой переменной.
// Далее идут его потомки каждый для реализации своей функции
// diff_sol - для определения n-го начального условия, n = 0..inf
// discont  - 1/решение -> для преодоления разрывов второго рода
// ?? - решение/(новая независимая переменная) -> для перехода к новой независимой переменной
// ?? - 1/diff(решение, t) -> для преодоления разрывов второго рода через производную (если не удается использовать просто 1/решение)
//
// sol - решение, которое подвергается преобразованию
// Rk_ex - откуда брать Rk - значение присваивается в конструкторе
// A и B - чтобы не было путаницы с A и B из родительского класса они = 1 в конструкторе
//
class new_var : public solution{
public:
	// aSol - решение;
	new_var(solution *aSol);
    new_var(const new_var &other);
    ~new_var(){};
public:
	// Возвращает значение R[i] для преобразованного решения
	virtual numeric numR(int i);
    // То есть, исходя из ICk для new_var нужно сформировать ICs для нормального solution-решения
    // функция set_ic(h) должна вызываться перед этой
  	virtual void set_ICs(vector<diff_sol> &ICs);

public:
	solution* sol;
	ex Rk_ex;
	ex A;
	ex B;
};

//-------------------------------------------------------------------
// Класс diff_sol для определения n-го начального условия, n = 0..inf
// дело в том, что для правильного определения n-го начального условия нужно полностью исследовать ряд diff(x(t),t,n)
class diff_sol : public new_var{
public:
	diff_sol(solution *aSol, int aN);
    diff_sol(const diff_sol &other);
    ~diff_sol(){};
public:
	// Возвращает значение R[i] для преобразованного решения
	numeric numR(int i);
    // Возвращает nIC +- GlobalD
    numeric numIC();
    void set_ic();
	void set_ic(const numeric &h);
public:
    numeric nIC;// значение начального условия
    int n;      // порядок производной
};

//-------------------------------------------------------------------
// Класс 1/решение -> для преодоления разрывов второго рода
class discont : public new_var{
public:
	discont(solution *aSol);
    // функция set_ic(h) должна вызываться перед этой
    discont(const discont &other);
    ~discont(){};
public:
	// Возвращает значение R[i] для преобразованного решения
	numeric numR(int i);
  	void set_ICs(vector<diff_sol> &ICs);
};

//-------------------------------------------------------------------
// Класс решение/(новая независимая переменная) -> для перехода к новой независимой переменной
class phase : public new_var{
public:
	phase(solution *aSol, solution *aVal);
    phase(const phase &other);
    ~phase(){};
public:
	// Возвращает значение R[i] для преобразованного решения
	numeric numR(int i);
    // функция set_ic(h) должна вызываться перед этой
  	void set_ICs(vector<diff_sol> &ICs);
};

//-------------------------------------------------------------------------------------------
// класс sub_val "под"-значения.
// Для использования вещей типа x(t).GlobalD или x(t).R[0] и т.д.(больше смотри в функции evalf())
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
// Класс - символ tk, для исследования параметрической чувствительности
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
// класс sym_IC - символьное обозначение начального условия x(0)

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
    bool bMinus; // флаг того, что шаг отрицательный
	// по умолчанию независимая переменная - TheTime. При переходе к новой независимой переменной TheTime необходимо вычислять хитрым образом: через ряд dt = 1/diff(новая независимая переменная)
//	solution *TheTime;
};

//--------------- ANM_system class -----------------------
// Класс, реализующий аналитическую и численную часть метода. Пока только для анализа.
//
class ANM_charts;
class ANM_system : public basic{
	GINAC_DECLARE_REGISTERED_CLASS(ANM_system, basic)
friend class ANM_solutions;
friend class ANM_chart_window;

public:
	ANM_system(const ex&);
	ANM_system(const lst&);
	virtual void Analitic(const ex&);							// Формирование РЯДОВ Тейлора для реакций
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
	virtual void Evulate();								// Вычисления проходят по шагам
	// инициализация матриц АЧМ
	void Init_A();
	void Init_G();
	void Init_H();
	void Init_Q();
	// довыделение с использованием матрицы Якоби (чтобы корни характеристического уравнения совпадали с собственными числами матрици Якоби)
	void SupSelecting();
	// довыделение линейной части, если она невыделенная
	void SupSelectLinearPart();
	// довыделение линейной части, для i-й строки и для l-й координаты. m_H - степень максимальной производной
	void SupSelectLinearPart(int i, int l, int m_H);
	// возвращает максимальную производную в выражении Expl для IdName
	int GetMaxDiffLevel(const ex& Expl, const ex& IdName);
	// возвращает коэффициент при максимальной производной m_H в выражении Expl для IdName
	ex GetMaxDiffCoef(const ex& Expl, const ex& IdName, int m_H);
	ex R(int l, int i);
	ex getX(int l);
	// возвращает линейную часть ODEi относительно IdName
	virtual ex GetLinearCoeff(const ex& ODEi, const ex& IdName);
	// возвращает нелинейную часть All
	virtual ex GetNotLinearPart(const ex& All);
	// заменяет x(t)->R(k) на x(t).R[k]. То есть sub_val на sym_rk
	virtual ex SubVal2Rk(const ex& ODEi);
	int Is_XorF(const ex& Expl);
	ex GetQElement(const ex& mxElem, const ex& IdName);
protected:
	sym_table analysis_table; // в этой таблице должны храниться все идентификаторы, которые используются исключительно при расчете конкретного уравнения
	ex InputData;	// входные данные. Функция 'analysis', введенная пользователем. Также при расчете используется таблица символов sym_tab
	matrix A;
	matrix X;
	matrix SymbX;	// для solve
	matrix G;
	matrix F;
	matrix H;
	matrix Q;
	matrix C;
	ANM_solutions Solutions;

	numeric InfT, SupT, tN;					// границы интервала исследования [InfT;SupT] и текущий момент времени tN

	int			L;					// количество реакций
	int			Lf;					// количество воздействий
	int			Forh;				// минимальное количество первых коэффициетов рядов для определения шага
	bool		Speciality;			// флаг расчета глобальной погрешности
	int			NE;					// NowEvulating: 0 - Приближенное решение; 1 - нижняя граница; 2 - верхняя граница;

	int			N;				// максимальная степень p в полиноме mainD
	float LineWidth;			// толщина линий прорисовки графиков
	int FontSize;				// размер шрифта
	int nWeight;				// жирность шрифта FW_NORMAL, FW_BOLD...
	ANM_charts* Result;			// указатель на хранилище результатов

	numeric h;				// шаг расчета
	numeric SupLocError;	// верхний уровень локальной погрешности - задается пользователем
	numeric StartError;		// начальное значение погрешности расчета (по умолчанию 0) - задается пользователем
	numeric Maxh;			// вычисленный шаг не может превышать этот...(для простых функций, чтобы их графики рисовались наглядно)
	numeric Minh;			// как только шаг становится меньше чем Minh вычисления прекращаются (если Minh достаточно мало, то это говорит о приближении к разрыву 2-го рода)
	lst SubResults;
	lst ODEs;
	lst ICs;
	lst AdditionalFuncs;
	bool bfInitialaized;
};

//--------------- ANM_sensitivity class -----------------------
// Класс, реализующий алгоритм исследования параметрической чувствительности при анализе
// ндс.
class all_charts;
class ANM_sensitivity : public ANM_system{
	GINAC_DECLARE_REGISTERED_CLASS(ANM_sensitivity, ANM_system)
friend class ANM_solutions;
friend class ANM_sens_system;

public:
	ANM_sensitivity(const ex& aInputData, const ex& aMu, numeric *aTime);
	void Calculate(const lst& aICs, all_charts*); // Вычисление границ, в которых лежат решения
	void Analitic();							// Формирование РЯДОВ Тейлора для реакций
//	void copy(const ANM_sensitivity &other);
protected:
	void Evulate();								// Вычисления проходят по шагам
//	ex GetLinearCoeff(const ex& ODEi, const ex& IdName);
//	ex GetNotLinearPart(const ex& All);
protected:
	ex mu; // параметр, относительно которого происходит расчет
	cur_tk Time_k;
	bool bSaveResult;
	int StepCntr;
};

//--------------- ANM_sens_system class -----------------------
// Класс, реализующий анализ ндс с учетом исследования параметрической чувствительности
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
	void Evulate();								// Вычисления проходят по шагам
	void EvulateSensSystem(const numeric& curx);
protected:
	ANM_sensitivity sens;
//	numeric delta_mu;
	all_charts* AllResults;			// указатель на хранилище результатов
	range* PlotsTimeRange;			// указатель на XRange графиков
};

//--------------- Plot class -----------------------
// Класс для рисования графиков. Как 2D так и 3D
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
	float LineWidth;			// толщина линий прорисовки графиков
	int FontSize;				// размер шрифта
	int nWeight;				// жирность шрифта FW_NORMAL, FW_BOLD...
	bool Is3D;
protected:
	ex InputData;	// входные данные. Функция 'plot', введенная пользователем.
	lst Funcs; // функции - их может быть несколько в виде {sin(t), cos(t)} или одна в виде sin(t)
	ex x, y; // переменные, которые изменяются x = rX.min..rX.max; y = rY.min..rY.max;
	numeric Xmin;
	numeric Xmax;
	numeric Ymin;
	numeric Ymax;
	numeric Zmin;
	numeric Zmax;
};

#endif // ndef ANM_H
