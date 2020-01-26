//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "SkeletonCustom.h"

extern int psSkeletonUpdate;

#ifdef DEBUG
void check_kinematics(CKinematics* _k, LPCSTR s);
#endif

void CKinematics::CalculateBones(BOOL bForceExact)
{
    // early out.
    // check if the info is still relevant
    // skip all the computations - assume nothing changes in a small period of time :)
    if (RDEVICE.dwTimeGlobal == UCalc_Time)
        return; // early out for "fast" update
    UCalc_mtlock lock;
    OnCalculateBones();
    if (!bForceExact && (RDEVICE.dwTimeGlobal < (UCalc_Time + UCalc_Interval)))
        return; // early out for "slow" update
    if (Update_Visibility)
        Visibility_Update();

    _DBG_SINGLE_USE_MARKER;
    // here we have either:
    //	1:	timeout elapsed
    //	2:	exact computation required
    UCalc_Time = RDEVICE.dwTimeGlobal;

// exact computation
// Calculate bones
#ifdef DEBUG
    RImplementation.BasicStats.Animation.Begin();
#endif

    Bone_Calculate(bones->at(iRoot), &Fidentity);
#ifdef DEBUG
    check_kinematics(this, dbg_name.c_str());
    RImplementation.BasicStats.Animation.End();
#endif
    VERIFY(LL_GetBonesVisible() != 0);
    // Calculate BOXes/Spheres if needed
    UCalc_Visibox++;
    if (UCalc_Visibox >= psSkeletonUpdate)
    {
        // mark
        UCalc_Visibox = -(::Random.randI(psSkeletonUpdate - 1));

        // the update itself
        Fbox Box;
        Box.invalidate();
        for (u32 b = 0; b < bones->size(); b++)
        {
            if (!LL_GetBoneVisible(u16(b)))
                continue;
            Fobb& obb = (*bones)[b]->obb;
            Fmatrix& Mbone = bone_instances[b].mTransform;
            Fmatrix Mbox;
            obb.xform_get(Mbox);
            Fmatrix X;
            X.mul_43(Mbone, Mbox);
            Fvector& S = obb.m_halfsize;

            Fvector P, A;
            A.set(-S.x, -S.y, -S.z);
            X.transform_tiny(P, A);
            Box.modify(P);
            A.set(-S.x, -S.y, S.z);
            X.transform_tiny(P, A);
            Box.modify(P);
            A.set(S.x, -S.y, S.z);
            X.transform_tiny(P, A);
            Box.modify(P);
            A.set(S.x, -S.y, -S.z);
            X.transform_tiny(P, A);
            Box.modify(P);
            A.set(-S.x, S.y, -S.z);
            X.transform_tiny(P, A);
            Box.modify(P);
            A.set(-S.x, S.y, S.z);
            X.transform_tiny(P, A);
            Box.modify(P);
            A.set(S.x, S.y, S.z);
            X.transform_tiny(P, A);
            Box.modify(P);
            A.set(S.x, S.y, -S.z);
            X.transform_tiny(P, A);
            Box.modify(P);
        }
        if (bones->size())
        {
            // previous frame we have updated box - update sphere
            vis.box.vMin = (Box.vMin);
            vis.box.vMax = (Box.vMax);
            vis.box.getsphere(vis.sphere.P, vis.sphere.R);
        }
#ifdef DEBUG
        // Validate
        VERIFY3(_valid(vis.box.vMin) && _valid(vis.box.vMax), "Invalid bones-xform in model", dbg_name.c_str());
        if (vis.sphere.R > 1000.f)
        {
            for (u16 ii = 0; ii < LL_BoneCount(); ++ii)
            {
                Fmatrix tr;
                tr = LL_GetTransform(ii);
                Log("bone ", LL_BoneName_dbg(ii));
                Log("bone_matrix", tr);
            }
            Log("end-------");
        }
        VERIFY3(vis.sphere.R < 1000.f, "Invalid bones-xform in model", dbg_name.c_str());
#endif
    }

    //
    if (Update_Callback)
        Update_Callback(this);
}

#ifdef DEBUG
void check_kinematics(CKinematics* _k, LPCSTR s)
{
    CKinematics* K = _k;
    Fmatrix& MrootBone = K->LL_GetBoneInstance(K->LL_GetBoneRoot()).mTransform;
    if (MrootBone.c.y > 10000)
    {
        Msg("all bones transform:--------[%s]", s);

        for (u16 ii = 0; ii < K->LL_BoneCount(); ++ii)
        {
            Fmatrix tr;

            tr = K->LL_GetTransform(ii);
            Log("bone ", K->LL_BoneName_dbg(ii));
            Log("bone_matrix", tr);
        }
        Log("end-------");
        VERIFY3(0, "check_kinematics failed for ", s);
    }
}
#endif

// Добавить скриптовое смещение для кости --#SM+#--
void CKinematics::LL_AddTransformToBone(KinematicsABT::additional_bone_transform& offset)
{
    m_bones_offsets.push_back(offset);
}

