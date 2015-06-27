#include "dx11/dx11material.h"

#include <unordered_map>

#include "dx11/dx11resources.h"
#include "dx11/dx11graphics.h"
#include "dx11/dx11sampler.h"

using namespace ::std;
using namespace ::gi_lib;
using namespace ::dx11;
using namespace ::windows;

namespace{

	/////////////////////////// MATERIAL ////////////////////////////////

	/// \brief Bundle of shader resources that will be bound to the pipeline.
	struct ShaderBundle{

		vector<ID3D11Buffer*> buffers;						/// \brief Buffer binding.

		vector<ID3D11ShaderResourceView*> resources;		/// \brief Resource binding.

		vector<ID3D11SamplerState*> samplers;				/// \brief Sampler binding.

		vector<ID3D11UnorderedAccessView*> unordered;		/// \brief UAV binding.

		/// \brief Default constructor;
		ShaderBundle();

		/// \brief Move constructor.
		ShaderBundle(ShaderBundle&& other);

	};

	/// \brief Describes the current status of a buffer.
	class BufferStatus{

	public:

		/// \brief Create a new buffer status.
		/// \param device Device used to create the constant buffer.
		/// \param size Size of the buffer to create.
		BufferStatus(ID3D11Device& device, size_t size);

		/// \brief Copy constructor
		/// \param Instance to copy.
		BufferStatus(const BufferStatus& other);

		/// \brief Move constructor.
		/// \param Instance to move.
		BufferStatus(BufferStatus&& other);

		/// \brief No assignment operator.
		BufferStatus& operator=(const BufferStatus&) = delete;

		/// \brief Destructor.
		~BufferStatus();

		/// \brief Write inside the constant buffer.
		/// \param source Buffer to read from.
		/// \param size Size of the buffer to read in bytes.
		/// \param offset Offset from the beginning of the constant buffer in bytes.
		void Write(const void * source, size_t size, size_t offset);

		/// \brief Write a value inside the constant buffer
		/// \param value The value to write.
		/// \param offset Offset from the beginning of the constant buffer in bytes.
		/// \remarks This method should have better performances while working with smaller values.
		template <typename TType>
		void Write(const TType& value, size_t offset);

		/// \brief Commit any uncommitted change to the constant buffer.
		void Commit(ID3D11DeviceContext& context);

		/// \brief Get the constant buffer.
		/// \return Return the constant buffer.
		ID3D11Buffer& GetBuffer();

		/// \brief Get the constant buffer.
		/// \return Return the constant buffer.
		const ID3D11Buffer& GetBuffer() const;

	private:

		unique_ptr<ID3D11Buffer, COMDeleter> buffer_;		/// \brief Constant buffer to bound to the graphic pipeline.

		void * data_;										/// \brief Buffer containing the data to send to the constant buffer.

		bool dirty_;										/// \brief Whether the constant buffer should be updated.

		size_t size_;										/// \brief Size of the buffer in bytes.

	};

	/////////////////////////// SHADER BUNDLE ///////////////////////////

	ShaderBundle::ShaderBundle(){}

	ShaderBundle::ShaderBundle(ShaderBundle&& other){

		buffers = std::move(other.buffers);
		resources = std::move(other.resources);
		samplers = std::move(other.samplers);
		unordered = std::move(other.unordered);

	}

	/////////////////////////// BUFFER STATUS ///////////////////////////

	BufferStatus::BufferStatus(ID3D11Device& device, size_t size){

		ID3D11Buffer * buffer;

		THROW_ON_FAIL(MakeConstantBuffer(device,
										 size,
										 &buffer));

		buffer_.reset(buffer);

		data_ = new char[size];

		size_ = size;

		dirty_ = false;

	}

