#include "profiler.hpp"

#include "../utils.hpp"

namespace UrhoExtras
{

namespace Debug
{

Urho3D::Mutex ProfileBlock::mutex;
ProfileBlock::ThreadStacks ProfileBlock::stacks;
ProfileBlock::ThreadStats ProfileBlock::stats;
unsigned ProfileBlock::auto_output_to_file_secs_interval = 0;
Urho3D::Timer ProfileBlock::auto_output_to_file_timer;


ProfileBlock::ProfileBlock(Urho3D::String const& name) :
    name(name)
{
    push(name, Urho3D::Thread::GetCurrentThreadID());
}

ProfileBlock::~ProfileBlock()
{
    pop(Urho3D::Thread::GetCurrentThreadID());
}

Urho3D::String ProfileBlock::getStats(bool reset)
{
    Urho3D::MutexLock lock(mutex);

    Urho3D::String result;

    for (auto stat : stats) {

        AsciiTable table;

        table.addCell("Name");
        table.addCell("Time");
        table.addCell("Total time");
        table.addCell("Percent");
        table.endRow();

        // Calculate total usecs of this Thread
        uint64_t total_usecs = 0;
        for (auto child : stat->stats_root.children) {
            total_usecs += child.second_->total_usecs;
        }

        addStatsRecursively(table, 0, stat->stats_root.children, total_usecs);

        result += "Thread " + Urho3D::String(stat->thread_id) + ":\n" + table.toString() + "\n";
    }

    if (reset) {
        stats.Clear();
    }

    return result;
}

void ProfileBlock::setAutoOutputToFile(unsigned seconds_interval)
{
    Urho3D::MutexLock lock(mutex);

    auto_output_to_file_secs_interval = seconds_interval;
    auto_output_to_file_timer.Reset();
}

void ProfileBlock::push(Urho3D::String const& name, ThreadID thread_id)
{
    Urho3D::MutexLock lock(mutex);

    checkAutoOutputToFile();

    ThreadStack& stack = getThreadStack(thread_id);

    // If there is something else in stack, then store its spent micro seconds
    if (!stack.items.Empty()) {
        StackItem& back_item = stack.items.Back();
        uint64_t total_usecs = back_item.timer.GetUSec(true);
        addToStats(stack, total_usecs - back_item.usecs_when_resumed, total_usecs);
    }

    stack.items.Push(StackItem(name));
}

void ProfileBlock::pop(ThreadID thread_id)
{
    Urho3D::MutexLock lock(mutex);

    checkAutoOutputToFile();

    ThreadStack& stack = getThreadStack(thread_id);

    // End current top item
    StackItem& back_item = stack.items.Back();
    uint64_t total_usecs = back_item.timer.GetUSec(false);
    addToStats(stack, total_usecs - back_item.usecs_when_resumed, total_usecs);

    stack.items.Pop();

    // If there was another item in the stack, resume it
    if (!stack.items.Empty()) {
        StackItem& back_item = stack.items.Back();
        back_item.usecs_when_resumed = back_item.timer.GetUSec(false);
    }
}

ProfileBlock::ThreadStack& ProfileBlock::getThreadStack(ThreadID thread_id)
{
    for (ThreadStack& stack : stacks) {
        if (stack.thread_id == thread_id) {
            return stack;
        }
    }
    stacks.Push(ThreadStack(thread_id));
    return stacks.Back();
}

void ProfileBlock::addToStats(ThreadStack const& stack, uint64_t usecs, uint64_t total_usecs)
{
    assert(usecs <= total_usecs);

    ThreadStat* stat = nullptr;
    for (ThreadStat* stat_find : stats) {
        if (stat_find->thread_id == stack.thread_id) {
            stat = stat_find;
        }
    }
    if (!stat) {
        stats.Push(Urho3D::SharedPtr<ThreadStat>(new ThreadStat(stack.thread_id)));
        stat = stats.Back();
    }

    StatsItem* stats_item = &stat->stats_root;
    for (StackItem const& stack_item : stack.items) {
        auto children_find = stats_item->children.Find(stack_item.name);
        if (children_find != stats_item->children.End()) {
            stats_item = children_find->second_.Get();
        } else {
            stats_item = stats_item->children[stack_item.name] = new StatsItem();
        }
    }
    stats_item->usecs += usecs;
    stats_item->total_usecs += total_usecs;
}

void ProfileBlock::addStatsRecursively(AsciiTable& result, unsigned indent, Urho3D::HashMap<Urho3D::String, Urho3D::SharedPtr<StatsItem>> const& children, uint64_t total_usecs)
{
    for (auto child : children) {
        // Name
        Urho3D::String name;
        for (unsigned i = 0; i < indent * 2; ++ i) name += " ";
        name += child.first_;
        result.addCell(name);

        // Time usage
        result.addCell(microsecondsToHumanReadable(child.second_->usecs), AsciiTable::A_RIGHT);

        // Total time usage
        result.addCell(microsecondsToHumanReadable(child.second_->total_usecs), AsciiTable::A_RIGHT);

        // Percent
        char rounded[40];
        ::sprintf(rounded, "%.2f", double(child.second_->usecs) / total_usecs * 100.0);
        result.addCell(Urho3D::String(rounded), AsciiTable::A_RIGHT);

        result.endRow();

        addStatsRecursively(result, indent + 1, child.second_->children, total_usecs);
    }
}

Urho3D::String ProfileBlock::microsecondsToHumanReadable(uint64_t usecs)
{
    unsigned usecs2 = usecs % 1000000;
    unsigned secs = (usecs / 1000000) % 60;
    unsigned mins = usecs / 60000000;
    return zfill(mins, 2) + ":" + zfill(secs, 2) + "." + zfill(usecs2, 6);
}

void ProfileBlock::checkAutoOutputToFile()
{
    if (auto_output_to_file_secs_interval > 0) {
        if (auto_output_to_file_timer.GetMSec(false) > auto_output_to_file_secs_interval * 1000) {
            std::ofstream file("auto_profile.log", std::ios_base::out);
            Urho3D::String stats = getStats(true);
            file.write(stats.CString(), stats.Length());
            file.close();
            auto_output_to_file_timer.Reset();
        }
    }
}

}

}
