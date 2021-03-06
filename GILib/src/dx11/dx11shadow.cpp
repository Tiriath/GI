#include "dx11/dx11shadow.h"

#include <algorithm>

#include "dx11/dx11graphics.h"
#include "dx11/dx11mesh.h"

#include "core.h"
#include "light_component.h"
#include "deferred_renderer.h"

using namespace ::std;
using namespace ::gi_lib;
using namespace ::dx11;

#undef max
#undef min

namespace {
	
	/// \brief Vertex shader constant buffer used to draw the geometry from the light perspective.
	struct VSMPerObjectCBuffer {

		Matrix4f world_light;								///< \brief World * Light-view matrix for point lights,
															///			World * Light-view * Light-proj matrix for directional lights

		Matrix4f world;										/// <\brief World matrix of the object

	};

	/// \brief Pixel shader constant buffer used to project the fragments to shadow space.
	struct VSMPerLightCBuffer {

		Matrix4f light_matrix;								///< \brief Light world matrix used to transform from light space to world space.

		float near_plane;									///< \brief Near clipping plane.

		float far_plane;									///< \brief Far clipping plane.

		Vector2i padding;

	};
		
	/// \brief Find the smallest chunk which can accommodate the given size in the given chunk array.
	/// If the method succeeds the best chunk is then moved at the end of the collection.
	/// \param size Size of the square region to accommodate.
	/// \return Returns returns the iterator to the best chunk in the given list. If non such chunk can be found the method returns an iterator to the end of the given collection
	template <typename TIterator>
	TIterator GetBestChunk(const Vector2i& size, TIterator begin, TIterator end) {

		auto best_chunk = end;

		for (auto it = begin; it != end; ++it) {

			if(it->sizes()(0) + 1 >= size(0) &&									// Fits horizontally
			   it->sizes()(1) + 1 >= size(1) &&									// Fits vertically
			   (best_chunk == end ||
			    it->sizes().maxCoeff() < best_chunk->sizes().maxCoeff())){		// Just an heuristic guess of "smallest"

				best_chunk = it;

			}

		}

		if (best_chunk != end) {

			// Swap the last element of the range with the best chunk and returns

			std::swap(*best_chunk,
					  *(--end));

		}

		
		return end;
		
	}
	
	/// \brief Splits the given chunk along each direction according to the requested size and returns the remaining chunks.
	/// The reserved chunk starts always from the top-left corner of the chunk.
	/// \param size Size of the reserved chunk.
	/// \param chunk Chunk to split.
	vector<AlignedBox2i> SplitChunk(const Vector2i& size, const AlignedBox2i& chunk) {

		// The reserved chunk is the top-left.
		vector<AlignedBox2i> bits;

		if (chunk.sizes()(0) + 1 > size(0)) {

			bits.push_back(AlignedBox2i(Vector2i(chunk.min()(0) + size(0), chunk.min()(1)),
										Vector2i(chunk.max()(0), chunk.min()(1) + size(1) - 1)));

		}

		if (chunk.sizes()(1) + 1 > size(1)) {

			bits.push_back(AlignedBox2i(Vector2i(chunk.min()(0), chunk.min()(1) + size(1)),
										chunk.max()));

		}

		return bits;

	}

	/// \brief Reserve a free chunk in the given chunk list and returns informations about the reserved chunk
	/// \param size Size of the region to reserve.
	/// \param chunks List of free chunks for each atlas page.
	/// \param page_index If the method succeeds, this parameter contains the atlas page where the chunk has been reserved.
	/// \param reserved_chunk if the method succeeds, this parameter contains the boundaries of the reserved chunk.
	/// \return Returns true if the method succeeds, return false otherwise.
	bool ReserveChunk(const Vector2i& size, vector<vector<AlignedBox2i>>& chunks, unsigned int& page_index, AlignedBox2i& reserved_chunk) {

		page_index = 0;

		for (auto&& page_chunks : chunks) {

			auto it = GetBestChunk(size, 
								   page_chunks.begin(), 
								   page_chunks.end());

			if (it != page_chunks.end()) {

				reserved_chunk = AlignedBox2i(it->min(),
											  it->min() + size - Vector2i::Ones());

				// Update the chunk list

				auto bits = SplitChunk(size, *it);

				page_chunks.pop_back();

				page_chunks.insert(page_chunks.end(),
								   bits.begin(),
								   bits.end());
				
				return true;

			}

			++page_index;

		}

		return false;

	}

