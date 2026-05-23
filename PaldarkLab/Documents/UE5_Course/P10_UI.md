# P10 — UI (UMG + CommonUI)

> Tự soạn từ: [01] Tom Looman, [09] Inventory, [10] Blaster, [11] Aura, [14] Lyra, [15] Lyra RPG.
>
> UI là cảm nhận đầu tiên — Aura's Widget Controller MVC + Lyra's CommonUI Activatable Stack = professional polish.

---

## Pillar Identity

**Cái gì:** Build UMG widget + CommonUI activatable + binding + drag-drop + focus management.

**Scope:**
- ✅ UUserWidget (UMG) basic.
- ✅ Widget Controller MVC pattern (Aura).
- ✅ CommonUI ActivatableWidget stack (Lyra).
- ✅ Async UI loading + image preload.
- ✅ Drag-drop UMG operation.
- ✅ Indicator Manager (3D world → 2D screen).
- ✅ Focus management (controller).
- ✅ Rich text + interactive markup.
- ✅ Anti-pattern: god-widget (RoN 5K LOC).

**Out of scope:**
- ❌ Slate (engine-level UI).
- ❌ Backend data fetch (P18).

---

## PALDARK cần gì từ pillar này?

| UI Component | Pattern | Location |
|-------------|---------|----------|
| HUD (always-on) | Widget Controller | World viewport |
| Pal Command Wheel | Activatable Widget | Modal overlay |
| Inventory + Equipment | Activatable Widget + Drag-drop | Modal full-screen |
| Hub Menu (shop, quest) | Activatable Widget Stack | Hub map |
| Pal Stats Panel | Widget Controller | Inventory subtab |
| Sanity Indicator | Custom paint canvas | HUD overlay |
| Extract Beacon (3D → 2D) | Indicator Manager | World |
| Pause Menu | Activatable Widget | Modal overlay |
| FrontEnd (Main Menu) | Activatable Widget Stack | Pre-game level |

---

## Sources Matrix

| Source | Coverage | Lesson lấy từ đó |
|--------|----------|------------------|
| [01] Tom Looman | 🟡 | L1 UMG basic + binding |
| [09] Inventory | 🟢 | L2 Drag-drop operation |
| [10] Blaster | 🟡 | L3 HUD overlay (compass, ammo) |
| [11] Aura | 🟢 | L4 Widget Controller MVC (CORE) |
| [14] Lyra | 🟢 | L5 CommonUI Activatable Stack, L6 Indicator Manager |
| [15] Lyra RPG | 🟡 | L7 FrontEnd Menu + Quick Bar |

---

## Prerequisite

- P02 (HUD class owned by PC).
- P04 (Input → UI focus swap via IMC).

---

## Lessons

### L1 — UMG basic + Binding 🧪

**Goal:** Tạo widget, bind text/progress bar to variable.

**Concept:**
- `UUserWidget` C++ subclass.
- WBP_* (Widget Blueprint) extends C++ class.
- `BindWidget` UPROPERTY meta — auto-find named widget child.
- Function binding vs Property binding (use Function for perf).

**Source learning path:**
- [01] Tom Looman § HUD chapter.

**API / Class chính:**
- `UPROPERTY(meta = (BindWidget)) UProgressBar* HealthBar;`
- `UTextBlock::SetText(FText)`.
- `UProgressBar::SetPercent(float)`.

**Practice exercise (2 hour):**
1. `UPaldarkLabHUDWidget` C++ class.
2. `BindWidget` HealthBar + StaminaBar + ScoreText.
3. `NativeOnInitialized` subscribe `OnHealthChanged` delegate from PlayerState.
4. WBP_PaldarkLabHUD extends C++ class, design layout.
5. PIE: take damage → HealthBar update.

**PALDARK take-away:**
- 🟢 Mọi widget có C++ base + BP design layer.

**Apply ở week:** Roadmap week 5-6.

---

### L2 — Drag-Drop UMG Operation (Inventory) 🧪

**Goal:** Drag item icon từ slot A → slot B.

**Concept:**
- `UDragDropOperation` subclass — payload class.
- Source widget override `OnDragDetected()` → `UWidgetBlueprintLibrary::CreateDragDropOperation()`.
- Target widget override `OnDrop()` → cast Operation → handle.
- `NativePaint` để vẽ preview.

**Source learning path:**
- [09] Inventory § Drag-Drop chapter.
- Cross-ref: `Documents/GameDesign/09-Inventory_GDD.md` § Phase 5 ADR-4.

