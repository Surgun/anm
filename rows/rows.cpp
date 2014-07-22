#include <windows.h>
#include <stdexcept>
#include <sstream>
#include <fstream>

#include "..\\rows\\rows.h"
#include "..\\parser\\parser.h"
#include "..\\system\\anm.h"

extern symbol GetSymbol(const string& aName);
extern ex ExpandWithSymTab(ex&);
extern const ex Laplace_trans(const ex& expr);
extern const ex ex_to_ppolinom(const ex& expr);
extern const ex ex_to_taylor_row(const ex& expr, ANM_solutions* XNames, bool bFunction);

//--------------- row_pexp class -----------------------
GINAC_IMPLEMENT_REGISTERED_CLASS(row_pexp, basic)

row_pexp::row_pexp()
{
}

/*void row_pexp::copy(const row_pexp &other)
{
	inherited::copy(other);
	expression = other.expression;
	pexp = other.pexp;
}*/

void row_pexp::archive(archive_node &n) const
{
	inherited::archive(n);
	n.add_ex("ex", expression);
}

row_pexp::row_pexp(const archive_node &n, lst &sym_lst) : inherited(n, sym_lst)
{
}

ex row_pexp::unarchive(const archive_node &n, lst &sym_lst)
{
	return (new row_pexp(n, sym_lst))->setflag(status_flags::dynallocated);
}

int row_pexp::compare_same_type(const basic &other) const
{
	const row_pexp &o = static_cast<const row_pexp &>(other);
	if (expression.is_equal(o.expression))
        if(pexp == o.pexp)
		    return 0;
        else if(pexp < o.pexp)
    		return -1;
	if (expression.compare(o.expression) < 0)
		return -1;
	return 1;
}

void row_pexp::print(const print_context &c, unsigned level) const
{
	// print_context::s is a reference to an ostream
	c.s << "pexp(";
	c.s << expression;
	c.s << ", " << pexp;
	c.s << ")";
}

row_pexp::row_pexp(const ex &aExpr, const int &aPexp) : expression(aExpr), pexp(aPexp)
{
	if(is_a<row_pexp>(aExpr))
	{
		expression = aExpr.op(0);
		pexp = ex_to<numeric>(aExpr.op(1)).to_int() + aPexp;
	}
}

int row_pexp::degree(const ex &s) const
{
	if(s != s_p)
		return expression.degree(s);
	return pexp + expression.degree(s);
}

ex row_pexp::coeff(const ex & s, int n) const
{
	if(s != s_p)
		return (power(s_p, pexp)*expression).coeff(s, n);
	return expression.coeff(s, n-pexp);
}

ex row_pexp::subs(const exmap & m, unsigned int options) const
{
	return row_pexp(expression.subs(m,options), pexp);
}

//--------------- taylor_row class -----------------------
GINAC_IMPLEMENT_REGISTERED_CLASS(taylor_row, basic)

taylor_row::taylor_row()
{
}

/*void taylor_row::copy(const taylor_row &other)
{
	inherited::copy(other);
	expression = other.expression;
}*/

void taylor_row::archive(archive_node &n) const
{
	inherited::archive(n);
	n.add_ex("ex", expression);
}

taylor_row::taylor_row(const archive_node &n, lst &sym_lst) : inherited(n, sym_lst)
{
}

ex taylor_row::unarchive(const archive_node &n, lst &sym_lst)
{
	return (new taylor_row(n, sym_lst))->setflag(status_flags::dynallocated);
}

int taylor_row::compare_same_type(const basic &other) const
{
	const taylor_row &o = static_cast<const taylor_row &>(other);
	if (expression.is_equal(o.expression))
		return 0;
	else if(expression.compare(o.expression) < 0)
		return -1;
	else
		return 1;
}

void taylor_row::print(const print_context &c, unsigned level) const
{
	// print_context::s is a reference to an ostream
	c.s << (expression);
}

taylor_row::taylor_row(const ex &aExpr, ANM_solutions* sols)
{
	expression = ex_to_taylor_row(aExpr, sols);
}