	BufferStatus::BufferStatus(const BufferStatus& other){

		ID3D11Device* device;
		ID3D11Buffer * buffer;

		other.buffer_->GetDevice(&device);

		COM_GUARD(device);

		THROW_ON_FAIL(MakeConstantBuffer(*device,
										 other.size_,
										 &buffer));

		buffer_.reset(buffer);

		data_ = new char[other.size_];

		size_ = other.size_;

		dirty_ = true;			// lazily update the constant buffer.

		memcpy_s(data_,			// copy the current content of the buffer.
				 size_, 
				 other.data_, 
				 other.size_);

	}

	BufferStatus::BufferStatus(BufferStatus&& other){

		buffer_ = std::move(other.buffer_);

		data_ = other.data_;
		other.data_ = nullptr;

		size_ = other.size_;

		dirty_ = other.dirty_;

	}

	BufferStatus::~BufferStatus(){

		if (data_){

			delete[] data_;

		}

	}

	inline void BufferStatus::Write(const void * source, size_t size, size_t offset){

		memcpy_s(static_cast<char*>(data_) + offset,
				 size_ - offset,
				 source,
				 size);

		dirty_ = true;

	}

	template <typename TType>
	inline void BufferStatus::Write(const TType& value, size_t offset){

		*reinterpret_cast<TType*>((static_cast<char*>(data_)+offset)) = value;

		dirty_ = true;

	}

	void BufferStatus::Commit(ID3D11DeviceContext& context){

		if (dirty_){

			D3D11_MAPPED_SUBRESOURCE mapped_buffer;

			context.Map(buffer_.get(),			
						0,
						D3D11_MAP_WRITE_DISCARD,		// Discard the previous buffer
						0,					
						&mapped_buffer);

			memcpy_s(mapped_buffer.pData,
					 size_,
					 data_,
					 size_);
			
			context.Unmap(buffer_.get(),
						  0);

			dirty_ = false;

		}

	}

	ID3D11Buffer& BufferStatus::GetBuffer(){

		return *buffer_;

	}

	const ID3D11Buffer& BufferStatus::GetBuffer() const{

		return *buffer_;

	}

	/////////////////////////// MISC ////////////////////////////////////

	/// \brief Convert a shader type to a numeric index.
	/// \return Returns a number from 0 to 4 if the specified type was referring to a single shader type, returns -1 otherwise.
	size_t ShaderTypeToIndex(ShaderType shader_type){

		// Sorted from the most common to the least common

		switch (shader_type){

		case ShaderType::VERTEX_SHADER:		

			return 0;

		case ShaderType::PIXEL_SHADER:

			return 4;

		case ShaderType::GEOMETRY_SHADER:

			return 3;

		case ShaderType::HULL_SHADER:

			return 1;

		case ShaderType::DOMAIN_SHADER:

			return 2;

		default:

			return static_cast<size_t>(-1);

		}

	}

	/// \brief Convert a numeric index to a shader type.
	/// \return Returns the shader type associated to the given numeric index.
	ShaderType IndexToShaderType(size_t index){

		switch (index){

		case 0:
		
			return ShaderType::VERTEX_SHADER;

		case 4:

			return ShaderType::PIXEL_SHADER;

		case 3:

			return ShaderType::GEOMETRY_SHADER;
		
		case 1:

			return ShaderType::HULL_SHADER;

		case 2:
			
			return ShaderType::DOMAIN_SHADER;

		default:

			return ShaderType::NONE;

		}

	}

	////////////////////////// DIRECTX 11 ///////////////////////////////

