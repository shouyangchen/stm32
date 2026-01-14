//
// Created by chenshouyang on 2025/12/19.
//

#ifndef F103C8_OBJECT_H
#define F103C8_OBJECT_H

#include <vector>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include "EventQueue/Event.h"
#include "Allocator/allocator.h"

// 对象类型 ID 枚举
enum class ObjectType : uint16_t {
    Base = 0,
    SerialPort,
    SerialPortManager,
    Observer,
    SerialPortObserver,
    GPSObserver,
    BluetoothObserver,
    ASRObserver,
    AccelObserver,
    UserTypeStart = 1000  // 用户自定义类型从这里开始
};

class Object {
public:
    explicit Object(Object* parent = nullptr, ObjectType type = ObjectType::Base);
    virtual ~Object();

    // 禁用拷贝，允许移动
    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;
    Object(Object&& other) noexcept;
    Object& operator=(Object&& other) noexcept;

    // 类型识别（替代 dynamic_cast）
    [[nodiscard]] ObjectType getObjectType() const { return m_objectType; }
    [[nodiscard]] bool isType(ObjectType type) const { return m_objectType == type; }

    // 类型安全的转换（替代 dynamic_cast）
    template<typename T>
    T* as();

    template<typename T>
    const T* as() const;

    template<typename T>
    [[nodiscard]] bool is() const;

    // 父子关系管理
    void setParent(Object* parent);
    [[nodiscard]] Object* getParent() const { return m_parent; }

    // 子对象管理
    void addChild(Object* child);
    void removeChild(Object* child);
    [[nodiscard]] const std::vector<Object*, ::  allocator<Object*>>& getChildren() const { return m_children; }
    [[nodiscard]] size_t getChildCount() const { return m_children.size(); }

    // 查找子对象
    Object* findChild(ObjectType type, const char* name = nullptr) const;
    Object* findChild(const char* name) const;

    [[nodiscard]] std::vector<Object*, :: allocator<Object*>> findChildren(ObjectType type) const;

    // 对象树遍历
    void dumpObjectTree(int indent = 0) const;

    // 事件处理
    virtual void event(Event* e);

    // 对象名称（用于调试）
    void setObjectName(const char* name);
    [[nodiscard]] const char* getObjectName() const { return m_objectName; }

    // 删除相关
    void deleteLater();
    [[nodiscard]] bool isMarkedForDeletion() const { return m_markedForDeletion; }

protected:
    virtual void childEvent(Object* child, bool added);
    virtual void parentChanged(Object* oldParent, Object* newParent);

private:
    void removeFromParent();
    void deleteChildren();

    ObjectType m_objectType;
    Object* m_parent;
    std::vector<Object*, ::allocator<Object*>> m_children;
    char m_objectName[32];
    bool m_markedForDeletion;
};

// ============ 类型转换辅助宏 ============

// 定义每个类的类型 ID
#define OBJECT_TYPE(TypeEnum) \
    static constexpr ObjectType staticObjectType() { return TypeEnum; } \
    ObjectType getObjectType() const override { return TypeEnum; }


template<typename T>
T* Object::  as() {
    // 检查类型是否匹配
    if (m_objectType == T::staticObjectType()) {
        return static_cast<T*>(this);
    }
    return nullptr;
}

template<typename T>
const T* Object::as() const {
    if (m_objectType == T:: staticObjectType()) {
        return static_cast<const T*>(this);
    }
    return nullptr;
}

template<typename T>
bool Object::is() const {
    return m_objectType == T::staticObjectType();
}



#endif //F103C8_OBJECT_H