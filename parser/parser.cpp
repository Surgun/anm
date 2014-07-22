#include <stdexcept>
#include <sstream>
#include <fstream>

#include "..\\rows\\rows.h"
#include "..\\parser\\parser.h"
#include "..\\system\\anm.h"

extern sym_table sym_tab;
extern expr_table expr_tab;

extern symbol GetSymbol(const string& aName);
extern ex GetExpression(const ex&);
extern ex ExpandWithSymTab(ex&);

//#define EMPTY(c) ((c) == ' ' || (c) == '\r' || (c) == '\n' || (c) == '\t')
//---------------- ginac functions ------------------------------------------

static fcn_tab::const_iterator find_function(const ex &sym, int req_params);

/*
 *  Built-in functions
 */
extern fcn_tab fcns;
// Table to map help topics to help strings
extern help_tab help;

extern void insert_fcn_help(const char *name, const char *str);
extern void print_help(const string &topic);
extern void print_help_topics(void);


/*
 *  Find a function given a name and number of parameters. Throw exceptions on error.
 */

static fcn_tab::const_iterator find_function(const ex &sym, int req_params)
{
	const string &name = ex_to<symbol>(sym).get_name();
	typedef fcn_tab::const_iterator I;
	pair<I, I> b = fcns.equal_range(name);
	if (b.first == b.second)
		throw(std::logic_error("unknown function '" + name + "'"));
	else {
		for (I i=b.first; i!=b.second; i++)
			if ((i->second.num_params == 0) || (i->second.num_params == req_params))
				return i;
	}
	throw(std::logic_error("invalid number of arguments to " + name + "()"));
}


//--------------- heaviside function -----------------------
static ex heaviside_eval(const ex & x)
{
	if (is_a<numeric>(x) && ex_to<numeric>(x).is_real())
		if(x < 0)
			return 0;
		else
			return 1;
	else
		return heaviside(x).hold();
}

static ex heaviside_evalf(const ex & x0)
{
	ex x = x0.subs(sym_tab.GetMap()).evalf();
	return heaviside_eval(x);
}

static ex heaviside_deriv(const ex & x, unsigned diff_param)
{
	return 0;
}

REGISTER_FUNCTION(heaviside, eval_func(heaviside_eval).
							evalf_func(heaviside_evalf).
							derivative_func(heaviside_deriv).
							latex_name("\\heaviside"));

//--------------- sign function -----------------------
static ex sign_eval(const ex & x)
{
	if (is_a<numeric>(x) && ex_to<numeric>(x).is_real())
	{
		if(x < 0)
			return -1;
		else if(x > 0)
			return 1;
		else
			return 0;
	}
	else
		return sign(x).hold();
}

static ex sign_evalf(const ex & x0)
{
	ex x = x0.subs(sym_tab.GetMap()).evalf();
	return sign_eval(x);
}

REGISTER_FUNCTION(sign, eval_func(sign_eval).
							evalf_func(sign_evalf).
							latex_name("\\sign"));

//--------------- Root_of x^3 function -----------------------
// см. справочник Б и С, стр. 139, уравнения 3-й степени
static ex root_of3_eval(const ex & a, const ex & b, const ex & c, const ex & d)
{
	ex y1, y2, y3;
	ex u, v, e1, e2, q, p;
	q = (2*pow(b,3)/numeric(27)/pow(a,3)-b*c/3/a/a+d/a)/numeric(2);
	//DEBUG_OUT("q = " << q << " = " << q.subs(sym_tab.GetMap()).evalf())
	p = (3*a*c-b*b)/numeric(9)/a/a;
	//DEBUG_OUT("p = " << p << " = " << p.subs(sym_tab.GetMap()).evalf())
	ex D = q*q+pow(p,numeric(3));
	ex bS1 = (sign(sign(D)+1)*sign(-q+sqrt(D)) - sign(D)*heaviside(-sign(D)));
	ex bS2 = (sign(sign(D)+1)*sign(-q-sqrt(D)) - sign(D)*heaviside(-sign(D)));
	u = bS1*pow(sign(sign(D)+1)*abs(-q+sqrt(D)) - sign(D)*heaviside(-sign(D))*(-q+sqrt(D)), numeric("1/3"));
	//DEBUG_OUT("u = " << u << " = " << u.subs(sym_tab.GetMap()).evalf())
	v = bS2*pow(sign(sign(D)+1)*abs(-q-sqrt(D)) - sign(D)*heaviside(-sign(D))*(-q-sqrt(D)), numeric("1/3"));
	//v = sign(-q-sqrt(D))*pow(abs(-q-sqrt(D)), numeric("1/3"));
	//DEBUG_OUT("v = " << v << " = " << v.subs(sym_tab.GetMap()).evalf())
	e1 = -numeric("1/2")+I*sqrt(numeric(3))/numeric(2);
	//DEBUG_OUT("e1 = " << e1)
	e2 = -numeric("1/2")-I*sqrt(numeric(3))/numeric(2);
	//DEBUG_OUT("e2 = " << e2)
	y1 = u + v;
	y2 = e1*u + e2*v;
	y3 = e2*u + e1*v;
	lst roots;
	roots.append(y1-b/a/numeric(3));
	roots.append(y2-b/a/numeric(3));
	roots.append(y3-b/a/numeric(3));
	return roots;
/*
q := (2*b^3/27/a^3-b*c/3/a^2+d/a)/2;
p := (3*a*c-b^2)/3/a^2/3;
D0 := q^2+p^3;
u := (-q+(D0)^(1/2))^(1/3); v := (-q-(D0)^(1/2))^(1/3);
epsilon[1] := -1/2+I*sqrt(3)/2;epsilon[2] := -1/2-I*sqrt(3)/2;
y1 := u+v:evalf(y1);
y2 := epsilon[1]*u+epsilon[2]*v:evalf(y2);
y3 := epsilon[2]*u+epsilon[1]*v:evalf(y3);
if q < 0 then
  r := -sqrt(abs(p));
else
  r := sqrt(abs(p));
fi;
if p < 0 then
   if D0 <= 0 then
      phi := arccos(q/r^3);
      y1 := -2*r*cos(phi/3);
   else
      phi := arcch(q/r^3);
      y1 := -2*r*ch(phi/3);
   fi
else
   phi := arcsh(q/r^3);
   y1 := -2*r*sh(phi/3);
fi;
*/
}

