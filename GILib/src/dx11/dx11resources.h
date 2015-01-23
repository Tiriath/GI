/// \file dx11resources.h
/// \brief Classes and methods for DirectX11 texture management.
///
/// \author Raffaele D. Facendola

#pragma once

#include <d3d11.h>
#include <d3dx11effect.h>
#include <string>
#include <map>
#include <memory>
#include <numeric>

#include "..\..\include\graphics.h"
#include "..\..\include\resources.h"
#include "..\..\include\resource_traits.h"
#include "dx11shared.h"

using ::std::wstring;
using ::std::unique_ptr;
using ::std::shared_ptr;
using ::std::map;

namespace gi_lib{

	namespace dx11{

		class DX11Texture2D;
		class DX11Mesh;

		/// \brief DirectX11 plain texture.
		/// \author Raffaele D. Facendola.
		class DX11Texture2D : public Texture2D{

		public:
			
			/// \brief Create a new texture from DDS file.
			/// \param device The device used to create the texture.
			/// \param settings The load settings
			DX11Texture2D(ID3D11Device & device, const LoadSettings<Texture2D, Texture2D::LoadMode::kFromDDS> & settings);

			/// \brief Create a mew texture from an existing DirectX11 texture.
			/// \param texture The DirectX11 texture.
			/// \param format The format used when sampling from the texture.
			DX11Texture2D(ID3D11Texture2D & texture, DXGI_FORMAT format);

			virtual ~DX11Texture2D(){}

			virtual size_t GetSize() const override;

			virtual ResourcePriority GetPriority() const override;

			virtual void SetPriority(ResourcePriority priority) override;

			virtual unsigned int GetWidth() const override;

			virtual unsigned int GetHeight() const override;

			virtual unsigned int GetMipMapCount() const override;

			virtual WrapMode GetWrapMode() const override;

			virtual void SetWrapMode(WrapMode wrap_mode) override;

			/// \brief Get the view used to bind this texture to a shader.
			/// \return Returns a reference to the shader resource view.
			ID3D11ShaderResourceView & GetShaderResourceView();

			/// \brief Get the view used to bind this texture to a shader.
			/// \return Returns a reference to the shader resource view.
			const ID3D11ShaderResourceView & GetShaderResourceView() const;

		private:

			void UpdateDescription();

			unique_ptr<ID3D11Texture2D, COMDeleter> texture_;

			unique_ptr<ID3D11ShaderResourceView, COMDeleter> shader_view_;

			unsigned int width_;

			unsigned int height_;

			unsigned int bits_per_pixel_;

			unsigned int mip_levels_;
			
			WrapMode wrap_mode_;

		};

		/// \brief DirectX11 render target.
		/// \author Raffaele D. Facendola
		class DX11RenderTarget : public RenderTarget{

		public:

			/// \brief Create a new render target from an existing buffer.

			/// \param buffer Buffer reference.
			/// \param device Device used to create the additional internal resources.
			DX11RenderTarget(ID3D11Texture2D & target);

			virtual ~DX11RenderTarget(){}

			virtual size_t GetSize() const override;

			virtual ResourcePriority GetPriority() const override;

			virtual void SetPriority(ResourcePriority priority) override;

			virtual unsigned int GetCount() const override;

			virtual shared_ptr<Texture2D> GetTexture(int index) override;

			virtual shared_ptr<const Texture2D> GetTexture(int index) const override;

			virtual shared_ptr<Texture2D> GetZStencil() override;

			virtual shared_ptr<const Texture2D> GetZStencil() const override;

			virtual float GetAspectRatio() const override;

			/// \brief Set new buffers for the render target.

			/// \param buffers The list of buffers to bound
			void SetBuffers(std::initializer_list<ID3D11Texture2D*> targets);

			/// \brief Releases al the buffers referenced by the render target.
			void ResetBuffers();

			/// \brief Bind the render target to the specified context.
			/// \param context The context to bound the render target to.
			void Bind(ID3D11DeviceContext & context);

