
#include "bq.h"
#include <vector>
#include <memory>
#include <mutex>
#include <random>

#include <tbb/mutex.h>
#include <tbb/task_scheduler_init.h>
#include <tbb/task_group.h>

#define MSG_LEN 1024
#define MQ_LEN 4096
#define Q_N 4

class Msg {
public:
    Msg():content(MSG_LEN) {
        for(int i = 0; i < MSG_LEN; ++i)
        {
            content[i] = '0' + i%10;
        }
    }
protected:
    std::vector<char> 	content;
};
typedef std::shared_ptr<Msg> Msg_ptr;

class BQ {
public:
    BQ()
        : lock_in()
        , lock_out()
        , pq_in(nullptr)
        , pq_out(nullptr)
        , s_in(0)
        , s_out(0) {
        pq_in = new Msg_ptr[MQ_LEN];
        pq_out = new Msg_ptr[MQ_LEN];
    }

    virtual ~BQ()
    {
	lock_out.lock();
	lock_in.lock();
        delete[] pq_in;
        delete[] pq_out;
	lock_in.unlock();
	lock_out.unlock();
    }

    bool		pop_msg(Msg_ptr & msg)
    {
        //lock_out.lock();
        if (s_out == 0)
        {
            lock_in.lock();
            MQ_t t = pq_in;
            pq_in = pq_out;
            pq_out = t;
            size_t ts = s_in;
            s_in = s_out;
            s_out = ts;
            lock_in.unlock();
        }
        if(s_out == 0) {
            //lock_out.unlock();
            return false;
        }
        s_out --;
        msg = pq_out[s_out];
        //lock_out.unlock();
        return true;
    }

    std::mutex & 	in_lock() {
        return lock_in;
    }
    std::mutex &	out_lock() {
        return lock_out;
    }


    bool		push_msg(Msg_ptr &msg)
    {
        //lock_in.lock();
        if(s_in == MQ_LEN)
        {
            lock_in.unlock();
            lock_out.lock();
            lock_in.lock();
            MQ_t t = pq_in;
            pq_in = pq_out;
            pq_out = t;
            size_t ts = s_in;
            s_in = s_out;
            s_out = ts;
            lock_out.unlock();
        }
        if(s_in == MQ_LEN)
        {
            //lock_in.unlock();
            return false;
        }
        pq_in[s_in] = msg;
        s_in ++;
        //lock_in.unlock();
        return true;

    }
protected:
    typedef Msg_ptr * MQ_t;

    std::mutex	lock_in;
    std::mutex	lock_out;
    MQ_t		pq_in;
    MQ_t		pq_out;
    size_t	s_in;
    size_t	s_out;
};

class BQ_TBB {
public:
    BQ_TBB()
        : lock_in()
        , lock_out()
        , pq_in(nullptr)
        , pq_out(nullptr)
        , s_in(0)
        , s_out(0) {
        pq_in = new Msg_ptr[MQ_LEN];
        pq_out = new Msg_ptr[MQ_LEN];
    }

    virtual ~BQ_TBB()
    {                
        out_lock();
        in_lock();
        delete[] pq_in;
        delete[] pq_out;
        in_unlock();
        out_unlock();
    }

    bool                pop_msg(Msg_ptr & msg)
    {
        //lock_out.lock();
        if (s_out == 0)
        {
            in_lock();
            MQ_t t = pq_in;
            pq_in = pq_out;
            pq_out = t;
            size_t ts = s_in;
            s_in = s_out;
            s_out = ts;
            in_unlock();
        }
        if(s_out == 0) {
            //lock_out.unlock();
            return false;
        }
        s_out --;
        msg = pq_out[s_out];
        //lock_out.unlock();
        return true;
    }

//     tbb::mutex &    in_lock() {
//         return lock_in;
//     }
//     tbb::mutex &    out_lock() {
//         return lock_out;
//     }
    
    void in_lock(void) {
        lock_in.acquire(mutex_in);
    }
    
    void in_unlock(void) {
        lock_in.release();
    }
    