static ex root_of3_evalf(const ex & a, const ex & b, const ex & c, const ex & d)
{
	return root_of3_eval(a,b,c,d).subs(sym_tab.GetMap()).evalf();
}

REGISTER_FUNCTION(root_of3, eval_func(root_of3_eval).
							evalf_func(root_of3_evalf).
							latex_name("\\root_of3"));

//--------------- Root_of x^4 function -----------------------
// см. справочник Б и С, стр. 139, уравнения 4-й степени
static ex root_of4_eval(const ex & a0, const ex & b0, const ex & c0, const ex & d0, const ex & e0)
{
	//нормируем коэффициенты по a
	ex b=b0/a0,c=c0/a0,d=d0/a0,e=e0/a0;
	ex root3 = root_of3(8, -numeric(4)*c, numeric(2)*b*d-numeric(8)*e, e*(numeric(4)*c-b*b) - d*d);
	ex y = root3.op(0);
	//DEBUG_OUT("a := 8; b := " << (-numeric(4)*c).subs(sym_tab.GetMap()).evalf() << ";")
	//DEBUG_OUT("c := " << (numeric(2)*b*d-numeric(8)*e).subs(sym_tab.GetMap()).evalf() << ";")
	//DEBUG_OUT("d := " << (e*(numeric(4)*c-b*b) - d*d).subs(sym_tab.GetMap()).evalf() << ";")
	//DEBUG_OUT("root_of3 = " << root3.subs(sym_tab.GetMap()).evalf())
	//DEBUG_OUT("y = " << y << " = " << y.subs(sym_tab.GetMap()).evalf())
	ex A1 = sqrt(numeric(8)*y+b*b-numeric(4)*c);
	//DEBUG_OUT("A1 = " << A1)
	ex A2 = -sqrt(numeric(8)*y+b*b-numeric(4)*c);
	ex r1(root_of2(1, (b+A1)/numeric(2), y + (b*y-d)/A1));
	//DEBUG_OUT("r1 = " << r1)
	ex r2(root_of2(1, (b+A2)/numeric(2), y + (b*y-d)/A2));
	//DEBUG_OUT("r2 = " << r2)
	lst roots;
	roots.append(r1.op(0));
	//DEBUG_OUT("r1.op(0) = " << r1.op(0))
	roots.append(r1.op(1));
	roots.append(r2.op(0));
	roots.append(r2.op(1));

	y = root3.op(1);
	//DEBUG_OUT("y = " << y.subs(sym_tab.GetMap()).evalf())
	A1 = sqrt(numeric(8)*y+b*b-numeric(4)*c);
	//DEBUG_OUT("A1 = " << A1)
	A2 = -sqrt(numeric(8)*y+b*b-numeric(4)*c);
	r1 = root_of2(1, (b+A1)/numeric(2), y + (b*y-d)/A1);
	//DEBUG_OUT("r1 = " << r1)
	r2 = root_of2(1, (b+A2)/numeric(2), y + (b*y-d)/A2);
	//DEBUG_OUT("r2 = " << r2)
	roots.append(r1.op(0));
	roots.append(r1.op(1));
	roots.append(r2.op(0));
	roots.append(r2.op(1));

	y = root3.op(2);
	//DEBUG_OUT("y = " << y.subs(sym_tab.GetMap()).evalf())
	A1 = sqrt(numeric(8)*y+b*b-numeric(4)*c);
	//DEBUG_OUT("A1 = " << A1)
	A2 = -sqrt(numeric(8)*y+b*b-numeric(4)*c);
	r1 = root_of2(1, (b+A1)/numeric(2), y + (b*y-d)/A1);
	//DEBUG_OUT("r1 = " << r1)
	r2 = root_of2(1, (b+A2)/numeric(2), y + (b*y-d)/A2);
	//DEBUG_OUT("r2 = " << r2)
	roots.append(r1.op(0));
	roots.append(r1.op(1));
	roots.append(r2.op(0));
	roots.append(r2.op(1));
	return roots;
}

