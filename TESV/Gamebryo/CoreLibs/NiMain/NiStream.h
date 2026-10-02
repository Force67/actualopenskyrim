#pragma once

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSTSmallArray.h"
#include "BSSystem/BSFixedString.h"
#include "NiObject.h"
#include "NiSmartPointer.h"
#include "NiTLargeArray.h"
#include "NiTPointerMap.h"
#include "NiTStringMap.h"
#include "NiTArray.h"
#include "Gamebryo/CoreLibs/NiSystem/NiSemaphore.h"
#include "Gamebryo/CoreLibs/NiSystem/NiThreadProcedure.h"
#include <windows.h>
#include <cstddef>
#include <cstdint>

class NiBinaryStream;
class NiSearchPath;
class NiThread;
class NiTexture;
class NiAVObject;

struct BSStreamHeader
{
	BSStreamHeader() : uiVersion(100)
	{
		pAuthor[0] = pProcessScript[0] = pExportScript[0] = 0;
	}
	uint32_t uiVersion;
	char pAuthor[64];
	char pProcessScript[64];
	char pExportScript[64];
};
static_assert(sizeof(BSStreamHeader) == 196);

class NiStream
{
public:
	using LoadFunction = NiObject* (*)();
	using PostProcessFunction = void (*)(NiStream&, NiObject*);

	NiStream();
	virtual ~NiStream();
	virtual bool Load(NiBinaryStream* pkIstr);
	virtual bool Load(char* pcBuffer, uint32_t uiBufferSize);
	virtual bool Load(const char* pcFileName);
	virtual bool Save(NiBinaryStream* pkOstr);
	virtual bool Save(char*& pcBuffer, uint32_t& uiBufferSize);
	virtual bool Save(const char* pcFileName);
	virtual void BackgroundLoadOnExit();
	virtual bool RegisterFixedString(const BSFixedString& kString);
	virtual bool RegisterSaveObject(NiObject* pkObject);
	virtual void ChangeObject(NiObject* pkObject);
	virtual uint32_t GetLinkIDFromObject(const NiObject* pkObject) const;
	virtual void SaveLinkID(const NiObject* pkObject);

	enum ThreadStatus { IDLE, LOADING, CANCELLING, PAUSING, PAUSED };
	class LoadState
	{
	public:
		float m_fReadProgress;
		float m_fLinkProgress;
	};
	class BackgroundLoadProcedure : public NiThreadProcedure
	{
	public:
		BackgroundLoadProcedure(NiStream* pkStream) : m_pkStream(pkStream) {}
		unsigned int ThreadProcedure(void* pvArg) override;
		NiStream* m_pkStream;
	};
	void BackgroundLoad();
	void BackgroundLoadBegin(const char* pcFileName);
	void BackgroundLoadBegin(NiBinaryStream* pkIstr);
	ThreadStatus BackgroundLoadPoll(LoadState* pkState);
	void BackgroundLoadPause();
	void BackgroundLoadResume();
	void BackgroundLoadCancel();
	bool BackgroundLoadFinish();
	void BackgroundLoadCleanup();

