// This class and its subclass can be used in a situation where WorkItem gets
// some job done, but does not know if the result is still expected or not.
//
// Before creating WorkItem, you should create WorkItemResult and store it to
// caller and make a copy of it for WorkItem too. When WorkItem finishes its
// job, it must lock the Mutex and store the result to its WorkItemResult.
// After this, the requesting code can use the result, or if it's not needed
// any more, it gets destroyed when WorkItem destroys the last copy of the
// relevant WorkItemResult object.
#ifndef URHOEXTRAS_WORKITEMWITHRESULT_HPP
#define URHOEXTRAS_WORKITEMWITHRESULT_HPP

#include <Urho3D/Core/WorkQueue.h>

namespace UrhoExtras
{

class WorkItemResult
{
	friend class WorkItemWithResult;

public:

	class ActualResult
	{
		friend class WorkItemResult;
	public:
		inline ActualResult() : refs(1), results_ready(false) {}
		inline virtual ~ActualResult() {}
		inline Urho3D::Mutex& getMutex() { return mutex; }
		inline void setResultsReady() { results_ready = true; }
		inline bool areResultsReady() { return results_ready; }
	private:
		Urho3D::Mutex mutex;
		unsigned refs;
		volatile bool results_ready;
	};

	inline WorkItemResult() :
	actual_result(NULL)
	{
	}

	inline WorkItemResult(ActualResult* actual_result) :
	actual_result(actual_result)
	{
	}

	inline WorkItemResult(WorkItemResult& wir) :
	actual_result(wir.actual_result)
	{
		if (wir.actual_result) {
			++ wir.actual_result->refs;
		}
	}

	inline ~WorkItemResult()
	{
		if (actual_result) {
			actual_result->mutex.Acquire();
			-- actual_result->refs;
			if (actual_result->refs == 0) {
				actual_result->mutex.Release();
				delete actual_result;
				return;
			}
			actual_result->mutex.Release();
		}
	}

	inline WorkItemResult& operator=(WorkItemResult const& wir)
	{
		// Unset current result
		if (actual_result) {
			actual_result->mutex.Acquire();
			-- actual_result->refs;
			if (actual_result->refs == 0) {
				actual_result->mutex.Release();
				delete actual_result;
				return *this;
			}
			actual_result->mutex.Release();
		}
		// Set new result
		if (wir.actual_result) {
			wir.actual_result->mutex.Acquire();
			actual_result = wir.actual_result;
			++ wir.actual_result->refs;
			wir.actual_result->mutex.Release();
		} else {
			actual_result = NULL;
		}
		return *this;
	}

	inline bool isResultReady() { return actual_result && actual_result->areResultsReady(); }

	inline void discardResult() {
		if (actual_result) {
			actual_result->mutex.Acquire();
			-- actual_result->refs;
			if (actual_result->refs == 0) {
				actual_result->mutex.Release();
				delete actual_result;
			} else {
				actual_result->mutex.Release();
			}
			actual_result = NULL;
		}
	}

	inline ActualResult* getActualWorkItemResult()
	{
		return actual_result;
	}

private:

	ActualResult* actual_result;
};

class WorkItemWithResult : public Urho3D::WorkItem
{

public:

	inline WorkItemWithResult(WorkItemResult& wir) :
	wir(wir)
	{
	}

	inline virtual ~WorkItemWithResult()
	{
	}

	inline WorkItemResult::ActualResult* getActualWorkItemResult() const
	{
		return wir.actual_result;
	}

private:

	WorkItemResult wir;
};

}

#endif
