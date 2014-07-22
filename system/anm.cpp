#include "..\\rows\\rows.h"
#include "..\\parser\\parser.h"
#include "..\\chart\\chart.h"

extern expr_table expr_tab;
extern sym_table sym_tab;
extern const ex Laplace_trans(const ex& expr);
extern const ex ex_to_ppolinom(const ex& expr);

//------------------------------ ANM_solutions class----------------------------------------------------
void ANM_solutions::set_bMinus(bool aBMinus)
{
    bMinus = aBMinus;
}

bool ANM_solutions::is_a_ANM_Solution(const myprocedure &proc)
{
	for(size_t i = 0; i < size(); i++)
		if((*this)[i].name.is_equal(proc))
			return true;
	return false;
}

bool ANM_solutions::is_a_ANM_IC(const myprocedure &proc)
{
	//DEBUG_OUT("proc = " << proc)
	//DEBUG_OUT("proc.op(1).op(0) = " << proc.op(1).op(0))
	if(proc.op(1).op(0).is_equal(0))
		return is_a_ANM_Solution(myprocedure(proc.op(0), lst(s_t)));
	if(proc.op(0).is_equal(GetSymbol("IC")) && is_a<numeric>(proc.op(1).op(1)))
		return is_a_ANM_Solution(ex_to<myprocedure>(proc.op(1).op(0)));
	return false;
}

vector<result> ANM_solutions::get_current_result(bool EvulateGlobal)
{
	vector<result> ret;
	for(int i = 0; i < size(); i++)
		ret.push_back((*this)[i].get_current_result(EvulateGlobal));
	return ret;
}

void ANM_solutions::null()
{
    //DEBUG_OUT("******************* null ***************************")
    ANM_solutions::iterator solIter;
	for(solIter = begin(); solIter != end(); solIter++)
    {
		solIter->CurSymR = -1;
		for(int k=0; k<3;k++)
			solIter->numRk[k].clear();
		if(solIter->newVar)
		{
			delete solIter->newVar;
			solIter->newVar = NULL;
		}
        vector<diff_sol>::iterator icIter;
        for(icIter = solIter->ICk.begin(); icIter != solIter->ICk.end(); icIter++)
            if(icIter->newVar)
            {
                delete icIter->newVar;
                icIter->newVar = NULL;
            }
    }
}

numeric ANM_solutions::get_time_step(const numeric &h)
{
    if((*this)[0].dwMode & ANMMODE_NEW_VAR)
    {
        numeric step = (*this)[0].newVar->evalf() - (*this)[0].newVar->numR(0);// - (*this)[0].newVar->GlobalD;
        if(step < 0)
        {
            *((*this)[0].step) *= numeric(-1);
            step = (*this)[0].newVar->evalf() - (*this)[0].newVar->numR(0);// - (*this)[0].newVar->GlobalD;
            *((*this)[0].step) *= numeric(-1);
            //DEBUG_OUT("sign(h(t)) = " << step)
            bMinus = true;
        }
        else
            bMinus = false;
        return abs(step);
    }
    //DEBUG_OUT("ANM_solutions::get_time_step - return h")
    if(bMinus)
        return -h;
    else
        return h;
}

numeric ANM_solutions::get_newVar(const numeric &tN)
{
    if((*this)[0].dwMode & ANMMODE_NEW_VAR)
	    return (*this)[0].numIC(0);
    return tN;
}

numeric ANM_solutions::get_step(int Forh, bool EvulateGlobal, const numeric &min_step)
{
    //DEBUG_OUT("ANM_solutions::get_step" << "\x0D\x0A";
	numeric step = 0;
    if((*this)[0].dwMode & ANMMODE_NEW_VAR)
    {
        // 0-е решение - x[0](t) -> t(x[0])
        (*this)[0].newVar = new phase(&((*this)[0]), NULL);
        // i-е решение - x[i](t) -> x[i](x[0])
		for(int i=1; i<size();i++)
	        (*this)[i].newVar = new phase(&((*this)[i]), &((*this)[0]));
    }
	for(int i=0; i<size();i++)
	{
        //DEBUG_OUT((*this)[i].name << ".ICk.size() = " << (*this)[i].ICk.size())
        numeric new_step;
        if((*this)[0].dwMode & ANMMODE_NEW_VAR)
            new_step = (*this)[i].newVar->get_step(Forh, EvulateGlobal, min_step);
        else
            new_step = (*this)[i].get_step(Forh, EvulateGlobal, min_step);
		if(step == 0)
			step = new_step;
		else
			step = min(step, new_step);
	}
    //DEBUG_OUT("end - ANM_solutions::get_step" << "\x0D\x0A";
	return step;
}

solution* ANM_solutions::get_sptr(const myprocedure &proc)
{
//	if(proc.op(0).is_equal(s_diff))
//		return get_sptr(ex_to<myprocedure>(proc.op(1).op(0)));
// Если это x(0)
	if(proc.op(1).op(0).is_equal(0))
		return get_sptr(myprocedure(proc.op(0), lst(s_t)));
// Если это IC(x(t), n)
	if(proc.op(0).is_equal(GetSymbol("IC")))
		return get_sptr(ex_to<myprocedure>(proc.op(1).op(0)));
	for(size_t i = 0; i < size(); i++)
	{
		if((*this)[i].name.is_equal(proc))
			return &((*this)[i]);
	}
	return 0;
}

void ANM_solutions::set_XNames(ANM_system& owner)
{
    //DEBUG_OUT("set_XNames" << "\x0D\x0A";
	clear();
	for(unsigned int i = 0; i < owner.X.rows(); i++)
	{
		solution tmpSol(0, 0, owner.X(i,0));
    //DEBUG_OUT("set_XNames - i = " << i << "\x0D\x0A";
		push_back(tmpSol);
	}
    //DEBUG_OUT("end - set_XNames" << "\x0D\x0A";
}

void ANM_solutions::setICs(const numeric& h)
{
    if((*this)[0].dwMode & ANMMODE_NEW_VAR)
        (*this)[0].ICk[0].nIC = (*this)[0].numIC(0) + h;
    else
		(*this)[0].set_ic(h);
	for(int i=1; i<size();i++)
		(*this)[i].set_ic(h);
}

