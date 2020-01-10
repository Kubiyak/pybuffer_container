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
auto realize_tuple(metal::list<Args...> metal_list)
{
    return std::make_tuple(extract_value<Args>()...);
}


template<typename T, T...>
struct integer_sequence { };


template<std::size_t N, std::size_t... I>
struct gen_indices : gen_indices<(N - 1), (N - 1), I...> { };
template<std::size_t... I>
struct gen_indices<0, I...> : integer_sequence<std::size_t, I...> { };



std::string& to_string_impl(std::string& s)
{
  return s;
}


template <typename... T>
std::string& to_string_impl(std::string& s, std::string c, T&&... t)
{
  s += c;
  return to_string_impl(s, std::forward<T>(t)...);
}

template<typename... T, std::size_t... I>
std::string to_string(const std::tuple<T...>& tup, integer_sequence<std::size_t, I...>)
{
  std::string result;
  int ctx[] = { (to_string_impl(result, std::get<I>(tup)...), 0), 0 };
  (void)ctx;
  return result;
}

template<typename... T>
std::string to_string(const std::tuple<T...>& tup)
{
  return to_string(tup, gen_indices<sizeof...(T)>{});
}


int main()
{
    auto s1 = boost::pfr::flat_structure_to_tuple(pod_type());
    using m1 = decltype(extract_tuple_elements(s1))::type;

    // Upto here, the elements of interest are encoded in the type signature of
    // a metal::list. This step now makes a real tuple out of that signature
    auto result = realize_tuple(m1());

    // Now the last bit is to turn that tuple into a string
    // https://stackoverflow.com/questions/23436406/converting-tuple-to-string
    std::cout << to_string(result) << std::endl;
    return 0;
}

