//
// Created by blomq on 2025-09-12.
//

#include "time_manager/time_manager.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>

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

void InitTimeManager(TimeManager* tm)
{
    assert(tm != NULL && "TimeManager pointer is null!");
    tm->physicsHz = 60;
    tm->physicsTimeStep = 1.0 / tm->physicsHz;
    tm->maxFrameTime = 0.25;
    tm->maxPhysicsSteps = 5;
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

    const HighResTimeT currentTime = tm->now();
    const double deltaTime = (currentTime.nanoseconds - tm->lastTime.nanoseconds) / 1000000000.0;
    const double cappedDeltaTime = fmin(deltaTime, tm->maxFrameTime);
    tm->lastTime = currentTime;

    const double scaledFrameTime = cappedDeltaTime * tm->timeScale;
    tm->accumulator += scaledFrameTime;
    tm->physicsStepsThisFrame = 0;

    while (tm->accumulator >= tm->physicsTimeStep && tm->physicsStepsThisFrame < tm->
        maxPhysicsSteps)
    {
        ++tm->physicsStepsThisFrame;
        tm->accumulator -= tm->physicsTimeStep;
    }

    const bool lagging = tm->accumulator >= tm->physicsTimeStep;
    if (lagging)
    {
        // If we're lagging, keep only the remainder of a timestep
        tm->accumulator = fmod(tm->accumulator, tm->physicsTimeStep);
    }

    // Calculate interpolation alpha
    const double alpha = tm->accumulator / tm->physicsTimeStep;

    UpdateFpsStats(tm, cappedDeltaTime);

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

void TmSetMaxFrameTime(TimeManager* tm,const double maxFrameTime)
{
    assert(tm != NULL && "TimeManager pointer is null!");
    tm->maxFrameTime = maxFrameTime;
}

void TmSetMaxPhysicsSteps(TimeManager* tm,const size_t maxPhysicsSteps)
{
    assert(tm != NULL && "TimeManager pointer is null!");
    tm->maxPhysicsSteps = maxPhysicsSteps;
}

void TmSetTimeScale(TimeManager* tm,const double timeScale)
{
    assert(tm != NULL && "TimeManager pointer is null!");
    tm->timeScale = timeScale;
}

double TmGetAccumulatedTime(const TimeManager* tm)
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

double TmGetFrameTime(const TimeManager* tm)
{
    assert(tm != NULL && "TimeManager pointer is null!");
    return tm->accumulator;
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
    return fabs(tm->timeScale) < 0.000001;
}
