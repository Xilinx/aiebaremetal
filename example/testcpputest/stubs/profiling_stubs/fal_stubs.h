#include "stubs.h"
#include <string>
#include <memory>
#include <atomic>
// Use mock XAieFAL classes for unit testing
#include "xaiefal_mock.hpp"
#include "aeg_api_message.h"  // For abr::err_code definition

// Note: errorMsg and debugMsg declarations are now provided by the production
// aeg_api_message.h header, so we don't redeclare them here.

namespace abr {
// The err_code enum is defined in the production header

namespace config_manager {
// Device instance pointer used by profiling API
extern void* s_pDevInst;
} // namespace config_manager

} // namespace abr

namespace fal_util {

class TilePlStub
{
public:
    std::shared_ptr<xaiefal::XAieRsc> perfCounter()
    {
        return std::static_pointer_cast<xaiefal::XAieRsc>(
            std::make_shared<xaiefal::XAiePerfCounter>());
    }
};

class TileStub
{
public:
    TilePlStub pl() { return TilePlStub(); }

    std::shared_ptr<xaiefal::XAieRsc> sswitchPort()
    {
        return std::static_pointer_cast<xaiefal::XAieRsc>(
            std::make_shared<xaiefal::XAieSingleTileRsc>());
    }
};

class DevWrapper
{
public:
    TileStub tile(int /*col*/, int /*row*/)
    {
        return TileStub();
    }

    // Broadcast helper used by profiling code
    std::shared_ptr<xaiefal::XAieRsc> broadcast(
        const std::vector<XAie_LocType>& locs,
        XAie_ModuleType startMod,
        XAie_ModuleType endMod)
    {
        auto bc = std::make_shared<xaiefal::XAieBroadcast>();
        bc->setChannel(locs, startMod, endMod);
        return std::static_pointer_cast<xaiefal::XAieRsc>(bc);
    }
};

extern DevWrapper* s_pXAieDev;

int  request(const std::shared_ptr<xaiefal::XAieRsc>& rsc);
bool release(const std::shared_ptr<xaiefal::XAieRsc>& rsc);
void setReleaseResult(bool result);

// Helper used by stub_controls to configure request() behaviour
void setRequestResults(const std::vector<int>& results);

} // namespace fal_util

namespace stub_controls {

// Struct definitions for stub configurations
struct PerfCounterCfg {
    bool            valid;
    XAie_LocType    loc;
    XAie_ModuleType module;
    u32             id;
    XAie_ReturnCode status;
};

struct PerfGetCfg {
    bool            valid;
    u8              col;
    u8              row;
    XAie_ModuleType module;
    u8              id;
    u32             value;
    XAie_ReturnCode status;
};

// Exported globals
extern PerfCounterCfg g_perfCfg[16];
extern int g_perfCfgUsed;
extern PerfGetCfg g_perfGetCfg[16];
extern std::atomic<int> g_perfGetCallCount;

void ResetAll();

void FalUtil_SetRequestResults(const std::vector<int>& results);
int  FalUtil_GetRequestCallCount();
void FalUtil_SetReleaseResult(bool result);

void XAie_SetEventSelectResult(int rc);
void XAie_SetPerfCounterControlResult(int rc);
int  XAie_GetEventSelectCallCount();
int  XAie_GetPerfCounterControlCallCount();

abr::err_code ErrorMsg_GetLastCode();
const char*   ErrorMsg_GetLastMessage();
void          ErrorMsg_Reset();

// Controls for XAiePerfCounter::getRscId() semantics
void Stub_XAiePerfCounter_Reset(void);
void Stub_XAiePerfCounter_SetGetRscIdResult(
    int            index,
    u8             col,
    u8             row,
    int            module,
    u32            id,
    XAie_ReturnCode status);

// Controls for XAie_PerfCounterGet() semantics
void Stub_XAie_PerfCounterGet_Reset(void);
void Stub_XAie_PerfCounterGet_SetValue(
    u8              col,
    u8              row,
    XAie_ModuleType module,
    u8              id,
    u32             value,
    XAie_ReturnCode status);
int  Stub_XAie_PerfCounterGet_GetCallCount(void);

// Stub device instance used by profiling API
extern XAie_DevInst g_stubDevInst;

} // namespace stub_controls