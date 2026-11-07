# AnimTree Variations: a2dOverride

One animTree can serve several characters that share its structure but play
different animations, for example a male and a female body. A variation is an
animTree asset that includes the base tree and remaps animation names in an
`a2dOverride{}` block:

```blk
// female_animbnl.animTree.blk
include "#/develop/assets/common/gameRes/characters/soldier_animbnl.animTree.blk"

a2dOverride{
  walk_0:t="walk_0_female"
  lower_run:t="lower_run_female"
}
```

Each parameter maps the `a2d` name of an `AnimBlendNodeLeaf` block in the base
tree to the replacement a2d asset. Leaves that are not listed keep the base
animation. The base tree needs no change, and a leaf added to the base tree
reaches every variation.

## How It Works

The animTree exporter in daBuild applies the table when it resolves the `a2d`
reference of each leaf. The variation references only the replacement a2d
assets, and the exported resource has the same format as any other animTree.
The runtime never sees the block.

The animTree editor in AssetViewer shows and plays the `a2d` written in each
leaf block. In a variation, it plays the base animation of an overridden leaf.

The key range, sync time and IRQ positions of a leaf are resolved from the
note track labels of its a2d when the anim graph is built, not from the tree.
A replacement a2d must therefore carry the same labels that the leaf
references (`key_start`, `key_end`, `sync_time`, `irq{ key }`, `labels{}`)
and the same `makeAdditive` setting.

## Rules

- Keys and values are a2d asset names. A later entry for the same key replaces
  an earlier one, across all `a2dOverride{}` blocks of the asset.
- A replacement is looked up by the leaf's own `a2d` name. Overrides do not
  chain.
- Export errors: an entry that is not a string param, a replacement asset that
  does not exist, an entry that matches no leaf, and a `makeAdditive` mismatch
  between the original and the replacement.
- A label that is missing in the replacement a2d is reported at runtime when
  the graph is built, not at export.
