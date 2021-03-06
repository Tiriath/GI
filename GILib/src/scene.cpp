#include "scene.h"

#include <algorithm>
#include <assert.h>

#include "gilib.h"
#include "exceptions.h"

#include "mesh.h"

using namespace ::gi_lib;
using namespace ::std;

namespace{

	/// \brief Calculate the view frustum from a projective camera description.
	/// \param camera_transform Camera transform matrix in world space.
	/// \param near_distance Minimum projected distance in world units.
	/// \param far_distance Maximum projected distance in world units.
	/// \param field_of_view Vertical field of view, in radians.
	/// \param aspect_ratio Width-to-height aspect ratio.
	Frustum ComputeProjectiveViewFrustum(const Affine3f& camera_transform, float near_distance, float far_distance, float field_of_view, float aspect_ratio){


		// See: http://cgvr.cs.uni-bremen.de/teaching/cg_literatur/lighthouse3d_view_frustum_culling/

		auto& camera_matrix = camera_transform.matrix();

		// Camera position

		auto camera_position = Math::ToVector3(camera_matrix.col(3));					// Position of the camera in world space.

		// Camera directions

		auto right_vector = Math::ToVector3(camera_matrix.col(0)).normalized();			// Right vector, from camera's point of view.

		auto up_vector = Math::ToVector3(camera_matrix.col(1)).normalized();			// Up vector, from camera's point of view.

		auto forward_vector = Math::ToVector3(camera_matrix.col(2)).normalized();		// Forward vector, from camera's point of view.

		// Half dimensions

		float half_height = near_distance * std::tanf(field_of_view * 0.5f);

		Vector2f near_half_dim(half_height * aspect_ratio,								// Half dimensions of the near clipping plane 
							   half_height);
		// Center points

		Vector3f near_center = forward_vector * near_distance;							// Center point of the near clipping plane (Calculated as the camera was @ 0;0;0).

		Vector3f far_center = forward_vector * far_distance;							// Center point of the far clipping plane (calculated as the camera was @ 0;0;0).

		// Create the frustum
		return Frustum({ Math::MakePlane( forward_vector, near_center + camera_position),														// Near clipping plane
						 Math::MakePlane(-forward_vector, far_center + camera_position),														// Far clipping plane
						 Math::MakePlane(-up_vector.cross((near_center + right_vector * near_half_dim(0)).normalized()), camera_position),		// Right clipping plane. The result of the cross product is normalized since the two operands are orthonormal.
						 Math::MakePlane( up_vector.cross((near_center - right_vector * near_half_dim(0)).normalized()), camera_position),		// Left clipping plane
						 Math::MakePlane( right_vector.cross((near_center + up_vector * near_half_dim(1)).normalized()), camera_position),		// Top clipping plane
						 Math::MakePlane(-right_vector.cross((near_center - up_vector * near_half_dim(1)).normalized()), camera_position) });	// Bottom clipping plane

	}

	/// \brief Calculate the view frustum from a orthographic camera description.
	/// \param camera_transform Camera transform matrix in world space.
	/// \param near_distance Minimum projected distance in world units.
	/// \param far_distance Maximum projected distance in world units.
	/// \param ortho_size Vertical orthographic size in radians.
	/// \param aspect_ratio Width-to-height aspect ratio.
	Frustum ComputeOrthographicViewFrustum(const Affine3f& camera_transform, float near_distance, float far_distance, float ortho_size, float aspect_ratio) {

		auto& camera_matrix = camera_transform.matrix();

		// Camera position

		auto camera_position = Math::ToVector3(camera_matrix.col(3));					// Position of the camera in world space.

		// Camera directions

		auto right_vector = Math::ToVector3(camera_matrix.col(0)).normalized();			// Right vector, from camera's point of view.

		auto up_vector = Math::ToVector3(camera_matrix.col(1)).normalized();			// Up vector, from camera's point of view.

		auto forward_vector = Math::ToVector3(camera_matrix.col(2)).normalized();		// Forward vector, from camera's point of view.

		// Half dimensions

		Vector2f half_dim(ortho_size * aspect_ratio,
						  ortho_size);

		// Create the frustum
		return Frustum({ Math::MakePlane( forward_vector, camera_position + forward_vector * near_distance),		// Near clipping plane
						 Math::MakePlane(-forward_vector, camera_position + forward_vector * far_distance),			// Far clipping plane
						 Math::MakePlane(-right_vector, camera_position + right_vector * half_dim(0)),				// Right clipping plane
						 Math::MakePlane( right_vector, camera_position - right_vector * half_dim(0)),				// Left clipping plane
						 Math::MakePlane(-up_vector, camera_position + up_vector * half_dim(1)),					// Top clipping plane
						 Math::MakePlane( up_vector, camera_position - up_vector * half_dim(1)) });					// Bottom clipping plane
		
	}

}