	/// \brief Reserve a free chunk in the given chunk list and fill the proper shadow information.
	/// \param size Size of the region to reserve.
	/// \param chunks List of free chunks for each atlas page.
	/// \param shadow If the method succeeds, this object contains the updated shadow informations.
	/// \return Returns true if the method succeeds, return false otherwise.
	bool ReserveChunk(const Vector2i& size, const Vector2i& atlas_size, vector<vector<AlignedBox2i>>& chunks, PointShadow& shadow) {

		unsigned int page_index;
		AlignedBox2i reserved_chunk;

		if (ReserveChunk(size,		
						 chunks, 
						 page_index, 
						 reserved_chunk)) {

			auto uv_size = (atlas_size - Vector2i::Ones()).cast<float>();

			shadow.atlas_page = page_index;
			
			shadow.min_uv = reserved_chunk.min().cast<float>().cwiseQuotient(uv_size);
			shadow.max_uv = reserved_chunk.max().cast<float>().cwiseQuotient(uv_size);

			return true;

		}

		return false;

	}

	/// \brief Reserve a free chunk in the given chunk list and fill the proper shadow information.
	/// \param size Size of the region to reserve.
	/// \param chunks List of free chunks for each atlas page.
	/// \param shadow If the method succeeds, this object contains the updated shadow informations.
	/// \return Returns true if the method succeeds, return false otherwise.
	bool ReserveChunk(const Vector2i& size, const Vector2i& atlas_size, vector<vector<AlignedBox2i>>& chunks, DirectionalShadow& shadow) {

		unsigned int page_index;
		AlignedBox2i reserved_chunk;

		if (ReserveChunk(size, 
						 chunks, 
						 page_index, 
						 reserved_chunk)) {

			auto uv_size = (atlas_size - Vector2i::Ones()).cast<float>();

			shadow.atlas_page = page_index;
			
			shadow.min_uv = reserved_chunk.min().cast<float>().cwiseQuotient(uv_size);
			shadow.max_uv = reserved_chunk.max().cast<float>().cwiseQuotient(uv_size);

			return true;

		}

		return false;

	}
	
	/// \brief Get the minimum and the maximum depth along the specified direction for any given mesh in the provided volume collection.
	/// \param volumes Volumes collection to cycle through
	/// \param direction Depth direction.
	/// \return Returns a 2-element vector where the first element is the minimum depth found and the second is the maximum one.
	Vector2f GetZRange(const vector<VolumeComponent*>& volumes, const Vector3f& direction) {

		Vector2f range(std::numeric_limits<float>::infinity(),			// Min
					   -std::numeric_limits<float>::infinity());		// Max

		float distance;

		for (auto&& volume : volumes) {

			for (auto&& mesh_component : volume->GetComponents<MeshComponent>()) {

				auto& sphere = mesh_component.GetBoundingSphere();

				distance = sphere.center.dot(direction);

				if (distance - sphere.radius < range(0)) {

					range(0) = distance - sphere.radius;

				}

				if (distance + sphere.radius > range(1)) {

					range(1) = distance + sphere.radius;

				}

			}

		}
		
		return range;

	}

