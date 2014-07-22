#include "..\\rows\\rows.h"
#include "..\\parser\\parser.h"
#include "..\\chart\\chart.h"

extern sym_table sym_tab;

// возвращает номер производной IdName
int get_diff_level(const ex &in_ex, const ex &IdName)
{
	if(is_a<myprocedure>(in_ex))
	{
		if(in_ex.op(0).is_equal(IdName.op(0)))
			return 0;
		else if(in_ex.op(0).is_equal(s_diff))
			return 1+get_diff_level(in_ex.op(1).op(0), IdName);
	}
	ERROR_5(in_ex)
}

//------------------------------ unknown_sol class----------------------------------------------------
GINAC_IMPLEMENT_REGISTERED_CLASS(unknown_sol, basic)

unknown_sol::unknown_sol()
{
	sptr = 0;
	n = 0;
}

unknown_sol::unknown_sol(solution *aSptr, int aN)
{
	sptr = aSptr;
	n = aN;
}

void unknown_sol::archive(archive_node &aN) const
{
	inherited::archive(aN);
}

unknown_sol::unknown_sol(const archive_node &aN, lst &sym_lst) : inherited(aN, sym_lst)
{
}

ex unknown_sol::unarchive(const archive_node &aN, lst &sym_lst)
{
	return (new unknown_sol(aN, sym_lst))->setflag(status_flags::dynallocated);
}

int unknown_sol::compare_same_type(const basic &other) const
{
	const unknown_sol &o = static_cast<const unknown_sol &>(other);
	if(sptr->name.is_equal(o.sptr->name))
		return 0;
	else if(sptr->name.compare(o.sptr->name) < 0)
		return -1;
	else
		return 1;
}

void unknown_sol::print(const print_context &c, unsigned level) const
{
	if(n > 1)
		c.s << "diff(u_sol(" << (sptr->name) << "), t, " << n << ")";
	if(n == 1)
		c.s << "diff(u_sol(" << (sptr->name) << "), t)";
	if(n < 0)
		c.s << "int(u_sol(" << (sptr->name) << "), t, " << n << ")";
	if(n == 0)
		c.s << "u_sol(" << (sptr->name) << ")";
}

int unknown_sol::degree(const ex &s) const
{
	if(s.is_equal(s_p))
		return -1;
	return inherited::degree(s);
}

ex unknown_sol::coeff(const ex & s, int aN) const
{
	if(s.is_equal(s_taylor))
	{
		return sym_rk(sptr, aN + n);//const_cast<unknown_sol*>(this)->sptr->R(n);
	}
	else if(s.is_equal(s_p))
		return coeff(s_taylor, -(aN+1));
	else
		return inherited::coeff(s, aN);
}

ex unknown_sol::evalf(int level) const
{
    if(sptr)
        return sptr->numIC(0);
    else
        return 0;
}

ex unknown_sol::derivative(const symbol &s) const
{
	// если дифференцируем по t, то возвращаем производную от неизвестного решения, иначе 0
	if(s.is_equal(s_t))
		return unknown_sol(sptr, n + 1);
	else
		return 0;
}

//------------------------------ solution class----------------------------------------------------
solution::solution()
{
    //DEBUG_OUT("solution0" << "\x0D\x0A";
	iHold = -1;
	newVar = 0;
	sign = 0;
	dwMode = 0;
	J = 0;
	ZeroRk = 0;
	CurNumR = CurSymR = 100;
    GlobalD = 0;
	//DEBUG_OUT("end - solution0" << "\x0D\x0A";
}

solution::solution(const ex& exB, const ex& exA, const ex aName)
{
    //DEBUG_OUT("solution" << "\x0D\x0A";
	iHold = -1;
	newVar = 0;
	sign = 0;
	J = 0;
	A = exA;
	B = exB;
	name = aName;
	dwMode = 0;
	ZeroRk = 0;
	CurNumR = CurSymR = 100;
    //DEBUG_OUT("end - solution" << "\x0D\x0A";
}

solution::solution(const solution &other)
{
	iHold = -1;
	copy(other);
}

void solution::copy(const solution &other)
{
    //DEBUG_OUT("solutioncopy" << "\x0D\x0A";
	iHold = -1;
	newVar = 0;
	sign = other.sign;
	J = other.J;
	A = other.A;
	B = other.B;
	name = other.name;
	dwMode = other.dwMode;
	ZeroRk = 0;
	CurNumR = CurSymR = 100;
	GlobalD = other.GlobalD;
    //DEBUG_OUT("end - solutioncopy" << "\x0D\x0A";
}

solution::~solution()
{
	if(newVar)
		delete newVar;
	newVar = NULL;
}

ex solution::Rn(int i, ex lambda)
{
	/*debug version*/ //Сейчас реализовано только для R[n][0] и для N = 2
	int N = A.degree(s_p);
	ex sum1 = 0;
	ex sum2 = 0;
	for(int m = 1; m < N; m++)
	{
		sum1 += (N-m)*(-A.coeff(s_p, N-m)/A.coeff(s_p, N))*pow(lambda, numeric(N-m-1));
		sum2 += (N-m)*(-A.coeff(s_p, N-m)/A.coeff(s_p, N))*pow(GetSymbol("lambda"), numeric(N-m-1));
	}
	ex sumUp = 0;
	ex sumUp2 = 0;
	for(int m = 0; m <= i; m++)
	{
		sumUp += (B.coeff(s_p, N-m-1)/A.coeff(s_p, N))*pow(lambda, numeric(-m));
		sumUp2 += (B.coeff(s_p, N-m-1)/A.coeff(s_p, N))*pow(GetSymbol("lambda"), numeric(-m));
	}
	ex r_l_n = sumUp/(N*pow(lambda, numeric(N-1)) - sum1);
	ex r_l_n2 = sumUp2/(N*pow(GetSymbol("lambda"), numeric(N-1)) - sum2);
	DEBUG_OUT(name << ".Rk[" << (i) << "] = " << (r_l_n2*pow(GetSymbol("lambda"), numeric(N+i-1))))
	return r_l_n*pow(lambda, numeric(N+i-1));//было "-sum1);" сделал "+sum1);". Вероятно в формуле в книге ошибка - вместо "-" стоит "+"
}