int taylor_row::degree(const ex &s) const
{
    return expression.degree(s);
	if(s.is_equal(s_p))
		return -1;
	return inherited::degree(s);
}
ex taylor_row::coeff(const ex & s, int n) const
{
	//DEBUG_OUT("expression = " << expression)
	//DEBUG_OUT("expression.coeff(" << s << ", " << n << ") = " << expression.coeff(s, n))
	return expression.coeff(s, n);
}

//--------------- row_all class -----------------------
GINAC_IMPLEMENT_REGISTERED_CLASS(row_all, basic)

row_all::row_all()
{
}

/*void row_all::copy(const row_all &other)
{
	inherited::copy(other);
	expression = other.expression;
	cur_diff = other.cur_diff;
	Tk.assign(other.Tk.begin(), other.Tk.end());
}*/

void row_all::archive(archive_node &n) const
{
	inherited::archive(n);
	n.add_ex("ex", expression);
	n.add_ex("ex", cur_diff);
}

row_all::row_all(const archive_node &n, lst &sym_lst) : inherited(n, sym_lst)
{
}

ex row_all::unarchive(const archive_node &n, lst &sym_lst)
{
	return (new row_all(n, sym_lst))->setflag(status_flags::dynallocated);
}

int row_all::compare_same_type(const basic &other) const
{
	const row_all &o = static_cast<const row_all &>(other);
    return expression.compare(o.expression);
}

void row_all::print(const print_context &c, unsigned level) const
{
	// print_context::s is a reference to an ostream
	c.s << "rall(";
	c.s << expression;
	c.s << ")";
}

row_all::row_all(const ex &aExpr) : expression(aExpr), cur_diff(aExpr)
{
}

int row_all::degree(const ex &s) const
{
	if(s.is_equal(s_p))
		return -1;
	return inherited::degree(s);
}
ex row_all::coeff(const ex & s, int n) const
{
	if(s.is_equal(s_taylor))
	{
		if(n < 0)
			return 0;
		if(n == 0)
			return expression;
		while(n > int(Tk.size()) - 1)
		{
			if(is_zero(cur_diff))
				return 0;
			const_cast<row_all*>(this)->cur_diff = cur_diff.diff(s_t);
            const_cast<row_all*>(this)->Tk.push_back(cur_diff);
		}
		return Tk[n-1];
	}
	else if(s.is_equal(s_p))
		return coeff(s_taylor, -(n+1));
	else
		if(n == 0)
			return expression;
		else
			return 0;
}

ex row_all::subs(const exmap & m, unsigned int options)
{
	return row_all(expression.subs(m,options));
}

//--------------- row_mul class -----------------------
// класс произведения рядов. Преобразует произведения рядов в ряд
GINAC_IMPLEMENT_REGISTERED_CLASS(row_mul, mul)

row_mul::row_mul()
{
}

/*void row_mul::copy(const row_mul &other)
{
	inherited::copy(other);
	Tk.assign(other.Tk.begin(), other.Tk.end());
}*/

void row_mul::archive(archive_node &n) const
{
	inherited::archive(n);
}

row_mul::row_mul(const archive_node &n, lst &sym_lst) : inherited(n, sym_lst)
{
}

ex row_mul::unarchive(const archive_node &n, lst &sym_lst)
{
	return (new row_mul(n, sym_lst))->setflag(status_flags::dynallocated);
}

void row_mul::print(const print_context &c, unsigned level) const
{
	// print_context::s is a reference to an ostream
	c.s << "rmul(";
	inherited::print(c, level);
	c.s << ")";
}

int row_mul::compare_same_type(const basic &other) const
{
	return inherited::compare_same_type(other);
}

ex row_mul::GetSum(int i, int nj) const
{
	ex s = s_taylor;
	if(nj == int(nops() - 1))
		return op(nj).coeff(s, i);
	ex res;
	for(int j = 0; j <= i; j++)
		res += binomial(j,0)*op(nj).coeff(s, j)*binomial(i, j)*GetSum(i - j, nj+1);
	return res;
}

int row_mul::degree(const ex & s) const
{
	if(s != s_p)
		return inherited::degree(s);
	return -1;
}

