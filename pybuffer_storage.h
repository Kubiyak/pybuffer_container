/*
 * The MIT License
 *
 * Copyright 2020 Kuberan Naganathan
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
# pragma once
#include <snapshot_container/snapshot_storage.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>


namespace pybuffer_container
{
    template <typename T>
    class vector_storage: public snapshot_container::storage_base<T, 48, virtual_iter::rand_iter<T, 48>>
    {
    public:
            // Unlike the default deque_storage implementation in snapshot_container, pybuffer_storage is built
            // on top of a std::vector. The reason for this is that std::vector provides access to its underlying storage
            // and this will be used to interface c++ containers built from elements of this type to python via
            // the buffer protocol. Otherwise, this is almost a direct copy of the code from deque_storage.
            // TODO: Refactor out the commonality if possible.

        static const size_t npos = 0xFFFFFFFFFFFFFFFF;
        typedef typename snapshot_container::storage_base<T, 48, virtual_iter::rand_iter<T,48>> storage_base_t;
        using storage_base_t::iter_mem_size;
        typedef T value_type;
        typedef std::shared_ptr<vector_storage<T>> shared_t;
        typedef std::shared_ptr<storage_base_t> shared_base_t;
        using fwd_iter_type = typename storage_base_t::fwd_iter_type;
        using rand_iter_type = typename storage_base_t::rand_iter_type;
        typedef virtual_iter::rand_iter<T,48> storage_iter_type;

        void append(const T& value) override
        {
            m_data.push_back (value);
        }

        void append(const fwd_iter_type& start_pos, const fwd_iter_type& end_pos) override
        {
            fwd_iter_type start_pos_copy(start_pos);

            std::function<bool(const value_type& v)> f =
                    [this](const value_type& v)
                    {
                        m_data.push_back (v);
                        return true;
                    };

            start_pos_copy.visit (end_pos, f);
        }

        void append(const rand_iter_type& start_pos, const rand_iter_type& end_pos) override
        {
            for (auto current_pos = start_pos; current_pos != end_pos; ++current_pos)
                m_data.push_back(*current_pos);
        }

        shared_base_t copy(size_t start_index = 0, size_t end_index = npos) const override;

        void insert(size_t index, const T& value) override
        {
            m_data.insert (m_data.begin () + index, value);
        }

        void insert(size_t index, const fwd_iter_type& start_pos, const fwd_iter_type& end_pos) override
        {
            m_data.insert(m_data.begin() + index, start_pos, end_pos);
        }

        void insert(size_t index, const rand_iter_type& start_pos, const rand_iter_type& end_pos) override
        {
            m_data.insert(m_data.begin() + index, start_pos, end_pos);
        }

        void remove(size_t index) override
        {
            m_data.erase(m_data.begin() + index);
        }

        void remove(size_t start_index, size_t end_index) override
        {
            m_data.erase(m_data.begin() + start_index, m_data.begin() + end_index);
        }

        size_t size() const override
        {return m_data.size ();}

        const T& operator[](size_t index) const override
        {return m_data[index];}

        T& operator[](size_t index) override
        {return m_data[index];}

        const storage_iter_type begin() const override
        {
            return storage_iter_type(_iter_impl, m_data.begin());
        }

        const storage_iter_type end() const override
        {
            return storage_iter_type(_iter_impl, m_data.end());
        }

        const storage_iter_type iterator(size_t offset) const override
        {
            if (offset > m_data.size())
                offset = m_data.size();
            return storage_iter_type(_iter_impl, m_data.begin() + offset);
        }

        storage_iter_type begin() override
        {
            return storage_iter_type(_iter_impl, m_data.begin());
        }

        storage_iter_type end() override
        {
            return storage_iter_type(_iter_impl, m_data.end());
        }

        storage_iter_type iterator(size_t offset) override
        {
            if (offset > m_data.size())
                offset = m_data.size();
            return storage_iter_type(_iter_impl, m_data.begin() + offset);
        }

        size_t id() const override
        {
            return m_storage_id;
        }

        static shared_t create();

        template <typename InputIter>
        static shared_t create(InputIter start_pos, InputIter end_pos);

        // The copy constructors should never be called. All construction is through the storage creator mechanism
        vector_storage(const vector_storage<T>& rhs) = delete;
        vector_storage(vector_storage<T>&& rhs) = delete;

        // access to underlying buffer. Should only be accessed from a snapshot and will only be valid
        // while the provider snapshot is live.
        const T* data() const
        {
            return m_data.data();
        }

        vector_storage():
        m_storage_id(storage_base_t::generate_storage_id())
        {}

        template <typename InputIter>
        vector_storage(InputIter start_pos, InputIter end_pos);

    private:
        static virtual_iter::std_rand_iter_impl<typename std::vector<value_type>::const_iterator, iter_mem_size> _iter_impl;
        std::vector<T> m_data;
        size_t m_storage_id;
    };


    template <typename T>
    template <typename InputIter>
    vector_storage<T>::vector_storage(InputIter start_pos, InputIter end_pos):
        m_data (start_pos, end_pos),
        m_storage_id(storage_base_t::generate_storage_id())
    {}


    template <typename T>
    typename vector_storage<T>::shared_base_t vector_storage<T>::copy(size_t start_index, size_t end_index) const
    {
        if (end_index == npos)
            end_index = m_data.size();

        auto new_storage = new vector_storage<T> (m_data.begin () + start_index, m_data.begin () + end_index);
        return vector_storage<T>::shared_base_t (new_storage);
    }


    template <typename T>
    typename vector_storage<T>::shared_t vector_storage<T>::create()
    {
        return std::make_shared<vector_storage<T>>();
    }


    template <typename T>
    template <typename InputItr>
    typename vector_storage<T>::shared_t vector_storage<T>::create(InputItr start_pos, InputItr end_pos)
    {
        return std::make_shared<vector_storage<T>>(start_pos, end_pos);
    }


    template <typename T>
    virtual_iter::std_rand_iter_impl<typename std::vector<T>::const_iterator, vector_storage<T>::iter_mem_size> vector_storage<T>::_iter_impl;


    template <typename T>
    struct vector_storage_creator
    {
        typedef typename vector_storage<T>::shared_base_t shared_base_t;
        shared_base_t operator() ()
        {
            return shared_base_t(vector_storage<T>::create());
        }

        template <typename IterType>
        shared_base_t operator() (IterType start_pos, IterType end_pos)
        {
            return shared_base_t(vector_storage<T>::create(start_pos, end_pos));
        }
    };


    template <typename T>
    struct _pybuffer_storage_control_block
    {
        std::mutex m_mutex;
        std::unordered_map<size_t, std::weak_ptr<vector_storage<T>>> m_map;
    };


    // Stateful storage creator with locate capability. Ideally this would be implemented with
    // a control block pointed at by std::atomic<std::shared_ptr>. This will need to wait for c++20
    template <typename T>
    struct pybuffer_storage_creator
    {
        typedef typename vector_storage<T>::shared_base_t shared_base_t;
        typedef typename vector_storage<T>::shared_t shared_t;
        typedef _pybuffer_storage_control_block<T> control_t;

        pybuffer_storage_creator():
        m_control(std::make_shared<control_t>())
        {
        }

        pybuffer_storage_creator(const pybuffer_storage_creator& other) = default;
        pybuffer_storage_creator& operator = (const pybuffer_storage_creator& other) = default;


        shared_base_t operator() ()
        {
            auto storage = vector_storage<T>::create();
            auto shared_t_storage = std::static_pointer_cast<vector_storage<T>>(storage);
            std::lock_guard<std::mutex> guard(m_control->m_mutex);
            m_control->m_map.insert(std::pair<size_t, std::weak_ptr<vector_storage<T>>>(storage->id(),
                                    std::weak_ptr<vector_storage<T>>(shared_t_storage)));

            return storage;
        }

        template <typename IterType>
        shared_base_t operator() (IterType start_pos, IterType end_pos)
        {
            auto storage = vector_storage<T>::create(start_pos, end_pos);
            auto shared_t_storage = std::static_pointer_cast<vector_storage<T>>(storage);
            std::lock_guard<std::mutex> guard(m_control->m_mutex);
            m_control->m_map.insert(std::pair<size_t, std::weak_ptr<vector_storage<T>>>(storage->id(),
                                    std::weak_ptr<vector_storage<T>>(shared_t_storage)));
            return storage;
        }

        // Obtain a shared ptr to the storage identified by id. Returns an empty shared_ptr if not found
        // or if the ptr has expired. Note the returned type is shared_t (std::shared_ptr<vector_storage<T>>)
        shared_t locate(size_t id)
        {
            std::lock_guard<std::mutex> guard(m_control->m_mutex);
            auto result = m_control->m_map.find(id);
            if (result == m_control->m_map.end())
                return shared_t();
            else
                return result->second.lock();
        }

        private:
            std::shared_ptr<control_t> m_control;
    };
}
