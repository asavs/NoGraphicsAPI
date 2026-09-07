# Known driver issues

These are locally reproduced observations, not vendor-confirmed root causes.

## NVIDIA 596.99: stale address-based texture readback

Observed on an RTX 4090 with NVIDIA 596.99, with both sequential and parallel command recording.
An upload through `vkCmdCopyMemoryToImageKHR` followed by `vkCmdCopyImageToMemoryKHR` returned stale
data despite a transfer-write to transfer-read barrier. The issue also reproduced without validation.

A timeline wait between separate upload and readback submissions works. A full Vulkan memory
dependency also worked in isolation; native `vkCmdCopyImageToBuffer2` readback worked in the comparison.
The parallel texture test uses an explicit timeline wait. No driver-specific barrier widening is applied
by the graphics API.