void ANM_solutions::set(const matrix& Sols, ANM_system& owner)
{
	DWORD mode = 0;
	if(owner.InputData.nops() > 3)
	{
		if(owner.InputData.op(3).has(GetSymbol("anm_evulate_square")))
			mode |= ANMMODE_EVULATE_SQUARE;
		if(owner.InputData.op(3).has(GetSymbol("anm_new_var")))
			mode |= ANMMODE_NEW_VAR;
		if(owner.InputData.op(3).has(GetSymbol("rk_eval")))
			mode |= RKMODE_EVAL;
		if(owner.InputData.op(3).has(GetSymbol("rk_evalf")))
			mode |= RKMODE_EVALf;
		if(owner.InputData.op(3).has(GetSymbol("rk_normal")))
			mode |= RKMODE_NORMAL;
	}
    //DEBUG_OUT("******************* set ***************************" << "\x0D\x0A";
	for(int i = 0; i < Sols.rows(); i++)
	{
        ex tmpEx, exB;//, SolsI = Sols(i,0);
		//DEBUG_OUT(SolsI);
		(*this)[i].GlobalD = owner.StartError;
		(*this)[i].newVar = NULL;
		(*this)[i].dwMode = mode;
		(*this)[i].NE = &(owner.NE);
//        (*this)[i].bMinus = &bMinus;
		//DEBUG_OUT((*this)[i].name << ".A = " << (*this)[i].A);
		//(*this)[i].J = exA.degree(s_p) - exB.degree(s_p) - 1;
		(*this)[i].J = 0;
		(*this)[i].curt = &(owner.tN);
		(*this)[i].step = &(owner.h);
		for(int k = 0; k < 3; k++)
			(*this)[i].phi[k].owner = &((*this)[i]);
		(*this)[i].phi[0].Sign = -1;
		(*this)[i].phi[0].name = ToString((*this)[i].name) + ".PhiMinus";
		(*this)[i].phi[1].Sign = 0;
		(*this)[i].phi[1].name = ToString((*this)[i].name) + ".Phi";
		(*this)[i].phi[2].Sign = 1;
		(*this)[i].phi[2].name = ToString((*this)[i].name) + ".PhiPlus";
		(*this)[i].others.clear();
		if(owner.InputData.nops() > 3)
			if((owner.InputData.op(3).has(GetSymbol("roots_chart"))))
			{
				// для построения корневых годографов нужно добавить в others реальные и мнимые части этих корней
				lst roots(ex_to<lst>(root_of((*this)[i].A, s_p)));
				exmap RtoX;
				for(int l = 0; l < owner.L; l++)
					RtoX.insert(std::make_pair(owner.R(l,0),owner.getX(l)));
				//DEBUG_OUT("re(roots.op(i)) = " << re(roots.op(i)))
				//DEBUG_OUT("tmpEx1 = " << tmpEx1)
				//DEBUG_OUT("ex_to_taylor_row 1 = " << ex_to_taylor_row(tmpEx1, this))
				//DEBUG_OUT("row_res 1 = " << row_res(ex_to_taylor_row(tmpEx1, this)))
				for(int k = 0; k < roots.nops(); k++)
				{
					if(i == 0)
					{
						DEBUG_OUT("roots.op("<< k << ") = " << roots.op(k))
						ex tmpEx1 = roots.op(k).subs(RtoX);
						lst tmpLst;
						for(int l = 0; l < owner.L; l++)
							tmpLst.append(owner.getX(l));
						(*this)[i].other_rows.push_back(row_res(re(tmpEx1), tmpLst, 2, GetSymbol("Re(lambda[" + ToString(k+1) + "])")));
						(*this)[i].other_rows.push_back(row_res(im(tmpEx1), tmpLst, 2, GetSymbol("Im(lambda[" + ToString(k+1) + "])")));
						//(*this)[i].others.push_back(sub_val(unknown_sol(&((*this)[i])), re(roots.op(k)),GetSymbol("Re(lambda[" + ToString(k+1) + "])")));
						//(*this)[i].others.push_back(sub_val(unknown_sol(&((*this)[i])), im(roots.op(k)),GetSymbol("Im(lambda[" + ToString(k+1) + "])")));
					}
					if(!(owner.InputData.op(3).has(GetSymbol("no_rk"))))
					{
						try{
							//DEBUG_OUT((*this)[i].name << ".k = " << (k+1))
							(*this)[i].others.push_back(sub_val(unknown_sol(&((*this)[i])), re((*this)[i].Rn(0,roots.op(k))),GetSymbol("Re(" + ToString((*this)[i].name) + "->R[" + ToString(k+1) + "](0))")));
							(*this)[i].others.push_back(sub_val(unknown_sol(&((*this)[i])), im((*this)[i].Rn(0,roots.op(k))),GetSymbol("Im(" + ToString((*this)[i].name) + "->R[" + ToString(k+1) + "](0))")));
							(*this)[i].Rn(1,roots.op(k));
							(*this)[i].Rn(2,roots.op(k));
							(*this)[i].Rn(3,roots.op(k));
						} catch (const exception &e) {
				            cout << "no_rk. Error: " << e.what() << "\x0D\x0A" << flush;
						}
					}
				}
			}

		for(int k = 0; k < owner.SubResults.nops(); k++)
		{
			if(is_a<sub_val>(owner.SubResults[k]))
				if(owner.SubResults[k].op(0) == (*this)[i].name)
				{
					if(is_a<row_res>(owner.SubResults[k].op(1)))
					{
						(*this)[i].other_rows.push_back(ex_to<row_res>(owner.SubResults[k].op(1)));
						continue;
					}
					// если имеем дело со структурой типа x(t).y(0,50) или x(t).R(0,50), то преобразуем это дело в 50 игреков или R-в
					if(is_a<myprocedure>(owner.SubResults[k].op(1)) &&
					(owner.SubResults[k].op(1).op(0) == GetSymbol("R") || owner.SubResults[k].op(1).op(0) == GetSymbol("y"))&&
					owner.SubResults[k].op(1).op(1).nops() == 2)
					{
						// Определяем нижний и верхний индексы
						ex begInd, endInd;
						begInd = owner.SubResults[k].op(1).op(1).op(0);
						endInd = owner.SubResults[k].op(1).op(1).op(1);
						//DEBUG_OUT("begInd = " << begInd << "; endInd = " << endInd);
						// если оба индекса - числа, то можно продолжать
						if(is_a<numeric>(begInd) && is_a<numeric>(endInd))
							for(int ii = (ex_to<numeric>(begInd)).to_int(); ii <= (ex_to<numeric>(endInd)).to_int(); ii++)
							{
								if(owner.SubResults[k].op(1).op(0) == GetSymbol("R"))
								{
									sub_val tmpSub((*this)[i].name, myprocedure(owner.SubResults[k].op(1).op(0), lst(ii)));
									// добавляем в список под-результатов R-ы
									(*this)[i].others.push_back(ex_to<sub_val>(ex_to_taylor_row(tmpSub, this)));
								}
								else
								{
//									ex tmpEx = -myprocedure(s_diff, lst(unknown_sol(&((*this)[i])),s_t,ii+2))/myprocedure(s_diff, lst(unknown_sol(&((*this)[i])),s_t,ii));
//									(*this)[i].other_rows.push_back(row_res(ex_to_taylor_row(tmpEx, this)));
									ex tmpEx1 = -myprocedure(s_diff, lst(unknown_sol(&((*this)[i])),s_t,ii+2));
									ex tmpEx2 = myprocedure(s_diff, lst(unknown_sol(&((*this)[i])),s_t,ii));
									(*this)[i].other_rows.push_back(row_res(ex_to_taylor_row(tmpEx1, this), ex_to_taylor_row(tmpEx2, this)));
								}
							}
					}
					else
						(*this)[i].others.push_back(ex_to<sub_val>(ex_to_taylor_row(owner.SubResults[k], this)));
				}
				else if(ToString(owner.SubResults[k].op(0)) == ToString((*this)[i].name))
					(*this)[i].others.push_back(ex_to<sub_val>(owner.SubResults[k]));
		}

	}
}

