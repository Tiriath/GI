/// \file dx11texture.h
/// \brief ???
///
/// \author Raffaele D. Facendola

#ifdef _WIN32

#pragma once

#include <vector>

#include "texture.h"
#include "debug.h"

#include "dx11/dx11.h"

#include "windows/win_os.h"
#include "instance_builder.h"

namespace gi_lib{

	namespace dx11{

		using windows::COMPtr;

		/// \brief DirectX11 2D texture.
		/// \author Raffaele D. Facendola.
		class DX11Texture2D : public ITexture2D{

		public:

			/// \brief Create a texture from a shader resource view.
			DX11Texture2D(const COMPtr<ID3D11ShaderResourceView>& shader_resource_view);

			/// \brief Create a new texture from DDS file.
			/// \param device The device used to create the texture.
			/// \param bundle The bundle used to load the texture.
			DX11Texture2D(const FromFile& args);

			virtual ~DX11Texture2D(){}

			virtual size_t GetSize() const override;

			virtual unsigned int GetWidth() const override;

			virtual unsigned int GetHeight() const override;

			virtual unsigned int GetMIPCount() const override;

			virtual TextureFormat GetFormat() const override;

			/// \brief Get the shader resource view used to bind this texture to the pipeline.
			ShaderResourceView GetShaderResourceView();

			/// \brief Get a pointer to the hardware texture
			COMPtr<ID3D11Texture2D> GetTexture();

		private:

			void UpdateDescription(const D3D11_TEXTURE2D_DESC& description);

			COMPtr<ID3D11ShaderResourceView> shader_resource_view_;		///< \brief Pointer to the shader resource view of the texture.

			unsigned int width_;										///< \brief Width of the texture, in pixels.

			unsigned int height_;										///< \brief Height of the texture, in pixels.
			
			unsigned int bits_per_pixel_;								///< \brief Bits per pixel.

			unsigned int mip_levels_;									///< \brief MIP levels.

			TextureFormat format_;										///< \brief Surface format.

		};

		/// \brief DirectX11 general-purpose 2D texture.
		/// This texture can be used as a regular texture but can also be bound as unordered access resource to a compute or pixel shader.
		class DX11GPTexture2D : public IGPTexture2D{

		public:

			/// \brief Create a new general-purpose 2D texture from an explicit description.
			DX11GPTexture2D(const IGPTexture2D::FromDescription& args);

			/// \brief Create a general-purpose 2D texture from an existing unordered access view.
			DX11GPTexture2D(const COMPtr<ID3D11ShaderResourceView>& shader_resource_view, const COMPtr<ID3D11UnorderedAccessView>& unordered_access_view);

			virtual ObjectPtr<ITexture2D> GetTexture() override;

			virtual unsigned int GetWidth() const override;

			virtual unsigned int GetHeight() const override;

			virtual unsigned int GetMIPCount() const override;

			virtual TextureFormat GetFormat() const override;

			virtual size_t GetSize() const override;
						
			/// \brief Get the shader resource view used to bind this texture to the pipeline.
			ShaderResourceView GetShaderResourceView();

			/// \brief Get the unordered access view used to bind this texture to the pipeline.
			UnorderedAccessView GetUnorderedAccessView();

		private:

			COMPtr<ID3D11UnorderedAccessView> unordered_access_view_;		///< \brief Pointer to the unordered access view of the texture.

			ObjectPtr<DX11Texture2D> texture_;								///< \brief Underlying texture.
			
		};

		/// \brief Base interface for general-purpose textures cache under DirectX11.
		/// \author Raffaele D. Facendola.
		class DX11GPTexture2DCache : public IGPTexture2DCache {

		public:

			DX11GPTexture2DCache(const Singleton&);

			virtual void PushToCache(const ObjectPtr<IGPTexture2D>& texture) override;

			virtual ObjectPtr<IGPTexture2D> PopFromCache(unsigned int width, unsigned int height, TextureFormat format, bool generate = true) override;

			virtual size_t GetSize() const override;

			static void PurgeCache();
			
		private:

			static std::vector<ObjectPtr<DX11GPTexture2D>> cache_;			///<\ brief Orphaned resource cache.

		};

		/// \brief DirectX11 2D texture array
		class DX11Texture2DArray : public ITexture2DArray {

		public:

			DX11Texture2DArray(const COMPtr<ID3D11ShaderResourceView>& shader_resource_view);

			virtual unsigned int GetWidth() const override;

			virtual unsigned int GetHeight() const override;

			virtual unsigned int GetMIPCount() const override;

			virtual unsigned int GetCount() const override;
			
			TextureFormat GetFormat() const override;