	/// \brief Load a shader from code and return a pointer to the shader object.
	/// \param device Device used to load the shader.
	/// \param code HLSL code to compile.
	/// \param file_name Name of the file used to resolve the #include directives.
	/// \param mandatory Whether the compile success if compulsory or not.
	/// \return Returns a pointer to the shader object if the method succeeded.
	///			If the entry point couldn't be found the method will throw if the mandatory flag was true, or will return nullptr otherwise.
	template <typename TShader>
	unique_ptr<TShader, COMDeleter> MakeShader(ID3D11Device& device, const string& code, const string& file_name, bool mandatory, ShaderReflection& reflection){

		const wstring entry_point_exception_code = L"X3501";	// Exception code thrown when the entry point could not be found (ie: a shader doesn't exists).

		TShader* shader;

		wstring errors;

		if (FAILED(::MakeShader(device,
								code,
								file_name,
								&shader,
								&reflection,
								&errors))){

			// Throws only if the compilation process fails when the entry point is found (on syntax error). 
			// If the entry point is not found, throws only if the shader presence was mandatory.

			if (mandatory ||
				errors.find(entry_point_exception_code) == wstring::npos){

				THROW(errors);

			}
			else{

				return nullptr;

			}

		}
		else{

			return make_unique_com(shader);

		}

	}

	/// \brief Bind a shader along with its resources to a device context.
	/// \param context The context where the shader will be bound.
	/// \param shader Pointer to the shader. Can be nullptr to disable a pipeline stage.
	/// \param bundle Bundle of resources associated to the shader.
	template <typename TShader, typename TDeleter>
	void BindShader(ID3D11DeviceContext& context, const unique_ptr<TShader, TDeleter>& shader_ptr, const ShaderBundle& bundle){

		TShader* shader = shader_ptr ?
						  shader_ptr.get() :
						  nullptr;
		// Shader

		SetShader(context,
				  shader);

		// Constant buffers
				
		SetConstantBuffers<TShader>(context,
									0,
									bundle.buffers.size() > 0 ? &bundle.buffers[0] : nullptr,
									bundle.buffers.size());

		// Resources

		SetShaderResources<TShader>(context,
									0,
									bundle.resources.size() > 0 ? &bundle.resources[0] : nullptr,
									bundle.resources.size());

		// Samplers

		SetShaderSamplers<TShader>(context,
								   0,
								   bundle.samplers.size() > 0 ? &bundle.samplers[0] : nullptr,
								   bundle.samplers.size());

		// Unordered access views
		SetShaderUAV<TShader>(context,
							  0,
							  bundle.unordered.size() > 0 ? &bundle.unordered[0] : nullptr,
							  bundle.unordered.size());

	}
		
}

////////////////////////////// MATERIAL //////////////////////////////////////////////

/// \brief Private implementation of DX11Material.
struct DX11Material::InstanceImpl{

	InstanceImpl(ID3D11Device& device, const ShaderReflection& reflection);

	InstanceImpl(const InstanceImpl& impl);

	/// \brief No assignment operator.
	InstanceImpl& operator=(const InstanceImpl&) = delete;
	
	void SetVariable(size_t index, const void * data, size_t size, size_t offset);

	template<typename TType>
	void SetVariable(size_t index, const TType& value, size_t offset);

	void SetResource(size_t index, ObjectPtr<DX11ResourceView> resource);
	
	void SetUAV(size_t index, ObjectPtr<DX11ResourceView> resource);

	/// \brief Write any uncommitted change to the constant buffers.
	void Commit(ID3D11DeviceContext& context);

	unordered_map<ShaderType, ShaderBundle> shader_bundles;				/// <\brief Shader bundles for each shader.

private:

	void AddShaderBundle(ShaderType shader_type);

	void CommitResources();

	void CommitUAVs();

	vector<BufferStatus> buffer_status_;								///< \brief Status of constant buffers.

	vector<ObjectPtr<DX11ResourceView>> resources_;						///< \brief Status of bound resources.

	ObjectPtr<DX11Sampler> sampler_;									///< \brief Sampler.

	vector<ObjectPtr<DX11ResourceView>> UAVs_;							///< \brief Status of bound UAVs.

	ShaderType resource_dirty_mask_;									///< \brief Dirty mask used to determine which bundle needs to be updated (shader resource view).

	ShaderType UAV_dirty_mask_;											///< \brief Dirty mask used to determine which bundle needs to be updated (unordered access view).

	const ShaderReflection& reflection_;
		
};

/// \brief Shared implementation of DX11Material.
struct DX11Material::MaterialImpl{

