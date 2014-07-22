#include <stdexcept>
#include <sstream>
#include <fstream>

#include "..\\rows\\rows.h"
#include "..\\parser\\parser.h"

extern sym_table sym_tab;
extern expr_table expr_tab;

extern symbol GetSymbol(const string& aName);

//-------------------- sym_table_element class --------------------------------------------------------//
sym_table_element::sym_table_element(const string &aName)
{
	name = GetSymbol(aName);
	value = name;
	func.name = GetSymbol(aName);
}
sym_table_element::sym_table_element(const string &aName, const ex &aValue)
{
    name = GetSymbol(aName);
	value = aValue;
    if(!value.is_equal(name))
        if(ExpandWithSymTab(value).has(name))
           ERROR_27(name)
	func.name = GetSymbol(aName);
}
sym_table_element::sym_table_element(const symbol &aName, const ex &aValue)
{
    //DEBUG_OUT("sym_table_element::sym_table_element(const symbol &aName, const ex &aValue)")
    //DEBUG_OUT("name = " << aName)
    //DEBUG_OUT("value = " << aValue)
	name = aName;
	value = aValue;
    if(!value.is_equal(name))
        if(ExpandWithSymTab(value).has(name))
            ERROR_27(aName)
	func.name = aName;
}
sym_table_element::sym_table_element(const myprocedure &aName, const ex &aValue)
{
	func = aName;
	//Rk.clear();
	value = aValue;
    if(!value.is_equal(aName))
        if(ExpandWithSymTab(value).has(aName))
            ERROR_27(aName)
	name = ex_to<symbol>(aName.name);
}

sym_table_element::sym_table_element(const sym_table_element &Copy)
{
	name = Copy.name;
	value = Copy.value;
	func = Copy.func;
	//Rk.assign(Copy.Rk.begin(), Copy.Rk.end());
}

//----------------------------------------------------------------------------//
sym_table&
sym_table::append(const string& symName)
{
	if(is_name(symName) == -1)
		push_back(sym_table_element(symName));
	return *this;
}

sym_table&
sym_table::append(const string& symName, const ex& symValue)
{
    return append(GetSymbol(symName), symValue);
}

sym_table&
sym_table::append(const symbol& symName, const ex& symValue)
{
	int i = is_name(symName.get_name());
	if(i == -1)
		push_back(sym_table_element(symName, symValue));
	else
	{
        ex tmpValue = symValue;
        if(!tmpValue.is_equal(symName))
            if(ExpandWithSymTab(tmpValue).has(symName))
                ERROR_27(symName)
		(*this)[i] = tmpValue;
		(*this)[i].func = myprocedure();
	}
	return *this;
}

sym_table&
sym_table::append(const symbol& symName)
{
	if(is_name(symName.get_name()) == -1)
		push_back(sym_table_element(symName, symName));
	return *this;
}

sym_table&
sym_table::append(const sym_table_element& elem)
{
	int i = is_name(elem.name.get_name());
	if(i == -1)
		push_back(elem);
	else
		(*this)[i] = elem;
	return *this;
}

sym_table&
sym_table::append(const myprocedure& symName, const ex& symValue)
{
	int i = is_name(ex_to<symbol>(symName.name).get_name());
	if(i == -1)
		push_back(sym_table_element(symName, symValue));
	else/* if((*this)[i].is_func(symName))*/
		(*this)[i] = sym_table_element(symName, symValue);
	return *this;
}

lst sym_table::GetSymList()
{
	lst SymList;
	vector <sym_table_element>::iterator vIter;
	for ( vIter = begin( ) ; vIter != end( ) ; vIter++ )
		if(!(*vIter).value.is_equal((*vIter).name))
			SymList.append((*vIter).name);
	//DEBUG_OUT("SymList = " << SymList)
	return SymList;
}

lst sym_table::GetValList()
{
	lst ValList;
	vector <sym_table_element>::iterator vIter;
	for ( vIter = begin( ) ; vIter != end( ) ; vIter++ )
		if(!(*vIter).value.is_equal((*vIter).name))
			ValList.append((*vIter).value);
	//DEBUG_OUT("ValList = " << ValList)
	return ValList;
}