ex solution::Rn(int n, int i)
{
	lst roots(ex_to<lst>(root_of(A, s_p)));
	return Rn(i, roots.op(n));
}

ex solution::R(int i)
{
	i = i - J;
	if(i<0)
		return 0;
	if(CurSymR >= i)
		return sym_rk(this, i);
	//DEBUG_OUT("solution::" << name << ".R[" << i << "]")
	int N = A.degree(s_p);
    //DEBUG_OUT("N = " << A << ".degree(s_p) = " << N)
	ex tmpR;
	while(i > int(Rk.size()) - 1)
	{
	    if(ZeroRk > N + 2 - J)
	    {
		    xI = int(Rk.size()) - 2;
		    return 0;
	    }
		int ii = int(Rk.size());
		CurSymR = int(Rk.size());
		//DEBUG_OUT("Evulating " << name << ".symR[" << CurSymR << "]")
		if(ii){
			//DEBUG_OUT("A = " << A)
			//DEBUG_OUT("B = " << B)
			tmpR = B.coeff(s_p, N - J - ii - 1);
			//DEBUG_OUT("B.coeff(s_p," << (N - J - ii - 1) << ") = " << tmpR)
			for(int k = 0; k < ii; k++)
			{
				//DEBUG_OUT("A.coeff(s_p, " << (N - ii + k) << ") = " << A.coeff(s_p, N - ii + k))
				tmpR -= A.coeff(s_p, N - ii + k)*sym_rk(this, k);//*/R(k);
			}
		}
		else
		{
            ex tmpA = A;
			A = tmpA.subs(sym_tab.GetMap());
		// R[0]
			//DEBUG_OUT("B.coeff(s_p, " << (N-1-J) << ") = " << B.coeff(s_p, N-1-J))
			tmpR = B.coeff(s_p, N-1-J);
		}
		ex tmpNumR = tmpR;
        tmpR = tmpNumR.normal(1) / A.coeff(s_p, N);
		//DEBUG_OUT("tmpR = tmpNumR.normal(1) / A.coeff(s_p, N) = " << tmpR)
		if(dwMode & RKMODE_EVALf)
			tmpR = tmpR.evalf();
		else if(dwMode & RKMODE_EVAL)
			tmpR = tmpR.eval();
		CurSymR = -1;
        Rk.push_back(tmpR);
		//DEBUG_OUT("R(size-1)[" << int(Rk.size()) - 1 << "] = " << tmpR)
        tmpNumR = tmpR.evalf();
		//DEBUG_OUT("tmpNumR = tmpR.evalf() = " << tmpNumR)
		if(tmpNumR.is_zero())
			ZeroRk++;
		else
			ZeroRk = 0;
		if(ii < 5)
			cout << name << ".R[" << int(Rk.size()) - 1 << "] = " << Rk[int(Rk.size()) - 1] << "\x0D\x0A" << flush;
		else if(!(i%5))
			cout << name << ".R[" << int(Rk.size()) - 1 << "] evulated" << "\x0D\x0A" << flush;
		//DEBUG_OUT("end - Evulating " << name << ".symR[" << CurSymR << "]")
	}
    //DEBUG_OUT(name << ".Rk[" << i << "] = " << Rk[i])
	return Rk[i];
}

