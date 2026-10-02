#pragma once

#include "NiObject.h"
#include "BSSystem/BSFixedString.h"

class NiExtraData : public NiObject
{
public:
	NiExtraData();
	explicit NiExtraData(const BSFixedString& kName);
	~NiExtraData() override;
	const NiRTTI* GetRTTI() const override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	virtual bool IsStreamable() const;
	virtual bool IsCloneable() const;

	const BSFixedString& GetName() const;
	void ClearName();
	void SetName(const BSFixedString& kName);
	static const NiRTTI ms_RTTI;

	BSFixedString m_kName;
};
static_assert(sizeof(NiExtraData) == 24);

inline const NiRTTI NiExtraData::ms_RTTI("NiExtraData", &NiObject::ms_RTTI);
