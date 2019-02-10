#ifndef RenderVisual_included
#define RenderVisual_included
#pragma once

class IKinematics;
class IKinematicsAnimated;
class IParticleCustom;
struct vis_data;
struct vis_object_data; //--#SM+#--

class IRenderVisual
{
public:
    virtual ~IRenderVisual() { ; }
    virtual vis_data& getVisData() = 0;
    virtual u32 getType() = 0;

#ifdef DEBUG
    virtual shared_str getDebugName() = 0;
#endif

    virtual IRenderVisual* getSubModel(u8 idx) { return nullptr; } //--#SM+#--
    virtual IKinematics* dcast_PKinematics() { return nullptr; }
    virtual IKinematicsAnimated* dcast_PKinematicsAnimated() { return nullptr; }
    virtual IParticleCustom* dcast_ParticleCustom() { return nullptr; }
    virtual vis_object_data* getObjectData() = 0; //--#SM+#--
    virtual void setObjectData(vis_object_data* pData) = 0; //--#SM+#--
    virtual void cloneObjectData(IRenderVisual* pSrc) = 0; //--#SM+#--
};

#endif //	RenderVisual_included
