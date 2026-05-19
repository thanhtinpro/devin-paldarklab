PALDARK W37-38 — /Game/Paldark/Inventory/Containers/ directory.

This directory hosts item data assets whose payload is a *nested
inventory* — that is, items that carry other items inside them. The
W37-38 fragment driving this behaviour is
`UPaldarkItemFragment_Container`; the host `FPaldarkInventoryEntry`
gains a `TArray<FPaldarkInventoryEntry> InnerEntries` member that
holds the nested rows. Replication is one level deep only — nesting
containers inside containers is intentionally NOT supported (the W47
FastArraySerializer pass will revisit this, see W37-38 plan).

Designer authoring contract (W37-38):

  1. Author DA_Item_LootCrate.uasset (or similar) with these fragments:
       UPaldarkItemFragment_Stackable  (MaxStackSize=1).
       UPaldarkItemFragment_Weight     (WeightKg=3.0; the container's
                                        own weight, separate from
                                        contents).
       UPaldarkItemFragment_Container  (InnerMaxWeightKg=5.0,
                                        InnerMaxSlots=4; runtime
                                        clamp caps slots at 16).
     ItemTag = (any Paldark.Item.* tag — pick a Resource subtag).

  2. Optionally seed the crate at spawn time via the W33-34
     `APaldarkLootBag` (server-side `InitializeContents` API).
     `UPaldarkPlayerInventoryComponent` does not currently expose a
     direct "AddInner" API in W37-38 — designer fills the InnerEntries
     array via a Blueprint event or a future authored sub-system.

  3. Designer test: `Paldark.Inventory.GiveAll` adds 1 of every item,
     including the crate; `Paldark.Inventory.DumpComposite` then logs
     the nested InnerEntries (or empty if no inner content seeded yet).

Risk notes:

  - The container's InnerMaxSlots is clamped to [0, 16] at runtime so
    a designer accidentally setting `InnerMaxSlots=1024` does not blow
    the DOREPLIFETIME packet past MTU. FastArraySerializer migration is
    deferred to W47 (save-game milestone).

  - Container weight reporting: the inventory component's
    `GetCurrentWeightKg` sums each row's own weight AND every nested
    `InnerEntries` row's weight. The container's own weight is
    counted; an empty crate is not free.

  - Drop-on-death: when the player dies, the W37-38 path flattens the
    crate + InnerEntries into the W33-34 loot bag (one level deep).
    Picking the bag back up will not restore the original nesting —
    this is intentional (UI ergonomics; the W47 save-game preserves
    the tree).
