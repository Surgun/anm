#ifndef PARSER_H
#define PARSER_H

#include "..\\FSFlibs.h"
#include <iostream>
#include <map>

template<class T>
std::string ToString(const T & t);

extern symbol GetSymbol(const string&);
extern ex GetExpression(const ex&);
extern ex ExpandWithSymTab(ex&);
class ANM_solutions;
extern const ex ex_to_taylor_row(const ex& expr, ANM_solutions* XNames, bool bFunction = false);
//--------------- some more symbolic functions -----------------------

// heaviside(t)
DECLARE_FUNCTION_1P(heaviside)

// sign(t)
DECLARE_FUNCTION_1P(sign)

// a*x + b = 0 - root found
DECLARE_FUNCTION_2P(root_of1)

// a*x^2 + b*x + c = 0 - root found
DECLARE_FUNCTION_3P(root_of2)

// a*x^3 + b*x^2 + c*x + d = 0 - root found
DECLARE_FUNCTION_4P(root_of3)

// a*x^4 + b*x^3 + c*x^2 + d*x + e = 0 - root found
DECLARE_FUNCTION_5P(root_of4)

// a[n]*x^n + a[n-1]*x^(n-1) + ... + a[0] = 0 - root found
DECLARE_FUNCTION_2P(root_of)

// re(t)
DECLARE_FUNCTION_1P(re)

// im(t)
DECLARE_FUNCTION_1P(im)

//root_k(roots, k) - возвращает k-й корен из массива корней характеристического уравнения. Определяет последовательность корней.
DECLARE_FUNCTION_2P(root_k)


//--------------- myprocedure class -----------------------

class myprocedure : public basic
{
	GINAC_DECLARE_REGISTERED_CLASS(myprocedure, basic)
	friend class sym_table_element;
	friend class sym_table;
public:
	myprocedure(const ex &aName, const ex &aParamsLst);

public:
	void print(const print_context &c, unsigned level = 0) const;
	myprocedure& set_name(const symbol &aName)
	{
		name = aName;
		return *this;
	}
	myprocedure& set_params(const lst &aParams)
	{
		params = aParams;
		return *this;
	}
	void set(const symbol &aName, const lst &aParamsLst)
	{
		set_name(aName);
		set_params(aParamsLst);
	}

	size_t nops() const;
	ex op(size_t i) const;
	ex subs(const exmap & m, unsigned options = 0) const;
	int degree(const ex &s) const
	{
		if(s.is_equal(s_p))
			return 0;
		return inherited::degree(s);
	}
	ex coeff(const ex & s, int n = 1) const;
	ex& let_op(size_t i);
	ex evalf(int level = 0) const;
	//ex eval(int level = 0) ;
	ex derivative(const symbol &s) const;
	bool IsSymbolName(){return is_a<symbol>(name);}
private:
	ex name;
	ex params;
};

//---------------- ginac functions ------------------------------------------
/*
 *  Built-in functions
 */
typedef ex (*fcnp)(const exprseq &e);
typedef ex (*fcnp2)(const exprseq &e, int serial);

struct fcn_desc {
	fcn_desc() : p(NULL), num_params(0), is_ginac(false), serial(0) {}
	fcn_desc(fcnp func, int num) : p(func), num_params(num), is_ginac(false), serial(0) {}
	fcn_desc(fcnp2 func, int num, int ser) : p((fcnp)func), num_params(num), is_ginac(true), serial(ser) {}

	fcnp p;		// Pointer to function
	int num_params;	// Number of parameters (0 = arbitrary)
	bool is_ginac;	// Flag: function is GiNaC function
	int serial;	// GiNaC function serial number (if is_ginac == true)
};

typedef multimap<string, fcn_desc> fcn_tab;

typedef multimap<string, string> help_tab;

// Tables for initializing the "fcns" map and the function help topics
struct fcn_init {
	const char *name;
	fcnp p;
	int num_params;
};

struct fcn_help_init {
	const char *name;
	const char *help;
};

//--------------- spline coeff class -----------------------
// в классе есть набор интервалов, набор коэффициентов (для каждого интервала набор коэффициентов)
// а также, значение value, которое должно лежать в нужном интервале
// функция spline(lst(), lst(), value) возвращает сумму:
// spline_coeff(lst, lst, value)*value^0 + spline_coeff(lst, lst, value)*value^1 + spline_coeff(lst, lst, value)*value^2 + ...

