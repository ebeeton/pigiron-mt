// Common utility functions and miscellany.
//
// Copyright Evan Beeton 2/1/2005

#include <cstdlib>

#include "PI_Utils.h"

int RandomNum(int high, int low)
{
	return rand() % (high - low + 1) + low;
}