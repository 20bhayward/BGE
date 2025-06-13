# BGE (Breaking Ground Engine)

A high-performance 2D falling sand game engine with modern graphics capabilities including 2D raytracing and global illumination.

## Features

### Core Engine
- **High-Performance Simulation**: Optimized cellular automata with data-oriented design
- **Modern Graphics**: 2D raytracing, global illumination, dynamic lighting
- **Cross-Platform**: Windows, Linux, macOS support
- **Multi-Threading**: Automatic workload distribution across CPU cores
- **GPU Acceleration**: Optional compute shader acceleration for simulation and lighting

### Developer Features
- **Simple API**: Easy-to-use C++ interface for game development
- **Material System**: Powerful material creation with physical and optical properties
- **Scripting Support**: Lua scripting for game logic and modding
- **Integrated Editor**: Visual tools for material creation and world design
- **Asset Pipeline**: Streamlined content creation workflow

### Graphics & Rendering
- **2D Raytracing**: Real-time light simulation with reflections and refractions
- **Global Illumination**: Multi-bounce lighting for realistic scenes
- **Material Shaders**: Custom shaders for different material types
- **Post-Processing**: Bloom, color grading, heat distortion effects
- **Dynamic Lighting**: Point lights, spot lights, area lights

## Quick Start

### Basic Sandbox Example
```cpp
#include "BGE/BGE.h"
using namespace BGE;

class MyGame : public Application {
public:
    bool Initialize() override {
        // Create materials
        auto sand = GetMaterialSystem()->CreateMaterial("Sand")
            .SetColor(200, 180, 120)
            .SetBehavior(MaterialBehavior::Powder);
            
        auto fire = GetMaterialSystem()->CreateMaterial("Fire")
            .SetColor(255, 100, 0)
            .SetEmission(2.0f)
            .SetBehavior(MaterialBehavior::Fire);
            
        return true;
    }
    
    void Update(float deltaTime) override {
        GetWorld()->Update(deltaTime);
    }
};

int main() {
    EngineConfig config;
    config.appName = "My Falling Sand Game";
    
    Engine::Instance().Initialize(config);
    Engine::Instance().Run(std::make_unique<MyGame>());
    return 0;
}
```

## Building

### Prerequisites
- CMake 3.20+
- C++20 compatible compiler
- Vulkan SDK (optional, for advanced graphics)
- OpenGL 4.5+ (fallback renderer)

### Build Steps
```bash
git clone https://github.com/your-repo/BGE.git
cd BGE
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Build Options
- `BGE_BUILD_EXAMPLES=ON` - Build example projects
- `BGE_BUILD_EDITOR=ON` - Build integrated editor
- `BGE_USE_VULKAN=ON` - Enable Vulkan renderer
- `BGE_USE_OPENGL=ON` - Enable OpenGL fallback

## Architecture

### Engine Modules
```
BGE/
├── Core/           # Engine foundation (math, memory, threading)
├── Simulation/     # Cellular automata and physics simulation
├── Renderer/       # Modern graphics pipeline with raytracing
├── Audio/          # 3D spatial audio system
├── Scripting/      # Lua scripting interface
├── Editor/         # Integrated development environment
├── AssetPipeline/  # Content creation and asset processing
└── Runtime/        # Game packaging and distribution
```

### Performance Features
- **Data-Oriented Design**: Optimized memory layout for cache efficiency
- **Spatial Partitioning**: Chunk-based world updates
- **Multi-Threading**: Parallel simulation and rendering
- **GPU Compute**: Offload intensive calculations to GPU
- **LOD System**: Automatic quality scaling

## Documentation

- [Getting Started Guide](Docs/GettingStarted.md)
- [API Reference](Docs/API.md)
- [Material System](Docs/Materials.md)
- [Rendering Pipeline](Docs/Rendering.md)
- [Editor Tools](Docs/Editor.md)
- [Performance Guide](Docs/Performance.md)

## Examples

- **BasicSandbox**: Simple falling sand simulation
- **LightingDemo**: 2D raytracing and global illumination showcase
- **MaterialEditor**: Interactive material creation tool
- **PhysicsDemo**: Rigid body integration with particle simulation
- **ProceduralWorld**: Procedural world generation example

## License

MIT License - see [LICENSE](LICENSE) for details.

## Contributing

Contributions welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

To run: cmake .. 
` -DCMAKE_TOOLCHAIN_FILE=G:/vcpkg/scripts/buildsystems/vcpkg.cmake -G "Visual Studio 17 2022" -A x64`
` cmake --build . --config Release --target InteractiveEditor`
