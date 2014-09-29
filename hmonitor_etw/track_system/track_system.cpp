#include "stdafx.h"
#include "config.h"
#include "snapshot.h"
#include "track_system.h"
#include "utility/lock_helper.h"


const size_t MAX_STACK_DEPTH = 192;

struct IIRAStackManager
{
	virtual const IRA_SymS* newStack(
		size_t depth,
		const IRA* frames,
		size_t &imgIdx
		) = 0;
};


typedef TSS_Stack CStack;

typedef TSS_Image CImage;
struct ImageBaseCmp
{
	bool operator()(const CImage* l, tst_pointer r) const
	{
		return l->id < r;
	}
	bool operator()(const CImage* l, const CImage* r) const
	{
		return l->id < r->id;
	}
};

typedef TSS_Heap CHeap;

typedef TSS_Thread CThread;


class CProcess : public StatBase<ProcessStat>
{
public:
	CProcess()
		: m_stackManager()
		, m_tsStart()
		, m_tsEnd()
		, m_pid()
		, m_ppid()
		, m_code()
	{
		// reserve
		m_imageIndex.reserve(200);
	}
	~CProcess()
	{
	}

	void init(IIRAStackManager* stackManager)
	{
		m_stackManager = stackManager;
	}

public:
	IRA addr2IRA(tst_pointer addr) const
	{
		ImageArray::const_iterator itor = std::lower_bound(
			m_imageIndex.begin(), m_imageIndex.end(), addr, ImageBaseCmp());

		// Equal
		if (itor != m_imageIndex.end())
		{
			const CImage* image = *itor;
			if (image->id == addr)
			{
				IRA ira;
				ira.index = (uint32_t)image->index;
				ira.offset = 0;
				ira.valid = 1;
				return ira;
			}
		}

		if (itor != m_imageIndex.begin())
		{
			--itor;
			const CImage* image = *itor;
			ASSERT (image->id < addr);
			if (image->id + image->size > addr)
			{
				IRA ira;
				ira.index = (uint32_t)image->index;
				ira.offset = (uint32_t)(addr - image->id);
				ira.valid = 1;
				return ira;
			}
		}

		IRA ira;
		ira.u64 = addr;
		ira.valid = 0;
		return ira;
	}

	const IRA_SymS* appendStack(STACKINFO si, CStack** ppStack = NULL)
	{
		if (ppStack != NULL)
			*ppStack = NULL;

		if (m_stackManager == NULL)
			return NULL;

		if (si.stackid == tst_stackid() || si.depth == 0 || si.frames == NULL)
			return NULL;

		CStack &stack = m_stacks[si.stackid];
		if (stack.id == 0)
		{
			IRA frames[MAX_STACK_DEPTH];
			for (size_t i = 0; i < si.depth && i < MAX_STACK_DEPTH; ++i)
				frames[i] = addr2IRA(si.frames[i]);
			stack.id = si.stackid;
			size_t imgIdx = 0;
			stack.ira_syms = m_stackManager->newStack(si.depth, frames, imgIdx);
			CImage* img = imageByIndex(imgIdx);
			if (img != NULL)
			{
				stack.imgid = img->id;
			}
		}

		if (ppStack != NULL)
			*ppStack = &stack;
		return stack.ira_syms;
	}

	CImage* image(tst_imgid imgid)
	{
		ImageMap::iterator itor = m_images.find(imgid);
		if (itor != m_images.end())
		{
			return &(itor->second);
		}
		return NULL;
	}
	CImage* imageByIndex(size_t index)
	{
		for (ImageArray::iterator itor = m_imageIndex.begin();
			itor < m_imageIndex.end(); ++itor)
		{
			const CImage* img = *itor;
			if (img->index == index)
			{
				return (CImage*)img;
			}
		}
		return NULL;
	}
	CHeap* heap(tst_heapid heapid)
	{
		HeapMap::iterator itor = m_heaps.find(heapid);
		if (itor != m_heaps.end())
		{
			return &(itor->second);
		}
		return NULL;
	}
	CThread* thread(tst_tid tid)
	{
		ThreadMap::iterator itor = m_threads.find(tid);
		if (itor != m_threads.end())
		{
			return &(itor->second);
		}
		return NULL;
	}

public:
	void OnStart(
		tst_time tsStart,
		tst_pid pid,
		tst_pid ppid,
		const char* name
		)
	{
		m_tsStart = tsStart;
		m_pid = pid;
		m_ppid = ppid;
		m_name = name;
	}
	void OnEnd(
		tst_time tsEnd,
		tst_exit code
		)
	{
		m_tsEnd = tsEnd;
		m_code = code;
	}

