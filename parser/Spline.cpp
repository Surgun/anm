#include <stdexcept>
#include <sstream>
#include <fstream>

#include "..\\rows\\rows.h"
#include "..\\parser\\parser.h"

extern sym_table sym_tab;
extern expr_table expr_tab;

extern symbol GetSymbol(const string& aName);

//--------------- spline function -----------------------
// в параметрах функции есть набор интервалов, набор коэффициентов (для каждого интервала набор коэффициентов д.б. одинакового количества)
// а также, значение value, которое должно лежать в нужном интервале
ex spline(const lst &intervals, const lst &coeffs, const ex &value)
{
	ex ret_value = 0;
	int i;
    for(int k = 0; k < coeffs.op(0).nops(); k++)
    {
        lst cur_coeffs;
        for(int i = 0; i < intervals.nops(); i++)
            cur_coeffs.append(coeffs.op(i).op(k));
        //DEBUG_OUT(intervals);
        if(k == 0)
            ret_value = spline_coeff(intervals, cur_coeffs, value);
        else
            ret_value += pow(value, ex(k)) * spline_coeff(intervals, cur_coeffs, value);
    }
    //DEBUG_OUT(ret_value << "\x0D\x0A";
	return ret_value;
}

//--------------- spline_coeff class-----------------------
GINAC_IMPLEMENT_REGISTERED_CLASS(spline_coeff, basic)

spline_coeff::spline_coeff()
{
}

spline_coeff::spline_coeff(const lst &aIntervals, const lst &aCoeffs, const ex &aValue) : intervals(aIntervals), coeffs(aCoeffs), value(aValue)
{
}

/*void spline_coeff::copy(const spline_coeff &other)
{
	inherited::copy(other);
	intervals = other.intervals;
	coeffs = other.coeffs;
    value = other.value;
}*/

void spline_coeff::archive(archive_node &n) const
{
	inherited::archive(n);
}

spline_coeff::spline_coeff(const archive_node &n, lst &sym_lst) : inherited(n, sym_lst)
{
}

ex spline_coeff::unarchive(const archive_node &n, lst &sym_lst)
{
	return (new spline_coeff(n, sym_lst))->setflag(status_flags::dynallocated);
}

int spline_coeff::compare_same_type(const basic &other) const
{
	const spline_coeff &o = static_cast<const spline_coeff &>(other);
	if (intervals.is_equal(o.intervals) && coeffs.is_equal(o.coeffs) && value.is_equal(o.value))
		return 0;
	if(intervals.compare(o.intervals) < 0)
		return -1;
	if(coeffs.compare(o.coeffs) < 0)
		return -1;
	if(value.compare(o.value) < 0)
		return -1;
	return 1;
}

void spline_coeff::print(const print_context &c, unsigned level) const
{
	// print_context::s is a reference to an ostream
	c.s << "spline_coeff(" << (intervals) << ", " << (coeffs) << ", " << (value) << ")";
}

ex spline_coeff::subs(const exmap & m, unsigned options) const
{
	ex new_value = value.subs(m,options);
	return spline_coeff(intervals, coeffs, new_value);
}

ex spline_coeff::coeff(const ex & s, int n) const
{
	if(s.is_equal(s_taylor))
    {
        if(n == 0)
		    return *this;
        else
            return 0;
    }
	else if(s.is_equal(s_p))
		return coeff(s_taylor, -(n+1));
	else
		return inherited::coeff(s, n);
}

ex spline_coeff::evalf(int level) const
{
	ex cur_val = value.evalf(level);
	int i;
	for(i = 0; i < intervals.nops() - 1; i++)
    {
		if(cur_val <= intervals.op(i).op(1))
			break;
    }
	return coeffs.op(i);
}

size_t spline_coeff::nops() const{
	return size_t(3);
}

ex spline_coeff::op(size_t i) const{
	GINAC_ASSERT(i<nops());

    if(2 == i)
		return value;
	if(1 == i)
		return coeffs;
	else
		return intervals;
}

/*ex& spline::let_op(size_t i)
{
	GINAC_ASSERT(i<nops());

	ensure_if_modifiable();

	if(2 == i)
		return value;
	if(1 == i)
		return intervals;
	else
		return coeffs;
}
*/