/* Обнулить скриптовое смещение для конкретной кости или всех сразу --#SM+#--
   bone_id - ID кости, с которой нужно снять смещения (BI_NONE если для всех)
   source_id - Источник смещения, по которому нужно их фильтровать (_ANY если снять со всех источников)
*/
void CKinematics::LL_ClearAdditionalTransform(u16 bone_id, KinematicsABT::SourceID source_id)
{
    BONE_TRANSFORM_VECTOR_IT it = m_bones_offsets.begin();
    while (it != m_bones_offsets.end())
    {
        bool bCond_1 = (bone_id == BI_NONE) || (it->m_bone_id == bone_id);
        bool bCond_2 = (source_id == KinematicsABT::SourceID::_ANY) || (it->m_source_id == source_id);
        if (bCond_1 && bCond_2)
        {
            it = m_bones_offsets.erase(it);
        }
        else
            ++it;
    }
}

/* Выставить текущее значение фактора интервала, которое используется для работы Fade In\Out эффекта в --#SM+#--
   CKinematics::CalculateBonesAdditionalTransforms
*/
void CKinematics::LL_SetAdditionalTransformCurIntervalFactor(float fCurIntFactor) //--#SM+#--
{
    m_bones_offsets_cur_interval_f = clampr(fCurIntFactor, 0.0f, 1.0f);
}

void CKinematics::BuildBoneMatrix(
    const CBoneData* bd, CBoneInstance& bi, const Fmatrix* parent, u8 channel_mask /*= (1<<0)*/)
{
    bi.mTransform.mul_43(*parent, bd->bind_transform);
    CalculateBonesAdditionalTransforms(bd, bi, parent, channel_mask); //--#SM+#--
}

// Добавляем константные смещения к нужным костям --#SM+#--
void CKinematics::CalculateBonesAdditionalTransforms(
    const CBoneData* bd, CBoneInstance& bi, const Fmatrix* parent, u8 channel_mask /* = (1<<0)*/)
{
    //--> Применяем модификаторы смещения
    for (auto& it : m_bones_offsets)
    {
        if (it.m_bone_id == bd->GetSelfID())
        {
            Fmatrix* mxTransformLocal = &it.m_transform_local;
            Fmatrix* mxTransformGlobal = &it.m_transform_global;

            // Fade In\Out effect
            float fPower = 1.0f; //--> Сила эффекта смещения

            if (it.m_fade.IsEnabled())
            {
                // С учётом текущего фактора интервала высчитываем текущую силу смещени кости относительно базового
                const bool& bInverseMode = it.m_fade.IsInverted();
                const Fvector2& vIntervalFadeIn = it.m_fade.GetIntervalFadeIn();
                const Fvector2& vIntervalFadeOut = it.m_fade.GetIntervalFadeOut();

                if (m_bones_offsets_cur_interval_f == 0.0f)
                { //--> Коэфицент интервала (КИ) находится в 0 (значение по умолчанию), оптимизируем код
                    if (bInverseMode)
                    { //--> Для инвертированного режима на таких значениях мы полностью игнорируем смещение
                        continue;
                    }
                    else
                    { //--> Для обычного режима применяем смещение на полную силу
                        goto skip_fade_effect;
                    }
                }
                else
                { //--> Коэфицент интервала (КИ) не равен значению по умолчанию, вычисляем силу смещения
                    // Обычный режим: ослаблаяем смещение в Fade In и восстанавливаем в Fade Out

                    float& fKI = m_bones_offsets_cur_interval_f; 

                    //--> КИ вне Fade In\Out интервалов
                    fPower = 1.0f;

                    //--> КИ в Fade In интервале
                    if (fKI >= vIntervalFadeIn.x && fKI < vIntervalFadeIn.y)
                    {
                        fPower = 1.0f - (((fKI - vIntervalFadeIn.x) / (vIntervalFadeIn.y - vIntervalFadeIn.x)));
                    }
                    
                    //--> КИ между Fade In End и Fade Out Start
                    if (fKI >= vIntervalFadeIn.y && fKI < vIntervalFadeOut.x)
                    {
                        fPower = 0.0f;
                    }

                    //--> КИ в Fade Out интервале
                    if (fKI >= vIntervalFadeOut.x && fKI < vIntervalFadeOut.y)
                    {
                        fPower = ((fKI - vIntervalFadeOut.x) / (vIntervalFadeOut.y - vIntervalFadeOut.x));
                    }

                    // Инвертированный режим: усиливаем смещение в Fade In и ослабляем в Fade Out
                    if (bInverseMode)
                    {
                        fPower = 1.0f - fPower;
                    }
                }
            }

            if (fPower < 1.0f)
            { //--> Применяем эффект затухания с учётом его текущей силы
                Fmatrix mxIdentity;
                mxIdentity.identity();

                mxTransformLocal = &Fmatrix(*mxTransformLocal);
                mxTransformLocal->i.lerp(mxIdentity.i, mxTransformLocal->i, fPower);
                mxTransformLocal->j.lerp(mxIdentity.j, mxTransformLocal->j, fPower);
                mxTransformLocal->k.lerp(mxIdentity.k, mxTransformLocal->k, fPower);
                mxTransformLocal->c.lerp(mxIdentity.c, mxTransformLocal->c, fPower);

                mxTransformGlobal = &Fmatrix(*mxTransformGlobal);
                mxTransformGlobal->i.lerp(mxIdentity.i, mxTransformGlobal->i, fPower);
                mxTransformGlobal->j.lerp(mxIdentity.j, mxTransformGlobal->j, fPower);
                mxTransformGlobal->k.lerp(mxIdentity.k, mxTransformGlobal->k, fPower);
                mxTransformGlobal->c.lerp(mxIdentity.c, mxTransformGlobal->c, fPower);
            }

skip_fade_effect:

            // Rotation
            //--> Запоминаем позицию кости
            const Fvector vOldPos = bi.mTransform.c;
            //--> Поворот в локальных координатах
            bi.mTransform.mulB_43(*mxTransformLocal);
            //--> Поворот в мировых координатах
            bi.mTransform.mulB_43(*mxTransformGlobal);
            //--> Восстанавливаем позицию кости
            bi.mTransform.translate_over(vOldPos);

            // Position
            //--> Смещение в локальных координатах
            Fvector vTemp;
            bi.mTransform.transform_dir(vTemp, mxTransformLocal->c);
            bi.mTransform.c.add(vTemp);
            //--> Смещение в мировых координатах
            bi.mTransform.c.add(mxTransformGlobal->c);
        }
    }
}