static ex root_of4_evalf(const ex & a, const ex & b, const ex & c, const ex & d, const ex & e)
{
	return root_of4_eval(a,b,c,d,e).subs(sym_tab.GetMap()).evalf();
}

REGISTER_FUNCTION(root_of4, eval_func(root_of4_eval).
							evalf_func(root_of4_evalf).
							latex_name("\\root_of4"));

//--------------- Root_of x^2 function -----------------------
static ex root_of2_eval(const ex & a, const ex & b, const ex & c)
{
	ex det = (pow(b,2) - 4*a*c);
	ex x1, x2;
	x1 = (-b+sqrt(det))/2/a;
	x2 = (-b-sqrt(det))/2/a;
	lst roots;
	roots.append(x1);
	roots.append(x2);
	return roots;
}

static ex root_of2_evalf(const ex & a, const ex & b, const ex & c)
{
	return root_of2_eval(a,b,c).subs(sym_tab.GetMap()).evalf();
}

REGISTER_FUNCTION(root_of2, eval_func(root_of2_eval).
							evalf_func(root_of2_evalf).
							latex_name("\\root_of2"));

//--------------- Root_of a*x + b = 0  function -----------------------
static ex root_of1_eval(const ex & a, const ex & b)
{
	lst roots;
	roots.append(-b/a);
	return roots;
}

static ex root_of1_evalf(const ex & a, const ex & b)
{
	return root_of1_eval(a,b).subs(sym_tab.GetMap()).evalf();
}

REGISTER_FUNCTION(root_of1, eval_func(root_of1_eval).
							evalf_func(root_of1_evalf).
							latex_name("\\root_of1"));

//--------------- Root_of function -----------------------
static ex root_of_eval(const ex & poli0, const ex & var)
{
	ex poli = poli0;
	while(poli.ldegree(var) < 0)
		poli = poli*var;
	if(poli.degree(var) == 1)
		return root_of1_eval(poli.coeff(var,1), poli.coeff(var,0));
	if(poli.degree(var) == 2)
		return root_of2_eval(poli.coeff(var,2), poli.coeff(var,1), poli.coeff(var,0));
	if(poli.degree(var) == 3)
		return root_of3_eval(poli.coeff(var,3), poli.coeff(var,2), poli.coeff(var,1), poli.coeff(var,0));
	if(poli.degree(var) == 4)
		return root_of4_eval(poli.coeff(var,4), poli.coeff(var,3), poli.coeff(var,2), poli.coeff(var,1), poli.coeff(var,0));
	lst roots;
	for(int i = 0; i < poli.degree(var); i++)
		roots.append(GetSymbol(ToString(var) + "[" + ToString(i+1) + "]"));
	return roots;
}

static ex root_of_evalf(const ex & poli0, const ex & var)
{
	return root_of_eval(poli0,var).subs(sym_tab.GetMap()).evalf();
/*	ex poli = poli0;
	while(poli.ldegree(var) < 0)
		poli *= var;
	if(poli.degree(var) == 2)
		return root_of2_evalf(poli.coeff(var,2), poli.coeff(var,1), poli.coeff(var,0));
	if(poli.degree(var) == 3)
		return root_of3_evalf(poli.coeff(var,3), poli.coeff(var,2), poli.coeff(var,1), poli.coeff(var,0));
	lst roots;
	for(int i = 0; i < poli.degree(var); i++)
		roots.append(subs(GetSymbol(ToString(var) + "[" + ToString(i+1) + "]"), sym_tab.GetMap()).evalf());
	return roots;
	*/
}

REGISTER_FUNCTION(root_of, eval_func(root_of_eval).
							evalf_func(root_of_evalf).
							latex_name("\\root_of"));

//--------------- re function -----------------------
static ex re_eval(const ex & x)
{
	if (is_a<numeric>(x))
		return ex_to<numeric>(x).real();
	else
		return re(x).hold();
}

static ex re_evalf(const ex & x)
{
	if (is_a<numeric>(x.evalf()))
		return ex_to<numeric>(x.evalf()).real();
	else
		return re(x).hold();
}

