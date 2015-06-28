

#include "dx11/dx11.h"

#include <sstream>
#include <algorithm>

#include "exceptions.h"
#include "enums.h"
#include "scope_guard.h"
#include "windows\win_os.h"

using namespace std;
using namespace gi_lib;
using namespace gi_lib::dx11;
using namespace gi_lib::windows;

namespace{

	D3D11_FILTER AnisotropyLevelToFilter(unsigned int anisotropy_level){

		return anisotropy_level > 0 ?
			   D3D11_FILTER_ANISOTROPIC :		// Anisotropic filtering
			   D3D11_FILTER_MIN_MAG_MIP_LINEAR;	// Trilinear filtering

	}

	D3D11_TEXTURE_ADDRESS_MODE TextureMappingToAddressMode(TextureMapping mapping){

		return mapping == TextureMapping::WRAP ?
			   D3D11_TEXTURE_ADDRESS_WRAP :
			   D3D11_TEXTURE_ADDRESS_CLAMP;

	}
	
}

/////////////////// METHODS ///////////////////////////

HRESULT gi_lib::dx11::MakeDepthStencil(ID3D11Device& device, unsigned int width, unsigned int height, ID3D11Texture2D** depth_stencil, ID3D11DepthStencilView** depth_stencil_view){

	ID3D11Texture2D * texture = nullptr;
	ID3D11DepthStencilView * view = nullptr;

	auto cleanup = make_scope_guard([&texture](){

		if (texture) texture->Release();
		
	});

	D3D11_TEXTURE2D_DESC desc;

	ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));

	desc.ArraySize = 1;
	desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;

	RETURN_ON_FAIL(device.CreateTexture2D(&desc, 
										  nullptr, 
										  &texture));

	if (depth_stencil_view){

		D3D11_DEPTH_STENCIL_VIEW_DESC view_desc;

		ZeroMemory(&view_desc, sizeof(view_desc));

		view_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

		RETURN_ON_FAIL(device.CreateDepthStencilView(reinterpret_cast<ID3D11Resource *>(texture),
													 &view_desc,
													 &view));

		*depth_stencil_view = view;

	}

	*depth_stencil = texture;

	cleanup.Dismiss();

	return S_OK;

}

HRESULT gi_lib::dx11::MakeRenderTarget(ID3D11Device& device, unsigned int width, unsigned int height, DXGI_FORMAT format, ID3D11Texture2D** texture, ID3D11RenderTargetView** render_target_view, ID3D11ShaderResourceView** shader_resource_view, ID3D11UnorderedAccessView** unordered_access_view, bool autogenerate_mips){

	ID3D11RenderTargetView* rtv = nullptr;
	ID3D11ShaderResourceView* srv = nullptr;
	ID3D11UnorderedAccessView* uav = nullptr;

	auto cleanup = make_scope_guard([&texture, &rtv, &srv, &uav](){

		windows::release_com({*texture, rtv, srv, uav});
		
	});

	D3D11_TEXTURE2D_DESC desc;

	ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));

	desc.ArraySize = 1;
	
	desc.BindFlags = (render_target_view ? D3D11_BIND_RENDER_TARGET : 0 ) |
					 (shader_resource_view ? D3D11_BIND_SHADER_RESOURCE : 0) |
					 (unordered_access_view ? D3D11_BIND_UNORDERED_ACCESS : 0);

	desc.CPUAccessFlags = 0;
	desc.Format = format;
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = autogenerate_mips ? 0 : 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.MiscFlags = autogenerate_mips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

	RETURN_ON_FAIL(device.CreateTexture2D(&desc,
										  nullptr,
										  texture));

	if (render_target_view){
		
		RETURN_ON_FAIL(device.CreateRenderTargetView(reinterpret_cast<ID3D11Resource*>(*texture),
													 nullptr,
													 &rtv));

	}
	
	if (shader_resource_view){

		RETURN_ON_FAIL(device.CreateShaderResourceView(reinterpret_cast<ID3D11Resource*>(*texture),
													   nullptr,
													   &srv));

	}
	
	if (unordered_access_view){
		
		RETURN_ON_FAIL(device.CreateUnorderedAccessView(reinterpret_cast<ID3D11Resource*>(*texture),
														nullptr,
														&uav));

	}
	
	if (render_target_view){

		*render_target_view = rtv;

	}
	
	if (shader_resource_view){

		*shader_resource_view = srv;

	}
	
	if (unordered_access_view){

		*unordered_access_view = uav;

	}

	cleanup.Dismiss();

	return S_OK;

}

HRESULT gi_lib::dx11::MakeVertexBuffer(ID3D11Device& device, const void* vertices, size_t size, ID3D11Buffer** buffer){

	// Fill in a buffer description.
	D3D11_BUFFER_DESC buffer_desc;

	buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
	buffer_desc.ByteWidth = static_cast<unsigned int>(size);
	buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	buffer_desc.CPUAccessFlags = 0;
	buffer_desc.MiscFlags = 0;

	// Fill in the subresource data.
	D3D11_SUBRESOURCE_DATA init_data;

	init_data.pSysMem = vertices;
	init_data.SysMemPitch = 0;
	init_data.SysMemSlicePitch = 0;

	// Create the buffer
	return device.CreateBuffer(&buffer_desc, 
							   &init_data,
							   buffer);

}

