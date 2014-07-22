/** @file FSFlibs.h
 */
#ifndef FSFLIBS_H
#define FSFLIBS_H

// GiNaC implementation
#include "cln/config.h"
#include <ginac/ginac.h>
#include <sstream>
#include "trans.h"
using namespace std;
using namespace GiNaC;

// tinfos
const unsigned TINFO_expr_table			= 0x42420001U;
const unsigned TINFO_sub_val			= 0x42420002U;
const unsigned TINFO_sym_rk				= 0x42420003U;
const unsigned TINFO_sym_IC				= 0x42420004U;
const unsigned TINFO_ANM_system			= 0x42420005U;
const unsigned TINFO_myprocedure		= 0x42420006U;
const unsigned TINFO_defining			= 0x42420007U;
const unsigned TINFO_row_res			= 0x42420008U;
const unsigned TINFO_row_pexp			= 0x42420009U;
const unsigned TINFO_row_all			= 0x42420010U;
const unsigned TINFO_row_mul			= 0x42420011U;
const unsigned TINFO_row_power			= 0x42420012U;
const unsigned TINFO_row_power_z		= 0x42420013U;
const unsigned TINFO_unknown_sol		= 0x42420014U;
const unsigned TINFO_taylor_row			= 0x42420015U;
const unsigned TINFO_sym_tk				= 0x42420016U;
const unsigned TINFO_spline_coeff		= 0x42420017U;
const unsigned TINFO_logic_op			= 0x42420019U;
const unsigned TINFO_cur_tk				= 0x42420020U;
const unsigned TINFO_ANM_sens_system	= 0x42420021U;
const unsigned TINFO_taylor_spline_element = 0x42420022U;
const unsigned TINFO_ANM_sensitivity	= 0x42420023U;
const unsigned TINFO_Plot	= 0x42420024U;

template<class T>
std::string ToString(const T & t)
{
	std::ostringstream buf;
	buf << (t);
	return buf.str();
}

#define ERROR_JUST(e) throw e;

#define ERROR_1(name, ind, Ri) throw runtime_error("determinant < 0 for " + ToString(name) + ".R[" + ToString(ind) + "]: R[" + ToString(ind) + "] = " + ToString(Ri));
#define ERROR_2(name, i, tmpRk, det) throw runtime_error("Cannot evulate " + ToString(name) + ".R[" + ToString(i) + "]:\n\tR[" + ToString(i) + "] = " + ToString(tmpRk) + "\ndet = " + ToString(det));
#define ERROR_3(name, i, Ri) throw runtime_error("recursive definition for " + ToString(name) + ".R[" + ToString(i) + "]: R[" + ToString(i) + "] = " + ToString(Ri));
#define ERROR_4(name, i, tmpRk) throw runtime_error("Cannot evulate " + ToString(name) + ".R[" + ToString(i) + "]:\n\tR[" + ToString(i) + "] = " + ToString(tmpRk));
#define ERROR_5(in_ex) throw runtime_error("unrecognized initial condition: " + ToString(in_ex));
#define ERROR_6(name, ICs) throw runtime_error("can not evulate initial condition for " + ToString(name) + ": " + ToString(ICs));
#define ERROR_7(name, i) throw runtime_error("not enough initial congitions for " + ToString(name) + ", need " + ToString(i));
#define ERROR_8(name) throw runtime_error("can not find max(R[i]) for " + ToString(name));
#define ERROR_9 throw std::runtime_error("wrong format of command \'analysis\'");
#define ERROR_10(ICs) throw std::runtime_error("undefined id" + ToString(ICs));
#define ERROR_11 throw std::runtime_error("too few equations");
#define ERROR_12 throw std::runtime_error("too many equations");
#define ERROR_13 throw runtime_error("different names of \'others\' values");
#define ERROR_14 throw runtime_error("different numbers of \'others\' values");
#define ERROR_15(op1, op0) throw runtime_error("undefined exponent \'" + ToString(op1) + "\' for series: "  + ToString(op0));

#define ERROR_16 throw runtime_error("unrecognized statement");
#define ERROR_17(signif, tmpOp) throw runtime_error("unexpected \'=\' after " + signif + " " + ToString(tmpOp));
#define ERROR_18 throw runtime_error("registered var \'Digits\' cannot assign nonnumeric");
#define ERROR_19(signif) throw runtime_error("\')\' expected, but " + signif + " found");
#define ERROR_20(signif) throw runtime_error("unrecognized or unexpected token: " + signif);
#define ERROR_21(signif) throw runtime_error("\'(\' expected, but " + signif + " found");
#define ERROR_22(fname) "Error: can't open output file - " + fname;
#define ERROR_23(fname) "Error: can't open input file - " + fname;
#define ERROR_24(signif) throw runtime_error("\'then\' expected, but " + signif + " found");
#define ERROR_25(signif) throw runtime_error("\'endif\' expected, but " + signif + " found");
#define ERROR_26(tmpOp) throw runtime_error("cannot determine if this expression is true or false: " + ToString(tmpOp));
#define ERROR_27(tmpOp) throw runtime_error("recursive assignment for " + ToString(tmpOp));

// s_taylor = symbol("__taylor") - используется при вызове функций coeff() для возврата коэффициентов рядов Тейлора
extern symbol s_taylor;
// s_p = symbol("__p") - используется при вызове функций coeff() для возврата коэффициентов рядов Лорана
extern symbol s_p;
// s_t = symbol("t") - время - независимая переменная дифференцирования
extern symbol s_t;
// s_bMinus = symbol("__bMinus") - используется при расчете в обратном направлении (при нечетных степенях p в матрице A(p) надо ставить "-" для расчета в обратном направлении
extern symbol s_bMinus;
// s_diff = symbol("diff") - функция такая - производная
extern symbol s_diff;

#define DEBUG_OUT(OutStr) \
{\
	cout << OutStr << endl << flush;\
}
// 	cout << OutStr << "\x0D\x0A" << flush;\
// << "File: " << __FILE__ << "\x0D\x0A"
//	SendMessage(hLogEdit, WM_COPYDATA, 1, LPARAM(BigLogString.str().c_str()));
#endif // ndef FSFLIBS_H
