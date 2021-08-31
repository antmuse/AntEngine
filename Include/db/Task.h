#ifndef APP_DB_TASK_H
#define	APP_DB_TASK_H

#include "Config.h"
#include <string>
#include <vector>
#include <type_traits>
#include "db/Util.h"

namespace app {
namespace db {

/**
 * @brief ��װDB��������
 * ��ʹ��Task::Argʵ���ַ�ת��
 */
class Task {
    friend class Connector;

public:

    /**
    * @brief Arg�ṹ�����ڹ�����ʽ����,�Զ��ƶ��Ƿ���Ҫת�塣
    */
    struct Arg {
        /**
         * @param T����Ϊstringʱ���Զ�ת�塣
         */
        template <typename T>
        Arg(const T& param);

        std::string mValStr;
        bool mRequiresEscaping;
    };

    ~Task() = default;
    Task() = default;

    Task(const Task&) = default;
    Task(Task&&) = default;
    Task& operator=(const Task&) noexcept = default;
    Task& operator=(Task&&) noexcept = default;

    /**
     * @brief ׷������ת�������
     * @param tpart ����, ���ṩ�Զ�ת��
     * @return A reference to this task.
     */
    template <typename T>
    Task& operator<<(const T& param);

    /**
     * @brief ׷������
     * @param tpart ����, Arg�ṩstring���͵��Զ�ת��
     * @return A reference to this task.
     */
    Task& operator<<(Arg tpart);

private:
    /**
     * �ڲ��ṹ��������Arg������ִ��ת���Ҳ��ƶ��Ƿ���Ҫת�塣
     */
    struct TaskPart {
        TaskPart(std::string value, bool requires_escaping);

        std::string mValStr;
        bool mRequiresEscaping;
    };

    std::vector<TaskPart> mTaskPart;

    void clear() {
        mTaskPart.clear();
    }

    /**
     * @brief �ϲ����������������
     * @param funcescape ת�庯��
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