#include <metal.hpp>
#include <boost/pfr.hpp>
#include <iostream>
#include <utility>
#include <tuple>
#include <string>

// Just some quick getting started finger exercises for metal/pfr
// Will remove later on

struct pod_type
{
    // plain pod type with lots of padding internally to test reflection on
    int i1;
    double d1;
    char c1;
    size_t sz1[4];
    void * p1;
    float f1;
    short s1;
    int * ip2[2];
};


// metal mapping from type to python struct character code
// this is just a partial mapping for now. Will need to be extended and will have to
// handle array types also which will require some sort of transform
template <typename T>
using map_to_struct_code = metal::at_key<
metal::map<
    metal::pair<int, metal::number<'i'>>,
    metal::pair<double, metal::number<'d'>>,
    metal::pair<char, metal::number<'c'>>,
    metal::pair<size_t, metal::number<'Q'>>,
    metal::pair<float, metal::number<'f'>>,
    metal::pair<short, metal::number<'h'>>
>, T>;


// To handle array types, initially map out the tuples from pfr into
// scalar and array elements based on the metal mapping
template <typename T>
struct tuple_element
{
    using value = map_to_struct_code<T>;
    static constexpr bool array = false;
};


template <typename T>
struct tuple_element<T*>
{

    using value = metal::number<'p'>;
    static constexpr bool array = false;
};

// Just supports single dim for now. Todo: Generalize to more dimensions
template <typename T, size_t N>
struct tuple_element<T[N]>
{
    using value = map_to_struct_code<T>;
    static constexpr bool array = true;
    static constexpr size_t length = N;
};


template <typename T, size_t N>
struct tuple_element<T*[N]>
{
    using value = metal::number<'p'>;
    static constexpr bool array = true;
    static constexpr size_t length = N;
};


template <typename ...Args>
struct extract_tuple_element_impl
{
    using type = metal::transform<metal::lambda<tuple_element>, metal::list<Args...>>;
};


template <template <typename ...> class T, typename ... Args>
auto extract_tuple_elements(T<Args...> prototype)
{
    return extract_tuple_element_impl<Args...>();
}


template <typename T, bool = T::array>
struct extract_value_impl;


template <typename T>
struct extract_value_impl<T, false>
{
    static std::string extract()
    {
        std::string s;
        s += T::value::value;
        return s;
    }
};



template <typename T>
struct extract_value_impl<T, true>
{
    static std::string extract()
    {
        std::string s;
        using std::to_string;
        s += to_string(T::length);
        s += T::value;
        return s;
    }
};


template <typename T>
std::string extract_value()
{
    return extract_value_impl<T>::extract();
}


template <typename ...Args>
std::string make_string(metal::list<Args...> metal_list)
{
    return (extract_value<Args>() + ...);
}


int main()
{
    auto s1 = boost::pfr::flat_structure_to_tuple(pod_type());
    using m1 = decltype(extract_tuple_elements(s1))::type;
    std::cout << make_string(m1()) << std::endl;
    return 0;
}

