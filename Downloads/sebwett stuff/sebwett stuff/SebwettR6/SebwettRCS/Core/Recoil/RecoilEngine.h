#pragma once

namespace RecoilEngine {

// Start the background recoil thread
void Start();

// Stop the background recoil thread
void Stop();

// Enable or disable recoil compensation
void SetEnabled(bool enabled);
bool IsEnabled();

// Set active recoil parameters.
//   recoilX   - horizontal compensation (negative = left, positive = right)
//   recoilY   - vertical compensation   (positive = down)
//   onlyADS   - only apply while right mouse button is held
//   onlyR6    - only apply while Rainbow Six Siege is the foreground window
void SetParams(float recoilX, float recoilY, bool onlyADS, bool onlyR6);

} // namespace RecoilEngine
