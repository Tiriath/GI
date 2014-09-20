/// \file resource_traits.h
/// \brief Template header for graphical resource's traits
///
/// \author Raffaele D. Facendola

#pragma once

#include <Eigen/Core>
#include <memory>

#include "resources.h"

using ::Eigen::Vector3f;
using ::Eigen::Vector2f;
using ::std::shared_ptr;

namespace gi_lib{

	/// \brief Resources' load setting's template.
	template <typename TResource, typename TResource::LoadMode kLoadMode> struct LoadSettings;

	/// \brief Settings used to load a Texture2D from a DDS file.
	template <> struct LoadSettings < Texture2D, Texture2D::LoadMode::kFromDDS > {

		/// \brief Name of the file to load relative to the resource folder.
		wchar_t * file_name;		

	};

	/// \brief Settings used to load a Shader from a text file.
	template <> struct LoadSettings < Shader, Shader::LoadMode::kCompileFromFile > {

		/// \brief Name of the file to load relative to the resource folder.
		wchar_t * file_name;						
		
	};

	/// \brief Resources' build setting's template.
	template <typename TResource, typename TResource::BuildMode kBuildMode> struct BuildSettings;

	/// \brief Build settings for normal textured meshes..
	template <> struct BuildSettings<Mesh, Mesh::BuildMode::kNormalTextured>{

		/// \brief Indices' data.
		vector<unsigned int> indices;

		/// \brief Vertices' data.
		vector<VertexFormatNormalTextured> vertices;

	};
	
	template <> struct BuildSettings < Material, Material::BuildMode::kFromShader > {

		/// \brief The material's shader.
		shared_ptr<Shader> shader;

	};

}