#pragma once
namespace storage {
// Optional deterministic I/O failure points for host regressions. Normal callers
// use None; these abort writes without claiming to simulate physical power loss.
enum class SaveFault { None, Write, Flush, Sync, Close, Backup, Commit };
}
