#pragma once

namespace Menu {

// Call once after Overlay::Initialize() to load settings and seed state.
void Initialize();

// Call every frame between Overlay::BeginFrame() and Overlay::EndFrame().
// Handles keyboard navigation and draws the side panel.
void Render();

} // namespace Menu