			/// \brief Clear the depth stencil view.
			/// \param context The context used to clear the view.
			/// \param clear_flags Determines whether to clear the depth and\or the stencil buffer. (see: D3D11_CLEAR_FLAGS)
			/// \param depth Depth value to store inside the depth buffer.
			/// \param stencil Stencil valuet o store inside the stencil buffer.
			void ClearDepthStencil(ID3D11DeviceContext & context, unsigned int clear_flags, float depth, unsigned char stencil);

			/// \brief Clear every target view.
			/// \param context The context used to clear the view.
			/// \param color The color used to clear the targets.
			void ClearTargets(ID3D11DeviceContext & context, Color color);

		private:
			
			vector<unique_ptr<ID3D11RenderTargetView, COMDeleter>> target_views_;

			unique_ptr < ID3D11DepthStencilView, COMDeleter > zstencil_view_;

			vector<shared_ptr<DX11Texture2D>> textures_;

			shared_ptr<DX11Texture2D> zstencil_;

		};

		/// \brief DirectX11 static mesh.
		/// \author Raffaele D. Facendola.
		class DX11Mesh: public Mesh{

		public:

			/// \brief Create a new DirectX11 mesh.
			/// \param device The device used to load the graphical resources.
			/// \param settings Settings used to build the mesh.
			DX11Mesh(ID3D11Device & device, const LoadSettings<Mesh, Mesh::LoadMode::kNormalTextured> & settings);

			virtual size_t GetSize() const override;

			virtual ResourcePriority GetPriority() const override;

			virtual void SetPriority(ResourcePriority priority) override;

			virtual size_t GetVertexCount() const override;

			virtual size_t GetPolygonCount() const override;

			virtual size_t GetLODCount() const override;

			virtual Bounds GetBounds() const override;

		private:

			unique_ptr<ID3D11Buffer, COMDeleter> vertex_buffer_;

			unique_ptr<ID3D11Buffer, COMDeleter> index_buffer_;

			size_t vertex_count_;

			size_t polygon_count_;

			size_t LOD_count_;

			size_t size_;

			Bounds bounds_;
			
		};

		/// \brief DirectX11 material.
		/// \author Raffaele D. Facendola
		class DX11Material : public Material{

			friend class DX11MaterialParameter;

		public:

			/// \brief Create a new DirectX11 material instance.
			/// \param device The device used to load the graphical resources.
			/// \param settings The settings used to build the material.
			DX11Material(ID3D11Device & device, const LoadSettings<Material, Material::LoadMode::kFromShader> & settings);

			virtual size_t GetSize() const override;

			virtual ResourcePriority GetPriority() const override;

			virtual void SetPriority(ResourcePriority priority) override;
		
			virtual unsigned int GetParameterIndex(const wstring& name) const override;

			virtual unsigned int GetTextureIndex(const wstring& name) const override;

			virtual bool SetTexture(const wstring &name, shared_ptr<Texture2D> texture) override;

			virtual bool SetTexture(unsigned int index, shared_ptr<Texture2D> texture) override;

		protected:

			virtual bool SetParameter(const wstring & name, const void* buffer, size_t size) override;

			virtual bool SetParameter(unsigned int index, const void* buffer, size_t size) override;

		private:
			
			ResourcePriority priority_;

			size_t size_;

		};

		/// \brief DirectX11 resource mapping template.
		template<typename TResource> struct ResourceMapping;

		/// \brief Texture 2D mapping
		template<> struct ResourceMapping < Texture2D > {

			/// \brief Concrete type associated to a Texture2D.
			using TMapped = DX11Texture2D;

		};

		/// \brief Mesh mapping.
		template<> struct ResourceMapping < Mesh > {

			/// \brief Concrete type associated to a Mesh.
			using TMapped = DX11Mesh;

		};

		/// \brief Material mapping.
		template<> struct ResourceMapping < Material > {

			/// \brief Concrete type associated to a Material.
			using TMapped = DX11Material;

		};

