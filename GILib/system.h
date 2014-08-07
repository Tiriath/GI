/// \file system.h
/// \brief Defines classes and structures used to query the system about its configuration.
///
/// \author Raffaele D. Facendola

#pragma once

#include <string>
#include <vector>

using ::std::wstring;
using ::std::vector;

namespace gi_lib{

	/// \brief Describes the CPU.
	struct CpuProfile{

		unsigned int cores;			/// Number of logical cores.
		unsigned __int64 frequency; /// Frequency of each core in Hz.

	};

	/// \brief Describes a particular drive.
	struct DriveProfile{

		unsigned long long size;			/// Total space, in bytes.
		unsigned long long available_space; /// Available space, in bytes.
		wstring unit_letter;				/// Unit letter.

	};

	/// \brief Describes the storage capabilities.
	struct StorageProfile{

		vector<DriveProfile> fixed_drives;	/// Vector of all fixed drives' profiles.

	};

	/// \brief Describes the system memory.
	struct MemoryProfile{

		unsigned long long total_physical_memory;		/// Total physical memory, in bytes.
		unsigned long long total_virtual_memory;		/// Total virtual address space for the current process, in bytes.
		unsigned long long total_page_memory;			/// Total page memory, in bytes.
		unsigned long long available_physical_memory;	/// Available physical memory, in bytes.
		unsigned long long available_virtual_memory;	/// Available virtual address space for the current process, in bytes.
		unsigned long long available_page_memory;		/// Available page memory, in bytes.

	};


	/// \brief Describes the desktop.
	struct DesktopProfile{

		unsigned int width;		/// Horizontal resolution of the dekstop.
		unsigned int height;	/// Vertical resolution of the desktop.

	};

	/// \brief Operating system.
	enum class OperatingSystem{

		WINDOWS		/// Windows OS

	};

	/// \brief Used to query the system about its capabilities.

	/// \remarks Static class.
	class System{

	public:

		System() = delete;

		/// \brief Get the current operating system

		/// \return Returns the current operating system
		static OperatingSystem GetOperatingSystem();

		/// \brief Get the full application path.

		/// \return Returns the full application path.
		static wstring GetApplicationPath();

		/// \brief Get the application name.

		/// \param extension Set this parameter to "true" to include the extension, "false" otherwise.
		/// \return Returns the application name, excluding the extension if not specified otherwise.
		static wstring GetApplicationName(bool extension = true);

		/// \brief Get the CPU capabilities.

		/// \return Returns the CPU capabilities.
		static CpuProfile GetCPUProfile();

		/// \brief Get the memory capabilities.

		/// \return Returns the memory capabilities.
		static MemoryProfile GetMemoryProfile();

		/// \brief Get informations about storage media

		/// \return Returns informations about storage media.
		static StorageProfile GetStorageProfile();

		/// \brief Get informations about user's desktop.

		/// \return Returns informations about user's desktop.
		static DesktopProfile GetDesktopProfile();

	};

}