			virtual size_t GetSize() const override;

			/// \brief Get the shader resource view used to bind this texture to the pipeline.
			/// \remarks The view is for the entire array not just for the single slices.
			ShaderResourceView GetShaderResourceView();

			/// \brief Get a pointer to the hardware texture
			COMPtr<ID3D11Texture2D> GetTextureArray();
			
		private:

			COMPtr<ID3D11ShaderResourceView> shader_resource_view_;		///< \brief Pointer to the shader resource view of the texture array.

			unsigned int width_;										///< \brief Width of the textures, in pixels.

			unsigned int height_;										///< \brief Height of the textures, in pixels.

			unsigned int bits_per_pixel_;								///< \brief Bits per pixel.

			unsigned int mip_levels_;									///< \brief MIP levels.

			unsigned int count_;										///< \brief Elements in the array.

			TextureFormat format_;										///< \brief Surface format.

		};

		/// \brief DirectX11 general-purpose 2D texture array.
		class DX11GPTexture2DArray : public IGPTexture2DArray {

		public:

			DX11GPTexture2DArray(const ITexture2DArray::FromDescription& args);

			virtual ObjectPtr<ITexture2DArray> GetTextureArray() override;

			virtual ObjectPtr<IGPTexture2D> GetTexture(unsigned int index) override;

			virtual unsigned int GetWidth() const override;

			virtual unsigned int GetHeight() const override;

			virtual unsigned int GetMIPCount() const override;

			virtual unsigned int GetCount() const override;
			
			TextureFormat GetFormat() const override;

			virtual size_t GetSize() const override;

			/// \brief Get the shader resource view used to bind this texture to the pipeline.
			ShaderResourceView GetShaderResourceView();

			/// \brief Get the unordered access view used to bind this texture to the pipeline.
			UnorderedAccessView GetUnorderedAccessView();
			
		private:

			COMPtr<ID3D11UnorderedAccessView> unordered_access_view_;		///< \brief Pointer to the unordered access view of the texture array.
			
			ObjectPtr<DX11Texture2DArray> texture_array_;					///< \brief Underlying texture array.
			
			std::vector<ObjectPtr<DX11GPTexture2D>> array_elements_;		///< \brief Elements inside the array as single resources.

		};

		/// \brief DirectX11 3D texture.
		/// \author Raffaele D. Facendola.
		class DX11Texture3D : public ITexture3D {

		public:

			/// \brief Create a texture from a shader resource view.
			DX11Texture3D(const COMPtr<ID3D11ShaderResourceView>& shader_resource_view);

			virtual ~DX11Texture3D() {}

			virtual size_t GetSize() const override;

			virtual unsigned int GetWidth() const override;

			virtual unsigned int GetHeight() const override;

			virtual unsigned int GetDepth() const override;

			virtual unsigned int GetMIPCount() const override;

			virtual TextureFormat GetFormat() const override;

			/// \brief Get the shader resource view used to bind this texture to the pipeline.
			ShaderResourceView GetShaderResourceView();

			/// \brief Get a pointer to the hardware texture
			COMPtr<ID3D11Texture3D> GetTexture();

		private:

			COMPtr<ID3D11ShaderResourceView> shader_resource_view_;		///< \brief Pointer to the shader resource view of the texture.

			unsigned int width_;										///< \brief Width of the texture, in pixels.

			unsigned int height_;										///< \brief Height of the texture, in pixels.

			unsigned int depth_;										///< \brief Depth of the texture, in pixels.

			unsigned int bits_per_pixel_;								///< \brief Bits per pixel.

			unsigned int mip_levels_;									///< \brief MIP levels.

			TextureFormat format_;										///< \brief Surface format.

		};

		/// \brief DirectX11 general-purpose 3D texture.
		/// This texture can be used as a regular texture but can also be bound as unordered access resource to a compute or pixel shader.
		class DX11GPTexture3D : public IGPTexture3D {

		public:

			/// \brief Create a new general-purpose 2D texture from an explicit description.
			DX11GPTexture3D(const DX11GPTexture3D::FromDescription& args);

			virtual ObjectPtr<ITexture3D> GetTexture() override;

			virtual unsigned int GetWidth() const override;

			virtual unsigned int GetHeight() const override;

			virtual unsigned int GetDepth() const override;

			virtual unsigned int GetMIPCount() const override;

			virtual ObjectPtr<IGPTexture3D> GetMIP(unsigned int mip_index) override;

			virtual TextureFormat GetFormat() const override;

			virtual size_t GetSize() const override;

			/// \brief Get the shader resource view used to bind this texture to the pipeline.
			ShaderResourceView GetShaderResourceView();

