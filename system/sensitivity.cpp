#include "..\\rows\\rows.h"
#include "..\\parser\\parser.h"
#include "..\\chart\\chart.h"

extern sym_table sym_tab;
extern const ex ex_to_ppolinom(const ex& expr);
//----------------------- ANM_sensitivity class ---------------------------------------------
//

GINAC_IMPLEMENT_REGISTERED_CLASS(ANM_sensitivity, ANM_system)

ANM_sensitivity::ANM_sensitivity()
{
}

ANM_sensitivity::ANM_sensitivity(const ex& aInputData, const ex& aMu, numeric *aTime) // tN
{
	StepCntr = 0;
	Time_k.cur_t = aTime;
	InputData = aInputData;
	bfInitialaized = false;
	mu = aMu;
}

void ANM_sensitivity::archive(archive_node &n) const
{
	inherited::archive(n);
}

ANM_sensitivity::ANM_sensitivity(const archive_node &n, lst &sym_lst) : inherited(n, sym_lst)
{
}

ex ANM_sensitivity::unarchive(const archive_node &n, lst &sym_lst)
{
	return (new ANM_sensitivity(n, sym_lst))->setflag(status_flags::dynallocated);
}

int ANM_sensitivity::compare_same_type(const basic &other) const
{
	const ANM_sensitivity &o = static_cast<const ANM_sensitivity &>(other);
	if (InputData.is_equal(o.InputData))
		return 0;
	if(InputData.compare(o.InputData) < 0)
		return -1;
	return 1;
}

void ANM_sensitivity::Calculate(const lst& aICs, all_charts* AllResults)
{
	if(!bfInitialaized)
		Analitic();
	exmap tmpMap;
	tmpMap = sym_tab.GetMap();
	tmpMap.insert(std::make_pair(s_t, Time_k.evalf()));
	if(!is_a<numeric>(mu.subs(tmpMap).evalf()))
		DEBUG_OUT("!!!!!!!!!!!!!Error: '" << mu.subs(tmpMap).evalf() << "' is not a numeric! Perhaps, wrong sensitivity parameter")
	numeric aMu0 = ex_to<numeric>(mu.subs(tmpMap).evalf());
	//DEBUG_OUT("aMu0 = " << aMu0)
	numeric t0, t1;
/*	if(InputData.op(3).nops() > 2)
	{
		//DEBUG_OUT("InputData.op(3).op(2) = " << InputData.op(3).op(2))
		//DEBUG_OUT("InputData.op(3).op(2).op(0) = " << InputData.op(3).op(2).op(0))
		//DEBUG_OUT("InputData.op(3).op(2).op(1) = " << InputData.op(3).op(2).op(1))
		t0 = ex_to<numeric>(InputData.op(3).op(2).op(0).evalf());
		t1 = ex_to<numeric>(InputData.op(3).op(2).op(1).evalf());
		bSaveResult = (Time_k.evalf() >= t0) && (Time_k.evalf() <= t1);
	}
	else
*/		bSaveResult = true;
	if(!bSaveResult)
		return;
	StepCntr++;
	// Чтобы все цвета для разных моментов времени были одинаковые (так удобнее)
	ColorInd = 0;
	// сначала исследуем чувствительность на промежутке [aMu0; aMu0 + MuSup]
	ICs = aICs;
	InfT = aMu0;
	if(sym_tab.is_name("MuSup") + 1)
		SupT = aMu0 + sym_tab.Evalf("MuSup");
	else
		SupT = aMu0 + numeric(1);
	Minh = sym_tab.Evalf("Minh");
    Solutions.bMinus = (InfT > SupT);
    tN = InfT;
	if(Minh > abs(SupT - InfT))
		Minh = Minh*numeric(1e-6);
	Maxh = 0;
	if(sym_tab.is_name("Maxh") + 1)
		Maxh = sym_tab.Evalf("Maxh");
	if(Maxh == 0)
		Maxh = abs(SupT - InfT);
	if(Maxh > abs(SupT - InfT)/numeric(10))
		Maxh = abs(SupT - InfT)/numeric(10);
	Solutions.setICs(ICs);
	h = abs(SupT - InfT);
	//DEBUG_OUT("Maxh = " << Maxh)
	//DEBUG_OUT("SupT = " << SupT)
	//DEBUG_OUT("InfT = " << InfT)
	//DEBUG_OUT("aMu0 = " << aMu0)
	for(int i = 0; i < Solutions.size(); i++)
	{
		Solutions[i].GlobalD = 0;
		Solutions[i].newVar = NULL;
	}
	if(bSaveResult)
	{
		Result = AllResults->push_back(GetEmptyResult());
		for(int i = 0; i < Solutions.size(); i++)
			Result->results[i].name += " [s" + ToString(StepCntr) + "] +";
		Result->SensOwnerTime = ex_to<numeric>(Time_k.evalf());
	}
	while(CanEvulate())
		Evulate();
	// затем исследуем чувствительность на промежутке [aMu0; aMu0 - MuInf]
	// Чтобы все цвета для разных моментов времени были одинаковые обнуляем ColorInd (так удобнее)
	ColorInd = 0;
	ICs = aICs;
	InfT = aMu0;
	if(sym_tab.is_name("MuInf") + 1)
		SupT = aMu0 - sym_tab.Evalf("MuInf");
	else
		SupT = aMu0 - numeric(1);
	Minh = sym_tab.Evalf("Minh");
    Solutions.bMinus = (InfT > SupT);
    tN = InfT;
	if(Minh > abs(SupT - InfT))
		Minh = Minh*numeric(1e-6);
	Maxh = 0;
	if(sym_tab.is_name("Maxh") + 1)
		Maxh = sym_tab.Evalf("Maxh");
	if(Maxh == 0)
		Maxh = abs(SupT - InfT);
	if(Maxh > abs(SupT - InfT)/numeric(10))
		Maxh = abs(SupT - InfT)/numeric(10);
	Solutions.setICs(ICs);
	h = abs(SupT - InfT);
	for(int i = 0; i < Solutions.size(); i++)
	{
		Solutions[i].GlobalD = 0;
		Solutions[i].newVar = NULL;
	}
	if(bSaveResult)
	{
		Result = AllResults->push_back(GetEmptyResult());
		for(int i = 0; i < Solutions.size(); i++)
			Result->results[i].name += " [s" + ToString(StepCntr) + "] -";
		Result->SensOwnerTime = ex_to<numeric>(Time_k.evalf());
	}
	while(CanEvulate())
		Evulate();
	for(int i = 0; i < Solutions.size(); i++)
	{
		Solutions[i].GlobalD = 0;
		Solutions[i].newVar = NULL;
	}
}

