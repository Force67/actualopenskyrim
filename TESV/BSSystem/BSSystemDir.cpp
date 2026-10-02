#include "BSSystem/BSSystemDir.h"

BSSystemDir::BSSystemDir()
{
	eLastError = BSSystemFile::EC_NONE;
	uiEntryPos = 0;
	// The default constructor opens no directory; DoOpen gets a null path and
	// leaves hFind untouched.
	eLastError = DoOpen(nullptr);
}
