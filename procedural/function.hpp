#ifndef URHOEXTRAS_PROCEDURAL_FUNCTION_HPP
#define URHOEXTRAS_PROCEDURAL_FUNCTION_HPP

#include <Urho3D/Container/HashMap.h>
#include <Urho3D/Container/Vector.h>
#include <Urho3D/Core/Mutex.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Core/WorkQueue.h>

#include <cstdint>

namespace UrhoExtras
{

namespace Procedural
{

template <class T> class Function : public Urho3D::Object
{
    URHO3D_OBJECT(Function, Urho3D::Object)

public:

    inline Function(Urho3D::Context* context, long begin_x, long begin_y, long end_x, long end_y) :
        Urho3D::Object(context),
        data_begin_x(begin_x),
        data_begin_y( begin_y),
        data_end_x(end_x),
        data_end_y(end_y),
        data_width(end_x - begin_x)
    {
        long data_height = end_y -  begin_y;
        data = new T[data_width * data_height];
        data_ready = new bool[data_width * data_height];
        std::fill(data_ready, data_ready + data_width * data_height, false);
    }
    inline virtual ~Function()
    {
        delete[] data;
        delete[] data_ready;
    }

    inline long getDataBeginX() const { return data_begin_x; }
    inline long getDataBeginY() const { return data_begin_y; }
    inline long getDataEndX() const { return data_end_x; }
    inline long getDataEndY() const { return data_end_y; }

    inline void get(T& result, long x, long y)
    {
        if (x < data_begin_x || y < data_begin_y || x >= data_end_x || y >= data_end_y) {
            if (!allowOutOfBounds()) {
                throw std::runtime_error("Out of bounds!");
            }
            Pos pos(x, y);
            Urho3D::MutexLock lock(oob_mutex);
            auto finder = oob_cache.Find(pos);
            if (finder != oob_cache.End()) {
                result = finder->second_;
                return;
            }
            doGet(result, x, y);
            oob_cache[pos] = result;
            return;
        }

        long offset = (y - data_begin_y) * data_width + x - data_begin_x;

        if (!data_ready[offset]) {
            if (needsMutexProtection()) {
                Urho3D::MutexLock lock(mutex);
// TODO: Would it be possible to implement some kind of per-cell-locking?
                doGet(data[offset], x, y);
                data_ready[offset] = true;
            } else {
                doGet(data[offset], x, y);
                data_ready[offset] = true;
            }
        }

        result = data[offset];
    }

    inline T get(long x, long y)
    {
        T result;
        get(result, x, y);
        return result;
    }

    inline void precalculateEverything()
    {
        Urho3D::WorkQueue* workqueue = GetSubsystem<Urho3D::WorkQueue>();
        // Divide work to worker units
        for (unsigned offset = 0; offset < workqueue->GetNumThreads() + 1; ++ offset) {
            Precalculator* precalculator = new Precalculator(this, offset, workqueue->GetNumThreads() + 1);
            precalculator->priority_ = 0xffffffff;
            precalculator->workFunction_ = doPrecalculate;
            workqueue->AddWorkItem(Urho3D::SharedPtr<Urho3D::WorkItem>(precalculator));
        }
        workqueue->Complete(0xffffffff);
    }

protected:

    inline void set(long x, long y, T val)
    {
        if (x < data_begin_x || y < data_begin_y || x >= data_end_x || y >= data_end_y) {
            if (!allowOutOfBounds()) {
                throw std::runtime_error("Out of bounds!");
            }
            oob_cache[Pos(x, y)] = val;
        } else {
            unsigned offset = y * (data_end_x - data_begin_x) + x;
            data[offset] = val;
            data_ready[offset] = true;
        }
    }

private:

    struct Pos
    {
        long x, y;
        inline Pos() {}
        inline Pos(long x, long y) : x(x), y(y) {}
        inline bool operator==(Pos const& p) const { return p.x == x && p.y == y; }
        inline unsigned ToHash() const { return x * 31l + y; }
    };

    class Precalculator : public Urho3D::WorkItem
    {
    public:
        Function* func;
        unsigned offset;
        unsigned step;

        inline Precalculator(Function* func, unsigned offset, unsigned step) :
            func(func),
            offset(offset),
            step(step)
        {
        }
    };

    typedef Urho3D::HashMap<Pos, T> Cache;

    // Data and the area it lives in
    T* data;
    long data_begin_x, data_begin_y, data_end_x, data_end_y;
    long data_width;

    // This tells if data is ready. Booleans are not packed,
    // in order to make them writable from multiple threads.
    bool* data_ready;

    Urho3D::Mutex mutex;

    // Cache and mutex for out of bounds values
    Cache oob_cache;
    Urho3D::Mutex oob_mutex;

    // Result is always cleared
    virtual void doGet(T& result, long x, long y) = 0;

    inline static void doPrecalculate(Urho3D::WorkItem const* workitem, unsigned)
    {
        Precalculator const* precalculator = (Precalculator*)(workitem);

        long data_begin_x = precalculator->func->data_begin_x;
        long data_begin_y = precalculator->func->data_begin_y;
        long data_end_x = precalculator->func->data_end_x;
        long data_end_y = precalculator->func->data_end_y;

        unsigned skip = precalculator->offset;
        unsigned offset = 0;
        for (long y = data_begin_y; y < data_end_y; ++ y) {
            for (long x = data_begin_x; x < data_end_x; ) {
                // If there are items to skip, then do it now.
                if (skip) {
                    x += skip;
                    offset += skip;
                    if (x >= data_end_x) {
                        unsigned overflow = x - data_end_x;
                        offset -= overflow;
                        skip = overflow;
                        break;
                    }
                }
                // Get and store to cache
                precalculator->func->doGet(precalculator->func->data[offset], x, y);
                precalculator->func->data_ready[offset] = true;

                // Advance iteration
                ++ offset;
                ++ x;
                skip = precalculator->step - 1;
            }
        }
    }

    virtual bool needsMutexProtection() const
    {
        return false;
    }

    virtual bool allowOutOfBounds() const
    {
        return false;
    }
};

}

}

#endif