REGISTER_FUNCTION(re, eval_func(re_eval).
							evalf_func(re_evalf).
							latex_name("\\re"));

//--------------- im function -----------------------
static ex im_eval(const ex & x)
{
	if (is_a<numeric>(x))
		return ex_to<numeric>(x).imag();
	else
		return im(x).hold();
}

static ex im_evalf(const ex & x)
{
	if (is_a<numeric>(x.evalf()))
		return ex_to<numeric>(x.evalf()).imag();
	else
		return im(x).hold();
}

REGISTER_FUNCTION(im, eval_func(im_eval).
							evalf_func(im_evalf).
							latex_name("\\im"));

//--------------- root_k function -----------------------
//- возвращает k-й корень из массива корней характеристического уравнения. Нужна для определения последовательности корней
static ex root_k_eval(const ex &roots, const ex &k)
{
	return root_k(roots, k).hold();
}

static ex root_k_evalf(const ex &roots, const ex &k)
{
	int ik, iReal;
	lst num_roots;
	ik = ex_to<numeric>(k).to_int(), iReal = -1;
	bool bIm = false;
	for(int i = 0; i < roots.nops(); i++)
	{
		if (!is_a<numeric>(roots.op(i).evalf()))
			return root_k(roots, k).hold();
		num_roots.append(roots.op(i).evalf());
		if(ex_to<numeric>(num_roots.op(i)).imag() != 0)
			bIm = true;
		else
			iReal = i;
	}
	if(roots.nops() == 3)
	{
		if(bIm)
		{
			if(ik == 1)
				return num_roots.op(iReal);
			if(iReal == 1)
				return num_roots.op(ik);
			if(iReal == 2)
				return num_roots.op((ik==1)?1:3);
			if(iReal == 3)
				return num_roots.op(ik-1);
		}
	}
	return num_roots.op(ik);
}

REGISTER_FUNCTION(root_k, eval_func(root_k_eval).
							evalf_func(root_k_evalf).
							latex_name("\\root_k"));

//------ Реализация класса распознавателя ------------------------------------//

// CurToken++ нужно ставить после использования текущей лексемы, если она больше не нужна.

parser::parser()
{
}

parser::~parser()
{
}

bool
parser::NotSeparator(char ch)
{
	int i = 0;

	while(!TokTabl[i].empty())
	{
		if(ch == '=' && InputStr[scanI + 1] == '=')
		{
			scanI++;
			return false;
		}
		if(TokTabl[i].length() == 1)
			if(ch == TokTabl[i][0])
// в этом ифе идет проверка на то, является ли текущий плюс или минус степенью экспоненты в экспоненциальном представлении числа
				if(!((ch == '+' || ch == '-') && scanI && (InputStr[scanI - 1] == 'e' || InputStr[scanI - 1] == 'E')))
// в этом ифе идет проверка на то, является ли текущая точка точкой в числе
					if(!(ch == '.' && (scanI && isdigit(InputStr[scanI - 1]) || scanI < InputStr.length() - 1 && isdigit(InputStr[scanI + 1]))))
		 				return false;
		i++;
	}
	return true;
}

int
parser::Scaner()
{
	while(scanI < InputStr.length() && isspace(InputStr[scanI]))
		scanI++;
	if(scanI == InputStr.length())			// Если дошли до конца входной строки
	{
		tokens[CurToken] = -2;
	 	return 0;
	}
	while(scanI < InputStr.length() && !isspace(InputStr[scanI]) && NotSeparator(InputStr[scanI]))
		significs[CurToken] += InputStr[scanI++];

	while(scanI < InputStr.length() && isspace(InputStr[scanI]))
		scanI++;
	if(significs[CurToken].empty())
	{
		if(InputStr[scanI] == '=' && InputStr[scanI-1] == '=')
			significs[CurToken] += InputStr[scanI-1];
		significs[CurToken] += InputStr[scanI++];
	}
	tokens[CurToken++] = GetToken();
	significs.push_back("");
	tokens.push_back(-2);
	while(scanI < InputStr.length() && isspace(InputStr[scanI]))
		scanI++;
	return 1;
}

int
parser::GetToken()
{
	int for_return;
	int i = 0;

	while(significs[CurToken] != TokTabl[i] && !(TokTabl[i].empty()))
		i++;
	if(TokTabl[i].empty())
	{
		if(isdigit(significs[CurToken][0]))
			for_return = 7;				// numeric
		else
			for_return = 6;				// symbol
	}
	else
		for_return = i;
	return for_return;
}

