#include <stdexcept>
#include <sstream>
#include <fstream>

#include "..\\rows\\rows.h"
#include "..\\parser\\parser.h"

extern sym_table sym_tab;
extern expr_table expr_tab;

extern symbol GetSymbol(const string& aName);

//--------------- expr_table class -----------------------
GINAC_IMPLEMENT_REGISTERED_CLASS(expr_table, lst)

expr_table::expr_table()
{

}

/*void expr_table::copy(const expr_table &other)
{
	inherited::copy(other);
}*/

void expr_table::archive(archive_node &n) const
{
	inherited::archive(n);
}

expr_table::expr_table(const archive_node &n, lst &sym_lst) : inherited(n, sym_lst)
{
}

ex expr_table::unarchive(const archive_node &n, lst &sym_lst)
{
	return (new expr_table(n, sym_lst))->setflag(status_flags::dynallocated);
}

void expr_table::print(const print_context &c, unsigned level) const
{
	// print_context::s is a reference to an ostream
	c.s << "expressions(";
	inherited::print(c, level);
	c.s << ")";
}

int expr_table::compare_same_type(const basic &other) const
{
	return inherited::compare_same_type(other);
}

unsigned int expr_table::get_ind(const ex &aExpr)
{
    for(unsigned int i = 0; i < nops(); i++)
        if(op(i).is_equal(aExpr))
            return i;
    append(aExpr);
    return nops() - 1;
}