	/// \brief Get the light frustum associated to the specified directional light.
	/// The frustum is computed based on the assumption that the affected geometry is the one inside the camera's frustum only.
	/// \param directional_light Light whose frustum will be computed.
	/// \param camera The view camera.
	/// \param ortho_size The computed orthographic size. Output, optional.
	/// \return Returns the frustum associated to the directional light.
	Frustum GetLightFrustum(const DirectionalLightComponent& directional_light, const CameraComponent& camera, float aspect_ratio, Vector2f* ortho_size) {

		if (camera.GetProjectionType() == ProjectionType::Perspective) {

			// Take the diameter of the view frustum as upper bound of the orthographic size - This assumes that the light's position is in the center of the frustum

			float far_height = 2.0f * camera.GetMaximumDistance() * std::tanf(camera.GetFieldOfView() * 0.5f);		// Height of the far plane

			float diameter = Vector3f(far_height * aspect_ratio, far_height, camera.GetMaximumDistance()).norm() / 3.0f;	

			float half_diameter = diameter * 0.5f;

			if (ortho_size) {

				*ortho_size = Vector2f(diameter, diameter);		// Equal for each direction

			}

			// Create the frustum
			auto& camera_transform = camera.GetWorldTransform();
			auto camera_forward = Math::ToVector3(camera_transform.matrix().col(2)).normalized();

			auto& light_transform = *directional_light.GetComponent<TransformComponent>();
			
			Vector3f frustum_center = camera_transform.translation() + camera_forward * (camera.GetMinimumDistance() + camera.GetMaximumDistance()) * 0.5f;

			Vector3f light_forward = light_transform.GetForward();
			Vector3f light_right = light_transform.GetRight();
			Vector3f light_up = light_transform.GetUp();

			Vector3f domain_size = 15000.f * Vector3f::Identity();

			// Create the frustum
			return Frustum({ Math::MakePlane( light_forward, frustum_center + light_forward.cwiseProduct(domain_size)),			// Near clipping plane. The projection range is infinite.
							 Math::MakePlane(-light_forward, frustum_center - light_forward.cwiseProduct(domain_size)),			// Far clipping plane. The projection range is infinite.
							 Math::MakePlane(-light_right, frustum_center + light_right * half_diameter),						// Right clipping plane
							 Math::MakePlane( light_right, frustum_center - light_right * half_diameter),						// Left clipping plane
							 Math::MakePlane(-light_up, frustum_center + light_up * half_diameter),								// Top clipping plane
							 Math::MakePlane( light_up, frustum_center - light_up * half_diameter) });							// Bottom clipping plane
			
		}
		else {

			THROW(L"Not supported, buddy!");

		}
		
	}

}

///////////////////////////////////// DX11 VSM ATLAS /////////////////////////////////////////

DX11VSMAtlas::DX11VSMAtlas(unsigned int size/*, unsigned int pages*/, bool full_precision) :
	fx_blur_(gi_lib::fx::FxGaussianBlur::Parameters{ 1.67f, 5 }) {

	auto&& device = *DX11Graphics::GetInstance().GetDevice();
	
	// Get the immediate rendering context.

	ID3D11DeviceContext* context;

	device.GetImmediateContext(&context);

	immediate_context_ << &context;

	shadow_state_.SetDepthBias(-10000, -0.1f, -10.0f)
                 .SetWriteMode(true, true, D3D11_COMPARISON_GREATER);
	
	// Create the shadow resources

	sampler_ = new DX11Sampler(ISampler::FromDescription{ TextureMapping::CLAMP, TextureFiltering::ANISOTROPIC, 4 });
	diffuse_sampler_ = new DX11Sampler(ISampler::FromDescription{ TextureMapping::WRAP, TextureFiltering::ANISOTROPIC, 4 });

	auto format = full_precision ? TextureFormat::RG_FLOAT : TextureFormat::RG_HALF;
	
	atlas_ = new DX11GPTexture2D(IGPTexture2D::FromDescription{ size,
																size, 
																1, 
																format } );

	point_shadow_material_ = new DX11Material(IMaterial::CompileFromFile{ Application::GetInstance().GetDirectory() + L"Data\\Shaders\\octahedron_vsm.hlsl" });

	directional_shadow_material_ = new DX11Material(IMaterial::CompileFromFile{ Application::GetInstance().GetDirectory() + L"Data\\Shaders\\vsm.hlsl" });

	per_object_ = new DX11StructuredBuffer(sizeof(VSMPerObjectCBuffer));

	per_light_ = new DX11StructuredBuffer(sizeof(VSMPerLightCBuffer));

	rt_cache_ = std::make_unique<DX11RenderTargetCache>(IRenderTargetCache::Singleton{});

	// One-time setup

	bool check;

	check = point_shadow_material_->SetInput("PerObject",
											 ObjectPtr<IStructuredBuffer>(per_object_));

	check = point_shadow_material_->SetInput("PerLight",
											 ObjectPtr<IStructuredBuffer>(per_light_));

	check = point_shadow_material_->SetInput("gDiffuseSampler",
											 ObjectPtr<ISampler>(diffuse_sampler_));

	check = directional_shadow_material_->SetInput("PerObject",
												   ObjectPtr<IStructuredBuffer>(per_object_));

	check = directional_shadow_material_->SetInput("gDiffuseSampler",
												   ObjectPtr<ISampler>(diffuse_sampler_));

}

