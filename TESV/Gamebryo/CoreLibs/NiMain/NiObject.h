#pragma once

#include "NiRefObject.h"
#include "NiRTTI.h"
#include <cstdint>

class NiNode;
class NiSwitchNode;
class BSFadeNode;
class BSMultiBoundNode;
class BSGeometry;
class NiTriStrips;
class BSTriShape;
class BSSegmentedTriShape;
class BSSubIndexTriShape;
class BSDynamicTriShape;
class NiGeometry;
class NiTriBasedGeom;
class NiTriShape;
class NiParticles;
class BSLines;
class bhkNiCollisionObject;
class bhkBlendCollisionObject;
class bhkNPCollisionObject;
class bhkRigidBody;
class bhkLimitedHingeConstraint;
class NiCloningProcess;
class NiStream;
class NiObjectGroup;
class NiControllerManager;

class NiObject : public NiRefObject
{
public:
	~NiObject() override = default;
	virtual const NiRTTI* GetRTTI() const;
	virtual NiNode* IsNode();
	virtual NiSwitchNode* IsSwitchNode();
	virtual BSFadeNode* IsFadeNode();
	virtual BSMultiBoundNode* IsMultiBoundNode();
	virtual BSGeometry* IsGeometry();
	virtual NiTriStrips* IsTriStrips();
	virtual BSTriShape* IsTriShape();
	virtual BSSegmentedTriShape* IsSegmentedTriShape();
	virtual BSSubIndexTriShape* IsSubIndexTriShape();
	virtual BSDynamicTriShape* IsDynamicTriShape();
	virtual NiGeometry* IsNiGeometry();
	virtual NiTriBasedGeom* IsNiTriBasedGeom();
	virtual NiTriShape* IsNiTriShape();
	virtual NiParticles* IsParticlesGeom();
	virtual BSLines* IsLinesGeom();
	virtual bhkNiCollisionObject* IsBhkNiCollisionObject();
	virtual bhkBlendCollisionObject* IsBhkBlendCollisionObject();
	virtual bhkNPCollisionObject* IsbhkNPCollisionObject();
	virtual bhkRigidBody* IsBhkRigidBody() const;
	virtual bhkLimitedHingeConstraint* IsBhkLimitedHingeConstraint();
	virtual NiObject* CreateClone(NiCloningProcess& kCloning);
	virtual void LoadBinary(NiStream& kStream);
	virtual void LinkObject(NiStream& kStream);
	virtual bool RegisterStreamables(NiStream& kStream);
	virtual void SaveBinary(NiStream& kStream);
	virtual bool IsEqual(NiObject* pkObject);
	virtual void ProcessClone(NiCloningProcess& kCloning);
	virtual void PostLinkObject(NiStream& kStream);
	virtual bool StreamCanSkip();
	virtual const NiRTTI* GetStreamableRTTI() const;
	virtual uint32_t GetBlockAllocationSize() const;
	virtual NiObjectGroup* GetGroup() const;
	virtual void SetGroup(NiObjectGroup* pkGroup);
	virtual NiControllerManager* IsNiControllerManager();

	NiObject* Clone(NiCloningProcess& kCloning);

	static const NiRTTI ms_RTTI;

protected:
	NiObject();
};
static_assert(sizeof(NiObject) == 16);

inline const NiRTTI NiObject::ms_RTTI("NiObject", nullptr);