			/// \brief Get the unordered access view used to bind this texture to the pipeline.
			UnorderedAccessView GetUnorderedAccessView();

		private:

			DX11GPTexture3D(DX11GPTexture3D& source, unsigned int mip_index);

			COMPtr<ID3D11UnorderedAccessView> unordered_access_view_;		///< \brief Pointer to the unordered access view of the texture.

			ObjectPtr<DX11Texture3D> texture_;								///< \brief Underlying texture.

			mutable std::vector<ObjectPtr<IGPTexture3D>> mips_;				///< \brief Texture restricted to each MIP level.

		};

		/// \brief DirectX11 general-purpose 3D clipmap.
		class DX11GPClipmap3D : public IGPClipmap3D {

		public:

			DX11GPClipmap3D(const DX11GPClipmap3D::FromDescription& args);

			virtual ObjectPtr<IGPTexture3D> GetPyramid() override;
			
			virtual ObjectPtr<IGPTexture3D> GetStack() override;
			
			virtual unsigned int GetWidth() const override;
			
			virtual unsigned int GetHeight() const override;
			
			virtual unsigned int GetDepth() const override;
			
			virtual unsigned int GetStacks() const override;
			
			virtual TextureFormat GetFormat() const override;
			
			virtual size_t GetSize() const override;

		private:

			ObjectPtr<DX11GPTexture3D> pyramid_;			///< \brief Pyramid part of the clipmap.

			ObjectPtr<DX11GPTexture3D> stack_;				///< \brief Stack part of the clipmap. MIPS increase along the Y axis.

		};

		/// \brief Downcasts an ITexture2D to the proper concrete type.
		ObjectPtr<DX11Texture2D> resource_cast(const ObjectPtr<ITexture2D>& resource);

		/// \brief Downcasts an IGPTexture2D to the proper concrete type.
		ObjectPtr<DX11GPTexture2D> resource_cast(const ObjectPtr<IGPTexture2D>& resource);

		/// \brief Downcasts an ITexture2DArray to the proper concrete type.
		ObjectPtr<DX11Texture2DArray> resource_cast(const ObjectPtr<ITexture2DArray>& resource);
		
		/// \brief Downcasts an IGPTexture2DArray to the proper concrete type.
		ObjectPtr<DX11GPTexture2DArray> resource_cast(const ObjectPtr<IGPTexture2DArray>& resource);

		/// \brief Downcasts an ITexture3D to the proper concrete type.
		ObjectPtr<DX11Texture3D> resource_cast(const ObjectPtr<ITexture3D>& resource);

		/// \brief Downcasts an IGPTexture3D to the proper concrete type.
		ObjectPtr<DX11GPTexture3D> resource_cast(const ObjectPtr<IGPTexture3D>& resource);

		/// \brief Convert a texture format to a DXGI format used by DirectX11.
		/// \param texture_format Texture format to convert.
		/// \return Returns the DXGI format corresponding to the texture format specified. If no conversion could be performed, returns DXGI_FORMAT_UNKNOWN.
		DXGI_FORMAT TextureFormatToDXGIFormat(const TextureFormat& texture_format);

		/// \brief Convert a DXGI format used by DirectX11 to a texture format.
		/// \param texture_format Texture format to convert.
		/// \return Returns the texture format corresponding to the texture format specified.
		TextureFormat DXGIFormatToTextureFormat(const DXGI_FORMAT& texture_format);
		
		/////////////////////////////// DX11 TEXTURE2D ///////////////////////////////
		
		INSTANTIABLE(ITexture2D, DX11Texture2D, ITexture2D::FromFile);

		inline unsigned int DX11Texture2D::GetWidth() const{

			return width_;

		}

		inline unsigned int DX11Texture2D::GetHeight()const {

			return height_;

		}

		inline unsigned int DX11Texture2D::GetMIPCount() const{

			return mip_levels_;

		}

		inline TextureFormat DX11Texture2D::GetFormat() const{

			return format_;

		}

		inline ShaderResourceView DX11Texture2D::GetShaderResourceView(){

			return ShaderResourceView(this,
									  shader_resource_view_);

		}

		inline COMPtr<ID3D11Texture2D> DX11Texture2D::GetTexture(){

			ID3D11Resource* resource;

			shader_resource_view_->GetResource(&resource);

			ID3D11Texture2D* texture = static_cast<ID3D11Texture2D*>(resource);

			return windows::COMMove(&texture);

		}

		///////////////////////////// DX11 GP TEXTURE2D //////////////////////////////
		
		INSTANTIABLE(IGPTexture2D, DX11GPTexture2D, IGPTexture2D::FromDescription);

