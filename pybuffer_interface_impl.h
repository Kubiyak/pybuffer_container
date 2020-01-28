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
#pragma once
#include "pybuffer_interface.h"


namespace pybuffer_container_detail
{
    template <typename T>
    void PyBufferViewWrapperImpl<T>::tp_dealloc(PyObject * obj)
    {
        using namespace pybuffer_container;
        PyBufferViewWrapper<T> * view_wrapper = static_cast<PyBufferViewWrapper*>(obj);
        if (obj->m_impl)
            delete obj->m_impl;

        // auto type_object = Py_Type(obj);

        // PyBufferViewWrapper objects are constructed with c++ new
        delete view_wrapper;
        return;
    }


    template <typename T>
    PyObject * tp_str(PyObject * obj)
    {
        using namespace pybuffer_container;
        PyBufferViewWrapper * view_wrapper = static_cast<PyBufferViewWrapper*>(obj);
        // TODO: Add more detail
        return PyString_FromString("PyBufferViewWrapper instance");
    }


    template <typename T>
    Py_ssize_t sq_length(PyObject * obj)
    {
        using namespace pybuffer_container;
        PyBufferViewWrapper * view_wrapper = static_cast<PyBufferViewWrapper*>(obj);
        if (!view_wrapper->m_impl)
            return 0;
        return view_wrapper->m_impl->m_storage_elements->size();
    }


    template <typename T>
    PyObject * PyBufferViewWrapperImpl<T>::sq_item(PyObject * obj, Py_ssize_t index)
    {
        using namespace pybuffer_container;
        PyBufferViewWrapper * view_wrapper = static_cast<PyBufferViewWrapper*>(obj);
        if (index < 0 || index >= obj->m_impl->m_storage_elements.size())
            return PyErr_SetString(PyExc_IndexError, "Index out of bounds to PyBufferViewWrapper object");
        return PyBufferStorageWrapper<T>::create_py_storage_wrapper(view_wrapper, index);
    }


    template <typename T>
    void PyBufferStorageWrapperImpl<T>::tp_dealloc(PyObject * object)
    {
        using namespace pybuffer_container;
        PyBufferStorageWrapper * storage_wrapper = static_cast<PyBufferStorageWrapper*>(object);
        delete storage_wrapper->m_impl;
        delete storage_wrapper;
        return;
    }


    template <typename T>
    PyObject* PyBufferStorageWrapperImpl<T>::tp_str(PyObject * object)
    {
        return PyString_FromString("PyBufferStorage instance");
    }


    template <typename T>
    int PyBufferStorageWrapperImpl<T>::bf_getbuffer(PyObject* exporter, Py_buffer* view, int flags)
    {
        using namespace pybuffer_container;
        // Only C-style contiguous RO buffers are supported
        if (flags & (PyBUF_WRITABLE | PyBUF_F_CONTIGUOUS))
        {
            view->obj = 0;
            return -1;
        }

        pybuffer_container::PyBufferStorageWrapper<T> * wrapper = static_cast<PyBufferStorageWrapper*>(exporter);
        Py_INCREF(wrapper);
        view->obj = wrapper;
        view->readonly = 1;
        view->buf = wrapper->m_impl->m_storage->data();
        view->ndim = 1;
        view->len = wrapper->m_impl->m_storage->size();
        view->shape = &wrapper->m_impl->m_shape;
        view->itemsize = sizeof(T);
        view->strides = &wrapper->m_impl->m_strides;
        view->suboffsets = nullptr;

        if (flags & PyBUF_FORMAT)
            view->format = wrapper->m_impl->m_format.data();
        else
            view->format = nullptr;

        return 0;
    }


    template <typename T>
    void PyBufferStorageWrapperImpl<T>::bf_releasebuffer(Py_buffer* view)
    {
        Py_DECREF(view->obj);
    }
}

namespace pybuffer_container
{
    using namespace pybuffer_container_detail;
    template <typename T>
    PyBufferViewWrapper * PyBufferViewWrapper<T>::create_py_view_wrapper(const pybuffer_container::container_view<T>& view)
    {
        PyBufferViewWrapper * view_wrapper = new PyBufferViewWrapper();
        view_wrapper->m_impl = new PyBufferViewWrapperImpl<T>(view);
        PyObject_Init(static_cast<PyObject*>(view_wrapper), pybuffer_view_type<T>());
        return view_wrapper;
    }


    template <typename T>
    PyBufferStorageWrapper * create_py_storage_wrapper(const PyBufferViewWrapper * view_wrapper, Py_ssize_t index)
    {
        PyBufferStorageWrapper * wrapper = new PyBufferStorageWrapper();
        wrapper->m_impl = new PyBufferStorageWrapperImpl(view_wrapper->m_impl->m_storage_elements[index]);
        PyObject_Init(wrapper, pybuffer_storage_type<T>());
        return wrapper;
    }
}