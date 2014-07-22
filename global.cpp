#include <stdexcept>
#include <sstream>
#include <fstream>

#include "parser\\parser.h"
#include "chart\\chart.h"
#include "rows\\rows.h"
#include <GL/gl.h>

symbol s_taylor("__taylor");
symbol s_p("__p");
symbol s_t("t");
symbol s_bMinus("__bMinus");
symbol s_diff("diff");

sym_table sym_tab;
expr_table expr_tab;

/*
 *  Built-in functions
 */
fcn_tab fcns;
// Table to map help topics to help strings
static help_tab help;

/*
 *  Built-in functions
 */

static ex f_collect(const exprseq &e) {return e[0].collect(e[1]);}
static ex f_collect_distributed(const exprseq &e) {return e[0].collect(e[1], true);}
static ex f_collect_common_factors(const exprseq &e) {return collect_common_factors(e[0]);}
static ex f_degree(const exprseq &e) {return e[0].degree(e[1]);}
static ex f_denom(const exprseq &e) {return e[0].denom();}
static ex f_eval1(const exprseq &e) {return e[0].subs(sym_tab.GetMap()).eval();}
static ex f_evalf1(const exprseq &e) {return e[0].subs(sym_tab.GetMap()).evalf();}
static ex f_evalm(const exprseq &e) {return e[0].subs(sym_tab.GetMap()).evalm();}
static ex f_expand(const exprseq &e) {return e[0].subs(sym_tab.GetMap()).expand();}
static ex f_gcd(const exprseq &e) {return gcd(e[0], e[1]);}
static ex f_has(const exprseq &e) {return e[0].subs(sym_tab.GetMap()).has(e[1]) ? ex(1) : ex(0);}
static ex f_lcm(const exprseq &e) {return lcm(e[0], e[1]);}
static ex f_lcoeff(const exprseq &e) {return e[0].subs(sym_tab.GetMap()).lcoeff(e[1]);}
static ex f_ldegree(const exprseq &e) {return e[0].subs(sym_tab.GetMap()).ldegree(e[1]);}
static ex f_lsolve(const exprseq &e) {return lsolve(e[0].subs(sym_tab.GetMap()), e[1].subs(sym_tab.GetMap()));}
static ex f_nops(const exprseq &e) {return e[0].nops();}
static ex f_normal1(const exprseq &e) {return e[0].normal();}
static ex f_numer(const exprseq &e) {return e[0].numer();}
static ex f_numer_denom(const exprseq &e) {return e[0].numer_denom();}
static ex f_pow(const exprseq &e) {return pow(e[0], e[1]);}
static ex f_sqrt(const exprseq &e) {return sqrt(e[0]);}
static ex f_sqrfree1(const exprseq &e) {return sqrfree(e[0]);}
static ex f_subs2(const exprseq &e) {return e[0].subs(e[1]);}
static ex f_tcoeff(const exprseq &e) {return e[0].tcoeff(e[1]);}

#define CHECK_ARG(num, type, fcn) if (!is_a<type>(e[num])) throw(std::invalid_argument("argument " #num " to " #fcn "() must be a " #type))

static ex f_charpoly(const exprseq &e)
{
	CHECK_ARG(0, matrix, charpoly);
	CHECK_ARG(1, symbol, charpoly);
	return ex_to<matrix>(e[0]).charpoly(ex_to<symbol>(e[1]));
}

static ex f_coeff(const exprseq &e)
{
	CHECK_ARG(2, numeric, coeff);
	return e[0].coeff(e[1], ex_to<numeric>(e[2]).to_int());
}

static ex f_content(const exprseq &e)
{
	CHECK_ARG(1, symbol, content);
	return e[0].content(ex_to<symbol>(e[1]));
}

static ex f_decomp_rational(const exprseq &e)
{
	CHECK_ARG(1, symbol, decomp_rational);
	return decomp_rational(e[0], ex_to<symbol>(e[1]));
}

static ex f_determinant(const exprseq &e)
{
	CHECK_ARG(0, matrix, determinant);
	return ex_to<matrix>(e[0]).determinant();
}

static ex f_diag(const exprseq &e)
{
	size_t dim = e.nops();
	matrix &m = *new matrix(dim, dim);
	for (size_t i=0; i<dim; i++)
		m.set(i, i, e.op(i));
	return m;
}

static ex f_diff2(const exprseq &e)
{
	CHECK_ARG(1, symbol, diff);
	return e[0].diff(ex_to<symbol>(e[1]));
}