numeric solution::numR(int i)
{
	if(i<0)
		return 0;
    //DEBUG_OUT("numRk[NE].size() = " << numRk[*NE].size())
    //DEBUG_OUT(name << ".numR[" << i << "]")
    //DEBUG_OUT("numR[" << i << "]... size = " << (numRk[*NE].size()) << "\x0D\x0A";
	// Если R[i] зависит от себя: R[i] = f(R[i],...), то решаем уравнение относительно него. Пока можно решить только уравнения первого, второго, третьего или четвертого порядка
	if(CurNumR == i)
	{
		ex tmpRk;
		ex tmpRi = GetSymbol("_root_R");//sym_rk(this, i);
		exmap aMap;
		aMap.clear();
		aMap.insert(std::make_pair(sym_rk(this, i), tmpRi));
		tmpRk = (R(i)).subs(aMap);
		iHold = 0;
		//DEBUG_OUT("for R[" << i << "].evalf: " << tmpRk.evalf())
		//DEBUG_OUT("for R[" << i << "].eval: " << tmpRk.eval())
		if(is_a<numeric>(tmpRk.evalf()))
		{
			if(!tmpRk.eval().has(tmpRi))
				Rk[i] = tmpRk.eval();
			iHold = -1;
			numRk[*NE][i]=(ex_to<numeric>(tmpRk.evalf()));
		}
		else
		{
			iHold = -1;
			lst real_roots;
			//DEBUG_OUT("tmpRi - R(" << i << ") = " << tmpRi - R(i))
			// Решаем уравение
			ex roots(root_of(tmpRi - tmpRk, tmpRi).evalf());
			//DEBUG_OUT("roots = " << roots)
			// определяем вещественные корни. Если корень не числовое значение, он не учитывается
			for(int k = 0; k < roots.nops(); k++)
			{
				//DEBUG_OUT("root_" << k << " = " << roots.op(k).evalf())
				if(is_a<numeric>(roots.op(k).evalf()))
				{
					numeric root_k = ex_to<numeric>(roots.op(k).evalf());
					if(is_real(root_k))
						real_roots.append(root_k);
				}
			}
			if(real_roots.nops() == 1)
				tmpRk = real_roots.op(0);
			else if(real_roots.nops() > 1)
			{
				tmpRk = real_roots.op(0);
			}
			else
				ERROR_3(name, i, R(i))
			aMap.clear();
			aMap.insert(std::make_pair(tmpRi,sym_rk(this, i)));
			tmpRk = tmpRk.subs(aMap);
			tmpRk = tmpRk.evalf();
			if(!is_a<numeric>(tmpRk))
			{
				ex tmpNumRk = tmpRk;
				tmpRk = numeric(0);
				tmpRk = tmpNumRk.subs(lst(s_t), lst(*curt)).evalf();
				tmpNumRk = tmpRk + numeric(1);
				while(!is_a<numeric>(tmpRk) && !tmpNumRk.is_equal(tmpRk))
				{
					tmpNumRk = tmpRk.subs(sym_tab.GetMap());
					tmpRk = tmpNumRk.evalf();
				}
			}
			if(!is_a<numeric>(tmpRk))
			{
				ERROR_4(name, i, tmpRk)
			}
			numRk[*NE].push_back(ex_to<numeric>(tmpRk));
		}
	}
	else
		while(i > int(numRk[*NE].size()) - 1)
		{
			ex tmpRk;
			CurNumR = numRk[*NE].size();
			tmpRk = R(numRk[*NE].size());
            //DEBUG_OUT(tmpRk)
			if(CurNumR == numRk[*NE].size())
			{
				ex tmpNumRk = tmpRk.evalf();
                //DEBUG_OUT("tmpNumRk = " << tmpNumRk)
		        if(!is_a<numeric>(tmpNumRk))
                {
                    ex tmp1 = tmpNumRk;
					exmap m = sym_tab.GetMap();
					m.insert(std::make_pair(s_t, *curt));
					tmpNumRk = tmp1.subs(m).evalf();
                    //tmpNumRk = tmp1.subs(lst(s_t), lst(*curt)).evalf();
                    //DEBUG_OUT("tmpNumRk = " << tmpNumRk)
                    tmp1 = tmpNumRk + numeric(1);
                    //DEBUG_OUT("tmp1 = " << tmp1)
                    while(!is_a<numeric>(tmpNumRk) && !tmp1.is_equal(tmpNumRk))
                    {
                        //DEBUG_OUT("tmp1 = " << tmp1)
                        //DEBUG_OUT("tmpNumRk = " << tmpNumRk)
                        tmp1 = tmpNumRk.subs(sym_tab.GetMap());
                        tmpNumRk = tmp1.evalf();
                    }
				    if(!is_a<numeric>(tmpNumRk))
				    {
					    int i = numRk[*NE].size();
					    ERROR_4(name, i, tmpNumRk)
				    }
                }
                numRk[*NE].push_back(ex_to<numeric>(tmpNumRk));
			}
		    if(i < 5)
                cout << name << ".numR[" << int(numRk[*NE].size()) - 1 << "] = " << numRk[*NE][int(numRk[*NE].size()) - 1] << "\x0D\x0A" << flush;
			CurNumR = -1;
		}
	return numRk[*NE][i];
}

result solution::get_empty_result()
{
	result res;
	res.name = ToString(name);
	res.bDraw = true;
	res.color = Colors[ColorInd];
	res.bkColor = res.color + 1;
	ColorInd++;
	if(Colors[ColorInd] == 0)
		ColorInd = 0;

	for(int i = 0; i < others.size(); i++)
	{
		res.others.push_back(ToString(others[i]));
		// если в имени результата есть "->R[1](0)" - первая составляющая , то делаем цвет зеленым
		if  (res.others[i].name.find("->R[1](0)")!=std::string::npos)
			res.others[i].color = Colors[21];
		// если в имени результата есть "->R[2](0)" - вторая составляющая , то делаем цвет коричневым
		else if  (res.others[i].name.find("->R[2](0)")!=std::string::npos)
			res.others[i].color = Colors[20];
		else
		{
			res.others[i].color = Colors[ColorInd];
			//DEBUG_OUT("res.others[i].bDraw = " << res.others[i].bDraw)
			ColorInd++;
			if(Colors[ColorInd] == 0)
				ColorInd = 0;
		}
	}
	for(int i = 0; i < other_rows.size(); i++)
	{
		res.other_rows.push_back(other_rows[i]);
		// если в имени результата есть lambda[1] - первый корень, то делаем цвет зеленым
		if (res.other_rows[i].name.find("lambda[1]")!=std::string::npos)
			res.other_rows[i].color = Colors[21];
		// если в имени результата есть lambda[2] - второй корень, то делаем цвет коричневым
		else if  (res.other_rows[i].name.find("lambda[2]")!=std::string::npos)
			res.other_rows[i].color = Colors[20];
		else
		{
			res.other_rows[i].color = Colors[ColorInd];
			ColorInd++;
			if(Colors[ColorInd] == 0)
				ColorInd = 0;
		}
	}
	res.bSquare = dwMode & ANMMODE_EVULATE_SQUARE;
	return res;
}

