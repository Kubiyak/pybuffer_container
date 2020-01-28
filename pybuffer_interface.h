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
#include <Python.h>
#include <metal.hpp> // https://github.com/brunocodutra/metal
#include <boost/pfr.hpp> // https://github.com/apolukhin/magic_get
#include "pybuffer_container.h"
#include <vector>


namespace pybuffer_container_detail
{
    // https://docs.python.org/3.8/library/struct.html
    template <typename T>
    using struct_code_map = metal::at_key<
    metal::map<
        metal::pair<char, metal::number<'c'>>,
        metal::pair<std::uint8_t, metal::number<'B'>>,
        metal::pair<bool,
                    std::conditional< sizeof(bool) == 4, metal::number<'I'>,
                    std::conditional< sizeof(bool) == 2, metal::number<'H'>, metal::number<'B'>>::type>::type>,
        metal::pair<short, metal::number<'h'>>,
        metal::pair<std::uint16_t, metal::number<'H'>>,
        metal::pair<int, metal::number<'i'>>,
        metal::pair<unsigned int, metal::number<'I'>>,
        metal::pair<size_t, metal::number<'Q'>>,
        metal::pair<ssize_t, metal::number<'q'>>,
        metal::pair<float, metal::number<'f'>>,
        metal::pair<double, metal::number<'d'>>
    >, T>;


    template <typename T>
    using size_map = metal::at_key<
    metal::map<
            metal::pair<metal::number<'c'>, metal::number<1>>,
            metal::pair<metal::number<'B'>, metal::number<1>>,
            metal::pair<metal::number<'h'>, metal::number<2>>,
            metal::pair<metal::number<'H'>, metal::number<2>>,
            metal::pair<metal::number<'i'>, metal::number<4>>,
            metal::pair<metal::number<'I'>, metal::number<4>>,
            metal::pair<metal::number<'q'>, metal::number<8>>,
            metal::pair<metal::number<'Q'>, metal::number<8>>,
            metal::pair<metal::number<'f'>, metal::number<4>>,
            metal::pair<metal::number<'d'>, metal::number<8>>,
            metal::pair<metal::number<'P'>, metal::number<8>>>, T>;


    template <typename T, size_t count_in, size_t max_size_in>
    struct py_struct_element
    {
        using type = T;
        using value = struct_code_map<T>;
        static constexpr size_t count = count_in;
        static constexpr size_t value_size = size_map<value>::value;
        static constexpr size_t max_size = std::max(max_size_in, value_size);
    };


    template <typename T, size_t count_in, size_t max_size_in>
    struct py_struct_element<T*, count_in, max_size_in>
    {
        using type = T*;
        using value = metal::number<'P'>;
        static constexpr size_t count = count_in;
        static constexpr size_t max_size = 8;
    };


    template <typename Args>
    struct extract_py_struct_element_impl;


    template <typename T, typename N, bool = std::is_same<T, typename N::type>::value>
    struct pystruct_combine_type;


    template <typename T, typename ...Args>
    struct extract_py_struct_element_impl<metal::list<T, Args...>> :public extract_py_struct_element_impl<metal::list<Args...>>
    {
        using list_t = typename extract_py_struct_element_impl<metal::list<Args...>>::result;
        using front = metal::front<list_t>;
        using rest = metal::range<list_t, metal::number<1>, metal::size<list_t>>;
        using combined_elem = typename pystruct_combine_type<T, front>::result;
        using result = metal::join<combined_elem, rest>;
    };


    template <typename T>
    struct extract_py_struct_element_impl<metal::list<T>>
    {
        using result = metal::list<py_struct_element<T, 1, 0>>;
    };


    template <>
    struct extract_py_struct_element_impl< metal::list<>>
    {
        using result = metal::list<>;
    };


    template <typename T, typename N>
    struct pystruct_combine_type<T, N, true>
    {
        using result = metal::list<py_struct_element<T, N::count + 1, 0>>;
    };


    template <typename T, typename N>
    struct pystruct_combine_type<T, N, false>
    {
        using result = metal::list<py_struct_element<T, 1, N::max_size>, N>;
    };


    template <template <typename ...> class T, typename ... Args>
    auto extract_py_struct_elements(T<Args...> prototype)
    {
        return typename extract_py_struct_element_impl<metal::list<Args...>>::result();
    }


    template <typename T>
    std::string extract_value()
    {
        size_t n = T::count;
        std::string s;
        if (T::count == 1)
        {
            s += T::value::value;
        }
        else
        {
            s += std::to_string(T::count);
            s += T::value::value;
        }
        return s;
    }


    template <typename ...Args>
    std::string make_pystruct_code(metal::list<Args...> metal_list)
    {
        std::string result;
        (result += ... += extract_value<Args>());
        if (result.size())
        {
            size_t alignment = metal::front<metal::list<Args...>>::max_size;
            if (alignment > 1)
            {
                result += (alignment == 2 ? "0h" : (alignment == 4 ? "0i" : "0P"));
            }
        }
        return result;
    }


