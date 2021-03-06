#ifndef ROWS_H
#define ROWS_H


#include "..\\FSFlibs.h"
/*
����� ��� �������������� ��������� � ���� �������
	pexp(expr, pexp) - ��� ������������ ���� expr �� ���������� ���������� � ������� pexp -> sum(R[i]/p^(i+1), i=0..inf)*p^k = sum(R[i]/p^(i+1-k),i=0..inf): coeff(i) = R[i+k]
*/

//--------------- row_pexp class -----------------------
// ����� ������������ ���� �� ���������� ���������� __p

class row_pexp : public basic
{
	GINAC_DECLARE_REGISTERED_CLASS(row_pexp, basic)
public:
	row_pexp(const ex &aExpr, const int &aPexp = 0);

public:
	bool has(const ex &other)
	{
		return expression.has(other);
	}
	int degree(const ex &s) const;
	ex coeff(const ex & s, int n = 1) const;
	void print(const print_context &c, unsigned level = 0) const;
	ex subs(const exmap & m, unsigned options = 0)const;
	size_t nops() const{
		return 2;
	}
	ex op(size_t i) const{
		if(i)
			return pexp;
		else
			return expression;
	}
	ex& let_op(size_t i){
		GINAC_ASSERT(i<nops());
		ensure_if_modifiable();
		return expression;
	}

private:
	ex expression;
	int pexp;
};

//--------------- taylor_row class -----------------------
// ������ ��� ������� ��� ������� H. ������������ ��� ����, ����� �� ����������������� row_all, ... ��� ��������� ���������
class ANM_solutions;
class taylor_row : public basic
{
	GINAC_DECLARE_REGISTERED_CLASS(taylor_row, basic)
public:
	taylor_row(const ex &aExpr, ANM_solutions *sols);

public:
	int degree(const ex &s) const;
	ex coeff(const ex & s, int n = 1) const;
	void print(const print_context &c, unsigned level = 0) const;
	ex op(size_t i) const {return expression;};

protected:
	ex expression;
};

//--------------- row_all class -----------------------
// ����� ����������� �������������� ������������ ������� � ��� ������� (����������� ������ n-� �����������)

class row_all : public basic
{
	GINAC_DECLARE_REGISTERED_CLASS(row_all, basic)
public:
	row_all(const ex &aExpr);

public:
	bool has(const ex &other)
	{
		return expression.has(other);
	}
	int degree(const ex &s) const;
	ex coeff(const ex & s, int n = 1) const;
	void print(const print_context &c, unsigned level = 0) const;
	ex eval(int level = 0){return hold();}
	ex subs(const exmap & m, unsigned int options = 0);

	vector<ex> Tk;	// ������ ������������� ����-����������
private:
	ex expression;
	ex cur_diff;
};

//--------------- row_all mul class -----------------------
// ����� ������������ �����. ����������� ������������ ����� � ���

class row_mul : public mul
{
	GINAC_DECLARE_REGISTERED_CLASS(row_mul, mul)

	// other ctors
public:
	row_mul(const ex & lh, const ex & rh) : mul(lh,rh){}
	row_mul(const mul & lh) : mul(lh){}
	row_mul(const exvector & v) : mul(v){}
	row_mul(const epvector & v) : mul(v){}
	row_mul(const epvector & v, const ex & oc) : mul(v,oc){}
	row_mul(std::auto_ptr<epvector> vp, const ex & oc) : mul(vp,oc){}
	row_mul(const ex & lh, const ex & mh, const ex & rh) : mul(lh,mh,rh){}

public:
	void print(const print_context &c, unsigned level = 0) const;
	ex coeff(const ex & s, int n = 1) const;
	int degree(const ex &s) const;
	ex eval(int level = 0)const{return hold();}
	ex subs(const exmap & m, unsigned int options = 0);
private:
	ex GetSum(int i, int nj) const;
	vector<ex> Tk;	// ������ ������������� ����-����������
};

//--------------- row_power class -----------------------
// ����� ���������� ����� � ����� �������.

class row_power : public power
{
	GINAC_DECLARE_REGISTERED_CLASS(row_power, power)
	// other ctors
public:
	template<typename T> row_power(const ex & lh, const ex & rh) : power(lh,rh) {}
	template<typename T> row_power(const ex & lh, const T & rh) : power(lh,rh) {}

public:
	ex coeff(const ex & s, int n = 1) const;
	void print(const print_context &c, unsigned level = 0) const;
	int degree(const ex &s) const;
	ex eval(int level = 0)const {return hold();}
	ex subs(const exmap & m, unsigned int options = 0);

private:
	ex GetSum(int i, int nj) const;
	vector<ex> Tk;	// ������ ������������� ����-����������
};

//--------------- row_power_z class -----------------------
// ����� ���������� ����� � ������������� �������. ����������� ���������� ����� � ������������� ������� � ���

class row_power_z : public power
{
	GINAC_DECLARE_REGISTERED_CLASS(row_power_z, power)
	// other ctors
public:
	template<typename T> row_power_z(const ex & lh, const ex & rh) : power(lh, rh) {}
	template<typename T> row_power_z(const ex & lh, const T & rh) : power(lh, rh) {}

public:
	ex coeff(const ex & s, int n = 1) const;
	void print(const print_context &c, unsigned level = 0) const;
	int degree(const ex &s) const;
	ex evalf(int ind);
//	ex eval(int level = 0)const {return hold();}
//	ex subs(const exmap & m, unsigned int options = 0);

private:
	ex GetSum(int i);
	matrix C;		// ������� �� ������� �����
	matrix invC;	// ��������������� ������� C
	vector<ex> Tk;	// ������ ������������� ����-����������
	vector<ex> numTk;
};

//------------------------ ������� �������������� ������ ��������� � ��� (��� ������� �������������) ---------------
//inline const ex ex_to_row_all(const ex& expr);
#endif

