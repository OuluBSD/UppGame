#include "SkyboxEnvironment.h"

NAMESPACE_UPP

Skybox::Skybox() {
    CreateSkyboxMesh();
}

bool Skybox::Initialize(const String& texturePath) {
    // In a real implementation, this would load a cubemap texture
    // For now, we'll generate a procedural sky
    return InitializeProcedural();
}

bool Skybox::Initialize(const Image& posX, const Image& negX,
                       const Image& posY, const Image& negY,
                       const Image& posZ, const Image& negZ) {
    textures.SetCount(6);
    textures[0] = posX;  // +X
    textures[1] = negX;  // -X
    textures[2] = posY;  // +Y
    textures[3] = negY;  // -Y
    textures[4] = posZ;  // +Z
    textures[5] = negZ;  // -Z
    
    return true;
}

bool Skybox::InitializeProcedural() {
    // Create a simple gradient sky
    Image skyTexture = Image(256, 256);
    ImageBuffer buf(256, 256);
    
    for (int y = 0; y < 256; y++) {
        RGBA* line = buf[y];
        double gradient = (double)y / 255.0;  // From 0 (top) to 1 (bottom)
        
        for (int x = 0; x < 256; x++) {
            // Create a gradient from blue (top) to lighter blue/white (bottom)
            byte r = (byte)(135 * (1 - gradient) + 200 * gradient);
            byte g = (byte)(206 * (1 - gradient) + 220 * gradient);
            byte b = (byte)(235 * (1 - gradient) + 255 * gradient);
            
            line[x] = RGBA(r, g, b, 255);
        }
    }
    
    // Set all 6 faces to the same texture
    textures.SetCount(6);
    Image skyImage = Image(buf);
    for (int i = 0; i < 6; i++) {
        textures[i] = skyImage;
    }
    
    return true;
}

void Skybox::Render(Draw& draw, const Matrix4& viewMatrix, const Matrix4& projectionMatrix, 
                   const Rect& viewport) {
    // For a skybox, we typically:
    // 1. Set the depth to far plane so it's always at the back
    // 2. Render the cube without translation affecting the view
    // 3. Apply the sky texture
    
    // In this simplified implementation, we'll draw a large cube with sky coloring
    // In a real implementation, this would use a proper cubemap and shader
    
    MeshRenderer renderer;
    
    // Create a temporary transformation matrix that positions the sky at the camera
    Matrix4 skyViewMatrix = viewMatrix;
    // Remove translation so skybox follows camera position
    skyViewMatrix.SetTranslation(Vector3(0, 0, 0));
    
    Matrix4 mvp = projectionMatrix * skyViewMatrix; // No model transform needed for sky
    
    // Apply time-based effects
    ApplyTimeEffects();
    
    // Render the skybox mesh with the sky color
    renderer.Render(draw, skyboxMesh, mvp, viewport, LightBlue());
}

void Skybox::CreateSkyboxMesh() {
    double half = size / 2.0;
    
    // Create vertices for a cube (skybox)
    Vector<Vertex> verts = {
        // Front face
        Vertex(Point3(-half, -half,  half), Vector3(0, 0, 1), Point2(0, 1)),
        Vertex(Point3( half, -half,  half), Vector3(0, 0, 1), Point2(1, 1)),
        Vertex(Point3( half,  half,  half), Vector3(0, 0, 1), Point2(1, 0)),
        Vertex(Point3(-half,  half,  half), Vector3(0, 0, 1), Point2(0, 0)),

        // Back face
        Vertex(Point3(-half, -half, -half), Vector3(0, 0, -1), Point2(1, 1)),
        Vertex(Point3(-half,  half, -half), Vector3(0, 0, -1), Point2(1, 0)),
        Vertex(Point3( half,  half, -half), Vector3(0, 0, -1), Point2(0, 0)),
        Vertex(Point3( half, -half, -half), Vector3(0, 0, -1), Point2(0, 1)),

        // Top face
        Vertex(Point3(-half,  half, -half), Vector3(0, 1, 0), Point2(0, 1)),
        Vertex(Point3(-half,  half,  half), Vector3(0, 1, 0), Point2(0, 0)),
        Vertex(Point3( half,  half,  half), Vector3(0, 1, 0), Point2(1, 0)),
        Vertex(Point3( half,  half, -half), Vector3(0, 1, 0), Point2(1, 1)),

        // Bottom face
        Vertex(Point3(-half, -half, -half), Vector3(0, -1, 0), Point2(1, 1)),
        Vertex(Point3( half, -half, -half), Vector3(0, -1, 0), Point2(0, 1)),
        Vertex(Point3( half, -half,  half), Vector3(0, -1, 0), Point2(0, 0)),
        Vertex(Point3(-half, -half,  half), Vector3(0, -1, 0), Point2(1, 0)),

        // Right face
        Vertex(Point3( half, -half, -half), Vector3(1, 0, 0), Point2(0, 1)),
        Vertex(Point3( half,  half, -half), Vector3(1, 0, 0), Point2(0, 0)),
        Vertex(Point3( half,  half,  half), Vector3(1, 0, 0), Point2(1, 0)),
        Vertex(Point3( half, -half,  half), Vector3(1, 0, 0), Point2(1, 1)),

        // Left face
        Vertex(Point3(-half, -half, -half), Vector3(-1, 0, 0), Point2(1, 1)),
        Vertex(Point3(-half, -half,  half), Vector3(-1, 0, 0), Point2(0, 1)),
        Vertex(Point3(-half,  half,  half), Vector3(-1, 0, 0), Point2(0, 0)),
        Vertex(Point3(-half,  half, -half), Vector3(-1, 0, 0), Point2(1, 0))
    };

    // Define indices for the cube (6 faces, 2 triangles each, 3 vertices per triangle)
    Vector<int> indices = {
        // Front face
        0, 1, 2, 0, 2, 3,
        // Back face
        4, 5, 6, 4, 6, 7,
        // Top face
        8, 9, 10, 8, 10, 11,
        // Bottom face
        12, 13, 14, 12, 14, 15,
        // Right face
        16, 17, 18, 16, 18, 19,
        // Left face
        20, 21, 22, 20, 22, 23
    };

    skyboxMesh.SetVertices(verts);
    skyboxMesh.SetIndices(indices);
}