//------------------------------ Plot class----------------------------------------------------
GINAC_IMPLEMENT_REGISTERED_CLASS(Plot, basic)

Plot::Plot()
{
}

Plot::Plot(const ex& aInputEx) : InputData(aInputEx)
{
}

Plot::Plot(const lst& aInputEx) : InputData(aInputEx)
{
}

void Plot::archive(archive_node &n) const
{
	inherited::archive(n);
}

Plot::Plot(const archive_node &n, lst &sym_lst) : inherited(n, sym_lst)
{
}

ex Plot::unarchive(const archive_node &n, lst &sym_lst)
{
	return (new Plot(n, sym_lst))->setflag(status_flags::dynallocated);
}

int Plot::compare_same_type(const basic &other) const
{
	const Plot &o = static_cast<const Plot &>(other);
	if (InputData.is_equal(o.InputData))
		return 0;
	if(InputData.compare(o.InputData) < 0)
		return -1;
	return 1;
}

void Plot::print(const print_context &c, unsigned level) const
{
	c.s << InputData << "\x0D\x0A";
}

void Plot::Check()
{
}

void Plot::Generate()
{
    ex old_input_data = InputData;
	int i;
//	InputData[0] - <set> №1 - {f1(x), f2(x)} или f1(x);
	Funcs.remove_all();
	if(is_a<lst>(InputData.op(0)))
		for(i = 0; i < InputData.op(0).nops(); i++)
			Funcs.append(InputData.op(0).op(i));
	else
		Funcs.append(InputData.op(0));
//	InputData[1] - <set> №2 - список параметров {x, у} или параметр x;
	if(InputData.nops() > 1)
	{
		if(InputData.op(1).nops() == 0)
			x = InputData.op(1);
		else
			x = InputData.op(1).op(0);
		y = 0;
		Is3D = false;
		if (InputData.op(1).nops() > 1)
		{
			y = InputData.op(1).op(1);
			Is3D = true;
		}
	}
	Xmin = -10;
	Xmax = 10;
	Ymin = -10;
	Ymax = 10;
	Zmin = -10;
	Zmax = 10;
//	InputData[2] - <set> №3 - диапазон значений параметров {{-10, 10},{-10, 10},{-10, 10}};
	try{
		if(InputData.nops() > 2 && !is_zero(InputData.op(2).op(0)))
		{
			if(is_a<lst>(InputData.op(2)))
			{
				if(is_a<lst>(InputData.op(2).op(0)))
				{
					if(is_a<numeric>(InputData.op(2).op(0).op(0).evalf()))
						Xmin = ex_to<numeric>(InputData.op(2).op(0).op(0).evalf());
					if(is_a<numeric>(InputData.op(2).op(0).op(1).evalf()))
						Xmax = ex_to<numeric>(InputData.op(2).op(0).op(1).evalf());
				}
				if(is_a<lst>(InputData.op(2).op(1)))
				{
					if(is_a<numeric>(InputData.op(2).op(1).op(0).evalf()))
						Zmin = ex_to<numeric>(InputData.op(2).op(1).op(0).evalf());
					if(is_a<numeric>(InputData.op(2).op(1).op(1).evalf()))
						Zmax = ex_to<numeric>(InputData.op(2).op(1).op(1).evalf());
				}
				if(is_a<lst>(InputData.op(2).op(2)))
				{
					if(is_a<numeric>(InputData.op(2).op(2).op(0).evalf()))
						Ymin = ex_to<numeric>(InputData.op(2).op(2).op(0).evalf());
					if(is_a<numeric>(InputData.op(2).op(2).op(1).evalf()))
						Ymax = ex_to<numeric>(InputData.op(2).op(2).op(1).evalf());
				}
			}
		}
	} catch (const exception &e) {
	}
//	InputData[3] - <set> №4 - дополнительные параметры;
	FontSize = 16;
	if(sym_tab.is_name("FontSize") + 1)
		FontSize = sym_tab.Evalf("FontSize").to_int();
	nWeight = FW_NORMAL;
	if(sym_tab.is_name("Bold") + 1)
		if(sym_tab.Evalf("Bold")>0)
			nWeight = FW_BOLD;
	LineWidth = 1;
	if(sym_tab.is_name("LineWidth") + 1)
		LineWidth = sym_tab.Evalf("LineWidth").to_double();
	InputData = old_input_data;
}

//------------------------------ ANM_system class----------------------------------------------------
GINAC_IMPLEMENT_REGISTERED_CLASS(ANM_system, basic)

ANM_system::ANM_system()
{
	bfInitialaized = false;
	Result = NULL;
}

ANM_system::ANM_system(const ex& aInputEx) :  InputData(aInputEx)
{
	bfInitialaized = false;
//	InputData = aInputEx;//GetInputData(aInputEx);
	Result = NULL;
}

ANM_system::ANM_system(const lst& aInputEx) : InputData(aInputEx)
{
	bfInitialaized = false;
//	InputData = aInputEx;//GetInputData(aInputEx);
	Result = NULL;
}

/*void ANM_system::copy(const ANM_system &other)
{
	inherited::copy(other);
	bfInitialaized = false;
	InputData = other.InputData;
	Result = NULL;
}*/

void ANM_system::archive(archive_node &n) const
{
	inherited::archive(n);
}

ANM_system::ANM_system(const archive_node &n, lst &sym_lst) : inherited(n, sym_lst)
{
}

ex ANM_system::unarchive(const archive_node &n, lst &sym_lst)
{
	return (new ANM_system(n, sym_lst))->setflag(status_flags::dynallocated);
}

int ANM_system::compare_same_type(const basic &other) const
{
	const ANM_system &o = static_cast<const ANM_system &>(other);
	if (InputData.is_equal(o.InputData))
		return 0;
	if(InputData.compare(o.InputData) < 0)
		return -1;
	return 1;
}

