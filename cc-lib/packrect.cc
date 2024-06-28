
#include "packrect.h"

#include <tuple>
#include <vector>
#include <utility>
#include <cmath>
#include <cstring>

#include "base/logging.h"
#include "opt/optimizer.h"

namespace {
// --------------------------------------------------
// start stb_rect_pact.h
//
// I did some cleanup to embed this, like assuming c++
// and coordinate type of 'int', etc.
//
// --------------------------------------------------
#define STBRP_LARGE_RECTS 1
#define STB_RECT_PACK_IMPLEMENTATION 1

// stb_rect_pack.h - v1.00 - public domain - rectangle packing
// Sean Barrett 2014
//
// Useful for e.g. packing rectangular textures into an atlas.
// Does not do rotation.
//
// Not necessarily the awesomest packing method, but better than
// the totally naive one in stb_truetype (which is primarily what
// this is meant to replace).
//
// Has only had a few tests run, may have issues.
//
// More docs to come.
//
// No memory allocations; uses qsort() and assert() from stdlib.
// Can override those by defining STBRP_SORT and STBRP_ASSERT.
//
// This library currently uses the Skyline Bottom-Left algorithm.
//
// Please note: better rectangle packers are welcome! Please
// implement them to the same API, but with a different init
// function.
//
// Credits
//
//  Library
//    Sean Barrett
//  Minor features
//    Martins Mozeiko
//    github:IntellectualKitty
//
//  Bugfixes / warning fixes
//    Jeremy Jaussaud
//    Fabian Giesen
//
// Version history:
//
//     1.00  (2019-02-25)  avoid small space waste; gracefully fail too-wide rectangles
//     0.99  (2019-02-07)  warning fixes
//     0.11  (2017-03-03)  return packing success/fail result
//     0.10  (2016-10-25)  remove cast-away-const to avoid warnings
//     0.09  (2016-08-27)  fix compiler warnings
//     0.08  (2015-09-13)  really fix bug with empty rects (w=0 or h=0)
//     0.07  (2015-09-13)  fix bug with empty rects (w=0 or h=0)
//     0.06  (2015-04-15)  added STBRP_SORT to allow replacing qsort
//     0.05:  added STBRP_ASSERT to allow replacing assert
//     0.04:  fixed minor bug in STBRP_LARGE_RECTS support
//     0.01:  initial release
//
// LICENSE
//
//   See end of file for license information.

//////////////////////////////////////////////////////////////////////////////
//
//       INCLUDE SECTION
//

#define STB_RECT_PACK_VERSION  1
#define STBRP_DEF static

typedef struct stbrp_context stbrp_context;
typedef struct stbrp_node    stbrp_node;
typedef struct stbrp_rect    stbrp_rect;

#ifdef STBRP_LARGE_RECTS
typedef int            stbrp_coord;
#else
typedef unsigned short stbrp_coord;
#endif

STBRP_DEF int stbrp_pack_rects (stbrp_context *context, stbrp_rect *rects, int num_rects);
// Assign packed locations to rectangles. The rectangles are of type
// 'stbrp_rect' defined below, stored in the array 'rects', and there
// are 'num_rects' many of them.
//
// Rectangles which are successfully packed have the 'was_packed' flag
// set to a non-zero value and 'x' and 'y' store the minimum location
// on each axis (i.e. bottom-left in cartesian coordinates, top-left
// if you imagine y increasing downwards). Rectangles which do not fit
// have the 'was_packed' flag set to 0.
//
// You should not try to access the 'rects' array from another thread
// while this function is running, as the function temporarily reorders
// the array while it executes.
//
// To pack into another rectangle, you need to call stbrp_init_target
// again. To continue packing into the same rectangle, you can call
// this function again. Calling this multiple times with multiple rect
// arrays will probably produce worse packing results than calling it
// a single time with the full rectangle array, but the option is
// available.
//
// The function returns 1 if all of the rectangles were successfully
// packed and 0 otherwise.

struct stbrp_rect
{
  // reserved for your use:
  // tom7 it removed because we don't use it
  // int            id;

   // input:
   stbrp_coord    w, h;