ex row_mul::coeff(const ex & s, int n) const
{
	if(s.is_equal(s_taylor))
	{
		if(n < 0)
			return 0;
		while(n > int(Tk.size()) - 1)
			const_cast<row_mul*>(this)->Tk.push_back(const_cast<row_mul*>(this)->GetSum(int(Tk.size()), 0));
		return Tk[n];
	}
	else if(s.is_equal(s_p))
		return coeff(s_taylor, -(n+1));
	else
		return inherited::coeff(s,n);
}

ex row_mul::subs(const exmap & m, unsigned int options)
{
	return row_mul(ex_to<mul>(inherited::subs(m,options)));
}

//--------------- row_power class -----------------------
// класс возведения рядов в целую степень.

GINAC_IMPLEMENT_REGISTERED_CLASS(row_power, power)

row_power::row_power() : inherited()
{
}

/*void row_power::copy(const row_power &other)
{
	inherited::copy(other);
	Tk.assign(other.Tk.begin(), other.Tk.end());
}*/

void row_power::archive(archive_node &n) const
{
	inherited::archive(n);
}

void row_power::print(const print_context &c, unsigned level) const
{
	// print_context::s is a reference to an ostream
	c.s << "rpower(";
	inherited::print(c, level);
	c.s << ")";
}

row_power::row_power(const archive_node &n, lst &sym_lst) : inherited(n, sym_lst)
{
}

ex row_power::unarchive(const archive_node &n, lst &sym_lst)
{
	return (new row_power(n, sym_lst))->setflag(status_flags::dynallocated);
}

int row_power::compare_same_type(const basic &other) const
{
	return inherited::compare_same_type(other);
}

int row_power::degree(const ex & s) const
{
	if(s != s_p)
		return inherited::degree(s);
	return -1;
}

ex row_power::GetSum(int i, int nj) const
{
	ex s = s_taylor;
	if(!is_exactly_a<numeric>(op(1).evalf()))
		ERROR_15(op(1), op(0))
	if(nj == int(ex_to<numeric>(op(1).eval()).to_int() - 1))
		return op(0).coeff(s, i);
	ex res;
	for(int j = 0; j <= i; j++)
		res += binomial(j,0)*op(0).coeff(s, j)*binomial(i, j)*GetSum(i - j, nj+1);
	return res;
}

ex row_power::coeff(const ex & s, int n) const
{
	if(s.is_equal(s_taylor))
	{
		if(n < 0)
			return 0;
		while(n > int(Tk.size()) - 1)
			const_cast<row_power*>(this)->Tk.push_back(const_cast<row_power*>(this)->GetSum(int(Tk.size()), 0));
		return Tk[n];
	}
	else if(s.is_equal(s_p))
		return coeff(s_taylor, -(n+1));
	else
		return inherited::coeff(s,n);
}

ex row_power::subs(const exmap & m, unsigned int options)
{
	ex tmpEx = inherited::subs(m,options);
	return row_power(tmpEx.op(0), tmpEx.op(1));
}

//--------------- row_power_z class -----------------------
// класс возведения рядов в целую степень.

GINAC_IMPLEMENT_REGISTERED_CLASS(row_power_z, power)

row_power_z::row_power_z() : inherited()
{
}

/*void row_power_z::copy(const row_power_z &other)
{
	inherited::copy(other);
	Tk.assign(other.Tk.begin(), other.Tk.end());
}*/

void row_power_z::archive(archive_node &n) const
{
	inherited::archive(n);
}

void row_power_z::print(const print_context &c, unsigned level) const
{
	// print_context::s is a reference to an ostream
	c.s << "rpower_z(";
	inherited::print(c, level);
	c.s << ")";
}

row_power_z::row_power_z(const archive_node &n, lst &sym_lst) : inherited(n, sym_lst)
{
}

ex row_power_z::unarchive(const archive_node &n, lst &sym_lst)
{
	return (new row_power_z(n, sym_lst))->setflag(status_flags::dynallocated);
}

int row_power_z::compare_same_type(const basic &other) const
{
	return inherited::compare_same_type(other);
}

int row_power_z::degree(const ex & s) const
{
	if(s != s_p)
		return inherited::degree(s);
	return -1;
}