void ANM_system::print(const print_context &c, unsigned level) const
{
	// print_context::s is a reference to an ostream
	if(InputData.nops() > 3)
		if(InputData.op(3).has(GetSymbol("debug_print")))
		{
			c.s << "bfInitialaized = " << bfInitialaized << "\x0D\x0A";
			if(!bfInitialaized)
			{
				c.s << "InputData: " << InputData << "\x0D\x0A";
				return;
			}
			string OutString;
			OutString += "\n ODEs = " + ToString(ODEs) + "\n";
			OutString += " ICs = " + ToString(ICs) + "\n";
			OutString += " A = " + ToString(A) + "\n";
			OutString += " X = " + ToString(X) + "\n";
			if(Lf)
			{
				OutString += " G = " + ToString(G) + "\n";
				OutString += " F = " + ToString(F) + "\n";
			}
			OutString += " H = " + ToString(H) + "\n";
			OutString += " Q = " + ToString(Q) + "\n";
			OutString += " C = " + ToString(C) + "\n";
//			OutString += " R[0] = " + ToString(const_cast<ANM_system*>(this)->Solutions[0].R(0)) + "\n";
			if(AdditionalFuncs.nops())
				OutString += "Functions = " + ToString(AdditionalFuncs) + "\n";
			c.s << "ANM{" << OutString << "}" << "\x0D\x0A";
		}

	c.s << InputData << "\x0D\x0A";
}

bool ANM_system::CanEvulate()
{
	if(L <= 0)
		return false;
	if(Solutions.bMinus)
		return tN > SupT && h > Minh;
	else
		return tN < SupT && h > Minh;
}

void ANM_system::Evulate()
{
// Обнулим решения - как-будто мы ничего не искали и у нас нет никаких данных кроме предначальных условий и полной погрешности расчета
    //DEBUG_OUT("Solutions.null")
	Solutions.null();
// выбор шага расчета
    //DEBUG_OUT("Solutions.get_step")
	h = Solutions.get_step(Forh, Speciality, Minh);
    //DEBUG_OUT("Maxh = " << Maxh)
    //DEBUG_OUT("step = h = " << h)
/*  debug version
	if(h == 10)
		h = SupT - tN;
	h = min(SupT - tN, h);
*/
    //DEBUG_OUT("if(h > Maxh) Maxh = " << ex(Maxh))
	if(h > Maxh)
		h = Maxh;
	//DEBUG_OUT("if(sym_tab.is_name(StopTimes)")
	if(sym_tab.is_name("StopTimes") + 1)
	{
		//DEBUG_OUT("GetSymbol(StopTimes).subs(sym_tab.GetMap()) = ")
		//DEBUG_OUT("GetSymbol(StopTimes).subs(sym_tab.GetMap()) = " << GetSymbol("StopTimes").subs(sym_tab.GetMap()))
		ex tmpEx = GetSymbol("StopTimes").subs(sym_tab.GetMap());
		for(int k = 0; k < tmpEx.nops(); k++)
			if(is_a<numeric>(tmpEx.op(k).evalf()))
				if(tmpEx.op(k).evalf() > tN && tmpEx.op(k).evalf() < tN + h ||
					tmpEx.op(k).evalf() - (tN+h) > 0 && tmpEx.op(k).evalf() - (tN+h) < Minh)
				{
					//DEBUG_OUT("tmpEx.op(k).evalf() = " << tmpEx.op(k).evalf())
					h = ex_to<numeric>(tmpEx.op(k).evalf()) - tN;
					break;
				}
	}

// поиск полной погрешности расчета
    //DEBUG_OUT("Solutions.set_global_delta")
	Solutions.set_global_delta(h, SupLocError, Speciality);

// если расчет ведется не по t а по x(t), то в функции get_time_step устанавливается флаг bMinus
// также если установлен флаг Solutions.bMinus то шаг возвражается со знаком минус
    cout << "h = " << Solutions.get_time_step(h) << "\x0D\x0A" << flush;
    //DEBUG_OUT("h = " << Solutions.get_time_step(h));
// добавляем шаг и найденные значения в таблицу найденных значений. Также можем добавить все что хотим
	Result->AddResult(Solutions.get_current_result(Speciality), Solutions.get_newVar(tN), (1-2*Solutions.bMinus)*h);
// передвигаем ось ординат на шаг вправо
	tN += Solutions.get_time_step(h);
    cout << "t[N] = " << tN << "\x0D\x0A" << flush;
//	tN += Solutions.get_time_step(h);
// переопределяем начальные условия: x(0+) = x(0-);
	Solutions.setICs((1-2*Solutions.bMinus)*h);
    //DEBUG_OUT("after Solutions.setICs")
}

ANM_charts ANM_system::GetEmptyResult()
{
    //DEBUG_OUT("ANM_system::GetEmptyResult")
	ANM_charts ret;
	for(int i = 0; i < Solutions.size(); i++)
    {
		ret.results.push_back(Solutions[i].get_empty_result());
    }
    //DEBUG_OUT("end - ANM_system::GetEmptyResult")
	return ret;
}