		inline ObjectPtr<ITexture2D> DX11GPTexture2D::GetTexture()
		{
			
			return ObjectPtr<ITexture2D>(texture_);

		}

		inline unsigned int DX11GPTexture2D::GetWidth() const
		{
			
			return texture_->GetWidth();

		}

		inline unsigned int DX11GPTexture2D::GetHeight() const
		{
			
			return texture_->GetHeight();

		}

		inline unsigned int DX11GPTexture2D::GetMIPCount() const
		{
			
			return texture_->GetMIPCount();

		}

		inline TextureFormat DX11GPTexture2D::GetFormat() const {

			return texture_->GetFormat();

		}

		inline size_t DX11GPTexture2D::GetSize() const
		{
			
			return texture_->GetSize();

		}

		inline ShaderResourceView DX11GPTexture2D::GetShaderResourceView(){

			return texture_->GetShaderResourceView();

		}

		inline UnorderedAccessView DX11GPTexture2D::GetUnorderedAccessView(){

			return UnorderedAccessView(this,
									   unordered_access_view_);

		}

		///////////////////////////// DX11 GP TEXTURE2D CACHE //////////////////////////////

		INSTANTIABLE(IGPTexture2DCache, DX11GPTexture2DCache, IGPTexture2DCache::Singleton);

		///////////////////////////// DX11 TEXTURE 2D ARRAY //////////////////////////////

		inline unsigned int DX11Texture2DArray::GetWidth() const {

			return width_;

		}

		inline unsigned int DX11Texture2DArray::GetHeight()const {

			return height_;

		}

		inline unsigned int DX11Texture2DArray::GetMIPCount() const {

			return mip_levels_;

		}

		inline unsigned int DX11Texture2DArray::GetCount() const {

			return count_;

		}

		inline TextureFormat DX11Texture2DArray::GetFormat() const {

			return format_;

		}

		inline ShaderResourceView DX11Texture2DArray::GetShaderResourceView() {

			return ShaderResourceView(this,
									  shader_resource_view_);

		}

		inline COMPtr<ID3D11Texture2D> DX11Texture2DArray::GetTextureArray() {

			ID3D11Resource* resource;

			shader_resource_view_->GetResource(&resource);

			ID3D11Texture2D* texture = static_cast<ID3D11Texture2D*>(resource);

			return windows::COMMove(&texture);

		}
		
		inline ObjectPtr<IGPTexture2D> DX11GPTexture2DArray::GetTexture(unsigned int index) {

			return ObjectPtr<IGPTexture2D>(array_elements_[index]);

		}

		///////////////////////////// DX11 GP TEXTURE 2D ARRAY //////////////////////////////

		INSTANTIABLE(IGPTexture2DArray, DX11GPTexture2DArray, ITexture2DArray::FromDescription);

		inline ObjectPtr<ITexture2DArray> DX11GPTexture2DArray::GetTextureArray(){
			
			return ObjectPtr<ITexture2DArray>(texture_array_);

		}

		inline unsigned int DX11GPTexture2DArray::GetWidth() const{
			
			return texture_array_->GetWidth();

		}

		inline unsigned int DX11GPTexture2DArray::GetHeight() const{
			
			return texture_array_->GetHeight();

		}

		inline unsigned int DX11GPTexture2DArray::GetMIPCount() const{

			return texture_array_->GetMIPCount();

		}

		inline unsigned int DX11GPTexture2DArray::GetCount() const{

			return texture_array_->GetCount();

		}
	
		inline TextureFormat DX11GPTexture2DArray::GetFormat() const {

			return texture_array_->GetFormat();

		}

		inline size_t DX11GPTexture2DArray::GetSize() const{

			return texture_array_->GetSize();

		}

		/// \brief Get the shader resource view used to bind this texture to the pipeline.
		inline ShaderResourceView DX11GPTexture2DArray::GetShaderResourceView() {

			return texture_array_->GetShaderResourceView();

		}

		/// \brief Get the unordered access view used to bind this texture to the pipeline.
		inline UnorderedAccessView DX11GPTexture2DArray::GetUnorderedAccessView() {

			return UnorderedAccessView(this,
									   unordered_access_view_);

		}

		/////////////////////////////// DX11 TEXTURE3D ///////////////////////////////

		inline unsigned int DX11Texture3D::GetWidth() const {

			return width_;

		}

		inline unsigned int DX11Texture3D::GetHeight() const {

			return height_;

		}

		inline unsigned int DX11Texture3D::GetDepth() const {

			return depth_;

		}

		inline unsigned int DX11Texture3D::GetMIPCount() const {

			return mip_levels_;

		}

