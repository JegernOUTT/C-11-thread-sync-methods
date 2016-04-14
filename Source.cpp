#include <iostream>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <chrono>

using namespace std;
using namespace std::chrono;

struct conditional_variable
{
	void operator()()
	{
		uint64_t a = 0ul;
		mutex m;
		condition_variable cv;

		thread t([&]
		{
			lock_guard<mutex> lg(m);
			while (a != 1000000000) ++a;
			cv.notify_one();
		});
		thread t2([&]()
		{
			unique_lock<mutex> ul(m);
			cv.wait(ul, [&] { return a == 1000000000; });
			cout << "Value a: " << a << endl;
		});

		t.join();
		t2.join();
	}
};

struct asyncs
{
	void operator()()
	{
		uint64_t a = 0ul;

		auto f1 = async(std::launch::async, [&]()-> uint64_t
		{
			while (a != 1000000000) ++a;
			return a;
		});

		auto f2 = async(std::launch::async, [&]()
		{
			cout << "Value a: " << f1.get() << endl;
		});

		f2.get();
	}
};

struct tasks
{
	void operator()()
	{
		uint64_t a = 0ul;

		packaged_task<uint64_t(uint64_t &)> pt1([](uint64_t & a)
		{
			while (a != 1000000000) ++a;
			return a;
		});
		packaged_task<void(shared_future<uint64_t> &&)> pt2([](shared_future<uint64_t> && f)
		{
			cout << "Value a: " << f.get() << endl;
		});

		auto f = pt1.get_future();
		thread t1(move(pt1), ref(a));
		thread t2(move(pt2), f.share());
		t1.join();
		t2.join();
	}
};

struct promises
{
	void operator()()
	{
		uint64_t a = 0ul;
		promise<uint64_t> p;

		thread t([&]
		{
			while (a != 1000000000) ++a;
			p.set_value(a);
		});
		thread t2([&]()
		{
			cout << "Value a: " << p.get_future().get() << endl;
		});

		t.join();
		t2.join();
	}
};

template <typename ... pred_args>
struct timed_test
{
	template<class F, class...Ts, std::size_t...Is>
	static void for_each_in_tuple(const std::tuple<Ts...> & tuple, F func, std::index_sequence<Is...>) {
		using expander = int[];
		(void)expander {
			0, ((void)func(std::get<Is>(tuple)), 0)...
		};
	}

	template<class F, class...Ts>
	static void for_each_in_tuple(const std::tuple<Ts...> & tuple, F func) {
		for_each_in_tuple(tuple, func, std::make_index_sequence<sizeof...(Ts)>());
	}

	static void test(tuple<pred_args...> tup = tuple<pred_args...>{})
	{
		for_each_in_tuple(tup, [](auto el)
		{
			auto t1 = high_resolution_clock::now();

			el();

			auto t2 = high_resolution_clock::now();
			cout << "Time for "
				<< typeid(el).name()
				<< ": " << duration_cast<milliseconds>(t2 - t1).count()
				<< " milsec." << endl << endl;
		});
	}
};

int main()
{
	timed_test<conditional_variable, asyncs, tasks, promises>::test();

	getchar();
	return 0;
}