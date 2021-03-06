/// \file dx11mesh.h
/// \brief ???
///
/// \author Raffaele D. Facendola

#ifdef _WIN32

#pragma once

#include "mesh.h"
#include "gimath.h"
#include "instance_builder.h"

#include "dx11/dx11.h"

#include "windows/win_os.h"

namespace gi_lib{

	namespace dx11{

		using windows::COMPtr;

		/// \brief DirectX11 static mesh.
		/// \author Raffaele D. Facendola.
		class DX11Mesh : public IStaticMesh{

		public:

			/// \brief Create a new DirectX11 mesh.
			/// \param device The device used to load the graphical resources.
			/// \param bundle Bundle used to create the mesh.
			DX11Mesh(const FromVertices<VertexFormatNormalTextured>& args);

			/// \brief Create a new DirectX11 mesh.
			/// \param device The device used to load the graphical resources.
			/// \param bundle Bundle used to create the mesh.
			DX11Mesh(const FromVertices<VertexFormatPosition>& args);

			virtual size_t GetSize() const override;

			virtual size_t GetVertexCount() const override;

			virtual size_t GetPolygonCount() const override;

			virtual size_t GetLODCount() const override;

			virtual const AABB& GetBoundingBox() const override;

			virtual size_t GetSubsetCount() const override;

			virtual const MeshSubset& GetSubset(unsigned int subset_index) const override;

			/// \brief Bind the mesh to the given context.
			void Bind(ID3D11DeviceContext& context, bool tessellable = false);

			/// \brief Draws the specified subset.
			void DrawSubset(ID3D11DeviceContext& context, unsigned int subset_index, unsigned int instances = 1) const;

			virtual MeshFlags GetFlags(unsigned int subset_index) const override;

			virtual void SetFlags(unsigned int subset_index, MeshFlags flags) override;

			virtual MeshFlags GetFlags() const override;

			virtual void SetFlags(MeshFlags flags) override;

			virtual void SetName(const std::wstring& name) override;

			virtual const std::wstring& GetName() const override;

			virtual void SetSubsetName(size_t subset_index, const std::wstring& name) override;

			virtual const std::wstring& GetSubsetName(size_t subset_index) const override;

		private:

			COMPtr<ID3D11Buffer> vertex_buffer_;

			COMPtr<ID3D11Buffer> index_buffer_;

			vector<MeshSubset> subsets_;

			vector<MeshFlags> flags_;										///< \brief Flags for each subset.
			size_t vertex_count_;

			size_t polygon_count_;

			size_t LOD_count_;

			size_t size_;

			size_t vertex_stride_;											///< \brief Size of each vertex in bytes

			AABB bounding_box_;
			
			vector<std::wstring> subset_names_;

			std::wstring name_;												///< \brief Mesh name

		};

		///////////////////////////// MESH ////////////////////////////////////////////////

		INSTANTIABLE(IStaticMesh, DX11Mesh, IStaticMesh::FromVertices<VertexFormatNormalTextured>);

		inline size_t DX11Mesh::GetVertexCount() const{

			return vertex_count_;

		}

		inline size_t DX11Mesh::GetPolygonCount() const{

			return polygon_count_;

		}

		inline size_t DX11Mesh::GetLODCount() const{

			return LOD_count_;

		}

		inline size_t DX11Mesh::GetSize() const{

			return size_;

		}

		inline const AABB& DX11Mesh::GetBoundingBox() const{

			return bounding_box_;

		}

		inline size_t DX11Mesh::GetSubsetCount() const{

			return subsets_.size();

		}

		inline const MeshSubset& DX11Mesh::GetSubset(unsigned int subset_index) const{

			return subsets_[subset_index];

		}

		inline void DX11Mesh::SetName(const std::wstring& name) {

			name_ = name;
			
		}

		inline const std::wstring& DX11Mesh::GetName() const {

			return name_;

		}

		inline void DX11Mesh::SetSubsetName(size_t subset_index, const std::wstring& name) {

			subset_names_[subset_index] = name;

		}

		inline const std::wstring& DX11Mesh::GetSubsetName(size_t subset_index) const {

			auto& subset_name = subset_names_[subset_index];

			return subset_name;

		}

	}
	
}



#endif