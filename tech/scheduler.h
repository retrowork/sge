///////////////////////////////////////////////////////////////////////////////
// $Id$

#ifndef INCLUDED_SCHEDULER_H
#define INCLUDED_SCHEDULER_H

#include "schedulerclock.h"

#include "tech/globalobjdef.h"
#include "tech/schedulerapi.h"

#include <queue>
#include <deque>

#ifdef _MSC_VER
#pragma once
#endif

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sTaskInfo
//

struct sTaskInfo
{
   sTaskInfo();
   sTaskInfo(const sTaskInfo & other);
   ~sTaskInfo();

   const sTaskInfo & operator =(const sTaskInfo & other);

   cAutoIPtr<ITask> pTask;
	double start;
	double period;
	double expiration;
	double next;
};

struct sTaskInfoCompare
{
   bool operator()(const sTaskInfo * pTask1, const sTaskInfo * pTask2) const
   {
      // Returning true puts the first argument toward the back of the
      // priority queue. Use a greater than test so that tasks with sooner
      // next run times are sorted toward the front of the priority queue.
      return pTask1->next > pTask2->next;
   }
};

typedef std::priority_queue<sTaskInfo *, std::deque<sTaskInfo *>, sTaskInfoCompare> tTaskQueue;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cScheduler
//

class cScheduler : public cComObject2<IMPLEMENTS(IScheduler), IMPLEMENTS(IGlobalObject)>
{
public:
	cScheduler();
	~cScheduler();

   DECLARE_NAME_STRING(kSchedulerName)
   DECLARE_NO_CONSTRAINTS()

   virtual tResult Init();
   virtual tResult Term();

	virtual void Start();
	virtual void Stop();
	virtual bool IsRunning() const;

	virtual void NextFrame();

   virtual tResult AddRenderTask(ITask * pTask);
	virtual tResult RemoveRenderTask(ITask * pTask);

	virtual tResult AddFrameTask(ITask * pTask, ulong start, ulong period, ulong duration);
	virtual tResult RemoveFrameTask(ITask * pTask);

	virtual tResult AddTimeTask(ITask * pTask, double start, double period, double duration);
	virtual tResult RemoveTimeTask(ITask * pTask);

private:
	cSchedulerClock m_clock;
   tTaskQueue m_frameTaskQueue;
   tTaskQueue m_timeTaskQueue;
   std::deque<ITask *> m_renderTaskQueue;
   uint m_lockRenderTaskQueue;
};

///////////////////////////////////////////////////////////////////////////////

#endif // INCLUDED_SCHEDULER_H
