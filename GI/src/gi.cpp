#include <Windows.h>

#include <string>
#include <sstream>

#include <core.h>
#include <exceptions.h>

#include "..\include\gi_logic.h"

#undef GetMessage	// Nice naming there.... 

using ::std::wstring;
using ::std::wstringstream;

using ::gi_lib::Application;
using ::gi_lib::Exception;

void CopyToClipboard(const wstring &);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd){

#define CATCH_EXCEPTIONS

#ifdef CATCH_EXCEPTIONS
	try{
#endif

		auto & app = Application::GetInstance();
		
		app.AddWindow<gi::GILogic>();

		app.Join();

#ifdef CATCH_EXCEPTIONS

	}catch (const Exception & e){

		//Unhandled exception!
		wstringstream stream;
		
		stream << e.GetError() << std::endl
			   << e.GetLocation() << std::endl
			   << e.GetStackTrace() << std::endl
			   << "Stack trace has been copied to the clipboard";
		
		auto message = stream.str();

		CopyToClipboard(message);

		MessageBox(NULL,
				   message.c_str(),
				   L"Unhandled exception",
				   MB_OK | MB_ICONERROR);

	}

#endif

}

void CopyToClipboard(const wstring & text){

	auto output = text.c_str();
	
	auto len = (text.length() + 1) * sizeof(wchar_t);

	//Allocate the text globally
	auto hMem = GlobalAlloc(GMEM_MOVEABLE, len);
	memcpy(GlobalLock(hMem), output, len);
	GlobalUnlock(hMem);

	//Send the data to the clipboard
	OpenClipboard(NULL);
	EmptyClipboard();
	SetClipboardData(CF_UNICODETEXT, hMem);
	CloseClipboard();

}