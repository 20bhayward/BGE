# Hierarchy Panel Polish Issues

## Critical Issues (High Priority)

### 1. Performance Problems
- **No caching system**: The panel rebuilds the entire hierarchy every frame
- **No dirty flagging**: Updates even when nothing has changed
- **Inefficient entity queries**: Calls GetAllEntityIDs() on every frame
- **String operations in render loop**: Creates new strings for every visible entity

### 2. Incomplete Functionality
- **Broken undo/redo**: Only works for rename operations, not create/delete/reparent
- **Missing TODO implementations**: Many menu items just call CreateEmpty()
- **Select Children not implemented**: Menu option exists but does nothing
- **No proper entity creation**: Most "create" options don't actually create the correct entity type

### 3. UI/UX Issues
- **Poor button labels**: "O" for visibility, "L" for lock - not intuitive
- **Hardcoded sizes**: Button widths (20px, 30px) don't scale with UI
- **ASCII icons**: Using [*], [O], [P] instead of proper icons
- **Missing tooltips**: No hover help for most operations

### 4. Type Safety and Memory Issues
- **Static clipboard**: Could cause issues with multiple panels
- **Fixed-size buffers**: Rename and search use 256-byte buffers
- **Unsafe event types**: Local FocusCameraEvent won't work with other systems
- **No validation**: Missing null checks in drag & drop operations

## Medium Priority Issues

### 5. Missing Features
- No prefab support
- No layer/tag filtering
- No sorting options
- No multi-selection batch operations
- No entity templates
- No hierarchy search highlighting

### 6. Code Quality
- Duplicate component copying code
- No generic component cloning
- Hardcoded world center position (1024, 1024, 0)
- Material ID generation uses simple hash (collision risk)

## Low Priority Issues

### 7. Visual Polish
- No hierarchy connection lines
- No entity previews/thumbnails
- No custom entity colors/highlights
- Limited drag preview information

### 8. Configuration
- Hardcoded double-click time (0.3s)
- Hardcoded drop zone threshold (30%)
- Hardcoded undo history limit (50)
- No user preferences for visual settings

## Recommended Fixes

1. **Implement caching and dirty flagging** to improve performance
2. **Complete all TODO items** for proper entity creation
3. **Fix undo/redo system** to support all operations
4. **Replace ASCII icons** with IconManager system
5. **Add proper tooltips** for all buttons and operations
6. **Implement select children** functionality
7. **Add validation** to all drag & drop operations
8. **Create proper event types** in Events.h instead of local structs
9. **Make UI values configurable** instead of hardcoded
10. **Add generic component cloning** to reduce code duplication