void solution::set_ic(const lst &ICs)
{
// ICs.op(i) -> diff(name(t), k)(0) == value
//	.op(0) -> diff(name(t), k)(0)
//		.op(0) -> diff(name(t), k)
//			.op(1).op(0) -> name(t)
//			.op(1).op(1) -> k
//	.op(1) -> value
	//DEBUG_OUT("setting ICs for " << name)
	//DEBUG_OUT("ICs = " << ICs)
	exmap tmpExmap = sym_tab.GetMap(), aMap;
	aMap.clear();
	for(size_t i = 0; i < ICs.nops(); i++)
		aMap.insert(std::make_pair(ICs.op(i).op(0), ICs.op(i).op(1)));
	for(exmap::const_iterator it = tmpExmap.begin(); it != tmpExmap.end(); it++)
		aMap.insert(std::make_pair(it->first, it->second));
	for(size_t i = 0; i < ICs.nops(); i++)
	{
		//DEBUG_OUT("ICs.op(i) = " << ICs.op(i))
		if(ICs.op(i).op(0).has(name) || ICs.op(i).op(0).op(0).is_equal(name.op(0)))
		{
			int k;
			if(ICs.op(i).op(0).op(0).is_equal(name.op(0)))
				k = 0;
			else
				k = get_diff_level(ICs.op(i).op(0).op(0), name);
			//DEBUG_OUT("k = " << k)
			while(k >= int(ICk.size()))
				ICk.push_back(diff_sol(this, ICk.size()));
            ex tmpEx = ICs.op(i).op(1);
            while(!tmpEx.subs(aMap).is_equal(tmpEx))
            {
                ex tmp1 = tmpEx;
                tmpEx = numeric(0);
                tmpEx = tmp1.subs(aMap);
            }
			if(is_a<numeric>(tmpEx.evalf()))
				ICk[k].nIC = ex_to<numeric>(tmpEx.evalf());
			else
				ERROR_6(name, ICs.op(i).op(1));
			//DEBUG_OUT("ICk[" << k << "].nIC = " << ICk[k].nIC)
		}
	}
}

void solution::set_ic(const numeric &h)
{
	if(newVar)
	{
		newVar->set_ic(h);
		newVar->set_ICs(ICk);
		return;
	}
	for(int i = 0; i < ICk.size(); i++)
    {
    	ICk[i].set_ic(h);
        //DEBUG_OUT("ICk[" << i << "] = " << ICk[i].nIC)
    }
}

numeric solution::evalf()
{
	numeric x = 0;
	if(!(*step).is_equal(0))
		for(int i = xI; i > 0; i--)
		{
			x += numR(i);
			x *= (*step)/numeric(i);
		}
	x += numR(0);
	return x;
}

ex solution::IC(int i)
{
	if(i < 0)
		return 0;
	if(ICk.size() == 0)
		return sym_IC(this, i);
	//DEBUG_OUT("solution::IC name = " << name << "; i = " << i << "; ICk.size() = " << ICk.size())
	if(i >= ICk.size())
		ERROR_7(name, i+1)
	return ICk[i].numIC();
}

numeric solution::numIC(int i)
{
	if(i < 0)
		return 0;
	//DEBUG_OUT("solution::numIC name = " << name << "; i = " << i << "; ICk.size() = " << ICk.size())
	if(i >= ICk.size())
		ERROR_7(name, i+1)
	return ICk[i].numIC();
}

int solution::set_max_r_ind(int &m, int &i, int Forh)
{
    //DEBUG_OUT("set_max_r_ind" << "\x0D\x0A";
	numeric maxR[3];						// maxR - максимум среди мажорант
	numeric curR[3];						// текущая мажоранта
	int maxInd[3];
	int N = A.degree(s_p);

	maxR[0] = maxR[1] = 0;

	for(int ii = 0; ii < Forh; ii++)
	{
		if(ZeroRk > N + 2 && ii >= Rk.size())
		{
			m = maxInd[0];
			return 0;
		}
		curR[0] = abs(numR(ii));
		curR[1] = curR[0]/factorial(numeric(ii));
		// kind - тип мажоранты
		for(int kind = 0; kind < 2; kind++)
			if(curR[kind] >= maxR[kind])
			{
				maxR[kind] = curR[kind];
				maxInd[kind] = ii;
			}
	}
// надо начинать с последнего kind чтобы шаг был не tN
//	for(int kind = 1; kind >= 0; kind--)
// или с первого, чтобы шаг был tN
	for(int kind = 0; kind < 2; kind++)
		if(maxInd[kind] < Forh - Forh/5)	// если среди R[i] или R[i]/i! есть максимум, то возвращается его индекс
		{
			m = maxInd[kind];
			return kind + 1;
		}

// если среди R[i] или R[i]/i! нет максимумов, то нужно делать максимум выбирая tau;
/*
	q = 1 - (tau(i1) - tau(i)) / tau(i)
	вообще-то нужно стремиться к тому, чтобы q было ближе к 1: 0,9 - 0,98 - это очень хорошо
*/
	numeric tau, tau1, dq = 0, pre_q = 2, hk, hm;
	int i1;

	m = 0;
//	выбор m
	while(is_zero(numR(m)) && m < 100)
		m++;
//	выбор начального i
	i = Forh - 1 + m;
	while(is_zero(numR(i)) && i > 0)
		i--;
	while(1)
	{
		i1 = i+1;
    //DEBUG_OUT(name << ".R[" << m << "] = " << numR(m) << " i1 = " << i1 << "\x0D\x0A";
    //DEBUG_OUT(name << ".R[" << i1 << "] = " << numR(i1) << "\x0D\x0A";
		while(is_zero(numR(i1)) && i1 < Forh*10)
			i1++;
		tau = pow(abs(numR(m)/numR(i)), numeric(1)/numeric(i - m));
//	проверка, является ли выбранный R[m]*tau^m максимумом
//	при этом способе выбора tau максимум может быть либо при m = 0, либо при m = 1
		numeric CurMax = abs(numR(m)) * pow(tau, numeric(m));
		hk = 1;
		int new_m = m;
		for(int k = m; k < 3; k++)
		{
			if(k)
				hk *= tau;
			if(CurMax < abs(numR(k))*hk)
			{
				new_m = k;
				CurMax = abs(numR(k))*hk;
			}
		}
		if(new_m != m)
		{
//			if(new_m > m + 1)
//				throw runtime_error("m > 1 при выборе шага третьим способом для " + ToString(name));
			m = new_m;
			tau = pow(abs(numR(m)/numR(i)), numeric(1)/numeric(i - m));
		}
		tau1 = pow(abs(numR(m)/numR(i1)), numeric(1)/numeric(i1 - m));
		q = numeric(1) - abs(tau1 - tau)/tau;
		dq = abs(q - pre_q);
		if(q > 0.9)
		{
			q = 0.9;
			break;
		}
        if(dq < numeric("1e-8"))
			break;
		i = i1;
		pre_q = q;
	}
	return 3;
    // Опрделение радиуса сходимости по признаку Даламбера
    i1++;
    q = pow(abs(numR(i)/factorial(numeric(i))), numeric(-1)/numeric(i));
    if(!is_zero(numR(i1)))
    {
        // Опрделение радиуса сходимости по признаку Коши
        numeric rKouchy = abs(numR(i1-1)/numR(i1)*numeric(i1));
        //DEBUG_OUT("rK = " << rKouchy << "\nq = " << q)
        q = max(q, rKouchy);
    }
    q = q/numeric(2);
	return 4;
}

