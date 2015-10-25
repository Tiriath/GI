#include "dx11/dx11deferred_renderer.h"

#include "gimath.h"
#include "mesh.h"

#include "object.h"

#include "dx11/dx11.h"
#include "dx11/dx11render_target.h"
#include "dx11/dx11mesh.h"
#include "dx11/dx11shader.h"

#include "windows/win_os.h"
#include "instance_builder.h"
#include "light_component.h"

using namespace ::std;
using namespace ::gi_lib;
using namespace ::gi_lib::dx11;
using namespace ::gi_lib::windows;

namespace{
	
	/// \brief Draw a mesh subset using the given context.
	void DrawIndexedSubset(ID3D11DeviceContext& context, const MeshSubset& subset){

		context.DrawIndexed(static_cast<unsigned int>(subset.count),
							static_cast<unsigned int>(subset.start_index),
							0);

	}

	/// \brief Compute the view-projection matrix given a camera and the aspect ratio of the target.
	Matrix4f ComputeViewProjectionMatrix(const CameraComponent& camera, float aspect_ratio){
	
		auto view_matrix = camera.GetViewTransform();

		Matrix4f projection_matrix;

		if (camera.GetProjectionType() == ProjectionType::Perspective){

			// Hey this method should totally become a member method of the camera component!
			// NOPE! DirectX and OpenGL calculate the projection differently (mostly because of the near plane which in the first case goes from 0 to 1, in the latter goes from -1 to 1).
			
			projection_matrix = ComputePerspectiveProjectionLH(camera.GetFieldOfView(),
															   aspect_ratio,
															   camera.GetMinimumDistance(),
															   camera.GetMaximumDistance());

		}
		else if (camera.GetProjectionType() == ProjectionType::Ortographic){

			THROW(L"Not implemented, buddy!");

		}
		else{

			THROW(L"What kind of projection are you trying to use again?! O.o");

		}
		
		return (projection_matrix * view_matrix).matrix();

	}

	/// \brief Compute the visible nodes inside a given hierachy given the camera and the aspect ratio of the target.
	vector<VolumeComponent*> ComputeVisibleNodes(const IVolumeHierarchy& volume_hierarchy, const CameraComponent& camera, float aspect_ratio){

		// Basic frustum culling

		auto camera_frustum = camera.GetViewFrustum(aspect_ratio);

		return volume_hierarchy.GetIntersections(camera_frustum);											// Updates the view frustum according to the output ratio.
		
	}

}

///////////////////////////////// DX11 DEFERRED RENDERER MATERIAL ///////////////////////////////

const Tag DX11DeferredRendererMaterial::kDiffuseMapTag = "gDiffuseMap";
const Tag DX11DeferredRendererMaterial::kDiffuseSampler = "gDiffuseSampler";
const Tag DX11DeferredRendererMaterial::kPerObjectTag = "PerObject";

DX11DeferredRendererMaterial::DX11DeferredRendererMaterial(const CompileFromFile& args) :
material_(new DX11Material(args))
{

	Setup();

}

DX11DeferredRendererMaterial::DX11DeferredRendererMaterial(const Instantiate& args) :
material_(new DX11Material(IMaterial::Instantiate{ args.base->GetMaterial() })){

	Setup();

}

void DX11DeferredRendererMaterial::SetMatrix(const Affine3f& world, const Matrix4f& view_projection){

	// Lock

	auto&& buffer = **per_object_cbuffer_;

	// Update

	buffer.world = world.matrix();
	buffer.world_view_proj = (view_projection * world).matrix();

	// Unlock

	per_object_cbuffer_->Unlock();
	
}

void DX11DeferredRendererMaterial::Setup(){

	auto& resources = DX11Graphics::GetInstance().GetResources();

	per_object_cbuffer_ = new StructuredBuffer<VSPerObjectBuffer>(resources.Load<IStructuredBuffer, IStructuredBuffer::FromSize>({sizeof(VSPerObjectBuffer)}));

	material_->SetInput(kPerObjectTag, per_object_cbuffer_);

}