	void OnImageLoad(
		tst_time tsLoad,
		tst_pointer base,
		tst_imgsz size,
		size_t index,
		STACKINFO siLoad
		)
	{
		if (m_images.find(base) != m_images.end())
		{
			OnImageUnload(tsLoad, base);
		}

		CImage &image = m_images[base];
		image.tsLoad = tsLoad;
		image.id = base;
		image.size = size;
		image.index = index;
		image.stackLoad = appendStack(siLoad);

		ImageArray::iterator itor = std::lower_bound(
			m_imageIndex.begin(), m_imageIndex.end(), base, ImageBaseCmp());

		m_imageIndex.insert(itor, &image);
	}
	void OnImageUnload(
		tst_time tsUnload,
		tst_pointer base
		)
	{
		ImageArray::iterator itorIdx = std::lower_bound(
			m_imageIndex.begin(), m_imageIndex.end(), base, ImageBaseCmp());

		if (itorIdx != m_imageIndex.end())
		{
			const CImage* image = *itorIdx;
			if (image->id == base)
			{
				m_imageIndex.erase(itorIdx);
			}
		}

		ImageMap::iterator itor = m_images.find(base);
		if (itor != m_images.end())
		{
			itor->second.tsUnload = tsUnload;
			// ohh
			m_images.erase(itor);
		}
	}

	void OnHeapCreate(
		tst_time tsCreate,
		tst_heapid heapid,
		STACKINFO siCreate
		)
	{
		if (m_heaps.find(heapid) != m_heaps.end())
		{
			OnHeapDestroy(tsCreate, heapid);
		}

		CHeap &heap = m_heaps[heapid];
		heap.tsCreate = tsCreate;
		heap.id = heapid;
		heap.stackCreate = appendStack(siCreate);
	}
	void OnHeapDestroy(
		tst_time tsDestroy,
		tst_heapid heapid
		)
	{
		HeapMap::iterator itor = m_heaps.find(heapid);
		if (itor != m_heaps.end())
		{
			itor->second.tsDestroy = tsDestroy;
			// ohh
			m_heaps.erase(itor);
		}
	}
	void OnHeapSpaceChange(
		tst_heapid heapid,
		bool fExpand,
		tst_ptdiffer size,
		unsigned noOfUCRs,
		tst_ptdiffer ss_committed
		)
	{
		HeapMap::iterator itor = m_heaps.find(heapid);
		if (itor != m_heaps.end())
		{
			CHeap &heap = itor->second;
			if (fExpand)
				heap.committed += size;
			else
			{
				if (heap.committed > size)
					heap.committed -= size;
				else
					heap.committed = 4096; // minimum
			}
			heap.noOfUCRs = noOfUCRs;
			heap.ss_committed = ss_committed;
		}
	}

	void OnThreadStart(
		tst_time tsStart,
		tst_tid tid,
		tst_pointer userStack,
		tst_pointer entry,
		STACKINFO siStart
		)
	{
		if (m_threads.find(tid) != m_threads.end())
		{
			OnThreadEnd(tsStart, tid);
		}

		CThread &thread = m_threads[tid];
		IRA iraEntry = addr2IRA(entry);
		thread.tsStart = tsStart;
		thread.id  = tid;
		thread.userStack = userStack;
		thread.entry.ira = addr2IRA(entry);
		thread.stackStart = appendStack(siStart);
	}
	void OnThreadEnd(
		tst_time tsEnd,
		tst_tid tid
		)
	{
		ThreadMap::iterator itor = m_threads.find(tid);
		if (itor != m_threads.end())
		{
			itor->second.tsEnd = tsEnd;
			// ooh
			m_threads.erase(itor);
		}
	}

