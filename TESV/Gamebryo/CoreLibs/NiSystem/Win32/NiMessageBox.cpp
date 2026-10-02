#include "Gamebryo/CoreLibs/NiSystem/NiMessageBox.h"

#include <windows.h>

unsigned int NiMessageBox::DefaultMessageBox(const char* pcText, const char* pcCaption, void*)
{
	MessageBoxA(nullptr, pcText, pcCaption, 0x40000);
	return 0;
}