ex row_power_z::evalf(int ind)
{
	if(Digits < ind*10)
		Digits = ind*10;
	if(ind < 0)
		return 0;
	while(ind > int(numTk.size()) - 1)
	{
		numTk.push_back(/*ex_to<numeric>*/(coeff(s_taylor, numTk.size()).evalf()));
		//DEBUG_OUT("numTk[" + ToString(ind) + "] = " << numTk[ind])
	}
	if(ind == 0)
		if(! coeff(s_taylor, 0).evalf().is_equal(numTk[ind]))
		{
			numTk.clear();
			return evalf(ind);
		}
	return numTk[ind];
}

ex row_power_z::GetSum(int i)
{
	ex s = s_taylor;
	if(op(0).coeff(s, 0).is_zero())
		return 0;
	if(i)
	{
		ex res;
		for(int j = 0; j < i; j++)
		{
			// для более долгого, но более точного вычисления нужно раскомментировать первую строку. Иначе вторую...
			//res += Tk[j]*op(0).coeff(s, i-j)*(binomial(i-1, j)*op(1) - binomial(i-1, j-1));
			res += sym_tk(this, j)*op(0).coeff(s, i-j)*(binomial(i-1, j)*op(1) - binomial(i-1, j-1));
		}
		return (res/op(0).coeff(s, 0));
	}
	else
		return pow(op(0).coeff(s, 0), op(1));
}

ex row_power_z::coeff(const ex &s, int n) const
{
	if(s.is_equal(s_taylor))
	{
		if(n < 0)
			return 0;
		//DEBUG_OUT(op(0) << "^(" << op(1) << ").Tk.size() = " << Tk.size())
		while(n > int(Tk.size()) - 1)
		{
			const_cast<row_power_z*>(this)->Tk.push_back(const_cast<row_power_z*>(this)->GetSum(Tk.size()));
		}
		//DEBUG_OUT("Tk[" << n << "] = " << Tk[n])
		//DEBUG_OUT("Tk[" << n << "].evalf = " << Tk[n].evalf())
		return Tk[n];
	}
	else if(s.is_equal(s_p))
		return coeff(s_taylor, -(n+1));
	else
		return inherited::coeff(s,n);
}

//------------------------------ sub_val class----------------------------------------------------
GINAC_IMPLEMENT_REGISTERED_CLASS(sub_val, basic)

sub_val::sub_val()
{
}

void sub_val::archive(archive_node &n) const
{
	inherited::archive(n);
}

sub_val::sub_val(const archive_node &n, lst &sym_lst) : inherited(n, sym_lst)
{
}

void sub_val::print(const print_context &c, unsigned level) const
{
	if(ToString(printName)==ToString(name))
	{
		if(!is_a<myprocedure>(val) && val.nops())
			c.s << printName << "->(" << val << ")";
		else
			c.s << printName << "->" << val;
	}
	else
		c.s << printName;
//	c.s << " " << (ex_to<myprocedure>(name).sptr) << "\x0D\x0A";
}

size_t sub_val::nops() const{
	return size_t(2);
}

ex sub_val::op(size_t i) const{
	GINAC_ASSERT(i<nops());
	if(i)
		return val;
	else
		return name;
}

ex& sub_val::let_op(size_t i){
	GINAC_ASSERT(i<nops());
	ensure_if_modifiable();
	if(i)
		return val;
	else
		return name;
}