    template <typename StructType>
    std::string _get_py_struct_code_impl()
    {
        static_assert(std::is_trivially_copyable<StructType>::value, "StructType must be trivially copyable");

        // Use pfr to obtain a tuple containing the types of all the elements in the pod struct
        auto reflection_type_info = boost::pfr::flat_structure_to_tuple(StructType());
        using python_type_info = decltype(extract_py_struct_elements(reflection_type_info));
        return make_pystruct_code(python_type_info());
    }


    template <typename StructType>
    std::string get_py_struct_code()
    {
        static std::string result = _get_py_struct_code_impl<T>();
        return result;
    }


    template <typename T>
    struct PyBufferViewWrapperImpl
    {
        static void tp_dealloc(PyObject * obj);
        static PyObject * tp_str(PyObject * obj);
        // This length is the number of slices in the view
        static Py_ssize_t sq_length(PyObject * obj);
        // This returns the slice wrapper at the specified index
        static PyObject * sq_item(PyObject * obj, Py_ssize_t index);
        pybuffer_container::container_view<T> m_view;
        std::vector<pybuffer_container::container_view::shared_storage_t> m_storage_elements;

        PyBufferViewWrapperImpl(const pybuffer_container::container_view<T>& view):
            m_view(view),
            m_storage_elements(view->get_storage_elements())
        {
        }
    };


    template <typename T>
    struct PyBufferStorageWrapperImpl
    {
        static void tp_dealloc(PyObject * object);
        static PyObject * tp_str(PyObject * object);
        static int bf_getbuffer(PyObject * exporter, Py_buffer * view, int flags);
        static void bf_releasebuffer(Py_buffer * view);

        pybuffer_container::container_view::shared_storage_t m_storage; // shared ptr
        std::string m_format; // py struct code format
        Py_ssize_t m_shape; // m_storage->size. buffer protocol views need this
        Py_ssize_t m_strides; // sizeof(T)

        PyBufferStorageWrapperImpl(const pybuffer_container::container_view::shared_storage_t& storage):
            m_storage(storage),
            m_format(get_py_struct_code<T>())
        {
            m_shape = m_storage->size();
            m_strides = sizeof(T);
        }
    };
}


namespace pybuffer_container
{

    // Python interface for a pybuffer_container. This type is manipulated inside Python and must be a pod type.
    template <typename T>
    struct PyBufferViewWrapper
    {
       PyObject_HEAD // PyObject ob_base;
       PyBufferViewWrapperImpl<T> * m_impl;
       // Must be called after python has been initialized.
       static PyBufferViewWrapper * create_py_view_wrapper(const pybuffer_container::container_view<T>& view);
    };


    template <typename T>
    struct PyBufferStorageWrapper
    {
        PyObject_HEAD
        PyBufferStorageWrapperImpl<T> * m_impl;
        // Note that PyBufferStorageWrapper owns a reference on view_wrapper.
        static int bf_getbuffer(PyObject* exporter, Py_buffer* view, int flags);
        static PyBufferStorageWrapper * create_py_storage_wrapper(const PyBufferViewWrapper * view_wrapper, Py_ssize_t index);
    };


    template <typename T>
    PyTypeObject * pybuffer_view_type()
    {
        using namespace pybuffer_container_detail;
        static std::string tp_name = "pybuffer_interface.PyBufferViewWrapper_" +
        get_py_struct_code<T>();

        static std::string doc_string = "Python wrapper for pybuffer_container::container_view with struct signature " +
        get_py_struct_code<T>();

        static PySequenceMethods sequence_methods = {
            &PyBufferViewWrapperImpl<T>::sq_length,
            0, /* concat not supported. TODO: Investigate feasibility */
            0, /* repeat not supported */
            &PyBufferViewWrapperImpl<T>::sq_item,
            0, /* formerly sq_slice. */
            0, /* assign not allowed */
            0, /* was assign slice. Not supported */
            0, /* sq_contains not supported */
            0, /* in place concat */
            0 /* in place repeat */
        };

       static PyTypeObject tp_object = {
            PyVarObject_HEAD_INIT(nullptr, 0)
            tp_name.c_str(),
            sizeof(PyBufferViewWrapper), /* tp_basicsize */
            0, /* tp_itemsize */
            &PyBufferViewWrapperImpl<T>::tp_dealloc,
            0, /* tp_vectorcall_offset: TODOL investigate */
            0, /* tp_getattr deprecated */
            0, /* tp_setattr deprecated */
            0, /* tp_as_async: TODO: Investigate */
            0, /* tp_repr */
            0,
            &sequence_methods, /* tp_as_sequence */
            0, /* tp_as_mapping */
            0, /* tp_hash */
            0, /* tp_call */
            &PyBufferViewWrapperImpl<T>::tp_str,
            0, /* tp_getattro */
            0, /* tp_setattro */
            0, /* tp_as_buffer TODO: Set this */
            0, /* tp_flags TODO: Set correctly */
            doc_string.c_str(), /* tp_doc */
            0, /* tp_traverse (for objects setting Py_TPFLAGS_HAVE_GC) */
            0, /* tp_clear. This is related to tp_traverse */
            0, /* tp_richcompare */
            0, /* tp_weaklist_offset */
            0, /* tp_iter. TODO: Set */
            0, /* tp_iternext */
            0, /* tp_methods */
            0, /* tp_members */
            0, /* tp_getset */
            0, /* tp_base (base type for this type) */
            0, /* tp_dict. Set by PyType_Ready */
            0, /* tp_descr_get */
            0, /* tp_descr_set */
            0, /* tp_dict_offset */
            0, /* tp_init: TODO: Investigate object initialization */
            0, /* tp_alloc: Investigate allocation and initialization */
            0, /* tp_new */
            0, /* tp_free: TODO: Investigate need for this */
            0, /* tp_is_gc: Should return 1 for collectible instance and 0 for otherwise */
            0, /* tp_bases: Only applicable for types created in Python source files */
            0, /* tp_mro: method resolution order. Only for types defined in Py source files */
            0, /* tp_cache: Internal use only */
            0, /* tp_subclasses: Internal use only */
            0, /* tp_weaklist: Internal use only */
            0, /* tp_del: deprecated. Use tp_finalize */
            0, /* tp_version: Internal use only */
            0, /* tp_finalize: Called before dealloc. TODO: Investigate */
            // Some other fields are defined conditionally but are not important
            // for PyBufferContainerWrapper
        };

        // Note: Caller is responsible for calling PyType_Ready
        return &tp_object;
    }


