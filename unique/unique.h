#pragma once

#include "compressed_pair.h"

#include <cstddef>  // std::nullptr_t
#include <type_traits>
#include <utility>

// написать дефолтный делитр. Он должен быть просто функтором

template <typename T>
struct DefaultDeleter {
    DefaultDeleter() = default;

    template <class Up>
    DefaultDeleter(DefaultDeleter<Up>&& other) {
    }

    void operator()(T* ptr) {
        static_assert(!std::is_void_v<T>, "cannot delete void type");
        delete ptr;
    }
};

template <typename T>
struct DefaultDeleter<T[]> {
    DefaultDeleter() = default;

    template <class Up>
    DefaultDeleter(DefaultDeleter<Up>&& other) {
    }
    void operator()(T* ptr) {
        static_assert(!std::is_void_v<T>, "cannot delete void type");
        delete[] ptr;
    }
};

// Primary template
template <typename T, typename Deleter = DefaultDeleter<T>>
class UniquePtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) : pair_ptr_deleter_(ptr, Deleter()) {
    }

    UniquePtr(T* ptr, Deleter deleter) : pair_ptr_deleter_(ptr, std::move(deleter)) {
    }

    template <class OtherT, class OtherD>
    UniquePtr(UniquePtr<OtherT, OtherD>&& other) noexcept
        : pair_ptr_deleter_(other.Release(), std::forward<OtherD>(other.GetDeleter())) {
    }  // noexcept

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    template <class OtherT, class OtherD>
    UniquePtr& operator=(UniquePtr<OtherT, OtherD>&& other) noexcept {
        Reset(other.Release());
        pair_ptr_deleter_.GetSecond() = std::move(other.GetDeleter());
        return *this;
        //        pair_ptr_deleter_.GetSecond();
    }  // noexcept
       // pair_ptr_deleter_.GetFirst() = other.pair_ptr_deleter_.GetFirst();
       // почитать про std::exchange()

    UniquePtr& operator=(std::nullptr_t) {
        pair_ptr_deleter_.GetSecond()(pair_ptr_deleter_.GetFirst());
        pair_ptr_deleter_.GetFirst() = nullptr;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    // если тут нулптр - то ничего не делаем
    // вызывать семантику удаления (кастомную)
    // если кастомного нет - то просто делаем delete
    ~UniquePtr() {
        if (pair_ptr_deleter_.GetFirst()) {
            pair_ptr_deleter_.GetSecond()(pair_ptr_deleter_.GetFirst());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() noexcept {
        T* current_pointer = pair_ptr_deleter_.GetFirst();
        pair_ptr_deleter_.GetFirst() = nullptr;
        return current_pointer;
    }

    void Reset(T* ptr = nullptr) noexcept {
        T* tmp = pair_ptr_deleter_.GetFirst();
        pair_ptr_deleter_.GetFirst() = ptr;
        if (tmp) {
            pair_ptr_deleter_.GetSecond()(tmp);
        }
    }

    void Swap(UniquePtr& other) {
        std::swap(pair_ptr_deleter_.GetFirst(), other.pair_ptr_deleter_.GetFirst());
        std::swap(pair_ptr_deleter_.GetSecond(), other.pair_ptr_deleter_.GetSecond());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return pair_ptr_deleter_.GetFirst();
    }

    Deleter& GetDeleter() {
        return pair_ptr_deleter_.GetSecond();
    }

    const Deleter& GetDeleter() const {
        return pair_ptr_deleter_.GetSecond();
    }

    explicit operator bool() const {
        return pair_ptr_deleter_.GetFirst() != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    // в T[] эти штуки не нужны
    std::add_lvalue_reference_t<T> operator*() const {
        return *pair_ptr_deleter_.GetFirst();
    }  // std::add_lvalue_reference_t<T> - эта
       // штука поможет бороться с типом
       // void
    T* operator->() const {
        return pair_ptr_deleter_.GetFirst();
    }

private:
    CompressedPair<T*, Deleter> pair_ptr_deleter_;
};

// Specialization for arrays
// тут надо сделать deleter[] и реализовать оператор []
template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> {
public:
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) : pair_ptr_deleter_(ptr, Deleter()) {
    }

    UniquePtr(T* ptr, Deleter deleter) : pair_ptr_deleter_(ptr, std::move(deleter)) {
    }

    template <class OtherT, class OtherD>
    UniquePtr(UniquePtr<OtherT, OtherD>&& other) noexcept
        : pair_ptr_deleter_(other.Release(), std::forward<OtherD>(other.GetDeleter())) {
    }  // noexcept

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    template <class OtherT, class OtherD>
    UniquePtr& operator=(UniquePtr<OtherT, OtherD>&& other) noexcept {
        Reset(other.Release());
        pair_ptr_deleter_.GetSecond() = std::move(other.GetDeleter());
        return *this;
    }  // noexcept
       // pair_ptr_deleter_.GetFirst() = other.pair_ptr_deleter_.GetFirst();
       // почитать про std::exchange()

    UniquePtr& operator=(std::nullptr_t) {
        pair_ptr_deleter_.GetSecond()(pair_ptr_deleter_.GetFirst());
        pair_ptr_deleter_.GetFirst() = nullptr;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        if (pair_ptr_deleter_.GetFirst()) {
            pair_ptr_deleter_.GetSecond()(pair_ptr_deleter_.GetFirst());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() noexcept {
        T* current_pointer = pair_ptr_deleter_.GetFirst();
        pair_ptr_deleter_.GetFirst() = nullptr;
        return current_pointer;
    }

    void Reset(T* ptr = nullptr) noexcept {
        T* tmp = pair_ptr_deleter_.GetFirst();
        pair_ptr_deleter_.GetFirst() = ptr;
        if (tmp) {
            pair_ptr_deleter_.GetSecond()(tmp);
        }
    }

    void Swap(UniquePtr& other) {
        std::swap(pair_ptr_deleter_.GetFirst(), other.pair_ptr_deleter_.GetFirst());
        std::swap(pair_ptr_deleter_.GetSecond(), other.pair_ptr_deleter_.GetSecond());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return pair_ptr_deleter_.GetFirst();
    }

    Deleter& GetDeleter() {
        return pair_ptr_deleter_.GetSecond();
    }

    const Deleter& GetDeleter() const {
        return pair_ptr_deleter_.GetSecond();
    }

    explicit operator bool() const {
        return pair_ptr_deleter_.GetFirst() != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    T& operator[](size_t index) {
        return pair_ptr_deleter_.GetFirst()[index];
    }

private:
    CompressedPair<T*, Deleter> pair_ptr_deleter_;
};