exmap sym_table::GetMap()
{
	exmap m;
	for (vector <sym_table_element>::iterator vIter = begin( ) ; vIter != end( ) ; vIter++ )
		if(!(*vIter).value.is_equal((*vIter).name))
			m.insert(std::make_pair((*vIter).name, (*vIter).value));
	return m;
}

exmap sym_table::GetSymMap()
{
	exmap m;
	for (vector <sym_table_element>::iterator vIter = begin( ) ; vIter != end( ) ; vIter++ )
		if(!(*vIter).value.is_equal((*vIter).name) && !(is_a<numeric>((*vIter).value)))
			m.insert(std::make_pair((*vIter).name, (*vIter).value));
	return m;
}

int sym_table::is_name(const string& aName)
{
	for ( int i=0; i < size() ; i++)
	{
		if((*this)[i].name.get_name() == aName)
			return i;
	}
	return -1;
}

numeric sym_table::Evalf(const string& aName)
{
	for ( int i=0; i < size() ; i++)
	{
		if((*this)[i].name.get_name() == aName)
			if(is_a<numeric>(((*this)[i].name).subs(GetMap()).eval()))
				return ex_to<numeric>(((*this)[i].name).subs(GetMap()).eval());
			else
				return 0;
	}
	return 0;
}

string
sym_table::GetStrData()
{

	string str;// = "[Symbol table]\n";
	vector <sym_table_element>::iterator vIter;
	for ( vIter = begin( ); vIter != end( ) ; vIter++ )
	{
		str += ToString((*vIter).name) + " = " + ToString((*vIter).value)+ "\n";
	}
	return str;
}

ex sym_table::Evalf(const myprocedure& proc, int level)
{
	vector <sym_table_element>::iterator vIter;
	for(vIter=begin(); vIter<end(); vIter++)
		if((*vIter).is_func(proc))
			if(!is_zero((*vIter).value - (*vIter).name))
            {
                ex tmpEx = ((*vIter).value.subs(ex_to<lst>((*vIter).func.params), ex_to<lst>(proc.params)));
				//DEBUG_OUT("sym_table::Evalf.tmpEx = " << tmpEx)
                ExpandWithSymTab(tmpEx);
				return tmpEx.evalf(level);
            }
	return proc;//.hold();
}

ex sym_table::get_procedure_body(const ex& tmpProc)
{
    myprocedure proc;
    if(is_a<myprocedure>(tmpProc))
        proc = ex_to<myprocedure>(tmpProc);
    else
        return tmpProc;
	vector <sym_table_element>::iterator vIter;
	for(vIter=begin(); vIter<end(); vIter++)
		if((*vIter).is_func(proc))
			if(!is_zero((*vIter).value - (*vIter).name))
				return ((*vIter).value.subs(ex_to<lst>((*vIter).func.params), ex_to<lst>(proc.params)));
	return tmpProc;//.hold();
}

ex sym_table::Eval(const myprocedure& proc, int level)
{
	cout << "error" << "\x0D\x0A" << flush;
	vector <sym_table_element>::iterator vIter;
	for(vIter=begin(); vIter<end(); vIter++)
		if((*vIter).is_func(proc))
			if(!is_zero((*vIter).value - (*vIter).func))
				return ((*vIter).value.subs(ex_to<lst>((*vIter).func.params), ex_to<lst>(proc.params))).eval(level);
	return proc;//.hold();
}

ex sym_table::Derivative(const myprocedure& proc, const symbol &s)
{
	vector <sym_table_element>::iterator vIter;
	for(vIter=begin(); vIter<end(); vIter++)
		if((*vIter).is_func(proc))
			if(!is_zero((*vIter).value - (*vIter).func) && ToString((*vIter).value) != ToString((*vIter).func))
			{
				//DEBUG_OUT("(*vIter).value = " << (*vIter).value)
				//DEBUG_OUT("(*vIter).func = " << (*vIter).func)
				return diff(((*vIter).value.subs(ex_to<lst>((*vIter).func.params), ex_to<lst>(proc.params))), s);
			}
	//diff(u(t), u) = 1;
	if(proc.op(0).is_equal(s))
		return 1;
	// если среди параметров нет символа дифференцирования, то возвращаем 0, например diff(i(t), u) = 0;
	if(!proc.op(1).has(s))
		return 0;
	return myprocedure(s_diff, lst(proc,s));
}