////////////////////////////////////// SCENE //////////////////////////////////////////

Scene::Scene(unique_ptr<IVolumeHierarchy> mesh_hierarchy, unique_ptr<IVolumeHierarchy> light_hierarchy) :
main_camera_(nullptr),
mesh_hierarchy_(std::move(mesh_hierarchy)),
light_hierarchy_(std::move(light_hierarchy)){}

Scene::~Scene(){

	for (auto&& node : nodes_){

		node->Dispose();

	}

	nodes_.clear();

	mesh_hierarchy_ = nullptr;		// 
	
}

NodeComponent* Scene::CreateNode(const wstring& name){

	auto node = Component::Create<NodeComponent>(*this, name);

	nodes_.push_back(node);

	return node;

}

TransformComponent* Scene::CreateNode(const wstring& name, const Translation3f& translation, const Quaternionf& rotation, const AlignedScaling3f& scale){

	auto node = Component::Create<NodeComponent>(*this, name);

	auto transform = node->AddComponent<TransformComponent>(translation, rotation, scale);

	// Node and transform are the same entity. When node is deleted, transform is deleted as well.
	nodes_.push_back(node);	

	return transform;

}

CameraComponent* Scene::GetMainCamera(){

	return main_camera_;

}

const CameraComponent* Scene::GetMainCamera() const{

	return main_camera_;

}

void Scene::SetMainCamera(CameraComponent* main_camera){

	main_camera_ = main_camera;

}

IVolumeHierarchy& Scene::GetMeshHierarchy(){

	return *mesh_hierarchy_;

}

const IVolumeHierarchy& Scene::GetMeshHierarchy() const{

	return *mesh_hierarchy_;

}

IVolumeHierarchy& Scene::GetLightHierarchy() {
	
	return *light_hierarchy_;

}

const IVolumeHierarchy& Scene::GetLightHierarchy() const {

	return *light_hierarchy_;

}

////////////////////////////////////// NODE COMPONENT /////////////////////////////////////

NodeComponent::NodeComponent(Scene& scene, const wstring& name) :
scene_(scene),
name_(name),
uid_(Unique<NodeComponent>::MakeUnique()){}

NodeComponent::~NodeComponent(){}

Scene& NodeComponent::GetScene(){

	return scene_;

}

const Scene& NodeComponent::GetScene() const{

	return scene_;

}

const wstring& NodeComponent::GetName() const{


	return name_;

}

const Unique<NodeComponent> NodeComponent::GetUid() const{

	return uid_;

}

NodeComponent::TypeSet NodeComponent::GetTypes() const{

	auto types = Component::GetTypes();

	types.insert(type_index(typeid(NodeComponent)));

	return types;

}

void NodeComponent::Initialize(){}

void NodeComponent::Finalize(){}

////////////////////////////////////// TRANSFORM COMPONENT /////////////////////////////////////

TransformComponent::TransformComponent() :
TransformComponent(Translation3f(Vector3f::Zero()),
				   Quaternionf::Identity(),
				   AlignedScaling3f(Vector3f::Ones())){}

TransformComponent::TransformComponent(const Translation3f& translation, const Quaternionf& rotation, const AlignedScaling3f& scale) :
parent_(nullptr),
translation_(translation),
rotation_(rotation),
scale_(scale),
local_dirty_(true),
world_dirty_(true){}

const Translation3f & TransformComponent::GetTranslation() const{

	return translation_;

}

