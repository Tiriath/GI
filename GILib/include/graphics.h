/// \file graphics.h
/// \brief Defines types, classes and methods used to manage the graphical subsystem. 
///
/// \author Raffaele D. Facendola

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <typeindex>
#include <typeinfo>
#include <tuple>

#include "resources.h"
#include "render_target.h"
#include "eigen.h"

#include "observable.h"

using ::std::wstring;
using ::std::vector;
using ::std::shared_ptr;
using ::std::unique_ptr;
using ::std::weak_ptr;
using ::std::type_index;

using ::Eigen::Vector2f;

namespace gi_lib{

	class Window;
	class Scene;
	class Time;

	class IResource;
	class IRenderer;
	class IOutput;

	/// \brief Enumeration of all the supported API.
	enum class API{

		DIRECTX_11,		///< DirectX 11.0

	};

	/// \brief Enumeration of all supported anti-aliasing techniques.
	enum class AntialiasingMode{

		NONE,			///< No antialiasing.
		MSAA_2X,		///< Multisample antialiasing, 2X.
		MSAA_4X,		///< Multisample antialiasing, 4X.
		MSAA_8X,		///< Multisample antialiasing, 8X.
		MSAA_16X,		///< Multisample antialiasing, 16X.

	};

	/// \brief Describes a projection type.
	enum class ProjectionType{

		Perspective,			///< \brief Perspective projection.
		Ortographic				///< \brief Ortographic projection.

	};

	/// \brief Describes a video mode.
	struct VideoMode{

		unsigned int horizontal_resolution;		///< Horizontal resolution, in pixels.
		unsigned int vertical_resolution;		///< Vertical resolution, in pixels.
		unsigned int refresh_rate;				///< Refresh rate, in Hz.

	};

	/// \brief Describes the video card's parameters and capabilities.
	struct AdapterProfile{

		wstring name;										///< Name of the video card.
		size_t dedicated_memory;							///< Dedicated memory, in bytes.
		size_t shared_memory;								///< Shared memory, in bytes.		
		vector<VideoMode> video_modes;						///< List of supported video modes.
		vector<AntialiasingMode> antialiasing_modes;		///< List of supported antialiasing modes.

		unsigned int max_anisotropy;						///< Maximum level of anisotropy.
		unsigned int max_mips;								///< Maximum number of MIP levels.

	};

	/// \brief Viewport bounds
	struct Viewport{

		Vector2f position;									///< \brief Position of the top-left corner in screen units. Valid range between 0 (top/left) and 1 (bottom/right)).
		Vector2f extents;									///< \brief Extents of the viewport in screen units. Valid range between 0 and 1 (full size).
		
	};

	/// \brief A color.
	union Color{

		struct{

			float alpha;									///< \brief Alpha component.
			float red;										///< \brief Red component.
			float green;									///< \brief Green component.
			float blue;										///< \brief Blue component.

		} color;											///< \brief Color.

		float argb[4];										///< \brief Array of components.

		Color();

		Color(float red, float green, float blue, float alpha);

		/// \brief Converts this to a 4-elements vector.
		Vector4f ToVector4f() const;

	};

	extern const Color kOpaqueWhite;			///< \brief Fully-opaque white color.

	extern const Color kOpaqueBlack;			///< \brief Fully-opaque black color.

	extern const Color kTransparentBlack;		///< \brief Fully-transparent black color.

	/// \brief Base interface for renderers.
	/// \author Raffaele D. Facendola
	class IRenderer{

	public:
		
		/// \brief Virtual destructor.
		virtual ~IRenderer(){}

		/// \brief Get the scene the renderer refers to.
		/// \return Returns the scene the renderer refers to.
		virtual Scene& GetScene() = 0;

		/// \brief Get the scene the renderer refers to.
		/// \return Returns the scene the renderer refers to.
		virtual const Scene& GetScene() const = 0;

