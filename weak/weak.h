#pragma once

#include "sw_fwd.h"  // Forward declaration
#include "shared.h"
// https://en.cppreference.com/w/cpp/memory/weak_ptr
template <typename T>
class WeakPtr {
    template <typename Y>
    friend class SharedPtr;

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    WeakPtr() : ptr_(nullptr), control_block_(nullptr) {
    }

    WeakPtr(const WeakPtr& other) {
        if (this != &other) {
            control_block_ = other.control_block_;
            ptr_ = other.ptr_;
            if (control_block_) {
                control_block_->AddWeakRef();
            }
        }
    }

    WeakPtr(WeakPtr&& other) {
        control_block_ = other.control_block_;
        ptr_ = other.ptr_;
        other.control_block_ = nullptr;
        other.ptr_ = nullptr;
    }

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    WeakPtr(const SharedPtr<T>& other) {
        control_block_ = other.control_block_;
        ptr_ = other.ptr_;
        control_block_->AddWeakRef();
    }

    template <class Y>
    WeakPtr(const SharedPtr<Y>& other) {
        control_block_ = other.control_block_;
        ptr_ = other.ptr_;
        control_block_->AddWeakRef();
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    WeakPtr& operator=(const WeakPtr& other) {
        if (this == &other) {
            return *this;
        }
        TryDeleteBlock();
        control_block_ = other.control_block_;
        ptr_ = other.ptr_;
        if (control_block_) {
            control_block_->AddWeakRef();
        }
        return *this;
    }

    WeakPtr& operator=(WeakPtr&& other) {
        if (this == &other) {
            return *this;
        }
        TryDeleteBlock();
        control_block_ = other.control_block_;
        ptr_ = other.ptr_;
        other.control_block_ = nullptr;
        other.ptr_ = nullptr;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~WeakPtr() {
        TryDeleteBlock();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        TryDeleteBlock();
        control_block_ = nullptr;
        ptr_ = nullptr;
    }

    void Swap(WeakPtr& other) {
        std::swap(control_block_, other.control_block_);
        std::swap(ptr_, other.ptr_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    size_t UseCount() const {
        if (!control_block_) {
            return 0;
        }
        return control_block_->GetRefCounter();
    }

    bool Expired() const {
        if (!control_block_) {
            return true;
        }
        return control_block_->GetRefCounter() == 0;
    }

    SharedPtr<T> Lock() const {
        if (control_block_ == nullptr || Expired()) {
            return SharedPtr<T>();
        }
        return SharedPtr<T>(*this);
    }

private:
    void TryDeleteBlock() {
        if (control_block_) {
            if (control_block_->GetRefCounter() == 0 && control_block_->GetWeakRefCounter() == 1) {
                delete control_block_;
                return;
            } else {
                control_block_->DecWeakRef();
            }
        }
    }

    T* ptr_;
    ControlBlockBase* control_block_;
};