numeric solution::get_step(int Forh, bool EvulateGlobal, const numeric &min_step)
{
    //DEBUG_OUT("solution::get_step: " << name << ".ICk.size() = " << ICk.size())
	int m, i;
	numeric h;
	numeric tau;
	TypeOfMaxR = set_max_r_ind(m, i, Forh);
    //DEBUG_OUT("TypeOfMaxR = " << TypeOfMaxR)
	MaxRInd = m;
	switch(TypeOfMaxR){
	case 0:
		h = 10;	// если нашли первым способом и N подряд идущих коэффициентов R нулевые, то мы нашли точное решение
		break;
	case 1:
		h = 1.5;	// если нашли первым способом, то шаг - любое число
		break;
	case 2:
		h = numeric(1)/numeric(2);			// если нашли вторым способом, то шаг - любое число меньше единицы
		break;
	case 3:					// если нашли третьим способом, то шаг выбирается в функции get_max_r_ind. m - номер максимума среди R[k]*tau^k; i - номер максимального учтенного коэффициента (50, но >= owner->Forh)
		tau = pow(abs( numR(m)/numR(i) ), numeric(1)/numeric(i - m));
		h = q*tau;
		break;
	case 4:					// шаг найден по признаку Коши или Даламбера
		h = q;
        TypeOfMaxR = 3;
		break;
	default:
		ERROR_8(name)
	}
    numeric h1;
	if(EvulateGlobal)
	{
		h1 = phi[0].get_step(Forh, 0);
		h = min(h,h1);
		h1 = phi[2].get_step(Forh, 2);
		h = min(h,h1);
		h1 = phi[1].get_step(Forh, 1);
		h = min(h,h1);
	}
    for(int i = 1; i < ICk.size(); i++)
    {
        h1 = ICk[i].get_step(Forh, EvulateGlobal, min_step);
		h = min(h,h1);
    }
	if(h < min_step)
	{
         //DEBUG_OUT("h < min_step")
		// если уже воспользовались 1/решение, то все, поезд ушел
		if(dynamic_cast<discont*>(this) != NULL)
			return h;
		newVar = new discont(this);
		h = newVar->get_step(Forh, EvulateGlobal, min_step);
	}
	return h;
}

// Способы формирования I см. в зеленой книге на стр. 33-34
void solution::set_I(const numeric &h, const numeric &aEpsilon, bool EvulateGlobal)
{
	if(newVar)
	{
		newVar->set_I(h, aEpsilon, EvulateGlobal);
		return;
	}
	if(ZeroRk > 1)
	{
		int N = A.degree(s_p);
		if(ZeroRk > N + 2)
		{
			LocalD = 0;
			xI = int(Rk.size()) - 2;
			if(EvulateGlobal)
			{
				phi[1].xI = xI;
				phi[0].xI = xI;
				phi[2].xI = xI;
				phi[1].LocalD = 0;
				phi[0].LocalD = 0;
				phi[2].LocalD = 0;
			}
			return;
		}
	}

	int k, im = MaxRInd;
	numeric fact = 1, sum = 1, hk = 1;
	numeric e;
	numeric c = abs(numR(im));
	LocalD = aEpsilon + numeric(1);

// Когда ведется подсчет глобальной (полной) погрешности расчета необходимо искать порядки для функций приращения для верхней и нижней границ
// эти порядки = порядок ограничивающего полинома для реакции - 1
	xI = 0;
	switch(TypeOfMaxR){		// Выбор I происходит разными способами, в зависимости от того, каким способом был выбран шаг
	case 0:
	case 1:
		e = exp(numeric(h));
		while(LocalD > aEpsilon)
		{
			xI++;
			fact *= h/numeric(xI);
			sum += fact;
			LocalD = c*abs(e - sum);
		}
		break;
	case 2:
		fact = factorial(numeric(im));
		e = h;
		while(LocalD > aEpsilon)
		{
			xI++;
			e *= h;
			LocalD = (c/fact)*(e/(numeric(1)-h));
		}
		break;
	case 3:
		e = exp(numeric(1));
		for(k = 0; k < im; k++)
			hk *= h;
		c = abs(hk*numR(im));
		while(LocalD > aEpsilon)
		{
			xI++;
			fact /= numeric(xI);
			sum += fact;
			LocalD = c*abs(e - sum);
		}
		break;
	}
	xI = xI + ICk.size();
	if(EvulateGlobal)
	{
		phi[1].set_I(1);
		phi[0].set_I(0);
		phi[2].set_I(2);
	}
    for(int i = 1; i < ICk.size(); i++)
        ICk[i].set_I(h, aEpsilon, EvulateGlobal);
}