void
parser::StatList(bool need_to_add)
{
//		<stat_list> ::== <statement>; | <stat_list> <statement>;
//      | if <logic_expr> then <stat_list> {else <stat_list>} endif;
//      | for id from <expl> to <logic_expr> do <stat_list> od;
	ex tmpOp;
	do{
		tmpOp = 0;
// endif or else
        if(tokens[CurToken] == T_Else || tokens[CurToken] == T_Endif)
            ERROR_20(significs[CurToken])
// if
        if(tokens[CurToken] == T_If)
        {
            CurToken++;
            logic_op logic_if;
// (a > b) or (a > c) - logic expression
            LogicExpr(logic_if);
// then
            if(tokens[CurToken++] != T_Then)
                ERROR_24(significs[CurToken-1])
// statement list
		    try{
                //DEBUG_OUT("logic_if.to_bool() = " << logic_if.to_bool())
                StatList(logic_if.to_bool());
		    } catch (const exception &e) {
                //DEBUG_OUT(ToString(e.what()))
                if("unrecognized statement" != ToString(e.what()) && "unrecognized or unexpected token: else" != ToString(e.what()) && "unrecognized or unexpected token: endif" != ToString(e.what()))
                    ERROR_JUST(e)
		    }
// else
            if(tokens[CurToken] == T_Else)
            {
                CurToken++;
// statement list
		        try{
                    StatList(!(logic_if.to_bool()));
		        } catch (const exception &e) {
                    //DEBUG_OUT("err Else " << ToString(e.what()))
                    if("unrecognized statement" != ToString(e.what()) && "unrecognized or unexpected token: endif" != ToString(e.what()))
                        ERROR_JUST(e)
		        }
            }
// endif;
            if(tokens[CurToken++] != T_Endif)
                ERROR_25(significs[CurToken-1])
            //DEBUG_OUT("after T_Endif... tokens[CurToken] = " << tokens[CurToken])
        }
        else if(tokens[CurToken] == T_For)
        {
        }
		else
            if(need_to_add)
                statements.push_back(Statement(tmpOp));
            else
                Statement(tmpOp);
        //DEBUG_OUT("tokens[CurToken] = " << tokens[CurToken])
		if(tokens[CurToken] != -2)
		{
			if(tokens[CurToken] != T_Semicolon && tokens[CurToken] != T_DblDot)
				ERROR_16
			CurToken++;
		}
	}while(tokens[CurToken] != -2);
}

logic_op&
parser::LogicExpr(logic_op& Operator)
{
//	<logic_expr>::== [not] <statement> | (<logic_expr>) | <statement> or <logic_expr> | <statement> and <logic_expr>
//<expr>
	ex tmpOp;
    logic_op::operation cur_oper, prev_oper;
    logic_op tmpLogOp, tmpNotOp;
    bool first = true;
    bool fnot = false;

    tmpNotOp.o = logic_op::log_not;

	if(tokens[CurToken] == T_Not)
    {
        fnot = true;
        CurToken++;
    }
	if(tokens[CurToken] == T_Opens)
    {
        logic_op tmpOp1;
        CurToken++;
        tmpOp = LogicExpr(tmpOp1);
    	if(tokens[CurToken] != T_Closes)
    	 	ERROR_21(significs[CurToken])
        CurToken++;
    }
    else
	    Statement(tmpOp);
    ExpandWithSymTab(tmpOp);
    if(fnot)
    {
        tmpNotOp.append(tmpOp);
        tmpOp = tmpNotOp;
        tmpNotOp.remove_all();
        fnot = false;
    }
    tmpLogOp.o = logic_op::log_and;
    Operator.o = logic_op::log_or;
    Operator.remove_all();
    ((tokens[CurToken] == T_And)?tmpLogOp:Operator).append(tmpOp);
    prev_oper = (tokens[CurToken] == T_And)?logic_op::log_and:logic_op::log_or;
	while(tokens[CurToken] == T_Or || tokens[CurToken] == T_And)
    {
        CurToken++;
	    if(tokens[CurToken] == T_Not)
        {
            fnot = true;
            CurToken++;
        }
	    if(tokens[CurToken] == T_Opens)
        {
            logic_op tmpOp1;
            CurToken++;
            tmpOp = LogicExpr(tmpOp1);
    	    if(tokens[CurToken] != T_Closes)
    	 	    ERROR_21(significs[CurToken])
            CurToken++;
        }
        else
	        Statement(tmpOp);
        cur_oper = (tokens[CurToken] == T_And)?logic_op::log_and:logic_op::log_or;
        ExpandWithSymTab(tmpOp);
        if(fnot)
        {
            tmpNotOp.append(tmpOp);
            tmpOp = tmpNotOp;
            tmpNotOp.remove_all();
            fnot = false;
        }
        if(cur_oper == logic_op::log_or && prev_oper == logic_op::log_or)
            Operator.append(tmpOp);
        else if(cur_oper == logic_op::log_and)
            tmpLogOp.append(tmpOp);
        else if(prev_oper == logic_op::log_and)
        {
            tmpLogOp.append(tmpOp);
            if((tokens[CurToken] == T_Or || tokens[CurToken] == T_And))
            {
                Operator.append(tmpLogOp);
                tmpLogOp.remove_all();
                //DEBUG_OUT("tmpLogOp after erase = " << tmpLogOp)
            }
        }
        prev_oper = cur_oper;
        first = false;
    }
    //DEBUG_OUT("Operator = " << Operator)
    //DEBUG_OUT("tmpLogOp = " << tmpLogOp)
    if(Operator.nops() == 0)
        Operator = tmpLogOp;
    else
        if(tmpLogOp.nops() != 0)
            Operator.append(tmpLogOp);
	Operator = ex_to<logic_op>(GetExpression(Operator));
    //DEBUG_OUT("LogicExpr returning " << Operator)
    return Operator;
}