**API / Class chính:**
- `UDragDropOperation`
- `OnDragDetected(FGeometry, FPointerEvent, UDragDropOperation*&)`
- `OnDrop(FGeometry, FDragDropEvent, UDragDropOperation*)`

**Practice exercise (3 hour):**
1. `UPaldarkItemSlotWidget` represents 1 inventory slot.
2. Drag detect → `UInventoryDragOperation` payload = ItemID + SourceSlotIndex.
3. Drop → swap or merge stack.
4. PIE: drag item across slot.

**PALDARK take-away:**
- 🟢 Inventory drag-drop pattern copy nguyên từ course [09].

**Apply ở week:** Roadmap week 19.

---

### L3 — HUD overlay (Blaster) 🧪

**Goal:** Crosshair + Ammo + Compass HUD elements.

**Concept:**
- HUD class draws or owns widget tree.
- Compass: rotate texture by yaw delta.
- Crosshair: 5 sprite (T/L/R/B/Center) spread based on velocity.

**Source learning path:**
- [10] Blaster § HUD chapter.

**Practice exercise:**
1. `UPaldarkCrosshairWidget` 5 sprite.
2. `Tick` update sprite offset based on `Owner->GetVelocity()`.

**PALDARK take-away:**
- 🟡 Crosshair spread pattern dùng cho PALDARK weapon shooting.

**Apply ở week:** Roadmap week 17.

---

### L4 — Widget Controller MVC (Aura CORE) 🧪

**Goal:** Tách widget khỏi business logic — MVC pattern.

**Concept:**
- **Model:** Game data (PlayerState, ASC attribute).
- **View:** UMG widget (passive — only render).
- **Controller:** `UAuraWidgetController` subclass — listen ASC delegate, broadcast to widget.
- Widget bind to controller's delegate, not directly to ASC.
- Ưu: swap model/view independently, easy test.

**Source learning path:**
- [11] Aura § Widget Controller chapter — best implementation in all 15 sources.
- Cross-ref: `Documents/GameDesign/11-Aura_RPG_GDD.md` ADR-5.

**API / Class chính:**
- `UAuraWidgetController` base.
- `UOverlayWidgetController` subclass cho HUD.
- `UAttributeMenuWidgetController` cho attribute UI.
- `BroadcastInitialValues()`.

**Practice exercise (4 hour):**
1. `UPaldarkLabWidgetController` base — owns ASC ref + PlayerState ref.
2. `UPaldarkLabOverlayWidgetController` subclass — listen `OnHealthChanged`, broadcast to widget.
3. `UPaldarkLabHUDWidget` bind to Controller's delegate, not ASC directly.
4. PIE: damage → Controller catch → broadcast → widget update.

**PALDARK take-away:**
- 🟢 **CORE** — mỗi UI section có Widget Controller:
  - `UPaldarkOverlayWidgetController` — HUD (Health/Sanity/Pal/Score).
  - `UPaldarkInventoryWidgetController` — Inventory grid.
  - `UPaldarkPalRosterWidgetController` — Pal team.
  - `UPaldarkBondRitualWidgetController` — Bond UI.

**Apply ở week:** Roadmap week 5-6.

---

### L5 — CommonUI Activatable Widget Stack (Lyra CORE) 🧪

**Goal:** Modal UI stack — Inventory → Pal Detail → Bond Confirm với navigation back.

**Concept:**
- `UCommonActivatableWidget` — widget có Activate/Deactivate lifecycle.
- `UCommonActivatableWidgetStack` container — push/pop stack.
- `UCommonActivatableWidgetSwitcher` — show 1 of N tab.
- Auto-handles input routing (focus, controller).
- Auto-handles back navigation (ESC pop top).

**Source learning path:**
- [14] Lyra § CommonUI chapter.
- [15] Lyra RPG § FrontEnd chapter.
- Cross-ref: `Documents/GameDesign/14-Exploring_Lyra_GDD.md` ADR-2.

**API / Class chính:**
- `UCommonActivatableWidget::ActivateWidget()` / `DeactivateWidget()`.
- `UCommonUIVisibilitySubsystem::PushVisibilityTag()` for hide HUD when menu open.

**Practice exercise (4 hour):**
1. Enable CommonUI plugin.
2. `WBP_PaldarkInventoryActivatable` extends `UCommonActivatableWidget`.
3. `WBP_PaldarkMenuStack` extends `UCommonActivatableWidgetStack`.
4. Press Tab → push Inventory.
5. Click Pal slot → push PalDetail.
6. ESC → pop PalDetail → still see Inventory.
7. ESC → pop Inventory → close all.

