/// \file fbx.h
/// \brief Classes and methods to manage Autodesk FBX files.
///
/// \author Raffaele D. Facendola

#pragma once

#include <string>
#include <vector>

#include <fbxsdk.h>

using ::std::string;

namespace gi_lib{

	/// \brief Manager of FBX files.
	/// \author Raffaele D. Facendola
	class FBX{

	public:

		/// \brief Get the singleton instance.
		/// \return Returns the singleton instance.
		static FBX & GetInstance();

		/// \brief Default destructor.
		~FBX();

		/// \brief Import a fbx scene file.
		/// \param path The path of the file to parse.
		/// \return Returns a pointer to the parsed scene.
		FbxScene * Import(const string & path);

		/// \brief Triangulate a scene in-place.
		/// \param scene The scene to be triangulated.
		void Triangulate(FbxScene & scene);

		/// \brief Rolls the attributes of the scene.
		/// \param scene The scene to remap the attributes of.
		void RemapAttributes(FbxScene & scene);

		/// \brief Normalizes the path of all the textures adressed by the scene.
		/// \param scene The scene to process.
		/// \param base_path The base path of the file.
		/// \param extension The extension that will replace the old one.
		void NormalizeTexturePaths(FbxScene & scene, const string& base_path, const string& extension);

		/// \brief Export a fbx scene into a file.
		/// \param scene The scene to export.
		/// \param path The output path.
		/// \param binary Choose amoung binary format (true) or ASCII format (false).
		void Export(FbxScene & scene, const string & path, bool binary = true);

	private:

		FBX();

		FbxManager * manager_;

		FbxIOSettings * settings_;

		FbxGeometryConverter * converter_;

	};

	//

	inline FBX & FBX::GetInstance(){

		static FBX instance;

		return instance;

	}

}