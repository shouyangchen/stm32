//
// Created by chenshouyang on 2025/12/21.
//

#ifndef F103C8_ALLOCATOR_H
#define F103C8_ALLOCATOR_H
#include <limits>
#include <vector>

#include "new/heap.h"
#include "new/new.h"
//在stm32上的STL分配器

template<typename Type>
class allocator {
    public:
    typedef Type value_type;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef Type& reference;
    typedef const Type& const_reference;
    typedef Type* pointer;
    typedef const Type* const_pointer;

    template<typename Tp1>
    struct rebind {
        typedef allocator<Tp1> other;
    };

    allocator()noexcept(true)= default;
    allocator(const allocator&)noexcept(true)= default;
    template<typename Tp1>
    allocator(const allocator<Tp1>&)noexcept(true){}
    ~allocator()noexcept(true)= default;
    pointer address(reference x)const;
    const_pointer address(const_reference x)const;
    pointer allocate(size_type n, const void* hint = 0);
    void deallocate(pointer p, size_type n);
    void construct(pointer p,const Type& t);
    void destroy(pointer p);
    [[nodiscard]] size_type max_size() const noexcept(true);
};

template<typename Type>
typename allocator<Type>::pointer allocator<Type>::allocate(size_type n,const void* hint) {
    if (n>this->max_size())
        return nullptr;
    return static_cast<Type *>(::operator new(n*sizeof(Type)));
}

template<typename Type>
typename allocator<Type>::const_pointer allocator<Type>::address(const_reference x) const {
    return &x;
}

template<typename Type>
typename allocator<Type>::pointer allocator<Type>::address(reference x) const {
    return &x;
}

template<typename Type>
void allocator<Type>::deallocate(pointer p, size_type n) {
  ::operator delete (p);
}

template<typename Type>
void allocator<Type>::construct(pointer p, const Type& t) {
   new (static_cast<void *>(p)) Type(t);
}

template<typename Type>
void allocator<Type>::destroy(pointer p) {
    static_cast<Type *>(p)->~Type();
}

template<typename Type>
typename allocator<Type>::size_type allocator<Type>::max_size() const noexcept(true) {
    return MAXSIZEOFHEAP/sizeof(Type);
}

template<typename Tp1,typename Tp2>
inline bool operator == (const allocator<Tp1>&,const allocator<Tp2>&) noexcept(true){return true;}

template<typename Tp1,typename Tp2>
inline bool operator !=(const allocator<Tp1>&,const allocator<Tp2>&) noexcept(true){return false;}




#ifdef MSVC_ARM_COMPILER
#endif



#endif //F103C8_ALLOCATOR_H