Dark cropper
============

A tool to aid mass creation of optionally-dark wallpapers, with the goal of
easing the pain of converting non-standard sized images to screen size.

Prerequisites
=============

* Qt5 sdk
* Imagemagick
* waifu2x-converter-cpp with models at ~/.waifu2x/models

Controls
========

* **Left mouse button + drag**: move
* **Middle mouse button + up/down**: scale
* **Right moust button + left/right**: rotate
* **Return**: export current image
* **Q**: go back to main dialog
* **S**: Skip this file
* **D**: Double source resolution (use when zooming >100%)
* **N**: Toggle perceived noise level (used during image doubling)
* **M**: Toggle multiply mode
* **W**: Fit to width
* **H**: Fit to height
* **1**: Reset scale
* **2**: Reset rotation
* **3**: Reset location


Note that exporting a single image (or the last image) will return to the main
dialog while imagemagick is still running.  Please wait a few seconds before
exiting.