	MaterialImpl(ID3D11Device& device, const CompileFromFile& bundle);

	ShaderReflection reflection;													/// \brief Combined reflection of the shaders.

	unique_ptr<ID3D11VertexShader, COMDeleter> vertex_shader;						/// \brief Pointer to the vertex shader.

	unique_ptr<ID3D11HullShader, COMDeleter> hull_shader;							/// \brief Pointer to the hull shader.

	unique_ptr<ID3D11DomainShader, COMDeleter> domain_shader;						/// \brief Pointer to the domain shader.

	unique_ptr<ID3D11GeometryShader, COMDeleter> geometry_shader;					/// \brief Pointer to the geometry shader.

	unique_ptr<ID3D11PixelShader, COMDeleter> pixel_shader;							/// \brief Pointer to the pixel shader.
	
	unique_ptr<ID3D11InputLayout, COMDeleter> input_layout;							/// \brief Vertices input layout. (Associated to the material, sigh)

};

//////////////////////////////  MATERIAL :: INSTANCE IMPL //////////////////////////////

DX11Material::InstanceImpl::InstanceImpl(ID3D11Device& device, const ShaderReflection& reflection) :
reflection_(reflection){

	// Buffer status

	for (auto& buffer : reflection.buffers){

		buffer_status_.push_back(BufferStatus(device, buffer.size));

	}

	// Resource status (empty)

	resources_.resize(reflection.resources.size());

	// Sampler state (the same)

	Resources& resources = DX11Graphics::GetInstance().GetResources();

	sampler_ = resources.Load<DX11Sampler, DX11Sampler::FromDescription>({ TextureMapping::WRAP, 16 });
	
	// Shader bundles

	AddShaderBundle(ShaderType::VERTEX_SHADER);
	AddShaderBundle(ShaderType::HULL_SHADER);
	AddShaderBundle(ShaderType::DOMAIN_SHADER);
	AddShaderBundle(ShaderType::GEOMETRY_SHADER);
	AddShaderBundle(ShaderType::PIXEL_SHADER);

	resource_dirty_mask_ = ShaderType::NONE;
	UAV_dirty_mask_ = ShaderType::NONE;

}

DX11Material::InstanceImpl::InstanceImpl(const InstanceImpl& impl) :
buffer_status_(impl.buffer_status_),				// Copy ctor will copy the buffer status
resources_(impl.resources_),						// Same resources bound
sampler_(impl.sampler_),
resource_dirty_mask_(impl.reflection_.shaders),		// Used to lazily update the resources' shader views
UAV_dirty_mask_(impl.reflection_.shaders),			// Used to lazily update the resources' unordered access views.
reflection_(impl.reflection_){

	// Shader bundles

	AddShaderBundle(ShaderType::VERTEX_SHADER);
	AddShaderBundle(ShaderType::HULL_SHADER);
	AddShaderBundle(ShaderType::DOMAIN_SHADER);
	AddShaderBundle(ShaderType::GEOMETRY_SHADER);
	AddShaderBundle(ShaderType::PIXEL_SHADER);

}

void DX11Material::InstanceImpl::SetVariable(size_t index, const void * data, size_t size, size_t offset){

	buffer_status_[index].Write(data, 
								size, 
								offset);

}

template<typename TType>
void DX11Material::InstanceImpl::SetVariable(size_t index, const TType& value, size_t offset){

	buffer_status_[index].Write(value, 
								offset);

}

void DX11Material::InstanceImpl::SetResource(size_t index, ObjectPtr<DX11ResourceView> resource){

	resources_[index] = resource;

	resource_dirty_mask_ |= reflection_.resources[index].shader_usage;	// Let the bundle know that the resource status changed.

}

void DX11Material::InstanceImpl::SetUAV(size_t index, ObjectPtr<DX11ResourceView> resource){

	UAVs_[index] = resource;

	UAV_dirty_mask_ |= reflection_.unordered[index].shader_usage;	// Let the bundle know that the resource status changed.

}

