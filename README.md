[![CMake on multiple platforms](https://github.com/Psychloor/time_manager/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/Psychloor/time_manager/actions/workflows/cmake-multi-platform.yml)

# Time Manager

A robust C library for game timing and physics simulation, implementing the fixed timestep with interpolation pattern from Glenn Fiedler's seminal article ["Fix Your Timestep!"](https://gafferongames.com/post/fix_your_timestep/).

## Features

- **Fixed timestep physics** - Deterministic physics simulation independent of frame rate
- **Interpolation support** - Smooth rendering between physics updates
- **Frame time capping** - Prevents spiral of death in low-performance scenarios
- **Time scaling** - Support for slow-motion and fast-forward effects
- **Pause/Resume** - Built-in game pause functionality
- **Performance metrics** - FPS tracking and timing statistics
- **Cross-platform** - Works on Windows (QueryPerformanceCounter) and POSIX systems (clock_gettime)

## Why Fixed Timestep?

Variable timestep physics can lead to:
- Non-deterministic behavior (different results on different machines)
- Numerical instability at low frame rates
- Difficulty in networking and replay systems

This library implements the widely accepted solution: running physics at a fixed rate while rendering at any framerate, using interpolation to maintain visual smoothness.

## Thread Safety

**The Time Manager library is not thread-safe.** Each `TimeManager` instance maintains mutable state and should only be accessed from a single thread. If you need timing functionality across multiple threads, use one of these approaches:

1. **Separate instances** - Create a separate `TimeManager` for each thread that needs timing
2. **Single-threaded timing** - Keep all timing logic in your main game loop thread
3. **Manual synchronization** - If you must share a `TimeManager`, protect all function calls with mutexes

**Recommended approach:** Most games use a single-threaded main loop for timing and physics, with only rendering or asset loading on separate threads. This library is designed for this common pattern.
```c
// Good: Each thread has its own TimeManager
void* physics_thread(void* arg) {
    TimeManager* tm = TmCreate();
    // ... use tm only in this thread
}

void* ai_thread(void* arg) {
    TimeManager* tm = TmCreate();
    // ... use tm only in this thread
}

// Also good: Single-threaded game loop
int main() {
    TimeManager* tm = TmCreate();
    
    while (running) {
        FrameTimingData frame = TmBeginFrame(tm);
        // All timing happens on main thread
    }
}
```
## Building

### Using CMake
```bash
mkdir build
cd build
cmake ..
cmake --build .
```
### Options

- `TIME_MANAGER_BUILD_SHARED` - Build as shared library (default: ON)

### Installation
```bash
cmake --install . --prefix /your/install/path
```
## Quick Example
```c
#include <time_manager/time_manager.h>
#include <stdio.h>

// Game state for interpolation
typedef struct {
    float x, y;
    float prev_x, prev_y;
} GameObject;

void physics_update(GameObject* obj, double dt) {
    // Save previous state for interpolation
    obj->prev_x = obj->x;
    obj->prev_y = obj->y;
    
    // Update physics (example: simple movement)
    obj->x += 100.0f * dt;  // Move 100 units per second
    obj->y += 50.0f * dt;
}

void render(GameObject* obj, double alpha) {
    // Interpolate between previous and current state
    float render_x = obj->prev_x + (obj->x - obj->prev_x) * alpha;
    float render_y = obj->prev_y + (obj->y - obj->prev_y) * alpha;
    
    printf("Render at: %.2f, %.2f\n", render_x, render_y);
    // Your actual rendering code here...
}

int main() {
    // Initialize time manager
    TimeManager* tm = TmCreate();
    
    // Optional: Configure physics rate (default is 60 Hz)
    TmSetPhysicsHz(tm, 120);  // 120 physics updates per second
    
    GameObject player = {0};
    
    // Game loop
    while (running) {
        // Begin frame and get timing data
        FrameTimingData frame = TmBeginFrame(tm);
        
        // Perform fixed timestep physics updates
        for (size_t i = 0; i < frame.physicsSteps; i++) {
            physics_update(&player, frame.fixedTimestep);
        }
        
        // Render with interpolation
        render(&player, frame.interpolationAlpha);
        
        // Display stats
        if (frame.lagging) {
            printf("Warning: Can't keep up with physics rate!\n");
        }
        printf("FPS: %.1f\n", TmGetAverageFps(tm));
        
        // Your frame limiting/vsync here...
    }
    
    // Free memory
    TmDestroy(tm);
    
    return 0;
}
```
## Core Concepts

### Fixed Timestep
Physics runs at a constant rate (e.g., 60 Hz) regardless of rendering framerate. This ensures deterministic, stable simulation.

### Accumulator Pattern
The library accumulates frame time and computes how many fixed-size physics steps to run each frame:
```
accumulator += frameTime
stepsD = floor((accumulator + epsilon) / physicsTimestep)
lagging = stepsD > maxPhysicsSteps
steps = lagging ? maxPhysicsSteps : stepsD

remainder = fmod(accumulator, physicsTimestep)
if lagging:
    accumulator = remainder         # preserve only the < dt remainder
else:
    accumulator -= steps * physicsTimestep

alpha = accumulator / physicsTimestep
```
This is equivalent to the traditional while-loop approach but computed in constant time. A tiny epsilon helps avoid missing a step due to floating-point rounding at exact boundaries.

### Interpolation
To maintain smooth visuals, positions are interpolated between physics states:
```
alpha = accumulator / physicsTimestep
renderPosition = previousState + (currentState - previousState) * alpha
```
### Spiral of Death Prevention
Limits maximum physics steps per frame. When clamped, backlog beyond a single-step remainder is discarded, preventing the simulation from getting stuck trying to catch up.

## API Reference

### Initialization
```c
TimeManager* tm = TmCreate(); // Initialize with defaults (60 Hz physics), NULL if memory allocation fails
TmDestroy(tm);  // Free memory allocated for TimeManager;
```
### Frame Processing
```c
FrameTimingData frame = TmBeginFrame(tm);
// Returns structure with:
// - physicsSteps: Number of physics updates to perform
// - fixedTimestep: Duration of each physics step
// - interpolationAlpha: Interpolation factor [0,1]
// - frameTime: Scaled frame time
// - lagging: True if can't maintain physics rate
// - rawFrameTime: Actual frame time before capping
// - unscaledFrameTime: Frame time before scaling
// - currentTimeScale: Active time scale factor
```
### Configuration
```c
TmSetPhysicsHz(tm, 120);        // Set physics rate (Hz)
TmSetMaxFrameTime(tm, 0.25);    // Max frame time cap (seconds)
TmSetMaxPhysicsSteps(tm, 5);    // Max physics steps per frame
TmSetTimeScale(tm, 0.5);        // Time scaling (0.5 = half speed)
```
### Pause/Resume
```c
TmPause(tm);                     // Pause time progression
TmResume(tm);                    // Resume from pause
bool paused = TmIsPaused(tm);   // Check pause state
```
### Monitoring
```c
double fps = TmGetAverageFps(tm);           // Average FPS
size_t steps = TmGetPhysicsSteps(tm);       // Physics steps last frame
double alpha = TmGetInterpolationAlpha(tm); // Current interpolation factor
```
### Reset
```c
TmReset(&tm);  // Reset all timing data
```
## Best Practices

1. **Store previous state** - Keep previous physics state for interpolation
2. **Separate update and render** - Don't mix physics and rendering logic
3. **Choose appropriate physics rate** - 60 Hz is common, 30 Hz for mobile, 120+ Hz for competitive games
4. **Handle lag gracefully** - Detect when `lagging` is true and adjust quality settings
5. **Use time scaling carefully** - Useful for pause menus, slow-motion effects, and debugging
6. **Single-threaded timing** - Keep timing logic on your main game loop thread

## Common Physics Rates

- **30 Hz** - Mobile games, turn-based games
- **60 Hz** - Standard for most games
- **120 Hz** - Competitive games, fighting games
- **240 Hz** - High-precision simulations

## Troubleshooting

### Jittery movement
- Ensure you're using interpolation (`interpolationAlpha`) in rendering
- Store and interpolate between previous and current physics states

### Simulation runs too fast/slow
- Check time scale isn't modified: `TmGetTimeScale(&tm)`
- Verify physics timestep matches your physics engine expectations

### "Lagging" frequently true
- Reduce physics rate or optimize physics code
- Increase `maxPhysicsSteps` (but beware of spiral of death)
- Consider dynamic quality adjustments

## References

- [Fix Your Timestep!](https://gafferongames.com/post/fix_your_timestep/) - Glenn Fiedler
- [Game Programming Patterns - Game Loop](https://gameprogrammingpatterns.com/game-loop.html) - Robert Nystrom
- [Timesteps and Achieving Smooth Motion](https://www.kinematicsoup.com/news/2016/8/9/rrypp5tkubynjwxhxjzd42s3o034o8) - Kinematics Soup

## License

Licensed under the MIT License. See LICENSE for more information.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.