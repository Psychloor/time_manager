//
// Created by blomq on 2025-09-12.
//

#ifndef TIMEMANAGER_TIME_MANAGER_H
#define TIMEMANAGER_TIME_MANAGER_H

#include <stdbool.h>
// ReSharper disable once CppUnusedIncludeDirective
#include <stddef.h>

#include "time_manager/time_manager_export.h"

#include "utils/time_utils.h"

// @formatter:off
#ifdef __cplusplus
extern "C" {
#endif
// @formatter:on

typedef struct
{
    /**
     * @brief Represents the number of physics simulation steps that occurred during the current frame.
     *
     * This variable stores the count of physics steps executed in a frame to integrate physics updates.
     * It is calculated within the game's timing system, ensuring a consistent and deterministic physics
     * simulation independent of frame rendering times.
     */
    size_t physicsSteps;
    /**
     * Fixed timestep duration in seconds used to manage the simulation's
     * physics updates at a consistent rate. This ensures stability in
     * the physics calculations and a predictable simulation behavior,
     * regardless of varying frame rates.
     */
    double fixedTimestep;
    /**
     * Represents the interpolation factor used to calculate the
     * interpolation state between the previous and current physics step.
     *
     * This value is calculated as the ratio of the time accumulator to
     * the fixed simulation timestep. It is used for rendering
     * interpolated frames to achieve smooth visuals during the physics update loop.
     *
     * Range: [0.0, 1.0] where 0.0 indicates the previous state (before the
     * next physics step) and 1.0 indicates the current state (at the next
     * physics step).
     */
    double interpolationAlpha;
    /**
     * Represents the time taken to render the last frame in seconds.
     *
     * This value is updated every frame and provides a precise measurement
     * of the duration for the most recent frame in the rendering or simulation process.
     * It is influenced by the current timescale and is capped by a maximum frame time
     * to ensure stability in the simulation.
     *
     * Useful for real-time performance tracking, frame rate calculation,
     * and game loop timing adjustments.
     */
    double frameTime;
    /**
     * Indicates whether the timing system is lagging behind the fixed physics update cycle.
     *
     * When `lagging` is true, this means that the accumulator in the timing system has exceeded
     * the defined timestep for physics updates, resulting in the system not being able to keep
     * up with the desired update rate. Conversely, when `lagging` is false, the system is operating
     * within the bounds of the fixed timestep.
     *
     * This flag is used by the timing system to manage physics update steps and ensure a smooth
     * experience by potentially skipping or adjusting updates when the system falls behind.
     */
    bool lagging;
    /**
     * @brief Represents the raw measured time of a single frame, in seconds.
     *
     * This value is calculated as the unmodified delta time between two frames
     * before applying any scaling or limiting constraints. It provides an accurate
     * representation of the real-world time elapsed during a frame and is useful
     * for debugging or analyzing frame performance without the influence of
     * scaling or capping mechanisms.
     */
    double rawFrameTime;
    /**
     * Stores the duration of the current frame in seconds, without applying any time scaling.
     *
     * This value represents the raw, capped delta time for the frame, independent of the current timescale.
     * It is used in frame timing calculations to ensure the timing logic remains consistent regardless
     * of any modifications to the global timescale.
     */
    double unscaledFrameTime;
    /**
     * Represents the current timescale factor applied to the simulation.
     *
     * This value determines the rate at which time progresses in the simulation.
     * A value of 1.0 indicates normal simulation speed, values greater than 1.0
     * result in faster simulation speeds, and values between 0.0 and 1.0 slow
     * down the simulation. A value of 0.0 effectively pauses the simulation.
     */
    double currentTimeScale;
} FrameTimingData;

typedef struct
{
    size_t physicsHz;
    double physicsTimeStep;
    double maxFrameTime;
    size_t maxPhysicsSteps;

    double accumulator;
    HighResTimeT lastTime;

    bool firstFrame;
    double timeScale;

    double timeScaleBeforePause;

    // Debug Stats
    size_t physicsStepsThisFrame;
    double averageFps;

    // Average
    double fpsAccumulator;
    size_t fpsFrameCount;
} TimeManager;

/**
 * @brief Initializes the time management system with default parameters.
 *
 * This function sets up the core parameters for managing time in the application,
 * including physics update rates, maximum allowable frame time, and scaling factors.
 * It resets the accumulator for frame timing, retrieves the initial high-resolution
 * timestamp, and prepares the time manager's state for the first frame of the application.
 *
 * @param tm A pointer to the TimeManager structure that will be initialized
 * with default values, including physics step settings and time-related statistics.
 */
TIME_MANAGER_API void InitTimeManager(TimeManager* tm);

/**
 * @brief Initializes and manages the timing tm for a new frame.
 *
 * This function handles the timing calculations necessary for synchronizing the game loop.
 * It computes the interpolated frame timing, physics step integration, and updates the frame
 * timing statistics. Additionally, it ensures that the simulation remains consistent even
 * during frame rate fluctuations by capping and scaling time deltas.
 *
 * @param tm Pointer to the TimeManager structure containing timing-related configuration
 *             and runtime values.
 * @return FrameTimingData structure containing the results of the current frame's timing calculations,
 *         including the number of physics steps performed, frame time, and interpolation values.
 */
TIME_MANAGER_API FrameTimingData TmBeginFrame(TimeManager* tm);

/**
 * @brief Sets the physics update frequency and calculates the corresponding time step.
 *
 * This function configures the number of physics updates per second for the given
 * TimeManager instance and updates the physics time step value based on the frequency.
 * It ensures the physics simulation maintains a consistent update interval.
 *
 * @param tm A pointer to the TimeManager structure that holds timing and physics-related settings.
 * @param physicsHz The desired physics update frequency, specified in hertz (updates per second).
 *                  Must be greater than zero to take effect.
 */
TIME_MANAGER_API void TmSetPhysicsHz(TimeManager* tm, size_t physicsHz);

/**
 * @brief Sets the physics time step for the physics simulation within the time manager.
 *
 * Updates the time step interval used for advancing the physics simulation. The configured
 * time step determines the simulated time duration for each physics update. This function
 * ensures that the time step value in the TimeManager structure is updated correctly.
 *
 * @param tm Pointer to the TimeManager structure containing the time management settings.
 * @param physicsTimeStep The new physics time step interval, represented in seconds.
 */
TIME_MANAGER_API void TmSetPhysicsTimeStep(TimeManager* tm, double physicsTimeStep);

/**
 * @brief Sets the maximum allowable frame time for the physics system.
 *
 * Updates the maximum frame time value within the provided TimeManager structure.
 * This value is used to cap the duration of a single frame to prevent excessively long
 * physics updates, ensuring stability in the simulation.
 *
 * @param tm Pointer to the TimeManager structure being updated.
 * @param maxFrameTime Maximum frame time (in seconds) to be set.
 */
TIME_MANAGER_API void TmSetMaxFrameTime(TimeManager* tm, double maxFrameTime);

/**
 * @brief Sets the maximum number of physics simulation steps allowed per frame.
 *
 * This function updates the TimeManager structure to define the upper limit on the
 * number of physics simulation steps that can be performed within a single frame.
 * Limiting the physics steps ensures that simulations do not run excessively in
 * scenarios where rendering lag occurs.
 *
 * @param tm A pointer to the TimeManager structure that manages timing and physics integration.
 * @param maxPhysicsSteps The maximum number of physics steps permitted per frame.
 */
TIME_MANAGER_API void TmSetMaxPhysicsSteps(TimeManager* tm, size_t maxPhysicsSteps);

/**
 * @brief Sets the timescale for the time manager.
 *
 * Adjusts the time progression multiplier for the simulation. A timescale of 1.0 indicates normal speed,
 * values greater than 1.0 speed-up time, and values less than 1.0 slow down time.
 *
 * @param tm A pointer to the TimeManager structure that contains the current state of the time manager.
 * @param timeScale The new timescale to set. Determines the multiplier applied to the time progression.
 */
TIME_MANAGER_API void TmSetTimeScale(TimeManager* tm, double timeScale);

/**
 * @brief Retrieves the accumulated time stored in the time manager.
 *
 * This function accesses the `accumulator` value from the provided
 * `TimeManager` structure, which represents the accumulated
 * time in the simulation.
 *
 * @param tm A pointer to the `TimeManager` structure containing
 *             the accumulated time information.
 * @return The accumulated time as a double value.
 */
TIME_MANAGER_API double TmGetAccumulatedTime(const TimeManager* tm);

/**
 * @brief Retrieves the timescale factor for the simulation.
 *
 * This function returns the current timescale factor stored in the TimeManager structure.
 * The timescale factor determines the rate at which in-game time progresses relative to real-world time.
 *
 * @param tm Pointer to the TimeManager structure containing the timescale value.
 * @return The current timescale factor as a double.
 */
TIME_MANAGER_API double TmGetTimeScale(const TimeManager* tm);

/**
 * @brief Retrieves the physics time step value from the given TimeManager structure.
 *
 * This function provides access to the fixed time step value used for the physics simulation.
 * The physics time step determines the interval at which physics updates are processed, ensuring
 * a consistent simulation rate.
 *
 * @param tm Pointer to the TimeManager structure containing the simulation timings and properties.
 * @return The fixed physics time step as a double.
 */
TIME_MANAGER_API double TmGetPhysicsTimeStep(const TimeManager* tm);

/**
 * @brief Retrieves the maximum frame time defined in the TimeManager structure.
 *
 * This function returns the value of the `maxFrameTime` field from the provided
 * TimeManager instance. The maximum frame time determines the upper limit for
 * the duration of a frame, ensuring stability in time-dependent calculations.
 *
 * @param tm A pointer to the TimeManager structure containing the time-related settings and properties.
 * @return The maximum frame time in seconds as a double-precision floating-point value.
 */
TIME_MANAGER_API double TmGetMaxFrameTime(const TimeManager* tm);

/**
 * @brief Retrieves the maximum number of physics steps allowed in the simulation.
 *
 * This function accesses the maximum number of physics simulation steps that can be performed
 * within a single frame, as defined in the given TimeManager instance. This value helps
 * ensure the physics engine does not perform excessive update steps during a single game update.
 *
 * @param tm A pointer to a TimeManager structure holding the physics simulation settings.
 * @return The maximum number of physics steps allowed in one frame.
 */
TIME_MANAGER_API size_t TmGetMaxPhysicsSteps(const TimeManager* tm);

/**
 * @brief Retrieves the physics simulation frequency from the TimeManager structure.
 *
 * This function provides access to the physics update rate (in Hertz) as stored
 * within the given TimeManager instance. It reflects the number of physics
 * updates that are intended to occur per second.
 *
 * @param tm A pointer to the TimeManager structure containing the physicsHz value.
 * @return The physics simulation frequency in Hertz.
 */
TIME_MANAGER_API size_t TmGetPhysicsHz(const TimeManager* tm);

/**
 * @brief Retrieves the average frames per second (FPS) from the TimeManager structure.
 *
 * This function provides the average FPS calculated over time, as stored within the TimeManager.
 * The average FPS serves as a performance metric, reflecting the rendering efficiency of the application.
 *
 * @param tm Pointer to the TimeManager structure containing the average FPS value.
 * @return The average frames per second as a double.
 */
TIME_MANAGER_API double TmGetAverageFps(const TimeManager* tm);

/**
 * @brief Retrieves the accumulated frame time from the TimeManager structure.
 *
 * This function returns the value of the accumulator stored in the provided
 * TimeManager instance, representing the total time accumulated during the
 * current physics frame. The accumulator is used to track the time progression
 * for managing physics simulations.
 *
 * @param tm A pointer to the TimeManager structure containing the
 *             accumulator value and other timing-related information.
 *             Must not be null.
 * @return The accumulated frame time as a double precision floating-point value.
 */
TIME_MANAGER_API double TmGetFrameTime(const TimeManager* tm);

/**
 * @brief Retrieves the number of physics simulation steps executed during the current frame.
 *
 * This function accesses a field in the provided TimeManager structure that tracks
 * the count of physics steps performed within the latest frame. It is used for ensuring
 * synchronization with the physics timing system in the application's update cycle.
 *
 * @param tm Pointer to the TimeManager structure containing timing and simulation information.
 * @return The number of physics steps executed during the current frame.
 */
TIME_MANAGER_API size_t TmGetPhysicsSteps(const TimeManager* tm);

/**
 * @brief Resets the state of the TimeManager instance to its initial values.
 *
 * This function initializes or resets various fields in the provided TimeManager structure,
 * such as resetting frame counters, accumulators, time tracking, and statistics for physics updates
 * and frame rate calculations. Ensures a clean and consistent start for the time management system.
 *
 * @param tm A pointer to the TimeManager structure to be reset.
 */
TIME_MANAGER_API void TmReset(TimeManager* tm);

/**
 * @brief Pauses the time management system by setting the timescale to zero.
 *
 * This function temporarily halts the progression of time updates by saving the current timescale
 * and then setting it to zero, effectively pausing the time simulation. It modifies the given
 * TimeManager structure to reflect the paused state.
 *
 * @param tm A pointer to the TimeManager structure that holds the current state of the
 *             time management system. This structure is updated to reflect the paused state.
 */
TIME_MANAGER_API void TmPause(TimeManager* tm);

/**
 * @brief Resumes the time manager by restoring the timescale to its value before a pause.
 *
 * This function is used to resume the normal operation of the time manager after a pause
 * by setting the timescale back to its state before pausing.
 *
 * @param tm Pointer to the TimeManager structure containing the time manager's state.
 */
TIME_MANAGER_API void TmResume(TimeManager* tm);


/**
 * @brief Determines if the TimeManager is currently paused.
 *
 * This function checks the time scaling factor within the provided TimeManager instance
 * to identify whether the simulation is paused. A very small or near-zero value
 * for the timescale signifies that the simulation is paused.
 *
 * @param tm A pointer to the TimeManager instance to check.
 * @return True if the TimeManager is paused; otherwise, false.
 */
TIME_MANAGER_API bool TmIsPaused(const TimeManager* tm);

#ifdef __cplusplus
}
#endif

#endif //TIMEMANAGER_TIME_MANAGER_H