	void OnHeapAlloc(
		tst_time tsAlloc,
		tst_heapid heapid,
		tst_pointer addr,
		tst_ptdiffer size,
		tst_tid tid,
		STACKINFO siAlloc
		)
	{
		AllocItem &item = m_allocItems[addr];
		item.tsAlloc = tsAlloc;
		item.heapid = heapid;
		item.size = size;
		item.tid = tid;
		item.stackAlloc = appendStack(siAlloc, &(item.cacheStackAlloc));
		if (item.cacheStackAlloc != NULL)
		{
			item.cacheStackAlloc->heapid = heapid;
		}

		// Stat
		{
			this->StatBy<AllocCountIdx>().Increase(1);
			this->StatBy<AllocBytesIdx>().Increase(size);

			CHeap* h = heap(heapid);
			if (h != NULL)
			{
				h->StatBy<AllocCountIdx>().Increase(1);
				h->StatBy<AllocBytesIdx>().Increase(size);
			}

			CThread* t = thread(tid);
			if (t != NULL)
			{
				t->StatBy<AllocCountIdx>().Increase(1);
				t->StatBy<AllocBytesIdx>().Increase(size);
			}

			CStack* s = item.cacheStackAlloc;
			if (s != NULL)
			{
				s->StatBy<AllocCountIdx>().Increase(1);
				s->StatBy<AllocBytesIdx>().Increase(size);

				CImage* i = image(s->imgid);
				if (i != NULL)
				{
					i->StatBy<AllocCountIdx>().Increase(1);
					i->StatBy<AllocBytesIdx>().Increase(size);
				}
			}
		}
	}
	void OnHeapRealloc(
		tst_time tsRealloc,
		tst_heapid heapid,
		tst_pointer newAddr,
		tst_ptdiffer newSize,
		tst_pointer oldAddr,
		tst_ptdiffer oldSize,
		tst_tid tid,
		STACKINFO siRealloc
		)
	{
		OnHeapFree(tsRealloc, heapid, oldAddr);
		OnHeapAlloc(tsRealloc, heapid, newAddr, newSize, tid, siRealloc);
	}
	void OnHeapFree(
		tst_time tsFree,
		tst_heapid heapid,
		tst_pointer addr
		)
	{
		tst_time tsAlloc = tst_time();
		tst_ptdiffer size = tst_ptdiffer();
		tst_tid tid = tst_tid();
		CStack* s = NULL;

		AllocItemMap::iterator itor = m_allocItems.find(addr);
		if (itor != m_allocItems.end())
		{
			tsAlloc = itor->second.tsAlloc;
			ASSERT (heapid = itor->second.heapid);
			size = itor->second.size;
			tid = itor->second.tid;
			s = itor->second.cacheStackAlloc;
			m_allocItems.erase(addr);
		}
		else
		{
			// Log
			// printf("%x ", addr);
		}

		// Stat
		{
			this->StatBy<AllocCountIdx>().Decrease(1);
			this->StatBy<AllocBytesIdx>().Decrease(size);

			CHeap* h = heap(heapid);
			if (h != NULL && tsAlloc >= h->tsCreate)
			{
				h->StatBy<AllocCountIdx>().Decrease(1);
				h->StatBy<AllocBytesIdx>().Decrease(size);
			}

			CThread* t = thread(tid);
			if (t != NULL && tsAlloc >= t->tsStart)
			{
				t->StatBy<AllocCountIdx>().Decrease(1);
				t->StatBy<AllocBytesIdx>().Decrease(size);
			}

			if (s != NULL)
			{
				s->StatBy<AllocCountIdx>().Decrease(1);
				s->StatBy<AllocBytesIdx>().Decrease(size);

				CImage* i = image(s->imgid);
				if (i != NULL)
				{
					i->StatBy<AllocCountIdx>().Decrease(1);
					i->StatBy<AllocBytesIdx>().Decrease(size);
				}
			}
		}
	}

public:
	void Snapshot(TSS_Process &p) const
	{
		(StatBase<ProcessStat>&)p = *this;

		p.tsStart = m_tsStart;
		p.tsEnd = m_tsEnd;
		p.id = m_pid;
		p.ppid = m_ppid;
		p.code = m_code;
		p.name = m_name;

		p.stacks.clear();
		p.stacks.reserve(m_stacks.size());
		for (StackMap::const_iterator itor = m_stacks.begin();
			itor != m_stacks.end(); ++itor)
		{
			p.stacks.push_back(itor->second);
		}

		p.images.clear();
		p.images.reserve(m_images.size());
		for (ImageMap::const_iterator itor = m_images.begin();
			itor != m_images.end(); ++itor)
		{
			p.images.push_back(itor->second);
		}

		p.heaps.clear();
		p.heaps.reserve(m_heaps.size());
		for (HeapMap::const_iterator itor = m_heaps.begin();
			itor != m_heaps.end(); ++itor)
		{
			p.heaps.push_back(itor->second);
		}

		p.threads.clear();
		p.threads.reserve(m_threads.size());
		for (ThreadMap::const_iterator itor = m_threads.begin();
			itor != m_threads.end(); ++itor)
		{
			p.threads.push_back(itor->second);
		}
	}

protected:
	IIRAStackManager* m_stackManager;
	tst_time m_tsStart;
	tst_time m_tsEnd;
	tst_pid m_pid;
	tst_pid m_ppid;
	tst_exit m_code;
	std::string m_name;

