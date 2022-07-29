#pragma once

namespace Zn
{
	class Editor
	{
	public:

		static Editor& Get();

		void PreUpdate(float deltaTime);

		void Update(float deltaTime);

		void PostUpdate(float deltaTime);

		bool IsRequestingExit() const;

	};
}