void DX11VSMAtlas::Reset() {
	
	// Clear any existing chunk and starts over again (Restore one big chunk for each atlas page)

	chunks_.resize(1/*atlas_->GetCount()*/);

	for (auto&& page_chunk : chunks_) {

		page_chunk.clear();
		page_chunk.push_back(AlignedBox2i(Vector2i::Zero(),
										  Vector2i(atlas_->GetWidth() - 1, 
												   atlas_->GetHeight() - 1)));

	}

}

bool DX11VSMAtlas::ComputeShadowmap(const PointLightComponent& point_light, const Scene& scene, PointShadow& shadow, ObjectPtr<IRenderTarget>* shadow_map) {

	if (!point_light.IsShadowEnabled() ||
		!ReserveChunk(point_light.GetShadowMapSize(),
					  Vector2i(atlas_->GetWidth(), atlas_->GetHeight()),
					  chunks_,
					  shadow)){

		shadow.enabled = 0;
		return false;

	}
	
	// Neutralize the light scaling

	auto& transform_component = point_light.GetTransformComponent();

	Matrix4f light_transform;

	light_transform.col(0) = Math::ToVector4(transform_component.GetRight(), 0.f);
	light_transform.col(1) = Math::ToVector4(transform_component.GetUp(), 0.f);
	light_transform.col(2) = Math::ToVector4(transform_component.GetForward(), 0.f);
	light_transform.col(3) = Math::ToVector4(transform_component.GetPosition(), 1.f);

	light_transform = light_transform.inverse().eval();

	// Fill the remaining shadow infos - Reverse depth

    shadow.near_plane = point_light.GetBoundingSphere().radius;
	shadow.far_plane = 100.0f;
	shadow.light_view_matrix = light_transform.matrix();

	shadow.enabled = 1;

	// Draw the actual shadowmap

	DrawShadowmap(shadow,
				  scene.GetMeshHierarchy().GetIntersections(point_light.GetBoundingSphere()),
				  light_transform,
				  shadow_map);
		
	return true;

}

bool DX11VSMAtlas::ComputeShadowmap(const DirectionalLightComponent& directional_light, const Scene& scene, DirectionalShadow& shadow, ObjectPtr<IRenderTarget>* shadow_map) {
		
	if (!directional_light.IsShadowEnabled() ||
		!ReserveChunk(directional_light.GetShadowMapSize(),
					  Vector2i(atlas_->GetWidth(), atlas_->GetHeight()),
					  chunks_,
					  shadow)){
		
		shadow.enabled = 0;
		return false;

	}

	// Calculate the light's boundaries

	Vector2f ortho_size(10000.f, 10000.f);

	Sphere domain{ Vector3f::Zero(), 15000.f };

	auto lit_geometry = scene.GetMeshHierarchy().GetIntersections(domain);

	auto z_range = GetZRange(lit_geometry,
							 directional_light.GetDirection());

	auto light_world_transform = directional_light.GetWorldTransform().matrix();

	auto light_transform = ComputeOrthographicProjectionLH(ortho_size(0),
														   ortho_size(1),
														   z_range(1),
										 				   z_range(0)) * light_world_transform.inverse();
	
	// Fill the remaining shadow infos

	shadow.light_view_matrix = light_transform;

	shadow.enabled = 1;

	// Draw the actual shadowmap

	DrawShadowmap(shadow,
				  lit_geometry,
				  light_transform,
				  shadow_map);

	return true;

}

void DX11VSMAtlas::DrawShadowmap(const PointShadow& shadow, const vector<VolumeComponent*>& nodes, const Matrix4f& light_view_transform, ObjectPtr<IRenderTarget>* shadow_map){

	// Per-light setup

	auto& per_light_front = *per_light_->Lock<VSMPerLightCBuffer>();

	per_light_front.near_plane = shadow.near_plane;
	per_light_front.far_plane = shadow.far_plane;

	per_light_->Unlock();

	// Draw the geometry to the shadowmap

	auto atlas_size = Vector2f(atlas_->GetWidth(), atlas_->GetHeight());

	AlignedBox2i boundaries(shadow.min_uv.cwiseProduct(atlas_size).cast<int>(),
							shadow.max_uv.cwiseProduct(atlas_size).cast<int>());

	DrawShadowmap(boundaries,
				  shadow.atlas_page,
				  nodes,
				  point_shadow_material_,
				  light_view_transform.matrix(),
				  shadow_map);
	
}

