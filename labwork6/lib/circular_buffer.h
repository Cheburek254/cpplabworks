#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <memory>
#include <iterator>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include <initializer_list>
#include <cstddef>

template <typename T, bool Extend = false, typename Allocator = std::allocator<T>>
class circular_buffer {
private:
    T* data_;
    const size_t capacity_;
    size_t size_;
    size_t head_;
    Allocator alloc_;
    
    using AllocTraits = std::allocator_traits<Allocator>;

    size_t real_index(size_t idx) const {
        return (head_ + idx) % capacity_;
    }

public:
    
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = T*;
    using const_pointer = const T*;
    
    
    class iterator;
    class const_iterator;
    class reverse_iterator;
    class const_reverse_iterator;

   
    circular_buffer() : circular_buffer(0) {}
    
    explicit circular_buffer(const Allocator& alloc) : circular_buffer(0, alloc) {}
    
    explicit circular_buffer(size_type capacity, const Allocator& alloc = Allocator())
        : data_(nullptr), capacity_(capacity), size_(0), head_(0), alloc_(alloc) {
        if (capacity_ > 0) {
            data_ = AllocTraits::allocate(alloc_, capacity_);
        }
    }
    
    circular_buffer(size_type capacity, const_reference value, const Allocator& alloc = Allocator())
        : circular_buffer(capacity, alloc) {
        for (size_type i = 0; i < capacity; ++i) {
            push_back(value);
        }
    }
    
    circular_buffer(std::initializer_list<T> init, const Allocator& alloc = Allocator())
        : circular_buffer(init.size(), alloc) {
        for (const auto& value : init) {
            push_back(value);
        }
    }
    
    template <typename InputIt>
    circular_buffer(InputIt first, InputIt last, const Allocator& alloc = Allocator())
        : circular_buffer(std::distance(first, last), alloc) {
        for (InputIt it = first; it != last; ++it) {
            push_back(*it);
        }
    }
    
    template <typename InputIt>
    circular_buffer(InputIt first, InputIt last, size_type capacity, const Allocator& alloc = Allocator())
        : circular_buffer(capacity, alloc) {
        for (InputIt it = first; it != last && size_ < capacity_; ++it) {
            push_back(*it);
        }
    }
    
    circular_buffer(const circular_buffer& other)
        : data_(nullptr), capacity_(other.capacity_), size_(0), head_(0), 
          alloc_(AllocTraits::select_on_container_copy_construction(other.alloc_)) {
        if (capacity_ > 0) {
            data_ = AllocTraits::allocate(alloc_, capacity_);
            for (size_type i = 0; i < other.size_; ++i) {
                push_back(other[i]);
            }
        }
    }
    
    circular_buffer(const circular_buffer& other, const Allocator& alloc)
        : data_(nullptr), capacity_(other.capacity_), size_(0), head_(0), alloc_(alloc) {
        if (capacity_ > 0) {
            data_ = AllocTraits::allocate(alloc_, capacity_);
            for (size_type i = 0; i < other.size_; ++i) {
                push_back(other[i]);
            }
        }
    }
    circular_buffer(circular_buffer&& other) noexcept: data_(other.data_), capacity_(other.capacity_), size_(other.size_), head_(other.head_), alloc_(std::move(other.alloc_)) {
        other.data_ = nullptr;
        other.size_ = 0;
        other.head_ = 0;
    }
    circular_buffer(circular_buffer&& other, const Allocator& alloc): data_(nullptr), capacity_(other.capacity_), size_(0), head_(0), alloc_(alloc) {
        if (alloc_ == other.alloc_) {
            std::swap(data_, other.data_);
            std::swap(size_, other.size_);
            std::swap(head_, other.head_);
        } else {
            for (const auto& v : other) {
                push_back(v);
            }
        }
    }
    
    
    ~circular_buffer() {
        clear();
        if (data_) {
            AllocTraits::deallocate(alloc_, data_, capacity_);
        }
    }
    
    circular_buffer& operator=(const circular_buffer& other) {
        if (this != &other) {
            circular_buffer tmp(other);
            swap(tmp);
        }
        return *this;
    }
    
    circular_buffer& operator=(std::initializer_list<T> ilist) {
        assign(ilist);
        return *this;
    }
    