		inline TextureFormat DX11Texture3D::GetFormat() const {

			return format_;

		}

		inline ShaderResourceView DX11Texture3D::GetShaderResourceView() {

			return ShaderResourceView(this,
									  shader_resource_view_);

		}

		inline COMPtr<ID3D11Texture3D> DX11Texture3D::GetTexture() {

			ID3D11Resource* resource;

			shader_resource_view_->GetResource(&resource);

			ID3D11Texture3D* texture = static_cast<ID3D11Texture3D*>(resource);

			return windows::COMMove(&texture);

		}

		///////////////////////////// DX11 GP TEXTURE3D //////////////////////////////

		INSTANTIABLE(IGPTexture3D, DX11GPTexture3D, IGPTexture3D::FromDescription);

		inline ObjectPtr<ITexture3D> DX11GPTexture3D::GetTexture()
		{

			return ObjectPtr<ITexture3D>(texture_);

		}

		inline unsigned int DX11GPTexture3D::GetWidth() const
		{

			return texture_->GetWidth();

		}

		inline unsigned int DX11GPTexture3D::GetHeight() const
		{

			return texture_->GetHeight();

		}

		inline unsigned int DX11GPTexture3D::GetDepth() const
		{

			return texture_->GetDepth();

		}

		inline unsigned int DX11GPTexture3D::GetMIPCount() const
		{

			return texture_->GetMIPCount();

		}

		inline TextureFormat DX11GPTexture3D::GetFormat() const {

			return texture_->GetFormat();

		}

		inline size_t DX11GPTexture3D::GetSize() const
		{

			return texture_->GetSize();

		}

		inline ShaderResourceView DX11GPTexture3D::GetShaderResourceView() {

			return texture_->GetShaderResourceView();

		}

		inline UnorderedAccessView DX11GPTexture3D::GetUnorderedAccessView() {

			return UnorderedAccessView(this,
									   unordered_access_view_);

		}
		
		///////////////////////////// DX11 GP CLIPMAP3D //////////////////////////////

		INSTANTIABLE(IGPClipmap3D, DX11GPClipmap3D, IGPClipmap3D::FromDescription);

		inline ObjectPtr<IGPTexture3D> DX11GPClipmap3D::GetPyramid(){
			
			return ObjectPtr<IGPTexture3D>(pyramid_);


		}

		inline ObjectPtr<IGPTexture3D> DX11GPClipmap3D::GetStack(){
			
			return ObjectPtr<IGPTexture3D>(stack_);

		}

		inline unsigned int DX11GPClipmap3D::GetWidth() const{
			
			return pyramid_->GetWidth();

		}

		inline unsigned int DX11GPClipmap3D::GetHeight() const{

			return pyramid_->GetHeight();

		}

		inline unsigned int DX11GPClipmap3D::GetDepth() const{

			return pyramid_->GetDepth();
			
		}

		inline unsigned int DX11GPClipmap3D::GetStacks() const{
			
			return stack_->GetHeight() / pyramid_->GetHeight();

		}

		inline TextureFormat DX11GPClipmap3D::GetFormat() const{
			
			return pyramid_->GetFormat();

		}

		inline size_t DX11GPClipmap3D::GetSize() const{
			
			return stack_->GetSize() + pyramid_->GetSize();

		}


		///////////////////////////// RESOURCE CAST ////////////////////////////////

		inline ObjectPtr<DX11Texture2D> resource_cast(const ObjectPtr<ITexture2D>& resource){

			return ObjectPtr<DX11Texture2D>(resource.Get());

		}

		inline ObjectPtr<DX11GPTexture2D> resource_cast(const ObjectPtr<IGPTexture2D>& resource){

			return ObjectPtr<DX11GPTexture2D>(resource.Get());

		}

		inline ObjectPtr<DX11Texture2DArray> resource_cast(const ObjectPtr<ITexture2DArray>& resource) {

			return ObjectPtr<DX11Texture2DArray>(resource.Get());

		}

		inline ObjectPtr<DX11GPTexture2DArray> resource_cast(const ObjectPtr<IGPTexture2DArray>& resource) {

			return ObjectPtr<DX11GPTexture2DArray>(resource.Get());

		}

		inline ObjectPtr<DX11Texture3D> resource_cast(const ObjectPtr<ITexture3D>& resource) {

			return ObjectPtr<DX11Texture3D>(resource.Get());

		}

		inline ObjectPtr<DX11GPTexture3D> resource_cast(const ObjectPtr<IGPTexture3D>& resource) {

			return ObjectPtr<DX11GPTexture3D>(resource.Get());

		}

	}

}

#endif