		/// \brief Draw the scene from the current main camera.
		/// \param Returns a pointer to the drawn image.
		/// \param time Current game time.
		/// \param width Width of the drawn image.
		/// \param height Height of the drawn image.
		virtual ObjectPtr<ITexture2D> Draw(const Time& time, unsigned int width, unsigned int height) = 0;

	protected:

		/// \brief Protected constructor. Prevent instantiation.
		IRenderer(){}

	};

	/// \brief Interface used to display an image to an output.
	/// \author Raffaele D. Facendola
	class IOutput{

	public:

		/// \brief Default destructor;
		virtual ~IOutput(){}

		/// \brief Set the video mode.
		/// \param video_mode The video mode to set.
		virtual void SetVideoMode(const VideoMode & video_mode) = 0;

		/// \brief Get the current video mode.
		/// \return Returns the current video mode.
		virtual const VideoMode & GetVideoMode() const = 0;

		/// \brief Enable or disable fullscreen state.
		/// \param fullscreen Set to true to enable fullscreen mode, false to disable it.
		virtual void SetFullscreen(bool fullscreen) = 0;

		/// \brief Get the current fullscreen state.
		/// \return Returns true if fullscreen is enabled, false otherwise.
		virtual bool IsFullscreen() const = 0;

		/// \brief Enable or disable VSync.
		/// \param vsync Set to true to enable VSync, false to disable it.
		virtual void SetVSync(bool vsync) = 0;

		/// \brief Get the current VSync state.
		/// \return Returns true if VSync is enabled, false otherwise.
		virtual bool IsVSync() const = 0;

		/// \brief Set the hardware antialiasing mode.
		/// \param antialiasing The new antialiasing mode.
		virtual void SetAntialiasing(AntialiasingMode antialiasing) = 0;

		/// \brief Get the current antialiasing mode.
		/// \return Return the current antialiasing mode.
		virtual AntialiasingMode GetAntialiasing() const = 0;

		/// \brief Display the given image onto this output.
		/// \param image Image to display.
		virtual void Display(const ObjectPtr<ITexture2D>& image) = 0;
		
	};

	/// \brief Resource manager interface.
	/// \author Raffaele D. Facendola.
	class Resources{

	public:

		/// \brief Default constructor.
		Resources();

		/// \brief Default destructor;
		virtual ~Resources();
		
		/// \brief Loads a resource.
		/// \tparam TResource Type of the resource to load. Must derive from IResource.
		/// \tparam TLoadArgs Type of the load arguments passed to the object. Arguments must expose caching capabilities.
		/// \param load_args Arguments that will be passed to the resource's constructor.
		/// \return Returns the loaded resource if possible, returns null otherwise. If the resource was already loaded, returns a pointer to the existing instance instead.
		template <typename TResource, typename TArgs, typename use_cache<TArgs>::type* = nullptr>
		ObjectPtr<TResource> Load(const typename TArgs& args);

		/// \brief Loads a resource.
		/// \tparam TResource Type of the resource to load. Must derive from IResource.
		/// \tparam TLoadArgs Type of the load arguments passed to the object.
		/// \param load_args Arguments that will be passed to the resource's constructor.
		/// \return Returns a new loaded resource instance if possible, returns null otherwise.
		template <typename TResource, typename TArgs, typename no_cache<TArgs>::type* = nullptr>
		ObjectPtr<TResource> Load(const typename TArgs& args);

		/// \brief Get the amount of memory used by the loaded resources.
		size_t GetSize() const;

	protected:

		/// \brief Load a resource.
		/// The method <i>requires</i> that <i>bundle<i> is compatible with <i>bundle_type</i>.
		/// The method <i>ensures</i> that the returned object is compatible with the type <i>resource_type</i>.
		/// \param resource_type Resource's type index.
		/// \param load_args_type Bundle's type index.
		/// \param load_args Pointer to the bundle to be used to load the resource.
		/// \return Returns a pointer to the loaded resource
		virtual ObjectPtr<IResource> Load(const type_index& resource_type, const type_index& args_type, const void* load_args) const = 0;

	private:

		/// \brief Private implementation of the class.
		struct Impl;