   // output:
   stbrp_coord    x, y;
   int            was_packed;  // non-zero if valid packing

}; // 16 bytes, nominally


STBRP_DEF void stbrp_init_target (stbrp_context *context, int width, int height, stbrp_node *nodes, int num_nodes);
// Initialize a rectangle packer to:
//    pack a rectangle that is 'width' by 'height' in dimensions
//    using temporary storage provided by the array 'nodes', which is 'num_nodes' long
//
// You must call this function every time you start packing into a new target.
//
// There is no "shutdown" function. The 'nodes' memory must stay valid for
// the following stbrp_pack_rects() call (or calls), but can be freed after
// the call (or calls) finish.
//
// Note: to guarantee best results, either:
//       1. make sure 'num_nodes' >= 'width'
//   or  2. call stbrp_allow_out_of_mem() defined below with 'allow_out_of_mem = 1'
//
// If you don't do either of the above things, widths will be quantized to multiples
// of small integers to guarantee the algorithm doesn't run out of temporary storage.
//
// If you do #2, then the non-quantized algorithm will be used, but the algorithm
// may run out of temporary storage and be unable to pack some rectangles.

STBRP_DEF void stbrp_setup_allow_out_of_mem (stbrp_context *context, int allow_out_of_mem);
// Optionally call this function after init but before doing any packing to
// change the handling of the out-of-temp-memory scenario, described above.
// If you call init again, this will be reset to the default (false).


STBRP_DEF void stbrp_setup_heuristic (stbrp_context *context, int heuristic);
// Optionally select which packing heuristic the library should use. Different
// heuristics will produce better/worse results for different data sets.
// If you call init again, this will be reset to the default.

enum
{
   STBRP_HEURISTIC_Skyline_default=0,
   STBRP_HEURISTIC_Skyline_BL_sortHeight = STBRP_HEURISTIC_Skyline_default,
   STBRP_HEURISTIC_Skyline_BF_sortHeight
};


//////////////////////////////////////////////////////////////////////////////
//
// the details of the following structures don't matter to you, but they must
// be visible so you can handle the memory allocations for them

struct stbrp_node
{
   stbrp_coord  x,y;
   stbrp_node  *next;
};

struct stbrp_context
{
   int width;
   int height;
   int align;
   int init_mode;
   int heuristic;
   int num_nodes;
   stbrp_node *active_head;
   stbrp_node *free_head;
   stbrp_node extra[2]; // we allocate two extra nodes so optimal user-node-count is 'width' not 'width+2'
};

//////////////////////////////////////////////////////////////////////////////
//
//     IMPLEMENTATION SECTION
//

#ifdef STB_RECT_PACK_IMPLEMENTATION
#ifndef STBRP_SORT
#include <stdlib.h>
#define STBRP_SORT qsort
#endif

#ifndef STBRP_ASSERT
#include <assert.h>
#define STBRP_ASSERT assert
#endif

#ifdef _MSC_VER
#define STBRP__NOTUSED(v)  (void)(v)
#else
#define STBRP__NOTUSED(v)  (void)sizeof(v)
#endif

enum
{
   STBRP__INIT_skyline = 1
};

STBRP_DEF void stbrp_setup_heuristic(stbrp_context *context, int heuristic)
{
   switch (context->init_mode) {
      case STBRP__INIT_skyline:
         STBRP_ASSERT(heuristic == STBRP_HEURISTIC_Skyline_BL_sortHeight || heuristic == STBRP_HEURISTIC_Skyline_BF_sortHeight);
         context->heuristic = heuristic;
         break;
      default:
         STBRP_ASSERT(0);
   }
}

STBRP_DEF void stbrp_setup_allow_out_of_mem(stbrp_context *context, int allow_out_of_mem)
{
   if (allow_out_of_mem)
      // if it's ok to run out of memory, then don't bother aligning them;
      // this gives better packing, but may fail due to OOM (even though
      // the rectangles easily fit). @TODO a smarter approach would be to only
      // quantize once we've hit OOM, then we could get rid of this parameter.
      context->align = 1;
   else {
      // if it's not ok to run out of memory, then quantize the widths
      // so that num_nodes is always enough nodes.
      //
      // I.e. num_nodes * align >= width
      //                  align >= width / num_nodes
      //                  align = ceil(width/num_nodes)

      context->align = (context->width + context->num_nodes-1) / context->num_nodes;
   }
}

STBRP_DEF void stbrp_init_target(stbrp_context *context, int width, int height, stbrp_node *nodes, int num_nodes)
{
   int i;
#ifndef STBRP_LARGE_RECTS
   STBRP_ASSERT(width <= 0xffff && height <= 0xffff);
#endif

   for (i=0; i < num_nodes-1; ++i)
      nodes[i].next = &nodes[i+1];
   nodes[i].next = NULL;
   context->init_mode = STBRP__INIT_skyline;
   context->heuristic = STBRP_HEURISTIC_Skyline_default;
   context->free_head = &nodes[0];
   context->active_head = &context->extra[0];
   context->width = width;
   context->height = height;
   context->num_nodes = num_nodes;
   stbrp_setup_allow_out_of_mem(context, 0);

   // node 0 is the full width, node 1 is the sentinel (lets us not store width explicitly)
   context->extra[0].x = 0;
   context->extra[0].y = 0;
   context->extra[0].next = &context->extra[1];
   context->extra[1].x = (stbrp_coord) width;
#ifdef STBRP_LARGE_RECTS
   context->extra[1].y = (1<<30);
#else
   context->extra[1].y = 65535;
#endif
   context->extra[1].next = NULL;
}

// find minimum y position if it starts at x1
static int stbrp__skyline_find_min_y(stbrp_context *c, stbrp_node *first, int x0, int width, int *pwaste)
{
   stbrp_node *node = first;
   int x1 = x0 + width;
   int min_y, visited_width, waste_area;

   STBRP__NOTUSED(c);

   STBRP_ASSERT(first->x <= x0);

   #if 0
   // skip in case we're past the node
   while (node->next->x <= x0)
      ++node;
   #else
   STBRP_ASSERT(node->next->x > x0); // we ended up handling this in the caller for efficiency
   #endif

   STBRP_ASSERT(node->x <= x0);

   min_y = 0;
   waste_area = 0;
   visited_width = 0;
   while (node->x < x1) {
      if (node->y > min_y) {
         // raise min_y higher.
         // we've accounted for all waste up to min_y,
         // but we'll now add more waste for everything we've visted
         waste_area += visited_width * (node->y - min_y);
         min_y = node->y;
         // the first time through, visited_width might be reduced
         if (node->x < x0)
            visited_width += node->next->x - x0;
         else
            visited_width += node->next->x - node->x;
      } else {
         // add waste area
         int under_width = node->next->x - node->x;
         if (under_width + visited_width > width)
            under_width = width - visited_width;
         waste_area += under_width * (min_y - node->y);
         visited_width += under_width;
      }
      node = node->next;
   }

   *pwaste = waste_area;
   return min_y;
}

typedef struct
{
   int x,y;
   stbrp_node **prev_link;
} stbrp__findresult;

static stbrp__findresult stbrp__skyline_find_best_pos(stbrp_context *c, int width, int height)
{
   int best_waste = (1<<30), best_x, best_y = (1 << 30);
   stbrp__findresult fr;
   stbrp_node **prev, *node, *tail, **best = NULL;

   // align to multiple of c->align
   width = (width + c->align - 1);
   width -= width % c->align;
   STBRP_ASSERT(width % c->align == 0);

   // if it can't possibly fit, bail immediately
   if (width > c->width || height > c->height) {
      fr.prev_link = NULL;
      fr.x = fr.y = 0;
      return fr;
   }

   node = c->active_head;
   prev = &c->active_head;
   while (node->x + width <= c->width) {
      int y,waste;
      y = stbrp__skyline_find_min_y(c, node, node->x, width, &waste);
      if (c->heuristic == STBRP_HEURISTIC_Skyline_BL_sortHeight) { // actually just want to test BL
         // bottom left
         if (y < best_y) {
            best_y = y;
            best = prev;
         }
      } else {
         // best-fit
         if (y + height <= c->height) {
            // can only use it if it fits vertically
            if (y < best_y || (y == best_y && waste < best_waste)) {
               best_y = y;
               best_waste = waste;
               best = prev;
            }
         }
      }
      prev = &node->next;
      node = node->next;
   }

   best_x = (best == NULL) ? 0 : (*best)->x;

   // if doing best-fit (BF), we also have to try aligning right edge to each node position
   //
   // e.g, if fitting
   //
   //     ____________________
   //    |____________________|
   //
   //            into
   //
   //   |                         |
   //   |             ____________|
   //   |____________|
   //
   // then right-aligned reduces waste, but bottom-left BL is always chooses left-aligned
   //
   // This makes BF take about 2x the time

   if (c->heuristic == STBRP_HEURISTIC_Skyline_BF_sortHeight) {
      tail = c->active_head;
      node = c->active_head;
      prev = &c->active_head;
      // find first node that's admissible
      while (tail->x < width)
         tail = tail->next;
      while (tail) {
         int xpos = tail->x - width;
         int y,waste;
         STBRP_ASSERT(xpos >= 0);
         // find the left position that matches this
         while (node->next->x <= xpos) {
            prev = &node->next;
            node = node->next;
         }
         STBRP_ASSERT(node->next->x > xpos && node->x <= xpos);
         y = stbrp__skyline_find_min_y(c, node, xpos, width, &waste);
         if (y + height <= c->height) {
            if (y <= best_y) {
               if (y < best_y || waste < best_waste || (waste==best_waste && xpos < best_x)) {
                  best_x = xpos;
                  STBRP_ASSERT(y <= best_y);
                  best_y = y;
                  best_waste = waste;
                  best = prev;
               }
            }
         }
         tail = tail->next;
      }
   }

   fr.prev_link = best;
   fr.x = best_x;
   fr.y = best_y;
   return fr;
}

static stbrp__findresult stbrp__skyline_pack_rectangle(stbrp_context *context, int width, int height)
{
   // find best position according to heuristic
   stbrp__findresult res = stbrp__skyline_find_best_pos(context, width, height);
   stbrp_node *node, *cur;

   // bail if:
   //    1. it failed
   //    2. the best node doesn't fit (we don't always check this)
   //    3. we're out of memory
   if (res.prev_link == NULL || res.y + height > context->height || context->free_head == NULL) {
      res.prev_link = NULL;
      return res;
   }

   // on success, create new node
   node = context->free_head;
   node->x = (stbrp_coord) res.x;
   node->y = (stbrp_coord) (res.y + height);

   context->free_head = node->next;

   // insert the new node into the right starting point, and
   // let 'cur' point to the remaining nodes needing to be
   // stiched back in

   cur = *res.prev_link;
   if (cur->x < res.x) {
      // preserve the existing one, so start testing with the next one
      stbrp_node *next = cur->next;
      cur->next = node;
      cur = next;
   } else {
      *res.prev_link = node;
   }

   // from here, traverse cur and free the nodes, until we get to one
   // that shouldn't be freed
   while (cur->next && cur->next->x <= res.x + width) {
      stbrp_node *next = cur->next;
      // move the current node to the free list
      cur->next = context->free_head;
      context->free_head = cur;
      cur = next;
   }

   // stitch the list back in
   node->next = cur;

   if (cur->x < res.x + width)
      cur->x = (stbrp_coord) (res.x + width);

#ifdef _DEBUG
   cur = context->active_head;
   while (cur->x < context->width) {
      STBRP_ASSERT(cur->x < cur->next->x);
      cur = cur->next;
   }
   STBRP_ASSERT(cur->next == NULL);

   {
      int count=0;
      cur = context->active_head;
      while (cur) {
         cur = cur->next;
         ++count;
      }
      cur = context->free_head;
      while (cur) {
         cur = cur->next;
         ++count;
      }
      STBRP_ASSERT(count == context->num_nodes+2);
   }
#endif

   return res;
}

static int rect_height_compare(const void *a, const void *b)
{
   const stbrp_rect *p = (const stbrp_rect *) a;
   const stbrp_rect *q = (const stbrp_rect *) b;
   if (p->h > q->h)
      return -1;
   if (p->h < q->h)
      return  1;
   return (p->w > q->w) ? -1 : (p->w < q->w);
}

static int rect_original_order(const void *a, const void *b)
{
   const stbrp_rect *p = (const stbrp_rect *) a;
   const stbrp_rect *q = (const stbrp_rect *) b;
   return (p->was_packed < q->was_packed) ? -1 : (p->was_packed > q->was_packed);
}

#ifdef STBRP_LARGE_RECTS
// tom7 added this cast (to signed) to suppress warnings.
// it's possible that the code actually wants this field to be
// unsigned, since it does some sorting and non-large rects
// have unsigned coordinates, though?
#define STBRP__MAXVAL  (stbrp_coord)0xffffffff
#else
#define STBRP__MAXVAL  0xffff
#endif

STBRP_DEF int stbrp_pack_rects(stbrp_context *context, stbrp_rect *rects, int num_rects)
{
   int i, all_rects_packed = 1;

   // we use the 'was_packed' field internally to allow sorting/unsorting
   for (i=0; i < num_rects; ++i) {
      rects[i].was_packed = i;
   }

   // sort according to heuristic
   STBRP_SORT(rects, num_rects, sizeof(rects[0]), rect_height_compare);

   for (i=0; i < num_rects; ++i) {
      if (rects[i].w == 0 || rects[i].h == 0) {
         rects[i].x = rects[i].y = 0;  // empty rect needs no space
      } else {
         stbrp__findresult fr = stbrp__skyline_pack_rectangle(context, rects[i].w, rects[i].h);
         if (fr.prev_link) {
            rects[i].x = (stbrp_coord) fr.x;
            rects[i].y = (stbrp_coord) fr.y;
         } else {
            rects[i].x = rects[i].y = STBRP__MAXVAL;
         }
      }
   }

   // unsort
   STBRP_SORT(rects, num_rects, sizeof(rects[0]), rect_original_order);

   // set was_packed flags and all_rects_packed status
   for (i=0; i < num_rects; ++i) {
      rects[i].was_packed = !(rects[i].x == STBRP__MAXVAL && rects[i].y == STBRP__MAXVAL);
      if (!rects[i].was_packed)
         all_rects_packed = 0;
   }

   // return the all_rects_packed status
   return all_rects_packed;
}
#endif

// (For cc-lib: Note that this copyright and license only applies to the
// embedded file stb_rect_pack.h.)
/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2017 Sean Barrett
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/


// --------------------------------------------------
//   end stb_rect_pack.h
// --------------------------------------------------
}  // namespace

