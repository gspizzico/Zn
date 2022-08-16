#include <Znpch.h>
#include <Core/HAL/SDL/SDLWrapper.h>
#include <SDL.h>

DEFINE_STATIC_LOG_CATEGORY(LogSDL, ELogVerbosity::Log);

using namespace Zn;

bool SDLWrapper::Initialize()
{
	SDLWrapper& instance = SDLWrapper::Get();

	if (!instance.m_Initialized)
	{
		if (SDL_InitSubSystem(SDL_INIT_VIDEO/*, add more flags here*/) < 0)
		{
			ZN_LOG(LogSDL, ELogVerbosity::Error, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		}
		else
		{
			instance.m_Initialized = true;
		}
	}

	return instance.m_Initialized;
}

void SDLWrapper::Shutdown()
{
	SDLWrapper& instance = SDLWrapper::Get();

	if (instance.m_Initialized)
	{
		SDL_Quit();
		instance.m_Initialized = false;
	}
}

bool SDLWrapper::IsInitialized()
{
	return SDLWrapper::Get().m_Initialized;
}

SDLWrapper& SDLWrapper::Get()
{
	static SDLWrapper Instance{};
	return Instance;
}