class spline_coeff : public basic
{
	GINAC_DECLARE_REGISTERED_CLASS(spline_coeff, basic)
public:
	// параметр 1 - набор из n интервалов по два значения
	// параметр 2 - набор из n значений коэффициента (для каждого интервала свое значение)
	spline_coeff(const lst &aIntervals, const lst &aCoeffs, const ex &aValue);

public:
	ex coeff(const ex & s, int n = 1) const;
	void print(const print_context &c, unsigned level = 0) const;
	size_t nops() const;
	ex op(size_t i) const;
	//ex& let_op(size_t i);
	ex subs(const exmap & m, unsigned options = 0) const;
	ex evalf(int level = 0) const;
private:
	lst intervals;
	lst coeffs;
	ex value;
};

//--------------- defining class -----------------------

class defining : public basic
{
	GINAC_DECLARE_REGISTERED_CLASS(defining, basic)
	friend class sym_table_element;

public:
	defining(const ex &aProc, const ex &aValue);

public:
	void print(const print_context &c, unsigned level = 0) const;
	void set(const ex &aProc, const ex &aValue)
	{
		proc = aProc;
		value = aValue;
	}
	size_t nops() const{
		return 2;
	}
	ex op(size_t i) const{
		if(i)
			return value;
		else
			return proc;
	}
	ex subs(const exmap & m, unsigned options = 0) const
	{
		ex new_proc = proc.subs(m,options);
		ex new_value = value.subs(m,options);
		return defining(new_proc, new_value);
	}
/*	ex& let_op(size_t i){
		if(i)
			return params;
		else
			return name;
	}
*/	ex evalf(int level = 0) const;

private:
	ex proc;
	ex value;
};

//--------------- sym_table_element class-----------------------
class sym_table_element{
	friend class sym_table;
// public ctors
public:
	sym_table_element(const string &aName);
	sym_table_element(const string &aName, const ex &aValue);
	sym_table_element(const symbol &aName, const ex &aValue);
	sym_table_element(const myprocedure &aName, const ex &aValue);
	sym_table_element(const sym_table_element &Copy);
// public members
public:
	sym_table_element& operator = (const sym_table_element &Copy){
		name = Copy.name;
		value = Copy.value;
		func = Copy.func;
		//Rk.assign(Copy.Rk.begin(), Copy.Rk.end());
		return *this;
	}
	sym_table_element& operator = (const ex& aValue)
	{
		value = aValue;
		//Rk.clear();
		return *this;
	}
	string get_name()
	{
		return name.get_name();
	}
	symbol get_symbol_name()
	{
		return name;
	}
	bool is_func(const myprocedure& proc)
	{
		// если имена и количество аргументов совпадают, значит это оно...
		if(proc.name.is_equal(func.name))// && func.op(1).nops() == proc.op(1).nops())
			return true;
		return false;
	}
	ex Derivative(const myprocedure& proc, const symbol &s)
	{
		if(!is_func(proc))
			return diff(proc,s);
		return diff((value.subs(ex_to<lst>(func.params), ex_to<lst>(proc.params))), s);
	}
protected:
	symbol name;		// имя переменной
	myprocedure func;	// если определяется функция
//	vector <ex> Rk;		// если определяется функция, то у нее может быть разложение в ряд Тейлора
	ex value;			// символьное значение
};

//--------------- expr_table class -----------------------
// класс для хранения всех выражений, используемых в ядре.
// используется в функции GetExpression;
class expr_table : public lst{
	GINAC_DECLARE_REGISTERED_CLASS(expr_table, lst)
	
	// other ctors
//public:
	
public:
    unsigned int get_ind(const ex &aExpr);
	void print(const print_context &c, unsigned level = 0) const;
//private:
};

//--------------- logic_op class -----------------------
// класс для логических операторов 'or', 'and'. Проверка условия на true/false.
class logic_op : public lst, public vector<relational>{
	GINAC_DECLARE_REGISTERED_CLASS(logic_op, lst)
    friend class parser;
	