///////////////////////////////// DX11 TILED DEFERRED RENDERER //////////////////////////////////

const Tag DX11TiledDeferredRenderer::kAlbedoTag = "gAlbedo";
const Tag DX11TiledDeferredRenderer::kNormalShininessTag = "gNormalShininess";
const Tag DX11TiledDeferredRenderer::kDepthStencilTag = "gDepthStencil";
const Tag DX11TiledDeferredRenderer::kPointLightsTag = "gPointLights";
const Tag DX11TiledDeferredRenderer::kDirectionalLightsTag = "gDirectionalLights";
const Tag DX11TiledDeferredRenderer::kLightBufferTag = "gLightAccumulation";
const Tag DX11TiledDeferredRenderer::kLightParametersTag = "gParameters";

DX11TiledDeferredRenderer::DX11TiledDeferredRenderer(const RendererConstructionArgs& arguments) :
TiledDeferredRenderer(arguments.scene),
fx_bloom_(1.0f, 1.67f, Vector2f(0.5f, 0.5f)),
fx_tonemap_(0.5f){

	auto&& device = *DX11Graphics::GetInstance().GetDevice();

	// Get the immediate rendering context.

	ID3D11DeviceContext* context;

	device.GetImmediateContext(&context);

	immediate_context_ << &context;

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

	depth_state_ << &depth_state;

	ZeroMemory(&depth_state_desc, sizeof(D3D11_DEPTH_STENCIL_DESC));

	device.CreateDepthStencilState(&depth_state_desc,
								   &depth_state);

	disable_depth_test_ << &depth_state;

	// Create the blend state

	D3D11_BLEND_DESC blend_state_desc;

	ID3D11BlendState* blend_state;
	
	blend_state_desc.AlphaToCoverageEnable = false;
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

	blend_state_ << &blend_state;

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

	rasterizer_state_ << &rasterizer_state;
	
	// Move the stuffs below somewhere else

	auto&& app = Application::GetInstance();

	// Light accumulation setup

	light_shader_ = DX11Resources::GetInstance().Load<IComputation, IComputation::CompileFromFile>({ app.GetDirectory() + L"Data\\Shaders\\lighting.hlsl" });

	point_lights_ = new DX11StructuredArray(32, sizeof(PointLight));
	directional_lights_ = new DX11StructuredArray(32, sizeof(DirectionalLight));
	light_accumulation_parameters_ = new DX11StructuredBuffer(sizeof(LightAccumulationParameters));

	// One-time setup
	bool check;

	check = light_shader_->SetInput(kLightParametersTag,
									ObjectPtr<IStructuredBuffer>(light_accumulation_parameters_));

	check = light_shader_->SetInput(kPointLightsTag,
									ObjectPtr<IStructuredArray>(point_lights_));

	check = light_shader_->SetInput(kDirectionalLightsTag,
									ObjectPtr<IStructuredArray>(directional_lights_));

}

DX11TiledDeferredRenderer::~DX11TiledDeferredRenderer(){
	
	immediate_context_ = nullptr;
	depth_state_ = nullptr;
	blend_state_ = nullptr;
	rasterizer_state_ = nullptr;
	disable_depth_test_ = nullptr;
	
}

ObjectPtr<ITexture2D> DX11TiledDeferredRenderer::Draw(unsigned int width, unsigned int height){
	
	// Draws only if there's a camera

	auto main_camera = GetScene().GetMainCamera();

	if (main_camera){

		FrameInfo frame_info;

		frame_info.scene = &GetScene();
		frame_info.camera = main_camera;
		frame_info.aspect_ratio = static_cast<float>(width) / static_cast<float>(height);
		frame_info.width = width;
		frame_info.height = height;
		frame_info.view_proj_matrix = ComputeViewProjectionMatrix(*main_camera, frame_info.aspect_ratio);

		DrawGBuffer(frame_info);							// Scene -> GBuffer
		
		ComputeLighting(frame_info);						// Scene, GBuffer, DepthBuffer -> LightBuffer

		ComputePostProcess(frame_info);					// LightBuffer -> Output

	}

	// Cleanup
	immediate_context_->ClearState();
	
	// Return the unexposed buffer
	return tonemap_output_->GetTexture();

}

