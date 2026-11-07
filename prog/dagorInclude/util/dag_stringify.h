//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_enhanced_barrier.h>
#include <drv/3d/dag_sampler.h>

inline const char *to_string(d3d::MipMapMode m)
{
  switch (m)
  {
    case d3d::MipMapMode::Disabled: return "Disabled";
    case d3d::MipMapMode::Point: return "Point";
    case d3d::MipMapMode::Linear: return "Linear";
    default: return "Unknown";
  }
}

inline const char *to_string(d3d::FilterMode f)
{
  switch (f)
  {
    case d3d::FilterMode::Disabled: return "Disabled";
    case d3d::FilterMode::Point: return "Point";
    case d3d::FilterMode::Linear: return "Linear";
    case d3d::FilterMode::Best: return "Best";
    case d3d::FilterMode::Compare: return "Compare";
    default: return "Unknown";
  }
}

inline const char *to_string(d3d::AddressMode a)
{
  switch (a)
  {
    case d3d::AddressMode::Wrap: return "Wrap";
    case d3d::AddressMode::Mirror: return "Mirror";
    case d3d::AddressMode::Clamp: return "Clamp";
    case d3d::AddressMode::Border: return "Border";
    case d3d::AddressMode::MirrorOnce: return "MirrorOnce";
    default: return "Unknown";
  }
}

inline const char *to_string(d3d::BorderColor::Color c)
{
  switch (c)
  {
    case d3d::BorderColor::Color::TransparentBlack: return "TransparentBlack";
    case d3d::BorderColor::Color::OpaqueBlack: return "OpaqueBlack";
    case d3d::BorderColor::Color::OpaqueWhite: return "OpaqueWhite";
    default: return "Custom";
  }
}

inline const char *to_string(d3d::AccessFlag flag)
{
  switch (flag)
  {
    case d3d::AccessFlag::NoAccess: return "NoAccess";
    case d3d::AccessFlag::IndirectArgument: return "IndirectArgument";
    case d3d::AccessFlag::VertexBuffer: return "VertexBuffer";
    case d3d::AccessFlag::IndexBuffer: return "IndexBuffer";
    case d3d::AccessFlag::ConstantBuffer: return "ConstantBuffer";
    case d3d::AccessFlag::RenderTargetRead: return "RenderTargetRead";
    case d3d::AccessFlag::RenderTargetWrite: return "RenderTargetWrite";
    case d3d::AccessFlag::UnorderedAccess: return "UnorderedAccess";
    case d3d::AccessFlag::DepthStencilWrite: return "DepthStencilWrite";
    case d3d::AccessFlag::DepthStencilRead: return "DepthStencilRead";
    case d3d::AccessFlag::InputAttachment: return "InputAttachment";
    case d3d::AccessFlag::ShaderResource: return "ShaderResource";
    case d3d::AccessFlag::CopyRead: return "CopyRead";
    case d3d::AccessFlag::CopyWrite: return "CopyWrite";
    case d3d::AccessFlag::BlitRead: return "BlitRead";
    case d3d::AccessFlag::BlitWrite: return "BlitWrite";
    case d3d::AccessFlag::ResolveRead: return "ResolveRead";
    case d3d::AccessFlag::ResolveWrite: return "ResolveWrite";
    case d3d::AccessFlag::ClearWrite: return "ClearWrite";
    case d3d::AccessFlag::ClearTargetWrite: return "ClearTargetWrite";
    case d3d::AccessFlag::ShadingRate: return "ShadingRate";
    default: return "Unknown";
  }
}

inline const char *to_string(d3d::TextureLayout layout)
{
  switch (layout)
  {
    case d3d::TextureLayout::Undefined: return "Undefined";
    case d3d::TextureLayout::GenericRead: return "GenericRead";
    case d3d::TextureLayout::RenderTarget: return "RenderTarget";
    case d3d::TextureLayout::UnorderedAccess: return "UnorderedAccess";
    case d3d::TextureLayout::DepthRwStencilRw: return "DepthRwStencilRw";
    case d3d::TextureLayout::DepthRwStencilRo: return "DepthRwStencilRo";
    case d3d::TextureLayout::DepthRoStencilRw: return "DepthRoStencilRw";
    case d3d::TextureLayout::DepthRoStencilRo: return "DepthRoStencilRo";
    case d3d::TextureLayout::DepthRw: return "DepthRw";
    case d3d::TextureLayout::DepthRo: return "DepthRo";
    case d3d::TextureLayout::ShaderResource: return "ShaderResource";
    case d3d::TextureLayout::CopySource: return "CopySource";
    case d3d::TextureLayout::CopyDest: return "CopyDest";
    case d3d::TextureLayout::BlitSource: return "BlitSource";
    case d3d::TextureLayout::BlitDest: return "BlitDest";
    case d3d::TextureLayout::ResolveSource: return "ResolveSource";
    case d3d::TextureLayout::ResolveDest: return "ResolveDest";
    case d3d::TextureLayout::ClearDest: return "ClearDest";
    case d3d::TextureLayout::ShadingRateSource: return "ShadingRateSource";
    default: return "Unknown";
  }
}
