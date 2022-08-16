#pragma once

#include "Core/HAL/BasicTypes.h"
#include "Core/Containers/Vector.h"
#include "Core/Log/OutputDevice.h"
#include <utility>

namespace Zn
{
	// Manager of output devices. Holds their references and is used as passthrough for messages.
	class OutputDeviceManager
	{
	public:

		static OutputDeviceManager& Get();

		bool OutputMessage(const String& message)
		{
			return OutputMessage(message.c_str());
		}

		bool OutputMessage(const char* message);

		template<typename T, typename ...Args>
		void RegisterOutputDevice(Args&& ... args);

	private:

		Vector<SharedPtr<IOutputDevice>> OutputDevices;

		OutputDeviceManager() = default;

		OutputDeviceManager(OutputDeviceManager const& other) = delete;
	};

	template<typename T, typename ...Args>
	inline void OutputDeviceManager::RegisterOutputDevice(Args&& ... args)
	{
		OutputDevices.emplace_back(std::make_shared<T>(T(std::forward<T>(args)...)));
	}
}
