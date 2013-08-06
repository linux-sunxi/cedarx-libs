LIBVECORE DEMO README
-------------

1) Documentation
----------------

* Read the documentation in ./doc directory.
*
* 1. "PMP file format.doc": introduce the file format of PMP file, helpful when 
*                           you are reading the pmp_file_parser source code.
*                           
* 2. "Libve User Guide.pkf": a guide for using the libve.a library.
*

2) Directory
------------
*
* 'libvecore_demo'
*       |
*       |---- 'pmp_file_parser': source code of parsing video data from a PMP file.
*       |
*       |
*       |---- 'vdecoder': source code of bitstream/picture frame buffer management and decoding using libvecore.a.
*                 |
*                 |
*                 |---- 'vbv': source code of VBV module, responsible of bitstream frame management.
*                 |
*                 |---- 'fbm': source code of FBM module, responsible of frame buffer management.
*                 |
*                 |---- 'libvecore': libvecore.a library(It is libve).
*                 |
*                 |---- 'libcedarv': the controlling logic of using VBV, FBM and libvecore.a.
*                 |
*                 |---- 'adapter': the implement of interfacies(IVBV/IFBM/IOS/IVE) for libvecore.a.
*                           |
*                           |---- cdxalloc: libcedarxalloc.a, I use this library to manage the memory allocation and free.
*


2) Note
------------
* 1. As the memory for VE is provided and mapped by the cedar_dev.ko driver, we implement a library named
*    'libcedarxalloc.a' to manage the allocation and free operation on this memory space. The 'palloc' and
*    'pfree' method used by libvecore.a are mapped to relative methods in this library. For details, see
*    libve_adapter.c.
*
* 2. Test files for this demo are in the outside directory 'libvecore_demo_test_files'. Currently this demo
*    supports PMP files only.
*
* 3. Compile this demo under folder "android2.3.4\frameworks\base\".
*
*