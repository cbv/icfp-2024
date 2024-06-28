// Low-level EXR image file reading and writing.
// From https://github.com/syoyo/tinyexr
// See tinyexr.cc for copyrights.
// Local changes by tom7:
//   - Split into normal .h/.cc
//   - Bake in STB zlib code so that this stands alone
//   - Assume C++ ABI
//   - Make it capable of loading huge files

#ifndef _CC_LIB_TINYEXR_H
#define _CC_LIB_TINYEXR_H

#include <stddef.h>  // for size_t
#include <stdint.h>  // guess stdint.h is available(C99)

#define TINYEXR_SUCCESS (0)
#define TINYEXR_ERROR_INVALID_MAGIC_NUMBER (-1)
#define TINYEXR_ERROR_INVALID_EXR_VERSION (-2)
#define TINYEXR_ERROR_INVALID_ARGUMENT (-3)
#define TINYEXR_ERROR_INVALID_DATA (-4)
#define TINYEXR_ERROR_INVALID_FILE (-5)
#define TINYEXR_ERROR_INVALID_PARAMETER (-6)
#define TINYEXR_ERROR_CANT_OPEN_FILE (-7)
#define TINYEXR_ERROR_UNSUPPORTED_FORMAT (-8)
#define TINYEXR_ERROR_INVALID_HEADER (-9)
#define TINYEXR_ERROR_UNSUPPORTED_FEATURE (-10)
#define TINYEXR_ERROR_CANT_WRITE_FILE (-11)
#define TINYEXR_ERROR_SERIALIZATION_FAILED (-12)
#define TINYEXR_ERROR_LAYER_NOT_FOUND (-13)
#define TINYEXR_ERROR_DATA_TOO_LARGE (-14)

// @note { OpenEXR file format: http://www.openexr.com/openexrfilelayout.pdf }

// pixel type: possible values are: UINT = 0 HALF = 1 FLOAT = 2
#define TINYEXR_PIXELTYPE_UINT (0)
#define TINYEXR_PIXELTYPE_HALF (1)
#define TINYEXR_PIXELTYPE_FLOAT (2)

#define TINYEXR_MAX_HEADER_ATTRIBUTES (1024)
#define TINYEXR_MAX_CUSTOM_ATTRIBUTES (128)

#define TINYEXR_COMPRESSIONTYPE_NONE (0)
#define TINYEXR_COMPRESSIONTYPE_RLE (1)
#define TINYEXR_COMPRESSIONTYPE_ZIPS (2)
#define TINYEXR_COMPRESSIONTYPE_ZIP (3)
#define TINYEXR_COMPRESSIONTYPE_PIZ (4)
#define TINYEXR_COMPRESSIONTYPE_ZFP (128)  // TinyEXR extension

#define TINYEXR_ZFP_COMPRESSIONTYPE_RATE (0)
#define TINYEXR_ZFP_COMPRESSIONTYPE_PRECISION (1)
#define TINYEXR_ZFP_COMPRESSIONTYPE_ACCURACY (2)

#define TINYEXR_TILE_ONE_LEVEL (0)
#define TINYEXR_TILE_MIPMAP_LEVELS (1)
#define TINYEXR_TILE_RIPMAP_LEVELS (2)

#define TINYEXR_TILE_ROUND_DOWN (0)
#define TINYEXR_TILE_ROUND_UP (1)

typedef struct TEXRVersion {
  int version;    // this must be 2
  // tile format image;
  // not zero for only a single-part "normal" tiled file (according to spec.)
  int tiled;
  int long_name;  // long name attribute
  // deep image(EXR 2.0);
  // for a multi-part file, indicates that at least one part is of type deep* (according to spec.)
  int non_image;
  int multipart;  // multi-part(EXR 2.0)
} EXRVersion;

typedef struct TEXRAttribute {
  char name[256];  // name and type are up to 255 chars long.
  char type[256];
  unsigned char *value;  // uint8_t*
  int size;
  int pad0;
} EXRAttribute;

typedef struct TEXRChannelInfo {
  char name[256];  // less than 255 bytes long
  int pixel_type;
  int x_sampling;
  int y_sampling;
  unsigned char p_linear;
  unsigned char pad[3];
} EXRChannelInfo;

typedef struct TEXRTile {
  int offset_x;
  int offset_y;
  int level_x;
  int level_y;

  int width;   // actual width in a tile.
  int height;  // actual height int a tile.

  unsigned char **images;  // image[channels][pixels]
} EXRTile;

typedef struct TEXRBox2i {
  int min_x;
  int min_y;
  int max_x;
  int max_y;
} EXRBox2i;

