//
// Created by blomq on 2025-09-12.
//

#include "time_manager/time_manager.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>

#define DEFAULT_PHYSICS_HZ 60
#define DEFAULT_PHYSICS_TIME_STEP (1.0 / DEFAULT_PHYSICS_HZ)
#define DEFAULT_MAX_FRAME_TIME 0.25
#define DEFAULT_MAX_PHYSICS_STEPS 5

struct TimeManager
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

    // Time source
    HighResTimeT (*now)(void);
};

static inline double Clamp(const double x, const double min, const double max)
{
    return fmax(fmin(x, max), min);
}

void UpdateFpsStats(TimeManager* tm, const double frameTime)
{
    tm->fpsAccumulator += frameTime;
    tm->fpsFrameCount++;

    if (tm->fpsAccumulator >= 1.0)
    {
        tm->averageFps = (double)tm->fpsFrameCount / tm->fpsAccumulator;
        tm->fpsAccumulator = 0.0;
        tm->fpsFrameCount = 0;
    }
}

void InitTimeManager(TimeManager* tm)
{
    assert(tm != NULL && "TimeManager pointer is null!");
    tm->physicsHz = DEFAULT_PHYSICS_HZ;
    tm->physicsTimeStep = DEFAULT_PHYSICS_TIME_STEP;
    tm->maxFrameTime = DEFAULT_MAX_FRAME_TIME;
    tm->maxPhysicsSteps = DEFAULT_MAX_PHYSICS_STEPS;
    tm->timeScale = 1.0;
    tm->accumulator = 0.0;
    tm->now = GetHighResolutionTime;
    tm->lastTime = tm->now();
    tm->firstFrame = true;
    tm->physicsStepsThisFrame = 0;
    tm->averageFps = 0.0;
    tm->fpsAccumulator = 0.0;
    tm->fpsFrameCount = 0;
    tm->timeScaleBeforePause = 1.0;
}

TimeManager* TmCreate(void)
{
    TimeManager* tm = calloc(1, sizeof(TimeManager));
    if (tm) { InitTimeManager(tm); }
    return tm;
}

void TmDestroy(TimeManager* tm)
{
    if (tm == NULL) { return; }
    free(tm);
}

FrameTimingData TmBeginFrame(TimeManager* tm)
{
    assert(tm != NULL && "TimeManager pointer is null!");
    if (tm->firstFrame)
    {
        tm->firstFrame = false;
        tm->lastTime = tm->now();
        return (FrameTimingData){
            .physicsSteps = 0,
            .fixedTimestep = tm->physicsTimeStep,
            .interpolationAlpha = 0.0,
            .frameTime = 0.0,
            .lagging = false,
            .rawFrameTime = 0.0,
            .unscaledFrameTime = 0.0,
            .currentTimeScale = tm->timeScale
        };
    }
    assert(tm->physicsTimeStep > 0.0 && "physicsTimeStep must be > 0");

    const HighResTimeT currentTime = tm->now();
    tm->lastTime = currentTime;

    const double deltaTime = fmax((double)(currentTime.nanoseconds - tm->lastTime.nanoseconds) / 1000000000.0, 0.0);
    const double cappedDeltaTime = fmin(deltaTime, tm->maxFrameTime);
    const double scaledFrameTime = cappedDeltaTime * tm->timeScale;
    tm->accumulator += scaledFrameTime;

    const double stepsD = floor((tm->accumulator + 1e-12) / tm->physicsTimeStep);
    const bool lagging = stepsD > (double)tm->maxPhysicsSteps;
    const size_t steps = lagging ? tm->maxPhysicsSteps : (size_t)stepsD;
    tm->physicsStepsThisFrame = steps;

    double remainder = fmod(tm->accumulator, tm->physicsTimeStep);
    if (remainder < 0.0) { remainder += tm->physicsTimeStep; }

    tm->accumulator = remainder;

    // Calculate interpolation alpha
    const double alpha = Clamp(tm->accumulator / tm->physicsTimeStep, 0.0, 1.0);

    UpdateFpsStats(tm, deltaTime);

    return (FrameTimingData){
        .physicsSteps = tm->physicsStepsThisFrame,
        .fixedTimestep = tm->physicsTimeStep,
        .interpolationAlpha = alpha,
        .frameTime = scaledFrameTime,
        .lagging = lagging,
        .rawFrameTime = deltaTime,
        .unscaledFrameTime = cappedDeltaTime,
        .currentTimeScale = tm->timeScale
    };
}

