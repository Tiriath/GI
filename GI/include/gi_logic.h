#pragma once

#include <memory>

#include "core.h"
#include "graphics.h"
#include "scene.h"

using namespace gi_lib;
using namespace std;

namespace gi_lib{
	
	class DeferredRenderer;
	class TransformComponent;

}

namespace gi{

	///Application's logic
	class GILogic : public IWindowLogic{

	public:

		GILogic();

		virtual ~GILogic();

	protected:

		virtual void Initialize(Window& window) override;

		virtual void Update(const Time & time) override;
		
	private:

		void SetupLights(Scene& scene);

		std::vector<TransformComponent*> point_lights;

		std::vector<TransformComponent*> directional_lights;

		Graphics& graphics_;

		unique_ptr<IOutput> output_;

		unique_ptr<DeferredRenderer> deferred_renderer_;

		unique_ptr<Scene> scene_;
		
		const IInput* input_;

		bool paused_;

	};

}

