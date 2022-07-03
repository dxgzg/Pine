#pragma once
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>

#include "Epoller.h"
#include "const.h"

class Channel;
class HeartConnect;
class TimerQueue;

class EventLoop{
    using Function = std::function<void()>;
public:
    EventLoop();
    void loop();
    void updateChannel(Channel* channel);
    void runInLoop(const Function& cb);

	// ��cb��������У�����loop���ڵ��̣߳�ִ��cb
	void queueInLoop(const Function& cb);
	// ����loop���ڵ��̵߳�
	void wakeup();
    void handleRead();
    void doPendingFunctor();

    std::thread::id getThreadId()const{return threadId_;}
    void addTimer(timerCallback cb,int expire,bool repeat = false);

//    HeartConnect* getHeartConnect(){return heartConnect_.get();}

    // void addHeadDetection(int fd,std::function<void()> callback);
    ~EventLoop();
private:
    std::unique_ptr<Epoller> epoller_;
    int wakefd_;
    std::vector<Function> pendingFunctors_;
    std::thread::id threadId_;
    mutable std::mutex mutex_;
    std::unique_ptr<Channel> wakeChanel_;
    
    // ��muduo��ͬ�ĳ�Ա
    std::unique_ptr<TimerQueue> timerQueue_;
};