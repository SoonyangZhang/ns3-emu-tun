#pragma once
#include "base_thread.h"
#include "ns3/epoll_api.h"
#include "ns3/tun_mutex.h"
#include <utility>
#include <memory>
#include <deque>
namespace std
{
template<typename T, typename ...Args>
std::unique_ptr<T> make_unique( Args&& ...args )
{
    return std::unique_ptr<T>( new T( std::forward<Args>(args)... ) );
}
}
namespace ns3{
class QueuedTask {
 public:
  QueuedTask() {}
  virtual ~QueuedTask() {}
  virtual bool Run() = 0;
};
template <class Closure>
class ClosureTask : public QueuedTask {
 public:
  explicit ClosureTask(Closure&& closure)
      : closure_(std::forward<Closure>(closure)) {}
 private:
  bool Run() override {
    closure_();
    return true;
  }

  typename std::remove_const<
      typename std::remove_reference<Closure>::type>::type closure_;
};
template <class Closure, class Cleanup>
class ClosureTaskWithCleanup : public ClosureTask<Closure> {
 public:
  ClosureTaskWithCleanup(Closure&& closure, Cleanup&& cleanup)
      : ClosureTask<Closure>(std::forward<Closure>(closure)),
        cleanup_(std::forward<Cleanup>(cleanup)) {}
  ~ClosureTaskWithCleanup() { cleanup_(); }

 private:
  typename std::remove_const<
      typename std::remove_reference<Cleanup>::type>::type cleanup_;
};

// Convenience function to construct closures that can be passed directly
// to methods that support std::unique_ptr<QueuedTask> but not template
// based parameters.
template <class Closure>
static std::unique_ptr<QueuedTask> NewClosure(Closure&& closure) {
  return std::make_unique<ClosureTask<Closure>>(std::forward<Closure>(closure));
}

template <class Closure, class Cleanup>
static std::unique_ptr<QueuedTask> NewClosure(Closure&& closure,
                                              Cleanup&& cleanup) {
  return std::make_unique<ClosureTaskWithCleanup<Closure, Cleanup>>(
      std::forward<Closure>(closure), std::forward<Cleanup>(cleanup));
}

class EpollThread :public BaseThread{
public:
    EpollThread(){}
    ~EpollThread();
    void Run() override;
    EpollServer *epoll_server();
    template <class Closure,
          typename std::enable_if<!std::is_convertible<
              Closure,
              std::unique_ptr<QueuedTask>>::value>::type* = nullptr>
    void PostTask(Closure&& closure){
        PostInnerTask(NewClosure(std::forward<Closure>(closure)));
    }     
private:
    void PostInnerTask(std::unique_ptr<QueuedTask> task);
    void ExecuteTask();
    EpollServer epoll_server_;
    mutable Mutex task_mutex_;
    std::deque<std::unique_ptr<QueuedTask>>  queued_tasks_;
};    
}