ex&
parser::Statement(ex& Operator)
{
//		<statement> ::== [<expl>=]<expl>[==<expl> | < <expr> | > <expr> | <= <expr> | >= <expr> | != <expr>]
//	string temp;
	ex tmpOp;
	ex tmpVal;
	ex tmpRight;

	Operator = Exp(tmpOp);
	if(tokens[CurToken] == T_Eqv) // id = <expl>[ == <expl>]
	{
		if(!(is_a<myprocedure>(tmpOp) || is_a<symbol>(tmpOp)))	// если перед = не просто идентификатор или функция с параметрами, то
			ERROR_17(significs[CurToken-1], tmpOp)
		CurToken++;
		Statement(tmpVal);
		if(is_a<myprocedure>(tmpOp))	// если tmpOp - функция с параметрами
		{
			myprocedure tmpProc = ex_to<myprocedure>(tmpOp);
			// если в tmpProc храниться не Id(...), а, например diff(x(t),t)(0), то не надо ничего добавлять в табицу идентификаторов
			if(tmpProc.IsSymbolName())
				sym_tab.append(tmpProc, tmpVal);
		}
		else
		{
			if(ex_to<symbol>(tmpOp).get_name() == "Digits")
			{
				if(is_a<numeric>(evalf(tmpVal)))
					Digits = ex_to<numeric>(tmpVal).to_int();
				else
					ERROR_18
			}
			sym_tab.append(ex_to<symbol>(tmpOp), tmpVal);
		}
        Operator = GetExpression(defining(tmpOp, tmpVal));
		return Operator;
	}
// если распознано выражение - отношение (<, <=, >, >=, <>), то нужно возвращать соответствующее отношение
    TokID cur_tok = TokID(tokens[CurToken]);
    TokID additional_tok = cur_tok;
    switch(cur_tok){
        case T_Less:
            if(tokens[CurToken + 1] == T_More || tokens[CurToken + 1] == T_Eqv)
            {
                CurToken++;
                additional_tok = TokID(tokens[CurToken]);
            }
            CurToken++;
    		Exp(tmpRight);
            break;
        case T_More:
            if(tokens[CurToken + 1] == T_Eqv)
            {
                CurToken++;
                additional_tok = TokID(tokens[CurToken]);
            }
            CurToken++;
    		Exp(tmpRight);
            break;
        case T_DoubleEqv:
            CurToken++;
    		Exp(tmpRight);
            break;
    }
    switch(cur_tok){
        case T_Less:
        case T_More:
        case T_DoubleEqv:
            ExpandWithSymTab(tmpRight);
            ExpandWithSymTab(Operator);
            break;
    }

    switch(cur_tok){
        case T_Less:
            switch(additional_tok){
                case T_Less:
                    Operator = (Operator < tmpRight);
                    break;
                case T_Eqv:
                    Operator = (Operator <= tmpRight);
                    break;
                case T_More:
                    Operator = (Operator != tmpRight);
                    break;
            }
            break;
        case T_More:
            switch(additional_tok){
                case T_Eqv:
                    Operator = (Operator >= tmpRight);
                    break;
                case T_More:
                    Operator = (Operator > tmpRight);
                    break;
            }
            break;
        case T_DoubleEqv:
    		Operator = (Operator == tmpRight);
            break;
    }
	Operator = GetExpression(Operator);
    return Operator;
}

ex&
parser::Exp(ex& Operator)
{
//		<expl>			::== <term> | <expl> + <term> | <expl> - <term>
	ex tmpOp;

	Operator = Term(tmpOp);

	while(tokens[CurToken] == T_Plus || tokens[CurToken] == T_Minus)
	{
		int PrevCop = tokens[CurToken];
		CurToken++;
		Term(tmpOp);
		if(PrevCop == T_Plus)
			Operator+=tmpOp;
		else
			Operator-=tmpOp;
	}

	Operator = GetExpression(Operator);
    return Operator;
}

