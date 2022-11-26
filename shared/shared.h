#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t
#include <iostream>

class ControlBlockBase {
public:
    void AddReference() {
        ref_counter_++;
    }

    void RemoveReference() {
        ref_counter_--;
    }

    size_t GetRefCounter() const {
        return ref_counter_;
    }
    virtual ~ControlBlockBase() = default;

    void AddWeakRef() {
        weak_ref_counter_++;
    }

    void DecWeakRef() {
        weak_ref_counter_--;
    }

    virtual void DeleteT() = 0;

    size_t GetWeakRefCounter() {
        return weak_ref_counter_;
    }

private:
    size_t weak_ref_counter_ = 0;
    size_t ref_counter_ = 0;
};

template <typename T>
class ControlBlockPointer : public ControlBlockBase {
public:
    ControlBlockPointer() = default;

    ControlBlockPointer(T* ptr) : ptr_(ptr) {
        if (ptr_) {
            ControlBlockBase::AddReference();
        }
    }

    void DeleteT() override {
        delete ptr_;
    }

    ~ControlBlockPointer() {
        //        std::cout << "Delete in blockptr" << '\n';
        delete ptr_;
    }

private:
    T* ptr_;
};

template <typename T>
class ControlBlockHolder : public ControlBlockBase {
    template <typename Y>
    friend class SharedPtr;

public:
    ControlBlockHolder() {
        AddReference();
    };

    template <class... Args>
    ControlBlockHolder(Args&&... args) {
        new (&storage_) T(std::forward<Args>(args)...);
        AddReference();
    }

    T* GetPointer() {
        return reinterpret_cast<T*>(&storage_);
    }

    void DeleteT() override {
        GetPointer()->~T();
    }

    ~ControlBlockHolder() {
        GetPointer()->~T();
    }

private:
    std::aligned_storage_t<sizeof(T), alignof(T)> storage_;
};

// https://en.cppreference.com/w/cpp/memory/shared_ptr
template <typename T>
class SharedPtr {
    template <typename Y>
    friend class SharedPtr;

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
    }

    template <class Y>
    SharedPtr(Y* ptr) : control_block_(new ControlBlockPointer<Y>(ptr)) {
        ptr_ = ptr;
    }

    SharedPtr(const SharedPtr<T>& other) : control_block_(other.control_block_), ptr_(other.ptr_) {
        if (control_block_) {
            control_block_->AddReference();
        }
    }

    template <class Y>
    SharedPtr(const SharedPtr<Y>& other) : control_block_(other.control_block_), ptr_(other.ptr_) {
        if (control_block_) {
            control_block_->AddReference();
        }
    }

    template <class Y>
    SharedPtr(SharedPtr<Y>&& other) {
        control_block_ = other.control_block_;
        ptr_ = other.ptr_;
        other.control_block_ = nullptr;
        other.ptr_ = nullptr;
    }

    SharedPtr(SharedPtr&& other) {
        control_block_ = other.control_block_;
        ptr_ = other.ptr_;
        other.control_block_ = nullptr;
        other.ptr_ = nullptr;
    }

    template <class Y>
    SharedPtr(ControlBlockHolder<Y>* block) : control_block_(block) {
        ptr_ = block->GetPointer();
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
    explicit SharedPtr(const WeakPtr<T>& other);

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
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right);

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    return SharedPtr<T>(new ControlBlockHolder<T>(std::forward<Args>(args)...));
}

// Look for usage examples in tests
template <typename T>
class EnableSharedFromThis {
public:
    SharedPtr<T> SharedFromThis();
    SharedPtr<const T> SharedFromThis() const;

    WeakPtr<T> WeakFromThis() noexcept;
    WeakPtr<const T> WeakFromThis() const noexcept;
};