		/// \brief Render target mapping.
		template<> struct ResourceMapping < RenderTarget > {

			/// \brief Concrete type associated to a Render Target
			using TMapped = DX11RenderTarget;

		};

		/// \brief Performs a resource cast from an abstract type to a concrete type.
		/// \tparam TResource Type of the resource to cast.
		/// \param resource The shared pointer to the resource to cast.
		/// \return Returns a shared pointer to the casted resource.
		template <typename TResource>
		typename ResourceMapping<TResource>::TMapped & resource_cast(TResource & resource){

			return static_cast<typename ResourceMapping<TResource>::TMapped&>(resource);

		}

		/// \brief Performs a resource cast from an abstract type to a concrete type.
		/// \tparam TResource Type of the resource to cast.
		/// \param resource The shared pointer to the resource to cast.
		/// \return Returns a shared pointer to the casted resource.
		template <typename TResource>
		typename const ResourceMapping<TResource>::TMapped & resource_cast(const TResource & resource){

			return static_cast<const typename ResourceMapping<TResource>::TMapped&>(resource);

		}


		//

		inline unsigned int DX11Texture2D::GetWidth() const{

			return width_;

		}

		inline unsigned int DX11Texture2D::GetHeight()const {

			return height_;

		}

		inline unsigned int DX11Texture2D::GetMipMapCount() const{

			return mip_levels_;

		}

		inline WrapMode DX11Texture2D::GetWrapMode() const{

			return wrap_mode_;

		}

		inline void DX11Texture2D::SetWrapMode(WrapMode wrap_mode){

			wrap_mode_ = wrap_mode;

		}

		inline ID3D11ShaderResourceView & DX11Texture2D::GetShaderResourceView(){

			return *shader_view_;
		}

		inline const ID3D11ShaderResourceView & DX11Texture2D::GetShaderResourceView() const{

			return *shader_view_;

		}

		// DX11RenderTarget

		inline size_t DX11RenderTarget::GetSize() const{

			return std::accumulate(textures_.begin(), 
								   textures_.end(), 
								   static_cast<size_t>(0), 
								   [](size_t size, const shared_ptr<DX11Texture2D> texture){ 
				
										return size + texture->GetSize(); 
			
								   });
			
		}

		inline ResourcePriority DX11RenderTarget::GetPriority() const{

			return textures_[0]->GetPriority();

		}

		inline void DX11RenderTarget::SetPriority(ResourcePriority priority){

			for (auto & texture : textures_){

				texture->SetPriority(priority);

			}

		}

		inline float DX11RenderTarget::GetAspectRatio() const{

			// The aspect ratio is guaranteed to be the same for all the targets.
			return static_cast<float>(textures_[0]->GetWidth()) /
				static_cast<float>(textures_[0]->GetHeight());

		}

		inline unsigned int DX11RenderTarget::GetCount() const{

			return static_cast<unsigned int>(textures_.size());

		}

		inline shared_ptr<Texture2D> DX11RenderTarget::GetTexture(int index){

			return std::static_pointer_cast<Texture2D>(textures_[index]);

		}

		inline shared_ptr<const Texture2D> DX11RenderTarget::GetTexture(int index) const{

			return std::static_pointer_cast<const Texture2D>(textures_[index]);

		}

		inline shared_ptr<Texture2D> DX11RenderTarget::GetZStencil(){

			return std::static_pointer_cast<Texture2D>(zstencil_);

		}

		inline shared_ptr<const Texture2D> DX11RenderTarget::GetZStencil() const{

			return std::static_pointer_cast<const Texture2D>(zstencil_);

		}

		// DX11Mesh

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

		inline Bounds DX11Mesh::GetBounds() const{

			return bounds_;

		}

		//

		inline size_t DX11Material::GetSize() const{

			return size_;

		}

		inline ResourcePriority DX11Material::GetPriority() const{

			return priority_;

		}

		inline void DX11Material::SetPriority(ResourcePriority priority){

			priority_ = priority;

		}

	}

}