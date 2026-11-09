#ifndef UPP_GAMEENGINE_H
#define UPP_GAMEENGINE_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <CtrlCore/CtrlCore.h>
#include <Geometry/Geometry.h>
#include <GameLib/GameLib.h>

// Include relevant API packages that we'll need
#include <api/Screen/Screen.h>     // For windowing and input
#include <api/Graphics/Graphics.h> // For rendering

// Define GAMEENGINE_NAMESPACE to indicate that we're opening the namespace
#define GAMEENGINE_NAMESPACE


NAMESPACE_UPP

// Higher-level game engine features go here
#include "GameWindow.h"
#include "Game.h"
#include "AssetManager.h"
#include "Scene.h"
#include "Rendering.h"
#include "Shader.h"
#include "SpriteMesh.h"
#include "Material.h"
#include "PostProcessing.h"
#include "TextureStreaming.h"
#include "RenderBatching.h"
#include "InputSystem.h"
#include "InputSystemExample.h"
#include "AudioSystem.h"
#include "PhysicsSystem.h"
#include "UISystem.h"
#include "VFS.h"
#include "AnimationSystem.h"
#include "EcsIntegration.h"
#include "SpriteBatch.h"
#include "TextureAtlas.h"
#include "ParticleSystem.h"
#include "SceneGraph.h"

END_UPP_NAMESPACE

#endif