    void out_lock(void) {
        lock_out.acquire(mutex_out);
    }
    
    void out_unlock(void) {
        lock_out.release();
    }


    bool                push_msg(Msg_ptr &msg)
    {
        //lock_in.lock();
        if(s_in == MQ_LEN)
        {
            in_unlock();
            out_lock();
            in_lock();
            MQ_t t = pq_in;
            pq_in = pq_out;
            pq_out = t;
            size_t ts = s_in;
            s_in = s_out;
            s_out = ts;
            out_unlock();
        }
        if(s_in == MQ_LEN)
        {
            //lock_in.unlock();
            return false;
        }
        pq_in[s_in] = msg;
        s_in ++;
        //lock_in.unlock();
        return true;

    }
protected:
    typedef Msg_ptr * MQ_t;

    tbb::mutex      mutex_in;
    tbb::mutex      mutex_out;
    tbb::mutex::scoped_lock lock_in;
    tbb::mutex::scoped_lock lock_out;
    MQ_t                pq_in;
    MQ_t                pq_out;
    size_t      s_in;
    size_t      s_out;
};

typedef Msg_ptr	*	MQ_t;

void serial(int task_num)
{
    MQ_t p = new Msg_ptr[MQ_LEN];

    for(int i = 0; i < task_num; ++i)
    {

    }
}
#define MIN_NUM 50000
#define MAX_NUM 100000

void parallel_bq(int msg_num, int thrd_num, BQ_TBB & bq)
{
    int iMsg_num = 0;

    tbb::task_group tg;
    while(iMsg_num < msg_num)
    {
        std::random_device rd;
        std::default_random_engine dre(rd());
        std::uniform_int_distribution<int> dist(MIN_NUM, MAX_NUM);
        int m = dist(dre);

        tg.run([&bq, m]() {
            bq.in_lock();
            for(int i = 0; i < m; i ++)
            {
                Msg_ptr msg = std::make_shared<Msg>();
                bq.push_msg(msg);
            }
            bq.in_unlock();
        });

        iMsg_num += m;

        std::mt19937 e2(rd());
        std::normal_distribution<> ndist(m, 2);
        int n = ndist(e2);

        tg.run([&bq, n]() {
            bq.out_lock();
            for(int i = 0; i<n; ++i)
            {
                Msg_ptr msg;
                bq.pop_msg(msg);
            }
            bq.out_unlock();
        });
    }
    tg.wait();
}

// void parallel_bq(int msg_num, int thrd_num, BQ<std::mutex> & bq)
void parallel_bq(int msg_num, int thrd_num, BQ & bq)
{
    int iMsg_num = 0;

    tbb::task_group tg;
    while(iMsg_num < msg_num)
    {
        std::random_device rd;
        std::default_random_engine dre(rd());
        std::uniform_int_distribution<int> dist(MIN_NUM, MAX_NUM);
        int m = dist(dre);

        tg.run([&bq, m]() {
            bq.in_lock().lock();
            for(int i = 0; i < m; i ++)
            {
                Msg_ptr msg = std::make_shared<Msg>();
                bq.push_msg(msg);
            }
            bq.in_lock().unlock();
        });

        iMsg_num += m;

        std::mt19937 e2(rd());
        std::normal_distribution<> ndist(m, 2);
        int n = ndist(e2);

        tg.run([&bq, n]() {
            bq.out_lock().lock();
            for(int i = 0; i<n; ++i)
            {
                Msg_ptr msg;
                bq.pop_msg(msg);
            }
            bq.out_lock().unlock();
        });
    }
    tg.wait();
}
void parallel(int msg_num, int thrd_num, bool tbb_lock)
{
  int sub = msg_num/Q_N;
  tbb::task_group tg;
  for(int i = 0; i <Q_N; ++i)
  {
    tg.run([sub, thrd_num, tbb_lock](){
    if(tbb_lock)
    {
        BQ_TBB bq;
        parallel_bq(sub, thrd_num, bq);
    }
    else
    {
        BQ bq;
        parallel_bq(sub, thrd_num, bq);
    }
    });
  }
  tg.wait();
}