HRESULT gi_lib::dx11::MakeIndexBuffer(ID3D11Device& device, const unsigned int* indices, size_t size, ID3D11Buffer** buffer){

	// Fill in a buffer description.
	D3D11_BUFFER_DESC buffer_desc;

	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.ByteWidth = static_cast<unsigned int>(size);
	buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	buffer_desc.CPUAccessFlags = 0;
	buffer_desc.MiscFlags = 0;

	// Define the resource data.
	D3D11_SUBRESOURCE_DATA init_data;
	init_data.pSysMem = indices;
	init_data.SysMemPitch = 0;
	init_data.SysMemSlicePitch = 0;

	// Create the buffer
	return device.CreateBuffer(&buffer_desc, 
							   &init_data, 
							   buffer);

}

HRESULT gi_lib::dx11::MakeConstantBuffer(ID3D11Device& device, size_t size, ID3D11Buffer** buffer){

	D3D11_BUFFER_DESC buffer_desc;

	buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
	buffer_desc.ByteWidth = static_cast<unsigned int>(size);
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	buffer_desc.MiscFlags = 0;
	buffer_desc.StructureByteStride = 0;

	// Create the buffer
	return device.CreateBuffer(&buffer_desc, 
							   nullptr, 
							   buffer);

}

HRESULT gi_lib::dx11::MakeStructuredBuffer(ID3D11Device& device, unsigned int element_count, unsigned int element_size, bool dynamic, ID3D11Buffer** buffer, ID3D11ShaderResourceView** shader_resource_view, ID3D11UnorderedAccessView** unordered_access_view){

	D3D11_BUFFER_DESC buffer_desc;

	buffer_desc.Usage = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;

	buffer_desc.ByteWidth = element_size * element_count;

	buffer_desc.BindFlags = (shader_resource_view ? D3D11_BIND_SHADER_RESOURCE : 0) |
							(unordered_access_view ? D3D11_BIND_UNORDERED_ACCESS : 0);

	buffer_desc.CPUAccessFlags = dynamic ? D3D11_CPU_ACCESS_WRITE : 0;

	buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	buffer_desc.StructureByteStride = element_size;

	// Transaction: either all the resources are created, or none.

	ID3D11Buffer* structured = nullptr;
	ID3D11ShaderResourceView* srv = nullptr;
	ID3D11UnorderedAccessView* uav = nullptr;

	auto guard = make_scope_guard([&structured, &srv, &uav]{

		release_com({ structured, srv, uav });

	});

	RETURN_ON_FAIL(device.CreateBuffer(&buffer_desc,
									   nullptr,
									   &structured));

	if (shader_resource_view){

		RETURN_ON_FAIL(device.CreateShaderResourceView(structured,
													   nullptr,
													   &srv));

	}

	if (unordered_access_view){

		RETURN_ON_FAIL(device.CreateUnorderedAccessView(structured,
														nullptr, 
														&uav));
		
	}

	// Commit

	*buffer = structured;

	if (shader_resource_view){

		*shader_resource_view = srv;

	}

	if (unordered_access_view){

		*unordered_access_view = uav;

	}

	guard.Dismiss();

	return S_OK;

}

HRESULT gi_lib::dx11::MakeSampler(ID3D11Device& device, TextureMapping texture_mapping, unsigned int anisotropy_level, ID3D11SamplerState** sampler){

	auto address_mode = TextureMappingToAddressMode(texture_mapping); // Same for each coordinate.

	auto filter = AnisotropyLevelToFilter(anisotropy_level);

	D3D11_SAMPLER_DESC desc;
	
	desc.Filter = filter;
	desc.AddressU = address_mode;
	desc.AddressV = address_mode;
	desc.AddressW = address_mode;
	desc.MipLODBias = 0.0f;								// This could be used to reduce the texture quality, however it will waste VRAM. 
	desc.MaxAnisotropy = anisotropy_level;
	desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	//desc.BorderColor[0] = 1.0f;						// Whatever, not used.
	//desc.BorderColor[1] = 1.0f;
	//desc.BorderColor[2] = 1.0f;
	//desc.BorderColor[3] = 1.0f;
	desc.MinLOD = -FLT_MAX;
	desc.MaxLOD = FLT_MAX;
	
	// Create the sampler state
	return device.CreateSamplerState(&desc, 
									 sampler);

}

Matrix4f gi_lib::dx11::ComputePerspectiveProjectionLH(float field_of_view, float aspect_ratio, float near_plane, float far_plane){

	Matrix4f projection_matrix = Matrix4f::Identity();

	auto height = 1.0f / std::tanf(field_of_view * 0.5f);

	auto width = height / aspect_ratio;

	projection_matrix(0, 0) = width;

	projection_matrix(1, 1) = height;

	projection_matrix(2, 2) = far_plane / (far_plane - near_plane);

	projection_matrix(3, 3) = 0.f;

	projection_matrix(2, 3) = -(near_plane * far_plane) / (far_plane - near_plane);

	projection_matrix(3, 2) = 1.0f;

	return projection_matrix;

}