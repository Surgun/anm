#include <stdexcept>
#include <sstream>
#include <fstream>

#include "..\\rows\\rows.h"
#include "..\\parser\\parser.h"

extern sym_table sym_tab;
extern expr_table expr_tab;

extern symbol GetSymbol(const string& aName);

//--------------- myprocedure class-----------------------
GINAC_IMPLEMENT_REGISTERED_CLASS(myprocedure, basic)
myprocedure::myprocedure()
{
}

myprocedure::myprocedure(const ex &aName, const ex &aParamsLst) : name(aName), params(aParamsLst)
{
}

/*void myprocedure::copy(const myprocedure &other)
{
	inherited::copy(other);
	name = other.name;
	params = other.params;
}*/

void myprocedure::archive(archive_node &n) const
{
	inherited::archive(n);
	n.add_ex("symbol", name);
	string outstr = "(";
	for(int i = 0; i < params.nops(); i++)
	{
		outstr += ToString(params.op(i));
		if(i < params.nops() - 1)
			outstr += ",";
	}
	outstr += ")";
	n.add_string("string", outstr);
}

myprocedure::myprocedure(const archive_node &n, lst &sym_lst) : inherited(n, sym_lst)
{
}

ex myprocedure::unarchive(const archive_node &n, lst &sym_lst)
{
	return (new myprocedure(n, sym_lst))->setflag(status_flags::dynallocated);
}

int myprocedure::compare_same_type(const basic &other) const
{
	const myprocedure &o = static_cast<const myprocedure &>(other);
	if (name.is_equal(o.name) && params.is_equal(o.params))
		return 0;
	if(name.compare(o.name) < 0)
		return -1;
	if(name.compare(o.name) > 0)
		return 1;
	if(params.compare(o.params) < 0)
		return -1;
	return 1;
}

void myprocedure::print(const print_context &c, unsigned level) const
{
	// print_context::s is a reference to an ostream
	c.s << name;
	if(params.nops())
	{
		c.s << "(";
		for(int i = 0; i < params.nops(); i++)
		{
			c.s << (params.op(i));
			if(i < params.nops() - 1)
				c.s << ",";
		}
		c.s << ")";
	}
}

ex myprocedure::evalf(int level) const
{
/*	if(name.is_equal(GetSymbol("R")) && params.nops() == 2)
	{
		return ex_to_taylor_row(params.op(1).evalf(),0).coeff(s_taylor, ex_to<numeric>(params.op(0)).to_int());
	}
*/
	//DEBUG_OUT("myprocedure = " << (*this))
	if(name.is_equal(s_diff))
	{
		if(!params.op(0).evalf(level).is_equal(params.op(0)))
			return (params.op(0).evalf(level).diff(ex_to<symbol>(params.op(1))));
		return this->hold();
	}
	return sym_tab.Evalf(*this, level);
}

ex myprocedure::derivative(const symbol &s) const
{
	// Если это уже производная
	if(name.is_equal(s_diff) && !params.op(1).is_equal(s))
	{
		bool bNeedDiff = true;
		ex tmpDiff = params.op(0).diff(s);
		if(is_a<myprocedure>(tmpDiff))
			if(tmpDiff.op(0).is_equal(s_diff))
				bNeedDiff = false;
		if(bNeedDiff)
			return params.op(0).diff(s).diff(ex_to<symbol>(params.op(1)));
	}
	return sym_tab.Derivative(*this, s);
	//return myprocedure(s_diff, lst(*this,s));
}

size_t myprocedure::nops() const{
	return size_t(2);
}

ex myprocedure::op(size_t i) const{
	GINAC_ASSERT(i<nops());
	if(i)
		return params;
	else
		return name;
}

ex myprocedure::subs(const exmap & m, unsigned options) const
{
	//DEBUG_OUT("m = ?")
	ex new_params = params.subs(m, options);
	for (exmap::const_iterator it = m.begin(); it != m.end(); ++it) {
		if(is_a<myprocedure>(it->first) && (it->first).op(0) == name && !((it->first).is_equal(it->second)))
			return (it->second).subs( lst((it->first).op(1)) , lst(params) ,options);
		if (is_equal(ex_to<basic>((it->first))))
			return (it->second);
	}
	return myprocedure(name, new_params);
}

ex myprocedure::coeff(const ex & s, int n) const
{
	extern int to_integer(const numeric &x);
	if(s.is_equal(s_taylor))
	{
		if(n < 0)
			return 0;
		//DEBUG_OUT("this = " << (*this) << "; n = " << n)
		if(name.is_equal(s_diff))
		{
			if(params.nops() > 2 && is_a<numeric>(params.op(2)))
				return params.op(0).coeff(s, n+to_integer(ex_to<numeric>(params.op(2))));
			else
				return params.op(0).coeff(s, n+1);
		}
		if(name.is_equal(GetSymbol("int")))
			return params.op(0).coeff(s, n-1);
		if(evalf().is_equal(*this))
			return myprocedure(GetSymbol("R"), lst(n, name));
		//DEBUG_OUT((*this) << ".coeff(" << s << ", " << n << ")")
		return ex_to_taylor_row(evalf(),0).coeff(s, n);
/*
		if(name.is_equal(GetSymbol("R")))
		{
			return ex_to_taylor_row(params.op(1).evalf(),0).coeff(s_taylor, ex_to<numeric>(params.op(0)).to_int());
		}
		return myprocedure(GetSymbol("R"), lst(n, *this));
*/
	}
	else if(s.is_equal(s_p))
	{
//		return *this;
		//DEBUG_OUT((*this) << ".coeff(" << s << ", " << n << ")")
		if(is_a<numeric>(evalf()))
			if(n == 0)
				return evalf();
			else
				return 0;
		return coeff(s_taylor, -(n+1));
	}
	else
		return inherited::coeff(s,n);
}

ex& myprocedure::let_op(size_t i)
{
	GINAC_ASSERT(i<nops());

	ensure_if_modifiable();

	if(i)
		return params;
	else
		return name;
}


