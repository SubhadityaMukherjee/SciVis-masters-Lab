#ifndef FFTWF_MALLOC_ALLOCATOR_H
#define FFTWF_MALLOC_ALLOCATOR_H

#include <fftw3.h>

#include <cstdlib>
#include <new>

template <typename T>
struct fftwf_malloc_allocator
{
    typedef T value_type;

    fftwf_malloc_allocator() = default;

    template <class U> constexpr fftwf_malloc_allocator(const fftwf_malloc_allocator<U>&) noexcept {}

    [[nodiscard]] T* allocate(std::size_t n)
    {
        if (n > std::size_t(-1) / sizeof(T))
            throw std::bad_alloc();
        if (auto p = static_cast<T*>(fftwf_malloc(n * sizeof(T))))
            return p;
        throw std::bad_alloc();
    }

    void deallocate(T* p, std::size_t) noexcept
    {
        fftwf_free(p);
    }
};

template <class T, class U>
bool operator==(const fftwf_malloc_allocator<T>&, const fftwf_malloc_allocator<U>&)
{
    return true;
}

template <class T, class U>
bool operator!=(const fftwf_malloc_allocator<T>&, const fftwf_malloc_allocator<U>&)
{
    return false;
}

#endif // FFTWF_MALLOC_ALLOCATOR_H