void solution::set_global_delta(const numeric &h, bool EvulateGlobal)
{
    //DEBUG_OUT("solution::set_global_delta: " << name << ".ICk.size() = " << ICk.size())
	if(GlobalD == 0)
		mu_min = mu_max = numIC(0);
	if(mu_min > numIC(0) - GlobalD)
		mu_min = numIC(0) - GlobalD;
	if(mu_max < numIC(0) + GlobalD)
		mu_max = numIC(0) + GlobalD;
	//DEBUG_OUT(name << ".mu_min = " << mu_min)
	//DEBUG_OUT(name << ".mu_max = " << mu_min)
	numeric GlobalD0;
	preGlobalD = GlobalD;
    //DEBUG_OUT("newVar = " << newVar);
	if(newVar)
	{
		if(dynamic_cast<discont*>(newVar) != NULL)
		{
            //DEBUG_OUT(name << ".nIC = " << numIC(0));
            //DEBUG_OUT("tmpGlob1 = GlobalD / " << ICk[0].nIC*(ICk[0].nIC+GlobalD));
			numeric tmpGlob1 = abs(GlobalD/(ICk[0].nIC*(ICk[0].nIC+GlobalD)));
            //DEBUG_OUT("tmpGlob2 = GlobalD / " << ICk[0].nIC*(ICk[0].nIC-GlobalD));
			numeric tmpGlob2 = abs(GlobalD/(ICk[0].nIC*(ICk[0].nIC-GlobalD)));
			newVar->GlobalD = max(tmpGlob1, tmpGlob2);
			//DEBUG_OUT("newVar->GlobalD(0-){y} = " << newVar->GlobalD);
			//DEBUG_OUT("GlobalD(0){x} = " << GlobalD);
			//DEBUG_OUT("ICk[0].nIC{x} = " << ICk[0].nIC);
			//newVar->GlobalD = abs(GlobalD/(ICk[0].nIC*ICk[0].nIC-GlobalD*GlobalD));
		}
		else
        {
            //DEBUG_OUT("else newVar->GlobalD");
			newVar->GlobalD = GlobalD;
        }

        //DEBUG_OUT("newVar->set_global_delta");
		newVar->set_global_delta(h, EvulateGlobal);

        //DEBUG_OUT("dynamic_cast 2");
		if(dynamic_cast<discont*>(newVar) != NULL)
		{
			numeric tmpGlob1 = abs(newVar->GlobalD/(newVar->evalf()*(newVar->evalf() + newVar->GlobalD)));
			numeric tmpGlob2 = abs(newVar->GlobalD/(newVar->evalf()*(newVar->evalf() - newVar->GlobalD)));
			GlobalD = max(tmpGlob1,tmpGlob2);
			//DEBUG_OUT("newVar->GlobalD(h){y} = " << newVar->GlobalD);
			//DEBUG_OUT("newVar->Value(h){y} = " << newVar->evalf());
			//DEBUG_OUT("GlobalD(0){x} = " << GlobalD);
			//GlobalD = abs(newVar->GlobalD/(newVar->evalf()*newVar->evalf()-newVar->GlobalD*newVar->GlobalD));
		}
		else
			GlobalD = newVar->GlobalD;
		return;
	}
	if(EvulateGlobal)
	{
		if(preGlobalD == 0)
		{
			GlobalD = LocalD;
			return;
		}
		numeric xPhi, PhiPlus, PhiMinus; // x - найденное значение;	yp -- x + GlobalD; ym -- x - GlobalD;
		numeric kappaM, kappaP;
		xPhi = phi[1].evalfPhi();
		*NE = 0;
		PhiMinus = phi[0].evalfPhi();
		*NE = 2;
		PhiPlus = phi[2].evalfPhi();
		*NE = 1;

		kappaM = xPhi + phi[1].LocalD - PhiMinus + phi[0].LocalD;
		kappaP = PhiPlus + phi[2].LocalD - xPhi + phi[1].LocalD;
		if(kappaM < 0)
			kappaM = 0;
		if(kappaP < 0)
			kappaP = 0;
		kappa = (max(kappaM, kappaP));
		cout << name << ".kappa = " << kappa << "\x0D\x0A" << flush;
		kappa /= GlobalD;
		GlobalD0 = (numeric(1) + h*kappa)*GlobalD; //GlobalD0 -- погрешность, вносимая неправильно заданными начальными данными
	}
	else
		GlobalD0 = preGlobalD;
	GlobalD = GlobalD0 + LocalD;
    //DEBUG_OUT("for(int i = 1; i < ICk.size(); i++)");
    if(dynamic_cast<discont*>(this) == NULL)
        for(int i = 1; i < ICk.size(); i++)
            ICk[i].set_global_delta(h, EvulateGlobal);
    //DEBUG_OUT("end - set_global_delta");
}

