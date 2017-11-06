#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<string.h> // memcpy
#include<unistd.h> // sleep
#include<sys/shm.h> // shm
#include<string>

using namespace std;

typedef long long money_t;

// shm macro
#define SHM_MODE    0600
#define SHM_ERROR   -1
#define SHM_OK      0
#define BUF_SIZE    1024
const char* g_content = "welcome to shm";

typedef struct TUser
{
    int uid;
    char name[64]; // can not use string! delete will core
    money_t money;
} TUser;

class CShm
{
public:
    CShm() {}
    ~CShm() {}
    
    int CreateShm(unsigned int size = SHM_SIZE)
    {
        m_id = ::shmget(SHM_KEY, size, SHM_OPT);
        if (m_id < 0)
        {
            printf("shmget error\n");
            return SHM_ERROR;
        }
        printf("create shmid=%d\n", m_id);
        return AtShm();
    }

    int AttachShm(void)
    {
        m_id = ::shmget(SHM_KEY, 0, 0);
        if (m_id < 0)
        {
            printf("shmget error\n");
            return SHM_ERROR;
        }
        printf("attach shmid=%d\n", m_id);
        return AtShm();
    }

    int ReadShm(char* data, int len)
    {
        assert((len > 0) && (len < SHM_SIZE));
        
        if (m_id < 0)
        {
            return SHM_ERROR;
        }

        memcpy(data, (char*)m_ptr, len);
    }

    int WriteShm(const char* data, int len)
    {
        assert((len > 0) && (len < SHM_SIZE));
        
        if (m_id < 0)
        {
            return SHM_ERROR;
        }
        
        memcpy((char*)m_ptr, data, len);
        return len;
    }
private:
    static const int SHM_KEY = 0x20180325;
    static const int SHM_SIZE = 1024 * 100;
    static const int SHM_OPT = IPC_CREAT | IPC_EXCL;

    int AtShm(void)
    {
        if (m_id < 0)
        {
            return SHM_ERROR;
        }
        
        m_ptr = ::shmat(m_id, 0, 0);
        if (m_ptr == (void*)-1)
        {
            printf("shmat error\n");
            return SHM_ERROR;
        }

        return SHM_OK;
    }

    int m_id;
    void* m_ptr;
};

void TestWriteShm()
{
    TUser user;
    CShm shm;

    user.uid = 2333;
    user.money = 66666;
    snprintf(user.name, sizeof(user.name), "sakula");

    shm.CreateShm();
    shm.WriteShm((char*)&user, sizeof(user));
    printf("shm write done\n");
}

void TestReadShm()
{
    TUser user;
    CShm shm;
    
    shm.AttachShm();
    shm.ReadShm((char*)&user, sizeof(user));
    printf("uid:%d, name:%s, money:%d\n", user.uid, user.name, user.money);
    printf("shm read done\n");
}

int main(void)
{
    printf("hello world\n");
    TestWriteShm();
    TestReadShm();
    while (1)
    {
        printf("sleep 2 seconde\n");
        sleep(2);
    }
    return 0;
}
