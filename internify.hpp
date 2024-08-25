#ifndef __SCC_INTERNIFY_HPP__
#define __SCC_INTERNIFY_HPP__
#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <functional>

namespace scc
{
    template <typename T>
    struct InterningNode
    {
        explicit InterningNode(const T &val)
            : value(std::make_shared<T>(val)), refCount(1) {}

        const std::shared_ptr<T> value;
        std::atomic<int> refCount;
    };

    template <typename T, typename HashFunc = std::hash<T>>
    class Internify
    {
    public:
        Internify() = default;
        ~Internify() = default;

        Internify(const Internify &) = delete;
        Internify &operator=(const Internify &) = delete;

        std::shared_ptr<T> internify(const T &value)
        {
            if (auto existing = findExisting(value))
            {
                return existing;
            }
            return insertNew(value);
        }

        void release(const T &value)
        {
            std::unique_lock lock(m_mutex);
            auto it = m_interningMap.find(hashValue(value));
            if (it != m_interningMap.end())
            {
                if (it->second->refCount.fetch_sub(1, std::memory_order_relaxed) == 1)
                {
                    m_interningMap.erase(it);
                }
            }
        }

        std::shared_ptr<T> find(const T &value) const
        {
            std::shared_lock lock(m_mutex);
            auto it = m_interningMap.find(hashValue(value));
            if (it != m_interningMap.end())
            {
                return it->second->value;
            }
            return nullptr;
        }

        void erase(const T &value)
        {
            std::unique_lock lock(m_mutex);
            m_interningMap.erase(hashValue(value));
        }

        std::size_t size() const
        {
            std::shared_lock lock(m_mutex);
            return m_interningMap.size();
        }

    private:
        using HashedValue = decltype(std::declval<HashFunc>()(std::declval<T>()));

        HashedValue hashValue(const T &value) const
        {
            HashFunc hasher;
            return hasher(value);
        }

        std::shared_ptr<T> findExisting(const T &value)
        {
            std::shared_lock lock(m_mutex);
            auto it = m_interningMap.find(hashValue(value));
            if (it != m_interningMap.end())
            {
                it->second->refCount.fetch_add(1, std::memory_order_relaxed);
                return it->second->value;
            }
            return nullptr;
        }

        std::shared_ptr<T> insertNew(const T &value)
        {
            std::unique_lock lock(m_mutex);
            auto hashed = hashValue(value);
            auto [it, inserted] = m_interningMap.try_emplace(hashed,
                                                             std::make_shared<InterningNode<T>>(value));
            if (inserted)
            {
                return it->second->value;
            }
            else
            {
                it->second->refCount.fetch_add(1, std::memory_order_acquire);
                return it->second->value;
            }
        }

        std::unordered_map<HashedValue, std::shared_ptr<InterningNode<T>>> m_interningMap;
        mutable std::shared_mutex m_mutex;
    };
}

#endif // __SCC_INTERNIFY_HPP__
