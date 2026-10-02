#include "NiObject.h"
#include <cstring>

NiObject::NiObject()
{
}

const NiRTTI* NiObject::GetRTTI() const
{
	return &ms_RTTI;
}

bool NiObject::IsEqual(NiObject* pkObject)
{
	if (!pkObject)
		return false;
	const char* pcOtherName = pkObject->GetRTTI()->GetName();
	const char* pcName = GetRTTI()->GetName();
	return strcmp(pcName, pcOtherName) == 0;
}

const NiRTTI* NiObject::GetStreamableRTTI() const
{
	return GetRTTI();
}

NiObject* NiObject::CreateClone(NiCloningProcess&)
{
	return this;
}

void NiObject::LoadBinary(NiStream&) {}
void NiObject::LinkObject(NiStream&) {}
void NiObject::SaveBinary(NiStream&) {}
void NiObject::PostLinkObject(NiStream&) {}
void NiObject::SetGroup(NiObjectGroup*) {}
bool NiObject::StreamCanSkip() { return false; }
uint32_t NiObject::GetBlockAllocationSize() const { return 0; }
NiObjectGroup* NiObject::GetGroup() const { return nullptr; }
NiControllerManager* NiObject::IsNiControllerManager() { return nullptr; }
NiNode* NiObject::IsNode() { return nullptr; }
NiSwitchNode* NiObject::IsSwitchNode() { return nullptr; }
BSFadeNode* NiObject::IsFadeNode() { return nullptr; }
BSMultiBoundNode* NiObject::IsMultiBoundNode() { return nullptr; }
BSGeometry* NiObject::IsGeometry() { return nullptr; }
NiTriStrips* NiObject::IsTriStrips() { return nullptr; }
BSTriShape* NiObject::IsTriShape() { return nullptr; }
BSSegmentedTriShape* NiObject::IsSegmentedTriShape() { return nullptr; }
BSSubIndexTriShape* NiObject::IsSubIndexTriShape() { return nullptr; }
BSDynamicTriShape* NiObject::IsDynamicTriShape() { return nullptr; }
NiGeometry* NiObject::IsNiGeometry() { return nullptr; }
NiTriBasedGeom* NiObject::IsNiTriBasedGeom() { return nullptr; }
NiTriShape* NiObject::IsNiTriShape() { return nullptr; }
NiParticles* NiObject::IsParticlesGeom() { return nullptr; }
BSLines* NiObject::IsLinesGeom() { return nullptr; }
bhkNiCollisionObject* NiObject::IsBhkNiCollisionObject() { return nullptr; }
bhkBlendCollisionObject* NiObject::IsBhkBlendCollisionObject() { return nullptr; }
bhkNPCollisionObject* NiObject::IsbhkNPCollisionObject() { return nullptr; }
bhkRigidBody* NiObject::IsBhkRigidBody() const { return nullptr; }
bhkLimitedHingeConstraint* NiObject::IsBhkLimitedHingeConstraint() { return nullptr; }

NiObject* NiObject::Clone(NiCloningProcess& kCloning)
{
	NiObject* pkClone = CreateClone(kCloning);
	ProcessClone(kCloning);
	return pkClone;
}