// Try packing into a rectangle of the given size.
// Returns the number of rectangles that weren't packed, so
// 0 means success.
static int TryPackSTB(
    // Rectangles to pack, given a pair width,height.
    const std::vector<std::pair<int, int>> &rects,
    int width, int height,
    bool bottom_left,
    // The output positions (x,y), parallel to the input rects.
    // Can be null.
    std::vector<std::pair<int, int>> *positions) {

  stbrp_rect *stbrects = (stbrp_rect*)malloc(rects.size() *
                                             sizeof (stbrp_rect));
  for (int i = 0; i < (int)rects.size(); i++) {
    stbrp_rect *r = &stbrects[i];
    r->w = rects[i].first;
    r->h = rects[i].second;
    // These do not actually need to be initialized...
    r->x = 0;
    r->y = 0;
    r->was_packed = 0;
  }

  stbrp_context context;

  const int num_nodes = width;
  stbrp_node *nodes = (stbrp_node *)malloc(num_nodes * sizeof (stbrp_node));
  stbrp_init_target(&context, width, height, nodes, num_nodes);

  // Could also change/disable sorting here easily.
  stbrp_setup_heuristic(&context,
                        bottom_left ?
                        STBRP_HEURISTIC_Skyline_BL_sortHeight :
                        STBRP_HEURISTIC_Skyline_BF_sortHeight);

  int num_unpacked = 0;
  if (stbrp_pack_rects(&context, stbrects, rects.size())) {
    positions->clear();
    positions->reserve(rects.size());
    // Order will be preserved.
    for (int i = 0; i < (int)rects.size(); i++) {
      positions->emplace_back(stbrects[i].x, stbrects[i].y);
    }

    num_unpacked = 0;
  } else {
    for (int i = 0; i < (int)rects.size(); i++) {
      if (!stbrects[i].was_packed) num_unpacked++;
    }
  }

  free(stbrects);
  free(nodes);

  return num_unpacked;
}

