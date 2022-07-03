#ifndef URHOEXTRAS_DEBUG_PROFILER_HPP
#define URHOEXTRAS_DEBUG_PROFILER_HPP

#include "asciitable.hpp"

#include <Urho3D/Container/Str.h>
#include <Urho3D/Core/Mutex.h>
#include <Urho3D/Core/Thread.h>
#include <Urho3D/Core/Timer.h>

#ifdef URHOEXTRAS_PROFILING
#define URHOEXTRAS_PROFILE(name) UrhoExtras::Debug::ProfileBlock name(#name);
#else
#define URHOEXTRAS_PROFILE(name)
#endif

namespace UrhoExtras
{

namespace Debug
{

class ProfileBlock
{

public:

    ProfileBlock(Urho3D::String const& name);
    virtual ~ProfileBlock();

    static Urho3D::String getStats(bool reset = true);

    static void setAutoOutputToFile(unsigned seconds_interval);

private:

    struct StackItem
    {
        Urho3D::String name;
        Urho3D::HiresTimer timer;
        uint64_t usecs_when_resumed;

        inline StackItem(Urho3D::String const& name) :
            name(name),
            usecs_when_resumed(0)
        {
        }

        inline StackItem() :
            usecs_when_resumed(0)
        {
        }
    };
    typedef Urho3D::Vector<StackItem> StackItems;
    struct ThreadStack
    {
        StackItems items;
        ThreadID thread_id;

        inline ThreadStack(ThreadID thread_id) :
            thread_id(thread_id)
        {
        }
    };
    typedef Urho3D::Vector<ThreadStack> ThreadStacks;

    struct StatsItem : public Urho3D::RefCounted
    {
        Urho3D::HashMap<Urho3D::String, Urho3D::SharedPtr<StatsItem>> children;
        uint64_t usecs;
        uint64_t total_usecs;

        inline StatsItem() :
            usecs(0),
            total_usecs(0)
        {
        }
    };
    struct ThreadStat : public Urho3D::RefCounted
    {
        StatsItem stats_root;
        ThreadID thread_id;

        inline ThreadStat(ThreadID thread_id) :
            thread_id(thread_id)
        {
        }

        inline ThreadStat()
        {
        }
    };
    typedef Urho3D::Vector<Urho3D::SharedPtr<ThreadStat>> ThreadStats;

    Urho3D::String name;

    Urho3D::HiresTimer timer;

    static Urho3D::Mutex mutex;

    static ThreadStacks stacks;

    static ThreadStats stats;

    static unsigned auto_output_to_file_secs_interval;
    static Urho3D::Timer auto_output_to_file_timer;

    static void push(Urho3D::String const& name, ThreadID thread_id);
    static void pop(ThreadID thread_id);

    static ThreadStack& getThreadStack(ThreadID thread_id);

    static void addToStats(ThreadStack const& stack, uint64_t usecs, uint64_t total_usecs);

    static void addStatsRecursively(AsciiTable& result, unsigned indent, Urho3D::HashMap<Urho3D::String, Urho3D::SharedPtr<StatsItem>> const& children, uint64_t total_usecs);

    static Urho3D::String microsecondsToHumanReadable(uint64_t usecs);

    static void checkAutoOutputToFile();
};

}

}

#endif
