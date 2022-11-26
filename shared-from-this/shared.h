#pragma once

#include "sw_fwd.h"  // Forward declaration
#include "weak.h"
#include <cstddef>  // std::nullptr_t
#include <type_traits>
#include <iostream>

class EFSTBase {};

// https://en.cppreference.com/w/cpp/memory/shared_ptr
template <typename T>
class SharedPtr {
    template <typename Y>
    friend class SharedPtr;

    template <typename Y>
    friend class WeakPtr;

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    SharedPtr() {
        control_block_ = nullptr;
    };

    SharedPtr(std::nullptr_t) : control_block_(nullptr) {
    }

    explicit SharedPtr(T* ptr) : control_block_(new ControlBlockPointer<T>(ptr)) {
        ptr_ = ptr;
        if constexpr (std::is_convertible_v<T*, EFSTBase*>) {
            ptr->weak_this_ = *this;
        }
    }

    template <class Y>
    SharedPtr(Y* ptr) : control_block_(new ControlBlockPointer<Y>(ptr)) {
        ptr_ = ptr;
        if constexpr (std::is_convertible_v<Y*, EFSTBase*>) {
            ptr->weak_this_ = *this;
        }
    }

    SharedPtr(const SharedPtr<T>& other) : control_block_(other.control_block_), ptr_(other.ptr_) {
        if (control_block_) {
            control_block_->AddReference();
        }
        if constexpr (std::is_convertible_v<T*, EFSTBase*>) {
            other.ptr_->weak_this_ = *this;
        }
    }

    template <class Y>
    SharedPtr(const SharedPtr<Y>& other) : control_block_(other.control_block_), ptr_(other.ptr_) {
        if (control_block_) {
            control_block_->AddReference();
        }
        if constexpr (std::is_convertible_v<Y*, EFSTBase*>) {
            other.ptr_->weak_this_ = *this;
        }
    }

    template <class Y>
    SharedPtr(SharedPtr<Y>&& other) {
        control_block_ = other.control_block_;
        ptr_ = other.ptr_;
        if constexpr (std::is_convertible_v<Y*, EFSTBase*>) {
            other.ptr_->weak_this_ = *this;
        }
        other.control_block_ = nullptr;
        other.ptr_ = nullptr;
    }

    SharedPtr(SharedPtr&& other) {
        control_block_ = other.control_block_;
        ptr_ = other.ptr_;
        if constexpr (std::is_convertible_v<T*, EFSTBase*>) {
            other.ptr_->weak_this_ = *this;
        }
        other.control_block_ = nullptr;
        other.ptr_ = nullptr;
    }

    template <class Y>
    SharedPtr(ControlBlockHolder<Y>* block) : control_block_(block) {
        ptr_ = block->GetPointer();
        if constexpr (std::is_convertible_v<T*, EFSTBase*>) {
            ptr_->weak_this_ = *this;
        }
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <class X>
    SharedPtr(const SharedPtr<X>& other, T* ptr) : control_block_(other.control_block_), ptr_(ptr) {
        if (control_block_) {
            control_block_->AddReference();
        }
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other) {
        if (other.control_block_->GetRefCounter() == 0) {
            throw BadWeakPtr();
        }
        control_block_ = other.control_block_;
        ptr_ = other.ptr_;
        control_block_->AddReference();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (this == &other) {
            return *this;
        }
        TryToDeleteBlock();
        control_block_ = other.control_block_;
        ptr_ = other.ptr_;
        if (control_block_) {
            control_block_->AddReference();
        }
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) {
        if (this == &other) {
            return *this;
        }
        TryToDeleteBlock();
        control_block_ = other.control_block_;
        ptr_ = other.ptr_;
        other.control_block_ = nullptr;
        other.ptr_ = nullptr;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        TryToDeleteBlock();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        TryToDeleteBlock();
        control_block_ = nullptr;
        ptr_ = nullptr;
    }

    void Reset(T* ptr) {
        TryToDeleteBlock();
        control_block_ = new ControlBlockPointer<T>(ptr);
        ptr_ = ptr;
    }

    template <class Y>
    void Reset(Y* ptr) {
        TryToDeleteBlock();
        control_block_ = new ControlBlockPointer<Y>(ptr);
        ptr_ = ptr;
    }

    void Swap(SharedPtr& other) {
        std::swap(control_block_, other.control_block_);
        std::swap(ptr_, other.ptr_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return ptr_;
    }

    T& operator*() const {
        return *ptr_;
    }

    T* operator->() const {
        return ptr_;
    }
    size_t UseCount() const {
        if (control_block_) {
            return control_block_->GetRefCounter();
        }
        return 0;
    }
    explicit operator bool() const {
        return control_block_ != nullptr;
    }

private:
    void TryToDeleteBlock() {
        if (control_block_) {
            if (control_block_->GetWeakRefCounter() == 0 && control_block_->GetRefCounter() == 1) {
                control_block_->DeleteT();
                delete control_block_;
                return;
            }
            if (control_block_->GetWeakRefCounter() > 0 && control_block_->GetRefCounter() == 1) {
                control_block_->RemoveReference();
                control_block_->DeleteT();
                return;
            }
            control_block_->RemoveReference();
        }
    }

    T* ptr_ = nullptr;
    ControlBlockBase* control_block_;
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    return left.Get() == right.Get();
}

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    return SharedPtr<T>(new ControlBlockHolder<T>(std::forward<Args>(args)...));
}

// Look for usage examples in tests and seminar
template <typename T>
class EnableSharedFromThis : public EFSTBase {
public:
    SharedPtr<T> SharedFromThis() {
        return SharedPtr<T>(weak_this_);
    }

    SharedPtr<const T> SharedFromThis() const {
        return SharedPtr<const T>(weak_this_);
    }

    WeakPtr<T> WeakFromThis() noexcept {
        return weak_this_;
    }

    WeakPtr<const T> WeakFromThis() const noexcept {
        return WeakPtr<const T>(weak_this_);
    }

public:
    WeakPtr<T> weak_this_;
};