		/// \brief Loads a resource from cache.
		/// \return Returns a pointer to the cached resource if any, otherwise returns a new instance. Returns null if the resource was not supported.
		ObjectPtr<IResource> LoadFromCache(const type_index& resource_type, const type_index& args_type, const void* args, size_t cache_key);

		/// \brief Loads a resource instance.
		/// \return Returns the resource loaded.
		ObjectPtr<IResource> LoadDirect(const type_index& resource_type, const type_index& args_type, const void* args);

		/// \brief Opaque pointer to the implementation of the class.
		unique_ptr<Impl> pimpl_;

	};

	/// \brief Factory interface used to create and initialize the graphical subsystem.
	/// \author Raffaele D. Facendola
	class Graphics{

	public:
		
		/// \brief Get a reference to a specific graphical subsystem.
		static Graphics& GetAPI(API api);
		
		/// \brief Default destructor;
		virtual ~Graphics();

		/// \brief Get the video card's parameters and capabilities.
		virtual AdapterProfile GetAdapterProfile() const = 0;

		/// \brief Create an output.
		/// \param window The window used to display the output.
		/// \param video_mode The initial window mode.
		/// \return Returns a pointer to the new output.
		virtual unique_ptr<IOutput> CreateOutput(Window& window, const VideoMode& video_mode) = 0;

		/// \brief Create a renderer.
		/// \return Returns a pointer to the new renderer.
		template <typename TRenderer>
		unique_ptr<TRenderer> CreateRenderer(Scene& scene);

		/// \brief Get the resource manager.
		/// \return Returns the resource manager.
		virtual Resources & GetResources() = 0;
		
		/// \brief Push an event that can be used to track the application flow under a performance tool.
		virtual void PushEvent(const std::wstring& event_name) = 0;

		/// \brief Pop the last event pushed.
		virtual void PopEvent() = 0;

	protected:

		Graphics();

		/// \brief Create a renderer.
		/// The method <i>requires</i> that <i>renderer_args<i> is compatible with <i>renderer_type</i>.
		/// The method <i>ensures</i> that the returned object is compatible with the type <i>renderer_type</i>.
		/// \param renderer_type Type of the renderer to create.
		/// \param scene Scene that will be bound to the new renderer.
		/// \return Returns a pointer to the new renderer.
		virtual IRenderer* CreateRenderer(const type_index& renderer_type, Scene& scene) const = 0;

	};

	///////////////////////////////// COLOR ////////////////////////////////////////

	inline Color::Color(){ /* Uninitialized */}

	inline Color::Color(float red, float green, float blue, float alpha){

		color.red = red;
		color.green = green;
		color.blue = blue;
		color.alpha = alpha;

	}

	inline Vector4f Color::ToVector4f() const {

		return Vector4f(color.red,
						color.green,
						color.blue,
						color.alpha);

	}

	///////////////////////////////// RESOURCES ////////////////////////////////////

	template <typename TResource, typename TArgs, typename use_cache<TArgs>::type*>
	ObjectPtr<TResource> Resources::Load(const typename TArgs& args){

		return ObjectPtr<TResource>(LoadFromCache(type_index(typeid(TResource)),
												  type_index(typeid(TArgs)),
												  &args,
												  args.GetCacheKey()));
				
	}

	template <typename TResource, typename TArgs, typename no_cache<TArgs>::type*>
	ObjectPtr<TResource> Resources::Load(const typename TArgs& args){

		return ObjectPtr<TResource>(LoadDirect(type_index(typeid(TResource)),
											   type_index(typeid(TArgs)),
											   &args));

	}
	
	///////////////////////////////// GRAPHICS ////////////////////////////////////

	template <typename TRenderer>
	unique_ptr<TRenderer> Graphics::CreateRenderer(Scene& scene){

		// Downcast from IRenderer to TRenderer. The cast here is safe.

		return unique_ptr<TRenderer>(static_cast<TRenderer*>(CreateRenderer(type_index(typeid(TRenderer)),
																			scene)));

	}

}