static ex f_diff3(const exprseq &e)
{
	CHECK_ARG(1, symbol, diff);
	CHECK_ARG(2, numeric, diff);
	numeric expon = ex_to<numeric>(e[2]).to_int();
	if(expon >= 0)
		return e[0].diff(ex_to<symbol>(e[1]), expon.to_int());
	return myprocedure(GetSymbol("int"), lst(ex_to<symbol>(e[0]), -expon));
}

static ex f_divide(const exprseq &e)
{
	ex q;
	if (divide(e[0], e[1], q))
		return q;
	else
		return fail();
}

static ex f_eval2(const exprseq &e)
{
	CHECK_ARG(1, numeric, eval);
	return e[0].eval(ex_to<numeric>(e[1]).to_int());
}

static ex f_evalf2(const exprseq &e)
{
	CHECK_ARG(1, numeric, evalf);
	return e[0].evalf(ex_to<numeric>(e[1]).to_int());
}

static ex f_find(const exprseq &e)
{
	lst found;
	e[0].find(e[1], found);
	return found;
}

static ex f_integr(const exprseq &e)
{
	return 0;//integral(1,1,1,1).evalf();
}

static ex f_inverse(const exprseq &e)
{
	CHECK_ARG(0, matrix, inverse);
	return ex_to<matrix>(e[0]).inverse();
}

static ex f_is(const exprseq &e)
{
	CHECK_ARG(0, relational, is);
	return (bool)ex_to<relational>(e[0]) ? ex(1) : ex(0);
}

class apply_map_function : public map_function {
	ex apply;
public:
	apply_map_function(const ex & a) : apply(a) {}
	virtual ~apply_map_function() {}
	ex operator()(const ex & e) { return apply.subs(wild() == e, true); }
};

static ex f_map(const exprseq &e)
{
	apply_map_function fcn(e[1]);
	return e[0].map(fcn);
}

static ex f_match(const exprseq &e)
{
	lst repl_lst;
	if (e[0].match(e[1], repl_lst))
		return repl_lst;
	else
		return fail();
}

static ex f_normal2(const exprseq &e)
{
	CHECK_ARG(1, numeric, normal);
	return e[0].normal(ex_to<numeric>(e[1]).to_int());
}

static ex f_op(const exprseq &e)
{
	CHECK_ARG(1, numeric, op);
	int n = ex_to<numeric>(e[1]).to_int();
	if (n < 0 || n >= (int)e[0].nops())
		throw(std::out_of_range("second argument to op() is out of range"));
	return e[0].op(n);
}

static ex f_prem(const exprseq &e)
{
	CHECK_ARG(2, symbol, prem);
	return prem(e[0], e[1], ex_to<symbol>(e[2]));
}

static ex f_primpart(const exprseq &e)
{
	CHECK_ARG(1, symbol, primpart);
	return e[0].primpart(ex_to<symbol>(e[1]));
}

static ex f_quo(const exprseq &e)
{
	CHECK_ARG(2, symbol, quo);
	return quo(e[0], e[1], ex_to<symbol>(e[2]));
}

static ex f_rem(const exprseq &e)
{
	CHECK_ARG(2, symbol, rem);
	return rem(e[0], e[1], ex_to<symbol>(e[2]));
}

static ex f_roun(const exprseq &e)
{
	CHECK_ARG(0, numeric, roun);
	return numeric(ex_to<numeric>(e[0]).to_cl_N());
}

static ex f_series(const exprseq &e)
{
	CHECK_ARG(2, numeric, series);
	return e[0].series(e[1], ex_to<numeric>(e[2]).to_int());
}

static ex f_sprem(const exprseq &e)
{
	CHECK_ARG(2, symbol, sprem);
	return sprem(e[0], e[1], ex_to<symbol>(e[2]));
}

static ex f_sqrfree2(const exprseq &e)
{
	CHECK_ARG(1, lst, sqrfree);
	return sqrfree(e[0], ex_to<lst>(e[1]));
}

static ex f_subs3(const exprseq &e)
{
	CHECK_ARG(1, lst, subs);
	CHECK_ARG(2, lst, subs);
	return e[0].subs(ex_to<lst>(e[1]), ex_to<lst>(e[2]));
}

static ex f_plot1(const exprseq &e)
{
	ex tmpEx = lst(e[0]);
	Plot aPlot(tmpEx);
	return aPlot;
}

static ex f_plot2(const exprseq &e)
{
	ex tmpEx = lst(e[0], e[1]);
	Plot aPlot(tmpEx);
	return aPlot;
}

static ex f_plot3(const exprseq &e)
{
	ex tmpEx = lst(e[0], e[1], e[2]);
	Plot aPlot(tmpEx);
	return aPlot;
}

