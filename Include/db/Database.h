#ifndef APP_DB_DATABASE_H
#define	APP_DB_DATABASE_H


#include "ThreadPool.h"
#include "db/ConnectorPool.h"


namespace app {
namespace db {

class Database {
public:
    explicit Database(ConnectConfig coninfo);

    ~Database();

    Database(const Database&) = delete;
    Database(Database&&) = delete;
    Database& operator=(const Database&) = delete;
    Database& operator=(Database&&) = delete;

    /**
     * @brief wait all task finished, then stop all threads.
    */
    void stop() {
        mThreads.stop();
        usz closed = mPool.close();
        printf("closed db connectors=%llu\n", closed);
    }

    /**
     * @brief open.
     * @param pmax max threads.
     */
    void start(u32 pmax);

    /**
     * @brief The given task launch with timeout and on complete callback.  The on complete
     * callback will be called when the task is completed or timed out.
     * @param task The task to execute.
     * @param timeout The timeout for this task, in seconds.
     * @param funcFinish The on complete callback.
     * @return true if success, else flase.
     */
    bool postTask(Task* task);

private:
    ConnectorPool mPool;
    ThreadPool mThreads;

    void executor(Task* task);
};

} //namespace db
} //namespace app

#endif //APP_DB_DATABASE_H