    template <typename InputIt>
    void assign(InputIt first, InputIt last) {
        clear();
        for (InputIt it = first; it != last && size_ < capacity_; ++it) {
            push_back(*it);
        }
    }
    
    void assign(size_type count, const_reference value) {
        clear();
        size_type num_to_copy = std::min(count, capacity_);
        for (size_type i = 0; i < num_to_copy; ++i) {
            push_back(value);
        }
    }
    
    void assign(std::initializer_list<T> ilist) {
        assign(ilist.begin(), ilist.end());
    }
    
    bool operator==(const circular_buffer& other) const {
        if (size_ != other.size_) return false;
        for (size_type i = 0; i < size_; ++i) {
            if ((*this)[i] != other[i]) return false;
        }
        return true;
    }
    
    bool operator!=(const circular_buffer& other) const {
        return !(*this == other);
    }
    
    allocator_type get_allocator() const { return alloc_; }
    
    reference front() {
        if (empty()) throw std::out_of_range("buff is empty");
        return data_[head_];
    }
    
    const_reference front() const {
        if (empty()) throw std::out_of_range("buff is empty");
        return data_[head_];
    }
    
    reference back() {
        if (empty()) throw std::out_of_range("buff is empty");
        return data_[(head_ + size_ - 1) % capacity_];
    }
    
    const_reference back() const {
        if (empty()) throw std::out_of_range("buff is empty");
        return data_[(head_ + size_ - 1) % capacity_];
    }
    
    reference operator[](size_type pos) {
        return data_[real_index(pos)];
    }
    
    const_reference operator[](size_type pos) const {
        return data_[real_index(pos)];
    }
    
    reference at(size_type pos) {
        if (pos >= size_) throw std::out_of_range("index out of range");
        return (*this)[pos];
    }
    
    const_reference at(size_type pos) const {
        if (pos >= size_) throw std::out_of_range("index out of range");
        return (*this)[pos];
    }
    
    bool empty() const { return size_ == 0; }
    bool full() const { return size_ == capacity_; }
    size_type size() const { return size_; }
    size_type max_size() const { return AllocTraits::max_size(alloc_); }
    size_type capacity() const { return capacity_; }
    
    void clear() {
        while (!empty()) {
            pop_back();
        }
    }
    
    void push_back(const_reference value) {
        if (capacity_ == 0) return;
        
        if (full()) {
            
            AllocTraits::destroy(alloc_, data_ + head_);
            AllocTraits::construct(alloc_, data_ + head_, value);
            head_ = (head_ + 1) % capacity_;
        } else {
            size_type pos = (head_ + size_) % capacity_;
            AllocTraits::construct(alloc_, data_ + pos, value);
            ++size_;
        }
    }
    
    void push_back(T&& value) {
        if (capacity_ == 0) return;
        
        if (full()) {
            AllocTraits::destroy(alloc_, data_ + head_);
            AllocTraits::construct(alloc_, data_ + head_, std::move(value));
            head_ = (head_ + 1) % capacity_;
        } else {
            size_type pos = (head_ + size_) % capacity_;
            AllocTraits::construct(alloc_, data_ + pos, std::move(value));
            ++size_;
        }
    }
    
    void pop_back() {
        if (empty()) throw std::out_of_range("buff is empty");
        --size_;
        size_type pos = (head_ + size_) % capacity_;
        AllocTraits::destroy(alloc_, data_ + pos);
    }
    
    void push_front(const_reference value) {
        if (capacity_ == 0) return;
        
        if (full()) {
            
            size_type tail_pos = (head_ + size_ - 1) % capacity_;
            AllocTraits::destroy(alloc_, data_ + tail_pos);
            head_ = (head_ + capacity_ - 1) % capacity_;
            AllocTraits::construct(alloc_, data_ + head_, value);
        } else {
            head_ = (head_ + capacity_ - 1) % capacity_;
            AllocTraits::construct(alloc_, data_ + head_, value);
            ++size_;
        }
    }
    