typedef struct TEXRHeader {
  float pixel_aspect_ratio;
  int line_order;
  EXRBox2i data_window;
  EXRBox2i display_window;
  float screen_window_center[2];
  float screen_window_width;

  int chunk_count;

  // Properties for tiled format(`tiledesc`).
  int tiled;
  int tile_size_x;
  int tile_size_y;
  int tile_level_mode;
  int tile_rounding_mode;

  int long_name;
  // for a single-part file, agree with the version field bit 11
  // for a multi-part file, it is consistent with the type of part
  int non_image;
  int multipart;
  unsigned int header_len;

  // Custom attributes(exludes required attributes(e.g. `channels`,
  // `compression`, etc)
  int num_custom_attributes;
  EXRAttribute *custom_attributes;  // array of EXRAttribute. size =
                                    // `num_custom_attributes`.

  EXRChannelInfo *channels;  // [num_channels]

  int *pixel_types;  // Loaded pixel type(TINYEXR_PIXELTYPE_*) of `images` for
  // each channel. This is overwritten with `requested_pixel_types` when
  // loading.
  int num_channels;

  int compression_type;        // compression type(TINYEXR_COMPRESSIONTYPE_*)
  int *requested_pixel_types;  // Filled initially by
                               // ParseEXRHeaderFrom(Meomory|File), then users
                               // can edit it(only valid for HALF pixel type
                               // channel)
  // name attribute required for multipart files;
  // must be unique and non empty (according to spec.);
  // use EXRSetNameAttr for setting value;
  // max 255 character allowed - excluding terminating zero
  char name[256];
} EXRHeader;

typedef struct TEXRMultiPartHeader {
  int num_headers;
  EXRHeader *headers;

} EXRMultiPartHeader;

typedef struct TEXRImage {
  EXRTile *tiles;  // Tiled pixel data. The application must reconstruct image
                   // from tiles manually. NULL if scanline format.
  struct TEXRImage* next_level; // NULL if scanline format or image is the last level.
  int level_x; // x level index
  int level_y; // y level index

  unsigned char **images;  // image[channels][pixels]. NULL if tiled format.

  int width;
  int height;
  int num_channels;

  // Properties for tile format.
  int num_tiles;

} EXRImage;

typedef struct TEXRMultiPartImage {
  int num_images;
  EXRImage *images;

} EXRMultiPartImage;

typedef struct TDeepImage {
  const char **channel_names;
  float ***image;      // image[channels][scanlines][samples]
  int **offset_table;  // offset_table[scanline][offsets]
  int num_channels;
  int width;
  int height;
  int pad0;
} DeepImage;

// @deprecated { For backward compatibility. Not recommended to use. }
// Loads single-frame OpenEXR image. Assume EXR image contains A(single channel
// alpha) or RGB(A) channels.
// Application must free image data as returned by `out_rgba`
// Result image format is: float x RGBA x width x hight
// Returns negative value and may set error string in `err` when there's an
// error
extern int LoadEXR(float **out_rgba, int *width, int *height,
                   const char *filename, const char **err);

// Loads single-frame OpenEXR image by specifying layer name. Assume EXR image
// contains A(single channel alpha) or RGB(A) channels. Application must free
// image data as returned by `out_rgba` Result image format is: float x RGBA x
// width x hight Returns negative value and may set error string in `err` when
// there's an error When the specified layer name is not found in the EXR file,
// the function will return `TINYEXR_ERROR_LAYER_NOT_FOUND`.
extern int LoadEXRWithLayer(float **out_rgba, int *width, int *height,
                            const char *filename, const char *layer_name,
                            const char **err);

// Added by tom7. Pass layername == nullptr for LoadEXRFromMemory.
// Note that the routines above don't work for large files (or if
// ftell doesn't do what you'd want), and the above functions load
// the entire file into memory multiple times (?) so using this with
// Util::ReadFileBytes is probably your best bet, at least if you're me.
extern int LoadEXRWithLayerFromMemory(
    float **out_rgba, int *width, int *height,
    const unsigned char *buffer, size_t size,
    const char *layername,
    const char **err);


//
// Get layer infos from EXR file.
//
// @param[out] layer_names List of layer names. Application must free memory
// after using this.
// @param[out] num_layers The number of layers
// @param[out] err Error string(will be filled when the function returns error
// code). Free it using FreeEXRErrorMessage after using this value.
//
// @return TINYEXR_SUCCEES upon success.
//
extern int EXRLayers(const char *filename, const char **layer_names[],
                     int *num_layers, const char **err);

// @deprecated
// Simple wrapper API for ParseEXRHeaderFromFile.
// checking given file is a EXR file(by just look up header)
// @return TINYEXR_SUCCEES for EXR image, TINYEXR_ERROR_INVALID_HEADER for
// others
extern int IsEXR(const char *filename);