void TransformComponent::SetTranslation(const Translation3f & translation){

	translation_ = translation;

	SetDirty(false);	// World and local

}

const Quaternionf & TransformComponent::GetRotation() const{

	return rotation_;

}

void TransformComponent::SetRotation(const Quaternionf & rotation){

	rotation_ = rotation;

	SetDirty(false);	// World and local

}

const AlignedScaling3f & TransformComponent::GetScale() const{

	return scale_;

}

void TransformComponent::SetScale(const AlignedScaling3f & scale){

	scale_ = scale;

	SetDirty(false);	// World and local

}

const Affine3f & TransformComponent::GetLocalTransform() const{

	if (local_dirty_){

		local_transform_ = translation_ * rotation_ * scale_;

		local_dirty_ = false;

	}

	return local_transform_;

}

const Affine3f & TransformComponent::GetWorldTransform() const{

	if (world_dirty_){

		auto local_transform = GetLocalTransform();

		world_transform_ = parent_ ?
						   parent_->GetWorldTransform() * local_transform :
						   local_transform;

		world_dirty_ = false;

	}

	return world_transform_;

}

TransformComponent* TransformComponent::GetParent(){

	return parent_;

}

const TransformComponent* TransformComponent::GetParent() const{

	return parent_;

}

void TransformComponent::SetParent(TransformComponent* parent){

	// Remove from the old parent
	if (parent_ != nullptr){

		auto& parent_children = parent->children_;

		parent_children.erase(std::remove(parent_children.begin(),
										  parent_children.end(),
										  this),
							  parent_children.end());

	}

	// Add to the new one
	parent_ = parent;

	if (parent != nullptr){

		parent->children_.push_back(this);

	}

	// The world transform is now invalid
	SetDirty(true);

}

TransformComponent::range TransformComponent::GetChildren(){

	return range(children_.begin(),
				 children_.end());

}

TransformComponent::const_range TransformComponent::GetChildren() const{

	return const_range(children_.cbegin(),
					   children_.cend());

}

TransformComponent::TypeSet TransformComponent::GetTypes() const{

	auto types = Component::GetTypes();

	types.insert(type_index(typeid(TransformComponent)));

	return types;

}

void TransformComponent::Initialize(){}

void TransformComponent::Finalize(){}

void TransformComponent::SetDirty(bool world_only){

	local_dirty_ |= !world_only;

	world_dirty_ = true;

	OnTransformChangedEventArgs args{ this };

	on_transform_changed_.Notify(args);

	// Invalidate the children recursively

	for (auto& child : children_){

		child->SetDirty(true);	// Children's matrix needs to be recalculated

	}

}

Observable<TransformComponent::OnTransformChangedEventArgs>& TransformComponent::OnTransformChanged(){

	return on_transform_changed_;

}

///////////////////////// VOLUME COMPONENT /////////////////////////

VolumeComponent::TypeSet VolumeComponent::GetTypes() const {

	auto types = Component::GetTypes();

	types.insert(type_index(typeid(VolumeComponent)));

	return types;

}

////////////////////////////////////// MESH COMPONENT /////////////////////////////////////

MeshComponent::MeshComponent() :
mesh_(nullptr){}

MeshComponent::MeshComponent(ObjectPtr<IStaticMesh> mesh) :
bounding_box_(mesh->GetBoundingBox()),
mesh_(mesh){}

ObjectPtr<IStaticMesh> MeshComponent::GetMesh(){

	return mesh_;

}

ObjectPtr<const IStaticMesh> MeshComponent::GetMesh() const{

	return ObjectPtr<const IStaticMesh>(mesh_);

}

void MeshComponent::SetMesh(ObjectPtr<IStaticMesh> mesh){

	mesh_ = mesh;

	ComputeBounds();

}

IntersectionType MeshComponent::TestAgainst(const Frustum& frustum) const{

	return frustum.Intersect(bounding_sphere_);

}

IntersectionType MeshComponent::TestAgainst(const AABB& box) const{

	return transformed_bounds_.Intersect(box);

}

