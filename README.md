texgenpack
==========

Compress, decompress and convert texure files using a genetic algorithm.
Supports KTX and DDS containers, and texture formats such as the DXT
family, ETC1/2, BC6/BC7 etc. Contains graphical user interface (designed
for Linux). Advantage is versatility and extendability, disadvantage is
slowness compared to less general texture compressors.

Primarily developed for Linux, it uses the libfgen genetic algorithm
library. It also contains a Windows port (which requires some
configuration to provide pthreads and GTK+ for GUI).

The detex repository (https://github.com/hglm/detex.git) contains a
stand-alone texture decompression library derived from texgenpack that
also contains a viewer program. It superseeds texgenpack for this
purpose as it is more complete and flexible, but does not contain
texture compression functionality. It is the intention that a future
version of texgenpack will make use of detex.

See the file README for more detailed instructions.

