#ifndef APP_DB_TASK_H
#define	APP_DB_TASK_H

#include "Config.h"
#include <string>
#include <vector>
#include "db/Util.h"

namespace app {
namespace db {
class Connector;
class Task;
using FuncDBTask = void (*)(Task*, Connector*);

/**
 * @brief 封装DB请求数据
 * 可使用Task::Arg实现字符转义
 */
class Task {
    friend class Connector;
    friend class Database;

public:

    /**
    * @brief Arg结构，用于构造流式任务,自动推断是否需要转义。
    */
    struct Arg {
        /**
         * @param T类型为string时将自动转义。
         */
        template <typename T>
        Arg(const T& param);

        std::string mValStr;
        bool mRequiresEscaping;
    };

    Task(FuncDBTask func, u32 timeout = 10) :mFuncFinish(func), mTimeout(timeout) {

    }

    ~Task() = default;
    Task() = default;

    Task(const Task&) = default;
    Task(Task&&) = default;
    Task& operator=(const Task&) noexcept = default;
    Task& operator=(Task&&) noexcept = default;

    /**
     * @brief 追加无需转义的命令
     * @param tpart 命令, 不提供自动转义
     * @return A reference to this task.
     */
    template <typename T>
    Task& operator<<(const T& param);

    /**
     * @brief 追加命令
     * @param tpart 命令, Arg提供string类型的自动转义
     * @return A reference to this task.
     */
    Task& operator<<(Arg tpart);

    /**
     * @brief must set the FuncDBTask of task
     * @param timeout The timeout for this task, in seconds.
     * @param func The on complete callback.
     */
    void setFuncFinish(FuncDBTask func, u32 timeout = 10) {
        mFuncFinish = func;
        mTimeout = AppClamp(timeout, 1U, 0x7FFFFFFFU);
    }

private:
    /**
     * 内部结构，类似于Arg，但不执行转义且不推断是否需要转义。
     */
    struct TaskPart {
        TaskPart(std::string value, bool requires_escaping);

        std::string mValStr;
        bool mRequiresEscaping;
    };

    u32 mTimeout = 10;
    FuncDBTask mFuncFinish = nullptr;
    std::vector<TaskPart> mTaskPart;

    void clear() {
        mTaskPart.clear();
    }

    /**
     * @brief 合并所有任务的命令行
     * @param funcescape 转义函数
     * @return the final commands
     */
    template <typename FuncEscape>
    std::string prepareCMD(FuncEscape funcescape);
};



/////////////////////////////////////////////////////////////////////////

template <typename T>
Task::Arg::Arg(const T& param) {
    using RawType = typename std::remove_cv<T>::type;
    constexpr bool is_string_type = std::is_constructible<std::string, RawType>::value;

    // all string values must be escaped by MySQL
    mRequiresEscaping = is_string_type;

    if (is_string_type) {
        mValStr = param;
    } else {
        auto& stream = getThreadLocalStream();
        stream << param; // error if you're trying to bind using a non-streamable type
        mValStr = stream.str();
    }
    if (mValStr.empty()) {
        throw std::invalid_argument("Argument evaulates to empty string");
    }
}

template <typename T>
Task& Task::operator<<(const T& param) {
    auto& stream = getThreadLocalStream();
    stream << param;
    mTaskPart.emplace_back(stream.str(), false);
    return *this;
}

template <typename FuncEscape>
std::string Task::prepareCMD(FuncEscape escape_functor) {
    std::string finalReq;

    for (auto& part : mTaskPart) {
        if (part.mRequiresEscaping) {
            finalReq.append(escape_functor(part.mValStr));
        } else {
            finalReq.append(part.mValStr);
        }
    }

    return finalReq;
}

} //namespace db
} //namespace app

#endif //APP_DB_TASK_H