    void push_front(T&& value) {
        if (capacity_ == 0) return;
        
        if (full()) {
            size_type tail_pos = (head_ + size_ - 1) % capacity_;
            AllocTraits::destroy(alloc_, data_ + tail_pos);
            head_ = (head_ + capacity_ - 1) % capacity_;
            AllocTraits::construct(alloc_, data_ + head_, std::move(value));
        } else {
            head_ = (head_ + capacity_ - 1) % capacity_;
            AllocTraits::construct(alloc_, data_ + head_, std::move(value));
            ++size_;
        }
    }
    
    void pop_front() {
        if (empty()) throw std::out_of_range("buff is empty");
        AllocTraits::destroy(alloc_, data_ + head_);
        head_ = (head_ + 1) % capacity_;
        --size_;
    }
    
    
    iterator insert(const_iterator pos, const_reference value);
    iterator insert(const_iterator pos, T&& value);
    iterator insert(const_iterator pos, size_type count, const_reference value);
    template <typename InputIt>
    iterator insert(const_iterator pos, InputIt first, InputIt last);
    iterator erase(const_iterator pos);
    iterator erase(const_iterator first, const_iterator last);
    
    
    iterator begin() { return iterator(this, 0); }
    const_iterator begin() const { return const_iterator(this, 0); }
    const_iterator cbegin() const { return const_iterator(this, 0); }
    
    iterator end() { return iterator(this, size_); }
    const_iterator end() const { return const_iterator(this, size_); }
    const_iterator cend() const { return const_iterator(this, size_); }
    
   
    reverse_iterator rbegin() { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() const { return const_reverse_iterator(cend()); }
    
    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend() const { return const_reverse_iterator(cbegin()); }
    
    void swap(circular_buffer& other) noexcept {
        using std::swap;
        swap(data_, other.data_);
        swap(const_cast<size_t&>(capacity_), const_cast<size_t&>(other.capacity_));
        swap(size_, other.size_);
        swap(head_, other.head_);
        swap(alloc_, other.alloc_);
    }

private:
    template <typename U>
    iterator insert_impl(const_iterator pos, U&& value);
    
    iterator erase_impl(const_iterator first, const_iterator last);
};


template <typename T, bool Extend, typename Allocator>
class circular_buffer<T, Extend, Allocator>::iterator {
private:
    circular_buffer* buffer_;
    size_type offset_;

public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = ptrdiff_t;
    using pointer = T*;
    using reference = T&;
    
    iterator(circular_buffer* buffer = nullptr, size_type offset = 0)
        : buffer_(buffer), offset_(offset) {}
    
    iterator(const const_iterator& other)
        : buffer_(const_cast<circular_buffer*>(other.buffer_)), offset_(other.offset_) {}
    
    reference operator*() const {
        return (*buffer_)[offset_];
    }
    
    pointer operator->() const {
        return &(*buffer_)[offset_];
    }
    
    reference operator[](difference_type n) const {
        return (*buffer_)[offset_ + n];
    }
    
    iterator& operator++() {
        ++offset_;
        return *this;
    }
    
    iterator operator++(int) {
        iterator tmp = *this;
        ++(*this);
        return tmp;
    }
    
    iterator& operator--() {
        --offset_;
        return *this;
    }
    
    iterator operator--(int) {
        iterator tmp = *this;
        --(*this);
        return tmp;
    }
    
    iterator& operator+=(difference_type n) {
        offset_ += n;
        return *this;
    }
    
    iterator& operator-=(difference_type n) {
        offset_ -= n;
        return *this;
    }
    
    iterator operator+(difference_type n) const {
        iterator tmp = *this;
        tmp += n;
        return tmp;
    }
    
    iterator operator-(difference_type n) const {
        iterator tmp = *this;
        tmp -= n;
        return tmp;
    }
    
    difference_type operator-(const iterator& other) const {
        return offset_ - other.offset_;
    }
    
    difference_type operator-(const const_iterator& other) const {
        return offset_ - other.offset_;
    }
    
    bool operator==(const iterator& other) const {
        return buffer_ == other.buffer_ && offset_ == other.offset_;
    }
    
    bool operator==(const const_iterator& other) const {
        return buffer_ == other.buffer_ && offset_ == other.offset_;
    }
    
    bool operator!=(const iterator& other) const {
        return !(*this == other);
    }
    
    bool operator!=(const const_iterator& other) const {
        return !(*this == other);
    }
    
