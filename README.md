texgenpack
==========

Compress, decompress and convert texure files using a genetic algorithm.
Supports KTX and DDS containers, and texture formats such as the DXT
family, ETC1/2, BC6/BC7 etc. Contains graphics user interface (designed
for Linux). Advantage is versatility and extendability, disadvantage is
slowness compared to less general texture compressors.

Primarily developed for Linux, it uses the libfgen genetic algorithm
library. It also contains a Windows port (which requires some
configuration to provide pthreads and GTK+ for GUI).

See the file README for more detailed instructions.

