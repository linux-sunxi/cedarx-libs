LIBVECORE DEMO README
-------------

1) Documentation
----------------

* Read the documentation in ./doc directory.
*
* 1. "PMP file format.doc": introduce the file format of PMP file, helpful when 
*                           you are reading the pmp_file_parser source code.
*                           
* 2. "Libve User Guide.pkf": a guide for using the libvecore.so library.
*

2) Directory
------------
*
* 'libvecore_demo'
*       |
*       |---- 'include': source code of include file.
*       |
*       |---- 'sunxi_allocator': source code of memory allocation and free.
*       |
*       |---- 'file_parser': source code of parsing video data from a PMP file.
*       |
*       |---- 'stream_parser': source code of parsing video data from stream.
*       |
*       |---- 'vdecoder': source code of bitstream/picture frame buffer management and decoding.
*                 |
*                 |---- 'vbv': source code of VBV module, responsible of bitstream frame management.
*                 |
*                 |---- 'fbm': source code of FBM module, responsible of frame buffer management.
*                 |
*                 |---- 'libve': libvecore.so library.
*                 |
*                 |---- 'adapter': the implement of interfacies(IVBV/IFBM/IOS/IVE) for libvecore.so.


