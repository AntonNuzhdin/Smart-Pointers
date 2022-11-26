#pragma once

#include <type_traits>
#include <memory>
#include <utility>

// Me think, why waste time write lot code, when few code do trick.
template <typename F, typename S, bool First_empty = std::is_empty_v<F> && !std::is_final_v<F>,
          bool Second_empty = std::is_empty_v<S> && !std::is_final_v<S>>
class CompressedPair;

// both empty && F == S
template <typename T>
class CompressedPair<T, T, true, true> : T {
public:
    template <typename U1, typename U2>
    CompressedPair(U1&& first, U2&& second)
        : U1(std::forward<U1>(first)), U2(std::forward<U2>(second)) {
    }

    CompressedPair& operator=(CompressedPair&& other) {
        std::swap(first_, other.first_);
        other.first_ = T();
    }

    T& GetFirst() {
        return *this;
    }

    const T& GetFirst() const {
        return *this;
    }

    T& GetSecond() {
        return *this;
    }

    const T& GetSecond() const {
        return *this;
    };

private:
    T first_ = T();
};

// both empty && F != S
template <typename F, typename S>
class CompressedPair<F, S, true, true> : private F, private S {
public:
    template <typename U1, typename U2>
    CompressedPair(U1&& first, U2&& second)
        : F(std::forward<U1>(first)), S(std::forward<U2>(second)) {
    }

    F& GetFirst() {
        return *this;
    }

    const F& GetFirst() const {
        return *this;
    }

    S& GetSecond() {
        return *this;
    }

    const S& GetSecond() const {
        return *this;
    };
};

// F != S && F empty
template <typename F, typename S>
class CompressedPair<F, S, true, false> : private F {
public:
    template <typename U1, typename U2>
    CompressedPair(U1&& first, U2&& second)
        : F(std::forward<U1>(first)), second_(std::forward<U2>(second)) {
    }

    CompressedPair& operator=(CompressedPair&& other) {
        std::swap(second_, other.second_);
        other.second_ = S();
    }

    F& GetFirst() {
        return *this;
    }

    const F& GetFirst() const {
        return *this;
    }

    S& GetSecond() {
        return second_;
    }

    const S& GetSecond() const {
        return second_;
    };

private:
    S second_;
};

// F != S && S empty
template <typename F, typename S>
class CompressedPair<F, S, false, true> : private S {
public:
    template <typename U1, typename U2>
    CompressedPair(U1&& first, U2&& second)
        : first_(std::forward<U1>(first)), S(std::forward<U2>(second)) {
    }

    CompressedPair& operator=(CompressedPair&& other) {
        std::swap(first_, other.first_);
        other.first_ = F();
    }

    F& GetFirst() {
        return first_;
    }

    const F& GetFirst() const {
        return first_;
    }

    S& GetSecond() {
        return *this;
    }

    const S& GetSecond() const {
        return *this;
    };

private:
    F first_;
};

// F != S && non empty
template <typename F, typename S>
class CompressedPair<F, S, false, false> {
public:
    CompressedPair() = default;
    template <typename U1, typename U2>
    CompressedPair(U1&& first, U2&& second)
        : first_(std::forward<U1>(first)), second_(std::forward<U2>(second)) {
    }

    CompressedPair& operator=(CompressedPair&& other) {
        std::swap(first_, other.first_);
        std::swap(second_, other.second_);
        other.first_ = F();
    }

    F& GetFirst() {
        return first_;
    }

    const F& GetFirst() const {
        return first_;
    }

    S& GetSecond() {
        return second_;
    }

    const S& GetSecond() const {
        return second_;
    };

private:
    F first_ = F();
    S second_ = S();
};