static ex f_analysis2(const exprseq &e)
{
	ex tmpEx = lst(e[0], e[1]);
	ANM_system method(tmpEx);
	return method;
}

static ex f_analysis3(const exprseq &e)
{
	ex tmpEx = lst(e[0], e[1], e[2]);
	ANM_system method(tmpEx);
	return method;
}

static ex f_analysis4(const exprseq &e)
{
	ex tmpEx = lst(e[0], e[1], e[2], e[3]);
	ANM_system method(tmpEx);
	return method;
}

static ex f_analysis5(const exprseq &e)
{
	ex tmpEx = lst(e[0], e[1], e[2], e[3], e[4]);
	ANM_system method(tmpEx);
	return method;
}

static ex f_analysis_sens3(const exprseq &e)
{
	ex tmpEx = lst(e[0], e[1], e[2]);
	ANM_sens_system method(tmpEx);
	return method;
}

static ex f_analysis_sens4(const exprseq &e)
{
	ex tmpEx = lst(e[0], e[1], e[2], e[3]);
	ANM_sens_system method(tmpEx);
	return method;
}

static ex f_analysis_sens5(const exprseq &e)
{
	ex tmpEx = lst(e[0], e[1], e[2], e[3], e[4]);
	ANM_sens_system method(tmpEx);
	return method;
}

static ex f_analysis_sens6(const exprseq &e)
{
	ex tmpEx = lst(e[0], e[1], e[2], e[3], e[4], e[5]);
	ANM_sens_system method(tmpEx);
	return method;
}

ex spline(const lst&, const lst&, const ex&);
static ex f_spline(const exprseq &e)
{
	CHECK_ARG(0, lst, spline);
	CHECK_ARG(1, lst, spline);
	return spline(ex_to<lst>(e[0]), ex_to<lst>(e[1]), e[2]);
}

static ex f_taylor(const exprseq &e)
{
	return (ex_to_taylor_row(e[0],0));
}

static ex f_taylor2(const exprseq &e)
{
	CHECK_ARG(1, numeric, taylor);
	ex tmpEx = (ex_to_taylor_row(e[0],0));
	return tmpEx.coeff(s_taylor, ex_to<numeric>(e[1]).to_int());
}

static ex f_trace(const exprseq &e)
{
	CHECK_ARG(0, matrix, trace);
	return ex_to<matrix>(e[0]).trace();
}

static ex f_transpose(const exprseq &e)
{
	CHECK_ARG(0, matrix, transpose);
	return ex_to<matrix>(e[0]).transpose();
}

static ex f_unassign(const exprseq &e)
{
	CHECK_ARG(0, symbol, unassign);
	const_cast<symbol&>(ex_to<symbol>(e[0])).unassign();
	return e[0];
}

static ex f_unit(const exprseq &e)
{
	CHECK_ARG(1, symbol, unit);
	return e[0].unit(ex_to<symbol>(e[1]));
}

static ex f_dummy(const exprseq &e)
{
	throw(std::logic_error("dummy function called (shouldn't happen)"));
}

static const fcn_init builtin_fcns[] = {
	{"round", f_roun, 1},
	{"analysis", f_analysis2, 2},
	{"analysis", f_analysis3, 3},
	{"analysis", f_analysis4, 4},
	{"analysis", f_analysis5, 5},
	{"analysis_sens", f_analysis_sens3, 3},
	{"analysis_sens", f_analysis_sens4, 4},
	{"analysis_sens", f_analysis_sens5, 5},
	{"analysis_sens", f_analysis_sens6, 6},
	{"charpoly", f_charpoly, 2},
	{"coeff", f_coeff, 3},
	{"collect", f_collect, 2},
	{"collect_common_factors", f_collect_common_factors, 1},
	{"collect_distributed", f_collect_distributed, 2},
	{"content", f_content, 2},
	{"decomp_rational", f_decomp_rational, 2},
	{"degree", f_degree, 2},
	{"denom", f_denom, 1},
	{"determinant", f_determinant, 1},
	{"diag", f_diag, 0},
	{"diff", f_diff2, 2},
	{"diff", f_diff3, 3},
	{"divide", f_divide, 2},
	{"eval", f_eval1, 1},
	{"eval", f_eval2, 2},
	{"evalf", f_evalf1, 1},
	{"evalf", f_evalf2, 2},
	{"evalm", f_evalm, 1},
	{"expand", f_expand, 1},
	{"find", f_find, 2},
	{"gcd", f_gcd, 2},
	{"has", f_has, 2},
	{"int", f_integr, 4},
	{"inverse", f_inverse, 1},
	{"is", f_is, 1},
	{"lcm", f_lcm, 2},
	{"lcoeff", f_lcoeff, 2},
	{"ldegree", f_ldegree, 2},
	{"lsolve", f_lsolve, 2},
	{"map", f_map, 2},
	{"match", f_match, 2},
	{"nops", f_nops, 1},
	{"normal", f_normal1, 1},
	{"normal", f_normal2, 2},
	{"numer", f_numer, 1},
	{"numer_denom", f_numer_denom, 1},
	{"op", f_op, 2},
	{"plot", f_plot1, 1},
	{"plot", f_plot2, 2},
	{"plot", f_plot3, 3},
	{"pow", f_pow, 2},
	{"prem", f_prem, 3},
	{"primpart", f_primpart, 2},
	{"quo", f_quo, 3},
	{"rem", f_rem, 3},
	{"series", f_series, 3},
	{"spline", f_spline, 3},
	{"sprem", f_sprem, 3},
	{"sqrfree", f_sqrfree1, 1},
	{"sqrfree", f_sqrfree2, 2},
	{"sqrt", f_sqrt, 1},
	{"subs", f_subs2, 2},
	{"subs", f_subs3, 3},
	{"taylor", f_taylor, 1},
	{"taylor", f_taylor2, 2},
	{"tcoeff", f_tcoeff, 2},
	{"time", f_dummy, 0},
	{"trace", f_trace, 1},
	{"transpose", f_transpose, 1},
	{"unassign", f_unassign, 1},
	{"unit", f_unit, 2},
	{NULL, f_dummy, 0}	// End marker
};

