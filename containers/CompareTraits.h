#ifndef __COMPARE_TRAITS_H__
#define __COMPARE_TRAITS_H__

template <typename T, typename CompareT> struct CompareTraits {
    using value_type        = T;
    using Compare           = CompareT;
};

template <typename T> struct AscendingTraits : public CompareTraits<T, std::less<T>> {};
template <typename T> struct DescendingTraits : public CompareTraits<T, std::greater<T>> {};

#endif // __TRAVERSING_TRAITS_H__