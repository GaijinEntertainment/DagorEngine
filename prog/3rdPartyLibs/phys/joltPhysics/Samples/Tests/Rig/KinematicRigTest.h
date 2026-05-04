// Jolt Physics Library (https://github.com/jrouwe/JoltPhysics)
// SPDX-FileCopyrightText: 2021 Jorrit Rouwe
// SPDX-License-Identifier: MIT

#pragma once

#include <Tests/Test.h>
#include <Jolt/Skeleton/Skeleton.h>
#include <Jolt/Skeleton/SkeletalAnimation.h>
#include <Jolt/Skeleton/SkeletonPose.h>
#include <Utils/RagdollLoader.h>
#include <Jolt/Physics/Ragdoll/Ragdoll.h>

class KinematicRigTest : public Test
{
public:
	JPH_DECLARE_RTTI_VIRTUAL(JPH_NO_EXPORT, KinematicRigTest)

	// Description of the test
	virtual const char *	GetDescription() const override
	{
		return "Tests a kinematic ragdoll moving towards a wall of boxes.\n"
			"Also demonstrates that kinematic bodies don't get influenced by static bodies.";
	}

	// Destructor
	virtual					~KinematicRigTest() override;

	// Number used to scale the terrain and camera movement to the scene
	virtual float			GetWorldScale() const override								{ return 0.2f; }

	virtual void			Initialize() override;
	virtual void			PrePhysicsUpdate(const PreUpdateParams &inParams) override;

	// Saving / restoring state for replay
	virtual void			SaveState(StateRecorder &inStream) const override;
	virtual void			RestoreState(StateRecorder &inStream) override;

private:
	float					mTime = 0.0f;
	Ref<RagdollSettings>	mRagdollSettings;
	Ref<Ragdoll>			mRagdoll;
	Ref<SkeletalAnimation>	mAnimation;
	SkeletonPose			mPose;
};