static const fcn_help_init builtin_help[] = {
	{"acos", "inverse cosine function"},
	{"acosh", "inverse hyperbolic cosine function"},
	{"asin", "inverse sine function"},
	{"asinh", "inverse hyperbolic sine function"},
	{"atan", "inverse tangent function"},
	{"atan2", "inverse tangent function with two arguments"},
	{"atanh", "inverse hyperbolic tangent function"},
	{"beta", "Beta function"},
	{"binomial", "binomial function"},
	{"cos", "cosine function"},
	{"cosh", "hyperbolic cosine function"},
	{"exp", "exponential function"},
	{"factorial", "factorial function"},
	{"lgamma", "natural logarithm of Gamma function"},
	{"tgamma", "Gamma function"},
	{"log", "natural logarithm"},
	{"psi", "psi function\npsi(x) is the digamma function, psi(n,x) the nth polygamma function"},
	{"sin", "sine function"},
	{"sinh", "hyperbolic sine function"},
	{"tan", "tangent function"},
	{"tanh", "hyperbolic tangent function"},
	{"zeta", "zeta function\nzeta(x) is Riemann's zeta function, zeta(n,x) its nth derivative"},
	{"Li2", "dilogarithm"},
	{"Li3", "trilogarithm"},
	{"Order", "order term function (for truncated power series)"},
	{"Derivative", "inert differential operator"},
	{NULL, NULL}	// End marker
};


symbol GetSymbol(const string& aName)
{
	if(aName == "t")
        return s_t;
//	if(aName == "CurT")
//        return s_CurT;
	if(aName == "__p")
        return s_p;
	if(aName == "__taylor")
        return s_taylor;
	if(aName == "__bMinus")
        return s_bMinus;
    int symInd;
	if((symInd = sym_tab.is_name(aName)) != -1)
		return sym_tab[symInd].get_symbol_name();
	else
	{
		symbol tmpSym = symbol(aName);
		sym_tab.append(tmpSym);
		return tmpSym;
	}
}

ex GetExpression(const ex& aExpr)
{
    int exInd = expr_tab.get_ind(aExpr);
    return expr_tab[exInd];
}

//------------------------ функция преобразования просто выражения в ряд (для матрицы нелинейностей) ---------------
const ex Laplace_trans(const ex& expr)
{
	ex res;
// если имеем дело с суммой
	if(is_a<add>(expr))
	{
		res = 0;
		for(int i = 0; i < expr.nops(); i++)
			res += Laplace_trans(expr.op(i));
		return res;
	}
// если это константа
	if(is_a<spline_coeff>(expr) || /*!is_a<myprocedure>(expr) && */!expr.has(s_t))
		return expr/s_p;
	return expr;
}