    bool operator<(const iterator& other) const {
        return offset_ < other.offset_;
    }
    
    bool operator<(const const_iterator& other) const {
        return offset_ < other.offset_;
    }
    
    bool operator>(const iterator& other) const {
        return offset_ > other.offset_;
    }
    
    bool operator>(const const_iterator& other) const {
        return offset_ > other.offset_;
    }
    
    bool operator<=(const iterator& other) const {
        return offset_ <= other.offset_;
    }
    
    bool operator<=(const const_iterator& other) const {
        return offset_ <= other.offset_;
    }
    
    bool operator>=(const iterator& other) const {
        return offset_ >= other.offset_;
    }
    
    bool operator>=(const const_iterator& other) const {
        return offset_ >= other.offset_;
    }
    
    friend iterator operator+(difference_type n, const iterator& it) {
        return it + n;
    }
    
    friend class const_iterator;
    
    size_type get_offset() const { return offset_; }
};


template <typename T, bool Extend, typename Allocator>
class circular_buffer<T, Extend, Allocator>::const_iterator {
private:
    const circular_buffer* buffer_;
    size_type offset_;

public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = const T;
    using difference_type = ptrdiff_t;
    using pointer = const T*;
    using reference = const T&;
    
    const_iterator(const circular_buffer* buffer = nullptr, size_type offset = 0)
        : buffer_(buffer), offset_(offset) {}
    
    const_iterator(const iterator& other)
        : buffer_(other.buffer_), offset_(other.offset_) {}
    
    reference operator*() const {
        return (*buffer_)[offset_];
    }
    
    pointer operator->() const {
        return &(*buffer_)[offset_];
    }
    
    reference operator[](difference_type n) const {
        return (*buffer_)[offset_ + n];
    }
    
    const_iterator& operator++() {
        ++offset_;
        return *this;
    }
    
    const_iterator operator++(int) {
        const_iterator tmp = *this;
        ++(*this);
        return tmp;
    }
    
    const_iterator& operator--() {
        --offset_;
        return *this;
    }
    
    const_iterator operator--(int) {
        const_iterator tmp = *this;
        --(*this);
        return tmp;
    }
    
    const_iterator& operator+=(difference_type n) {
        offset_ += n;
        return *this;
    }
    
    const_iterator& operator-=(difference_type n) {
        offset_ -= n;
        return *this;
    }
    
    const_iterator operator+(difference_type n) const {
        const_iterator tmp = *this;
        tmp += n;
        return tmp;
    }
    
    const_iterator operator-(difference_type n) const {
        const_iterator tmp = *this;
        tmp -= n;
        return tmp;
    }
    
    difference_type operator-(const const_iterator& other) const {
        return offset_ - other.offset_;
    }
    
    difference_type operator-(const iterator& other) const {
        return offset_ - other.offset_;
    }
    
    bool operator==(const const_iterator& other) const {
        return buffer_ == other.buffer_ && offset_ == other.offset_;
    }
    
    bool operator==(const iterator& other) const {
        return buffer_ == other.buffer_ && offset_ == other.offset_;
    }
    
    bool operator!=(const const_iterator& other) const {
        return !(*this == other);
    }
    
    bool operator!=(const iterator& other) const {
        return !(*this == other);
    }
    
    bool operator<(const const_iterator& other) const {
        return offset_ < other.offset_;
    }
    
    bool operator<(const iterator& other) const {
        return offset_ < other.offset_;
    }
    
    bool operator>(const const_iterator& other) const {
        return offset_ > other.offset_;
    }
    
    bool operator>(const iterator& other) const {
        return offset_ > other.offset_;
    }
    
    bool operator<=(const const_iterator& other) const {
        return offset_ <= other.offset_;
    }
    
    bool operator<=(const iterator& other) const {
        return offset_ <= other.offset_;
    }
    
    bool operator>=(const const_iterator& other) const {
        return offset_ >= other.offset_;
    }
    
    bool operator>=(const iterator& other) const {
        return offset_ >= other.offset_;
    }
    
    friend const_iterator operator+(difference_type n, const const_iterator& it) {
        return it + n;
    }
    
    friend class iterator;
};


template <typename T, bool Extend, typename Allocator>
class circular_buffer<T, Extend, Allocator>::reverse_iterator {
private:
    iterator base_;

public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    reverse_iterator() : base_() {}
    
