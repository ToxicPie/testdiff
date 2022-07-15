#ifndef _DIFF_HPP
#define _DIFF_HPP

#include "cfg.hpp"
#include <vector>


double similarity(long long x, long long y, double thres);

double similarity(const BasicBlock &a, const BasicBlock &b);

double similarity(const Function &a, const Function &b);

void diff(const BinaryFile &a, const BinaryFile &b);


#endif // _DIFF_HPP