void Skybox::ApplyTimeEffects() {
    // In a real implementation, this would adjust the sky color based on time of day
    // For example, changing the color gradient from blue (day) to dark blue/purple (night)
    // and adding elements like sun/moon positions
    
    // For now, we'll just adjust the base color slightly
    if (timeOfDay < 0.3 || timeOfDay > 0.7) {
        // Night time - darker sky
        // This would be handled by changing the texture or shader parameters in a real implementation
    } else if (timeOfDay > 0.4 && timeOfDay < 0.6) {
        // Noon - brightest sky
        // This would be handled by changing the texture or shader parameters in a real implementation
    }
}

EnvironmentSystem::EnvironmentSystem() {
}

bool EnvironmentSystem::Initialize() {
    // In a real implementation, this would initialize GPU resources for effects
    return true;
}

bool EnvironmentSystem::SetSkybox(std::shared_ptr<Skybox> skybox) {
    this->skybox = skybox;
    return skybox != nullptr;
}

void EnvironmentSystem::Update(double dt) {
    if (dayNightCycleEnabled) {
        // Update time based on dt and timeScale
        currentTime += dt * timeScale / (24.0 * 60.0 * 60.0);  // Assuming dt is in seconds
        if (currentTime > 1.0) {
            currentTime -= 1.0;  // Wrap around to next day
        }
    }
    
    // Apply time effects to skybox if it exists
    if (skybox) {
        skybox->SetTimeOfDay(currentTime);
    }
}

void EnvironmentSystem::Render(Draw& draw, const Matrix4& viewMatrix, const Matrix4& projectionMatrix, 
                              const Rect& viewport) {
    // Render the skybox first
    if (skybox) {
        skybox->Render(draw, viewMatrix, projectionMatrix, viewport);
    }
    
    // Apply fog effects if enabled
    if (fogEnabled) {
        // In a real implementation, fog would be applied as a post-process effect
        // or handled in the shader. For this implementation, we'll just note it
    }
}

EnvironmentPreset EnvironmentPreset::GetDaytimePreset() {
    EnvironmentPreset preset;
    preset.name = "Daytime";
    preset.fogColor = Color(135, 206, 235);  // Sky blue
    preset.fogDensity = 0.05;
    preset.fogEnabled = false;
    preset.atmosphericScattering = true;
    preset.dayNightCycle = false;
    preset.initialTimeOfDay = 0.5;  // Noon
    return preset;
}

EnvironmentPreset EnvironmentPreset::GetNighttimePreset() {
    EnvironmentPreset preset;
    preset.name = "Nighttime";
    preset.fogColor = Color(25, 25, 112);  // Midnight blue
    preset.fogDensity = 0.1;
    preset.fogEnabled = true;
    preset.atmosphericScattering = false;
    preset.dayNightCycle = false;
    preset.initialTimeOfDay = 0.0;  // Midnight
    return preset;
}

EnvironmentPreset EnvironmentPreset::GetSunsetPreset() {
    EnvironmentPreset preset;
    preset.name = "Sunset";
    preset.fogColor = Color(255, 140, 0);  // Dark orange
    preset.fogDensity = 0.08;
    preset.fogEnabled = true;
    preset.atmosphericScattering = true;
    preset.dayNightCycle = false;
    preset.initialTimeOfDay = 0.8;  // Evening
    return preset;
}

EnvironmentPreset EnvironmentPreset::GetFoggyPreset() {
    EnvironmentPreset preset;
    preset.name = "Foggy";
    preset.fogColor = Color(211, 211, 211);  // Light gray
    preset.fogDensity = 0.3;
    preset.fogEnabled = true;
    preset.atmosphericScattering = false;
    preset.dayNightCycle = false;
    preset.initialTimeOfDay = 0.5;  // Noon
    return preset;
}

END_UPP_NAMESPACE