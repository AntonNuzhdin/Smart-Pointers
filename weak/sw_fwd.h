#pragma once

#include <exception>

class BadWeakPtr : public std::exception {};

template <typename T>
class SharedPtr;

template <typename T>
class WeakPtr;

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
    }

private:
    T* ptr_;
};

template <typename T>
class ControlBlockHolder : public ControlBlockBase {
    template <typename Y>
    friend class SharedPtr;

public:
    //    ControlBlockHolder() {
    //        AddReference();
    //    };

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
        //        GetPointer()->~T();
    }

private:
    std::aligned_storage_t<sizeof(T), alignof(T)> storage_;
};