result solution::get_current_result(bool EvulateGlobal)
{
	result ret;
	ret.name = ToString(name);
	numArray Rs;
	if(newVar)
	{
		newVar->numR(newVar->xI + 1);
		Rs.assign(newVar->numRk[1].begin(), newVar->numRk[1].begin() + newVar->xI + 1); // именно так и никак иначе: см. определение ICk - функция set_IC(h)
		ret.Rk.push_back(taylor_spline_element(Rs, newVar->GlobalD, newVar->preGlobalD, dynamic_cast<discont*>(newVar) != NULL));
        if(EvulateGlobal)
        {
		    *NE = 0;
		    newVar->numR(newVar->phi[0].xI + 1);
		    Rs.assign(newVar->numRk[0].begin(), newVar->numRk[0].begin() + newVar->phi[0].xI + 1); // именно так и никак иначе: см. определение ICk - функция set_IC(h)
		    ret.PhiMinusRk.push_back(taylor_spline_element(Rs, 0, 0));
		    *NE = 2;
		    newVar->numR(newVar->phi[2].xI + 1);
		    Rs.assign(newVar->numRk[2].begin(), newVar->numRk[2].begin() + newVar->phi[2].xI + 1); // именно так и никак иначе: см. определение ICk - функция set_IC(h)
		    ret.PhiPlusRk.push_back(taylor_spline_element(Rs, 0, 0));
		    *NE = 1;
        }
	}
	else
	{
		numR(xI + 1);
		Rs.assign(numRk[1].begin(), numRk[1].begin() + xI + 1); // именно так и никак иначе: см. определение ICk - функция set_IC(h)
		ret.Rk.push_back(taylor_spline_element(Rs, GlobalD, preGlobalD, false));
	    if(EvulateGlobal)
	    {
		    *NE = 0;
		    numR(phi[0].xI + 1);
		    Rs.assign(numRk[0].begin(), numRk[0].begin() + phi[0].xI + 1); // именно так и никак иначе: см. определение ICk - функция set_IC(h)
		    ret.PhiMinusRk.push_back(taylor_spline_element(Rs, 0, 0));
		    *NE = 2;
		    numR(phi[2].xI + 1);
		    Rs.assign(numRk[2].begin(), numRk[2].begin() + phi[2].xI + 1); // именно так и никак иначе: см. определение ICk - функция set_IC(h)
		    ret.PhiPlusRk.push_back(taylor_spline_element(Rs, 0, 0));
		    *NE = 1;
	    }
	}
	for(int i = 0; i < others.size(); i++)
	{
		ret.others.push_back(sub_res(ToString(others[i])));
		if(is_a<numeric>(others[i].evalf()))
			ret.others[i].push_back(ex_to<numeric>(others[i].evalf()));
	}
	for(int i = 0; i < other_rows.size(); i++)
	{
		ret.other_rows.push_back(other_rows[i]);
		Rs.clear();
		if(ret.other_rows[i].ex_mode != 2)
			for(int k = 0; k < xI+10; k++)
			{
				//DEBUG_OUT("other_rows[i].stat = " << other_rows[i].stat)
				//DEBUG_OUT("stat.R[" << k << "].evalf = " << other_rows[i].stat.coeff(s_taylor, k).evalf())
				Rs.push_back(ex_to<numeric>(other_rows[i].stat.coeff(s_taylor, k).evalf()));
			}
		ret.other_rows[i].Rk.push_back(taylor_spline_element(Rs, 0, 0, false));
		Rs.clear();
		if(ret.other_rows[i].ex_mode != 2)
			for(int k = 0; k < xI+10; k++)
			{
				//DEBUG_OUT("other_rows[i].for_y = " << other_rows[i].for_y)
				//DEBUG_OUT(other_rows[i].for_y << ".coeff(s_taylor," << k << ").evalf = " << other_rows[i].for_y.coeff(s_taylor, k).evalf())
				Rs.push_back(ex_to<numeric>(other_rows[i].ex_stat.coeff(s_taylor, k).evalf()));
			}
		ret.other_rows[i].yRk.push_back(taylor_spline_element(Rs, 0, 0, false));
	}
	return ret;
}

//---------- Класс new_var -----------------------------------------------------
new_var::new_var(solution *aSol)
{
    preGlobalD = GlobalD = 0;
	sol = aSol;
	A = 1;
	B = 1;
	NE = sol->NE;
	curt = sol->curt;
	step = sol->step;
    name = sol->name;
    newVar = NULL;
	for(int k = 0; k < 3; k++)
		phi[k].Init(this, k-1);
    if(newVar != 0)
        cout << "***** debug error: newVar ne nil *****" << "\x0D\x0A" << flush;
}

new_var::new_var(const new_var &other)
{
	sol = other.sol;
	A = 1;
	B = 1;
	NE = sol->NE;
	curt = sol->curt;
	step = sol->step;
    name = sol->name;
    newVar = NULL;
	for(int k = 0; k < 3; k++)
		phi[k].Init(this, k-1);
}

numeric new_var::numR(int i)
{
	return 0;
}

void new_var::set_ICs(vector<diff_sol> &ICs)
{
}

//---------- Класс diff_sol -----------------------------------------------------
diff_sol::diff_sol(solution *aSol, int aN):new_var(aSol)
{
    GlobalD = preGlobalD = aSol->GlobalD;
    n = aN;
    if(n > 0)
        ICk.push_back(diff_sol(this, 0));
}

