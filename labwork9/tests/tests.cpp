#include "TTaskScheduler.h"
#include <gtest/gtest.h>
#include <string>

TEST(TaskSchedulerTest, BasicDependency) {
    TTaskScheduler scheduler;

    auto t1 = scheduler.add([]() { return 10; });
    auto t2 = scheduler.add([](int x) { return x + 20; }, t1.getFutureResult<int>());
    
    scheduler.executeAll();
    EXPECT_EQ(t2.getResultSync<int>(), 30);
}

TEST(TaskSchedulerTest, ApplyChain) {
    TTaskScheduler scheduler;

    auto t1 = scheduler.add([]() { return std::string("Hello"); });
    
    auto t2 = t1.apply([](std::string&& s) {
        return s + " World";
    });

    scheduler.executeAll();
    EXPECT_EQ(t2.getResultSync<std::string>(), "Hello World");
}

TEST(TaskSchedulerTest, SingleMoveSemantics) {
    TTaskScheduler scheduler;

    auto t1 = scheduler.add([]() { return 100; });
    auto future1 = t1.getFutureResult<int>();
    auto future2 = t1.getFutureResult<int>(); 
    
    auto t2 = scheduler.add([](int x) { return x; }, future1);
    
    scheduler.executeAll();
    EXPECT_EQ(t2.getResultSync<int>(), 100);

    EXPECT_THROW(future2.get(), mystd::runtime_error);
}

TEST(TaskSchedulerTest, MultipleReferences) {
    TTaskScheduler scheduler;
    auto t1 = scheduler.add([]() { return 42; });

    auto f1 = t1.getFutureResult<const int&>();
    auto f2 = t1.getFutureResult<const int&>();

    auto t2 = scheduler.add([](const int& x) { return x * 2; }, f1);
    auto t3 = scheduler.add([](const int& x) { return x * 3; }, f2);

    scheduler.executeAll();
    EXPECT_EQ(t2.getResultSync<int>(), 84);
    EXPECT_EQ(t3.getResultSync<int>(), 126);
}

TEST(TaskSchedulerTest, VoidTaskExecution) {
    TTaskScheduler scheduler;
    bool executed = false;
    
    auto t1 = scheduler.add([&executed]() { executed = true; });
    auto t2 = scheduler.add([]() { return 5; }, t1.getFutureResult<void>());

    scheduler.executeAll();
    EXPECT_TRUE(executed);
    EXPECT_EQ(t2.getResultSync<int>(), 5);
}

struct TestObject {
    int value = 0;
    int add(int x) { return value + x; }
};

TEST(TaskSchedulerTest, PointerToMemberFunction) {
    TTaskScheduler scheduler;
    TestObject obj{10};

    auto t1 = scheduler.add([]() { return 5; });
    auto t2 = scheduler.add(&TestObject::add, &obj, t1.getFutureResult<int>());

    scheduler.executeAll();
    EXPECT_EQ(t2.getResultSync<int>(), 15);
}