void TmSetPhysicsHz(TimeManager* tm,const size_t physicsHz)
{
    assert(tm != NULL && "TimeManager pointer is null!");
    if (physicsHz == 0)
    {
        return;
    }

    tm->physicsHz = physicsHz;
    tm->physicsTimeStep = 1.0 / (double)physicsHz;
}

void TmSetPhysicsTimeStep(TimeManager* tm, const double physicsTimeStep)
{
    assert(tm != NULL && "TimeManager pointer is null!");
    if (physicsTimeStep <= 0.0)
    {
        return;
    }

    tm->physicsTimeStep = physicsTimeStep;
    tm->physicsHz = (size_t)(1.0 / physicsTimeStep);
}

void TmSetMaxFrameTime(TimeManager* tm,const double maxFrameTime)
{
    assert(tm != NULL && "TimeManager pointer is null!");
    assert(maxFrameTime > 0.0 && "MaxFrameTime must be positive!");
    tm->maxFrameTime = fmax(maxFrameTime, DBL_EPSILON);
}

void TmSetMaxPhysicsSteps(TimeManager* tm,const size_t maxPhysicsSteps)
{
    assert(tm != NULL && "TimeManager pointer is null!");
    tm->maxPhysicsSteps = maxPhysicsSteps > 0 ? maxPhysicsSteps : 1;
}

void TmSetTimeScale(TimeManager* tm,const double timeScale)
{
    assert(tm != NULL && "TimeManager pointer is null!");
    assert(timeScale >= 0.0 && "TimeScale must be non-negative!");
    tm->timeScale = (timeScale < 0.0) ? 0.0 : timeScale;
}

double TmGetAccumulator(const TimeManager* tm)
{
    assert(tm != NULL && "TimeManager pointer is null!");
    return tm->accumulator;
}

double TmGetTimeScale(const TimeManager* tm)
{
    assert(tm != NULL && "TimeManager pointer is null!");
    return tm->timeScale;
}

void TmSetTimeSource(TimeManager* tm, HighResTimeT (*nowFn)(void))
{
    assert(tm != NULL && "TimeManager pointer is null!");
    assert(nowFn != NULL && "nowFn pointer is null!");
    tm->now = nowFn;
    tm->lastTime = tm->now();
}

double TmGetPhysicsTimeStep(const TimeManager* tm)
{
    assert(tm != NULL && "TimeManager pointer is null!");
    return tm->physicsTimeStep;
}

double TmGetMaxFrameTime(const TimeManager* tm)
{
    assert(tm != NULL && "TimeManager pointer is null!");
    return tm->maxFrameTime;
}

size_t TmGetMaxPhysicsSteps(const TimeManager* tm)
{
    assert(tm != NULL && "TimeManager pointer is null!");
    return tm->maxPhysicsSteps;
}

size_t TmGetPhysicsHz(const TimeManager* tm)
{
    assert(tm != NULL && "TimeManager pointer is null!");
    return tm->physicsHz;
}

double TmGetAverageFps(const TimeManager* tm)
{
    assert(tm != NULL && "TimeManager pointer is null!");
    return tm->averageFps;
}

size_t TmGetPhysicsSteps(const TimeManager* tm)
{
    assert(tm != NULL && "TimeManager pointer is null!");
    return tm->physicsStepsThisFrame;
}

void TmReset(TimeManager* tm)
{
    assert(tm != NULL && "TimeManager pointer is null!");
    tm->firstFrame = true;
    tm->accumulator = 0.0;
    tm->lastTime = tm->now();
    tm->physicsStepsThisFrame = 0;
    tm->fpsAccumulator = 0.0;
    tm->fpsFrameCount = 0;
    tm->timeScale = 1.0;
    tm->averageFps = 0.0;
}

void TmPause(TimeManager* tm)
{
    assert(tm != NULL && "TimeManager pointer is null!");
    tm->timeScaleBeforePause = tm->timeScale;
    tm->timeScale = 0.0;
}

void TmResume(TimeManager* tm)
{
    assert(tm != NULL && "TimeManager pointer is null!");
    tm->timeScale = tm->timeScaleBeforePause;
}

bool TmIsPaused(const TimeManager* tm)
{
    assert(tm != NULL && "TimeManager pointer is null!");
    return fabs(tm->timeScale) <= DBL_EPSILON;
}