ex&
parser::Term(ex& Operator)
{
//		<term>		::== <factor> | <term> * <factor> | <term> / <factor>
	ex tmpOp;

	Operator = Factor(tmpOp);
	while(tokens[CurToken] == T_Mul || tokens[CurToken] == T_Div)
	{
		int PrevCop = tokens[CurToken];
		CurToken++;
		Factor(tmpOp);
		if(PrevCop == T_Mul)
			Operator *= tmpOp;
		else
			Operator /= tmpOp;
	};
	Operator = GetExpression(Operator);
    return Operator;
}

ex&
parser::Factor(ex& Operator)
{
//	<factor>::== id | float	| id(<expl>) | (<expl>) | id^<m_div_n> | <func_name>(<options_list>)
	ex tmpOp, tmpOp1;
	pair<fcn_tab::const_iterator, fcn_tab::const_iterator> b;
	Operator = 0;

	int FactorToken = tokens[CurToken];
	int symInd;
	switch(TokID(tokens[CurToken])){
		case T_Symbol:					// просто символьная лексема
// если лексема - стандартная константа
			tmpOp = 0;
			if (significs[CurToken] == "Pi")
				tmpOp = Pi;
			else if (significs[CurToken] == "Catalan")
				tmpOp = Catalan;
			else if (significs[CurToken] == "Euler")
				tmpOp = Euler;
			else if (significs[CurToken] == "I")
				tmpOp = I;
			if(!is_zero(tmpOp))
			{
				CurToken++;
				break;
			}
			b = fcns.equal_range(significs[CurToken]);
			//DEBUG_OUT("significs[CurToken] = " << significs[CurToken])
// если significs[CurToken] - имя зарегистрированной функции, то
			if (b.first != b.second)
			{
				//DEBUG_OUT("b.first != b.second")
				tmpOp = symbol(significs[CurToken]);
				GetParams(tmpOp1);
				fcn_tab::const_iterator func_ptr = find_function(tmpOp, tmpOp1.nops());
				if (func_ptr->second.is_ginac){
					tmpOp = ((fcnp2)(func_ptr->second.p))(ex_to<exprseq>(tmpOp1), func_ptr->second.serial);
				}
				else{
					tmpOp = (func_ptr->second.p)(ex_to<exprseq>(tmpOp1));
				}
				break;
			}
// если в таблице уже есть символ с таким именем, то нужно вернуть именно его (т.к. он будет использоваться потом)
			tmpOp = GetSymbol(significs[CurToken]);
			//DEBUG_OUT("tmpOp = " << tmpOp)
			CurToken++;
			break;
		case T_Minus:						// -id
			CurToken++;
			tmpOp = - Factor(tmpOp);
			break;
		case T_Float:
			tmpOp = ex(significs[CurToken], lst());
			CurToken++;
			break;
		case T_Opens:
			CurToken++;
			Exp(tmpOp);
			if(tokens[CurToken] != T_Closes)
		 		ERROR_19(significs[CurToken])
			CurToken++;
			break;
		case T_OpenF:
		case T_OpenQ:
			Set(tmpOp);
			break;
		default:
			ERROR_20(significs[CurToken])
	}
	Operator = tmpOp;
	bool b_flag = true;
	while(b_flag){
		switch(tokens[CurToken]){
		case T_Power:
			CurToken++;
			Operator = pow(Operator, Factor(tmpOp1));
			break;
		case T_Dot:
			CurToken++;
			Operator = sub_val(Operator, Factor(tmpOp1));
			break;
		case T_Opens:
			if(FactorToken == T_Symbol)
			{
				lst tmpLst;
				FactorToken = T_Opens;
				myprocedure proc(Operator, ParamsList(tmpLst));
				//DEBUG_OUT("myproc (" << Operator << ", " << tmpLst)
				Operator = proc.hold();
				break;
			}
		default:
			b_flag = false;
			break;
		}
	}
	Operator = GetExpression(Operator);
	//DEBUG_OUT("Operator = "<< Operator)
    return Operator;
}

