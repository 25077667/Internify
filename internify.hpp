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
    /**
     * @brief The Internify class template provides a mechanism for interning objects of type T.
     *
     * Interning is a process where identical objects are stored only once in memory,
     * and all references to these objects point to the same memory location.
     * This can reduce memory usage and improve performance in cases where many identical objects are used.
     *
     * @tparam T The type of objects to be interned. T must be copyable.
     * @tparam HashFunc A hash function object that takes an object of type T and returns a std::size_t. Defaults to std::hash<T>.
     */
    template <typename T, typename HashFunc = std::hash<T>>
    class Internify
    {
    public:
        /**
         * @brief A smart pointer-like object that manages a reference to an interned object.
         *
         * This class is responsible for managing the reference count of the interned object and
         * ensures that the object is only deleted when there are no more references.
         */
        class InternedPtr
        {
        public:
            /**
             * @brief Constructs an InternedPtr that points to an interned object managed by owner.
             *
             * @param owner Pointer to the owning Internify instance.
             * @param ptr Pointer to the interned object.
             */
            InternedPtr(Internify *owner, const T *ptr)
                : m_owner(owner), m_ptr(ptr) {}

            /**
             * @brief Move constructor. Transfers ownership from other to the new InternedPtr.
             *
             * @param other The other InternedPtr to move from.
             */
            InternedPtr(InternedPtr &&other) noexcept
                : m_owner(other.m_owner), m_ptr(other.m_ptr)
            {
                other.reset();
            }

            /**
             * @brief Move assignment operator. Transfers ownership from other to the current InternedPtr.
             *
             * @param other The other InternedPtr to move from.
             * @return InternedPtr& Reference to the current InternedPtr.
             */
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

            /**
             * @brief Destructor. Decrements the reference count of the interned object and deletes it if no more references exist.
             */
            ~InternedPtr()
            {
                release();
            }

            /**
             * @brief Returns a pointer to the interned object.
             *
             * @return const T* Pointer to the interned object.
             */
            const T *get() const { return m_ptr; }

            /**
             * @brief Dereferences the pointer to access the interned object.
             *
             * @return const T& Reference to the interned object.
             * @note it is undefined behavior if this instance is not valid.
             */
            const T &operator*() const { return *m_ptr; }

            /**
             * @brief Returns a pointer to the interned object.
             *
             * @return const T* Pointer to the interned object.
             */
            const T *operator->() const { return m_ptr; }

            /**
             * @brief Checks if the InternedPtr is valid (i.e., points to an interned object).
             *
             * @return true If the InternedPtr is valid, false otherwise.
             */
            operator bool() const { return m_ptr != nullptr && m_owner != nullptr; }

            /**
             * @brief Returns true if the InternedPtr is valid, false otherwise.
             *
             * @return true If the InternedPtr is valid, false otherwise.
             */
            bool is_valid() const { return m_ptr != nullptr && m_owner != nullptr; }

            /**
             * @brief Compares two InternedPtr objects for equality.
             *
             * @param other The other InternedPtr to compare with.
             * @return true If both InternedPtr objects point to the same interned object.
             * @return false If the InternedPtr objects point to different interned objects.
             */
            bool operator==(const InternedPtr &other) const { return m_ptr == other.m_ptr; }

            /**
             * @brief Compares two InternedPtr objects for inequality.
             *
             * @param other The other InternedPtr to compare with.
             * @return true If the InternedPtr objects point to different interned objects.
             * @return false If both InternedPtr objects point to the same interned object.
             */
            bool operator!=(const InternedPtr &other) const { return m_ptr != other.m_ptr; }

            // Disable copying
            InternedPtr(const InternedPtr &) = delete;
            InternedPtr &operator=(const InternedPtr &) = delete;

            /**
             * @brief Releases the interned object, decrementing its reference count.
             */
            void release()
            {
                if (m_owner && m_ptr)
                {
                    m_owner->release(*m_ptr);
                }

                reset();
            }

        private:
            /**
             * @brief Resets the InternedPtr to an invalid state.
             */
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

        /**
         * @brief Interns the given value. If the value is already interned, returns an InternedPtr pointing to the existing interned object.
         *
         * If the value is not already interned, inserts the value into the intern pool and returns an InternedPtr pointing to the newly interned object.
         *
         * @param value The value to be interned.
         * @return InternedPtr A smart pointer to the interned object.
         */
        [[nodiscard]] InternedPtr internify(const T &value)
        {
            const T *existing = findExisting(value);
            if (existing)
            {
                return InternedPtr(this, existing);
            }
            return InternedPtr(this, insertNew(value));
        }

        /**
         * @brief Finds the interned object corresponding to value without creating a new entry.
         *
         * If the value is not found, returns an invalid InternedPtr.
         *
         * @param value The value to find in the intern pool.
         * @return InternedPtr A smart pointer to the interned object, or an invalid InternedPtr if the object is not found.
         */
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

        /**
         * @brief Returns the number of unique interned objects currently stored in the intern pool.
         *
         * @return std::size_t The number of interned objects.
         */
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

        /**
         * @brief Decrements the reference count of the interned object corresponding to value.
         *
         * If the reference count reaches zero, the object is removed from the intern pool.
         *
         * @param value The value whose reference count should be decremented.
         */
        void release(const T &value)
        {
            std::unique_lock lock(m_mutex);
            auto it = m_interningMap.find(hashValue(value));
            if (it != m_interningMap.end() && it->second->refCount.fetch_sub(1, std::memory_order_relaxed) == 1)
            {
                m_interningMap.erase(it);
            }
        }

        /**
         * @brief Finds an existing interned object corresponding to value.
         *
         * If found, increments the reference count.
         *
         * @param value The value to find.
         * @return const T* A pointer to the interned object, or nullptr if the object is not found.
         */
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

        /**
         * @brief Inserts a new object into the intern pool and returns a pointer to the interned object.
         *
         * @param value The value to insert.
         * @return const T* A pointer to the newly interned object.
         */
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

        /**
         * @brief Hashes the given value using the hash function provided in the template parameter.
         *
         * @param value The value to hash.
         * @return HashedValue The hashed value.
         */
        HashedValue hashValue(const T &value) const
        {
            return HashFunc{}(value);
        }

        std::unordered_map<HashedValue, std::unique_ptr<InterningNode>> m_interningMap;
        mutable std::shared_mutex m_mutex;
    };
}

#endif // __SCC_INTERNIFY_HPP__