namespace {
// Old Escape method. Dense pixel map, first fit.
struct UsedMap {
  // PERF could use less memory with a bit mask.
  char *arr = nullptr;
  int w = 0, h = 0;

  UsedMap(int ww, int hh) : w(ww), h(hh) {
    arr = (char*)malloc(ww * hh * sizeof(char));
    std::memset(arr, 0, ww * hh * sizeof (char));
  }

  /* new areas are unused */
  void Resize(int ww, int hh) {
    char *na = (char*)malloc(ww * hh * sizeof (char));

    /* start unused */
    std::memset(na, 0, ww * hh * sizeof (char));

    /* copy old used */
    for (int xx = 0; xx < w; xx++) {
      for (int yy = 0; yy < h; yy++) {
        if (Used(xx, yy) && xx < ww && yy < hh) {
          na[yy * ww + xx] = 1;
        }
      }
    }

    free(arr);
    arr = na;
    w = ww;
    h = hh;
  }

  bool Used(int x, int y) {
    return arr[x + y * w];
  }

  bool UsedRange(int x, int y, int ww, int hh) {
    for (int yy = 0; yy < hh; yy++) {
      for (int xx = 0; xx < ww; xx++) {
        if (Used(x + xx, y + yy)) return true;
      }
    }
    return false;
  }

