#include "CppUTestExt/MockSupport_c.h"

#include <xaiengine/xaiegbl.h>
#include <xaiengine/xaie_events.h>
#include <string.h>
#include <stdbool.h>


AieRC XAie_CoreReadDoneBit(XAie_DevInst *DevInst, XAie_LocType Loc, u8 *DoneBit) {
	return (AieRC) mock_c()->actualCall("XAie_CoreReadDoneBit")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withOutputParameter("DoneBit", DoneBit)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_DmaGetPendingBdCount(XAie_DevInst *DevInst, XAie_LocType Loc, u8 ChNum, XAie_DmaDirection Dir, u8 *PendingBd) {
	return (AieRC) mock_c()->actualCall("XAie_DmaGetPendingBdCount")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withUnsignedIntParameters("ChNum", ChNum)
		->withIntParameters("Dir", Dir)
		->withOutputParameterOfType("u8", "PendingBd", PendingBd)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_DmaGetMaxQueueSize(XAie_DevInst *DevInst, XAie_LocType Loc, u8 *QueueSize) {
	return (AieRC) mock_c()->actualCall("XAie_DmaGetMaxQueueSize")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withOutputParameterOfType("u8", "QueueSize", QueueSize)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_SetupPartitionConfig(XAie_DevInst *DevInst, u64 PartBaseAddr, u8 PartStartCol, u8 PartNumCols) {
	AieRC rc = (AieRC) mock_c()->actualCall("XAie_SetupPartitionConfig")
		->withPointerParameters("DevInst", DevInst)
		->withUnsignedLongLongIntParameters("PartBaseAddr", PartBaseAddr)
		->withUnsignedIntParameters("PartStartCol", PartStartCol)
		->withUnsignedIntParameters("PartNumCols", PartNumCols)
		->returnIntValueOrDefault(XAIE_OK);
	/* Real driver fills DevInst; mock must mirror it. When driver blob has partition_num_cols==0,
	 * still set a minimal valid range so clock APIs and BacktrackErrors() see NumCols>0. */
	if (DevInst != NULL && rc == XAIE_OK) {
		if (PartNumCols != 0U) {
			DevInst->StartCol = PartStartCol;
			DevInst->NumCols = PartNumCols;
		} else {
			DevInst->StartCol = PartStartCol;
			DevInst->NumCols = 1U;
		}
	}
	return rc;
}

AieRC XAie_CfgInitialize(XAie_DevInst *InstPtr, XAie_Config *ConfigPtr) {
	return (AieRC) mock_c()->actualCall("XAie_CfgInitialize")
		->withPointerParameters("InstPtr", InstPtr)
		->withPointerParameters("ConfigPtr", ConfigPtr)
		->returnIntValueOrDefault(XAIE_OK);
}

u8 XAie_GetTileTypefromLoc(XAie_DevInst *DevInst, XAie_LocType Loc) {
	return (u8) mock_c()->actualCall("XAie_GetTileTypefromLoc")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->returnIntValueOrDefault(0);
}

AieRC XAie_IsDeviceCheckerboard(XAie_DevInst *DevInst, u8 *IsCheckerBoard) {
	return (AieRC) mock_c()->actualCall("XAie_IsDeviceCheckerboard")
		->withPointerParameters("DevInst", DevInst)
		->withOutputParameter("IsCheckerBoard", IsCheckerBoard)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_CheckModule(XAie_DevInst *DevInst, XAie_LocType Loc, XAie_ModuleType Module) {
	return (AieRC) mock_c()->actualCall("XAie_CheckModule")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withIntParameters("Module", Module)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_PerfCounterGet(XAie_DevInst *DevInst, XAie_LocType Loc, XAie_ModuleType Module, u8 Counter, u32 *CounterVal) {
	return (AieRC) mock_c()->actualCall("XAie_PerfCounterGet")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withIntParameters("Module", Module)
		->withUnsignedIntParameters("Counter", Counter)
		->withOutputParameter("CounterVal", CounterVal)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_ReadTimer(XAie_DevInst *DevInst, XAie_LocType Loc, XAie_ModuleType Module, u64 *TimerVal) {
	return (AieRC) mock_c()->actualCall("XAie_ReadTimer")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withIntParameters("Module", Module)
		->withOutputParameter("TimerVal", TimerVal)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_StartTransaction(XAie_DevInst *DevInst, u32 Flags) {
	return (AieRC) mock_c()->actualCall("XAie_StartTransaction")
		->withPointerParameters("DevInst", DevInst)
		->withIntParameters("Flags", Flags)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_ClearCoreDisableEventOccurred(XAie_DevInst *DevInst, XAie_LocType Loc) {
	return (AieRC) mock_c()->actualCall("XAie_ClearCoreDisableEventOccurred")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_CoreConfigureEnableEvent(XAie_DevInst *DevInst, XAie_LocType Loc, XAie_Events Event) {
	return (AieRC) mock_c()->actualCall("XAie_CoreConfigureEnableEvent")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withIntParameters("Event", Event)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_SubmitTransaction(XAie_DevInst *DevInst, XAie_TxnInst *TxnInst) {
	return (AieRC) mock_c()->actualCall("XAie_SubmitTransaction")
		->withPointerParameters("DevInst", DevInst)
		->withPointerParameters("TxnInst", TxnInst)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_EventGenerate(XAie_DevInst *DevInst, XAie_LocType Loc, XAie_ModuleType Module, u32 Event) {
	return (AieRC) mock_c()->actualCall("XAie_EventGenerate")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withIntParameters("Module", Module)
		->withUnsignedIntParameters("Event", Event)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_CoreEnable(XAie_DevInst *DevInst, XAie_LocType Loc) {
	return (AieRC) mock_c()->actualCall("XAie_CoreEnable")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_DataMemWrWord(XAie_DevInst *DevInst, XAie_LocType Loc, u32 Addr, u32 Data) {
	return (AieRC) mock_c()->actualCall("XAie_DataMemWrWord")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withUnsignedIntParameters("Addr", Addr)
		->withUnsignedIntParameters("Data", Data)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_DmaDescInit(XAie_DevInst *DevInst, XAie_DmaDesc *DmaDesc, XAie_LocType Loc) {
	return (AieRC) mock_c()->actualCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", DevInst)
		->withPointerParameters("DmaDesc", DmaDesc)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_DmaSetMultiDimAddr(XAie_DmaDesc *DmaDesc, XAie_DmaTensor *Tensor, u64 Addr, u32 Len) {
	return (AieRC) mock_c()->actualCall("XAie_DmaSetMultiDimAddr")
		->withPointerParameters("DmaDesc", DmaDesc)
		->withPointerParameters("Tensor", Tensor)
		->withUnsignedLongLongIntParameters("Addr", Addr)
		->withUnsignedIntParameters("Len", Len)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_DmaSetZeroPadding(XAie_DmaDesc *DmaDesc, u8 Dim, XAie_DmaZeroPaddingPos Pos, u8 NumZeros) {
	return (AieRC) mock_c()->actualCall("XAie_DmaSetZeroPadding")
		->withPointerParameters("DmaDesc", DmaDesc)
		->withUnsignedIntParameters("Dim", Dim)
		->withIntParameters("Pos", Pos)
		->withUnsignedIntParameters("NumZeros", NumZeros)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_DmaSetPkt(XAie_DmaDesc *DmaDesc, XAie_Packet Pkt) {
	return (AieRC) mock_c()->actualCall("XAie_DmaSetPkt")
		->withPointerParameters("DmaDesc", DmaDesc)
		->withParameterOfType("XAie_Packet", "Pkt", &Pkt)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_DmaSetOutofOrderBdId(XAie_DmaDesc *DmaDesc, u8 OutofOrderBdId) {
	return (AieRC) mock_c()->actualCall("XAie_DmaSetOutofOrderBdId")
		->withPointerParameters("DmaDesc", DmaDesc)
		->withUnsignedIntParameters("OutofOrderBdId", OutofOrderBdId)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_DmaEnableCompression(XAie_DmaDesc *DmaDesc) {
	return (AieRC) mock_c()->actualCall("XAie_DmaEnableCompression")
		->withPointerParameters("DmaDesc", DmaDesc)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_DmaSetLock(XAie_DmaDesc *DmaDesc, XAie_Lock Acq, XAie_Lock Rel) {
	return (AieRC) mock_c()->actualCall("XAie_DmaSetLock")
		->withPointerParameters("DmaDesc", DmaDesc)
		->withParameterOfType("XAie_Lock", "Acq", &Acq)
		->withParameterOfType("XAie_Lock", "Rel", &Rel)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_DmaSetAxi(XAie_DmaDesc *DmaDesc, u8 Smid, u8 BurstLen, u8 Qos, u8 Cache, u8 Secure) {
	return (AieRC) mock_c()->actualCall("XAie_DmaSetAxi")
		->withPointerParameters("DmaDesc", DmaDesc)
		->withUnsignedIntParameters("Smid", Smid)
		->withUnsignedIntParameters("BurstLen", BurstLen)
		->withUnsignedIntParameters("Qos", Qos)
		->withUnsignedIntParameters("Cache", Cache)
		->withUnsignedIntParameters("Secure", Secure)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_DmaEnableBd(XAie_DmaDesc *DmaDesc) {
	return (AieRC) mock_c()->actualCall("XAie_DmaEnableBd")
		->withPointerParameters("DmaDesc", DmaDesc)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_DmaWriteBd(XAie_DevInst *DevInst, XAie_DmaDesc *DmaDesc, XAie_LocType Loc, u16 BdNum) {
	return (AieRC) mock_c()->actualCall("XAie_DmaWriteBd")
		->withPointerParameters("DevInst", DevInst)
		->withPointerParameters("DmaDesc", DmaDesc)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withUnsignedIntParameters("BdNum", BdNum)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_DmaUpdateBdAddr(XAie_DevInst *DevInst, XAie_LocType Loc, u64 Addr, u16 BdNum) {
	return (AieRC) mock_c()->actualCall("XAie_DmaUpdateBdAddr")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withUnsignedLongLongIntParameters("Addr", Addr)
		->withUnsignedIntParameters("BdNum", BdNum)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_LockAcquire(XAie_DevInst *DevInst, XAie_LocType Loc, XAie_Lock Lock, u32 TimeOut) {
	return (AieRC) mock_c()->actualCall("XAie_LockAcquire")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withParameterOfType("XAie_Lock", "Lock", &Lock)
		->withUnsignedIntParameters("TimeOut", TimeOut)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_LockRelease(XAie_DevInst *DevInst, XAie_LocType Loc, XAie_Lock Lock, u32 TimeOut) {
	return (AieRC) mock_c()->actualCall("XAie_LockRelease")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withParameterOfType("XAie_Lock", "Lock", &Lock)
		->withUnsignedIntParameters("TimeOut", TimeOut)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_DmaWaitForDone(XAie_DevInst *DevInst, XAie_LocType Loc, u8 ChNum, XAie_DmaDirection Dir, u32 TimeOutUs) {
	return (AieRC) mock_c()->actualCall("XAie_DmaWaitForDone")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withUnsignedIntParameters("ChNum", ChNum)
		->withIntParameters("Dir", Dir)
		->withUnsignedIntParameters("TimeOutUs", TimeOutUs)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_DmaChannelSetStartQueue(XAie_DevInst *DevInst, XAie_LocType Loc, u8 ChNum, XAie_DmaDirection Dir, u16 BdNum, u32 RepeatCount, u8 EnTokenIssue) {
	return (AieRC) mock_c()->actualCall("XAie_DmaChannelSetStartQueue")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withUnsignedIntParameters("ChNum", ChNum)
		->withIntParameters("Dir", Dir)
		->withUnsignedIntParameters("BdNum", BdNum)
		->withUnsignedIntParameters("RepeatCount", RepeatCount)
		->withUnsignedIntParameters("EnTokenIssue", EnTokenIssue)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_DmaTlastDisable(XAie_DmaDesc *DmaDesc) {
	return (AieRC) mock_c()->actualCall("XAie_DmaTlastDisable")
		->withPointerParameters("DmaDesc", DmaDesc)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_DmaSetBdIteration(XAie_DmaDesc *DmaDesc, u32 StepSize, u16 Wrap, u8 IterCurr) {
	return (AieRC) mock_c()->actualCall("XAie_DmaSetBdIteration")
		->withPointerParameters("DmaDesc", DmaDesc)
		->withUnsignedIntParameters("StepSize", StepSize)
		->withUnsignedIntParameters("Wrap", Wrap)
		->withUnsignedIntParameters("IterCurr", IterCurr)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_DmaSetNextBd(XAie_DmaDesc *DmaDesc, u16 NextBd, u8 EnableNextBd) {
	return (AieRC) mock_c()->actualCall("XAie_DmaSetNextBd")
		->withPointerParameters("DmaDesc", DmaDesc)
		->withUnsignedIntParameters("NextBd", NextBd)
		->withUnsignedIntParameters("EnableNextBd", EnableNextBd)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_LockSetValue(XAie_DevInst *DevInst, XAie_LocType Loc, XAie_Lock Lock) {
	return (AieRC) mock_c()->actualCall("XAie_LockSetValue")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withParameterOfType("XAie_Lock", "Lock", &Lock)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_DataMemRdWord(XAie_DevInst *DevInst, XAie_LocType Loc, u32 Addr, u32 *Data) {
	return (AieRC) mock_c()->actualCall("XAie_DataMemRdWord")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withUnsignedIntParameters("Addr", Addr)
		->withOutputParameter("Data", Data)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_DataMemBlockWrite(XAie_DevInst *DevInst, XAie_LocType Loc, u32 Addr, const u32 *Data, u32 Size) {
	return (AieRC) mock_c()->actualCall("XAie_DataMemBlockWrite")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withUnsignedIntParameters("Addr", Addr)
		->withPointerParameters("Data", (void*)Data)
		->withUnsignedIntParameters("Size", Size)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_DmaSetAddrLen(XAie_DmaDesc *DmaDesc, u64 Addr, u32 Len) {
	return (AieRC) mock_c()->actualCall("XAie_DmaSetAddrLen")
		->withPointerParameters("DmaDesc", DmaDesc)
		->withUnsignedLongLongIntParameters("Addr", Addr)
		->withUnsignedIntParameters("Len", Len)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_DataMemBlockRead(XAie_DevInst *DevInst, XAie_LocType Loc, u32 Addr, u32 *Data, u32 Size) {
	return (AieRC) mock_c()->actualCall("XAie_DataMemBlockRead")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withUnsignedIntParameters("Addr", Addr)
		->withOutputParameter("Data", Data)
		->withUnsignedIntParameters("Size", Size)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_DmaChannelPushBdToQueue(XAie_DevInst *DevInst, XAie_LocType Loc, u8 ChNum, XAie_DmaDirection Dir, u16 BdNum) {
	return (AieRC) mock_c()->actualCall("XAie_DmaChannelPushBdToQueue")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withUnsignedIntParameters("ChNum", ChNum)
		->withIntParameters("Dir", Dir)
		->withUnsignedIntParameters("BdNum", BdNum)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_DmaChannelEnable(XAie_DevInst *DevInst, XAie_LocType Loc, u8 ChNum, XAie_DmaDirection Dir) {
	return (AieRC) mock_c()->actualCall("XAie_DmaChannelEnable")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withUnsignedIntParameters("ChNum", ChNum)
		->withIntParameters("Dir", Dir)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_Read32(XAie_DevInst *DevInst, u64 RegOff, u32 *Data) {
	return (AieRC) mock_c()->actualCall("XAie_Read32")
		->withPointerParameters("DevInst", DevInst)
		->withUnsignedLongLongIntParameters("RegOff", RegOff)
		->withOutputParameter("Data", Data)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XScuGic_DeviceInitialize(void* GicInstancePtr) {
	return (AieRC) mock_c()->actualCall("XScuGic_DeviceInitialize")
		->withPointerParameters("GicInstancePtr", GicInstancePtr)
		->returnIntValueOrDefault(XAIE_OK);
}

void* XScuGic_LookupConfig(u32 DeviceId) {
	return mock_c()->actualCall("XScuGic_LookupConfig")
		->withUnsignedIntParameters("DeviceId", DeviceId)
		->returnPointerValueOrDefault(0);
}

AieRC XScuGic_CfgInitialize(void *InstancePtr, void *ConfigPtr, u32 EffectiveAddr) {
	return mock_c()->actualCall("XScuGic_CfgInitialize")
		->withPointerParameters("InstancePtr", InstancePtr)
		->withPointerParameters("ConfigPtr", ConfigPtr)
		->withUnsignedIntParameters("EffectiveAddr", EffectiveAddr)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XScuGic_DeviceInterruptHandler(void *InstancePtr) {
	return mock_c()->actualCall("XScuGic_DeviceInterruptHandler")
		->withPointerParameters("InstancePtr", InstancePtr)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC Xil_ExceptionRegisterHandler(u32 ExceptionId, void *Handler, void *Data) {
	return mock_c()->actualCall("Xil_ExceptionRegisterHandler")
		->withUnsignedIntParameters("ExceptionId", ExceptionId)
		->withPointerParameters("Handler", Handler)
		->withPointerParameters("Data", Data)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XScuGic_Connect(void *InstancePtr, u32 Int_Id, void *Handler, void *CallBackRef) {
	return mock_c()->actualCall("XScuGic_Connect")
		->withPointerParameters("InstancePtr", InstancePtr)
		->withUnsignedIntParameters("Int_Id", Int_Id)
		->withPointerParameters("Handler", Handler)
		->withPointerParameters("CallBackRef", CallBackRef)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XScuGic_Enable(void *InstancePtr, u32 Int_Id) {
	return mock_c()->actualCall("XScuGic_Enable")
		->withPointerParameters("InstancePtr", InstancePtr)
		->withUnsignedIntParameters("Int_Id", Int_Id)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XScuGic_Disable(void *InstancePtr, u32 Int_Id) {
	return mock_c()->actualCall("XScuGic_Disable")
		->withPointerParameters("InstancePtr", InstancePtr)
		->withUnsignedIntParameters("Int_Id", Int_Id)
		->returnIntValueOrDefault(XAIE_OK);
}

XAie_MemInst* XAie_MemAllocate(XAie_DevInst *DevInst, u64 Size, XAie_MemCacheProp Cache) {
	return mock_c()->actualCall("XAie_MemAllocate")
		->withPointerParameters("DevInst", DevInst)
		->withUnsignedLongLongIntParameters("Size", Size)
		->withIntParameters("Cache", Cache)
		->returnPointerValueOrDefault(0);
}

AieRC XAie_MemFreeVAddr(XAie_DevInst *DevInst, void *VAddr) {
	return mock_c()->actualCall("XAie_MemFreeVAddr")
		->withPointerParameters("DevInst", DevInst)
		->withPointerParameters("VAddr", VAddr)
		->returnIntValueOrDefault(XAIE_OK);
}
AieRC XAie_MemSyncForCPUVAddr(XAie_DevInst *DevInst, void *VAddr, uint64_t size)
{
	return mock_c()->actualCall("XAie_MemSyncForCPUVAddr")
		->withPointerParameters("DevInst", &DevInst)
		->withPointerParameters("VAddr", VAddr)
		->withUnsignedLongLongIntParameters("size", size)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_MemSyncForDevVAddr(XAie_DevInst *DevInst, void *VAddr, uint64_t size) {
	return mock_c()->actualCall("XAie_MemSyncForDevVAddr")
		->withPointerParameters("DevInst", DevInst)
		->withPointerParameters("VAddr", VAddr)
		->withUnsignedLongLongIntParameters("size", size)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_MemGetDevAddrFromVAddr(XAie_DevInst *DevInst, void *VAddr, uint64_t *DevAddr) {
	return (AieRC) mock_c()->actualCall("XAie_MemGetDevAddrFromVAddr")
		->withPointerParameters("DevInst", DevInst)
		->withPointerParameters("VAddr", VAddr)
		->withPointerParameters("DevAddr", DevAddr)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_ErrorHandlingInit(XAie_DevInst *DevInst) {
	return (AieRC) mock_c()->actualCall("XAie_ErrorHandlingInit")
		->withPointerParameters("DevInst", DevInst)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_PmRequestTiles(XAie_DevInst *DevInst, u32 NumTiles, XAie_LocType *Locs) {
	return (AieRC) mock_c()->actualCall("XAie_PmRequestTiles")
		->withPointerParameters("DevInst", DevInst)
		->withUnsignedIntParameters("NumTiles", NumTiles)
		->withPointerParameters("Locs", Locs)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_UpdateNpiAddr(XAie_DevInst *DevInst, u64 NpiAddr) {
	return (AieRC) mock_c()->actualCall("XAie_UpdateNpiAddr")
		->withPointerParameters("DevInst", DevInst)
		->withUnsignedIntParameters("NpiAddr", NpiAddr)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_BacktrackErrorInterrupts(XAie_DevInst *DevInst, XAie_ErrorMetaData *MData) {
	AieRC rc = (AieRC) mock_c()->actualCall("XAie_BacktrackErrorInterrupts")
		->withPointerParameters("DevInst", DevInst)
		->withPointerParameters("MData", MData)
		->returnIntValueOrDefault(XAIE_OK);
	/* Caller reads ErrorCount / IsNextInfoValid; default mock leaves zeros → empty cache → nullptr */
	if (MData != NULL && rc == XAIE_OK) {
		MData->ErrorCount = 1U;
		MData->IsNextInfoValid = 0U;
		if (MData->Payload != NULL && MData->ArraySize > 0U) {
			memset(&MData->Payload[0], 0, sizeof(XAie_ErrorPayload));
		}
	}
	return rc;
}

AieRC XAie_PmSetColumnClk(XAie_DevInst *DevInst, u32 StartCol, u32 NumCols, u8 Enable) {
	return (AieRC) mock_c()->actualCall("XAie_PmSetColumnClk")
		->withPointerParameters("DevInst", DevInst)
		->withUnsignedIntParameters("StartCol", StartCol)
		->withUnsignedIntParameters("NumCols", NumCols)
		->withUnsignedIntParameters("Enable", Enable)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_EventSelectStrmPortReset(XAie_DevInst *DevInst, XAie_LocType Loc, u8 SelectId) {
	return (AieRC) mock_c()->actualCall("XAie_EventSelectStrmPortReset")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withUnsignedIntParameters("SelectId", SelectId)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_EventSelectStrmPort(XAie_DevInst *DevInst, XAie_LocType Loc, u8 SelectId, XAie_StrmPortIntf PortIntf, StrmSwPortType Port, u8 PortNum) {
	return (AieRC) mock_c()->actualCall("XAie_EventSelectStrmPort")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withUnsignedIntParameters("SelectId", SelectId)
		->withUnsignedIntParameters("PortIntf", PortIntf)
		->withUnsignedIntParameters("Port", Port)
		->withUnsignedIntParameters("PortNum", PortNum)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_EventBroadcastReset(XAie_DevInst *DevInst, XAie_LocType Loc, XAie_ModuleType Module, u8 BroadcastId) {
	return (AieRC) mock_c()->actualCall("XAie_EventBroadcastReset")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withIntParameters("Module", Module)
		->withUnsignedIntParameters("BroadcastId", BroadcastId)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_PerfCounterEventValueReset(XAie_DevInst *DevInst, XAie_LocType Loc, XAie_ModuleType Module, u8 Counter) {
	return (AieRC) mock_c()->actualCall("XAie_PerfCounterEventValueReset")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withIntParameters("Module", Module)
		->withUnsignedIntParameters("Counter", Counter)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_PerfCounterReset(XAie_DevInst *DevInst, XAie_LocType Loc, XAie_ModuleType Module, u8 Counter) {
	return (AieRC) mock_c()->actualCall("XAie_PerfCounterReset")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withIntParameters("Module", Module)
		->withUnsignedIntParameters("Counter", Counter)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_PerfCounterResetControlReset(XAie_DevInst *DevInst, XAie_LocType Loc, XAie_ModuleType Module, u8 Counter) {
	return (AieRC) mock_c()->actualCall("XAie_PerfCounterResetControlReset")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withIntParameters("Module", Module)
		->withUnsignedIntParameters("Counter", Counter)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_PerfCounterControlReset(XAie_DevInst *DevInst, XAie_LocType Loc, XAie_ModuleType Module, u8 Counter) {
	return (AieRC) mock_c()->actualCall("XAie_PerfCounterControlReset")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withIntParameters("Module", Module)
		->withUnsignedIntParameters("Counter", Counter)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_PerfCounterResetControlSet(XAie_DevInst *DevInst, XAie_LocType Loc, XAie_ModuleType Module, u8 Counter, u32 ResetEvent) {
	return (AieRC) mock_c()->actualCall("XAie_PerfCounterResetControlSet")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withIntParameters("Module", Module)
		->withUnsignedIntParameters("Counter", Counter)
		->withUnsignedIntParameters("ResetEvent", ResetEvent)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_PerfCounterControlSet(XAie_DevInst *DevInst, XAie_LocType Loc, XAie_ModuleType Module, u8 Counter, u32 StartEvent, u32 StopEvent) {
	return (AieRC) mock_c()->actualCall("XAie_PerfCounterControlSet")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withIntParameters("Module", Module)
		->withUnsignedIntParameters("Counter", Counter)
		->withUnsignedIntParameters("StartEvent", StartEvent)
		->withUnsignedIntParameters("StopEvent", StopEvent)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_EventBroadcast(XAie_DevInst *DevInst, XAie_LocType Loc, XAie_ModuleType Module, u8 BroadcastId, u32 Event) {
	return (AieRC) mock_c()->actualCall("XAie_EventBroadcast")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withIntParameters("Module", Module)
		->withUnsignedIntParameters("BroadcastId", BroadcastId)
		->withUnsignedIntParameters("Event", Event)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_PerfCounterEventValueSet(XAie_DevInst *DevInst, XAie_LocType Loc, XAie_ModuleType Module, u8 Counter, u32 EventVal) {
	return (AieRC) mock_c()->actualCall("XAie_PerfCounterEventValueSet")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withIntParameters("Module", Module)
		->withUnsignedIntParameters("Counter", Counter)
		->withUnsignedIntParameters("EventVal", EventVal)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_EventBroadcastUnblockDir(XAie_DevInst *DevInst, XAie_LocType Loc, XAie_ModuleType Module, XAie_BroadcastSw Switch, u8 BroadcastId, u8 Dir) {
	return (AieRC) mock_c()->actualCall("XAie_EventBroadcastUnblockDir")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withIntParameters("Module", Module)
		->withUnsignedIntParameters("Switch", Switch)
		->withUnsignedIntParameters("BroadcastId", BroadcastId)
		->withUnsignedIntParameters("Dir", Dir)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_EventBroadcastBlockDir(XAie_DevInst *DevInst, XAie_LocType Loc, XAie_ModuleType Module, XAie_BroadcastSw Switch, u8 BroadcastId, u8 Dir) {
	return (AieRC) mock_c()->actualCall("XAie_EventBroadcastBlockDir")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withIntParameters("Module", Module)
		->withUnsignedIntParameters("Switch", Switch)
		->withUnsignedIntParameters("BroadcastId", BroadcastId)
		->withUnsignedIntParameters("Dir", Dir)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_EventBroadcastBlockMapDir(XAie_DevInst *DevInst, XAie_LocType Loc, XAie_ModuleType Module, XAie_BroadcastSw Switch, u32 ChannelBitMap, u8 Dir) {
	return (AieRC) mock_c()->actualCall("XAie_EventBroadcastBlockMapDir")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withIntParameters("Module", Module)
		->withUnsignedIntParameters("Switch", Switch)
		->withUnsignedIntParameters("ChannelBitMap", ChannelBitMap)
		->withUnsignedIntParameters("Dir", Dir)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_EventPCReset(XAie_DevInst *DevInst, XAie_LocType Loc, u8 PCEventId) {
	return (AieRC) mock_c()->actualCall("XAie_EventPCReset")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withUnsignedIntParameters("PCEventId", PCEventId)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_Write32(XAie_DevInst *DevInst, u64 RegOff, u32 Value) {
	return (AieRC) mock_c()->actualCall("XAie_Write32")
		->withPointerParameters("DevInst", DevInst)
		->withUnsignedLongLongIntParameters("RegOff", RegOff)
		->withUnsignedIntParameters("Value", Value)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_MemSyncForCPU(XAie_MemInst *MemInst) {
	// Side effect: simulate firmware setting completion signal (for testcpputest)
	extern uint32_t* g_test_completion_ptr;
	extern void* g_test_comp_sig_meminst;
	extern int g_test_sync_call_count;
	extern int g_test_completion_threshold;
	extern uint32_t g_test_completion_value;

	// Only count and set signal when syncing the completion signal MemInst
	if (g_test_comp_sig_meminst != NULL && MemInst == g_test_comp_sig_meminst) {
	    g_test_sync_call_count++;
	    if (g_test_completion_ptr != NULL && g_test_sync_call_count > g_test_completion_threshold) {
	        *g_test_completion_ptr = g_test_completion_value;
	    }
	}

	return mock_c()->actualCall("XAie_MemSyncForCPU")
		->withPointerParameters("MemInst", MemInst)
		->returnIntValueOrDefault(XAIE_OK);
}


AieRC XAie_MemSyncForDev(XAie_MemInst *MemInst) {
	return mock_c()->actualCall("XAie_MemSyncForDev")
		->withPointerParameters("MemInst", MemInst)
		->returnIntValueOrDefault(XAIE_OK);
}

u64 XAie_MemGetDevAddr(XAie_MemInst *MemInst) {
	return (u64) mock_c()->actualCall("XAie_MemGetDevAddr")
		->withPointerParameters("MemInst", MemInst)
		->returnIntValueOrDefault(0);
}

void* XAie_MemGetVAddr(XAie_MemInst *MemInst) {
	return mock_c()->actualCall("XAie_MemGetVAddr")
		->withPointerParameters("MemInst", MemInst)
		->returnPointerValueOrDefault(0);
}

AieRC XAie_MemFree(XAie_MemInst *MemInst) {
	return mock_c()->actualCall("XAie_MemFree")
		->withPointerParameters("MemInst", MemInst)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_LoadUcMem(XAie_DevInst *DevInst, XAie_LocType Loc, const unsigned char* ElfMem) {
	return mock_c()->actualCall("XAie_LoadUcMem")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withPointerParameters("ElfMem", (void*)ElfMem)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_Finish(XAie_DevInst *DevInst) {
	return mock_c()->actualCall("XAie_Finish")
		->withPointerParameters("DevInst", DevInst)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_CoreWaitForDone(XAie_DevInst *DevInst, XAie_LocType Loc, u32 TimeOut) {
	return (AieRC) mock_c()->actualCall("XAie_CoreWaitForDone")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withUnsignedIntParameters("TimeOut", TimeOut)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_CoreDisable(XAie_DevInst *DevInst, XAie_LocType Loc) {
	return (AieRC) mock_c()->actualCall("XAie_CoreDisable")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_WaitCycles(XAie_DevInst *DevInst, XAie_LocType Loc, XAie_ModuleType Module, u64 CycleCnt) {
	return (AieRC) mock_c()->actualCall("XAie_WaitCycles")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withIntParameters("Module", Module)
		->withUnsignedLongLongIntParameters("CycleCnt", CycleCnt)
		->returnIntValueOrDefault(XAIE_OK);
}
// Mock function to replace GetTTypefromLoc
u8 _XAie_GetTTypefromLoc(XAie_DevInst *DevInst, XAie_LocType Loc) {
	// return mock_c()->actualCall("GetTTypefromLoc")
	//       ->withPointerParameters("DevInst", DevInst)
	//       ->withParameterOfType("XAie_LocType", "Loc", &Loc)
	//       ->returnIntValueOrDefault(XAIE_OK);
	return 1;
}
u32  XAie_GetNumRows(XAie_DevInst *DevInst,u8 TileType) {
	return (AieRC) mock_c()->actualCall("XAie_GetNumRows")
		->withPointerParameters("DevInst", DevInst)
		->withUnsignedIntParameters("Tiletype",TileType)
		->returnIntValueOrDefault(XAIE_OK);
}
u32  XAie_GetStartRow(XAie_DevInst *DevInst,u8 TileType) {
	return (AieRC) mock_c()->actualCall("XAie_GetStartRow")
		->withPointerParameters("DevInst", DevInst)
		->withUnsignedIntParameters("Tiletype",TileType)
		->returnIntValueOrDefault(XAIE_OK);
}
AieRC XAie_TraceEventReset(XAie_DevInst *DevInst, XAie_LocType Loc,
		XAie_ModuleType Module, u8 SlotId) {
	return (AieRC) mock_c()->actualCall("XAie_TraceEventReset")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withIntParameters("Module", Module)
		->withIntParameters("SlotId", SlotId)
		->returnIntValueOrDefault(XAIE_OK);
}
AieRC XAie_TraceEventList(XAie_DevInst *DevInst, XAie_LocType Loc,
		XAie_ModuleType Module, XAie_Events *Events, u8 *SlotId, u8 NumEvents) {
	return (AieRC) mock_c()->actualCall("XAie_TraceEventList")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withIntParameters("Module", Module)
		->withPointerParameters("Events", Events)
		->withPointerParameters("SlotId", SlotId)
		->withIntParameters("NumEvents", NumEvents)
		->returnIntValueOrDefault(XAIE_OK);
}
AieRC XAie_TraceControlConfigReset(XAie_DevInst *DevInst, XAie_LocType Loc,
		XAie_ModuleType Module)
{
	return (AieRC) mock_c()->actualCall("XAie_TraceControlConfigReset")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withIntParameters("Module", Module)
		->returnIntValueOrDefault(XAIE_OK);
}
typedef enum {
	XAIE_TRACE_EVENT_TIME,
	XAIE_TRACE_EVENT_PC,
	XAIE_TRACE_INST_EXEC,
} XAie_TraceMode;

AieRC XAie_TraceControlConfig(XAie_DevInst *DevInst, XAie_LocType Loc,
		XAie_ModuleType Module, XAie_Events StartEvent, XAie_Events StopEvent,
		XAie_TraceMode Mode) {
	return (AieRC) mock_c()->actualCall("XAie_TraceControlConfig")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withIntParameters("Module", Module)
		->withIntParameters("StartEvent", StartEvent)
		->withIntParameters("StopEvent", StopEvent)
		->withIntParameters("Mode", Mode)
		->returnIntValueOrDefault(XAIE_OK);
}
AieRC XAie_TracePktConfig(XAie_DevInst *DevInst, XAie_LocType Loc,
		XAie_ModuleType Module, XAie_Packet Pkt){
	return (AieRC) mock_c()->actualCall("XAie_TracePktConfig")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withIntParameters("Module", Module)
		->withParameterOfType("XAie_Packet", "Pkt", &Pkt)
		->returnIntValueOrDefault(XAIE_OK);
}

AieRC XAie_GetUngatedLocsInPartition(XAie_DevInst *DevInst, u32 *NumTiles,
		XAie_LocType *Locs){
	return (AieRC) mock_c()->actualCall("XAie_GetUngatedLocsInPartition")
		->withPointerParameters("DevInst", DevInst)
		->withPointerParameters("NumTiles", NumTiles)
		->withParameterOfType("XAie_LocType", "Loc", &Locs)
		->returnIntValueOrDefault(XAIE_OK);
}
AieRC XAie_EventPhysicalToLogicalConv(XAie_DevInst *DevInst, XAie_LocType Loc,
		XAie_ModuleType Module, u16 HwEvent, XAie_Events *EnumEvent) {
	return (AieRC) mock_c()->actualCall("XAie_EventPhysicalToLogicalConv")
		->withPointerParameters("DevInst", DevInst)
		->withParameterOfType("XAie_LocType", "Loc", &Loc)
		->withIntParameters("Module", Module)
		->withIntParameters("HwEvent", HwEvent)  // Corrected to handle non-pointer type
		->withPointerParameters("EnumEvent", EnumEvent)
		->returnIntValueOrDefault(XAIE_OK);
}
AieRC XAie_SetIOBackend(XAie_DevInst *DevInst, XAie_BackendType Backend) {
	return (AieRC) mock_c()->actualCall("XAie_SetIOBackend")
		->withPointerParameters("DevInst", DevInst)
		->withIntParameters("Backend", Backend)
		->returnIntValueOrDefault(XAIE_OK);
}
