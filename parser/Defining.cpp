#include <stdexcept>
#include <sstream>
#include <fstream>

#include "..\\rows\\rows.h"
#include "..\\parser\\parser.h"

extern sym_table sym_tab;
extern expr_table expr_tab;

extern symbol GetSymbol(const string& aName);

//--------------- defining class -----------------------
GINAC_IMPLEMENT_REGISTERED_CLASS(defining, basic)

defining::defining()
{
}

defining::defining(const ex &aProc, const ex &aValue) : proc(aProc), value(aValue)
{
}

/*void defining::copy(const defining &other)
{
	inherited::copy(other);
	proc = other.proc;
	value = other.value;
}
*/

void defining::archive(archive_node &n) const
{
	inherited::archive(n);
	n.add_ex("ex", proc);
	n.add_ex("ex", value);
}

defining::defining(const archive_node &n, lst &sym_lst) : inherited(n, sym_lst)
{
}

ex defining::unarchive(const archive_node &n, lst &sym_lst)
{
	return (new defining(n, sym_lst))->setflag(status_flags::dynallocated);
}

int defining::compare_same_type(const basic &other) const
{
	const defining &o = static_cast<const defining &>(other);
	if (proc.is_equal(o.proc) && value.is_equal(o.value))
		return 0;
	if (proc.compare(o.proc) < 0)
		return -1;
	if (proc.is_equal(o.proc) && value.compare(o.value) < 0)
		return -1;
	return 1;
}

void defining::print(const print_context &c, unsigned level) const
{
	// print_context::s is a reference to an ostream
	c.s << (proc) << " = " << (value);
}

ex defining::evalf(int level) const
{
	return defining(proc.evalf(level), value.evalf(level));
}



