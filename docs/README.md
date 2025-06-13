# BGE (Blacksmithing Game Engine) Documentation

Welcome to the BGE documentation. This engine is designed to support the development of a 2D pixel-art blacksmithing game with advanced physics simulation and material interactions.

## 📚 Documentation Structure

### Core Architecture
- [Engine Architecture Overview](architecture/engine-overview.md)
- [Service Locator System](architecture/service-locator.md)
- [Event Bus System](architecture/event-bus.md)
- [Entity-Component System](architecture/entity-component-system.md)
- [Configuration Management](architecture/configuration.md)
- [Logging System](architecture/logging.md)

### Engine Systems
- [Rendering System](systems/rendering.md)
- [Simulation & Physics](systems/simulation.md)
- [Audio System](systems/audio.md)
- [Input Management](systems/input.md)
- [Asset Pipeline](systems/assets.md)
- [UI System](systems/ui.md)

### Game Features
- [Blacksmithing System](gameplay/blacksmithing.md)
- [Material Properties](gameplay/materials.md)
- [Combat System](gameplay/combat.md)
- [World Generation](gameplay/world-generation.md)
- [AI Systems](gameplay/ai.md)

### Tools & Editors
- [Material Editor](tools/material-editor.md)
- [World Editor](tools/world-editor.md)
- [VFX Editor](tools/vfx-editor.md)
- [Asset Tools](tools/asset-tools.md)

### Development
- [Building the Engine](development/building.md)
- [Contributing Guidelines](development/contributing.md)
- [Code Style Guide](development/code-style.md)
- [Testing Framework](development/testing.md)

## 🚀 Quick Start

1. Clone the repository
2. Review [Building the Engine](development/building.md)
3. Run the InteractiveEditor example
4. Read [Engine Architecture Overview](architecture/engine-overview.md)

## 📋 Current Status

✅ **Phase 1 Complete: Core Architecture Refactoring**
- Service Locator Pattern implemented
- Event Bus system operational
- Centralized logging system
- Data-driven configuration
- Entity-Component foundation
- Refactored Engine class

🎯 **Phase 2: Advanced Engine Features** (Next)
- Enhanced ECS system
- Advanced rendering pipeline
- Material system improvements
- AI framework

🔮 **Phase 3: Game Features** (Future)
- Blacksmithing mechanics
- Advanced physics simulation
- World generation
- Combat system

## 🎮 Examples

- `Examples/InteractiveEditor/` - Material editing and world interaction
- `Examples/BasicSandbox/` - Simple physics demonstration
- More examples coming in Phase 2

## 🛠️ Key Features

- **Modular Architecture**: Service-based system design
- **Event-Driven**: Decoupled component communication
- **Data-Driven**: Configuration-based engine behavior
- **Extensible**: Plugin-friendly component system
- **Developer Friendly**: Comprehensive logging and debugging tools