	// stacks
	typedef std::map<tst_stackid, CStack> StackMap;
	StackMap m_stacks;

	// images
	typedef std::map<tst_imgid, CImage> ImageMap;
	ImageMap m_images;

	// image index
	// sort by p->base, ascending order
	typedef std::vector<const CImage*> ImageArray;
	ImageArray m_imageIndex;

	// heaps
	typedef std::map<tst_heapid, CHeap> HeapMap;
	HeapMap m_heaps;

	// threads
	typedef std::map<tst_tid, CThread> ThreadMap;
	ThreadMap m_threads;

	struct AllocItem
	{
		tst_time tsAlloc;
		tst_heapid heapid;
		tst_ptdiffer size;
		tst_tid tid;
		const IRA_SymS* stackAlloc;
		CStack* cacheStackAlloc;
	};

	typedef std::map<tst_pointer, AllocItem> AllocItemMap;
	AllocItemMap m_allocItems;
};

class CTrackSystem : public ITrackSystem, public IIRAStackManager
{
public:
	CTrackSystem()
		: m_tsLast()
	{
	}

public:
	void SetFocus(const char* name)
	{
		m_focus.push_back(std::string(name));
	}

	void SetImgLevel(const char* name, size_t level)
	{
		Img_NameLevel nl;
		nl.name = "\\";
		nl.name += name;
		nl.level = level;
		m_imgLevelByName.push_back(nl);
	}

	void TrackBegin()
	{
		m_lock.lock();
	}
	void TrackEnd(tst_time tsLast)
	{
		m_tsLast = tsLast;
		m_lock.unlock();
	}

	void OnProcessStart(
		tst_time tsStart,
		tst_pid pid,
		tst_pid ppid,
		const char* name
		)
	{
		if (m_processes.find(pid) != m_processes.end())
		{
			#define PROCESS_EXIT_FAKE  8720921
			OnProcessEnd(tsStart, pid, PROCESS_EXIT_FAKE);
		}

		if (!m_focus.empty())
		{
			bool bFocus = false;
			for (std::list<std::string>::iterator itor = m_focus.begin();
				itor != m_focus.end(); ++itor)
			{
				if (_stricmp(name, (*itor).c_str()) == 0)
				{
					bFocus = true;
					break;
				}
			}
			if (!bFocus)
			{
				return;
			}
		}

		CProcess &process = m_processes[pid];
		process.init(this);
		process.OnStart(tsStart, pid, ppid, name);
	}
	void OnProcessEnd(
		tst_time tsEnd,
		tst_pid pid,
		tst_exit code
		)
	{
		ProcessMap::iterator itor = m_processes.find(pid);
		if (itor != m_processes.end())
		{
			itor->second.OnEnd(tsEnd, code);
			m_processes.erase(itor);
		}
	}