void ANM_system::Analitic(const ex& aFlags)
{
    //DEBUG_OUT("ANM_system::Analitic")
    ex old_input_data = InputData;
    for(int k = 0; k < InputData.op(1).nops(); k++)
        if(is_a<symbol>(InputData.op(1).op(k)))
        {
            myprocedure tmpProc = myprocedure(InputData.op(1).op(k), lst(s_t));
            InputData.let_op(1).let_op(k) = tmpProc;
    	}
    //ExpandWithSymTab(InputData);
    //DEBUG_OUT("InputData = " << InputData)
	//DEBUG_OUT("InputData.subs = " << InputData.subs(lst(myprocedure(GetSymbol("i"), lst(s_t))), lst(s_t)))
    //DEBUG_OUT("InputData after subs = " << InputData)
	int i;
	L = Lf = 0;
//	InputData[0] - <set> №1 - {ODE's, IC's};
	i = 0;
	ICs.remove_all();
	ODEs.remove_all();
	while(i < InputData.op(0).nops())
		if(is_a<defining>(InputData.op(0).op(i++))) // если это x(0) = x0
			ICs.append(InputData.op(0).op(i-1));
		else
		{
            ex tmpRel; // tmp relational
// Если это просто символ, то нужно поискать в таблице символов его, и подставить значение вместо символа
            if(is_a<symbol>(InputData.op(0).op(i-1)))
			{
				int k = sym_tab.is_name(ToString(InputData.op(0).op(i-1)));
				if(k != -1)
                    tmpRel = InputData.op(0).op(i-1).subs(sym_tab.GetSymMap());
				else
					ERROR_10(InputData.op(0).op(i-1))
				//DEBUG_OUT("tmpRel = " << tmpRel)
			}
			else
                tmpRel = InputData.op(0).op(i-1);
// Если выяснилось, что tmpRel не уравнение вида <expr> == <expr>, то это ошибка
            if(is_a<relational>(tmpRel))
            {
                //DEBUG_OUT(tmpRel);
// если левая часть равенства вида x(0) или diff(x(t),t)(0), то добавляем эту фигню в массив ICs
                if(is_a<myprocedure>(tmpRel.op(0)) && tmpRel.op(0).op(1).op(0).evalf().is_zero())
                    ICs.append(tmpRel);
                else
                    ODEs.append(tmpRel);
            }
		}
	//DEBUG_OUT("ODEs = " << ODEs)
	//DEBUG_OUT("ODEs.subs = " << ODEs.subs(sym_tab.GetMap()))
//	InputData[1] - <set> №2 - список реакций {x1(t), x2(t)};
	L = InputData.op(1).nops();
//	InputData[2] - <set> №3 -  список воздействий {f1(t), f2(t)};
	if(InputData.nops() > 2 && !is_zero(InputData.op(2).op(0)))
		Lf = InputData.op(2).nops();
// а также проверим количество введенных строк системы уравнений
	if(L > ODEs.nops())
	{
        //DEBUG_OUT(ODEs);
        //DEBUG_OUT(ICs);
		ERROR_11
	}
	if(L < ODEs.nops())
	{
        //DEBUG_OUT(ODEs);
        //DEBUG_OUT(ICs);
		ERROR_12
	}

	NE = 1;
// Определение параметров системы
	// элементы интерфейса
	FontSize = 16;
	if(sym_tab.is_name("FontSize") + 1)
		FontSize = sym_tab.Evalf("FontSize").to_int();
	nWeight = FW_NORMAL;
	if(sym_tab.is_name("Bold") + 1)
		if(sym_tab.Evalf("Bold")>0)
			nWeight = FW_BOLD;
	LineWidth = 1;
	if(sym_tab.is_name("LineWidth") + 1)
		LineWidth = sym_tab.Evalf("LineWidth").to_double();

	StartError =  sym_tab.Evalf("StartError");
	SupLocError = sym_tab.Evalf("SupLocError");
	Forh = (sym_tab.Evalf("Forh")).to_int();
	Speciality = (sym_tab.Evalf("Complicated") != 0);
	InfT = sym_tab.Evalf("InfT");;
	SupT = sym_tab.Evalf("SupT");
	Minh = sym_tab.Evalf("Minh");
	Maxh = 0;
	if(sym_tab.is_name("Maxh") + 1)
		Maxh = sym_tab.Evalf("Maxh");
	if(Maxh == 0)
		Maxh = abs(SupT - InfT);
    // Проверка правильности заданных данных
	if(StartError < 0)
		StartError = 0;
	if(SupLocError <= 0)
		SupLocError = 1e-5;
	if(Forh < 10)
		Forh = 10;
//	if(Maxh <= 1e-6)
//		Maxh = 1e-5;
    Solutions.bMinus = (InfT > SupT);
    tN = InfT;
	if(L > 0)
	{
		// инициализация матриц для выполнения аналитической части метода
		A = matrix(L,L);
		X = matrix(L,1);
		SymbX = matrix(L,1);
		if(Lf)
		{
			G = matrix(L,Lf);
			F = matrix(Lf,1);
		}
		H = matrix(L,1);
		Q = matrix(L,1);
		C = matrix(L,1);
	}
	for(int k = 0; k < L; k++)
	{
   		X.set(k, 0, InputData.op(1).op(k));
		SymbX.set(k, 0, symbol());
	}
// Определим имена воздействий и функции воздействий
//	exmap m_func_to_symb, m_symb_to_func;
	if(Lf)
		for(int k = 0; k < Lf; k++)
        {
			F.set(k, 0, InputData.op(2).op(k));
        }
	if(InputData.nops() > 3)
		SubResults = ex_to<lst>(InputData.op(3));
// определение данных для дополнительного вывода (функции от t)
	if(InputData.nops() > 4)
		AdditionalFuncs = ex_to<lst>(InputData.op(4));
// Формируем массив неизвестных решений уравнения (просто формируем, без полной инициализации)
	if(L > 0)
		Solutions.set_XNames(*this);
// После формирования массива неизвестных решений можно заменить в уравнениях sub_val на sym_rk.
	//DEBUG_OUT("ODEs = " << ODEs)
	for(int k = 0; k < L; k++)
		ODEs[k] = SubVal2Rk(ODEs[k]);
	//DEBUG_OUT("ODEs = " << ODEs)
	if(L > 0)
	{
	// инициализация матрицы A
	// перед инициализацией некоторых матриц нужно заменить имена воздействий на временные символы (чтобы они не раскрывались)
		cout << "matrix A(D) initialisation" << "\x0D\x0A" << flush;
		Init_A();
		//DEBUG_OUT("2.ODEs = " << ODEs)
	//DEBUG_OUT("A = " << A)
	// инициализация матрицы G
		cout << "matrix G(D) initialisation" << "\x0D\x0A" << flush;
		Init_G();
		//DEBUG_OUT("3.ODEs = " << ODEs)
		//DEBUG_OUT("G = " << G)
	// инициализация матрицы H
	// сразу при инициализации она трансформируется в матрицу T. Это нужно будет менять при реализации довыделения...
		cout << "matrix H(x(t),f(t),t) initialisation" << "\x0D\x0A" << flush;
		Init_H();
		//DEBUG_OUT("5.ODEs = " << ODEs)
		// если установлен флаг довыделения, то делаем его
		if(InputData.nops() > 3)
			if(InputData.op(3).has(GetSymbol("sup_allocate")))
				SupSelecting();
		// если обнаруживается, что система с невыделенной линейной частью, довыделяем ее
		SupSelectLinearPart();
		//Solution
		//DEBUG_OUT("H = " << H)

	//---------------------- Аналитическая часть метода ---------
	// преобразование полученной системы по Лапласу происходит при инициализации матрицы H
	// инициализация матрицы Q
		cout << "matrix Q(p) initialisation" << "\x0D\x0A" << flush;
		Init_Q();
		//DEBUG_OUT("6.ODEs = " << ODEs)
		//DEBUG_OUT("Q = " << Q)
	// Теперь самое главное - решение сформированной системы алгебраических уравнений
	// в результате которого мы получим решения x[l](t) = sum(R[l][i]*t^i/i!, i = minp..?)
		if(Lf)
			C = G.mul(F).add(Q).add(H);
		else
			C = Q.add(H);
		//DEBUG_OUT("after Init C")
		///*debug version
		if(aFlags.is_equal(numeric(-1)))
		{
    		Solutions.setICs(ICs);
			InputData = old_input_data;
			//DEBUG_OUT("end0 - Analitic")
			return;
		}
		cout << "Initialisation completed!" << "\x0D\x0A" << flush;
		if(!is_a<symbol>(aFlags)) // aFlags является символом, если функция вызывается из ANM_sensitivity или ANM_sens_system
		{
			bfInitialaized = true;
			if(InputData.nops() > 3)
				if(InputData.op(3).has(GetSymbol("debug_print")))
				{
					string OutString;
					//OutString += "\x0D\x0A Expressions = " + ToString(expr_tab) + "\x0D\x0A";
					OutString += "\x0D\x0A ODEs = " + ToString(ODEs) + "\x0D\x0A";
					OutString += " ICs = " + ToString(ICs) + "\x0D\x0A";
					OutString += " A = " + ToString(A) + "\x0D\x0A";
					OutString += " X = " + ToString(X) + "\x0D\x0A";
					if(Lf)
					{
						OutString += " G = " + ToString(G) + "\x0D\x0A";
						OutString += " F = " + ToString(F) + "\x0D\x0A";
					}
					OutString += " H = " + ToString(H) + "\x0D\x0A";
					OutString += " Q = " + ToString(Q) + "\x0D\x0A";
					OutString += " C = " + ToString(C) + "\x0D\x0A";
					if(AdditionalFuncs.nops())
						OutString += "Functions = " + ToString(AdditionalFuncs) + "\x0D\x0A";
					DEBUG_OUT("ANM{" << OutString << "}");
				}
		}
		cout << "Solving algebraic equations..." << "\x0D\x0A" << flush;
		ex exA;
		//if(L > 1)
			exA = A.determinant();
		//else
		//	exA = A(0,0);
		//DEBUG_OUT("A = " << exA)
		exA = (exA.normal());
		N = exA.degree(s_p);
		for(int k = 0; k < L; k++)
		{
			matrix tmpA = A;
			for(int j = 0; j < L; j++)
				tmpA.set(j, k, C(j, 0));
			Solutions[k].A = ex_to_taylor_row(ex_to_ppolinom(exA.subs(sym_tab.GetMap())), &Solutions);
			//DEBUG_OUT("(tmpA.determinant()) = " << (tmpA.determinant()))
			//tmpA.determinant().print(print_tree(std::cout));
			//DEBUG_OUT("(tmpA.determinant()).expand() = " << (tmpA.determinant()).expand())
			//DEBUG_OUT("(tmpA.determinant()).expand().eval() = " << (tmpA.determinant()).expand().eval())
			//DEBUG_OUT("pow(s_p, numeric(-1))*s_p = " << (pow(s_p, numeric(-1))*s_p))
			ex tmpEx = ex_to_ppolinom((tmpA.determinant()).expand());
			//DEBUG_OUT("tmpEx = " << tmpEx);
			ex exB = tmpEx;//.subs(sym_tab.GetMap());
			Solutions[k].B = ex_to_taylor_row(exB, &Solutions);
			if(InputData.nops() > 3)
				if(InputData.op(3).has(GetSymbol("debug_print")))
				{
					DEBUG_OUT(Solutions[k].name << ".B = " << Solutions[k].B);
					DEBUG_OUT(Solutions[k].name << ".A = " << Solutions[k].A);
				}
			//tmpSolution.set(k, 0, (exB/exA).normal());
			cout << "line " << (k+1) << " completed" << "\x0D\x0A" << flush;
		}
		cout << "Algebraic equations solved!" << "\x0D\x0A" << flush;
		cout << "Data generation for solutions..." << "\x0D\x0A" << flush;
		if(aFlags.nops() > 1)
			for(int k = 0; k < aFlags.nops(); k++)
				SubResults.append(aFlags.op(k));
		matrix tmpSolution = matrix(L, 1);
		Solutions.set(tmpSolution, *this);
		cout << "Data generation completed!" << "\x0D\x0A" << flush;
		cout << "Setting initial conditions..." << "\x0D\x0A" << flush;
		Solutions.setICs(ICs);
		cout << "...completed!" << "\x0D\x0A" << flush;
		h = abs(SupT - InfT);
	} // if(L > 0)
	InputData = old_input_data;
    //DEBUG_OUT("end - Analitic")
}

