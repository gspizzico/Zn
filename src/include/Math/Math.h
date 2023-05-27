#pragma once

// glm
// The following define will default align glm:: types.
// There are still alignment problems though. for example
// struct
// {
//		glm::vec3
//		glm::vec3
//		float
// }
// is GPU aligned to 32 while is cpp aligned to 48.
// Leaving it commented because we might find a way to fix this.
// #define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <Math/MathUtils.h>
