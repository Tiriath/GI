/// \file resource_traits.h
/// \brief Template header for graphical resource's traits
///
/// \author Raffaele D. Facendola

#pragma once

#include "resources.h"

namespace gi_lib{

	/// \brief Resources' load setting's template.
	template <typename TResource, typename TResource::LoadMode kLoadMode> struct LoadSettings;

	/// \brief Resources' build setting's template.
	template <typename TResource, typename TResource::BuildMode kBuildMode> struct BuildSettings;

	/// \brief Settings used to load a Texture2D from a DDS file.
	template <> struct LoadSettings < Texture2D, Texture2D::LoadMode::kFromDDS > {

		wchar_t * file_name;		///< \brief Name of the file to load relative to the resource folder.

	};

	/// \brief Build settings for position-only meshes.
	template <> struct BuildSettings<Mesh, Mesh::BuildMode::kPosition>{

		/// \brief Indices' data.
		vector<unsigned int> indices;

		/// \brief Vertices' data.
		vector<VertexFormatPosition> vertices;

	};

	/// \brief Build settings for textured meshes.
	template <> struct BuildSettings<Mesh, Mesh::BuildMode::kTextured>{

		/// \brief Indices' data.
		vector<unsigned int> indices;

		/// \brief Vertices' data.
		vector<VertexFormatTextured> vertices;

	};

	/// \brief Build settings for normal textured meshes..
	template <> struct BuildSettings<Mesh, Mesh::BuildMode::kNormalTextured>{

		/// \brief Indices' data.
		vector<unsigned int> indices;

		/// \brief Vertices' data.
		vector<VertexFormatNormalTextured> vertices;

	};
	
}