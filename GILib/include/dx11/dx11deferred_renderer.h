/// \file dx11deferred_renderer.h
/// \brief Deferred rendering classes for DirectX11.
///
/// \author Raffaele D. Facendola

#pragma once

#include "deferred_renderer.h"

#include "instance_builder.h"
#include "dx11renderer.h"
#include "dx11graphics.h"
#include "dx11material.h"
#include "dx11buffer.h"
#include "buffer.h"


namespace gi_lib{

	namespace dx11{

		/// \brief Structure of the per-object constant buffer.
		struct PerObjectBuffer{

			Matrix4f gWorldViewProj;		// World * View * Projection matrix.
			Matrix4f gWorld;				// World matrix.

		};

		/// \brief Material for a DirectX11 deferred renderer.
		/// A custom material should not be compiled from code directly since there's no way of knowing whether the code is compatible with the custom renderer.
		/// A concrete deferred material is not a subclass of a DirectX11 material to prevent a DDoD. Composition is preferred here.
		/// \author Raffaele D. Facendola
		class DX11DeferredRendererMaterial : public DeferredRendererMaterial{

		public:

			/// \brief Create a new DirectX11 deferred material from shader code.
			/// \param device The device used to load the graphical resources.
			/// \param bundle Bundle used to load the material.
			DX11DeferredRendererMaterial(const CompileFromFile& args);

			/// \brief Instantiate a DirectX11 deferred material from another one.
			/// \param device The device used to load the graphical resources.
			/// \param bundle Bundle used to instantiate the material.
			DX11DeferredRendererMaterial(const Instantiate& args);

			virtual ObjectPtr<IMaterial> GetMaterial() override;

			virtual ObjectPtr<const IMaterial> GetMaterial() const override;

			/// \brief Set the matrices needed to transform the object.
			void SetMatrix(const Affine3f& world, const Affine3f& view, const Matrix4f& projection);
			
			/// \brief Commit all the constant buffers and bind the material to the pipeline.
			void Commit(ID3D11DeviceContext& context);

			virtual size_t GetSize() const override;

		private:

			static const Tag kDiffuseMapTag;									///< \brief Tag associated to the diffuse map.

			static const Tag kDiffuseSampler;									///< \brief Tag associated to the sampler used to sample from the diffuse map.

			static const Tag kPerObjectTag;										///< \brief Tag associated to the per-object constant buffer.

			ObjectPtr<StructuredBuffer<PerObjectBuffer>> per_object_cbuffer_;	///< \brief Constant buffer containing the per-object constants used by the vertex shader.

			/// \brief Setup the material variables and resources.
			void Setup();											

			ObjectPtr<DX11Material> material_;							///< \brief DirectX11 material.

		};

		/// \brief Deferred renderer with tiled lighting computation for DirectX11.
		/// \author Raffaele D. Facendola
		class DX11TiledDeferredRenderer : public TiledDeferredRenderer{

		public:

			/// \brief Create a new tiled deferred renderer.
			/// \param arguments Arguments used to construct the renderer.
			DX11TiledDeferredRenderer(const RendererConstructionArgs& arguments);

			/// \brief No copy constructor.
			DX11TiledDeferredRenderer(const DX11TiledDeferredRenderer&) = delete;

			/// \brief Virtual destructor.
			virtual ~DX11TiledDeferredRenderer();

			/// \brief No assignment operator.
			DX11TiledDeferredRenderer& operator=(DX11TiledDeferredRenderer&) = delete;

			virtual void Draw(ObjectPtr<IRenderTarget> render_target) override;

		private:

			void SetupLights();

			void BindGBuffer(unsigned int width, unsigned int height);

			void DrawGBuffer(unsigned int width, unsigned int height);

			void ComputeLighting(unsigned int width, unsigned int height);

			/// \brief Starts the post process stage.
			void StartPostProcess();
			
			//void InitializeBloom();

			void Bloom(ObjectPtr<ITexture2D>& source, ObjectPtr<IGPTexture2D>& destination);

			/// \brief Initialize tonemap-related objects.
			void InitializeToneMap();

			/// \brief Perform a tonemap to the specified source surface and output the result to the destination surface.
			/// \param source_view Shader view of the HDR surface.
			/// \param destination Destination render target.
			void ToneMap(ObjectPtr<ITexture2D>& source, ObjectPtr<IGPTexture2D>& destination);

			// Render context

			COMPtr<ID3D11DeviceContext> immediate_context_;				///< \brief Immediate rendering context.

			COMPtr<ID3D11DepthStencilState> depth_state_;				///< \brief Depth-stencil buffer state.

			COMPtr<ID3D11BlendState> blend_state_;						///< \brief Output merger blending state.

			COMPtr<ID3D11RasterizerState> rasterizer_state_;			///< \brief Rasterizer state.

			// Lights

			ObjectPtr<DX11StructuredArray> light_array_;				///< \brief Array containing the lights.

			// Deferred resources

			ObjectPtr<DX11RenderTarget> gbuffer_;						///< \brief GBuffer.

			ObjectPtr<DX11RenderTarget> light_buffer_;					///< \brief Light buffer.

			COMPtr<ID3D11ComputeShader> light_cs_;						///< \brief DELETE ME

			COMPtr<ID3D11DepthStencilState> disable_depth_test_;		///< \brief Used to disable the depth testing.
			
			// Post process - Tonemapping

			ObjectPtr<DX11Material> tonemapper_;						///< \brief Material used to perform tonemapping.

			Tag tonemap_exposure_;

			Tag tonemap_vignette_;

			Tag tonemap_source_;
						
		};
		
		/////////////////////////////////// DX11 DEFERRED RENDERER MATERIAL ///////////////////////////////////

		INSTANTIABLE(DeferredRendererMaterial, DX11DeferredRendererMaterial, DeferredRendererMaterial::CompileFromFile);
		INSTANTIABLE(DeferredRendererMaterial, DX11DeferredRendererMaterial, DeferredRendererMaterial::Instantiate);

		inline ObjectPtr<IMaterial> DX11DeferredRendererMaterial::GetMaterial(){

			return ObjectPtr<IMaterial>(material_);

		}
		
		inline ObjectPtr<const IMaterial> gi_lib::dx11::DX11DeferredRendererMaterial::GetMaterial() const{

			return ObjectPtr<const IMaterial>(material_);

		}

		inline void gi_lib::dx11::DX11DeferredRendererMaterial::Commit(ID3D11DeviceContext& context){

			material_->Bind(context);
			
		}	

		inline size_t gi_lib::dx11::DX11DeferredRendererMaterial::GetSize() const{

			return material_->GetSize();

		}

		///////////////////////////////////// DX11 TILED DEFERRED RENDERER //////////////////////////////////

		INSTANTIABLE(TiledDeferredRenderer, DX11TiledDeferredRenderer, RendererConstructionArgs);

	}

}