void ANM_sensitivity::Analitic()
{
	StepCntr = 0;
    //DEBUG_OUT("ANM_sensitivity::Analitic")
    ex old_input_data = InputData;
	DEBUG_OUT("sens InputData before replace = " << InputData)
// заменяем во всех уравнениях mu на t
	int i = 0;
	while(i < InputData.op(0).nops())
		if(!is_a<defining>(InputData.op(0).op(i++))) // если это не x(0) = x0
		{
            ex tmpRel; // tmp relational
// Если это просто символ, то нужно поискать в таблице символов его, и подставить значение вместо символа
            if(is_a<symbol>(InputData.op(0).op(i-1)))
			{
				int k = sym_tab.is_name(ToString(InputData.op(0).op(i-1)));
				if(k != -1)
				{
					tmpRel = InputData.let_op(0).let_op(i-1);
					DEBUG_OUT("tmpRel = " << tmpRel)
					tmpRel = InputData.op(0).op(i-1).subs(sym_tab.GetSymMap()).subs(lst(mu), lst(s_t));
					DEBUG_OUT("tmpRel = " << tmpRel)
                    InputData.let_op(0).let_op(i-1) = InputData.op(0).op(i-1).subs(sym_tab.GetSymMap()).subs(lst(mu), lst(s_t));
				}
				else
					ERROR_10(InputData.op(0).op(i-1))
				//DEBUG_OUT("tmpRel = " << tmpRel)
			}
			else
                InputData.let_op(0).let_op(i-1) = InputData.op(0).op(i-1).subs(lst(mu), lst(s_t));
		}
	DEBUG_OUT("sens InputData after replace = " << InputData)
	inherited::Analitic(GetSymbol("ANM_sensitivity"));
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
			//DEBUG_OUT("ANM_sens{" << OutString << "}");
		}
    InputData = old_input_data;
    //DEBUG_OUT("end - ANM_sensitivity::Analitic")
}