//-------------------------------------------------------------------------------
const ex ex_to_ppolinom(const ex& expr)
{
	ex res;
// произведение
	if(is_a<mul>(expr))
	{
		res = 1;
		ex tmpEx;
		int pexp = 0;
		for(int i = 0; i < expr.nops(); i++)
		{
			tmpEx = ex_to_ppolinom(expr.op(i));
			if(is_a<row_pexp>(tmpEx))
				pexp += ex_to<numeric>(tmpEx.op(1)).to_int();
			else
				res *= tmpEx;
		}
		if(pexp)
			return row_pexp(res,pexp);
		else
			return res;
	}
// сумма
	if(is_a<add>(expr))
	{
		res = 0;
		for(int i = 0; i < expr.nops(); i++)
			res += ex_to_ppolinom(expr.op(i));
		return res;
	}
// возведение в степень
	if(is_a<power>(expr))
	{
		if(expr.op(0).is_equal(s_p))
			if(is_a<numeric>(expr.op(1))/* && ex_to<numeric>(expr.op(1)).to_int() > 0*/)
				return row_pexp(1, ex_to<numeric>(expr.op(1)).to_int());
		return expr;
	}
// если это __p
	if(expr.is_equal(s_p))
		return row_pexp(1, 1);
	return expr;
}

//-------------------------------------------------------------------------------
// функция преобразования просто выражения в ряд (для матрицы нелинейностей)
const ex ex_to_taylor_row(const ex& expr, ANM_solutions* XNames, bool bFunction)
{
    //DEBUG_OUT("expr = " << expr << "; bFunction = " << bFunction)
	ex res;
// произведение __p на что-то (pexp::op(0))
	if(is_a<row_pexp>(expr))
		return row_pexp(ex_to_taylor_row(expr.op(0), XNames, bFunction), ex_to<numeric>(expr.op(1)).to_int());
// произведение
	if(is_a<mul>(expr))
	{
		res = 1;
		ex rmul = 1;
		ex tmpEx;
		//DEBUG_OUT("expr = " << expr)
		for(int i = 0; i < expr.nops(); i++)
		{
			//DEBUG_OUT("expr.op(i) = " << expr.op(i))
			tmpEx = ex_to_taylor_row(expr.op(i), XNames, bFunction);
			//DEBUG_OUT("tmpEx = " << tmpEx)
			if((!tmpEx.is_equal(expr.op(i)) && !is_a<spline_coeff>(tmpEx)
				|| is_a<myprocedure>(tmpEx) && tmpEx.op(0) != GetSymbol("IC")))// && !is_a<sym_rk>(tmpEx)))// && !(is_a<power>(tmpEx)&&is_a<sym_rk>(tmpEx.op(0))))
				rmul *= tmpEx;
			else
				res *= tmpEx;
			//DEBUG_OUT("rmul = " << rmul)
			//DEBUG_OUT("res = " << res)
		}
        if(is_a<mul>(rmul))
			return row_mul(ex_to<mul>(rmul))*res;
		else
			return rmul*res;
	}
// возведение в степень
	if(is_a<power>(expr))
	{
		//DEBUG_OUT("expr = " << expr)
		if(is_a<sym_rk>(expr.op(0)))
			return expr;
		if(expr.op(0).is_equal(s_t))
			return row_all(expr);
		if(is_a<numeric>(expr.op(1).evalf()) && ex_to<numeric>(expr.op(1).eval()).is_integer() && expr.op(1).evalf() > 0)
			return row_power(ex_to_taylor_row(expr.op(0),XNames, bFunction), expr.op(1));
		else
		{
			return row_power_z(ex_to_taylor_row(expr.op(0),XNames, bFunction), expr.op(1));
		}
	}
// сумма
	if(is_a<add>(expr))
	{
		res = 0;
		for(int i = 0; i < expr.nops(); i++)
			res += ex_to_taylor_row(expr.op(i),XNames,bFunction);
		return res;
	}
// sub_val - x(t).R(0)
	if(is_a<sub_val>(expr) && is_a<myprocedure>(expr.op(0)))
	{
		myprocedure tmpProc = ex_to<myprocedure>(expr.op(0));
		if(XNames && XNames->is_a_ANM_Solution(tmpProc))
		{
			return sub_val(unknown_sol(XNames->get_sptr(tmpProc)), ex_to_taylor_row(expr.op(1),XNames,bFunction));
		}
		return expr;
	}
// если это коэффициент сплайна
	if(is_a<spline_coeff>(expr))
		return spline_coeff(ex_to<lst>(expr.op(0)), ex_to<lst>(expr.op(1)), ex_to_taylor_row(expr.op(2), XNames,bFunction));
// какая-то нераспознанная функция
	if(is_a<myprocedure>(expr))
	{
		myprocedure tmpProc = ex_to<myprocedure>(expr);
		if(tmpProc.op(0).is_equal(s_diff))
		{
			tmpProc.let_op(1).let_op(0) = ex_to_taylor_row(tmpProc.op(1).op(0),XNames,bFunction);
			return tmpProc;
		}
		if(XNames && XNames->is_a_ANM_Solution(tmpProc))
			return unknown_sol(XNames->get_sptr(tmpProc));
		if(XNames && XNames->is_a_ANM_IC(tmpProc))
		{
			if(tmpProc.op(1).op(0).is_equal(0))
				return sym_IC(XNames->get_sptr(tmpProc), 0);
			return sym_IC(XNames->get_sptr(tmpProc), ex_to<numeric>(tmpProc.op(1).op(1)).to_int());
		}
		return row_all(expr);
	}
// функции sin(), cos()...
	if(is_a<function>(expr))
	{
		string fun_name = ex_to<function>(expr).get_name();
		//DEBUG_OUT("function name = " << fun_name)
        ex tmpFunc = expr;
        if(tmpFunc.nops() == 1)
		{
			//DEBUG_OUT("ex_to_taylor_row(tmpFunc.op(0),XNames, true) = " << ex_to_taylor_row(tmpFunc.op(0),XNames, true))
			tmpFunc.let_op(0) = ex_to_taylor_row(tmpFunc.op(0),XNames, true);
		    //tmpFunc.subs(lst(tmpFunc.op(0)), lst(ex_to_taylor_row(tmpFunc.op(0),XNames, true)));
		}
		//DEBUG_OUT("tmpFunc = " << tmpFunc)
		if(fun_name == "sign" || fun_name == "abs")
			return tmpFunc;
		return row_all(tmpFunc);
	}
// независимая переменная - t
	if(expr.is_equal(s_t))
        if(bFunction)
            return expr;
        else
		    return row_all(expr);

// символ - имя реакции (например x1 вместо x1(t))
	if(is_a<symbol>(expr))
	{
		myprocedure tmpProc = myprocedure(expr, lst(s_t));
		if(XNames && XNames->is_a_ANM_Solution(tmpProc))
			return unknown_sol(XNames->get_sptr(tmpProc));
		return expr;//.subs(sym_tab.GetMap());
	}
// константа или __p
	if(is_a<numeric>(expr) || !expr.has(s_t) || is_a<sym_rk>(expr) || is_a<unknown_sol>(expr))
		return expr;
	return row_all(expr);
}

