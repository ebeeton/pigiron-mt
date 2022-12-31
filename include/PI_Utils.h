// Common utility functions and miscellany.
//
// Copyright Evan Beeton 2/1/2005

#pragma once

#define ERRORBOX(message) MessageBox(0, message, "Error", MB_ICONERROR | MB_OK)
#define KEYDOWN(key) (GetAsyncKeyState(key) & 0x8000)

#include <vector>
using std::vector;

int RandomNum(int high, int low);

// A vector that keeps track of how many of its elements are "in use" in a linear fashion.
// This allows you to "empty" the vector without actually deleting memory.
template <typename T>
class reusable_vector : public vector<T>
{
	unsigned int numInUse;
public:
	reusable_vector<T>(void) : numInUse(0) { }

	void push_back(const T &v);

	void mark_all_unused(void) { numInUse = 0; }

	unsigned int size(void) { return numInUse; }
};

template <typename T>
void reusable_vector<T>::push_back(const T &v)
{
	// Look for an "unused" spot in the onscreen vector.
	if (++numInUse < vector<T>::size())
		(*this)[numInUse - 1] = v;
	else
		// Make room!
		vector<T>::push_back(v);
}