	void OnImageLoad(
		tst_time tsLoad,
		tst_pid pid,
		tst_pointer base,
		tst_imgsz size,
		const char* name,
		STACKINFO siLoad
		)
	{
		ProcessMap::iterator itor = m_processes.find(pid);
		if (itor != m_processes.end())
		{
			size_t e = m_nimImg.end();
			size_t i = m_nimImg.append(name);
			if (i >= e)
			{
				m_imgLevelByIndex[i] = imgLevelByName(name);
			}

			itor->second.OnImageLoad(
				tsLoad, base, size, i, siLoad);
		}
	}
	void OnImageUnload(
		tst_time tsUnload,
		tst_pid pid,
		tst_pointer base
		)
	{
		ProcessMap::iterator itor = m_processes.find(pid);
		if (itor != m_processes.end())
		{
			itor->second.OnImageUnload(tsUnload, base);
		}
	}

	void OnHeapCreate(
		tst_time tsCreate,
		tst_pid pid,
		tst_heapid heapid,
		STACKINFO siCreate
		)
	{
		ProcessMap::iterator itor = m_processes.find(pid);
		if (itor != m_processes.end())
		{
			itor->second.OnHeapCreate(
				tsCreate, heapid, siCreate);
		}
	}
	void OnHeapDestroy(
		tst_time tsDestroy,
		tst_pid pid,
		tst_heapid heapid
		)
	{
		ProcessMap::iterator itor = m_processes.find(pid);
		if (itor != m_processes.end())
		{
			itor->second.OnHeapDestroy(tsDestroy, heapid);
		}
	}
	void OnHeapSpaceChange(
		tst_pid pid,
		tst_heapid heapid,
		bool fExpand,
		tst_ptdiffer size,
		unsigned noOfUCRs,
		tst_ptdiffer ss_committed
		)
	{
		ProcessMap::iterator itor = m_processes.find(pid);
		if (itor != m_processes.end())
		{
			itor->second.OnHeapSpaceChange(heapid, fExpand, size, noOfUCRs, ss_committed);
		}
	}

	void OnThreadStart(
		tst_time tsStart,
		tst_pid pid,
		tst_tid tid,
		tst_pointer userStack,
		tst_pointer entry,
		STACKINFO siStart
		)
	{
		ProcessMap::iterator itor = m_processes.find(pid);
		if (itor != m_processes.end())
		{
			itor->second.OnThreadStart(
				tsStart, tid, userStack, entry, siStart);
		}
	}
	void OnThreadEnd(
		tst_time tsEnd,
		tst_pid pid,
		tst_tid tid
		)
	{
		ProcessMap::iterator itor = m_processes.find(pid);
		if (itor != m_processes.end())
		{
			itor->second.OnThreadEnd(tsEnd, tid);
		}
	}

	void OnHeapAlloc(
		tst_time tsAlloc,
		tst_pid pid,
		tst_heapid heapid,
		tst_pointer addr,
		tst_ptdiffer size,
		tst_tid tid,
		STACKINFO siAlloc
		)
	{
		ProcessMap::iterator itor = m_processes.find(pid);
		if (itor != m_processes.end())
		{
			itor->second.OnHeapAlloc(
				tsAlloc, heapid, addr, size, tid, siAlloc);
		}
	}
	void OnHeapRealloc(
		tst_time tsRealloc,
		tst_pid pid,
		tst_heapid heapid,
		tst_pointer newAddr,
		tst_ptdiffer newSize,
		tst_pointer oldAddr,
		tst_ptdiffer oldSize,
		tst_tid tid,
		STACKINFO siRealloc
		)
	{
		ProcessMap::iterator itor = m_processes.find(pid);
		if (itor != m_processes.end())
		{
			itor->second.OnHeapRealloc(
				tsRealloc, heapid, newAddr, newSize,
				oldAddr, oldSize, tid, siRealloc);
		}
	}
	void OnHeapFree(
		tst_time tsFree,
		tst_pid pid,
		tst_heapid heapid,
		tst_pointer addr
		)
	{
		ProcessMap::iterator itor = m_processes.find(pid);
		if (itor != m_processes.end())
		{
			itor->second.OnHeapFree(tsFree, heapid, addr);
		}
	}

