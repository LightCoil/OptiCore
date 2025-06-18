#include "memory_optimizer.h"
#include "../Memory/swap_manager.h"
#include "../Memory/prefetch_manager.h"
#include "../Memory/hugepages_manager.h"
#include <vector>
#include <fstream>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

class MemoryOptimizer : public IMemoryOptimizer {
    IPlatformAdapter* adapter = nullptr;
    std::string status = "init";
    swap_manager_t swap_mgr{};
    prefetch_manager_t prefetch_mgr{};
    hugepages_manager_t huge_mgr{};
public:
    bool Init(IPlatformAdapter* a) override {
        adapter = a;
        swap_manager_init(&swap_mgr, 128, "/tmp/opticore.swap");
        prefetch_manager_init(&prefetch_mgr, PREFETCH_LOOKAHEAD, 4);
        prefetch_manager_set_load_cb(&prefetch_mgr, [](uint64_t block_id){
            void* data = nullptr;
            swap_manager_load_block(&swap_mgr, block_id, &data);
        });
        hugepages_manager_init(&huge_mgr, 2*1024*1024, 8); // 2MB pages, 8 штук
        status = "initialized";
        return true;
    }
    void Start() override {
        status = "running";
    }
    void Stop() override {
        status = "stopped";
    }
    void SetSwappiness(int value) override {
        if (adapter) adapter->SetSwappiness(value);
        std::ofstream ofs("/proc/sys/vm/swappiness");
        if (ofs) ofs << value;
        status = "swappiness=" + std::to_string(value);
    }
    void DropCaches() override {
        if (adapter) adapter->DropCaches();
        std::ofstream ofs("/proc/sys/vm/drop_caches");
        if (ofs) ofs << "3";
        status = "caches dropped";
    }
    void EnableHugePages(bool enable) override {
        if (adapter) adapter->EnableHugePages(enable);
        std::ofstream ofs("/proc/sys/vm/nr_hugepages");
        if (ofs) ofs << (enable ? "8" : "0");
        status = enable ? "hugepages enabled" : "hugepages disabled";
    }
    bool SwapLoadBlock(uint64_t block_id, void** out_data) override {
        return swap_manager_load_block(&swap_mgr, block_id, out_data);
    }
    bool SwapWritebackBlock(uint64_t block_id) override {
        return swap_manager_writeback_block(&swap_mgr, block_id);
    }
    void SwapEvictLRU() override {
        swap_manager_evict_lru(&swap_mgr);
    }
    void PrefetchBlocks(const std::vector<uint64_t>& block_ids) override {
        prefetch_manager_prefetch(&prefetch_mgr, block_ids.empty() ? 0 : block_ids[0]);
    }
    size_t QueryHugePagesUsage() override {
        return huge_mgr.allocated_pages;
    }
    std::string QueryStatus() override {
        return status;
    }
    ~MemoryOptimizer() {
        swap_manager_destroy(&swap_mgr);
        prefetch_manager_destroy(&prefetch_mgr);
        hugepages_manager_destroy(&huge_mgr);
    }
}; 