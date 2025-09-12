#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "time_manager/time_manager.h"  // pulls in utils/time_utils.h

// ---------- tiny assert helpers ----------
#define ASSERT_TRUE(x) do { if (!(x)) { \
  fprintf(stderr,"ASSERT_TRUE failed: %s:%d: %s\n", __FILE__, __LINE__, #x); \
  return 1; } } while (0)

#define ASSERT_EQ_SIZE(a,b) do { size_t _aa=(a), _bb=(b); if (_aa!=_bb) { \
  fprintf(stderr,"ASSERT_EQ_SIZE failed: %s:%d: %zu != %zu\n", __FILE__, __LINE__, _aa, _bb); \
  return 1; } } while (0)

#define ASSERT_NEAR(a,b,eps) do { double _aa=(a), _bb=(b), _ee=(eps); \
  if (fabs(_aa-_bb) > _ee) { \
    fprintf(stderr,"ASSERT_NEAR failed: %s:%d: %.17g vs %.17g (eps=%.1e)\n", \
            __FILE__, __LINE__, _aa, _bb, _ee); \
    return 1; } } while (0)

// ---------- fake clocks ----------
// A) Scripted clock (steps through a provided array of nanoseconds)
static const long long* g_script = NULL;
static size_t g_script_len = 0, g_script_idx = 0;

static HighResTimeT fake_now_script(void) {
    HighResTimeT t;
    if (!g_script || g_script_len == 0) { t.nanoseconds = 0; return t; }
    size_t i = (g_script_idx < g_script_len) ? g_script_idx++ : (g_script_len - 1);
    t.nanoseconds = g_script[i];
    return t;
}

static void set_script(const long long* arr, size_t n) {
    g_script = arr; g_script_len = n; g_script_idx = 0;
}

// B) Steady clock (returns start + k*step each call)
static long long g_cur_ns = 0;
static long long g_step_ns = 0;

static HighResTimeT fake_now_steady(void) {
    HighResTimeT t; t.nanoseconds = g_cur_ns; g_cur_ns += g_step_ns; return t;
}

static void set_steady(long long start_ns, long long step_ns) {
    g_cur_ns = start_ns; g_step_ns = step_ns;
}

// ---------- tests ----------
static int test_defaults_and_setters(void) {
    TimeManager tm; InitTimeManager(&tm);  // default clock inside
    // initial values from your implementation
    ASSERT_EQ_SIZE(TmGetPhysicsHz(&tm), 60);                      // default 60 Hz
    ASSERT_NEAR(TmGetPhysicsTimeStep(&tm), 1.0/60.0, 1e-12);      // derived from Hz
    ASSERT_NEAR(TmGetMaxFrameTime(&tm), 0.25, 1e-12);             // default cap
    ASSERT_EQ_SIZE(TmGetMaxPhysicsSteps(&tm), 5);                 // default 5
    ASSERT_NEAR(TmGetTimeScale(&tm), 1.0, 1e-12);                 // default 1x

    // Set via Hz
    TmSetPhysicsHz(&tm, 120);
    ASSERT_EQ_SIZE(TmGetPhysicsHz(&tm), 120);
    ASSERT_NEAR(TmGetPhysicsTimeStep(&tm), 1.0/120.0, 1e-12);

    // Set timestep directly (note: in your code, Hz stays as-is)
    TmSetPhysicsTimeStep(&tm, 0.02);
    ASSERT_NEAR(TmGetPhysicsTimeStep(&tm), 0.02, 1e-12);
    ASSERT_EQ_SIZE(TmGetPhysicsHz(&tm), 120); // unchanged by design

    // Caps & limits
    TmSetMaxFrameTime(&tm, 0.1);
    ASSERT_NEAR(TmGetMaxFrameTime(&tm), 0.1, 1e-12);
    TmSetMaxPhysicsSteps(&tm, 4);
    ASSERT_EQ_SIZE(TmGetMaxPhysicsSteps(&tm), 4);

    return 0;
}