lst&
parser::ParamsList(lst& Operator)
{
//		<params_list>::== <set> | <params_list>, <set>
	ex tmpOp;

	Operator.remove_all();
	if(tokens[CurToken] != T_Opens)
	 	ERROR_21(significs[CurToken])
	CurToken++;
// добавляем аргумент в список аргументов
	Operator.append(Set(tmpOp));
	while(tokens[CurToken] == T_Comma)
	{
		CurToken++;
// добавляем аргумент в список аргументов
		Operator.append(Set(tmpOp));
	}
	if(tokens[CurToken] != T_Closes)
	 	ERROR_21(significs[CurToken])
	CurToken++;
	Operator = ex_to<lst>(GetExpression(Operator));
    return Operator;
}
/*
ex&
parser::SubElement(ex& Operator)
{
//		<SubElement> ::== id | id[<factor>]
	ex tmpOp;

	Operator = aFactor(tmpOp);
// добавляем аргумент в список аргументов
	if(tokens[CurToken] != oOpenQ)
		Operator = GetExpression(Operator);
        return Operator;
	CurToken++;
	Operator.Add(aFactor(tmpOp));
	if(tokens[CurToken] != oCloseQ)
	 	AddError("SubElement : ] expected");
	CurToken++;
	Operator = GetExpression(Operator);
    return Operator;
}
*/
// возвращает список параметров, используется, когда инициализируется стандартная функция (sin, cos...)
ex&
parser::GetParams(ex& Operator)
{
//		<params_list>::== <set> | <params_list>, <set>
	ex tmpOp;
	exprseq tmpExp;

	Operator = 0;
	CurToken++;
	if(tokens[CurToken] != T_Opens)
	 	ERROR_21(significs[CurToken])
	CurToken++;
// добавляем аргумент в список аргументов
	tmpExp.append(Set(tmpOp));
	while(tokens[CurToken] == T_Comma)
	{
		CurToken++;
// добавляем аргумент в список аргументов
		tmpExp.append(Set(tmpOp));
	}
	if(tokens[CurToken] != T_Closes)
	 	ERROR_21(significs[CurToken])
	CurToken++;
	return (Operator = tmpExp);
}

ex&
parser::Set(ex& Operator)
{
//		<set> ::== {id, <set>} | id
	ex tmpOp;
	lst tmpLst;
	Operator = 0;
	if(tokens[CurToken] != T_OpenF && tokens[CurToken] != T_OpenQ)
	{
		Statement(Operator);
		Operator = GetExpression(Operator);
        return Operator;
	}
	CurToken++;
	tmpLst.append(Statement(tmpOp));
	while(tokens[CurToken] == T_Comma)
	{
		CurToken++;
		tmpLst.append(Statement(tmpOp));
	}
	if(tokens[CurToken] != T_CloseF && tokens[CurToken] != T_CloseQ)
	 	ERROR_21(significs[CurToken])
	CurToken++;
    Operator = GetExpression(tmpLst);
	return Operator;
}

bool parser::Parse(const string& aInputStr)
{
	ex res;
	InputStr = aInputStr;
	// Отсканировать строку в лексемы
	scanI = CurToken = 0;
	significs.clear();
	tokens.clear();
	statements.clear();
	significs.push_back("");
	tokens.push_back(-2);
	while(Scaner());
	CurToken = 0;
	// преобразовать набор лексем в четверки
	// Определить тип выражения: одно из уравнений системы уравнений, ошибочное уравнение, ввод предначальных условий или др.
	StatList();
	return statements.size() > 0;
}

size_t parser::StatementCount()
{
    return statements.size();
}

ex parser::GetStatement(size_t i)
{
    if(i >= 0 && i < statements.size())
        return statements[i];
    return 0;
}

/*//--------------- ex_ptr class-----------------------

GINAC_IMPLEMENT_REGISTERED_CLASS(ex_ptr, basic)
ex_ptr::ex_ptr() : inherited(TINFO_ex_ptr)
{
	value = NULL;
}

ex_ptr::ex_ptr(const ex &aName, numeric *aValue) : inherited(TINFO_ex_ptr), name(aName)
{
	value = aValue;
}

void ex_ptr::copy(const ex_ptr &other)
{
	inherited::copy(other);
	name = other.name;
	value = other.value;
}

void ex_ptr::archive(archive_node &n) const
{
}

ex_ptr::ex_ptr(const archive_node &n, lst &sym_lst) : inherited(n, sym_lst)
{
}

ex ex_ptr::unarchive(const archive_node &n, lst &sym_lst)
{
	return (new ex_ptr(n, sym_lst))->setflag(status_flags::dynallocated);
}

int ex_ptr::compare_same_type(const basic &other) const
{
	const ex_ptr &o = static_cast<const ex_ptr &>(other);
	if (name.is_equal(o.name) && (value == o.value || value->is_equal(*(o.value))))
		return 0;
	if(name.compare(o.name) < 0)
		return -1;
	if((*value).compare((*(o.value))) < 0)
		return -1;
	return 1;
}

void ex_ptr::print(const print_context &c, unsigned level) const
{
	// print_context::s is a reference to an ostream
	if(name.is_zero())
		c.s << (*value);
	else
		c.s << "^" << name;
}

ex ex_ptr::evalf(int level) const
{
	return value->evalf(level);
}

size_t ex_ptr::nops() const{
	return size_t(2);
}

ex ex_ptr::op(size_t i) const{
	GINAC_ASSERT(i<nops());
	if(i)
		return *value;
	else
		return name;
}
*/


