// Copyright 2025-2026 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#if !defined(_BK_G722_H_)
#define _BK_G722_H_

#include <stdint.h>

enum { G722_SAMPLE_RATE_8000 = 0x0001, G722_PACKED = 0x0002 };

#ifndef INT16_MAX
#define INT16_MAX 32767
#endif
#ifndef INT16_MIN
#define INT16_MIN (-32768)
#endif

typedef struct {
  /*! TRUE if the operating in the special ITU test mode, with the band split filters
             disabled. */
  int itu_test_mode;
  /*! TRUE if the G.722 data is packed */
  int packed;
  /*! TRUE if encode from 8k samples/second */
  int eight_k;
  /*! 6 for 48000kbps, 7 for 56000kbps, or 8 for 64000kbps. */
  int bits_per_sample;

  /*! Signal history for the QMF */
  int x[24];

  struct {
    int s;
    int sp;
    int sz;
    int r[3];
    int a[3];
    int ap[3];
    int p[3];
    int d[7];
    int b[7];
    int bp[7];
    int sg[7];
    int nb;
    int det;
  } band[2];

  unsigned int in_buffer;
  int in_bits;
  unsigned int out_buffer;
  int out_bits;
} g722_encode_state_t;

typedef struct {
  /*! TRUE if the operating in the special ITU test mode, with the band split filters
             disabled. */
  int itu_test_mode;
  /*! TRUE if the G.722 data is packed */
  int packed;
  /*! TRUE if decode to 8k samples/second */
  int eight_k;
  /*! 6 for 48000kbps, 7 for 56000kbps, or 8 for 64000kbps. */
  int bits_per_sample;

  /*! Signal history for the QMF */
  int x[24];

  struct {
    int s;
    int sp;
    int sz;
    int r[3];
    int a[3];
    int ap[3];
    int p[3];
    int d[7];
    int b[7];
    int bp[7];
    int sg[7];
    int nb;
    int det;
  } band[2];

  unsigned int in_buffer;
  int in_bits;
  unsigned int out_buffer;
  int out_bits;
} g722_decode_state_t;

#ifdef __cplusplus
extern "C" {
#endif

int bk_g722_encode_init(g722_encode_state_t *s, int rate, int options);
int bk_g722_encode_release(g722_encode_state_t *s);
int bk_g722_encode(g722_encode_state_t *s, uint8_t g722_data[], const int16_t amp[], int len);

int bk_g722_decode_init(g722_decode_state_t *s, int rate, int options);
int bk_g722_decode_release(g722_decode_state_t *s);
int bk_g722_decode(g722_decode_state_t *s, int16_t amp[], const uint8_t g722_data[], int len);

#ifdef __cplusplus
}
#endif
#endif /* _BK_G722_H_ */