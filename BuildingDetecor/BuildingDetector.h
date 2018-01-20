#include "stdafx.h"

extern "C"
{
	_declspec(dllexport) int add(int a, int b);
	typedef int(*ApiAdd)(int, int);
}