// Simple wrapper API for ParseEXRHeaderFromMemory.
// Check if given data is a EXR image(by just looking up a header section)
// @return TINYEXR_SUCCEES for EXR image, TINYEXR_ERROR_INVALID_HEADER for
// others
extern int IsEXRFromMemory(const unsigned char *memory, size_t size);

// @deprecated
// Saves single-frame OpenEXR image to a buffer. Assume EXR image contains RGB(A) channels.
// components must be 1(Grayscale), 3(RGB) or 4(RGBA).
// Input image format is: `float x width x height`, or `float x RGB(A) x width x
// hight`
// Save image as fp16(HALF) format when `save_as_fp16` is positive non-zero
// value.
// Save image as fp32(FLOAT) format when `save_as_fp16` is 0.
// Use ZIP compression by default.
// `buffer` is the pointer to write EXR data.
// Memory for `buffer` is allocated internally in SaveEXRToMemory.
// Returns the data size of EXR file when the value is positive(up to 2GB EXR data).
// Returns negative value and may set error string in `err` when there's an
// error
extern int SaveEXRToMemory(const float *data, const int width, const int height,
                   const int components, const int save_as_fp16,
                   const unsigned char **buffer, const char **err);

// @deprecated { Not recommended, but handy to use. }
// Saves single-frame OpenEXR image to a buffer. Assume EXR image contains RGB(A) channels.
// components must be 1(Grayscale), 3(RGB) or 4(RGBA).
// Input image format is: `float x width x height`, or `float x RGB(A) x width x
// hight`
// Save image as fp16(HALF) format when `save_as_fp16` is positive non-zero
// value.
// Save image as fp32(FLOAT) format when `save_as_fp16` is 0.
// Use ZIP compression by default.
// Returns TINYEXR_SUCCEES(0) when success.
// Returns negative value and may set error string in `err` when there's an
// error
extern int SaveEXR(const float *data, const int width, const int height,
                   const int components, const int save_as_fp16,
                   const char *filename, const char **err);

// Returns the number of resolution levels of the image (including the base)
extern int EXRNumLevels(const EXRImage* exr_image);

// Initialize EXRHeader struct
extern void InitEXRHeader(EXRHeader *exr_header);

// Set name attribute of EXRHeader struct (it makes a copy)
extern void EXRSetNameAttr(EXRHeader *exr_header, const char* name);

// Initialize EXRImage struct
extern void InitEXRImage(EXRImage *exr_image);

// Frees internal data of EXRHeader struct
extern int FreeEXRHeader(EXRHeader *exr_header);

// Frees internal data of EXRImage struct
extern int FreeEXRImage(EXRImage *exr_image);

// Frees error message
extern void FreeEXRErrorMessage(const char *msg);

// Parse EXR version header of a file.
extern int ParseEXRVersionFromFile(EXRVersion *version, const char *filename);

// Parse EXR version header from memory-mapped EXR data.
extern int ParseEXRVersionFromMemory(EXRVersion *version,
                                     const unsigned char *memory, size_t size);

// Parse single-part OpenEXR header from a file and initialize `EXRHeader`.
// When there was an error message, Application must free `err` with
// FreeEXRErrorMessage()
extern int ParseEXRHeaderFromFile(EXRHeader *header, const EXRVersion *version,
                                  const char *filename, const char **err);

// Parse single-part OpenEXR header from a memory and initialize `EXRHeader`.
// When there was an error message, Application must free `err` with
// FreeEXRErrorMessage()
extern int ParseEXRHeaderFromMemory(EXRHeader *header,
                                    const EXRVersion *version,
                                    const unsigned char *memory, size_t size,
                                    const char **err);

// Parse multi-part OpenEXR headers from a file and initialize `EXRHeader*`
// array.
// When there was an error message, Application must free `err` with
// FreeEXRErrorMessage()
extern int ParseEXRMultipartHeaderFromFile(EXRHeader ***headers,
                                           int *num_headers,
                                           const EXRVersion *version,
                                           const char *filename,
                                           const char **err);

// Parse multi-part OpenEXR headers from a memory and initialize `EXRHeader*`
// array
// When there was an error message, Application must free `err` with
// FreeEXRErrorMessage()
extern int ParseEXRMultipartHeaderFromMemory(EXRHeader ***headers,
                                             int *num_headers,
                                             const EXRVersion *version,
                                             const unsigned char *memory,
                                             size_t size, const char **err);

// Loads single-part OpenEXR image from a file.
// Application must setup `ParseEXRHeaderFromFile` before calling this function.
// Application can free EXRImage using `FreeEXRImage`
// Returns negative value and may set error string in `err` when there's an
// error
// When there was an error message, Application must free `err` with
// FreeEXRErrorMessage()
extern int LoadEXRImageFromFile(EXRImage *image, const EXRHeader *header,
                                const char *filename, const char **err);