// GBuffer

void DX11TiledDeferredRenderer::DrawGBuffer(const FrameInfo& frame_info){

	// Setup the GBuffer and bind it to the immediate context.
	
	BindGBuffer(frame_info);

	// Draw the visible nodes

	DrawNodes(ComputeVisibleNodes(frame_info.scene->GetMeshHierarchy(), *frame_info.camera, frame_info.aspect_ratio), 
			  frame_info);
	
	// Cleanup

	gbuffer_->Unbind(*immediate_context_);

}

void DX11TiledDeferredRenderer::BindGBuffer(const FrameInfo& frame_info){

	// Rasterizer state setup

	immediate_context_->RSSetState(rasterizer_state_.Get());

	// Setup of the output merger - This should be defined per render section actually but for now only opaque geometry with no fancy stencil effects is supported.

	immediate_context_->OMSetDepthStencilState(depth_state_.Get(),
											   0);

	immediate_context_->OMSetBlendState(blend_state_.Get(),
										0,
										0xFFFFFFFF);


	// Setup of the render target surface (GBuffer)

	if (!gbuffer_){

		// Lazy initialization

		gbuffer_ = new DX11RenderTarget(frame_info.width,
										frame_info.height,
										{ DXGI_FORMAT_R16G16B16A16_FLOAT,
										  DXGI_FORMAT_R16G16B16A16_FLOAT });

	}
	else{

		// GBuffer resize (will actually resize the target only if the size has been changed since the last frame)

		gbuffer_->Resize(frame_info.width,
						 frame_info.height);

	}

	static const Color kSkyColor = Color{ 0.66f, 2.05f, 3.96f, 1.0f };

	gbuffer_->ClearTargets(*immediate_context_, kSkyColor);
	
	gbuffer_->ClearDepth(*immediate_context_);

	// Bind the gbuffer to the immediate context

	gbuffer_->Bind(*immediate_context_);

}

void DX11TiledDeferredRenderer::DrawNodes(const vector<VolumeComponent*>& meshes, const FrameInfo& frame_info){

	ObjectPtr<DX11Mesh> mesh;
	ObjectPtr<DX11DeferredRendererMaterial> material;

	// Trivial solution: cycle through each node and for each drawable component draws the subsets.

	// TODO: Implement some batching strategy here!

	for (auto&& node : meshes){

		for (auto&& drawable : node->GetComponents<DeferredRendererComponent>()){

			// Bind the mesh

			mesh = drawable.GetMesh();

			mesh->Bind(*immediate_context_);

			// For each subset

			for (unsigned int subset_index = 0; subset_index < mesh->GetSubsetCount(); ++subset_index){
				
				// Bind the subset material

				material = drawable.GetMaterial(subset_index);

				material->SetMatrix(drawable.GetWorldTransform(),
									frame_info.view_proj_matrix);

				material->Bind(*immediate_context_);

				// Draw	the subset

				DrawIndexedSubset(*immediate_context_,
								  mesh->GetSubset(subset_index));

			}

		}

	}

}

// Lighting

void DX11TiledDeferredRenderer::ComputeLighting(const FrameInfo& frame_info){

	// Lazy setup and resize of the light accumulation buffer

	if (!light_buffer_ ||
		light_buffer_->GetWidth() != frame_info.width ||
		light_buffer_->GetHeight() != frame_info.height){

		light_buffer_ = new DX11GPTexture2D(frame_info.width,
											frame_info.height,											
											DXGI_FORMAT_R11G11B10_FLOAT);
		
	}

	// Accumulate the visible lights
	auto&& visible_lights = ComputeVisibleNodes(frame_info.scene->GetLightHierarchy(), *frame_info.camera, frame_info.aspect_ratio);

	AccumulateLight(visible_lights,
					frame_info);

}

