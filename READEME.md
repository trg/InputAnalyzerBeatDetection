# Beat Detection with Cinder and BTrack

This repo is a sample of adding [BTrack](https://github.com/adamstark/BTrack) to [Cinder](https://libcinder.org/docs/index.html) so you can detect beats.

It uses Cinder's [MonitorSpectralNode](https://libcinder.org/docs/classcinder_1_1audio_1_1_monitor_spectral_node.html) for buffer values; though it's possible other node types will work as well.  I did not have immediate success using the AudioBufferNode but I'd start there if you don't want to import the monitor node.

BTrack requires a bit of work to get going but not much; you should be able to `brew install libsamplerate` and then copy the BTrack files into your project.

The non-BTrack code is mostly from Cinder's [InputAnalyzer](https://github.com/cinder/Cinder/tree/master/samples/_audio/InputAnalyzer) sample code.

Caveat emptor; I'm not a professional C++ engineer, there may be bugs or memory issues or I might not have chosen good frame size values.