#include <stdexcept>
#include <sstream>
#include <fstream>

#include "..\\rows\\rows.h"
#include "..\\parser\\parser.h"

extern sym_table sym_tab;
extern expr_table expr_tab;

extern symbol GetSymbol(const string& aName);

//--------------- logic_op class -----------------------
GINAC_IMPLEMENT_REGISTERED_CLASS(logic_op, lst)

logic_op::logic_op()
{
    o = log_or;
}

/*void logic_op::copy(const logic_op &other)
{
	inherited::copy(other);
    o = other.o;
}*/

void logic_op::archive(archive_node &n) const
{
	inherited::archive(n);
}

logic_op::logic_op(const archive_node &n, lst &sym_lst) : inherited(n, sym_lst)
{
}

ex logic_op::unarchive(const archive_node &n, lst &sym_lst)
{
	return (new logic_op(n, sym_lst))->setflag(status_flags::dynallocated);
}

void logic_op::print(const print_context &c, unsigned level) const
{
	// print_context::s is a reference to an ostream
	c.s << "(";
    if(o == log_or)
        c.s << "or:";
    if(o == log_and)
        c.s << "and:";
    if(o == log_not)
        c.s << "not:";
	inherited::print(c, level);
	c.s << ")";
}

int logic_op::compare_same_type(const basic &other) const
{
	const logic_op &oth = static_cast<const logic_op &>(other);
    if(oth.o == o)
	    return inherited::compare_same_type(other);
    if(oth.o == log_and)
        return -1;
    return 1;
}

bool logic_op::to_bool() const
{
    bool res = (o == log_and);
    for(int i = 0; i < nops(); i++)
    {
        bool cur_res;
        if(is_a<relational>(op(i)))
        {
            relational tmpRel = ex_to<relational>(op(i));
            cur_res = !(!tmpRel);
        }
        else if(is_a<logic_op>(op(i)))
            cur_res = ex_to<logic_op>(op(i)).to_bool();
        else
        {
            if(is_a<numeric>(op(i).evalf()))
                cur_res = !op(i).evalf().is_zero();
            else
                ERROR_26(op(i));
        }
        if(o == log_not)
            cur_res = !cur_res;
        (o == log_and)?(res *= cur_res):(res += cur_res);
    }
    return res;
}