void CKinematics::CLBone(const CBoneData* bd, CBoneInstance& bi, const Fmatrix* parent, u8 channel_mask /*= (1<<0)*/)
{
    u16 SelfID = bd->GetSelfID();

    if (LL_GetBoneVisible(SelfID))
    {
        if (bi.callback_overwrite())
        {
            if (bi.callback())
                bi.callback()(&bi);
        }
        else
        {
            BuildBoneMatrix(bd, bi, parent, channel_mask);
#ifndef MASTER_GOLD
            R_ASSERT2(_valid(bi.mTransform), "anim kils bone matrix");
#endif // #ifndef MASTER_GOLD
            if (bi.callback())
            {
                bi.callback()(&bi);
#ifndef MASTER_GOLD
                R_ASSERT2(_valid(bi.mTransform), make_string("callback kils bone matrix bone: %s ", bd->name.c_str()));
#endif // #ifndef MASTER_GOLD
            }
        }
        bi.mRenderTransform.mul_43(bi.mTransform, bd->m2b_transform);
    }
}

void CKinematics::Bone_GetAnimPos(Fmatrix& pos, u16 id, u8 mask_channel, bool ignore_callbacks)
{
    R_ASSERT(id < LL_BoneCount());
    CBoneInstance bi = LL_GetBoneInstance(id);
    BoneChain_Calculate(&LL_GetData(id), bi, mask_channel, ignore_callbacks);
#ifndef MASTER_GOLD
    R_ASSERT(_valid(bi.mTransform));
#endif
    pos.set(bi.mTransform);
}

void CKinematics::Bone_Calculate(CBoneData* bd, Fmatrix* parent)
{
    u16 SelfID = bd->GetSelfID();
    CBoneInstance& BONE_INST = LL_GetBoneInstance(SelfID);
    CLBone(bd, BONE_INST, parent, u8(-1));
    // Calculate children
    for (xr_vector<CBoneData*>::iterator C = bd->children.begin(); C != bd->children.end(); C++)
        Bone_Calculate(*C, &BONE_INST.mTransform);
}

void CKinematics::BoneChain_Calculate(const CBoneData* bd, CBoneInstance& bi, u8 mask_channel, bool ignore_callbacks)
{
    u16 SelfID = bd->GetSelfID();
    // CBlendInstance& BLEND_INST	= LL_GetBlendInstance(SelfID);
    // CBlendInstance::BlendSVec &Blend = BLEND_INST.blend_vector();
    // ignore callbacks
    BoneCallback bc = bi.callback();
    BOOL ow = bi.callback_overwrite();
    if (ignore_callbacks)
    {
        bi.set_callback(bi.callback_type(), nullptr, bi.callback_param(), 0);
    }
    if (SelfID == LL_GetBoneRoot())
    {
        CLBone(bd, bi, &Fidentity, mask_channel);
        // restore callback
        bi.set_callback(bi.callback_type(), bc, bi.callback_param(), ow);
        return;
    }
    u16 ParentID = bd->GetParentID();
    R_ASSERT(ParentID != BI_NONE);
    CBoneData* ParrentDT = &LL_GetData(ParentID);
    CBoneInstance parrent_bi = LL_GetBoneInstance(ParentID);
    BoneChain_Calculate(ParrentDT, parrent_bi, mask_channel, ignore_callbacks);
    CLBone(bd, bi, &parrent_bi.mTransform, mask_channel);
    // restore callback
    bi.set_callback(bi.callback_type(), bc, bi.callback_param(), ow);
}