  void Use(int x, int y) {
    arr[x + y * w] = 1;
  }

  void UseRange(int x, int y, int ww, int hh) {
    for (int yy = 0; yy < hh; yy++) {
      for (int xx = 0; xx < ww; xx++) {
        Use(x + xx, y + yy);
      }
    }
  }

  ~UsedMap() {
    free(arr);
  }
};
}  // namespace

static std::pair<int, int> FitImage(UsedMap *um, int w, int h) {
  // Escape used a linear growth rate, but this seems excessively
  // slow (especially since we crop after the fact).
  static constexpr float GROWRATE = 0.1f;
  for (;;) {
    for (int yy = 0; yy <= um->h - h; yy++) {
      for (int xx = 0; xx <= um->w - w; xx++) {
        if (!um->UsedRange(xx, yy, w, h)) {
          um->UseRange(xx, yy, w, h);
          return std::make_pair(xx, yy);
        }
      }
    }

    // Didn't fit. Expand to make the image more square,
    // but at least accommodate the current target image size.
    int nw = um->w, nh = um->h;
    if (um->w < w) {
      nw = w;
    } else if (um->h < h) {
      nh = h;
    } else if (um->w <= um->h) {
      int gw = (int)(um->w * GROWRATE);
      nw = um->w + std::max(1, gw);
    } else {
      int gh = (int)(um->h * GROWRATE);
      nh = um->h + std::max(1, gh);
    }

    um->Resize(nw, nh);
    // printf("Resize to %d x %d -> %d x %d\n", um->w, um->h, nw, nh);
  }
}

