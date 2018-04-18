#include <stdio.h>
#include <string>
#include <deque>

using namespace std;

class CBlockQueue
{
public:
    CBlockQueue();
    ~CBlockQueue();

    void Put(const std::string& x);
    std::string Take(void);
    size_t Size(void);

private:
    // cond operation
    void Notify(void);
    void Wait(void);

    // mutex operation
    void AssignHolder(void);
    void UnassignHolder(void);

private:
    pid_t               m_holder; // locking or not
    pthread_mutex_t     m_mutex;  // protect queue
    pthread_cond_t      m_cond;   // wait-notify  
    std::deque<string>  m_queue;  // share var, string temporary
};