**PALDARK take-away:**
- 🟢 **CORE** — FrontEnd + Pause + Inventory + Hub all dùng Activatable Stack.
- 🟢 ESC handling tự động.
- 🟢 Controller navigation built-in.

**Apply ở week:** Roadmap week 19 + 33.

---

### L6 — Indicator Manager (3D → 2D) 🧪

**Goal:** Hiển thị marker trên màn hình cho objective trong world (extract beacon, enemy, ally).

**Concept:**
- `UIndicatorManagerComponent` — register/unregister indicator descriptor.
- `FIndicatorDescriptor` — TargetActor + Widget class + Offset.
- `UIndicatorLayer` (UMG) tự project 3D position → 2D screen + clamp at edge.

**Source learning path:**
- [14] Lyra § Indicator Manager chapter.
- [15] Lyra RPG § Indicator chapter.

**Practice exercise (3 hour):**
1. Enable Lyra IndicatorManager (port to PaldarkLab).
2. `WBP_LabExtractIndicator` icon arrow + distance text.
3. Spawn ExtractBeacon → register descriptor.
4. PIE: indicator follow beacon, clamp at edge khi off-screen.

**PALDARK take-away:**
- 🟢 Extract Beacon + Pal Roster (where Pal is in world) + Enemy ping.

**Apply ở week:** Roadmap week 11 + 33.

---

### L7 — Anti-pattern: god-widget (RoN warning) 🔴

**Goal:** TRÁNH widget > 1000 LOC chứa toàn bộ logic.

**Concept:**
- RoN `SwatCommand` widget = 5K LOC — gồm UI render + command dispatch + AI hint + audio cue.
- Vi phạm SRP — sửa 1 chỗ break 10 chỗ.
- Cách tránh:
  - Widget chỉ render.
  - Logic ở Widget Controller (P10 L4).
  - Mỗi sub-section là sub-widget.

**Source learning path:**
- [04] RoN GDD § Phase 5 anti-pattern.

**PALDARK take-away:**
- 🟢 Mỗi widget ≤ 300 LOC C++/BP.
- 🟢 Sub-widget composition over inheritance.

**Apply ở week:** Mỗi review.

---

## ⚡ Capstone Exercise

**Goal:** PaldarkLab UI hoàn chỉnh.

**Yêu cầu:**
1. HUD (Widget Controller pattern): Health + Stamina + Sanity + PalIcon + Score.
2. Inventory (Activatable + Drag-drop): 24 slot grid.
3. Pal Detail (Activatable, push on stack from Inventory).
4. Indicator Manager: 3 extract beacon trong world.
5. Pause Menu Activatable Stack.

**Acceptance:** ESC navigation; focus controller-friendly; widget Controller listen ASC delegate; no widget > 300 LOC.

**Effort:** 1.5 tuần.

---

## Khi apply vào PALDARK roadmap

| Week | Pillar P10 usage |
|------|------------------|
| W5-6 | HUD + Widget Controller |
| W11 | Extract Indicator Manager |
| W19 | Inventory Activatable + Drag-drop |
| W33 | Lyra adopt — FrontEnd Activatable Stack |
| W34 | Hub menu Activatable Stack |

---

## Anti-patterns (cảnh báo)

- 🔴 God-widget > 1000 LOC (RoN pattern).
- 🔴 Logic trong widget thay vì controller.
- 🔴 Widget bind directly to ASC (skip controller) — coupling.
- 🔴 Native UMG cho modal UI thay vì CommonUI Activatable — focus + back nav fail on controller.
- 🔴 Synchronous load 100 image trong inventory mở → freeze frame. Dùng `AsyncLoadAsset`.

---

## Tham chiếu

- [`Documents/UE5_Core_Pillars.md`](../UE5_Core_Pillars.md) § P10.
- [`Documents/GameDesign/11-Aura_RPG_GDD.md`](../GameDesign/11-Aura_RPG_GDD.md) — Widget Controller.
- [`Documents/GameDesign/14-Exploring_Lyra_GDD.md`](../GameDesign/14-Exploring_Lyra_GDD.md) — CommonUI Stack.
- [`Documents/GameDesign/09-Inventory_GDD.md`](../GameDesign/09-Inventory_GDD.md) — Drag-drop.