// Loads single-part OpenEXR image from a memory.
// Application must setup `EXRHeader` with
// `ParseEXRHeaderFromMemory` before calling this function.
// Application can free EXRImage using `FreeEXRImage`
// Returns negative value and may set error string in `err` when there's an
// error
// When there was an error message, Application must free `err` with
// FreeEXRErrorMessage()
extern int LoadEXRImageFromMemory(EXRImage *image, const EXRHeader *header,
                                  const unsigned char *memory,
                                  const size_t size, const char **err);

// Loads multi-part OpenEXR image from a file.
// Application must setup `ParseEXRMultipartHeaderFromFile` before calling this
// function.
// Application can free EXRImage using `FreeEXRImage`
// Returns negative value and may set error string in `err` when there's an
// error
// When there was an error message, Application must free `err` with
// FreeEXRErrorMessage()
extern int LoadEXRMultipartImageFromFile(EXRImage *images,
                                         const EXRHeader **headers,
                                         unsigned int num_parts,
                                         const char *filename,
                                         const char **err);

// Loads multi-part OpenEXR image from a memory.
// Application must setup `EXRHeader*` array with
// `ParseEXRMultipartHeaderFromMemory` before calling this function.
// Application can free EXRImage using `FreeEXRImage`
// Returns negative value and may set error string in `err` when there's an
// error
// When there was an error message, Application must free `err` with
// FreeEXRErrorMessage()
extern int LoadEXRMultipartImageFromMemory(EXRImage *images,
                                           const EXRHeader **headers,
                                           unsigned int num_parts,
                                           const unsigned char *memory,
                                           const size_t size, const char **err);

// Saves multi-channel, single-frame OpenEXR image to a file.
// Returns negative value and may set error string in `err` when there's an
// error
// When there was an error message, Application must free `err` with
// FreeEXRErrorMessage()
extern int SaveEXRImageToFile(const EXRImage *image,
                              const EXRHeader *exr_header, const char *filename,
                              const char **err);

// Saves multi-channel, single-frame OpenEXR image to a memory.
// Image is compressed using EXRImage.compression value.
// Return the number of bytes if success.
// Return zero and will set error string in `err` when there's an
// error.
// When there was an error message, Application must free `err` with
// FreeEXRErrorMessage()
extern size_t SaveEXRImageToMemory(const EXRImage *image,
                                   const EXRHeader *exr_header,
                                   unsigned char **memory, const char **err);

// Saves multi-channel, multi-frame OpenEXR image to a memory.
// Image is compressed using EXRImage.compression value.
// File global attributes (eg. display_window) must be set in the first header.
// Returns negative value and may set error string in `err` when there's an
// error
// When there was an error message, Application must free `err` with
// FreeEXRErrorMessage()
extern int SaveEXRMultipartImageToFile(const EXRImage *images,
                                       const EXRHeader **exr_headers,
                                       unsigned int num_parts,
                                       const char *filename, const char **err);

// Saves multi-channel, multi-frame OpenEXR image to a memory.
// Image is compressed using EXRImage.compression value.
// File global attributes (eg. display_window) must be set in the first header.
// Return the number of bytes if success.
// Return zero and will set error string in `err` when there's an
// error.
// When there was an error message, Application must free `err` with
// FreeEXRErrorMessage()
extern size_t SaveEXRMultipartImageToMemory(const EXRImage *images,
                                            const EXRHeader **exr_headers,
                                            unsigned int num_parts,
                                            unsigned char **memory, const char **err);
// Loads single-frame OpenEXR deep image.
// Application must free memory of variables in DeepImage(image, offset_table)
// Returns negative value and may set error string in `err` when there's an
// error
// When there was an error message, Application must free `err` with
// FreeEXRErrorMessage()
extern int LoadDeepEXR(DeepImage *out_image, const char *filename,
                       const char **err);

// NOT YET IMPLEMENTED:
// Saves single-frame OpenEXR deep image.
// Returns negative value and may set error string in `err` when there's an
// error
// extern int SaveDeepEXR(const DeepImage *in_image, const char *filename,
//                       const char **err);

// NOT YET IMPLEMENTED:
// Loads multi-part OpenEXR deep image.
// Application must free memory of variables in DeepImage(image, offset_table)
// extern int LoadMultiPartDeepEXR(DeepImage **out_image, int num_parts, const
// char *filename,
//                       const char **err);

// For emscripten.
// Loads single-frame OpenEXR image from memory. Assume EXR image contains
// RGB(A) channels.
// Returns negative value and may set error string in `err` when there's an
// error
// When there was an error message, Application must free `err` with
// FreeEXRErrorMessage()
extern int LoadEXRFromMemory(float **out_rgba, int *width, int *height,
                             const unsigned char *memory, size_t size,
                             const char **err);

#endif  // _CC_LIB_TINYEXR_H