	// other ctors
//public:
	
public:
	void print(const print_context &c, unsigned level = 0) const;
    enum operation{log_or, log_and, log_not};
    bool to_bool() const;
protected:
    operation o;
};

//--------------- sym_table class -----------------------
// класс для хранения всех присвоений в ядре.
class sym_table : public vector<sym_table_element>{
public:
	sym_table(){}
	sym_table& append(const string& symName);
	sym_table& append(const string& symName, const ex& symValue);
	sym_table& append(const symbol& symName, const ex& symValue);
	sym_table& append(const symbol& symName);
    sym_table& append(const sym_table_element& elem);
	sym_table& append(const myprocedure& symName, const ex& symValue);
	lst GetSymList();
	lst GetValList();
	// возвращает exmap для замены переменных на их значения, включая конструкции вида i := 1
	exmap GetMap();
	// возвращает exmap для замены переменных на их значения, кроме переменных, для которых значением является число. То есть исключая конструкции вида i := 1
	exmap GetSymMap();
	int is_name(const string& aName);
	numeric Evalf(const string& aName);
//	vector <ex> Rk(const myprocedure&) ;
//	vector <ex>& let_Rk(const myprocedure&);
	ex Evalf(const myprocedure&, int level);
    ex get_procedure_body(const ex&);
	ex Eval(const myprocedure&, int level);
	ex Derivative(const myprocedure&, const symbol&);
	string GetStrData();
};


const string TokTabl[] = {
"+",		// 0
"-",		// 1
"*",		// 2
"/",		// 3
"(",		// 4
")",		// 5
"__id",		// 6	symbol
"__Float",	// 7
"=",		// 8
"==",		// 9
",",		// 10
";",		// 11
"^",		// 12
"{",		// 13
"}",		// 14
"[",		// 15
"]",		// 16
".",		// 17
"if",		// 18
"then",		// 19
"else",		// 20
"endif",	// 21
"for",  	// 22
"from",  	// 23
"to",     	// 24
"do",     	// 25
"od",     	// 26
":",		// 27
"<",		// 28
">",		// 29
"or",		// 30
"and",		// 31
"not",		// 32
""
};

enum TokID{
	T_Plus = 0,
	T_Minus = 1,
	T_Mul = 2,
	T_Div = 3,
	T_Opens = 4,
	T_Closes = 5,
	T_Symbol = 6,
	T_Float = 7,
	T_Eqv = 8,
	T_DoubleEqv = 9,
	T_Comma = 10,
	T_Semicolon = 11,
	T_Power = 12,
	T_OpenF = 13,
	T_CloseF = 14,
	T_OpenQ = 15,
	T_CloseQ = 16,
	T_Dot = 17,
	T_If = 18,
	T_Then = 19,
	T_Else = 20,
	T_Endif = 21,
	T_For = 22,
	T_From = 23,
	T_To = 24,
	T_Do = 25,
	T_Od = 26,
	T_DblDot = 27,
	T_Less = 28,
	T_More = 29,
	T_Or = 30,
	T_And = 31,
	T_Not = 32
};

//--------------- class parser -----------------------------------------------------------
class parser{
public:
	parser();
	~parser();

	bool Parse(const string& aInputStr);  // распознать!
    size_t StatementCount();
    ex GetStatement(size_t i);
	const string& GetErrors();			// возвращает распознанные ошибки

protected:			
	int	Scaner();						// Делает тоже, что и функция tokenize
	bool NotSeparator(char);
	int	GetToken();						// используется при сканировании
	// следующие функции и распознают всю эту фигню
	void StatList(bool need_to_add = true);
	ex&	Statement(ex&);
	logic_op& LogicExpr(logic_op&);
	ex&	Exp(ex&);
	ex&	Term(ex&);
	ex&	Factor(ex&);
	ex&	Function(ex&);
	ex&	Const(ex&);
	ex&	List(ex&);
	ex&	SubElement(ex&);
	ex&	Set(ex&);
	lst& ParamsList(lst&);
	ex&	GetParams(ex&);

	vector<int> tokens;			// список лексем
	vector<string> significs;	// список строковых значений лексем
	vector<ex> statements;		// список распознанных выражений

	int scanI;					// для сканера - номер текущего символа
	int CurToken;				// Номер текущей лексемы 
	string InputStr;
};

#endif // ndef