diff_sol::diff_sol(const diff_sol &other):new_var(other.sol)
{
    GlobalD = preGlobalD = other.GlobalD;
    n = other.n;
    nIC = other.nIC;
    if(n > 0)
        ICk.push_back(diff_sol(this, 0));
}

numeric diff_sol::numR(int i)
{
	return sol->numR(i+n);
}

numeric diff_sol::numIC()
{
    if(0 == n)
        GlobalD = sol->GlobalD;
    //DEBUG_OUT("diff_sol::numIC")
    if(ICk.size())
        ICk[0].nIC = nIC;
	return nIC + ((*NE) - 1)*GlobalD;
}

void diff_sol::set_ic(const numeric &h)
{
    //DEBUG_OUT("diff_sol::set_ic")
	int k;
    nIC = 0;
    if(0 == n)
        xI = sol->xI;
	for(k = xI; k > 0; k--)
	{
		nIC += numR(k);
		nIC *= h/numeric(k);
	}
	nIC += numR(k);
    if(ICk.size())
    {
        ICk[0].nIC = nIC;
        //DEBUG_OUT(name << ".ICk[0].nIC = " << ICk[0].nIC)
    }
    //DEBUG_OUT(name << ".nIC = " << nIC)
}

//---------- Класс discont -----------------------------------------------------
discont::discont(solution *aSol):new_var(aSol)
{
    // вместо решения будем исследовать 1/решение
	Rk_ex = row_power_z(unknown_sol(sol), -1);
    // Нам понадобятся предначальные условия в конце шага...
    for(int i = 0; i < aSol->ICk.size(); i++)
        ICk.push_back(diff_sol(this, i));
}

discont::discont(const discont &other):new_var(other.sol)
{
    // здесь нужно добавлять что-то, чтобы все правильно работало
    cout << "Error: copy constructor for class discont" << "\x0D\x0A" << flush;
    // вместо решения будем исследовать 1/решение
	Rk_ex = other.Rk_ex;
    for(int i = 0; i < other.ICk.size(); i++)
        ICk.push_back(diff_sol(this, i));
}

numeric discont::numR(int i)
{
    //DEBUG_OUT("discont::numR")
	while(i > int(numRk[*NE].size()) - 1)
	{
        try{
		    ex tmpRk = Rk_ex.coeff(s_taylor, numRk[*NE].size()).evalf();
		    if(is_a<numeric>(tmpRk))
			    numRk[*NE].push_back(ex_to<numeric>(tmpRk));
		    else
			    ERROR_4(sol->name, i, tmpRk)
		}
        catch (const exception &e)
        {
			cout << "Error: " + ToString(e.what()) << "\x0D\x0A" << flush;
        }
	}
	return numRk[*NE][i];
}

void discont::set_ICs(vector<diff_sol> &ICs)
{
    ex tmpIC = numeric(1)/sym_IC(this, 0);
	for(int i = 0; i < ICs.size(); i++)
	{
    	ICs[i].nIC = ex_to<numeric>(tmpIC.evalf());
        if(i > 0)
            ICs[i].ICk[0].nIC = ICs[i].nIC;
        tmpIC = tmpIC.diff(s_t);
        //DEBUG_OUT(tmpIC);
	}
}

//---------- Класс phase -----------------------------------------------------
phase::phase(solution *aSol, solution *aVar):new_var(aSol)
{
    if(aVar)
    {
	    // Rk_ex = diff(решение)/diff(новая независимая переменная);
	    ex rmul = row_power_z(myprocedure(s_diff, lst(unknown_sol(aVar), s_t)), -1);
	    rmul *= myprocedure(s_diff, lst(unknown_sol(sol), s_t));
	    Rk_ex = row_mul(ex_to<mul>(rmul));
        for(*NE = 0; *NE < 3; (*NE)++)
		    numRk[*NE].push_back(sol->numR(0));
        *NE = 1;
    }
    else
    {
	    // Rk_ex = 1/diff(новая независимая переменная) == dt/dx
	    Rk_ex = row_power_z(myprocedure(s_diff, lst(unknown_sol(aSol), s_t)), -1);
        // R[0] = tN == t(0-)
        for(*NE = 0; *NE < 3; (*NE)++)
    	    numRk[*NE].push_back(*curt);
        *NE = 1;
    }
    // Нам понадобятся предначальные условия в конце шага...
    for(int i = 0; i < aSol->ICk.size(); i++)
        ICk.push_back(diff_sol(this, i));

}

phase::phase(const phase &other):new_var(other.sol)
{
    // здесь нужно добавлять что-то, чтобы все правильно работало
    cout << "Error: copy constructor for class phase" << "\x0D\x0A" << flush;
    // вместо решения будем исследовать 1/решение
	Rk_ex = other.Rk_ex;
}

numeric phase::numR(int i)
{
    //DEBUG_OUT("phase::numR")
	while(i > int(numRk[*NE].size()) - 1)
	{
        try{
		// в конструкторе, если переходим к новой независимой переменной, добавляется R[0]...
		    ex tmpRk = Rk_ex.coeff(s_taylor, numRk[*NE].size()-1).evalf();
		    if(is_a<numeric>(tmpRk))
			    numRk[*NE].push_back(ex_to<numeric>(tmpRk));
		    else
			    ERROR_4(sol->name, i, tmpRk)
		}
        catch (const exception &e)
        {
			cout << "Error: " + ToString(e.what()) << "\x0D\x0A" << flush;
        }
	}
	return numRk[*NE][i];
}

void phase::set_ICs(vector<diff_sol> &ICs)
{
	ICs[0].nIC = ICk[0].nIC;
}