ex sub_val::evalf(int level) const
{
	if(is_a<unknown_sol>(name))
	{
		solution *sptr = ex_to<unknown_sol>(name).sptr;
		if(sptr)
		{
			if(val.is_equal(0))
				return sptr->evalf();
// Переменные класса solution
			if(val.is_equal(GetSymbol("mu_min")))
				return sptr->mu_min;
			if(val.is_equal(GetSymbol("mu_max")))
				return sptr->mu_max;
			if(val.is_equal(GetSymbol("xI")))
				return sptr->xI;
			if(val.is_equal(GetSymbol("kappa")))
				return sptr->kappa;
			if(val.is_equal(GetSymbol("GlobalD")))
				return sptr->GlobalD;
			if(val.is_equal(GetSymbol("LocalD")))
				return sptr->LocalD;
			if(val.is_equal(GetSymbol("MaxRInd"))||val.is_equal(GetSymbol("m")))
				return sptr->MaxRInd;
			if(val.is_equal(GetSymbol("q")))
				return sptr->q;
			if(val.is_equal(GetSymbol("TypeOfMaxR")))
				return sptr->TypeOfMaxR;
			if(val.is_equal(GetSymbol("h")))
				return *(sptr->step);
// Переменные-вектора класса solution
			if(is_a<myprocedure>(val))
			{
				if(val.op(0).is_equal(GetSymbol("R")))	// R[i]
					return sptr->numR(ex_to<numeric>(val.op(1).op(0)).to_int());
				if(val.op(0).is_equal(GetSymbol("y")))	// y[i]=-R(i+2)/R(i) {yi = -Ri2/Ri}
				{
					try{
						numeric yi, Ri = sptr->numR(ex_to<numeric>(val.op(1).op(0)).to_int());
						numeric Ri2 = sptr->numR(ex_to<numeric>(val.op(1).op(0)).to_int() + 2);
						yi = -Ri2/Ri;
						return yi;
					} catch (const exception &e) {
						return 0;
					}
				}
			}
		}
	}
	//DEBUG_OUT("sub_val = " << *this)
	ex ret;
	if(val.is_equal(0))
		ret = name.evalf(level);
	else
	{
		try{
			//DEBUG_OUT("val = " << val)
			//DEBUG_OUT("evalf(val) = " << val.evalf())
			ret = val.evalf(level);
		} catch (const exception &e) {
			return 0;
		}
	}
	if(is_a<numeric>(ret))
		return ret;
	return 0;
}

ex sub_val::unarchive(const archive_node &n, lst &sym_lst)
{
	return (new sub_val(n, sym_lst))->setflag(status_flags::dynallocated);
}

int sub_val::compare_same_type(const basic &other) const
{
	const sub_val &o = static_cast<const sub_val &>(other);
	if (name.is_equal(o.name))
        return val.compare(o.val);
	if(name.compare(o.name) < 0)
		return -1;
	return 1;
}

//------------------------------ sym_tk class----------------------------------------------------
GINAC_IMPLEMENT_REGISTERED_CLASS(sym_tk, basic)

sym_tk::sym_tk()
{
}

sym_tk::sym_tk(row_power_z *aName, const int &aInd)
{
	name = aName;
	ind = aInd;
}

/*void sym_tk::copy(const sym_tk &other)
{
	inherited::copy(other);
	name = other.name;
	ind = other.ind;
}*/

void sym_tk::print(const print_context &c, unsigned level) const
{
	c.s << "Tk[" << ind << "]";
//	c.s << (*name) << ".R[" << ind << "]";
}
void sym_tk::archive(archive_node &n) const
{
	inherited::archive(n);
}

sym_tk::sym_tk(const archive_node &n, lst &sym_lst) : inherited(n, sym_lst)
{
}

ex sym_tk::unarchive(const archive_node &n, lst &sym_lst)
{
	return (new sym_tk(n, sym_lst))->setflag(status_flags::dynallocated);
}

int sym_tk::compare_same_type(const basic &other) const
{
	const sym_tk &o = static_cast<const sym_tk &>(other);
	if ((*name).is_equal(*(o.name)))
        if(ind == o.ind)
		    return 0;
        else if(ind < o.ind)
    		return -1;
	if((*name).compare(*(o.name)) < 0)
		return -1;
	return 1;
}

ex sym_tk::evalf(int level) const
{
	return name->evalf(ind);
}
//------------------------------ cur_tk class----------------------------------------------------
GINAC_IMPLEMENT_REGISTERED_CLASS(cur_tk, basic)

cur_tk::cur_tk()
{
}

cur_tk::cur_tk(numeric *aCur_t)
{
	cur_t = aCur_t;
}

/*void cur_tk::copy(const cur_tk &other)
{
	inherited::copy(other);
	cur_t = other.cur_t;
}*/

void cur_tk::print(const print_context &c, unsigned level) const
{
	c.s << "cur_tk(" << (*cur_t) << ")";
}
void cur_tk::archive(archive_node &n) const
{
	inherited::archive(n);
}