	void Snapshot(
		TSS_System &sys
		)
	{
		AutoLock _alock(&m_lock);

		sys.processes.resize(m_processes.size());

		ProcessMap::const_iterator itor = m_processes.begin();
		for (size_t i = 0; itor != m_processes.end(); ++itor, ++i)
		{
			itor->second.Snapshot(sys.processes[i]);
		}

		sys.nimImg = m_nimImg;

		sys.tsLast = m_tsLast;
	}

	const IRA_SymS* newStack(
		size_t depth,
		const IRA* frames,
		size_t &imgIdx
		)
	{
		ASSERT(depth != 0 && frames != NULL);

		tst_stackid stackid = 0;
		{
			union
			{
				struct {
				uint64_t sum : 56;
				uint64_t cnt : 8;
				};
				uint64_t u64;
			} id; id.u64 = 0;
			for (size_t i = 0; i < depth; ++i)
				id.sum += frames[i].u64;
			id.cnt = depth;
			stackid = id.u64;
		}

		StackInfo &s = m_stacks[stackid];
		if (s.ira_syms.empty())
		{
			bool fSet = false;
			size_t index = 0, level = 0;

			s.ira_syms.resize(depth);
			for (size_t i = 0; i < depth; ++i)
			{
				IRA ira = frames[i];
				s.ira_syms[i].ira = ira;

				if (!fSet)
				{
					Index2LevelMap::iterator itor =
						m_imgLevelByIndex.find(ira.index);
					if (itor != m_imgLevelByIndex.end())
					{
						if (itor->second == MaxImgLevel)
						{
							fSet = true;
							s.imgIdx = itor->first;
						}
						else
						{
							if (itor->second > level)
							{
								index = itor->first;
								level = itor->second;
							}
						}
					}
				}
			}

			if (!fSet)
			{
				s.imgIdx = index;
			}
		}

		imgIdx = s.imgIdx;
		return &(s.ira_syms);
	}

protected:
	static const size_t MaxImgLevel = -1;

	size_t imgLevelByName(const char* name) const
	{
		size_t l = strlen(name);
		for (std::list<Img_NameLevel>::const_iterator itor = m_imgLevelByName.begin();
			itor != m_imgLevelByName.end(); ++itor)
		{
			const std::string &s = itor->name;
			if (l > s.size() &&
				_strcmpi(name + l - s.size(), s.c_str()) == 0)
			{
				return itor->level;
			}
		}
		return MaxImgLevel;
	}

protected:
	std::list<std::string> m_focus;

	struct Img_NameLevel
	{
		std::string name;
		size_t level;
	};
	std::list<Img_NameLevel> m_imgLevelByName;

	KCriticalSection m_lock;
	typedef KAutoLock<KCriticalSection> AutoLock;

	// processes
	typedef std::map<tst_pid, CProcess> ProcessMap;
	ProcessMap m_processes;

	// all image list
	// image index is based on this list
	jtwsm::nameidx_map<char> m_nimImg;

	typedef std::map<size_t, size_t> Index2LevelMap;
	Index2LevelMap m_imgLevelByIndex;

	tst_time m_tsLast;

	struct StackInfo
	{
		IRA_SymS ira_syms;
		size_t imgIdx;
	};
	typedef std::map<tst_stackid, StackInfo> StackMap;
	StackMap m_stacks;
};


CTrackSystem g_TrackSystem;

ITrackSystem* getTrackSystem()
{
	return &g_TrackSystem;
}
