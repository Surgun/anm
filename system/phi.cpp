#include "..\\rows\\rows.h"
#include "..\\parser\\parser.h"
#include "..\\chart\\chart.h"

//----------------------------------------------------------------------------//
Phi::Phi()
{
}

Phi::~Phi()
{
}

void Phi::Init(solution* aOwner, numeric aSign)
{
	Sign = aSign;
	owner = aOwner;
	name = ToString(owner->name) + ".Phi";
	if(Sign > 0)
		name += "+";
	if(Sign < 0)
		name += "-";
}

int
Phi::get_max_r_ind(int &m, int &i, int Forh)
{
	 //DEBUG_OUT("Phi::get_max_r_ind")
    numeric maxR[3];						// maxR - максимум среди мажорант
	numeric curR[3];						// текущая мажоранта
	int maxInd[3];
	int N = owner->A.degree(s_p);
	 //DEBUG_OUT("int N = owner->A.degree(s_p)")

	maxR[0] = maxR[1] = 0;
	for(int ii = 0; ii < Forh; ii++)
	{
		if(owner->ZeroRk > N + 2 && ii >= owner->Rk.size())
		{
			m = maxInd[0];
			return 0;
		}
	 //DEBUG_OUT("after if")
		curR[0] = abs(R(ii));
	 //DEBUG_OUT("curR[0] = abs(R(ii));")
		curR[1] = curR[0]/factorial(numeric(ii));
	 //DEBUG_OUT("curR[1] = curR[0]/factorial(numeric(ii));")
		// kind - тип мажоранты
		for(int kind = 0; kind < 2; kind++)
			if(curR[kind] >= maxR[kind])
			{
				maxR[kind] = curR[kind];
				maxInd[kind] = ii;
			}
	}
	 //DEBUG_OUT("for(int ii = 0; ii < Forh; ii++)")
// надо начинать с последнего kind чтобы шаг был не tN
//	for(int kind = 1; kind >= 0; kind--)
// или с первого, чтобы шаг был tN
	for(int kind = 0; kind < 2; kind++)
		if(maxInd[kind] < Forh - Forh/5)	// если среди R[i] или R[i]/i! есть максимум, то возвращается его индекс
		{
			m = maxInd[kind];
			return kind + 1;
		}

	 //DEBUG_OUT("// если среди R[i] или R[i]/i! нет максимумов,")
// если среди R[i] или R[i]/i! нет максимумов, то нужно делать максимум выбирая tau;
/*
	q = 1 - (tau(i1) - tau(i)) / tau(i)
	вообще-то нужно стремиться к тому, чтобы q было ближе к 1: 0,9 - 0,98 - это очень хорошо
*/
	numeric tau, tau1, dq = 0, pre_q = 2, hk, hm;
	int i1;
	m = 0;
//	выбор m
	while(is_zero(R(m)) && m < 100)
		m++;
//	выбор начального i
	i = Forh - 1 + m;
	while(is_zero(R(i)) && i > 0)
		i--;
	while(1)
	{
		i1 = i+1;
		while(is_zero(R(i1)) && i1 < Forh*10)
			i1++;
	 //DEBUG_OUT("before tau = pow(abs(R(m)/R(i))")
		tau = pow(abs(R(m)/R(i)), numeric(1)/numeric(i - m));
//	проверка, является ли выбранный R[m]*tau^m максимумом
//	при этом способе выбора tau максимум может быть либо при m = 0, либо при m = 1
		numeric CurMax = abs(R(m)) * pow(tau, numeric(m));
		hk = 1;
		int new_m = m;
		for(int k = m; k < 3; k++)
		{
			if(k)
				hk *= tau;
			if(CurMax < abs(R(k))*hk)
			{
				new_m = k;
				CurMax = abs(R(k))*hk;
			}
		}
		if(new_m != m)
		{
			if(new_m > m + 1)
				cout << "!!!!!!!! warning: m > 1 при выборе шага третьим способом для " << ToString(name) << endl << flush;
			m = new_m;
			tau = pow(abs(R(m)/R(i)), numeric(1)/numeric(i - m));
		}
		tau1 = pow(abs(R(m)/R(i1)), numeric(1)/numeric(i1 - m));
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
}

numeric
Phi::evalf()
{
	numeric x = 0;

	for(int i = xI; i > 0; i--)
	{
		x += owner->numR(i);
		x *= *owner->step/numeric(i);
	}
	x += owner->numR(0);
	return x;
}

numeric
Phi::evalfPhi()
{
//	return (evalf() - numIC(0))/(* owner->step);
	numeric x = 0;

	for(int i = xI; i > 0; i--)
	{
		x += R(i);
		x *= (*owner->step)/numeric(i);
	}
	x += R(0);
	return x;
}

/*numeric
Phi::numIC(int n)
{
	return owner->numIC(n) + Sign*owner->GlobalD;	// это пока используются только производные первого порядка
}
*/
numeric
Phi::get_step(int Forh, int tmpNE)
{
	*(owner->NE) = tmpNE;
	int m, i;
	numeric tau;
	TypeOfMaxR = get_max_r_ind(m, i, Forh);
     //DEBUG_OUT("TypeOfMaxR = get_max_r_ind(m, i, Forh);")
	MaxRInd = m;
	switch(TypeOfMaxR){
	case 0:
		*(owner->NE) = 1;
		return 10;	// если нашли первым способом, то шаг - любое число
	case 1:
		*(owner->NE) = 1;
		return 1.5;	// если нашли первым способом, то шаг - любое число
	case 2:
		*(owner->NE) = 1;
		return numeric(1)/numeric(2);			// если нашли вторым способом, то шаг - любое число меньше единицы
	case 3:					// если нашли третьим способом, то шаг выбирается в функции get_max_r_ind. m - номер максимума среди R[k]*tau^k; i - номер максимального учтенного коэффициента (50, но >= Forh)
		tau = pow(abs(R(m)/R(i)), numeric(1)/numeric(i - m));
		*(owner->NE) = 1;
		return q*tau;
	default:
		ERROR_8(name)
		return 0;
	}
}

numeric
Phi::R(int i)
{
    //DEBUG_OUT("owner->numR(i+1) = " << owner->numR(i+1))
	return owner->numR(i+1)/numeric(i+1);
}

// Способы формирования I см. в зеленой книге на стр. 33-34
//
//
void
Phi::set_I(int tmpNE)
{
	*(owner->NE) = tmpNE;
	int k, im = MaxRInd, ii;
	numeric fact = 1, sum = 1, hk = 1;
	numeric e;
	numeric c = abs(R(im));

// В случае с функцией приращения для приближенного решения (не сдвинутого)
// порядок = порядок приближенного решения - 1
// В остальных случаях порядок выбирается исходя из погрешности для функции приращения не сдвинутого решения
	if(*(owner->NE) == 1)
	{
		xI = owner->xI;
		ii = 0;
		switch(TypeOfMaxR){		// Выбор I происходит разными способами, в зависимости от того, каким способом был выбран шаг
		case 0:
		case 1:
			e = exp(numeric(*owner->step));
			while(ii < xI)
			{
				ii++;
				fact *= (*(owner->step))/numeric(ii);
				sum += fact;
			}
			LocalD = c*abs(e - sum);
			break;
		case 2:
			fact = factorial(numeric(im));
			e = (*(owner->step));
			while(ii < xI)
			{
				ii++;
				e *= (*(owner->step));
			}
			LocalD = (c/fact)*(e/(numeric(1)-(*(owner->step))));
			break;
		case 3:
			e = exp(numeric(1));
			for(k = 0; k < im; k++)
				hk *= (*(owner->step));
			c = abs(hk*R(im));
			while(ii < xI)
			{
				ii++;
				fact /= ii;
				sum += fact;
			}
			LocalD = c*abs(e - sum);
			break;
		}
	}
	else
	{
		SupLocError = owner->phi[1].LocalD;
		LocalD = SupLocError + numeric(1);
		switch(TypeOfMaxR){		// Выбор I происходит разными способами, в зависимости от того, каким способом был выбран шаг
		case 0:
		case 1:
			e = exp(numeric((*(owner->step))));
			xI = 0;
			while(LocalD > SupLocError)
			{
				xI++;
				fact *= (*(owner->step))/numeric(xI);
				sum += fact;
				LocalD = c*abs(e - sum);
			}
			break;
		case 2:
			xI = 0;
			fact = factorial(numeric(im));
			e = (*(owner->step));
			while(LocalD > SupLocError)
			{
				xI++;
				e *= (*(owner->step));
				LocalD = (c/fact)*(e/(numeric(1)-(*(owner->step))));
			}
			break;
		case 3:
			e = exp(numeric(1));
			for(k = 0; k < im; k++)
				hk *= (*(owner->step));
			c = abs(hk*R(im));
			xI = 0;
			while(LocalD > SupLocError)
			{
				xI++;
				fact = numeric(1)/factorial(numeric(xI));
				sum += fact;
				LocalD = c*abs(e - sum);
			}
			break;
		}
	}
	*(owner->NE) = 1;
}