void ANM_sensitivity::Evulate()
{
	//DEBUG_OUT("ANM_sensitivity::Evulate()");
// Обнулим решения - как-будто мы ничего не искали и у нас нет никаких данных кроме предначальных условий и полной погрешности расчета
	Solutions.null();
// выбор шага расчета
	h = Solutions.get_step(Forh, Speciality, Minh);
	if(h > Maxh)
		h = Maxh;
	if(sym_tab.is_name("StopTimes") + 1)
	{
		ex tmpEx = (GetSymbol("StopTimes").subs(sym_tab.GetMap())).evalf();
		for(int k = 0; k < tmpEx.nops(); k++)
			if(is_a<numeric>(tmpEx.op(k).evalf()))
				if(tmpEx.op(k).evalf() > tN && tmpEx.op(k).evalf() < tN + h)
				{
					h = ex_to<numeric>(tmpEx.op(k).evalf()) - tN;
					break;
				}
	}

// поиск полной погрешности расчета
	Solutions.set_global_delta(h, SupLocError, Speciality);

// если расчет ведется не по t а по x(t), то в функции get_time_step устанавливается флаг bMinus
// также если установлен флаг Solutions.bMinus то шаг возвражается со знаком минус
    cout << "mu_h = " << Solutions.get_time_step(h) << "\x0D\x0A" << flush;
// добавляем шаг и найденные значения в таблицу найденных значений. Также можем добавить все что хотим
	vector<result> cur_res;
	cur_res = Solutions.get_current_result(Speciality);
	if(bSaveResult)
	{
		if(Solutions.bMinus)
			for(int i = 0; i < cur_res.size(); i++)
				cur_res[i].name +=  " [s" + ToString(StepCntr) + "] -";
		else
			for(int i = 0; i < cur_res.size(); i++)
				cur_res[i].name +=  " [s" + ToString(StepCntr) + "] +";
		Result->AddResult(cur_res, tN, (1-2*Solutions.bMinus)*h);
	}
// передвигаем ось ординат на шаг вправо
	tN += Solutions.get_time_step(h);
    cout << "mu_t[N] = " << tN << "\x0D\x0A" << flush;
// переопределяем начальные условия: x(0+) = x(0-);
	Solutions.setICs((1-2*Solutions.bMinus)*h);
	//DEBUG_OUT("end - ANM_sensitivity::Evulate()");
}


//------------------------------ ANM_sens_system class----------------------------------------------------
GINAC_IMPLEMENT_REGISTERED_CLASS(ANM_sens_system, ANM_system)

ANM_sens_system::ANM_sens_system()
{
	AllResults = NULL;
}

ANM_sens_system::ANM_sens_system(const ex& aInputEx) : ANM_system(aInputEx)
{
	AllResults = NULL;
}

ANM_sens_system::ANM_sens_system(const lst& aInputEx) : ANM_system(aInputEx)
{
	AllResults = NULL;
}

/*void ANM_sens_system::copy(const ANM_sens_system &other)
{
	inherited::copy(other);
	delta_mu = other.delta_mu;
}*/

void ANM_sens_system::archive(archive_node &n) const
{
	inherited::archive(n);
}

ANM_sens_system::ANM_sens_system(const archive_node &n, lst &sym_lst) : inherited(n, sym_lst)
{
}

ex ANM_sens_system::unarchive(const archive_node &n, lst &sym_lst)
{
	return (new ANM_sens_system(n, sym_lst))->setflag(status_flags::dynallocated);
}

int ANM_sens_system::compare_same_type(const basic &other) const
{
	const ANM_sens_system &o = static_cast<const ANM_sens_system &>(other);
	if (InputData.is_equal(o.InputData))
		return 0;
	if(InputData.compare(o.InputData) < 0)
		return -1;
	return 1;
}

void ANM_sens_system::EvulateSensSystem(const numeric& curx)
{
	//Result;
	lst param_ICs;
	numeric tmptN = tN;
	tN = curx;
	// переделать для производных > 1
	for(int i = 0; i < Solutions.size(); i++)
		param_ICs.append(relational(ICs.op(i).op(0), Result->GetIC(ToString(Solutions[i].name), curx, 0)));
	//DEBUG_OUT("param_ICs = " << param_ICs)
	sens.Calculate(param_ICs, AllResults);
	tN = tmptN;
}

void ANM_sens_system::Evulate()
{
/*	lst param_ICs;
	// переделать для производных > 1
	//DEBUG_OUT("ANM_sens_system::Evulate()");
	for(int i = 0; i < Solutions.size(); i++)
	{
		//DEBUG_OUT("ICs.op(i).op(0) = " << ICs.op(i).op(0) << "; Solutions[i].name = " << Solutions[i].name);
		param_ICs.append(relational(ICs.op(i).op(0), Solutions[i].IC(0)));
	}
	sens.Calculate(param_ICs, AllResults);
*/	inherited::Evulate();
	//DEBUG_OUT("end - ANM_sens_system::Evulate()");
}

ANM_charts ANM_sens_system::GetEmptyResult()
{
	return inherited::GetEmptyResult();
}

void ANM_sens_system::Analitic(const ex& aFlags)
{
    //DEBUG_OUT("ANM_sens_system::Analitic")
	sens.InputData = InputData;
	///*debug*/ переделать
	sens.mu = InputData.op(3).op(0);
	sens.Time_k = &tN;
	sens.Analitic();
    //DEBUG_OUT("after sens.Analitic()")
	if(aFlags.is_equal(numeric(-1)))
		inherited::Analitic(aFlags);//GetSymbol("ANM_sens_system"));
	else
		inherited::Analitic(SubResults);
    //DEBUG_OUT("if(aFlags.is_equal(numeric(-1)))")
	if(!bfInitialaized)
		return;
    //DEBUG_OUT("end - ANM_sens_system::Analitic")
}