IntersectionType MeshComponent::TestAgainst(const Sphere& sphere) const{

	return bounding_sphere_.Intersect(sphere);

}

MeshComponent::TypeSet MeshComponent::GetTypes() const{
	
	auto types = VolumeComponent::GetTypes();

	types.insert(type_index(typeid(MeshComponent)));

	return types;

}

void MeshComponent::Initialize(){

	// Listen to the transform component
	 
	transform_ = GetComponent<TransformComponent>();
	 
	ComputeBounds(false);

	on_transform_changed_lister_ = transform_->OnTransformChanged().Subscribe([this](_, _) {
	 
	 	ComputeBounds();	// The world matrix of the transform component changed.
	 
	});

	// Plug the mesh inside the mesh hierarchy

	GetComponent<NodeComponent>()->GetScene()
								 .GetMeshHierarchy()
								 .AddVolume(this);

}

void MeshComponent::Finalize(){

	// Remove the volume from the mesh hierarchy

	GetComponent<NodeComponent>()->GetScene()
								 .GetMeshHierarchy()
								 .RemoveVolume(this);

}

void MeshComponent::ComputeBounds(bool notify) {

	transformed_bounds_ = bounding_box_ * transform_->GetWorldTransform();
	bounding_sphere_ = Sphere::FromAABB(transformed_bounds_);
	
	if (notify) {

		NotifyChange();

	}

}

////////////////////////////////////// CAMERA COMPONENT /////////////////////////////////////

CameraComponent::CameraComponent(){

	projection_type_ = ProjectionType::Perspective;
	field_of_view_ = Math::DegToRad(45);
	minimum_distance_ = 1.0f;
	maximum_distance_ = 10000.f;
	transform_component_ = nullptr;
	transform_ = Affine3f::Identity();

}

CameraComponent::~CameraComponent(){}

ProjectionType CameraComponent::GetProjectionType() const{

	return projection_type_;

}

void CameraComponent::SetProjectionType(ProjectionType projection_type){

	projection_type_ = projection_type;

}

float CameraComponent::GetFieldOfView() const{

	return field_of_view_;

}

void CameraComponent::SetFieldOfView(float field_of_view){

	field_of_view_ = field_of_view;

}

float CameraComponent::GetOrthoSize() const {

	return ortho_size_;

}

void CameraComponent::SetOrthoSize(float ortho_size) {

	ortho_size_ = ortho_size;

}

float CameraComponent::GetMinimumDistance() const{

	return minimum_distance_;

}

void CameraComponent::SetMinimumDistance(float minimum_distance){

	minimum_distance_ = minimum_distance;

}

float CameraComponent::GetMaximumDistance() const{

	return maximum_distance_;

}

void CameraComponent::SetMaximumDistance(float maximum_distance){

	maximum_distance_ = maximum_distance;

}

Frustum CameraComponent::GetViewFrustum(float aspect_ratio) const{

	if (projection_type_ == ProjectionType::Perspective){

		return ComputeProjectiveViewFrustum(GetWorldTransform(),
											minimum_distance_,
											maximum_distance_,
											field_of_view_,
											aspect_ratio);

	}
	else if(projection_type_ == ProjectionType::Ortographic){

		return ComputeOrthographicViewFrustum(GetWorldTransform(),
											  minimum_distance_,
											  maximum_distance_,
											  ortho_size_,
											  aspect_ratio);
		
	}
	else {

		THROW(L"Unsupported projection mode!");

	}
	
}

CameraComponent::TypeSet CameraComponent::GetTypes() const{

	auto types = Component::GetTypes();

	types.insert(type_index(typeid(CameraComponent)));

	return types;

}

void CameraComponent::Initialize(){

	// Initialize the transform component

	transform_component_ = GetComponent<TransformComponent>();
	
}

void CameraComponent::Finalize(){}

void CameraComponent::Clone(CameraComponent& target) const{

	target.projection_type_ = projection_type_;
	target.field_of_view_ = field_of_view_;
	target.minimum_distance_ = minimum_distance_;
	target.maximum_distance_ = maximum_distance_;
	target.transform_component_ = nullptr;
	target.transform_ = GetWorldTransform();
	
}