    template <typename T>
    PyTypeObject * pybuffer_storage_type()
    {
        using namespace pybuffer_container_detail;
        static std::string tp_name = "pybuffer_interface.PyBufferStorageWrapper_" +
        get_py_struct_code<T>();

        static std::string doc_string = "Python wrapper for pybuffer_container::container_view::shared_storage_t with struct signature " +
        get_py_struct_code<T>();

        static PyBufferProcs buffer_protocol_methods = {
          &PyBufferStorageWrapperImpl<T>::bf_getbuffer,
          &PyBufferStorageWrapperImpl<T>::bf_releasebuffer
        };

       static PyTypeObject tp_object = {
            PyVarObject_HEAD_INIT(nullptr, 0)
            tp_name.c_str(),
            sizeof(PyBufferViewWrapper), /* tp_basicsize */
            0, /* tp_itemsize */
            &PyBufferStorageWrapperImpl<T>::tp_dealloc,
            0, /* tp_vectorcall_offset: TODOL investigate */
            0, /* tp_getattr deprecated */
            0, /* tp_setattr deprecated */
            0, /* tp_as_async: TODO: Investigate */
            0, /* tp_repr */
            0,
            0, /* tp_as_sequence */
            0, /* tp_as_mapping */
            0, /* tp_hash */
            0, /* tp_call */
            &PyBufferStorageWrapperImpl<T>::tp_str,
            0, /* tp_getattro */
            0, /* tp_setattro */
            &buffer_protocol_methods, /* buffer protocol */
            Py_TPFLAGS_DEFAULT, /* tp_flags */
            doc_string.c_str(), /* tp_doc */
            0, /* tp_traverse (for objects setting Py_TPFLAGS_HAVE_GC) */
            0, /* tp_clear. This is related to tp_traverse */
            0, /* tp_richcompare */
            0, /* tp_weaklist_offset */
            0, /* tp_iter.(This type implements the sequence protocol so iter implemented based on that) */
            0, /* tp_iternext */
            0, /* tp_methods */
            0, /* tp_members */
            0, /* tp_getset */
            0, /* tp_base (base type for this type) */
            0, /* tp_dict. Set by PyType_Ready */
            0, /* tp_descr_get */
            0, /* tp_descr_set */
            0, /* tp_dict_offset */
            0, /* tp_init: TODO: Investigate object initialization */
            0, /* tp_alloc: Investigate allocation and initialization */
            0, /* tp_new: for immutable types, initialization should go here... */
            0, /* tp_free: TODO: Investigate need for this */
            0, /* tp_is_gc: Should return 1 for collectible instance and 0 for otherwise */
            0, /* tp_bases: Only applicable for types created in Python source files */
            0, /* tp_mro: method resolution order. Only for types defined in Py source files */
            0, /* tp_cache: Internal use only */
            0, /* tp_subclasses: Internal use only */
            0, /* tp_weaklist: Internal use only */
            0, /* tp_del: deprecated. Use tp_finalize */
            0, /* tp_version: Internal use only */
            0, /* tp_finalize: Called before dealloc. TODO: Investigate */
            // Some other fields are defined conditionally but are not important
            // for PyBufferContainerWrapper
        };

       if (!PyType_GetFlags(&tp_object) & Py_TPFLAGS_READY)
           PyType_Ready(&tp_object);
       return &tp_object;
    }
}


#include "pybuffer_interface_impl.h"

