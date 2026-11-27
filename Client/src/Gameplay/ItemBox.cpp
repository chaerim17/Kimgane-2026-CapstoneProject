#include "Pch.h"

#include "ItemBox.h"

#include "../Core/VectorMath.h"

namespace Kimgane::Client
{
void ItemBox::AttachLid(GameObject& lid, const DirectX::XMFLOAT3& lidLocalOffsetM) noexcept
{
    mLid = &lid;
    mLid->GetTransform().SetPositionM(VectorMath::Add(GetTransform().GetPositionM(), lidLocalOffsetM));
}

void ItemBox::Despawn() noexcept
{
    SetActive(false);
    if (mLid != nullptr)
    {
        mLid->SetActive(false);
    }
}
} // namespace Kimgane::Client
