#ifndef __SCC_INTERNIFY_HPP__
#define __SCC_INTERNIFY_HPP__
#pragma once

#include <string>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include <memory>

namespace scc
{
    template <typename T, typename HashFunc = std::hash<T>>
    class Internify
    {
    public:
        class InternedPtr
        {
        public:
            InternedPtr(Internify *owner, const T *ptr)
                : m_owner(owner), m_ptr(ptr) {}

            InternedPtr(InternedPtr &&other) noexcept
                : m_owner(other.m_owner), m_ptr(other.m_ptr)
            {
                other.reset();
            }

            InternedPtr &operator=(InternedPtr &&other) noexcept
            {
                if (this != &other)
                {
                    release();
                    m_owner = other.m_owner;
                    m_ptr = other.m_ptr;
                    other.reset();
                }
                return *this;
            }

            ~InternedPtr()
            {
                release();
            }

            const T *get() const { return m_ptr; }

            const T &operator*() const { return *m_ptr; }
            const T *operator->() const { return m_ptr; }

            operator bool() const { return m_ptr != nullptr; }

            bool operator==(const InternedPtr &other) const { return m_ptr == other.m_ptr; }
            bool operator!=(const InternedPtr &other) const { return m_ptr != other.m_ptr; }

            // Disable copying
            InternedPtr(const InternedPtr &) = delete;
            InternedPtr &operator=(const InternedPtr &) = delete;

            void release()
            {
                if (m_owner && m_ptr)
                {
                    m_owner->release(*m_ptr);
                }

                reset();
            }

        private:
            void reset()
            {
                m_owner = nullptr;
                m_ptr = nullptr;
            }

            Internify *m_owner = nullptr;
            const T *m_ptr = nullptr;
        };

        Internify() = default;
        ~Internify() = default;

        Internify(const Internify &) = delete;
        Internify &operator=(const Internify &) = delete;

        [[nodiscard]] InternedPtr internify(const T &value)
        {
            const T *existing = findExisting(value);
            if (existing)
            {
                return InternedPtr(this, existing);
            }
            return InternedPtr(this, insertNew(value));
        }

        [[nodiscard]] InternedPtr find(const T &value) const
        {
            std::shared_lock lock(m_mutex);
            auto it = m_interningMap.find(hashValue(value));
            if (it != m_interningMap.end())
            {
                it->second->refCount.fetch_add(1, std::memory_order_relaxed);
                return InternedPtr(const_cast<Internify *>(this), &(it->second->value));
            }
            return InternedPtr(nullptr, nullptr);
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
        struct InterningNode
        {
            explicit InterningNode(const T &val)
                : value(val), refCount(1) {}

            const T value;
            std::atomic<int> refCount;
        };

        using HashedValue = decltype(std::declval<HashFunc>()(std::declval<T>()));

        void release(const T &value)
        {
            std::unique_lock lock(m_mutex);
            auto it = m_interningMap.find(hashValue(value));
            if (it != m_interningMap.end() && it->second->refCount.fetch_sub(1, std::memory_order_relaxed) == 1)
            {
                m_interningMap.erase(it);
            }
        }

        const T *findExisting(const T &value) const
        {
            std::shared_lock lock(m_mutex);
            auto it = m_interningMap.find(hashValue(value));
            if (it != m_interningMap.end())
            {
                it->second->refCount.fetch_add(1, std::memory_order_relaxed);
                return &(it->second->value);
            }
            return nullptr;
        }

        const T *insertNew(const T &value)
        {
            std::unique_lock lock(m_mutex);
            auto [it, inserted] = m_interningMap.try_emplace(hashValue(value),
                                                             std::make_unique<InterningNode>(value));
            if (inserted)
            {
                return &(it->second->value);
            }
            else
            {
                it->second->refCount.fetch_add(1, std::memory_order_relaxed);
                return &(it->second->value);
            }
        }

        HashedValue hashValue(const T &value) const
        {
            return HashFunc{}(value);
        }

        std::unordered_map<HashedValue, std::unique_ptr<InterningNode>> m_interningMap;
        mutable std::shared_mutex m_mutex;
    };
}

#endif // __SCC_INTERNIFY_HPP__
