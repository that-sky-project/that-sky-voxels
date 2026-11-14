// Copyright (c) 2013-2016, Stoyan Nikolov
// All rights reserved.
// Voxels Library, please see LICENSE for licensing details.
#pragma once

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace Voxels
{

/// A small C++17 worker pool. Create one per library operation (grid
/// voxelization, polygonization) and reuse it for the phases of that
/// operation. Worker threads never outlive the operation, which keeps the
/// library safe to unload and to call from several threads.
class ParallelExecutor
{
public:
	ParallelExecutor() = default;
	~ParallelExecutor()
	{
		Stop();
	}

	ParallelExecutor(const ParallelExecutor&) = delete;
	ParallelExecutor& operator=(const ParallelExecutor&) = delete;

	/// Runs fn(0) ... fn(count-1). The calling thread participates. Work
	/// items are handed out dynamically, so blocks with different costs are
	/// balanced between the threads. Below minParallelCount the call runs
	/// serially and no threads are created.
	template<typename Func>
	void Run(unsigned count, unsigned minParallelCount, Func&& fn)
	{
		if (count == 0)
			return;

		if (count < minParallelCount || count <= 1)
		{
			for (unsigned i = 0; i < count; ++i)
				fn(i);
			return;
		}

		Start();
		if (m_Workers.empty())
		{
			for (unsigned i = 0; i < count; ++i)
				fn(i);
			return;
		}

		std::function<void(unsigned)> callable(fn);
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			m_Fn = callable;
			m_Count = count;
			m_Next.store(0, std::memory_order_relaxed);
			m_Pending = unsigned(m_Workers.size());
			++m_Generation;
		}
		m_Wake.notify_all();

		for (;;)
		{
			const unsigned i = m_Next.fetch_add(1, std::memory_order_relaxed);
			if (i >= count)
				break;
			fn(i);
		}

		std::unique_lock<std::mutex> lock(m_Mutex);
		m_Done.wait(lock, [this]() { return m_Pending == 0; });
	}

private:
	void Start()
	{
		if (m_Started)
			return;
		m_Started = true;

		unsigned threadCount = std::thread::hardware_concurrency();
		if (threadCount == 0)
			threadCount = 1;
		m_Workers.reserve(threadCount - 1);
		for (unsigned t = 1; t < threadCount; ++t)
			m_Workers.emplace_back([this]() { WorkerLoop(); });
	}

	void Stop()
	{
		if (!m_Started)
			return;
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			m_Stop = true;
		}
		m_Wake.notify_all();
		for (auto& worker : m_Workers)
			worker.join();
		m_Workers.clear();
	}

	void WorkerLoop()
	{
		std::uint64_t seenGeneration = 0;
		for (;;)
		{
			std::function<void(unsigned)> fn;
			unsigned count = 0;
			{
				std::unique_lock<std::mutex> lock(m_Mutex);
				m_Wake.wait(lock, [this, seenGeneration]()
				{
					return m_Stop || m_Generation != seenGeneration;
				});
				if (m_Stop)
					return;
				seenGeneration = m_Generation;
				fn = m_Fn;
				count = m_Count;
			}

			for (;;)
			{
				const unsigned i = m_Next.fetch_add(1, std::memory_order_relaxed);
				if (i >= count)
					break;
				fn(i);
			}

			{
				std::lock_guard<std::mutex> lock(m_Mutex);
				if (--m_Pending == 0)
					m_Done.notify_one();
			}
		}
	}

	std::vector<std::thread> m_Workers;

	std::mutex m_Mutex;
	std::condition_variable m_Wake;
	std::condition_variable m_Done;
	std::function<void(unsigned)> m_Fn;
	std::atomic<unsigned> m_Next{ 0 };
	unsigned m_Count = 0;
	unsigned m_Pending = 0;
	std::uint64_t m_Generation = 0;
	bool m_Started = false;
	bool m_Stop = false;
};

}
