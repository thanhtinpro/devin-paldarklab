# 04. Ready or Not — Project (Void Interactive)

| | |
|---|---|
| **Loại** | Project (AAA, community-leaked/study) |
| **Tier** | T6 — Real-Project Reverse Engineering (mức cao nhất) |
| **Studio** | Void Interactive |
| **Trong repo** | `04.ReadyOrNot/` (Documents có 1 GDD .docx, 2 .drawio.png, **1 doc kiến trúc Vietnamese 612 dòng**) |

> Đây là **project AAA shipped** (không phải course). Trang phân tích đầy đủ ở [Projects/ReadyOrNot_Assessment.md](../Projects/ReadyOrNot_Assessment.md).
>
> Kích thước: ~1.500 file C++, ~298.000 LOC. File lớn nhất `PlayerCharacter.cpp` ~10.000 dòng. Pattern: **Monolithic Character** + **Manager** + **Data-Driven** + **Activity-Based AI** + **GameMode/GameState Split**.
>
> Đọc cho phân tích kiến trúc, **không nên copy nguyên pattern** — nhiều quyết định là tech debt do scale.