void DX11Material::InstanceImpl::AddShaderBundle(ShaderType shader_type){

	if (reflection_.shaders && shader_type){
		
		ShaderBundle bundle;

		// Buffers, built once, updated automatically.
		for (size_t buffer_index = 0; buffer_index < buffer_status_.size(); ++buffer_index){

			if (reflection_.buffers[buffer_index].shader_usage && shader_type){

				bundle.buffers.push_back(&buffer_status_[buffer_index].GetBuffer());

			}

		}

		// Resources, built once, update by need (when the material is bound to the pipeline).
		bundle.resources.resize(std::count_if(reflection_.resources.begin(),
											  reflection_.resources.end(),
											  [shader_type](const ShaderResourceDesc& resource_desc){

													return resource_desc.shader_usage && shader_type;

											  }));

		// Sampler never change (unless the game options change at runtime)

		for (auto&& sampler : reflection_.samplers){

			if (sampler.shader_usage && shader_type){

				bundle.samplers.push_back(&(sampler_->GetSamplerState()));

			}

		}

		// UAV, build once, update by need (when the material is bound to the pipeline).
		bundle.unordered.resize(std::count_if(reflection_.unordered.begin(),
											  reflection_.unordered.end(),
											  [shader_type](const ShaderUnorderedDesc& UAV_desc){

													return UAV_desc.shader_usage && shader_type;

											  }));

		// Move it to the vector.

		shader_bundles[shader_type] = std::move(bundle);

	}

}

void DX11Material::InstanceImpl::CommitResources(){

	if (resource_dirty_mask_ == ShaderType::NONE){

		// Most common case: no resource was touched.

		return;

	}

	size_t resource_index;								// Index of a resource within the global resource array.
	size_t bind_point;									// Resource bind point relative to a particular shader.
	size_t index = 0;									// Shader index

	ObjectPtr<DX11ResourceView> resource_view;			// Current resource view

	for (auto&& bundle_entry : shader_bundles){		

		// Cycle through dirty bundles

		if (resource_dirty_mask_ && bundle_entry.first){

			auto& resource_views = bundle_entry.second.resources;

			// Cycle trough every resource. If any given resource is used by the current shader type, update the shader resource view vector's corresponding location.

			for (resource_index = 0, bind_point = 0; resource_index < reflection_.resources.size(); ++resource_index){

				if (reflection_.resources[resource_index].shader_usage && bundle_entry.first){

					resource_view = resources_[resource_index];

					resource_views[bind_point] = resource_view ?
												 resource_view->GetShaderView() :
												 nullptr;

					++bind_point;

				}

			}

		}

		++index;

	}

	resource_dirty_mask_ = ShaderType::NONE;

}

void DX11Material::InstanceImpl::CommitUAVs(){

	if (UAV_dirty_mask_ == ShaderType::NONE){

		// Most common case: no UAV was touched.

		return;

	}

	size_t UAV_index;									// Index of an UAV within the global UAV array.
	size_t bind_point;									// UAV bind point relative to a particular shader.
	size_t index = 0;									// Shader index

	ObjectPtr<DX11ResourceView> resource_view;			// Current resource view

	for (auto&& bundle_entry : shader_bundles){

		// Cycle through dirty bundles

		if (UAV_dirty_mask_ && bundle_entry.first){

			auto& UAVs = bundle_entry.second.unordered;

			// Cycle trough every unordered access view. If any given UAV is used by the current shader type, update the UAV vector's corresponding location.

			for (UAV_index = 0, bind_point = 0; UAV_index < reflection_.unordered.size(); ++UAV_index){

				if (reflection_.unordered[UAV_index].shader_usage && bundle_entry.first){

					resource_view = UAVs_[UAV_index];

					UAVs[bind_point] = resource_view ?
									   resource_view->GetUnorderedAccessView() :
									   nullptr;

					++bind_point;

				}

			}

		}

		++index;

	}

	UAV_dirty_mask_ = ShaderType::NONE;

}