	static void _SDMInit();
	static void _SDMShutdown();
	static int RegisterLoader(const char* pcRTTI, LoadFunction pfnLoad);
	static void UnregisterLoader(const char* pcRTTI);
	static NiObject* CreateObjectByRTTI(const char* pcRTTI);
	static void RegisterPostProcessFunction(PostProcessFunction pfnFunction);
	static void UnregisterPostProcessFunction(PostProcessFunction pfnFunction);
	static uint32_t GetVersionFromString(const char* pcVersionString);
	void SetSaveAsLittleEndian(bool bLittle);
	void SetLastError(uint32_t uiError);
	void SetFilePath(const char* pcFilePath);
	bool GetNextSearchPath(char* pcPath, uint32_t uiMaxSize);
	void InsertObject(NiObject* pkObject);
	void RemoveObject(NiObject* pkObject);
	void RemoveAllObjects();
	void LoadCString(char*& pcString);
	void SaveCString(const char* pcString);
	void LoadFixedString(BSFixedString& kString);
	void SaveFixedString(const BSFixedString& kString);
	void LoadCStringAsFixedString(BSFixedString& kString);
	void ReadLinkID();
	uint32_t ReadMultipleLinkIDs();
	NiObject* ResolveLinkID();
	NiObject* GetObjectFromLinkID();
	uint32_t GetNumberOfLinkIDs();
	void SetNumberOfLinkIDs(uint32_t uiLinks);
	NiObjectGroup* GetGroupFromID(uint32_t uiID) const;

protected:
	void BackgroundLoadBegin();
	bool LoadRTTI();
	void SaveRTTI();
	void LoadObjectGroups();
	void SaveObjectGroups();
	void UpdateObjectGroups();
	void FreeLoadData();
	void SetSelectiveUpdateFlagsForOldVersions();
	void SetSelectiveUpdateFlagsTTTFRecursive(NiAVObject* pkObject);
	void LoadRTTIString(char* pcString);
	void RTTIError(const char* pcRTTI);
	void SaveFixedStringTable();
	bool LoadFixedStringTable();
	virtual bool LoadHeader();
	virtual void SaveHeader();
	virtual bool LoadStream();
	virtual bool SaveStream();
	virtual void RegisterObjects();
	virtual void LoadTopLevelObjects();
	virtual void SaveTopLevelObjects();
	virtual bool LoadObject();
	virtual uint32_t PreSaveObjectSizeTable();
	virtual bool SaveObjectSizeTable(uint32_t uiPosition);
	virtual bool LoadObjectSizeTable();

public:
	BSStreamHeader m_BSStreamHeader;
	BSTSmallArray<NiObjectGroup*, 4> m_kGroups;
	uint32_t m_uiNifFileVersion;
	uint32_t m_uiNifFileUserDefinedVersion;
	char m_acFileName[260];
	bool m_bSaveLittleEndian;
	bool m_bSourceIsLittleEndian;
	NiSearchPath* m_pkSearchPath;
	NiTLargeObjectArray<NiPointer<NiObject>> m_kObjects;
	NiTLargePrimitiveArray<uint32_t> m_kObjectSizes;
	NiTLargeObjectArray<NiPointer<NiObject>> m_kTopObjects;
	NiTLargeObjectArray<BSFixedString> m_kFixedStrings;
	NiBinaryStream* m_pkIstr;
	NiBinaryStream* m_pkOstr;
	uint32_t m_uiLinkIndex;
	uint32_t m_uiLinkBlockIndex;
	NiTPointerMap<const NiObject*, uint32_t> m_kRegisterMap;
	uint16_t m_usNiAVObjectFlags;
	uint16_t m_usNiTimeControllerFlags;
	uint16_t m_usNiPropertyFlags;
	uint32_t m_uiBackgroundLoadStatus;
	bool m_bBackgroundLoadResult;
	uint32_t m_uiLoad;
	uint32_t m_uiLink;
	uint32_t m_uiPostLink;
	NiThread* m_pkThread;
	void* m_pkBackgroundLoadProcedure;
	uint32_t m_uiBackgroundLoadPriority;
	uint32_t m_uiBackgroundLoadProcessor;
	uint32_t m_uiBackgroundLoadThread;
	char m_acLastLoadedRTTI[260];
	uint32_t m_uiLastError;
	char m_acLastErrorMessage[260];
	char m_acFilePath[260];

	static bool bUseDefaultPath;

private:
	static NiTStringPointerMap<LoadFunction>* ms_pkLoaders;
	static NiTPrimitiveArray<PostProcessFunction>* ms_pkPostProcessFunctions;
	static CRITICAL_SECTION ms_kCleanupCriticalSection;
	static NiSemaphore m_kSemaphore;
	static const uint32_t ms_uiNifMinVersion;
	static const uint32_t ms_uiNifMaxVersion;
	static const uint32_t ms_uiNifMinUserDefinedVersion;
	static const uint32_t ms_uiNifMaxUserDefinedVersion;
};
static_assert(sizeof(NiStream) == 1568);
static_assert(offsetof(NiStream, m_kGroups) == 208);
static_assert(offsetof(NiStream, m_kObjects) == 536);
static_assert(offsetof(NiStream, m_pkIstr) == 664);
static_assert(offsetof(NiStream, m_kRegisterMap) == 688);
static_assert(offsetof(NiStream, m_uiBackgroundLoadStatus) == 728);
static_assert(offsetof(NiStream, m_pkThread) == 752);
static_assert(offsetof(NiStream, m_acLastLoadedRTTI) == 780);
static_assert(offsetof(NiStream, m_uiLastError) == 1040);
static_assert(offsetof(NiStream, m_acFilePath) == 1304);

inline NiSemaphore NiStream::m_kSemaphore;

extern thread_local constinit BSScrapArray<uint32_t>* pLinkIDAS;
extern thread_local constinit BSScrapArray<uint32_t>* pLinkIDBlocksAS;
