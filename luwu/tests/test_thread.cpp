//
// Created by liucxi on 2022/4/16.
//

#include "luwu/thread.h"
#include "luwu/macro.h"

liucxi::Logger::ptr g_logger = LUWU_LOG_ROOT();

int count = 0;
liucxi::Mutex s_mutex;

void func1(void *arg) {
    LUWU_LOG_INFO(g_logger) << "name:" << liucxi::Thread::GetName()
                             << " this.name:" << liucxi::Thread::GetThis()->getName()
                             << " thread name:" << liucxi::getThreadName()
                             << " id:" << liucxi::getThreadId()
                             << " this.id:" << liucxi::Thread::GetThis()->getId();
    LUWU_LOG_INFO(g_logger) << "arg: " << *(int*)arg;
    for(int i = 0; i < 10000; i++) {
        liucxi::Mutex::Lock lock(s_mutex);
        ++count;
    }
}

int main(int argc, char *argv[]) {

    std::vector<liucxi::Thread::ptr> thrs;
    int arg = 123456;
    for(int i = 0; i < 3; i++) {
        // 带参数的线程用std::bind进行参数绑定
        liucxi::Thread::ptr thr(new liucxi::Thread([argv0 = &arg] { return func1(argv0); }
                                                        , "thread_" + std::to_string(i)));
        thrs.push_back(thr);
    }

    for(int i = 0; i < 3; i++) {
        thrs[i]->join();
    }

    LUWU_LOG_INFO(g_logger) << "count = " << count;
    return 0;
}