void DX11VSMAtlas::DrawShadowmap(const DirectionalShadow& shadow, const vector<VolumeComponent*>& nodes, const Matrix4f& light_proj_transform, ObjectPtr<IRenderTarget>* shadow_map) {
	
	// Draw the geometry to the shadowmap

	auto atlas_size = Vector2f(atlas_->GetWidth(), atlas_->GetHeight());

	AlignedBox2i boundaries(shadow.min_uv.cwiseProduct(atlas_size).cast<int>(),
							shadow.max_uv.cwiseProduct(atlas_size).cast<int>());

	DrawShadowmap(boundaries,
				  shadow.atlas_page,
				  nodes,
				  directional_shadow_material_,
				  light_proj_transform,
				  shadow_map);

}

void DX11VSMAtlas::DrawShadowmap(const AlignedBox2i& boundaries, unsigned int /*atlas_page*/, const vector<VolumeComponent*> nodes, const ObjectPtr<DX11Material>& shadow_material, const Matrix4f& light_transform, ObjectPtr<IRenderTarget>* out_shadow_map, bool tessellable) {

	auto& graphics_ = DX11Graphics::GetInstance();

	auto& context = graphics_.GetContext();

	graphics_.PushEvent(L"Shadowmap");

	// Bind a new shadowmap to the output

	auto shadow_map = rt_cache_->PopFromCache(boundaries.sizes()(0) + 1,
											  boundaries.sizes()(1) + 1,
											  { atlas_->GetFormat(),
												TextureFormat::RGBA_BYTE_UNORM },
											   true);

	ObjectPtr<DX11Mesh> mesh;
	ObjectPtr<ITexture2D> diffuse_map;
	
	context.PushPipelineState(shadow_state_);

	resource_cast(shadow_map)->ClearTargets(*immediate_context_, kOpaqueWhite);
	resource_cast(shadow_map)->ClearDepth(*immediate_context_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.f);

	resource_cast(shadow_map)->Bind(*immediate_context_);

	for (auto&& node : nodes) {

		for (auto&& drawable : node->GetComponents<AspectComponent<DeferredRendererMaterial>>()) {

			// Per-object setup

			mesh = drawable.GetMesh();

			graphics_.PushEvent(mesh->GetName());

			mesh->Bind(*immediate_context_,
					   tessellable);

			auto& per_object = *per_object_->Lock<VSMPerObjectCBuffer>();

			per_object.world_light = (light_transform * drawable.GetWorldTransform()).matrix();
			per_object.world = drawable.GetWorldTransform().matrix();

			per_object_->Unlock();

			for (unsigned int subset_index = 0; subset_index < mesh->GetSubsetCount(); ++subset_index) {

				graphics_.PushEvent(mesh->GetSubsetName(subset_index));

				if (mesh->GetFlags(subset_index) && MeshFlags::kShadowcaster) {
					
					auto&& mesh_material = drawable.GetMaterial(subset_index)->GetMaterial();

					if (mesh_material->GetInput(IMaterial::kDiffuseMap, diffuse_map)) {

						shadow_material->SetInput(IMaterial::kDiffuseMap, diffuse_map);

					}
					else {

						shadow_material->SetInput(IMaterial::kDiffuseMap, ObjectPtr<ITexture2D>());

					}

					shadow_material->Bind(*immediate_context_);

					// Draw	the subset

					mesh->DrawSubset(*immediate_context_, 
									 subset_index);

				}

				graphics_.PopEvent();

			}

			graphics_.PopEvent();

		}

	}

	shadow_material->Unbind(*immediate_context_);

	resource_cast(shadow_map)->Unbind(*immediate_context_);

	context.PopPipelineState();
	
	// Blur the shadow map on top of the atlas

	graphics_.PushEvent(L"VSM Blur");

	fx_blur_.Blur((*shadow_map)[0],
			      ObjectPtr<IGPTexture2D>(atlas_),
				  boundaries.corner(AlignedBox2i::BottomLeft));

	graphics_.PopEvent();

	// Cleanup
	
	if (!out_shadow_map) {
		
		rt_cache_->PushToCache(shadow_map);		// Not needed anymore
	
	}
	else {

		*out_shadow_map = shadow_map;			// Return the texture to the output

	}

	
	graphics_.PopEvent();
	
}

