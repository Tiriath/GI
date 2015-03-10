/// \file os_windows.h
/// \brief Windows-specific interfaces.
///
/// \author Raffaele D. Facendola

#pragma once

#ifdef _WIN32

#include <Windows.h>
#include <memory>
#include <string>

#include "..\macros.h"

using std::unique_ptr;

/// \brief If the provided expression fails the caller returns the expression value, otherwise nothing happens.
/// The expression fails if FAILED(.) is true.
#define RETURN_ON_FAIL(expr) do{ \
								HRESULT __hr = expr; \
								if (FAILED(__hr)) return __hr; \
							 }WHILE0

/// \brief If the provided expression fails the caller throws an exception with the error code, otherwise nothing happens.
/// The expression fails if FAILED(.) is true.
#define THROW_ON_FAIL(expr) do{ \
								HRESULT __hr = expr; \
								if(FAILED(__hr)) THROW(std::to_wstring(__hr)); \
							}WHILE0

/// \brief If the provided expression if false the caller throws an exception whose error code is equal to GetLastError() current value.
#define THROW_ON_FALSE(expr) do{ \
								auto __expr = (expr); \
								if(!__expr) THROW(std::to_wstring(GetLastError())); \
							 }WHILE0

/// \brief Defines a raii guard for COM interfaces.
#define COM_GUARD(com) unique_ptr<IUnknown, COMDeleter> ANONYMOUS(com, COMDeleter{})

namespace gi_lib{

	namespace windows{

		/// \brief Functor used to delete COM interfaces.
		struct COMDeleter{

			/// \brief Release the given COM interface.
			/// \param ptr Pointer to the COM resource to delete.
			void operator()(IUnknown * com);
			
		};

		// COMDeleter

		inline void COMDeleter::operator()(IUnknown * com){

			com->Release();
			
		}

	}
	
}

#endif