void DX11TiledDeferredRenderer::AccumulateLight(const vector<VolumeComponent*>& lights, const FrameInfo& frame_info) {

	PointLight* point_lights_ptr = reinterpret_cast<PointLight*>(point_lights_->Lock());
	DirectionalLight* directional_lights_ptr = reinterpret_cast<DirectionalLight*>(directional_lights_->Lock());
	LightAccumulationParameters* parameters_ptr = reinterpret_cast<LightAccumulationParameters*>(light_accumulation_parameters_->Lock());

	unsigned int point_light_index = 0;
	unsigned int directional_light_index = 0;

	for (auto&& node : lights) {

		for (auto&& point_light : node->GetComponents<PointLightComponent>()) {

			point_lights_ptr[point_light_index].position = Math::ToVector4(point_light.GetPosition(), 1.0f);
			point_lights_ptr[point_light_index].color = point_light.GetColor().ToVector4f();
			point_lights_ptr[point_light_index].kc = point_light.GetConstantFactor();
			point_lights_ptr[point_light_index].kl= point_light.GetLinearFactor();
			point_lights_ptr[point_light_index].kq = point_light.GetQuadraticFactor();
			point_lights_ptr[point_light_index].cutoff = point_light.GetCutoff();

			++point_light_index;

		}

		for (auto&& directional_light : node->GetComponents<DirectionalLightComponent>()) {

			directional_lights_ptr[directional_light_index].direction = Math::ToVector4(directional_light.GetDirection(), 1.0f);
			directional_lights_ptr[directional_light_index].color = directional_light.GetColor().ToVector4f();

			++directional_light_index;

		}

	}

	parameters_ptr->camera_position = frame_info.camera->GetPosition();
	parameters_ptr->inv_view_proj_matrix = frame_info.view_proj_matrix.inverse();
	parameters_ptr->point_lights = point_light_index;
	parameters_ptr->directional_lights = directional_light_index;

	point_lights_->Unlock();
	directional_lights_->Unlock();
	light_accumulation_parameters_->Unlock();
	
	// These entities may change from frame to frame
	bool check;

	check = light_shader_->SetInput(kAlbedoTag,
									(*gbuffer_)[0]);

	check = light_shader_->SetInput(kNormalShininessTag,
									(*gbuffer_)[1]);
	
	check = light_shader_->SetInput(kDepthStencilTag,
									gbuffer_->GetDepthBuffer());
	
	check = light_shader_->SetOutput(kLightBufferTag,
									 ObjectPtr<IGPTexture2D>(light_buffer_));

	// Actual light computation

	light_shader_->Dispatch(*immediate_context_,			// Dispatch one thread for each GBuffer's pixel
							light_buffer_->GetWidth(),
							light_buffer_->GetHeight(),
							1);

}

// Post processing

void DX11TiledDeferredRenderer::ComputePostProcess(const FrameInfo& frame_info){

	// LightBuffer == [Bloom] ==> Unexposed ==> [Tonemap] ==> Output

	// Lazy initialization and resize of the bloom and tonemap surfaces

	if (!bloom_output_ ||
		 bloom_output_->GetWidth() != frame_info.width ||
		 bloom_output_->GetHeight() != frame_info.height) {

		bloom_output_ = new DX11RenderTarget(frame_info.width,
											 frame_info.height, 
											 { DXGI_FORMAT_R11G11B10_FLOAT });

		tonemap_output_ = new DX11GPTexture2D(frame_info.width,
											  frame_info.height, 
											  DXGI_FORMAT_R8G8B8A8_UNORM);

	}

	// Bloom

	fx_bloom_.Process(light_buffer_->GetTexture(),
					  bloom_output_);

	
	// Tonemap

	fx_tonemap_.Process((*bloom_output_)[0],
						tonemap_output_);
	
}