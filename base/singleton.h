#include <stdio.h>
#include <assert.h>
#include <pthread.h>


// thread safe singleton
template<typename T>
class Singleton
{
public:
    // init just run once in mutli pthread
    static T& Instance(void)
    {
        ::pthread_once(&m_ponce, &Singleton::Init);
        assert(m_value != NULL);
        return *m_value;
    }

private:
    Singleton();
    ~Singleton();

    static void Init(void)
    {
        m_value = new T();
    }

private:
    static pthread_once_t   m_ponce;
    static T*               m_value;
};

// init static variable
template<typename T> 
pthread_once_t Singleton<T>::m_ponce = PTHREAD_ONCE_INIT;

template<typename T>
T* Singleton<T>::m_value = NULL;
