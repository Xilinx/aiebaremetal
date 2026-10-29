// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef STUB_XAIEFAL_MOCK_HPP
#define STUB_XAIEFAL_MOCK_HPP

#include <memory>
#include <vector>
#include <cstdint>

extern "C" {
    #include "xaiengine.h"
}

static inline int StubXaieRc_ErrorCode(void)
{
    return -1;
}

namespace xaiefal
{

    class XAieRsc
    {
    public:
        XAieRsc()
            : m_reserveResult(0),
              m_releaseResult(0),
              m_reserveCalls(0),
              m_releaseCalls(0)
        {}
        virtual ~XAieRsc() = default;

        virtual int reserve() noexcept
        {
            ++m_reserveCalls;
            return m_reserveResult;
        }

        virtual int release() noexcept
        {
            ++m_releaseCalls;
            return m_releaseResult;
        }

        virtual int getRscId(XAie_LocType& loc, XAie_ModuleType& module, uint32_t& id) noexcept
        {
            (void)loc;
            (void)module;
            id = 0;
            return 0;
        }

        void setReserveResult(int v) { m_reserveResult = v; }
        void setReleaseResult(int v) { m_releaseResult = v; }

        int getReserveCallCount() const { return m_reserveCalls; }
        int getReleaseCallCount() const { return m_releaseCalls; }

    private:
        int m_reserveResult;
        int m_releaseResult;
        int m_reserveCalls;
        int m_releaseCalls;
    };

    class XAieSingleTileRsc : public XAieRsc
    {
    public:
        XAieSingleTileRsc()
            : m_getRscIdStatus(0),
              m_id(0),
              m_getRscIdCalls(0)
        {}
        ~XAieSingleTileRsc() override = default;

        int getRscId(XAie_LocType &loc, XAie_ModuleType &module, uint32_t &id) noexcept override
        {
            (void)loc;
            (void)module;
            ++m_getRscIdCalls;
            id = static_cast<uint32_t>(m_id);
            return m_getRscIdStatus;
        }

        void setGetRscIdResult(int status, int id)
        {
            m_getRscIdStatus = status;
            m_id = id;
        }

        int getGetRscIdCallCount() const { return m_getRscIdCalls; }

    private:
        int m_getRscIdStatus;
        int m_id;
        int m_getRscIdCalls;
    };

    class XAiePerfCounter : public XAieSingleTileRsc
    {
    public:
        XAiePerfCounter() : m_instanceIndex(s_instanceCounter++) {}
        ~XAiePerfCounter() override = default;

        int getRscId(XAie_LocType& loc, XAie_ModuleType& module, uint32_t& id) noexcept override;

        static void resetInstanceCounter() { s_instanceCounter = 0; }

    private:
        int m_instanceIndex;
        static int s_instanceCounter;
    };

    class XAieStreamPortSelect : public XAieSingleTileRsc
    {
    public:
        XAieStreamPortSelect() = default;
        ~XAieStreamPortSelect() override = default;
    };

    class XAiePCEvent : public XAieSingleTileRsc
    {
    public:
        XAiePCEvent() = default;
        ~XAiePCEvent() override = default;
    };

    class XAieBroadcast : public XAieRsc
    {
    public:
        XAieBroadcast()
            : m_bc(0),
              m_getBcCalls(0)
        {
            m_tileLocs.clear();
            m_startMod = static_cast<XAie_ModuleType>(2);
            m_endMod = static_cast<XAie_ModuleType>(2);
        }
        ~XAieBroadcast() override = default;

        int getBc() noexcept
        {
            ++m_getBcCalls;
            return m_bc;
        }

        void getChannel(std::vector<XAie_LocType>& tileLocs,
                       XAie_ModuleType& startMod,
                       XAie_ModuleType& endMod) const noexcept
        {
            tileLocs = m_tileLocs;
            startMod = m_startMod;
            endMod = m_endMod;
        }

        void setBc(int v) { m_bc = v; }
        void setChannel(const std::vector<XAie_LocType>& locs, XAie_ModuleType start, XAie_ModuleType end)
        {
            m_tileLocs = locs;
            m_startMod = start;
            m_endMod = end;
        }

    private:
        int m_bc;
        int m_getBcCalls;
        std::vector<XAie_LocType> m_tileLocs;
        XAie_ModuleType m_startMod;
        XAie_ModuleType m_endMod;
    };

    class XAieTilePl
    {
    public:
        std::shared_ptr<XAieRsc> perfCounter() noexcept
        {
            return std::make_shared<XAiePerfCounter>();
        }
    };

    class XAieTile
    {
    public:
        XAieTilePl pl() noexcept { return XAieTilePl(); }

        std::shared_ptr<XAieRsc> sswitchPort() noexcept
        {
            return std::make_shared<XAieSingleTileRsc>();
        }
    };

    class XAieDev
    {
    public:
        XAieDev() = default;
        explicit XAieDev(void* pDevInst) { (void)pDevInst; }

        XAieTile tile(int , int ) noexcept
        {
            return XAieTile();
        }

        std::shared_ptr<XAieRsc> broadcast(
            const std::vector<XAie_LocType>& ,
            XAie_ModuleType ,
            XAie_ModuleType ) noexcept
        {
            return std::make_shared<XAieBroadcast>();
        }
    };

}

static inline std::shared_ptr<xaiefal::XAieBroadcast> StubXaie_CreateBroadcast()
{
    return std::make_shared<xaiefal::XAieBroadcast>();
}

static inline void StubXaieBroadcast_SetReserveResult(std::shared_ptr<xaiefal::XAieBroadcast> const &r,
                                                      int v)
{
    r->setReserveResult(v);
}

static inline void StubXaieBroadcast_SetBcValue(std::shared_ptr<xaiefal::XAieBroadcast> const &r,
                                                int v)
{
    r->setBc(v);
}

static inline int StubXaieBroadcast_GetReserveCallCount(std::shared_ptr<xaiefal::XAieBroadcast> const &r)
{
    return r->getReserveCallCount();
}

static inline std::shared_ptr<xaiefal::XAieSingleTileRsc> StubXaie_CreateSingleTile()
{
    return std::make_shared<xaiefal::XAieSingleTileRsc>();
}

static inline void StubXaieSingleTile_SetReserveResult(std::shared_ptr<xaiefal::XAieSingleTileRsc> const &r,
                                                       int v)
{
    r->setReserveResult(v);
}

static inline void StubXaieSingleTile_SetGetRscIdResult(std::shared_ptr<xaiefal::XAieSingleTileRsc> const &r,
                                                        int status,
                                                        int id)
{
    r->setGetRscIdResult(status, id);
}

static inline int StubXaieSingleTile_GetReserveCallCount(std::shared_ptr<xaiefal::XAieSingleTileRsc> const &r)
{
    return r->getReserveCallCount();
}

static inline int StubXaieSingleTile_GetGetRscIdCallCount(std::shared_ptr<xaiefal::XAieSingleTileRsc> const &r)
{
    return r->getGetRscIdCallCount();
}

static inline std::shared_ptr<xaiefal::XAieRsc> StubXaie_CreateGenericRsc()
{
    return std::make_shared<xaiefal::XAieRsc>();
}

static inline void StubXaieRsc_SetReleaseResult(std::shared_ptr<xaiefal::XAieRsc> const &r,
                                                int v)
{
    r->setReleaseResult(v);
}

static inline int StubXaieRsc_GetReleaseCallCount(std::shared_ptr<xaiefal::XAieRsc> const &r)
{
    return r->getReleaseCallCount();
}

#endif