//-------------------------------------------------------------------------------
// функция раскрывает выражение, с учетом всех значений, присвоенных и записанных в sym_table
ex ExpandWithSymTab(ex& aExpr)
{
    //DEBUG_OUT("ExpandWithSymTab")
    //DEBUG_OUT("aExpr = " << aExpr)
    while(!aExpr.subs(sym_tab.GetMap()).is_equal(aExpr))
        aExpr = aExpr.subs(sym_tab.GetMap());
    //DEBUG_OUT("aExpr after circle = " << aExpr)
    return aExpr;
}


//-------------------------------------------------------------------------------
// All registered GiNaC functions
static ex f_ginac_function(const exprseq &es, int serial)
{
	return function(serial, es).eval(1);
}

void GiNaC::ginsh_get_ginac_functions(void)
{
	vector<function_options>::const_iterator i = function::registered_functions().begin(), end = function::registered_functions().end();
	unsigned serial = 0;
	while (i != end) {
		fcns.insert(make_pair(i->get_name(), fcn_desc(f_ginac_function, i->get_nparams(), serial)));
		++i;
		serial++;
	}
}
//---------------------------------------------------------------------------
/*
 *  Add functions to ginsh
 */

// Functions from fcn_init array
static void insert_fcns(const fcn_init *p)
{
	while (p->name) {
		fcns.insert(make_pair(string(p->name), fcn_desc(p->p, p->num_params)));
		p++;
	}
}

/*
 *  Insert help strings
 */

// Normal help string
static void insert_help(const char *topic, const char *str)
{
	help.insert(make_pair(string(topic), string(str)));
}

// Help string for functions, automatically generates synopsis
static void insert_fcn_help(const char *name, const char *str)
{
	typedef fcn_tab::const_iterator I;
	pair<I, I> b = fcns.equal_range(name);
	if (b.first != b.second) {
		string help_str = string(name) + "(";
		for (int i=0; i<b.first->second.num_params; i++) {
			if (i)
				help_str += ", ";
			help_str += "expression";
		}
		help_str += ") - ";
		help_str += str;
		help.insert(make_pair(string(name), help_str));
	}
}

// Help strings for functions from fcn_help_init array
static void insert_help(const fcn_help_init *p)
{
	while (p->name) {
		insert_fcn_help(p->name, p->help);
		p++;
	}
}


/*
 *  Print help to cout
 */