void DX11Material::InstanceImpl::Commit(ID3D11DeviceContext& context){

	// Commit dirty constant buffers

	for (auto&& buffer : buffer_status_){

		buffer.Commit(context);
			
	}

	// Commit resource views

	CommitResources();

	// Commit unordered access views

	CommitUAVs();

}

//////////////////////////////  MATERIAL :: MATERIAL IMPL //////////////////////////////

DX11Material::MaterialImpl::MaterialImpl(ID3D11Device& device, const CompileFromFile& bundle){
	
	auto& file_system = FileSystem::GetInstance();

	string code = to_string(file_system.Read(bundle.file_name));

	string file_name = to_string(bundle.file_name);

	auto rollback = make_scope_guard([&](){

		vertex_shader = nullptr;
		hull_shader = nullptr;
		domain_shader = nullptr;
		geometry_shader = nullptr;
		pixel_shader = nullptr;

	});

	// Shaders

	reflection.shaders = ShaderType::NONE;

	vertex_shader = ::MakeShader<ID3D11VertexShader>(device, code, file_name, true, reflection);		// mandatory
	hull_shader = ::MakeShader<ID3D11HullShader>(device, code, file_name, false, reflection);			// optional
	domain_shader = ::MakeShader<ID3D11DomainShader>(device, code, file_name, false, reflection);		// optional
	geometry_shader = ::MakeShader<ID3D11GeometryShader>(device, code, file_name, false, reflection);	// optional
	pixel_shader = ::MakeShader<ID3D11PixelShader>(device, code, file_name, true, reflection);			// mandatory
				
	// Input layout

	ID3D11InputLayout* input_layout;

	// The bytecode is needed to validate the input layout. Genius idea... - TODO: Remove this from here

	ID3DBlob * bytecode;

	THROW_ON_FAIL(Compile<ID3D11VertexShader>(code,
											  file_name,
											  &bytecode,
											  nullptr));

	COM_GUARD(bytecode);

	D3D11_INPUT_ELEMENT_DESC input_elements[3];

	input_elements[0].SemanticName = "SV_Position";
	input_elements[0].SemanticIndex = 0;
	input_elements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	input_elements[0].InputSlot = 0;
	input_elements[0].AlignedByteOffset = 0;
	input_elements[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	input_elements[0].InstanceDataStepRate = 0;

	input_elements[1].SemanticName = "NORMAL";
	input_elements[1].SemanticIndex = 0;
	input_elements[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	input_elements[1].InputSlot = 0;
	input_elements[1].AlignedByteOffset = 12;
	input_elements[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	input_elements[1].InstanceDataStepRate = 0;

	input_elements[2].SemanticName = "TEXCOORD";
	input_elements[2].SemanticIndex = 0;
	input_elements[2].Format = DXGI_FORMAT_R32G32_FLOAT;
	input_elements[2].InputSlot = 0;
	input_elements[2].AlignedByteOffset = 24;
	input_elements[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	input_elements[2].InstanceDataStepRate = 0;

	THROW_ON_FAIL(device.CreateInputLayout(input_elements,
										   3,
										   bytecode->GetBufferPointer(),
										   bytecode->GetBufferSize(),
										   &input_layout));

	this->input_layout.reset(input_layout);

	// Dismiss
	rollback.Dismiss();

}

//////////////////////////////  MATERIAL :: VARIABLE //////////////////////////////

DX11MaterialVariable::DX11MaterialVariable(DX11Material::InstanceImpl& instance_impl, size_t buffer_index, size_t variable_size, size_t variable_offset) :
instance_impl_(&instance_impl),
buffer_index_(buffer_index),
variable_size_(variable_size),
variable_offset_(variable_offset){}

void DX11MaterialVariable::Set(const void * buffer, size_t size){

	if (size > variable_size_){

		THROW(L"Wrong variable size.");

	}

	instance_impl_->SetVariable(buffer_index_, 
								buffer, 
								size, 
								variable_offset_);

}

//////////////////////////////  MATERIAL :: RESOURCE //////////////////////////////

DX11MaterialResource::DX11MaterialResource(DX11Material::InstanceImpl& instance_impl, size_t resource_index) :
instance_impl_(&instance_impl),
resource_index_(resource_index){}

void DX11MaterialResource::Set(ObjectPtr<IResourceView> resource){

	instance_impl_->SetResource(resource_index_, 
								ObjectPtr<DX11ResourceView>(resource));

}

//////////////////////////////  MATERIAL //////////////////////////////

DX11Material::DX11Material(const CompileFromFile& args){

	auto& device = DX11Graphics::GetInstance().GetDevice();

	shared_impl_ = make_shared<MaterialImpl>(device, args);

	private_impl_ = make_unique<InstanceImpl>(device, shared_impl_->reflection);

}

DX11Material::DX11Material(const Instantiate& args){

	ObjectPtr<DX11Material> material = args.base;
	
	shared_impl_ = material->shared_impl_;

	private_impl_ = make_unique<InstanceImpl>(*material->private_impl_);
	
}

DX11Material::~DX11Material(){

	private_impl_ = nullptr;	// Must be destroyed before the shared implementation!
	shared_impl_ = nullptr;

}

ObjectPtr<IMaterialParameter> DX11Material::GetParameter(const string& name){

	auto& buffers = shared_impl_->reflection.buffers;

	size_t buffer_index = 0;

	// O(#total variables)

	for (auto& buffer : buffers){

		auto it = std::find_if(buffer.variables.begin(),
							   buffer.variables.end(),
							   [&name](const ShaderVariableDesc& desc){

									return desc.name == name;

							   });

		if (it != buffer.variables.end()){

			return new DX11MaterialVariable(*private_impl_,
											buffer_index,
											it->size,
											it->offset);

		}

		++buffer_index;

	}
		
	return nullptr;

}

ObjectPtr<IMaterialResource> DX11Material::GetResource(const string& name){

	// Check among the resources

	auto& resources = shared_impl_->reflection.resources;

	if (resources.size() > 0){
	
		// O(#total resources)

		auto it = std::find_if(resources.begin(),
							   resources.end(),
							   [&name](const ShaderResourceDesc& desc){

									return desc.name == name;

							   });

		if (it != resources.end()){

			return new DX11MaterialResource(*private_impl_,
											std::distance(resources.begin(),
														  it));
			
		}

	}

	// Not found
	return nullptr;

}

size_t DX11Material::GetSize() const{

	auto& buffers = shared_impl_->reflection.buffers;

	return std::accumulate(buffers.begin(),
						   buffers.end(),
						   static_cast<size_t>(0),
						   [](size_t size, const ShaderBufferDesc& desc){

								return size + desc.size;

						   });

}

void DX11Material::Commit(ID3D11DeviceContext& context){

	// Update the constant buffers

	private_impl_->Commit(context);

	// Set the vertex input layout - TODO: Move this from here to the mesh...

	context.IASetInputLayout(shared_impl_->input_layout.get());

	// Bind Every shader to the pipeline

	auto& bundles = private_impl_->shader_bundles;

	BindShader(context,
			   shared_impl_->vertex_shader,
			   bundles[ShaderType::VERTEX_SHADER]);

	BindShader(context,
			   shared_impl_->hull_shader,
			   bundles[ShaderType::HULL_SHADER]);

	BindShader(context,
			   shared_impl_->domain_shader,
			   bundles[ShaderType::DOMAIN_SHADER]);

	BindShader(context,
			   shared_impl_->geometry_shader,
			   bundles[ShaderType::GEOMETRY_SHADER]);

	BindShader(context,
			   shared_impl_->pixel_shader,
			   bundles[ShaderType::PIXEL_SHADER]);

}