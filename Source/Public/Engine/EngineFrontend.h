#pragma once

#include <Core/HAL/BasicTypes.h>
#include <Core/Name.h>
#include <Core/Containers/Map.h>

namespace Zn
{	
	class EngineFrontend
	{
	public:		

		EngineFrontend() = default;

		void DrawMainMenu();
		void DrawAutomationWindow();
		
		bool bAutomationWindow = false;
		bool bIsRequestingExit = false;

		Map<Name, bool> SelectedTests;
	};

}