void ANM_system::SupSelectLinearPart()
{
	int m = 0, m_H = 0, m_H_i;
	// проверяем для каждой координаты, довыделена ли для нее линейная часть
	for(int l = 0; l < L; l++)
	{
		// Находим старшую производную по l-й координате в матрице A
		int m = 0;
		for(int i = 0; i < L; i++)
		{
			int m1 = (A(i,l)).degree(s_p);
			m = max(m,m1);
		}
		// Находим старшую производную по l-й координате в матрице H
		// Сравниваем старшую производную в матрице A со старшей производной по l-й координате в матрице H
		for(int i = 0; i < L; i++)
		{
			int m_H1 = GetMaxDiffLevel(H(i,0), X(l,0));
			if(m_H < m_H1)
			{
				m_H = m_H1;
				m_H_i = i;
			}
		}
		// если в m_H_i-й строке матрицы H есть производная, старше, чем в матрице A, то производим довыделение.
		if(m_H > m)
			for(int i = 0; i < L; i++)
			{
				if(m_H == GetMaxDiffLevel(H(i,0), X(l,0)))
					SupSelectLinearPart(i, l, m_H);
			}
	}
}

void ANM_system::SupSelectLinearPart(int i, int l, int m_H)
{
	ex exAddon = GetMaxDiffCoef(H(i,0), X(l,0), m_H);
	exmap XtoRm, XtoSymb, SymbToX;
	DEBUG_OUT("A before sup select linear part = " << A)
	DEBUG_OUT("H before = " << H)
	XtoRm.clear();
	for(int k = 0; k < L; k++)
		XtoRm.insert(std::make_pair(X(k,0), R(k,0)));
	XtoSymb.clear();
	for(int k = 0; k < L; k++)
		XtoSymb.insert(std::make_pair(X(k,0), SymbX(k,0)));
	/*debug version пробуем правильно довыделять при наличии функций вида f(t)=const {в Якоби, например, c(t)=-0.51}*/
	XtoSymb.insert(std::make_pair(s_t, 0));
	SymbToX.clear();
	for(int k = 0; k < L; k++)
		SymbToX.insert(std::make_pair(SymbX(k,0), X(k,0)));
	exAddon = exAddon.subs(XtoSymb).evalf();
	exAddon = exAddon.subs(SymbToX);
	exAddon = exAddon.subs(XtoRm);
	A.set(i,l,A(i,l) - pow(s_p, numeric(m_H))*exAddon);
	exAddon *= X(l,0).diff(s_t, m_H);
	H.set(i,0, H(i,0) - Laplace_trans(exAddon));

	DEBUG_OUT("A after sup select linear part = " << A)
	DEBUG_OUT("H after = " << H)
}

int ANM_system::GetMaxDiffLevel(const ex& Expl, const ex& IdName)
{
	if(!Expl.has(IdName))
		return 0;
	if(is_a<myprocedure>(Expl))
		if(Expl.op(0).is_equal(s_diff))
			return (1 + GetMaxDiffLevel(Expl.op(1).op(0), IdName));
	if(is_a<add>(Expl) || is_a<mul>(Expl))
	{
		int m1, m = 0;
		for(int i = 0; i < Expl.nops(); i++)
		{
			m1 = GetMaxDiffLevel(Expl.op(i), IdName);
			m = max(m,m1);
		}
		return m;
	}
	return 0;
}

ex ANM_system::GetMaxDiffCoef(const ex& Expl, const ex& IdName, int m_H)
{
	if(!Expl.has(IdName))
		return 0;
	ex res;
	if(is_a<add>(Expl))
	{
		for(int i = 0; i < Expl.nops(); i++)
			if(m_H == GetMaxDiffLevel(Expl.op(i), IdName))
			{
				res = Expl.op(i);
				break;
			}
	}
	else
		res = Expl;
	return (res/IdName.diff(s_t, m_H));
}

