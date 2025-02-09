
#include<iostream>
#include<vector>
#include<thread>
#include<mutex>
#include<condition_variable>
#include<queue>
#include<future>
class A {
public:
	int a;

};
template<typename T>
class threadpool {
public:
	threadpool(int n) :threads(n), stop(false) {
		for (int i = 0; i < threads; i++) {
			works.emplace_back(
				[this] {
				while (1) {
					std::function<void(void)> task;
					{
						std::unique_lock<std::mutex> lk(this->mutex__);
						this->cond.wait(lk, [this] {return  (stop) || (!this->q.empty()); });
						if (stop)break;
						task = q.front();
						q.pop();
					}

					task();
				}
				std::unique_lock<std::mutex> lk(this->mutex__);
				std::cout << "done" << std::endl;
				lk.unlock();
			}
			);
		}
	}

	std::future<T> enqueue(std::function<T()> f) {
		std::future<T> fu;
		auto task = std::make_shared< std::packaged_task<T()>>(f);
		{
			std::unique_lock<std::mutex> lk(this->mutex__);

			fu = task->get_future();
			q.emplace([task]() {    (*task)();  });
			//(*task)();
			cond.notify_one();
		}
		return fu;

	}


	~threadpool()
	{
		stop = true;
		cond.notify_all();
		for (std::thread &worker : works)
			worker.join();
	}
private:
	int threads;
	std::vector<std::thread> works;
	std::condition_variable cond;
	std::mutex mutex__;
	std::queue < std::function<void()> > q;
	bool stop;
};


int main() {

	threadpool<A> tp(4);
	std::vector< std::future<A> > fus;

	for (int i = 0; i < 8; i++) {
		fus.emplace_back(tp.enqueue([i] {
			A aa;
			aa.a = i * i;
			return aa;
		})
		);

	}

	for (int i = 0; i < 8; i++) {
		A t = fus[i].get();
		std::cout << t.a << " " << std::endl;
	}
	return 0;
}