#include <stdio.h>
#include <functional> // c++11
#include <pthread.h>

typedef std::function<void ()> ThreadFunc;

// compiler flag -std=c++11
class CThread
{
public:
    CThread(const ThreadFunc& func);
    ~CThread();

    int Start();
    int Join();

    // for everyone to find their tid
    static pid_t Tid();

private:
    bool        m_started;
    bool        m_joined;
    pthread_t   m_pthreadId;
    ThreadFunc  m_func;
};
