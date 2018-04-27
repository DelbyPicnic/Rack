#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>
#if !(defined(__arm__) || defined(__aarch64__))
#include <xmmintrin.h>
#endif
#include "tinythread.h"
#include "concurrentqueue.h"

#include "engine.hpp"

namespace rack {

bool gPaused = false;
std::vector<Module*> gModules;
std::vector<Wire*> gWires;

static bool running = false;
static float sampleRate;
static float sampleTime;

// static std::thread thread;

// Parameter interpolation
static Module *smoothModule = NULL;
static int smoothParamId;
static float smoothValue;

struct UOW {
    Module *module;
    int step, step2;
};
moodycamel::ConcurrentQueue<UOW> q;
moodycamel::ProducerToken *ptoks[10];
volatile int runningt;

tthread::mutex m;
tthread::condition_variable cond;
tthread::condition_variable cond2;
int numWorkers;

float Light::getBrightness() {
    // LEDs are diodes, so don't allow reverse current.
    // For some reason, instead of the RMS, the sqrt of RMS looks better
    //return powf(fmaxf(0.f, value), 0.25f);
    return value;
}

void Light::setBrightnessSmooth(float brightness, float frames) {
    setBrightness(brightness);

    /*float v = (brightness > 0.f) ? brightness * brightness : 0.f;

    if (v < value) {
        // Fade out light with lambda = framerate
        value += (v - value) * sampleTime * frames * 60.f;
    }
    else {
        // Immediately illuminate light
        value = v;
    }*/
}


void Wire::step() {
    float value = outputModule->outputs[outputId].value;
    inputModule->inputs[inputId].value = value;
}

void Wire::stepMultiple(int steps) {
    for (int i = 0; i < steps;i++)
    {
        float value = outputModule->outputs[outputId].queue[i];
        inputModule->inputs[inputId].queue[i] = value;
    }
}

void do_work(int qq)
{
    pthread_t tID = pthread_self();
    sched_param prio = { 15 };
    if (!pthread_setschedparam(tID, SCHED_RR, &prio))
        printf("Set higher priority for audio thread.\n");
    else
        printf("NOT set higher priority for audio thread.\n");

    // int qq=0;
    // printf("MM? %d\n",qq);
    m.lock();
    while(1)
    {
        // printf("READY %d\n",qq);
        cond.wait(m);
        m.unlock();
        // printf("WAKE %d\n",qq);
        UOW item;
        while(q.try_dequeue(item))
        {
            // printf("%d deq %p %d %d\n", qq, item.module, item.step, item.step2);
            Module *module = item.module;
            for (int step = item.step; step < item.step2; step++)
            {
                int id=0;
                for (auto &in : module->inputs)
                {
                    if (in.pos < step)
                    {
                        item.step = step;
                        q.enqueue(*ptoks[qq], item);
                        // printf("%d enq %p %d %d\n", qq, item.module, item.step, item.step2);
                        goto nope;
                    }
                    in.value = *(volatile float*)(in.queue + step);
                    // if (in.active)
                    //  printf("%d :: read %p %d @ %d = %f\n", qq, module, id, step, in.value);
                    id++;
                }
                
            // printf("work %p %d\n", item.module, step);
                module->step();

                for (auto &out : module->outputs)
                {
                    //out.queue[i] = out.value;
                    for (Wire *w : out.wires)
                    {
                        // if (w->inputModule == outm)
                        //  printf("wrote OUT %d\n",step+1);
                        *(volatile float*)(w->inputModule->inputs[w->inputId].queue + step + 1) = out.value;
                        __sync_synchronize();
                        w->inputModule->inputs[w->inputId].pos = step+1;
                        // printf("%d :: wrote %p to %p %d @ %d = %f\n", qq, module, w->inputModule, w->inputId, w->inputModule->inputs[w->inputId].pos, out.value);
                    }
                }                   
            }

            nope:;
        }

        m.lock();
        runningt--;
        // printf("FINISHED %d\n",qq);
        cond2.notify_all();
    }
    // printf("EXITED %d\n",qq);
}

void engineInit() {
    engineSetSampleRate(48000.0);

    numWorkers = 3;
    for (int i = 0; i < numWorkers; i++)
    {
        ptoks[i] = new moodycamel::ProducerToken(q);
        (new tthread::thread((void(*)(void*))do_work, (void*)i))->detach();
    }
    printf("Started %d DSP threads\n", numWorkers);
}

void engineDestroy() {
    // Make sure there are no wires or modules in the rack on destruction. This suggests that a module failed to remove itself before the WINDOW was destroyed.
    assert(gWires.empty());
    assert(gModules.empty());
}

void engineStep() {
    // Param interpolation
    if (smoothModule) {
        float value = smoothModule->params[smoothParamId].value;
        const float lambda = 60.0; // decay rate is 1 graphics frame
        float delta = smoothValue - value;
        float newValue = value + delta * lambda * sampleTime;
        if (value == newValue) {
            // Snap to actual smooth value if the value doesn't change enough (due to the granularity of floats)
            smoothModule->params[smoothParamId].value = smoothValue;
            smoothModule = NULL;
        }
        else {
            smoothModule->params[smoothParamId].value = newValue;
        }
    }

    // Step modules
    for (Module *module : gModules) {
        module->step();

        // TODO skip this step when plug lights are disabled
        // Step ports
        /*for (Input &input : module->inputs) {
            if (input.active) {
                float value = input.value / 5.f;
                // input.plugLights[0].setBrightnessSmooth(value);
                // input.plugLights[1].setBrightnessSmooth(-value);
                input.plugLights[0].setBrightnessSmooth(value > 0);
                input.plugLights[1].setBrightnessSmooth(value < 0);
            }
        }
        for (Output &output : module->outputs) {
            if (output.active) {
                float value = output.value / 5.f;
                // output.plugLights[0].setBrightnessSmooth(value);
                // output.plugLights[1].setBrightnessSmooth(-value);
                output.plugLights[0].setBrightnessSmooth(value > 0);
                output.plugLights[1].setBrightnessSmooth(value < 0);                
            }
        }*/
    }

    // Step cables by moving their output values to inputs
    for (Wire *wire : gWires) {
        wire->step();
    }
}

void engineStepMT(int steps) {
    if (smoothModule) {
        float value = smoothModule->params[smoothParamId].value;
        const float lambda = 60.0; // decay rate is 1 graphics frame
        float delta = smoothValue - value;
        float newValue = value + delta * lambda * sampleTime*steps;
        if (value == newValue) {
            // Snap to actual smooth value if the value doesn't change enough (due to the granularity of floats)
            smoothModule->params[smoothParamId].value = smoothValue;
            smoothModule = NULL;
        }
        else {
            smoothModule->params[smoothParamId].value = newValue;
        }
    }

    // Step modules
    // printf("---\n");
    m.lock();
    Module *outm = NULL;
    for (Module *module : gModules) {
        if(0&&module->outputs.size() == 0 && !outm)
        {
            for (Input &in : module->inputs)
                in.queue[0] = in.queue[steps];
            outm = module;
            continue;
        }
        bool good = true;
        for (Input &in : module->inputs)
        {
            in.pos = in.active ? 0 : steps-1;
            in.queue[0] = in.queue[steps];
            // if (in.active)
            //  good = false;
        }
        module->curstep = 0;
        UOW uow = { module, 0, good ? steps : 1 };
        q.enqueue(*ptoks[0], uow);
    }

    runningt = numWorkers;
    m.unlock();
    cond.notify_all();

    // printf("WAITING\n");
    // m.lock();
    // while(runningt)
    //  cond2.wait(m);
    // m.unlock();

    // printf("DONE\n");
    if (outm)
    {
        // printf("OUT\n");
        for (int step = 0; step < steps; step++)
        {
            for (auto &in : outm->inputs)
                in.value = in.queue[step];
            outm->step();
        }
    }
}

void engineWaitMT() {
    static bool primed = false;

    if (!primed)
    {
        primed = true;
        return;
    }

    m.lock();
    while(runningt)
        cond2.wait(m);
    m.unlock(); 
}

static void engineRun() {
    /*
    // Every time the engine waits and locks a mutex, it steps this many frames
    const int mutexSteps = 64;
    // Time in seconds that the engine is rushing ahead of the estimated clock time
    double ahead = 0.0;
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (running) {
        vipMutex.wait();

        if (!gPaused) {
            std::lock_guard<std::mutex> lock(mutex);
            // for (int i = 0; i < mutexSteps; i++) {
            //  engineStep();
            // }
            engineStepMT(mutexSteps);
        }

        double stepTime = mutexSteps * sampleTime;
        ahead += stepTime;
        auto currTime = std::chrono::high_resolution_clock::now();
        const double aheadFactor = 2.0;
        ahead -= aheadFactor * std::chrono::duration<double>(currTime - lastTime).count();
        lastTime = currTime;
        ahead = fmaxf(ahead, 0.0);

        // Avoid pegging the CPU at 100% when there are no "blocking" modules like AudioInterface, but still step audio at a reasonable rate
        // The number of steps to wait before possibly sleeping
        const double aheadMax = 1.0; // seconds
        if (ahead > aheadMax) {
            std::this_thread::sleep_for(std::chrono::duration<double>(stepTime));
        }
    }*/
}

void engineStart() {
    // running = true;
    // thread = std::thread(engineRun);

#if !(defined(__arm__) || defined(__aarch64__))
    // Set CPU to flush-to-zero (FTZ) and denormals-are-zero (DAZ) mode
    // https://software.intel.com/en-us/node/682949
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif
}

void engineStop() {
    // running = false;
    // thread.join();
}

void engineAddModule(Module *module) {
    assert(module);

    // Check that the module is not already added
    auto it = std::find(gModules.begin(), gModules.end(), module);
    assert(it == gModules.end());

    m.lock();
    while(runningt)
        cond2.wait(m);

    gModules.push_back(module);
    m.unlock(); 
}

void engineRemoveModule(Module *module) {
    assert(module);

    // If a param is being smoothed on this module, stop smoothing it immediately
    if (module == smoothModule) {
        smoothModule = NULL;
    }
    // Check that all wires are disconnected
    for (Wire *wire : gWires) {
        assert(wire->outputModule != module);
        assert(wire->inputModule != module);
    }
    // Check that the module actually exists
    auto it = std::find(gModules.begin(), gModules.end(), module);
    assert(it != gModules.end());
    // Remove it
    m.lock();
    while(runningt)
        cond2.wait(m);

    gModules.erase(it);
    m.unlock(); 
}

static void updateActive() {
    // Set everything to inactive
    for (Module *module : gModules) {
        for (Input &input : module->inputs) {
            input.active = false;
        }
        for (Output &output : module->outputs) {
            output.active = false;
        }
    }
    // Set inputs/outputs to active
    for (Wire *wire : gWires) {
        wire->outputModule->outputs[wire->outputId].active = true;
        wire->inputModule->inputs[wire->inputId].active = true;
    }
}

void engineAddWire(Wire *wire) {
    assert(wire);

    // Check wire properties
    assert(wire->outputModule);
    assert(wire->inputModule);
    // Check that the wire is not already added, and that the input is not already used by another cable
    for (Wire *wire2 : gWires) {
        assert(wire2 != wire);
        assert(!(wire2->inputModule == wire->inputModule && wire2->inputId == wire->inputId));
    }

    // Add the wire
    m.lock();
    while(runningt)
        cond2.wait(m);

    gWires.push_back(wire);
    wire->outputModule->outputs[wire->outputId].wires.push_back(wire);
    updateActive();
    m.unlock();
}

void engineRemoveWire(Wire *wire) {
    assert(wire);

    // Check that the wire is already added
    auto it = std::find(gWires.begin(), gWires.end(), wire);
    auto it2 = std::find(wire->outputModule->outputs[wire->outputId].wires.begin(), wire->outputModule->outputs[wire->outputId].wires.end(), wire);
    assert(it != gWires.end());

    // Remove the wire
    m.lock();
    while(runningt)
        cond2.wait(m);

    // Set input to 0V
    wire->inputModule->inputs[wire->inputId].value = 0.0;
    memset(wire->inputModule->inputs[wire->inputId].queue, 0, sizeof(wire->inputModule->inputs[wire->inputId].queue));

    gWires.erase(it);
    wire->outputModule->outputs[wire->outputId].wires.erase(it2);
    updateActive();
    m.unlock();
}

void engineSetParam(Module *module, int paramId, float value) {
    module->params[paramId].value = value;
}

void engineSetParamSmooth(Module *module, int paramId, float value) {
    // VIPLock vipLock(vipMutex);
    // std::lock_guard<std::mutex> lock(mutex);
    // Since only one param can be smoothed at a time, if another param is currently being smoothed, skip to its final state
    if (smoothModule && !(smoothModule == module && smoothParamId == paramId)) {
        smoothModule->params[smoothParamId].value = smoothValue;
    }
    smoothModule = module;
    smoothParamId = paramId;
    smoothValue = value;
}

void engineSetSampleRate(float newSampleRate) {
    // VIPLock vipLock(vipMutex);
    // std::lock_guard<std::mutex> lock(mutex);
    sampleRate = newSampleRate;
    sampleTime = 1.0 / sampleRate;

    // onSampleRateChange
    m.lock();
    while(runningt)
        cond2.wait(m);

    for (Module *module : gModules)
        module->onSampleRateChange();
    m.unlock();
}

float engineGetSampleRate() {
    return sampleRate;
}

float engineGetSampleTime() {
    return sampleTime;
}

} // namespace rack
