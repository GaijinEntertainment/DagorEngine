#pragma once

namespace das {
   template <> struct das_alias<ax::NodeEditor::PinId> : das_alias_ref<ax::NodeEditor::PinId,int32_t> {
        static __forceinline ax::NodeEditor::PinId to ( int32_t value ) {
            return ax::NodeEditor::PinId(value);
        }
   };
   template <> struct das_alias<ax::NodeEditor::LinkId> : das_alias_ref<ax::NodeEditor::LinkId,int32_t> {
        static __forceinline ax::NodeEditor::LinkId to ( int32_t value ) {
            return ax::NodeEditor::LinkId(value);
        }
   };
   template <> struct das_alias<ax::NodeEditor::NodeId> {
        static __forceinline ax::NodeEditor::NodeId to ( int32_t value ) {
            return ax::NodeEditor::NodeId(value);
        }
   };
}


