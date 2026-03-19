#include <coroutine>
#include <exception>

struct Task
{
    struct promise_type
    {
        Task get_return_object()
        {
            return Task{
                std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }

        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }

        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };

    std::coroutine_handle<promise_type> handle;
};

// 1.方案1 h.resume()的A线程任务 h.resume()的B线程任务 只要调用 就能将协程拉回指定线程
// co_await ResumeOnMainThread{};
// updateUI();
// co_await ResumeOnWorkerThread{};
// doHeavyWork();

// 2.方案2 提供线程接口 在回调种混线程切换语义
// struct FetchFileAwaiter {
//     bool await_ready() { return false; }
//     void await_suspend(std::coroutine_handle<> h) {
//         downloadThread.enqueue([h] {
//             bool ok = do_download();
//             post_to_main_thread([h, ok] {
//                 h.resume();
//             });
//         });
//     }
//     bool await_resume() { return ok; }
// };

// 3.方案3 协程 + TaskGraph（引擎常用）
// 协程只描述流程
// 每个 step 注册到调度系统
// 没有随意 h.resume

// 那会很奇怪 比如我的UI线程在阻塞执行完connect 后协程在下载器线程执行 下载器线程执行完之后 通过上述前两种方法 显示恢复到UI线程执行
// UI线程此时在执行while loop 这本来就是阻塞循环的 那么恢复后的逻辑是怎么插入的 还是我理解错了

// 这个 UI 线程的任务队列不是自己手写的吧 是操作系统调度的吧

// 你说
// 误解：resume 到 UI 线程 = OS 马上切换 CPU 给 UI 线程
// 我的意思是 CPU现在本来就在执行UI线程的工作 比如说正在计算矩阵等 计算到一半
// 下载线程调用ui线程的接口 h.resume()? 哦哦 这样是不行的 因为执行这个代码的还是下载线程 恢复还是恢复到下载线程
// 要塞进ui线程的队列 ui线程自行检查并且执行 之后执行剩下的阻塞任务
// 这样确实会卡下一帧的UI渲染

// 不如一开始就将整个任务包装给一个拥有独立线程的命令调度器线程 这个命令调度器线程可以是单线程
// 执行
// while(true)
// {
//     if(checkqueue())
//         queue.pop().dotask();
// }

// 提供接口
// pushtask(task)
// {
//     queue.push(task)
// }

// resume(
//     pushTask([](){h.resume();})
// )