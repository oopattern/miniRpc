#include <stdio.h>
#include <string>
#include <deque>

using namespace std;

class CBlockQueue
{
public:
    CBlockQueue();
    ~CBlockQueue();

    // get size of queue
    size_t Size(void);

    // put one obj
    void Put(const std::string& x);

    // take one obj, without timeout
    std::string Take(void);

    // take mutli obj, timeout with sec
    std::vector<string> TakeMutli(int num, int timeout); 

private:
    // cond operation
    void Notify(void);
    void Wait(void);
    void WaitTimeout(int sec);

    // mutex operation
    void AssignHolder(void);
    void UnassignHolder(void);

private:
    pid_t               m_holder; // locking or not
    pthread_mutex_t     m_mutex;  // protect queue
    pthread_cond_t      m_cond;   // wait-notify  
    std::deque<string>  m_queue;  // share var, string temporary
};