cur_tk::cur_tk(const archive_node &n, lst &sym_lst) : inherited(n, sym_lst)
{
}

ex cur_tk::unarchive(const archive_node &n, lst &sym_lst)
{
	return (new cur_tk(n, sym_lst))->setflag(status_flags::dynallocated);
}

int cur_tk::compare_same_type(const basic &other) const
{
	const cur_tk &o = static_cast<const cur_tk &>(other);
	if (cur_t == o.cur_t)
	    return 0;
	if (*cur_t < *(o.cur_t))
		return -1;
	return 1;
}

ex cur_tk::evalf(int level) const
{
	return *cur_t;
}

//------------------------------ sym_rk class----------------------------------------------------
GINAC_IMPLEMENT_REGISTERED_CLASS(sym_rk, basic)

sym_rk::sym_rk()
{
    sptr = 0;
}

/*void sym_rk::copy(const sym_rk &other)
{
	inherited::copy(other);
	sptr = other.sptr;
	ind = other.ind;
}*/

void sym_rk::archive(archive_node &n) const
{
	inherited::archive(n);
}

sym_rk::sym_rk(const archive_node &n, lst &sym_lst) : inherited(n, sym_lst)
{
}

ex sym_rk::unarchive(const archive_node &n, lst &sym_lst)
{
	return (new sym_rk(n, sym_lst))->setflag(status_flags::dynallocated);
}

int sym_rk::compare_same_type(const basic &other) const
{
	const sym_rk &o = static_cast<const sym_rk &>(other);
    if(!sptr)
        return -1;
    if(!o.sptr)
        return 1;
	if(sptr->name.is_equal(o.sptr->name) && ind == o.ind)
		return 0;
	if(sptr->name.is_equal(o.sptr->name) && ind < o.ind)
		return -1;
	if(sptr->name.compare(o.sptr->name) < 0)
		return -1;
	return 1;
}

void sym_rk::print(const print_context &c, unsigned level) const
{
	c.s << (sptr->name) << ".R[" << ind << "]";
}

ex sym_rk::evalf(int level) const
{
    if(sptr)
        return sptr->numR(ind);
    else
        return 0;
}

ex sym_rk::eval(int level) const
{
	//DEBUG_OUT("iHold = " << sptr->iHold)
	//DEBUG_OUT("ind = " << ind << "; size = " << sptr->Rk.size())
    if(!sptr || ind < 0)
        return 0;
	else if(sptr->iHold < ind)
		return hold();
	else
		return sptr->R(ind);//hold();
}

//------------------------------ sym_IC class ----------------------------------------------------
GINAC_IMPLEMENT_REGISTERED_CLASS(sym_IC, sym_rk)

sym_IC::sym_IC() : sym_rk()
{
	sptr = 0;
}

/*void sym_IC::copy(const sym_IC &other)
{
	inherited::copy(other);
	sptr = other.sptr;
	ind = other.ind;
}*/

void sym_IC::archive(archive_node &n) const
{
	inherited::archive(n);
}

sym_IC::sym_IC(const archive_node &n, lst &sym_lst) : inherited(n, sym_lst)
{
}

ex sym_IC::unarchive(const archive_node &n, lst &sym_lst)
{
	return (new sym_IC(n, sym_lst))->setflag(status_flags::dynallocated);
}

int sym_IC::compare_same_type(const basic &other) const
{
	const sym_IC &o = static_cast<const sym_IC &>(other);
	if (sptr->name.is_equal(o.sptr->name) && ind == o.ind)
		return 0;
	if(sptr->name.compare(o.sptr->name) < 0)
		return -1;
	if(sptr->name.is_equal(o.sptr->name) && ind < o.ind)
		return -1;
	return 1;
}

ex sym_IC::derivative(const symbol &s) const
{
    if(s.is_equal(s_t))
        return sym_IC(sptr, ind+1);
    return myprocedure(s_diff, lst(*this,s));
}

ex sym_IC::evalf(int level) const{
    //DEBUG_OUT(sptr->numIC(ind) << "\x0D\x0A";
	return sptr->numIC(ind);
}