    explicit reverse_iterator(iterator it) : base_(it) {}
    
    reverse_iterator(const reverse_iterator& other) = default;
    
    iterator base() const {
        return base_;
    }
    
    reference operator*() const {
        iterator tmp = base_;
        return *(--tmp);
    }
    
    pointer operator->() const {
        return &(operator*());
    }
    
    reference operator[](difference_type n) const {
        return *(*this + n);
    }
    
    reverse_iterator& operator++() {
        --base_;
        return *this;
    }
    
    reverse_iterator operator++(int) {
        reverse_iterator tmp = *this;
        --base_;
        return tmp;
    }
    
    reverse_iterator& operator--() {
        ++base_;
        return *this;
    }
    
    reverse_iterator operator--(int) {
        reverse_iterator tmp = *this;
        ++base_;
        return tmp;
    }
    
    reverse_iterator& operator+=(difference_type n) {
        base_ -= n;
        return *this;
    }
    
    reverse_iterator& operator-=(difference_type n) {
        base_ += n;
        return *this;
    }
    
    reverse_iterator operator+(difference_type n) const {
        return reverse_iterator(base_ - n);
    }
    
    reverse_iterator operator-(difference_type n) const {
        return reverse_iterator(base_ + n);
    }
    
    difference_type operator-(const reverse_iterator& other) const {
        return other.base_ - base_;
    }
    
    bool operator==(const reverse_iterator& other) const {
        return base_ == other.base_;
    }
    
    bool operator!=(const reverse_iterator& other) const {
        return base_ != other.base_;
    }
    
    bool operator<(const reverse_iterator& other) const {
        return base_ > other.base_;
    }
    
    bool operator>(const reverse_iterator& other) const {
        return base_ < other.base_;
    }
    
    bool operator<=(const reverse_iterator& other) const {
        return base_ >= other.base_;
    }
    
    bool operator>=(const reverse_iterator& other) const {
        return base_ <= other.base_;
    }
    

    bool operator==(const const_reverse_iterator& other) const;
    bool operator!=(const const_reverse_iterator& other) const;
    bool operator<(const const_reverse_iterator& other) const;
    bool operator>(const const_reverse_iterator& other) const;
    bool operator<=(const const_reverse_iterator& other) const;
    bool operator>=(const const_reverse_iterator& other) const;
    
    friend reverse_iterator operator+(difference_type n, const reverse_iterator& it) {
        return it + n;
    }
};


template <typename T, bool Extend, typename Allocator>
class circular_buffer<T, Extend, Allocator>::const_reverse_iterator {
private:
    const_iterator base_;

public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = const T;
    using difference_type = ptrdiff_t;
    using pointer = const T*;
    using reference = const T&;

    const_reverse_iterator() : base_() {}
    
    explicit const_reverse_iterator(const_iterator it) : base_(it) {}
    
    const_reverse_iterator(const reverse_iterator& other) : base_(other.base()) {}
    
    const_reverse_iterator(const const_reverse_iterator& other) = default;
    
    const_iterator base() const {
        return base_;
    }
    
    reference operator*() const {
        const_iterator tmp = base_;
        return *(--tmp);
    }
    
    pointer operator->() const {
        return &(operator*());
    }
    
    reference operator[](difference_type n) const {
        return *(*this + n);
    }
    
    const_reverse_iterator& operator++() {
        --base_;
        return *this;
    }
    
    const_reverse_iterator operator++(int) {
        const_reverse_iterator tmp = *this;
        --base_;
        return tmp;
    }
    
    const_reverse_iterator& operator--() {
        ++base_;
        return *this;
    }
    
    const_reverse_iterator operator--(int) {
        const_reverse_iterator tmp = *this;
        ++base_;
        return tmp;
    }
    
    const_reverse_iterator& operator+=(difference_type n) {
        base_ -= n;
        return *this;
    }
    
    const_reverse_iterator& operator-=(difference_type n) {
        base_ += n;
        return *this;
    }
    
    const_reverse_iterator operator+(difference_type n) const {
        return const_reverse_iterator(base_ - n);
    }
    
    const_reverse_iterator operator-(difference_type n) const {
        return const_reverse_iterator(base_ + n);
    }
    
