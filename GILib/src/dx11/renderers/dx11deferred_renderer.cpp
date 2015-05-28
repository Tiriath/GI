#include "dx11deferred_renderer.h"

#include "..\dx11.h"
#include "..\..\..\include\gimath.h"
#include "..\..\..\include\windows\os_windows.h"

using namespace ::std;
using namespace ::gi_lib;
using namespace ::gi_lib::dx11;
using namespace ::gi_lib::windows;
using namespace ::Eigen;

namespace{

	void DrawIndexedSubset(ID3D11DeviceContext& context, const MeshSubset& subset){

		context.DrawIndexed(static_cast<unsigned int>(subset.count),
							static_cast<unsigned int>(subset.start_index),
							0);

	}

}

///////////////////////////////// DX11 DEFERRED RENDERER MATERIAL ///////////////////////////////

DX11DeferredRendererMaterial::DX11DeferredRendererMaterial(const CompileFromFile& args) :
material_(new DX11Material(args))
{}

DX11DeferredRendererMaterial::DX11DeferredRendererMaterial(const Instantiate& args) :
material_(new DX11Material(Material::Instantiate{ args.base->GetMaterial() })){}

///////////////////////////////// DX11 TILED DEFERRED RENDERER //////////////////////////////////

DX11TiledDeferredRenderer::DX11TiledDeferredRenderer(const RendererConstructionArgs& arguments) :
TiledDeferredRenderer(arguments.scene){

	auto& device = DX11Graphics::GetInstance().GetDevice();

	// Get the immediate rendering context.

	ID3D11DeviceContext* context;

	device.GetImmediateContext(&context);

	immediate_context_ = std::move(unique_com(context));

	// Create the depth stencil state

	D3D11_DEPTH_STENCIL_DESC depth_state_desc;

	ID3D11DepthStencilState* depth_state;
	
	depth_state_desc.DepthEnable = true;
	depth_state_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depth_state_desc.DepthFunc = D3D11_COMPARISON_LESS;
	depth_state_desc.StencilEnable = false;
	depth_state_desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	depth_state_desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	
	depth_state_desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	depth_state_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depth_state_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depth_state_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

	depth_state_desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	depth_state_desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depth_state_desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depth_state_desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

	device.CreateDepthStencilState(&depth_state_desc,
								   &depth_state);

	depth_state_.reset(depth_state);

	// Create the blend state

	D3D11_BLEND_DESC blend_state_desc;

	ID3D11BlendState* blend_state;
	
	blend_state_desc.AlphaToCoverageEnable = true;
	blend_state_desc.IndependentBlendEnable = false;

	blend_state_desc.RenderTarget[0].BlendEnable = false;
	blend_state_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blend_state_desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
	blend_state_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blend_state_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blend_state_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blend_state_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blend_state_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	device.CreateBlendState(&blend_state_desc,
							&blend_state);

	blend_state_.reset(blend_state);

	// Create the raster state.

	D3D11_RASTERIZER_DESC rasterizer_state_desc;

	ID3D11RasterizerState* rasterizer_state;

	rasterizer_state_desc.FillMode = D3D11_FILL_SOLID;
	rasterizer_state_desc.CullMode = D3D11_CULL_BACK;
	rasterizer_state_desc.FrontCounterClockwise = false;
	rasterizer_state_desc.DepthBias = 0;
	rasterizer_state_desc.SlopeScaledDepthBias = 0.0f;
	rasterizer_state_desc.DepthBiasClamp = 0.0f;
	rasterizer_state_desc.DepthClipEnable = true;
	rasterizer_state_desc.ScissorEnable = false;
	rasterizer_state_desc.MultisampleEnable = false;
	rasterizer_state_desc.AntialiasedLineEnable = false;

	device.CreateRasterizerState(&rasterizer_state_desc,
								 &rasterizer_state);

	rasterizer_state_.reset(rasterizer_state);
	
}

DX11TiledDeferredRenderer::~DX11TiledDeferredRenderer(){}

void DX11TiledDeferredRenderer::Draw(IOutput& output){
	
	// Scene to draw
	auto& scene = GetScene();

	// The cast is safe as long as the client is not mixing different APIs.
	auto& dx11output = static_cast<DX11Output&>(output);

	// Draws only if there's a camera

	if (scene.GetMainCamera()){

		auto& camera = *scene.GetMainCamera();

		auto render_target = resource_cast(dx11output.GetRenderTarget());
	
		// Frustum culling

//  		auto nodes = scene.GetVolumeHierarchy()
//  						  .GetIntersections(camera.GetViewFrustum(render_target->GetAspectRatio()),		// Updates the view frustum according to the output ratio.
//  										    IVolumeHierarchy::PrecisionLevel::Medium);					// Avoids extreme false positive while keeping reasonably high performances.


		auto nodes = scene.GetNodes();

		// Setup of the render context

		immediate_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Viewport
		D3D11_VIEWPORT viewport;

		viewport.Width = static_cast<float>(render_target->GetTexture(0)->GetWidth());
		viewport.Height = static_cast<float>(render_target->GetTexture(0)->GetHeight());
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;

		immediate_context_->RSSetViewports(1, 
										   &viewport);

		immediate_context_->RSSetState(rasterizer_state_.get());

		immediate_context_->OMSetDepthStencilState(depth_state_.get(), 
												   0);

		immediate_context_->OMSetBlendState(blend_state_.get(), 
											0, 
											0xFFFFFFFF);

		// Set up render GBuffer render targets

		render_target->ClearDepthStencil(*immediate_context_,
										 D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
										 1.0f,
										 0);

		Color color;

		color.color.alpha = 0.0f;
		color.color.red = 0.0f;
		color.color.green = 0.0f;
		color.color.blue = 0.0f;

		render_target->ClearTargets(*immediate_context_, 
									color);

		// Bind the render target

		render_target->Bind(*immediate_context_);

		auto& camera_transform = *camera.GetComponent<TransformComponent>();

		Matrix4f camera_view = camera_transform.GetWorldTransform().matrix().inverse();

		Matrix4f camera_projection = ComputePerspectiveProjectionLH(camera.GetFieldOfView(),
																	render_target->GetAspectRatio(),
																	camera.GetMinimumDistance(),
																	camera.GetMaximumDistance());

		

		// Draw GBuffer
		for (auto&& node : nodes){

			// Items to draw

			for (auto&& drawable : node->GetComponents<DeferredRendererComponent>()){

				// Bind the mesh

				auto mesh = resource_cast(drawable.GetMesh());

				mesh->Bind(*immediate_context_);

				// For each subset
				for (unsigned int subset_index = 0; subset_index < mesh->GetSubsetCount(); ++subset_index){

					// Bind the material
					
					auto deferred_material = drawable.GetMaterial(subset_index);

					auto material = resource_cast(deferred_material->GetMaterial());


					auto variable = material->GetVariable("gWorldViewProj");

					auto world = drawable.GetComponent<TransformComponent>()->GetWorldTransform();

					variable->Set(camera_projection * (camera_view * world));

					material->Commit(*immediate_context_);

					// Draw	the subset
					DrawIndexedSubset(*immediate_context_,
									  mesh->GetSubset(subset_index));

				}

			}

		}

		// Compute lighting

	}

	// Present the image
	dx11output.Present();

	// Restore the rendering context

	immediate_context_->ClearState();
	
}