ex ANM_system::GetLinearCoeff(const ex& Expl, const ex& IdName)
{
	if(!Expl.eval().is_equal(Expl))
		return GetLinearCoeff(Expl.eval(), IdName);
	if(Expl.is_equal(IdName))
		return 1;
	//DEBUG_OUT("Expl = " << Expl)
	//DEBUG_OUT("Expl.subs = " << Expl.subs(sym_tab.GetMap()))
    if(/*is_a<symbol>(Expl) && */!Expl.is_equal(Expl.subs(sym_tab.GetSymMap())))
        return GetLinearCoeff(Expl.subs(sym_tab.GetSymMap()), IdName);
//	Если в выражении нет IdName...
	if(!Expl.has(IdName))
		return 0;
	if(is_a<sub_val>(Expl))
		return 0;
	if(is_a<myprocedure>(Expl))
	{
		if(Expl.op(0).is_equal(s_diff))
			return s_p*GetLinearCoeff(Expl.op(1).op(0), IdName);
		if(Expl.op(0).is_equal(GetSymbol("int")))
			return pow(s_p, -Expl.op(1).op(1))*GetLinearCoeff(Expl.op(1).op(0), IdName);
	}
	if(is_a<add>(Expl))
	{
		ex res;
		for(int i = 0; i < Expl.nops(); i++)
			res += GetLinearCoeff(Expl.op(i), IdName);
		return res;
	}
	if(is_a<mul>(Expl))
	{
		ex res = 1;
		if(!Expl.has(IdName) || Is_XorF(Expl) != 1)
			return 0;
		//DEBUG_OUT("Expl = " << Expl)
		int idcounter = 0;
		for(int i = 0; i < Expl.nops(); i++)
			idcounter += Is_XorF(Expl.op(i));
		//DEBUG_OUT("idcounter = " << idcounter)
		if(idcounter != 1)
			return 0;
		for(int i = 0; i < Expl.nops(); i++)
		{
			ex tmpRes = GetLinearCoeff(Expl.op(i), IdName);
			//DEBUG_OUT("tmpRes = " << tmpRes)
			if(is_zero(tmpRes))
			{
				if(is_a<sub_val>(Expl.op(i)) && is_a<myprocedure>(Expl.op(i).op(0)))
				{
					myprocedure tmpProc = ex_to<myprocedure>(Expl.op(i).op(0));
					if(Solutions.is_a_ANM_Solution(tmpProc))
						if(is_a<myprocedure>(Expl.op(i).op(1)))
						{
							myprocedure tmpProc1 = ex_to<myprocedure>(Expl.op(i).op(1));
							if(tmpProc1.op(0).is_equal(GetSymbol("R")) && is_a<numeric>((tmpProc1.op(1).op(0))))
								res *= sym_rk(Solutions.get_sptr(tmpProc), (ex_to<numeric>(((tmpProc1.op(1).op(0))))).to_int());
						}
				}
				else
					res *= Expl.op(i);
			}
			else
				res *= tmpRes;
			//DEBUG_OUT("res = " << res)
		}
		return res;
	}

// Если выражение - отношение (например Exp1 == Exp2)
	if(is_a<relational>(Expl))
		return GetLinearCoeff(Expl.op(0), IdName) - GetLinearCoeff(Expl.op(1), IdName);
	return 0;
}

int ANM_system::Is_XorF(const ex& Expl)
{
	//DEBUG_OUT("Is_XorF.Expl = " << Expl)
	if(is_a<myprocedure>(Expl) && Expl.op(0).is_equal(GetSymbol("_H")))
		return 2;
	for(int l = 0; l < L; l++)
		if(Expl.has(X(l,0)))
		{
			//DEBUG_OUT("Expl 1 = " << Expl)
			if(is_a<power>(Expl))
				if(Is_XorF(Expl.op(0)) != 0)
					return 2;
				else
					return 0;
			if(is_a<myprocedure>(Expl) && Solutions.is_a_ANM_IC(ex_to<myprocedure>(Expl)))
				return 0;
			if(is_a<sub_val>(Expl))
				return 0;
			return 1;
		}
	for(int lf = 0; lf < Lf; lf++)
		if(Expl.has(F(lf,0)))
		{
			if(is_a<power>(Expl))
				return 2;
			return 1;
		}
	// Если это функция от t то возвращаем 2 - чтобы правильно распознавалась нелинейность в функции GetNotLinearPart
	if(/*is_a<function>(Expl) && */ Expl.has(s_t))
		return 2;
	return 0;
}

ex ANM_system::SubVal2Rk(const ex& Expl)
{
	if(!((Expl.eval()).is_equal(Expl)))
		return SubVal2Rk(Expl.eval());
    if(!Expl.is_equal(Expl.subs(sym_tab.GetSymMap())))
		return SubVal2Rk(Expl.subs(sym_tab.GetSymMap()));
    //DEBUG_OUT("Expl = " << Expl)
	//Если это функция _H(ex)
	if(is_a<myprocedure>(Expl) && Expl.op(0).is_equal(GetSymbol("_H")))
		return myprocedure(Expl.op(0),SubVal2Rk(Expl.op(1)));
	if(is_a<add>(Expl))
	{
		ex res;
		for(int i = 0; i < Expl.nops(); i++)
			res += SubVal2Rk(Expl.op(i));
		return res;
	}
	if(is_a<lst>(Expl))
	{
		lst tmpLst;
		for(int i = 0; i < Expl.nops(); i++)
			tmpLst.append(SubVal2Rk(Expl.op(i)));
		return tmpLst;
	}
	if(is_a<power>(Expl))
		return power(SubVal2Rk(Expl.op(0)), Expl.op(1));
	if(is_a<sub_val>(Expl) && is_a<myprocedure>(Expl.op(0)))
	{
		myprocedure tmpProc = ex_to<myprocedure>(Expl.op(0));
		if(Solutions.is_a_ANM_Solution(tmpProc))
			if(is_a<myprocedure>(Expl.op(1)))
			{
				myprocedure tmpProc1 = ex_to<myprocedure>(Expl.op(1));
				if(tmpProc1.op(0).is_equal(GetSymbol("R")) && is_a<numeric>((tmpProc1.op(1).op(0))))
					return sym_rk(Solutions.get_sptr(tmpProc), (ex_to<numeric>(((tmpProc1.op(1).op(0))))).to_int());
			}
	}
	if(is_a<mul>(Expl))
	{
		ex tmpRes = 1;
		for(int i = 0; i < Expl.nops(); i++)
			tmpRes *= SubVal2Rk(Expl.op(i));
		return tmpRes;
	}
// Если выражение - отношение (например Exp1 == Exp2)
	if(is_a<relational>(Expl))
		return relational(SubVal2Rk(Expl.op(0)),SubVal2Rk(Expl.op(1)));
	return Expl;
}