static int test_first_frame_and_basic_stepping(void) {
    // Use a scripted clock: 0ns (set), 16ms (first frame consumes), then 32ms, 48ms...
    static const long long script[] = { 0LL, 16LL*1000*1000, 32LL*1000*1000, 48LL*1000*1000 };
    TimeManager tm; InitTimeManager(&tm);
    set_script(script, sizeof(script)/sizeof(script[0]));
    TmSetTimeSource(&tm, fake_now_script); // resets lastTime to script[0] per header/impl

    // 1) first frame is special: zeros & sets lastTime to script[1]
    FrameTimingData f0 = TmBeginFrame(&tm);
    ASSERT_EQ_SIZE(f0.physicsSteps, 0);
    ASSERT_NEAR(f0.fixedTimestep, 1.0/60.0, 1e-12);
    ASSERT_NEAR(f0.interpolationAlpha, 0.0, 1e-12);
    ASSERT_NEAR(f0.frameTime, 0.0, 1e-12);
    ASSERT_TRUE(!f0.lagging);
    ASSERT_NEAR(f0.rawFrameTime, 0.0, 1e-12);
    ASSERT_NEAR(f0.unscaledFrameTime, 0.0, 1e-12);
    ASSERT_NEAR(f0.currentTimeScale, 1.0, 1e-12);

    // 2) delta = 16ms -> with 60Hz timestep (~16.666ms), 0 physics steps; alpha ~ 0.96
    FrameTimingData f1 = TmBeginFrame(&tm);
    ASSERT_EQ_SIZE(f1.physicsSteps, 0);
    ASSERT_TRUE(!f1.lagging);
    ASSERT_NEAR(f1.unscaledFrameTime, 0.016, 1e-6);
    ASSERT_NEAR(f1.frameTime,       0.016, 1e-6);
    ASSERT_TRUE(f1.interpolationAlpha > 0.9 && f1.interpolationAlpha < 1.0);

    // 3) next 16ms pushes us over one step -> 1 physics step this frame
    FrameTimingData f2 = TmBeginFrame(&tm);
    ASSERT_EQ_SIZE(f2.physicsSteps, 1);
    ASSERT_TRUE(!f2.lagging);
    ASSERT_NEAR(f2.unscaledFrameTime, 0.016, 1e-6);

    return 0;
}

static int test_lagging_and_max_steps(void) {
    // 100 Hz (10ms step), but only allow 2 steps per frame.
    TimeManager tm; InitTimeManager(&tm);
    TmSetPhysicsTimeStep(&tm, 0.01);
    TmSetMaxPhysicsSteps(&tm, 2);

    // Script: 0ns (set), 0ns (first frame consumes -> zero), 50ms jump => should try 5 steps but cap at 2
    static const long long script[] = { 0LL, 0LL, 50LL*1000*1000 };
    set_script(script, sizeof(script)/sizeof(script[0]));
    TmSetTimeSource(&tm, fake_now_script);

    (void)TmBeginFrame(&tm); // first frame

    FrameTimingData f = TmBeginFrame(&tm);
    ASSERT_EQ_SIZE(f.physicsSteps, 2);  // capped
    ASSERT_TRUE(f.lagging);             // accumulator still >= step before fmod
    ASSERT_NEAR(f.unscaledFrameTime, 0.05, 1e-9);
    // After fmod the remainder is near 0 (could be tiny fp noise)
    ASSERT_TRUE(f.interpolationAlpha >= 0.0 && f.interpolationAlpha < 1.0 + 1e-9);

    return 0;
}

static int test_cap_and_raw_delta(void) {
    TimeManager tm; InitTimeManager(&tm);
    TmSetMaxFrameTime(&tm, 0.10); // cap at 100ms

    // 0ns, 0ns, then +1.5s raw -> raw should be 1.5, unscaled gets capped to 0.10
    static const long long script[] = { 0LL, 0LL, 1500LL*1000*1000 };
    set_script(script, sizeof(script)/sizeof(script[0]));
    TmSetTimeSource(&tm, fake_now_script);

    (void)TmBeginFrame(&tm);
    FrameTimingData f = TmBeginFrame(&tm);
    ASSERT_NEAR(f.rawFrameTime,      1.5, 1e-12);
    ASSERT_NEAR(f.unscaledFrameTime, 0.10, 1e-12);  // capped
    ASSERT_NEAR(f.frameTime,         0.10, 1e-12);  // scale=1.0

    return 0;
}

static int test_pause_resume(void) {
    TimeManager tm; InitTimeManager(&tm);
    TmSetTimeScale(&tm, 0.5);
    TmPause(&tm);
    ASSERT_TRUE(TmIsPaused(&tm));
    TmResume(&tm);
    ASSERT_NEAR(TmGetTimeScale(&tm), 0.5, 1e-12);
    return 0;
}

static int test_average_fps(void) {
    // Use steady 20ms frames → exactly 50 frames per second when summed to 1.0s+
    TimeManager tm; InitTimeManager(&tm);
    set_steady(0LL, 20LL*1000*1000);
    TmSetTimeSource(&tm, fake_now_steady);

    (void)TmBeginFrame(&tm); // first frame (zero)

    // Drive a bit over 1 second of frames so stats roll
    for (int i = 0; i < 52; ++i) { (void)TmBeginFrame(&tm); }

    double avg = TmGetAverageFps(&tm);
    ASSERT_NEAR(avg, 50.0, 0.75); // allow a little fp noise/timing granularity
    return 0;
}

int main(void) {
    int rc = 0;
    if ((rc = test_defaults_and_setters()))      return rc;
    if ((rc = test_first_frame_and_basic_stepping())) return rc;
    if ((rc = test_lagging_and_max_steps()))     return rc;
    if ((rc = test_cap_and_raw_delta()))         return rc;
    if ((rc = test_pause_resume()))              return rc;
    if ((rc = test_average_fps()))               return rc;
    return 0;
}