// Always succeeds, but is slow, and usually worse than STB.
static void TryPackEsc(
    // e.g. size of largest bitmap
    int initial_width, int initial_height,
    // Rectangles to pack, given a pair width,height.
    const std::vector<std::pair<int, int>> &rects,
    // The output positions (x,y). Can be null.
    std::vector<std::pair<int, int>> *positions) {
  // Can be null for uniformity, but then the function does nothing.
  if (positions == nullptr) return;

  UsedMap um{initial_width, initial_height};

  positions->clear();
  for (const auto &[rect_w, rect_h] : rects) {
    // find a place where it will fit.
    const auto [x, y] = FitImage(&um, rect_w, rect_h);
    positions->emplace_back(x, y);
  }
}


// Assuming the rectangles have been positioned legally,
// return minimum width/height of the containing rectangle.
static std::pair<int, int> Crop(
    const std::vector<std::pair<int, int>> &rects,
    const std::vector<std::pair<int, int>> &positions) {
  int max_w = 0, max_h = 0;
  CHECK(rects.size() == positions.size());
  for (int i = 0; i < (int)rects.size(); i++) {
    const auto [x, y] = rects[i];
    const auto [w, h] = positions[i];
    max_w = std::max(max_w, x + w);
    max_h = std::max(max_h, y + h);
  }
  return std::make_pair(max_w, max_h);
}