ex ANM_system::GetNotLinearPart(const ex& Expl)
{
	if(!((Expl.eval()).is_equal(Expl)))
		return GetNotLinearPart(Expl.eval());
    //DEBUG_OUT(Expl << " = " << Expl.subs(sym_tab.GetMap()))
	if(!Expl.is_equal(Expl.subs(sym_tab.GetSymMap())))
		return GetNotLinearPart(Expl.subs(sym_tab.GetSymMap()));
	//Если это функция _H(ex), то считается что ex - нелинейная часть и возвращается она
	if(is_a<myprocedure>(Expl) && Expl.op(0).is_equal(GetSymbol("_H")))
		return Expl.op(1).op(0);
	if(is_a<add>(Expl))
	{
		ex res;
		for(int i = 0; i < Expl.nops(); i++)
			res += GetNotLinearPart(Expl.op(i));
		return res;
	}
	if(is_a<mul>(Expl))
	{
		//DEBUG_OUT("Expl = " << Expl)
		int idcounter = 0;
		ex tmpRes = 1;
		for(int i = 0; i < Expl.nops(); i++)
		{
			if(is_a<myprocedure>(Expl) && Expl.op(0).is_equal(GetSymbol("_H")))
				tmpRes *= Expl.op(i).op(1).op(0);
			else
				tmpRes *= Expl.op(i);
			idcounter += Is_XorF(Expl.op(i));
		}
		//DEBUG_OUT("idcounter = " << idcounter)
		if(idcounter == 1)
			return 0;
		return tmpRes;
	}
// Если выражение - отношение (например Exp1 == Exp2)
	if(is_a<relational>(Expl))
		return GetNotLinearPart(Expl.op(0)) - GetNotLinearPart(Expl.op(1));
	if(Is_XorF(Expl) == 1)
		return 0;
	else
		return Expl;
}

void ANM_system::Init_A()
{
	unsigned int i, j;
	for(i = 0; i < L; i++)
		for(j = 0; j < L; j++)
			A.set(i,j,GetLinearCoeff(ODEs.op(i),X(j,0)));
}

void ANM_system::Init_G()
{
	int i,j;
	for(i = 0; i < L; i++)
		for(j = 0; j < Lf; j++)
			G.set(i,j,-GetLinearCoeff(ODEs.op(i),F(j,0)));
}

void ANM_system::Init_H()
{
	int i;
	for(i = 0; i < L; i++)
    {
        ex tmpEx = Laplace_trans(-GetNotLinearPart(ODEs.op(i)));
		//DEBUG_OUT("tmpEx = " << tmpEx)
	    H.set(i,0,tmpEx);//taylor_row(tmpEx, &Solutions));
    }
}

ex ANM_system::GetQElement(const ex& mxElem, const ex& IdName)
{
	ex res = 0;
	for(int curexp = -10; curexp < 10; curexp++)
	{
		ex curcoeff = mxElem.coeff(s_p, curexp);
		if(!is_zero(curcoeff))
		{
			if(curexp >= 0)
				for(int i = 0; i < curexp; i++)
					res += curcoeff*pow(s_p, numeric(curexp - 1 - i))*myprocedure(GetSymbol("IC"), lst(IdName, i));
			//if(curexp < 0)
			//	for(int i = 0; i < -curexp; i++)
			//		res += -curcoeff*pow(s_p, curexp + i)*ex_to_taylor_row(myprocedure(GetSymbol("IC"), lst(IdName, 0)), &Solutions);
		}
	}
	return res;
}

ex ANM_system::R(int l, int i)
{
	if(i < 0)
		return 0;
	return sym_rk(&(Solutions[l]), i);
}

ex ANM_system::getX(int l)
{
	return unknown_sol(&(Solutions[l]));
}

void ANM_system::SupSelecting()
{
	int i;
	DEBUG_OUT("A before = " << A)
	DEBUG_OUT("H before = " << H)
	// инициализация таблиц замены для subs
	exmap /*XtoRm,*/ XtoSymb, SymbToX;
	/*XtoRm.clear();
	for(int l = 0; l < L; l++)
		XtoRm.insert(std::make_pair(X(l,0), Solutions.get_sptr(ex_to<myprocedure>(X(l,0)))));
	*/
	XtoSymb.clear();
	for(int l = 0; l < L; l++)
		XtoSymb.insert(std::make_pair(X(l,0), SymbX(l,0)));
	/*debug version пробуем правильно довыделять при наличии функций вида f(t)=const {в Якоби, например, c(t)=-0.51}*/
	XtoSymb.insert(std::make_pair(s_t, 0));
	SymbToX.clear();
	for(int l = 0; l < L; l++)
		SymbToX.insert(std::make_pair(SymbX(l,0), X(l,0)));
	// Построчно проводим довыделение
	for(i = 0; i < L; i++)
	{
		ex tmpEx = -GetNotLinearPart(ODEs.op(i));
		ex newH = tmpEx;
		tmpEx = tmpEx.subs(XtoSymb).evalf();
		for(int l = 0; l < L; l++)
		{
			ex tmpExDiff = tmpEx.diff(ex_to<symbol>(SymbX(l,0)));
			//DEBUG_OUT("tmpExDiff = tmpEx.diff(ex_to<symbol>(SymbX(l,0))) = " << tmpExDiff)
			tmpExDiff = tmpExDiff.subs(SymbToX);
			tmpExDiff = ex_to_taylor_row(tmpExDiff, &Solutions);//.subs(XtoRm);
			//DEBUG_OUT("tmpExDiff = ex_to_taylor_row(tmpExDiff.subs(SymbToX)) = " << tmpExDiff)
			tmpExDiff = tmpExDiff.coeff(s_taylor, 0);
			//DEBUG_OUT("tmpExDiff = tmpExDiff.coeff(s_taylor, 0)) = " << tmpExDiff)
			if(!tmpExDiff.is_zero())
			{
				A.set(i,l,A(i,l) - tmpExDiff);
				tmpExDiff *= X(l,0);
				newH -= tmpExDiff.expand();
			}
		}
		H.set(i,0,Laplace_trans(newH));
	}
	DEBUG_OUT("A after = " << A)
	DEBUG_OUT("H after = " << H)
}

void ANM_system::Init_Q()
{
	// См. книгу Ю.А.Бычкова "Аналитически-численный расчет динамики нелинейных систем
	// детерминированные кусочно-степенные модели с сосредоточенными параметрами.
	// Переходные и периодические режимы.
	// Анализ, синтез, оптимизация."
	// СПб 1997 год; стр.23

	int u, l, lf, i;

// просмотр строк матриц A(p) и G(p)
	for(u = 0; u < L; u++)
	{
		for(l = 0; l < L; l++)
			Q.set(u, 0, Q(u,0) + GetQElement(A(u,l), X(l,0)) );
		for(lf = 0; lf < Lf; lf++)
			Q.set(u, 0, Q(u,0) + GetQElement(G(u,lf), F(lf,0)));
	}
//*/
}
