#pragma once

#include <vector>
#include "Utils.h"

class TTaskScheduler;
class TTask;
template <typename T> class TFuture;

class ITaskNode {
public:
    virtual ~ITaskNode() = default;
    virtual void execute() = 0;
    virtual void* getResultPtr() = 0;
    virtual bool claim_move() = 0;
};

struct VoidResult {};

template <typename Ret>
struct ResultTraits { using Type = mystd::decay_t<Ret>; };

template <>
struct ResultTraits<void> { using Type = VoidResult; };

template <typename Func, typename Ret>
class TaskNode : public ITaskNode {
    Func func_;
    using ActualRet = typename ResultTraits<Ret>::Type;
    std::unique_ptr<ActualRet> result_;
    
    enum class State { NotStarted, InProgress, Completed } state_ = State::NotStarted;
    bool result_moved_ = false;

public:
    explicit TaskNode(Func f) : func_(mystd::move(f)) {}

    void execute() override {
        if (state_ == State::Completed) return;
        if (state_ == State::InProgress) throw mystd::runtime_error("Circular dependency detected");
        
        state_ = State::InProgress;
        if constexpr (mystd::is_void_v<Ret>) {
            func_();
            result_ = std::make_unique<ActualRet>();
        } else {
            result_ = std::make_unique<ActualRet>(func_());
        }
        state_ = State::Completed;
    }

    void* getResultPtr() override {
        execute();
        return result_.get();
    }

    bool claim_move() override {
        if (result_moved_) return false;
        result_moved_ = true;
        return true;
    }
};

template <typename T>
class TFuture {
    std::shared_ptr<ITaskNode> node_;
public:
    explicit TFuture(std::shared_ptr<ITaskNode> node) : node_(mystd::move(node)) {}

    T get() {
        node_->execute();
        using BaseT = mystd::decay_t<T>;
        BaseT* ptr = static_cast<BaseT*>(node_->getResultPtr());

        if constexpr (!mystd::is_lvalue_reference_v<T>) {
            if (!node_->claim_move()) {
                throw mystd::runtime_error("Future result was already moved");
            }
            return static_cast<T>(mystd::move(*ptr));
        } else {
            return static_cast<T>(*ptr);
        }
    }
};

template <>
class TFuture<void> {
    std::shared_ptr<ITaskNode> node_;
public:
    explicit TFuture(std::shared_ptr<ITaskNode> node) : node_(mystd::move(node)) {}
    VoidResult get() { node_->execute(); return VoidResult{}; }
};

template <typename T> struct is_future : mystd::false_type {};
template <typename T> struct is_future<TFuture<T>> : mystd::true_type {};

template <typename T>
decltype(auto) resolve_arg(T&& arg) {
    if constexpr (is_future<mystd::decay_t<T>>::value) {
        return arg.get();
    } else {
        return mystd::forward<T>(arg);
    }
}

class TTask {
    std::shared_ptr<ITaskNode> node_;
    TTaskScheduler* scheduler_;
public:
    TTask(std::shared_ptr<ITaskNode> node, TTaskScheduler* sched)
        : node_(mystd::move(node)), scheduler_(sched) {}

    template <typename T> T getResultSync() {
        node_->execute();
        if constexpr (mystd::is_void_v<T>) return;
        else return static_cast<T>(*static_cast<mystd::decay_t<T>*>(node_->getResultPtr()));
    }

    template <typename T> TFuture<T> getFutureResult() { return TFuture<T>(node_); }
    template <typename Func> TTask apply(Func&& func);
};

class TTaskScheduler {
    std::vector<std::shared_ptr<ITaskNode>> tasks_;
public:
    template <typename Func, typename... Args>
    TTask add(Func&& func, Args&&... args) {
        auto bound = [f = mystd::forward<Func>(func), tup = mystd::make_tuple(mystd::forward<Args>(args)...)]() mutable {
            return apply_helper<0, sizeof...(Args)>(f, tup);
        };
        auto node = std::make_shared<TaskNode<decltype(bound), decltype(bound())>>(mystd::move(bound));
        tasks_.push_back(node);
        return TTask(node, this);
    }

    void executeAll() { for (auto& task : tasks_) task->execute(); }
private:
    template <mystd::size_t I, mystd::size_t N, typename F, typename Tuple, typename... Args>
    static decltype(auto) apply_helper(F& f, Tuple& tup, Args&&... args) {
        if constexpr (I == N) {
            return detail::my_invoke(f, mystd::forward<Args>(args)...);
        } else if constexpr (mystd::is_same_v<mystd::decay_t<decltype(resolve_arg(mystd::move(mystd::get<I>(tup))))>, VoidResult>) {
            resolve_arg(mystd::move(mystd::get<I>(tup)));
            return apply_helper<I + 1, N>(f, tup, mystd::forward<Args>(args)...);
        } else {
            return apply_helper<I + 1, N>(f, tup, mystd::forward<Args>(args)..., resolve_arg(mystd::move(mystd::get<I>(tup))));
        }
    }
};

template <typename Func>
TTask TTask::apply(Func&& func) {
    using ArgType = typename detail::function_traits<mystd::decay_t<Func>>::arg_type;
    return scheduler_->add(mystd::forward<Func>(func), mystd::move(getFutureResult<ArgType>()));
}