bool PackRect::Pack(
    PackRect::Config config,
    // Rectangles to pack, given a pair width,height.
    const std::vector<std::pair<int, int>> &rects,
    // Size of the output rectangle.
    int *out_width, int *out_height,
    // The output positions (x,y), parallel to the input rects.
    std::vector<std::pair<int, int>> *out_positions) {

  // The largest dimensions in the input. We can also do some
  // quick validation.
  int max_input_width = 0;
  int max_input_height = 0;
  int total_width = 0, total_height = 0;
  int total_area = 0;
  for (const auto &[w, h] : rects) {
    if (w <= 0) return false;
    if (h <= 0) return false;
    max_input_width = std::max(max_input_width, w);
    max_input_height = std::max(max_input_height, h);
    total_area += w * h;
    total_width += w;
    total_height += h;
  }
  // We can't succeed unless the biggest rectangle fits within
  // constraints.
  if (config.max_width != 0 && max_input_width > config.max_width)
    return false;
  if (config.max_height != 0 && max_input_height > config.max_height)
    return false;


  // Starting rectangle size. We try to be square, but we know we
  // need at least the dimensions given above.
  int arg_width =
    std::max(max_input_width, (int)ceilf(sqrtf(total_area)));
  int arg_height =
    std::max(max_input_height, (int)ceilf(total_area / (float)arg_width));

  // In this first phase, we're trying to find *any* packing; we'll
  // only fail if the width/height have constraints. We won't usually
  // achieve optimal packing, and the optimal size is typically larger
  // than the square root of the area anyway. So give ourselves
  // significant slack so that the first pass is likely to succeed.

  // (XXX This first phase could perhaps be replaced by just
  // calling Sample below with some arg that we "know" will work, like
  // using width=total_width.)

  std::vector<std::pair<int, int>> pos;
  for (;;) {
    arg_width = ceilf(arg_width * 1.25f);
    arg_height = ceilf(arg_height * 1.25f);

    if (config.max_width != 0)
      arg_width = std::max(config.max_width, arg_width);
    if (config.max_height != 0)
      arg_height = std::max(config.max_height, arg_height);

    pos.clear();
    if (0 == TryPackSTB(rects, arg_width, arg_height, true, &pos)) {
      // We have a candidate solution; now start optimizing.
      break;
    } else {
      // did we just try at the maximum size?
      if (config.max_width != 0 &&
          config.max_height != 0 &&
          arg_width >= config.max_width &&
          arg_height >= config.max_height) {
        return false;
      }
    }
  }

  // We know they fit, but get the portion of the rectangle that's
  // actually used.
  [[maybe_unused]]
  auto [width, height] = Crop(rects, pos);

  // We have a solution in width/height. Now optimize.

  enum Method : int32_t {
    STB_BL = 0,
    STB_BF,
    ESCAPE,

    NUM_METHODS,
  };

  // TODO: Improve this search space. Maybe can reframe these
  // algorithms to only take the width as input? Also experiment
  // with different orderings, which is the thing that most
  // affects the output (although it is hard to map to a double?).
  //
  // Arguments are:
  //    - width,height
  //    - method

  // width, height, positions
  using OutputType = std::tuple<int, int, std::vector<std::pair<int, int>>>;
  using RectOptimizer = Optimizer<3, 0, OutputType>;

  // const double large_area = width * height + 1;
  auto Optimize = [&rects](
      const RectOptimizer::arg_type arg) -> RectOptimizer::return_type {
      auto [w, h, method] = arg.first;
      // printf("Optimize(%d,%d,%d)\n", w, h, method);

      #if 0
      // This "optimization" is disabled because...
      //   - we crop at the end anyway, so it is not conservative
      //   - it really should look at the current width and height,
      //     not the initial feasible solution
      //   - we need to actually force this when we call Sample
      //     below, otherwise there won't be a solution.
      // (If we called the current GetBest, then we could cover the
      //  second two, at least, but this would mean having a reference
      //  to the optimizer before creating the Optimize function.)
      //
      // But stop early if the result will be worse (at least
      // pre-cropping) than the current best.
      if (w * h > width * height) {
        // TODO: Would be better if we could return a penalty
        // in this case, here probably dw + dh...
        // printf("%d * %d > %d * %d", w, h, width, height);
        return std::make_pair(RectOptimizer::LARGE_SCORE, std::nullopt);
      }
      #endif

      std::vector<std::pair<int, int>> tmp_pos;
      switch (method) {
      case STB_BL:
      case STB_BF: {
        // printf("STB %d x %d (%d)...\n", w, h, method);
        int unpacked = TryPackSTB(rects, w, h, method == STB_BL, &tmp_pos);
        if (unpacked > 0) {
          // TODO: As above, good to be able to indicate the gradient...
          // printf("Unpacked %d\n", unpacked);
          return std::make_pair(RectOptimizer::LARGE_SCORE, std::nullopt);
          // Worse than our upper bound.
          // return large_area + 10 * unpacked;
        } else {
          CHECK_EQ(rects.size(), tmp_pos.size()) <<
            "rects: " << rects.size() << " tmp pos: " << tmp_pos.size();
          // Otherwise, minimize the area.
          const auto [cw, ch] = Crop(rects, tmp_pos);
          // printf("ok stb %d x %d!\n", cw, ch);
          return std::make_pair(
              (double)cw * ch,
              std::make_optional(std::make_tuple(cw, ch, std::move(tmp_pos))));
        }
        break;
      }

      case ESCAPE: {
        // Always succeeds.
        // printf("Esc %d x %d...\n", w, h);
        TryPackEsc(w, h, rects, &tmp_pos);
        const auto [cw, ch] = Crop(rects, tmp_pos);
        // printf("ok %d x %d\n", cw, ch);
        return std::make_pair(
            (double)cw * ch,
            std::make_optional(std::make_tuple(cw, ch, std::move(tmp_pos))));
      }

      default:
        // printf("bad method\n");
        LOG(FATAL) << "illegal method in Optimize";
      }
    };

  RectOptimizer optimizer(Optimize);
  // Start with our existing solution. PERF: Note that this currently
  // recomputes the packing we already had, for uniformity!
  optimizer.Sample(
      RectOptimizer::arg_type{{arg_width, arg_height, STB_BL}, {}});
  CHECK(optimizer.GetBest().has_value());

  const int width_ub =
    config.max_width == 0 ? total_width :
    std::min(total_width, config.max_width);
  const int height_ub =
    config.max_height == 0 ? total_height :
    std::min(total_height, config.max_height);

  const std::array<std::pair<int, int>, 3> int_bounds =
    // upper bounds are one past the max value to try
    {std::make_pair(max_input_width, width_ub + 1),
     std::make_pair(max_input_height, height_ub + 1),
     std::make_pair(0, NUM_METHODS)};

  const std::optional<int> passes =
    config.budget_passes > 0 ? std::make_optional(config.budget_passes) :
    std::nullopt;

  const std::optional<double> seconds =
    config.budget_seconds > 0 ?
    std::make_optional((double)config.budget_seconds) :
    std::nullopt;

  optimizer.Run(
      int_bounds, {},
      passes,
      std::nullopt,
      seconds,
      // stop if we achieve an optimal result, too
      {(double)total_area});

  auto best = optimizer.GetBest();
  CHECK(best.has_value()) << "Bug: We supposedly seeded this with a "
    "feasible solution!";

  OutputType best_out = std::get<2>(best.value());
  std::tie(*out_width, *out_height, *out_positions) = best_out;
  return true;
}


