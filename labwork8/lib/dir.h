#pragma once

#include <cstddef>
#include <filesystem>
#include <iterator>
#include <type_traits>
#include <utility>
#include <variant>

class Dir {
public:
    Dir(std::filesystem::path path, bool recursive = false)
        : path_(std::move(path)), recursive_(recursive) {}

    class Iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::filesystem::path;
        using difference_type = std::ptrdiff_t;
        using pointer = std::filesystem::path*;
        using reference = std::filesystem::path&;

        Iterator() = default;

        Iterator(const std::filesystem::path& path, bool recursive) {
            if (recursive) {
                it_ = std::filesystem::recursive_directory_iterator(path);
            } else {
                it_ = std::filesystem::directory_iterator(path);
            }
            UpdatePath();
        }

        reference operator*() { return current_path_; }
        pointer operator->() { return &current_path_; }

        Iterator& operator++() {
            std::visit([](auto&& it) { ++it; }, it_);
            UpdatePath();
            return *this;
        }

        bool operator==(const Iterator& other) const {
            return IsEnd() == other.IsEnd();
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }

    private:
        std::variant<std::filesystem::directory_iterator, std::filesystem::recursive_directory_iterator> it_;
        std::filesystem::path current_path_;

        bool IsEnd() const {
            return std::visit([](auto&& it) { return it == std::decay_t<decltype(it)>(); }, it_);
        }

        void UpdatePath() {
            if (!IsEnd()) {
                current_path_ = std::visit([](auto&& it) { return it->path(); }, it_);
            }
        }
    };

    // NOLINTNEXTLINE
    Iterator begin() const { return Iterator(path_, recursive_); }
    // NOLINTNEXTLINE
    Iterator end() const { return Iterator(); }

private:
    std::filesystem::path path_;
    bool recursive_;
};