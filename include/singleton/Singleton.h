//
// Created by chenshouyang on 2026/1/2.
//

#ifndef SPLPROJECT_SINGLETON_H
#define SPLPROJECT_SINGLETON_H
#include <utility>
template<typename T>
class Singleton {
private:
    static T *instance;
protected:
    Singleton() = default;
public:
    Singleton(const Singleton &) = delete;
    Singleton &operator=(const Singleton &) = delete;
    Singleton(Singleton &&) = delete;
    template<typename... Args>
    static T* getInstance(Args...args) {
        if (instance==nullptr) {
                instance=new T(std::forward<Args>(args)...);
                return instance;
            }
        else
            return instance;
        }
};


template<typename T>
T* Singleton<T>::instance = nullptr;

#endif //SPLPROJECT_SINGLETON_H