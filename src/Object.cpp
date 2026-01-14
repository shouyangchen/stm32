//
// Created by chenshouyang on 2025/12/19.
//

#include "Object.h"
#include "Applications.h"

Object::Object(Object* parent, ObjectType type)
    : m_objectType(type)
    , m_parent(nullptr)
    , m_markedForDeletion(false) {
    m_objectName[0] = '\0';

    if (parent) {
        setParent(parent);
    }
}

Object::~Object() {
    removeFromParent();
    deleteChildren();
}

Object::Object(Object&& other) noexcept
    : m_objectType(other.m_objectType)
    , m_parent(other.m_parent)
    , m_children(std::move(other.m_children))
    , m_markedForDeletion(other. m_markedForDeletion) {

    std::memcpy(m_objectName, other.m_objectName, sizeof(m_objectName));

    for (auto* child : m_children) {
        if (child) {
            child->m_parent = this;
        }
    }

    if (m_parent) {
        auto& siblings = m_parent->m_children;
        auto it = std::find(siblings. begin(), siblings.end(), &other);
        if (it != siblings.end()) {
            *it = this;
        }
    }

    other.m_parent = nullptr;
    other.m_children.clear();
    other.m_objectName[0] = '\0';
}

Object& Object::operator=(Object&& other) noexcept {
    if (this != &other) {
        removeFromParent();
        deleteChildren();

        m_objectType = other.m_objectType;
        m_parent = other.m_parent;
        m_children = std::move(other.m_children);
        m_markedForDeletion = other.m_markedForDeletion;
        std::memcpy(m_objectName, other.m_objectName, sizeof(m_objectName));

        for (auto* child : m_children) {
            if (child) {
                child->m_parent = this;
            }
        }

        if (m_parent) {
            auto& siblings = m_parent->m_children;
            auto it = std::find(siblings.begin(), siblings.end(), &other);
            if (it != siblings.end()) {
                *it = this;
            }
        }

        other.m_parent = nullptr;
        other.m_children.clear();
        other.m_objectName[0] = '\0';
    }
    return *this;
}

void Object::setParent(Object* parent) {
    if (m_parent == parent) {
        return;
    }

    Object* oldParent = m_parent;

    if (m_parent) {
        m_parent->removeChild(this);
    }

    m_parent = parent;

    if (m_parent) {
        m_parent->addChild(this);
    }

    parentChanged(oldParent, parent);
}

void Object::addChild(Object* child) {
    if (!child) {
        return;
    }

    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        return;
    }

    m_children.push_back(child);

    if (child->m_parent && child->m_parent != this) {
        child->m_parent->removeChild(child);
    }

    child->m_parent = this;
    childEvent(child, true);
}

void Object::removeChild(Object* child) {
    if (!child) {
        return;
    }

    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        m_children.erase(it);

        if (child->m_parent == this) {
            child->m_parent = nullptr;
        }

        childEvent(child, false);
    }
}

Object* Object::findChild(ObjectType type, const char* name) const {
    for (auto* child : m_children) {
        if (child->m_objectType == type) {
            if (name == nullptr || strcmp(child->m_objectName, name) == 0) {
                return child;
            }
        }

        // 递归查找
        if (auto* found = child->findChild(type, name)) {
            return found;
        }
    }

    return nullptr;
}

Object* Object::findChild(const char* name) const {
    if (!name) {
        return nullptr;
    }

    for (auto* child : m_children) {
        if (strcmp(child->m_objectName, name) == 0) {
            return child;
        }

        if (auto* found = child->findChild(name)) {
            return found;
        }
    }

    return nullptr;
}

std::vector<Object*, ::allocator<Object*>> Object::findChildren(ObjectType type) const {
    std::vector<Object*, :: allocator<Object*>> result;

    for (auto* child : m_children) {
        if (child->m_objectType == type) {
            result.push_back(child);
        }

        auto childResults = child->findChildren(type);
        result.insert(result.end(), childResults.begin(), childResults.end());
    }

    return result;
}

void Object::dumpObjectTree(int indent) const {
    // 这里需要实现你的打印函数
    // 可以通过串口输出
    for (int i = 0; i < indent; ++i) {
        // 输出缩进
    }

    // 输出对象信息
    // printf("%s [%d] (%p)\n", m_objectName[0] ? m_objectName :  "unnamed",
    //        static_cast<int>(m_objectType), this);

    for (const auto* child : m_children) {
        if (child) {
            child->dumpObjectTree(indent + 1);
        }
    }
}

void Object::event(Event* e) {
    // 默认实现
}

void Object::setObjectName(const char* name) {
    if (name) {
        strncpy(m_objectName, name, sizeof(m_objectName) - 1);
        m_objectName[sizeof(m_objectName) - 1] = '\0';
    } else {
        m_objectName[0] = '\0';
    }
}

void Object::deleteLater() {
    if (m_markedForDeletion) {
        return;
    }

    m_markedForDeletion = true;

    auto* event = new Event(Event::EventType_DeleteObject, this);
    Applications::postEvent(event);
}

void Object::childEvent(Object* child, bool added) {
    // 默认实现
}

void Object::parentChanged(Object* oldParent, Object* newParent) {
    // 默认实现
}

void Object::removeFromParent() {
    if (m_parent) {
        m_parent->removeChild(this);
    }
}

void Object::deleteChildren() {
    auto childrenCopy = m_children;
    m_children.clear();

    for (auto it = childrenCopy.rbegin(); it != childrenCopy.rend(); ++it) {
        Object* child = *it;
        if (child) {
            child->m_parent = nullptr;
            delete child;
        }
    }
}