// Help for a given topic
static void print_help(const string &topic)
{
	typedef help_tab::const_iterator I;
	pair<I, I> b = help.equal_range(topic);
	if (b.first == b.second)
		cout << "no help for '" << topic << "'\n" << flush;
	else {
		for (I i=b.first; i!=b.second; i++)
			cout << i->second << "\x0D\x0A" << flush;
	}
}

// List of help topics
static void print_help_topics(void)
{
	cout << "Available help topics:\n" << flush;
	help_tab::const_iterator i;
	string last_name = string("*");
	int num = 0;
	for (i=help.begin(); i!=help.end(); i++) {
		// Don't print duplicates
		if (i->first != last_name) {
			if (num)
				cout << ", " << flush;
			num++;
			cout << i->first << flush;
			last_name = i->first;
		}
	}
	cout << "\nTo get help for a certain topic, type ?topic\n" << flush;
}

//---------------------------------------------------------------------------
void lib_init()
{
	// Init function table
	insert_fcns(builtin_fcns);
	ginsh_get_ginac_functions();

	// Help for GiNaC functions is added manually
	insert_help(builtin_help);
}

//----------------------------------------------------------------------------------------
string num_to_string(const numeric& x, int MaxN = 0)
{
	std::ostringstream buf;
	int aDig = Digits;
	Digits = 8;
	if(x.is_integer())
		buf << (x);
	else
		buf << (x.evalf());
	string num_str = buf.str();
	if(num_str.length() > Digits)
	{
		std::ostringstream buf1;
		buf1 << x.evalf();
		num_str = buf1.str();
	}
	Digits = aDig;
	return num_str;
}

//---------------------------------------------------------------------------
void SetGLColor(long lColor)
{
	glColor4ub((GetRValue(lColor)),(GetGValue(lColor)),(GetBValue(lColor)), 220);
}

//---------------------------------------------------------------------------
void DrawName(HDC &hdc, unsigned long color, ANM_chart_window *prop, const string &name)
{
	if(!prop->bLegend)
		return;
	extern int iBottom;
	extern void TextOut3D(HDC hdc, int x, int y, const char* str, int length, int z = 0);
	LOGBRUSH logBrush;
	logBrush.lbColor = color;
	logBrush.lbStyle = BS_SOLID;
	HBRUSH brush;
	brush = CreateBrushIndirect(&logBrush);

	int ind = prop->CurPlotInd++;
	prop->MaxNameLength = max(prop->MaxNameLength, int(name.length()*prop->cWidth));
	POINT fr[4];
	fr[0].x = prop->cRect.left + prop->cWidth;
	fr[0].y = prop->cRect.bottom + (6*prop->cHeight*ind)/5 + prop->cHeight;
	fr[1].x = fr[0].x + 2*prop->cWidth;
	fr[1].y = fr[0].y;
	fr[2].x = fr[1].x;
	fr[2].y = fr[0].y + 4;
	fr[3].x = fr[0].x;
	fr[3].y = fr[2].y;
	glNewList(prop->glListInd++, GL_COMPILE);
	//DEBUG_OUT("prop->bOpenGL = " << prop->bOpenGL)
	if(prop->bOpenGL)
	{
		glBegin(GL_QUADS);
		SetGLColor(color);
		DEBUG_OUT("fr[0].y = " << fr[0].y)
		glVertex3f(fr[0].x,iBottom - fr[0].y,0);
		glVertex3f(fr[1].x,iBottom - fr[1].y,0);
		glVertex3f(fr[2].x,iBottom - fr[2].y,0);
		glVertex3f(fr[3].x,iBottom - fr[3].y,0);
		glEnd();
	}
	else
	{
		HRGN rgn = CreatePolygonRgn(fr, 4, ALTERNATE);
		FillRgn(hdc, rgn, brush);
		DeleteObject(rgn);
	}
	SetTextAlign(hdc, TA_LEFT | TA_TOP );
	//DEBUG_OUT("fr[0].y - prop->cHeight/2 = " << (fr[0].y - prop->cHeight/2)) + 3*prop->cHeight/2
	TextOut3D(hdc, fr[1].x + prop->cWidth/2, fr[0].y + prop->cHeight/4, name.c_str(), name.length());
	glEndList();
	DeleteObject(brush);
}
//---------------------------------------------------------------------------
bool bound(const POINT3D &p, const RECT &br)
{
	if(p.x < br.left || p.x > br.right || p.y < br.top || p.y > br.bottom)
		return false;
    return true;
}

