
typedef std::vector<CoroutineUnit> __cors_type__;
typedef std::vector<NThread> __nth_type__;
typedef long __msg_type__
typedef std::map<long, void*> __args_list_type__;

typedef struct {
	typedef enum { kCorsAsyncFullfilled, kCorsInterrupt, kCorsAwait, kCorEnded, kNThreadsBlock } __msg_htype__;
	__msg_htype__ type;
	long content;
	void* data;
} msg_t;

class CoroutineUnit {
public:
	CoroutineUnit();
	~CoroutineUnit();

private:
	//memory
	size_t memory;
	time_t io_time;
	cpuinfo_t cpuinfo;
	//size of stack
	size_t ssize;
	typedef enum { kRunning, kAwait, kPreparing, kDying } state_t;
	state_t status;
	void* data; 
};

class SchedulerBase : public Boost::nocopyable {};

class NThread {};

class CorScheduler : public SchedulerBase{
public:
	CorScheduler();
	~CorScheduler();

	void Dispatch(msg_t);
	__cors_type__ priority_cors;
	__nth_type__ nthreads;
	const CoroutineUnit& ref_running;
	long loopId[MAX_CORS]; 
};

//similar to NThreadScheduler
//In this case, asynchronous task is handled by coroutine instead of NThreads, which scheduled by system suitable for muti-processers
void CorScheduler::Dispatch(msg_t msg) {
	//M:N dispatcher
	if (priority_cors.IsEmpty()) return;
	switch (msg.type) {
		case msg_t::kCorEnded:
		//a cor yields
		case msg_t::kCorsAwait:
			__msg_type__ cor_id = msg.content;
			//take out the one who has largest priority
			//compute via 'M = Y * A', in which 'M' is for priority vector, 'Y' is matrix for resource occupied by cors
			//and A is a weight vector, this list is sorted by priority
			//FOREACH_EXCEPT_AWAIT(except, set, operation:Lamda expression)
			//when 'Await', continue
			FOREACH_EXCEPT_AWAIT(cor_id, priority_cors, [](const CoroutineUnit& cor) {
				//cors interact with 'args'
				if (cor.status == CoroutineUnit::kPreparing)
					cor.Start();
				else {
					cor.Resume(ref_running.args);
				}
			});
			break;
		//for preemption
		case msg_t::kCorsInterrupt:
			//compute cpu usage info and IO time
			//predicted by last IO time
			auto status = ref_running.status;
			_assert(status == CoroutineUnit::kRunning || status == CoroutineUnit::kPreparing);
			auto res_info = AnalysisYRes(msg.content);
			auto winner = OptimizeRes(ref_running.id, msg.content);
			CoroutineUnit& intr_cor = *static_cast<CoroutineUnit*>(msg.data);
			if (winner.id == ref_running.id) return;
			//current cor -> Await state
			auto args = ref_running.Yield();
			intr_cor.SetArgsForRes(args);
			//only preparing cor can interrupt a running cor
			_assert(intr_cor.status == CoroutineUnit::kPreparing);
			intr_cor.Start();
			break;
		//when looper notify scheduler that asynchronous tasks are fullfilled	
		case msg_t::kCorsAsyncFullfilled:
			CoroutineUnit& yielded_cor = *static_cast<CoroutineUnit*>(msg.data);
			_assert(yielded_cor.status == CoroutineUnit::kAwait);
			auto args_map = SearchForArgs();
			if (ref_running.status == CoroutineUnit::kDying) {
				yielded_cor.Resume(args_map);
			} else {
				yielded_cor.Interrupt(args_map);
			}
			break;
	}
}

void NThread::Initialize() {
	//register handler for block nthreads
	//install some observers
	Spy([](const CoroutineUnit& cor) {
		{
			//mutex for safety
			Guard::TLock lock;
			//notify scheduler that current nthread blocks
			msg_t msg = { msg_t::kNThreadsBlock, this->thread_id, &(this->currentCor) };
			this->scheduler->Dispatch(msg);
		}
	}, 1e4, [] {
		//return from syscall
		Guard::TLock lock;
		msg_t msg = { msg_t::kNThreadsSysCallReturn, this->currentCor, &(this->currentCor) };
		this->scheduler->Dispatch(msg);
	});
}

__cors_type__ NThreadScheduler::StealCors(__cors_type__ source, size_t num) {
	__cors_type__ rcors;
	FOREACH (source, [](const CoroutineUnit& cor) {
					if (source.IndexOf(cor) < num) {
						rcors.append(cor);
						source.pop(0);
					}
				})
}

NThread& NThreadScheduler::AnalysisUnbalance() {
	FOREACH (this->pool.GetAll(), [](const NThread& new_host) {
				if (new_host.president->cors >= kCorsUnbalance) {
					return new_host;
				}
			})
	return nullptr;
}

CoroutineUnit& CorScheduler::SelectOptimalCor(__cors_type__ cors) {
	Common::Sort(&cors, [](const CoroutineUnit& x, const CoroutineUnit& y) {
		return x < y;
	});
	return cors[0];
}

void NThreadScheduler::TransferContext(NThread& from, NThread& dest) {
	auto new_cors = StealCors(from.president->cors, from.president->cors.size());
	dest.appendCors(new_cors);
	//remove all the idle cors
	from.ClearIdle();
	//interrupt the current
	auto optimal_cor = CorScheduler::SelectOptimalCor(new_cors);
	dest.president->Dispatch({ msg_t::kCorsInterrupt, optimal_cor.id, &optimal_cor });
}

void NThreadScheduler::Dispatch(msg_t msg) {
	if (this->pool.size() < kNThreadPoolSize) {
		CreateNewNThread(kNThreadPoolSize - this->pool.size());
	}
	switch (msg) {
		case msg_t::kNThreadsBlock:
			//we will find another idle thread
			NThread t_idle = this->pool.Retrieve();
			NThread t_block = this->pool.Get(msg.content);
			TransferContext(t_block, t_idle);
			break;
		case msg_t::kNThreadsAsyncSysCallReturn:
			//search for globalRunQueue and steal from others
			NThread t_retrieved = this->pool.Get(msg.content);
			auto target = AnalysisUnbalance();
			if (target) {
				t_retrieved.appendCors(StealCors(target.president->cors, Common::Fix(target.president->cors.size() / 3)));
				return;
			}
			if (!globalRunQueue.IsEmpty()) {
				//just comsume 1/6 of this queue, for balance
				size_t ns = Common::Fix(globalRunQueue.size() / 6);
				t_retrieved.appendCors(StealCors(globalRunQueue, ns));
			}
			break;
		case msg_t::kNThreadsRun:
			__assert(this->pool.IsInitialized());
			this->pool.StartAllTask();	
	}

}

void NthreadPool::StartAllTask() {
	FOREACH (this->nthreads, [](const NThreads& worker) {
		worker.president->Dispatch({ msg_t::kCorsStart, 0, nullptr} );
	})
}

void* CoroutineUnit::Yield() {
	StoreContext();
	this->status = kAwait;
	//pass its args to all cors
	auto res = SpreadArgsAll(this->scheduler->priority_cors, this->arg_expr);
	return res;
}

void CoroutineUnit::Resume(__args_list_type__& args) {
	RetrieveContext();
	this->status = kRunning;
	SetArgsForRes(args);
}

void CoroutineUnit::Interrupt() {
	this->status = kCorsInterrupt;
}