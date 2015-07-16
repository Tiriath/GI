#include "dx11/dx11shader_state.h"

#include "dx11/dx11buffer.h"
#include "dx11/dx11texture.h"

using namespace gi_lib;
using namespace gi_lib::dx11;

namespace{

	/// \brief Reflect a collection of shader members into an unordered multimap.
	/// \param shader_state Shader state associated to the setters.
	/// \param collection Collection of shader members to enumerate.
	/// \param table Destination unordered map containing the setters.
	template <typename TSetter, typename TCollection>
	void AddShaderBindings(BaseShaderState& shader_state, const TCollection& collection, std::unordered_multimap<size_t, TSetter>& table){

		for (auto&& item : collection){

			table.insert(std::make_pair(Tag(item.name),
										TSetter(shader_state,
												item.slot)));
			
		}

	}

	/// \brief Calls the setter functions of all the shader members with a particular name.
	/// \param tag Tag associated to the shader members to set.
	/// \param value Actual value to pass to the setter.
	/// \param table Map containing the definition of the shader setters.
	template <typename TSetter, typename TValue>
	bool SetShaderMember(const Tag& tag, TValue&& value, std::unordered_multimap < size_t, TSetter >& table){

		auto&& range = table.equal_range(tag);

		auto it = range.first;

		while (it != range.second){

			it->second(std::forward<TValue>(value));

			++it;

		}

		return range.first != range.second;

	}

}

//////////////////////////////// SHADER STATE COMPOSITE //////////////////////////////////////

ShaderStateComposite::ShaderStateComposite(){}

ShaderStateComposite::ShaderStateComposite(const ShaderStateComposite& other){

	shaders_.reserve(other.shaders_.size());

	for (auto&& shader : other.shaders_){

		shaders_.push_back(std::unique_ptr<BaseShaderState>(shader->Instantiate()));

		AddShaderBindings(*shader);

	}

}

bool ShaderStateComposite::SetConstantBuffer(const Tag& tag, const ObjectPtr<IStructuredBuffer>& constant_buffer){

	return SetShaderMember(tag,
						   resource_cast(constant_buffer)->GetConstantBuffer(),
						   cbuffer_table_);

}

bool ShaderStateComposite::SetShaderResource(const Tag& tag, const ObjectPtr<ITexture2D>& texture_2D){

	return SetShaderMember(tag,
						   resource_cast(texture_2D)->GetShaderResourceView(),
						   srv_table_);

}

bool ShaderStateComposite::SetShaderResource(const Tag& tag, const ObjectPtr<IStructuredArray>& structured_array){

	return SetShaderMember(tag,
						   resource_cast(structured_array)->GetShaderResourceView(),
						   srv_table_);

}

bool ShaderStateComposite::SetUnorderedAccess(const Tag& tag, const ObjectPtr<IGPTexture2D>& gp_texture_2D){

	return SetShaderMember(tag,
						   resource_cast(gp_texture_2D)->GetUnorderedAccessView(),
						   uav_table_);

}

void ShaderStateComposite::AddShaderBindings(BaseShaderState& shader_state){

	auto&& reflection = shader_state.GetReflection();

	::AddShaderBindings<CBufferSetter>(shader_state,
									   reflection.buffers,
									   cbuffer_table_);

	::AddShaderBindings<SRVSetter>(shader_state,
								   reflection.shader_resource_views,
								   srv_table_);

	::AddShaderBindings<UAVSetter>(shader_state,
								   reflection.unordered_access_views,
								   uav_table_);

	::AddShaderBindings<SamplerSetter>(shader_state,
									   reflection.samplers,
									   sampler_table_);

}