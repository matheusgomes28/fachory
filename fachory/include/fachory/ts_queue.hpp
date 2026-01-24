#ifndef FACHORY_TS_QUEUE_H
#define FACHORY_TS_QUEUE_H

#include <deque>
#include <mutex>
#include <optional>

namespace fachory::app {
    template <typename T>
    class ThreadSafeQueue {
    public:
        void push_back(T elem) {
            std::lock_guard<std::mutex> const queue_lk{_queue_m};
            _queue.push_back(std::move(elem));
        }

        std::optional<T> pop_back() {
            std::lock_guard<std::mutex> const queue_lk{_queue_m};
            if (_queue.size() < 1) {
                return std::nullopt;
            }

            auto const elem = std::move(_queue.front());
            _queue.pop_front();
            return elem;
        }

        void push_front(T elem) {
            std::lock_guard<std::mutex> const queue_lk{_queue_m};
            _queue.push_front(std::move(elem));
        }

        std::optional<T> pop_front() {
            std::lock_guard<std::mutex> const queue_lk{_queue_m};
            if (_queue.size() < 1) {
                return std::nullopt;
            }

            auto const elem = std::move(_queue.back());
            _queue.pop_back();
            return elem;
        }

    private:
        std::deque<T> _queue;
        std::mutex _queue_m;
    };
} // namespace fachory::app

#endif // FACHORY_TS_QUEUE_H
