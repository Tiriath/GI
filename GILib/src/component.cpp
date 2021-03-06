#include "component.h"

#include <algorithm>

#include "exceptions.h"

using namespace ::gi_lib;
using namespace ::std;

namespace{

	/// \brief Unmaps a component from a given multimap.
	/// \param component Component to remove.
	/// \param map Map where the component should be unmapped from.
	void UnmapInterface(Component* component, Component::ComponentMap& map){

		// O(#types * #interfaces_per_type)

		for (auto type : component->GetTypes()){

			auto range = map.equal_range(type);

			while (range.first != range.second){

				if (range.first->second == component){

					map.erase(range.first);		

					break; // At most once per type

				}
				else{

					++(range.first);

				}

			}

		}

	}

}

/////////////////////////////// COMPONENT :: ARBITER //////////////////////////////

/// \brief Enables intra-component communications.
class Component::Arbiter{

public:

	using ComponentSet = set < Component* >;

	using ComponentMap = unordered_multimap < type_index, Component* >;

	using range = Component::map_range;

	Arbiter();

	~Arbiter();

	void AddComponent(Component* component);

	void RemoveComponent(Component* component);

	void RemoveAll();

	Component* GetComponent(type_index type);

	range GetComponents(type_index type);

private:

	void DeleteComponent(ComponentSet::iterator it);

	void FinalizeComponent(Component& component);

	ComponentSet component_set_;

	ComponentMap component_map_;

	bool autodestroy_;

};

Component::Arbiter::Arbiter() :
autodestroy_(true){}

Component::Arbiter::~Arbiter(){

	autodestroy_ = false;		// This ensures that this destructor is called exactly once

	// Finalize all the component together

	for (auto&& component : component_set_) {

		FinalizeComponent(*component);				// TODO: break if a component is added or removed while the object is being destroyed!

	}

	// Destroy each component independently

	while (!component_set_.empty()){

		DeleteComponent(component_set_.begin());	// Will cause the component set to shrink

	}

}

void Component::Arbiter::AddComponent(Component* component){

	component_set_.insert(component);

	// Map each component type - O(#types)

	for (auto& type : component->GetTypes()){

		component_map_.insert(ComponentMap::value_type(type, component));

	}

	component->arbiter_ = this;

	// The initialization must occur after the registration because if Component::Initialize removes the last interface, 
	// the arbiter would be destroyed erroneously.

	component->Initialize();		// Cross-component construction

}

void Component::Arbiter::RemoveComponent(Component* component){

	auto it = component_set_.find(component);
	
	if (it != component_set_.end()){

		FinalizeComponent(**it);
		DeleteComponent(it);

		if (autodestroy_ &&
			component_set_.empty()){

			delete this;	// Autodestruction

		}

	}
	
}

Component* Component::Arbiter::GetComponent(type_index type){

	auto it = component_map_.find(type);

	return it != component_map_.end() ?
		   it->second :
		   nullptr;

}

Component::Arbiter::range Component::Arbiter::GetComponents(type_index type){

	return range(component_map_.equal_range(type));

}

void Component::Arbiter::FinalizeComponent(Component& component) {

	// Notify the remove event before.

	OnRemovedEventArgs args{ &component };

	component.on_removed_event_.Notify(args);

	// Actual deletion

	component.Finalize();							// Cross-component destruction

}

void Component::Arbiter::DeleteComponent(ComponentSet::iterator it){

	auto component = *it;

	// Actual deletion

	component->arbiter_ = nullptr;					// No really needed, ensures that the dtor of the component won't access the other components.

	component_set_.erase(it);

	UnmapInterface(component, component_map_);

	delete component;								// Independent destruction

}


/////////////////////////////// COMPONENT /////////////////////////////////////////

Component::Component() :
arbiter_(nullptr){}

Component::~Component(){}

void Component::RemoveComponent(){

	arbiter_->RemoveComponent(this);

}

void Component::Dispose(){

	delete arbiter_;

}

Observable<Component::OnRemovedEventArgs>& Component::OnRemoved(){

	return on_removed_event_;

}

Component::TypeSet Component::GetTypes() const{

	set<type_index> types;

	types.insert(type_index(typeid(Component)));

	return types;

}

Component* Component::GetComponent(type_index type) const{

	return arbiter_->GetComponent(type);

}

Component::map_range Component::GetComponents(type_index type) const{

	return arbiter_->GetComponents(type);

}

void Component::Setup(Arbiter* arbiter){

	arbiter->AddComponent(this);

}

void Component::Setup(){

	Setup(new Arbiter());

}