    difference_type operator-(const const_reverse_iterator& other) const {
        return other.base_ - base_;
    }
    
    bool operator==(const const_reverse_iterator& other) const {
        return base_ == other.base_;
    }
    
    bool operator!=(const const_reverse_iterator& other) const {
        return base_ != other.base_;
    }
    
    bool operator<(const const_reverse_iterator& other) const {
        return base_ > other.base_;
    }
    
    bool operator>(const const_reverse_iterator& other) const {
        return base_ < other.base_;
    }
    
    bool operator<=(const const_reverse_iterator& other) const {
        return base_ >= other.base_;
    }
    
    bool operator>=(const const_reverse_iterator& other) const {
        return base_ <= other.base_;
    }
    

    bool operator==(const reverse_iterator& other) const;
    bool operator!=(const reverse_iterator& other) const;
    bool operator<(const reverse_iterator& other) const;
    bool operator>(const reverse_iterator& other) const;
    bool operator<=(const reverse_iterator& other) const;
    bool operator>=(const reverse_iterator& other) const;
    
    friend const_reverse_iterator operator+(difference_type n, const const_reverse_iterator& it) {
        return it + n;
    }
};


template <typename T, bool Extend, typename Allocator>
typename circular_buffer<T, Extend, Allocator>::iterator
circular_buffer<T, Extend, Allocator>::insert(const_iterator pos, const_reference value) {
    return insert_impl(pos, value);
}

template <typename T, bool Extend, typename Allocator>
typename circular_buffer<T, Extend, Allocator>::iterator
circular_buffer<T, Extend, Allocator>::insert(const_iterator pos, T&& value) {
    return insert_impl(pos, std::move(value));
}

template <typename T, bool Extend, typename Allocator>
typename circular_buffer<T, Extend, Allocator>::iterator
circular_buffer<T, Extend, Allocator>::insert(const_iterator pos, size_type count, const_reference value) {
    if (full()) throw std::overflow_error("buff is full");
    
    iterator result = iterator(this, pos - cbegin());
    for (size_type i = 0; i < count; ++i) {
        if (full()) throw std::overflow_error("buff is full");
        result = insert_impl(const_iterator(this, result.get_offset()), value);
    }
    return result;
}

template <typename T, bool Extend, typename Allocator>
template <typename InputIt>
typename circular_buffer<T, Extend, Allocator>::iterator
circular_buffer<T, Extend, Allocator>::insert(const_iterator pos, InputIt first, InputIt last) {
    if (full()) throw std::overflow_error("buff is full");
    
    iterator result = iterator(this, pos - cbegin());
    for (InputIt it = first; it != last; ++it) {
        if (full()) throw std::overflow_error("buff is full");
        result = insert_impl(const_iterator(this, result.get_offset()), *it);
    }
    return result;
}

template <typename T, bool Extend, typename Allocator>
typename circular_buffer<T, Extend, Allocator>::iterator
circular_buffer<T, Extend, Allocator>::erase(const_iterator pos) {
    if (pos == cend()) throw std::out_of_range("invalid iterator");
    return erase_impl(pos, pos + 1);
}

template <typename T, bool Extend, typename Allocator>
typename circular_buffer<T, Extend, Allocator>::iterator
circular_buffer<T, Extend, Allocator>::erase(const_iterator first, const_iterator last) {
    if (first > last) throw std::invalid_argument("invalid range");
    return erase_impl(first, last);
}


template <typename T, bool Extend, typename Allocator>
template <typename U>
typename circular_buffer<T, Extend, Allocator>::iterator
circular_buffer<T, Extend, Allocator>::insert_impl(const_iterator pos, U&& value) {
    if (capacity_ == 0) return iterator(this, 0);
    if (full()) {
        throw std::overflow_error("buff is full");
    }
    
    difference_type offset = pos - cbegin();
    
    for (difference_type i = size_ - 1; i >= offset; --i) {
        size_type src_idx = (head_ + i) % capacity_;
        size_type dst_idx = (head_ + i + 1) % capacity_;
        
        AllocTraits::construct(alloc_, data_ + dst_idx, 
                              std::move_if_noexcept(data_[src_idx]));
        AllocTraits::destroy(alloc_, data_ + src_idx);
    }
    
    size_type target_idx = (head_ + offset) % capacity_;
    AllocTraits::construct(alloc_, data_ + target_idx, std::forward<U>(value));
    ++size_;
    
    return iterator(this, offset);
}

template <typename T, bool Extend, typename Allocator>
typename circular_buffer<T, Extend, Allocator>::iterator
circular_buffer<T, Extend, Allocator>::erase_impl(const_iterator first, const_iterator last) {
    difference_type first_offset = first - cbegin();
    difference_type last_offset = last - cbegin();
    difference_type count = last_offset - first_offset;
    
    if (count <= 0) return iterator(this, first_offset);
    
    for (difference_type i = first_offset; i < last_offset; ++i) {
        size_type idx = (head_ + i) % capacity_;
        AllocTraits::destroy(alloc_, data_ + idx);
    }
    
    for (difference_type i = last_offset; i < static_cast<difference_type>(size_); ++i) {
        size_type src_idx = (head_ + i) % capacity_;
        size_type dst_idx = (head_ + i - count) % capacity_;
        
        AllocTraits::construct(alloc_, data_ + dst_idx, 
                              std::move_if_noexcept(data_[src_idx]));
        AllocTraits::destroy(alloc_, data_ + src_idx);
    }
    
    size_ -= count;
    return iterator(this, first_offset);
}

template <typename T, bool Extend, typename Allocator>
bool circular_buffer<T, Extend, Allocator>::reverse_iterator::operator==(
    const const_reverse_iterator& other) const {
    return base_ == other.base();
}

template <typename T, bool Extend, typename Allocator>
bool circular_buffer<T, Extend, Allocator>::reverse_iterator::operator!=(
    const const_reverse_iterator& other) const {
    return base_ != other.base();
}

template <typename T, bool Extend, typename Allocator>
bool circular_buffer<T, Extend, Allocator>::reverse_iterator::operator<(
    const const_reverse_iterator& other) const {
    return base_ > other.base();
}

template <typename T, bool Extend, typename Allocator>
bool circular_buffer<T, Extend, Allocator>::reverse_iterator::operator>(
    const const_reverse_iterator& other) const {
    return base_ < other.base();
}

template <typename T, bool Extend, typename Allocator>
bool circular_buffer<T, Extend, Allocator>::reverse_iterator::operator<=(
    const const_reverse_iterator& other) const {
    return base_ >= other.base();
}

template <typename T, bool Extend, typename Allocator>
bool circular_buffer<T, Extend, Allocator>::reverse_iterator::operator>=(
    const const_reverse_iterator& other) const {
    return base_ <= other.base();
}

template <typename T, bool Extend, typename Allocator>
bool circular_buffer<T, Extend, Allocator>::const_reverse_iterator::operator==(
    const reverse_iterator& other) const {
    return base_ == other.base();
}

template <typename T, bool Extend, typename Allocator>
bool circular_buffer<T, Extend, Allocator>::const_reverse_iterator::operator!=(
    const reverse_iterator& other) const {
    return base_ != other.base();
}

template <typename T, bool Extend, typename Allocator>
bool circular_buffer<T, Extend, Allocator>::const_reverse_iterator::operator<(
    const reverse_iterator& other) const {
    return base_ > other.base();
}

template <typename T, bool Extend, typename Allocator>
bool circular_buffer<T, Extend, Allocator>::const_reverse_iterator::operator>(
    const reverse_iterator& other) const {
    return base_ < other.base();
}

template <typename T, bool Extend, typename Allocator>
bool circular_buffer<T, Extend, Allocator>::const_reverse_iterator::operator<=(
    const reverse_iterator& other) const {
    return base_ >= other.base();
}

template <typename T, bool Extend, typename Allocator>
bool circular_buffer<T, Extend, Allocator>::const_reverse_iterator::operator>=(
    const reverse_iterator& other) const {
    return base_ <= other.base();
}


template <typename T, bool Extend, typename Alloc>
void swap(circular_buffer<T, Extend, Alloc>& lhs, circular_buffer<T, Extend, Alloc>& rhs) noexcept {
    lhs.swap(rhs);
}

#endif // CIRCULAR_BUFFER_H