//---------------------------------------------------------------------------
// p1 - точка внутри br, p2 - точка вне br. возвращает точку на границе прямоугольника br
const POINT3D get_pt(const POINT3D &p1, const POINT3D &p2, const RECT &br)
{
	//DEBUG_OUT("get_pt")
    POINT3D p;
	int dx = 0;
	if(p2.x < br.left)
    {
		p.x = br.left - dx;
        if(p2.x != p1.x)
		    p.y = ((p.x - p2.x)*(p1.y-p2.y))/(p1.x-p2.x)+p2.y;
    }
	if(p2.x > br.right)
    {
		p.x = br.right + dx;
        if(p2.x != p1.x)
    		p.y = ((p.x - p2.x)*(p1.y-p2.y))/(p1.x-p2.x)+p2.y;
    }
	if(p2.y < br.top)
    {
		p.y = br.top - dx;
        if(p1.y != p2.y)
    		p.x = ((p.y - p2.y)*(p1.x-p2.x))/(p1.y-p2.y)+p2.x;
    }
	if(p2.y > br.bottom)
    {
		p.y = br.bottom + dx;
        if(p1.y != p2.y)
    		p.x = ((p.y - p2.y)*(p1.x-p2.x))/(p1.y-p2.y)+p2.x;
    }
	//DEBUG_OUT("get_pt - end")
    return p;
}

//---------------------------------------------------------------------------
// если обе точки не принадлежат прямоугольнику, то нужно добавить две другие точки
// или ничего не добавлять (если нет пересечений с прямоугольником)
// (1) left_top |(2)top   |(3)right_top
//	____________|_________|________
//              |         |
//   (8) left   |    0    |(4)right
//  ____________|_________|________
//     (7)      |  (6)    |
//   left_bottom| bottom  |(5)right_bottom

int get_pt_quad(const POINT3D &p1, const RECT &br)
{
	if(bound(p1, br))
		return 0;
	bool p1lefter = p1.x < br.left;
	bool p1topper = p1.y < br.top;
	bool p1center26 = !p1lefter && (p1.x < br.right);
	bool p1center84 = !p1topper && (p1.y < br.bottom);
	if(p1lefter)
		if(p1topper)
			return 1;
		else if(p1center84)
			return 8;
		else
			return 7;
	else if(p1center26)
		if(p1topper)
			return 2;
		else
			return 6;
	else
	{
		if(p1topper)
			return 3;
		else if(p1center84)
			return 4;
		else
			return 5;
	}
}

//---------------------------------------------------------------------------
// нормализация точек. Если есть точки, выходящие за рамки экрана, то они "придвигаются" к краю
void normalize_pts(vector <POINT3D> &pts, vector <POINT3D> &new_pts, const RECT &br)
{
	//DEBUG_OUT("normalize_pts - begin")
	// если точек < 2, то ничего нормализовать не надо
    if(pts.size() < 2)
    {
        new_pts.assign(pts.begin(), pts.end());
		//DEBUG_OUT("normalize_pts - end < 2")
        return;
    }
    vector<POINT3D>::iterator pIter;
    POINT3D p1, p2;
    int b1, b2, pre_b1;
    pIter = pts.begin();
	// определяем две исследуемые точки
    p1 = *pIter;
    pIter++;
    p2 = *pIter;
	// определяем, лежат ли эти точки внутри прямоугольника
    b1 = get_pt_quad(p1, br);
    b2 = get_pt_quad(p2, br);
	pre_b1 = -1;
	new_pts.push_back(p1);
    while(1)
    {
		// переходим к следующей точке
        p1 = p2;
		pre_b1 = b1;
        b1 = b2;
		pIter++;
		if(pIter == pts.end())
		{
			new_pts.push_back(p1);
			break;
		}
        p2 = *pIter;
        b2 = get_pt_quad(p2, br);
		//
		if(pre_b1!=b1 || b1 != b2 || b1 == 0)
			new_pts.push_back(p1);
    }
}

// функция преобразовывает numeric в integer. Если numeric > INT_MAX, возвращается число INT_MAX/1000
int to_integer(const numeric& x)
{
	if(x >= numeric(INT_MAX/1000))
		return INT_MAX/1000;
	if(x <= numeric(INT_MIN/1000))
		return INT_MIN/1000;
	return int(x.to_double());
}

// для 3х-мерных графиков. Возвращает цвет в зависимости от высоты точки
long GetPointColor(POINT3D p, numeric dy, range rY, int bottom)
{
	numeric miny = -8, maxy = 13;
	numeric mingr = 64, maxgr = 168;
	numeric cury = (bottom - p.y)*dy + rY.rmin;

	numeric green = (maxgr-mingr)/rY.get()*(cury-rY.rmin)+mingr;
	numeric red = (maxy-miny)/rY.get()*(cury-rY.rmin)+miny;
	red = red*red + 64;
	return RGB(to_integer(red),to_integer(green),80);
}



