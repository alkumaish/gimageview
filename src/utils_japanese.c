/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GImageView
 * Copyright (C) 2001 Takuro Ashie
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils_char_code.h"
#include "utils_japanese.h"

/* force convert hankaku SJIS character to zenkaku */
#undef NO_HANKAKU_SJIS

#define NUL     0
#define LF      10
#define FF      12
#define CR      13
#define ESC     27
#define SS2     142

#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE    (!FALSE)
#endif

enum
{
   KC_ASCII,
   KC_EUC,
   KC_JIS,
   KC_SJIS,
   KC_EUCORSJIS,
   KC_UTF8
};


static const gchar *locale_euc[] = {
   "ja", "ja_JP", "ja_JP.ujis", "ja_JP.EUC", "ja_JP.eucJP", "ja_JP.eucjp",
};
static const gchar *locale_jis[] = {
   "ja_JP.JIS", "ja_JP.jis", "ja_JP.iso-2022-jp",
};
static const gchar *locale_sjis[] = {
   "ja_JP.SJIS", "ja_JP.sjis", "ja_JP.Shift_JIS",
};


const gchar *
japanese_locale_charset (const gchar *locale)
{
   gint i, n_locale_euc, n_locale_jis, n_locale_sjis;

   if (!locale || !*locale) return NULL;

   n_locale_euc = sizeof (locale_euc) / sizeof (gchar*);
   for (i = 0; i < n_locale_euc; i++)
      if (!g_ascii_strcasecmp (locale_euc[i], locale))
         return CHARSET_EUC_JP;

   n_locale_jis = sizeof (locale_jis) / sizeof (gchar*);
   for (i = 0; i < n_locale_jis; i++)
      if (!g_ascii_strcasecmp (locale_jis[i], locale))
         return CHARSET_JIS;

   n_locale_sjis = sizeof (locale_sjis) / sizeof (gchar*);
   for (i = 0; i < n_locale_sjis; i++)
      if (!g_ascii_strcasecmp (locale_sjis[i], locale))
         return CHARSET_SJIS;

   return NULL;
};



/******************************************************************************
 *
 *  these codes are mostly taken from kanji_conv.c
 *
 *   Copyright (C) 2000 Takuo Kitame <kitame@gnome.gr.jp>
 *
 *****************************************************************************/
static int 
detect_kanji (const guchar *str)
{
   int expected = KC_ASCII;
   register int c;
   int c1, c2;
   int euc_c = 0, sjis_c = 0;
   const guchar *ptr = str;

   g_return_val_if_fail (str && *str, 0);

   while ((c = *ptr)!= '\0') {
      if (c == ESC) {
         if ((c = *(++ptr)) == '\0')
            break;
         if (c == '$') {
            if ((c = *(++ptr)) == '\0')
               break;
            if (c == 'B' || c == '@')
               return KC_JIS;
         }
         ptr++;
         continue;
      }

      if ((c >= 0x81 && c <= 0x8d) || (c >= 0x8f && c <= 0x9f))
         return KC_SJIS;

      if (c == SS2) {
         if ((c = *(++ptr)) == '\0')
            break;
         if ((c >= 0x40 && c <= 0x7e)
             || (c >= 0x80 && c <= 0xa0)
             || (c >= 0xe0 && c <= 0xfc))
         {
            return KC_SJIS;
         }
         if (c >= 0xa1 && c <= 0xdf)
            break;

         ptr++;
         continue;
      }

      if (c >= 0xa1 && c <= 0xdf) {
         if ((c = *(++ptr)) == '\0')
            break;

         if (c >= 0xe0 && c <= 0xfe)
            return KC_EUC;
         if (c >= 0xa1 && c <= 0xdf) {
            expected = KC_EUCORSJIS;
            ptr++;
            continue;
         }
#if 1
         if (c == 0xa0 || (0xe0 <= c && c <= 0xfe)) {
            return KC_EUC;
         } else {
            expected = KC_EUCORSJIS;
            ptr++;
            continue;
         }
#else
         if (c <= 0x9f)
            return KC_SJIS;
         if (c >= 0xf0 && c <= 0xfe)
            return KC_EUC;
#endif

         if (c >= 0xe0 && c <= 0xef) {
            expected = KC_EUCORSJIS;
            while(c >= 0x40) {
               if(c >= 0x81) {
                  if(c <= 0x8d || (c >= 0x8f && c <= 0x9f)) {
                     return KC_SJIS;
                  } else if(c >= 0xfd && c <= 0xfe) {
                     return KC_EUC;
                  }
               }
               if((c = *(++ptr)) == '\0')
                  break;
            }
            ptr++;
            continue;
         }

         if (c >= 0xe0 && c <= 0xef) {
            if ((c = *(++ptr)) == '\0')
               break;
            if ((c >= 0x40 && c <= 0x7e) || (c >= 0x80 && c <= 0xa0))
               return KC_SJIS;
            if (c >= 0xfd && c <= 0xfe)
               return KC_EUC;
            if (c >= 0xa1 && c <= 0xfc)
               expected = KC_EUCORSJIS;
         }
      }
#if 1
      if (0xf0 <= c && c <= 0xfe)
         return KC_EUC;
#endif
      ptr++;
   }

   ptr = str;
   c2 = 0;
   while ((c1 = *ptr++) != '\0') {
      if (((c2 >  0x80 && c2 < 0xa0) || (c2 >= 0xe0 && c2 < 0xfd)) &&
          ((c1 >= 0x40 && c1 < 0x7f) || (c1 >= 0x80 && c1 < 0xfd)))
      {
         sjis_c++, c1 = *ptr++;
      }
      c2 = c1;
      if (c1 == '\0') break;
   }

/*
  if(sjis_c == 0)
  expected = KC_EUC;
  else {
*/
   {
      ptr = str, c2 = 0;
      while((c1 = *ptr++) != '\0') {
         if((c2 > 0xa0  && c2 < 0xff) &&
            (c1 > 0xa0  && c1 < 0xff))
         {
            euc_c++, c1 = *ptr++;
         }
         c2 = c1;
         if (c1 == '\0') break;
      }
      if(sjis_c > euc_c)
         expected = KC_SJIS;
      else if (euc_c > 0)
         expected = KC_EUC;
      else 
         expected = KC_ASCII;
   }

   /* FIXME!! nnnnm */
   if (g_utf8_validate (str, strlen (str), NULL))
      return KC_UTF8;
   else
      return expected;
}


const gchar *
japanese_detect_charset (const gchar *str)
{
   gint detected;

   if (g_utf8_validate (str, strlen (str), NULL))
      return CHARSET_UTF8;

   detected = detect_kanji (str);

   switch (detected) {
   case KC_EUC:
      return CHARSET_EUC_JP;
   case KC_JIS:
      return CHARSET_JIS;
   case KC_SJIS:
      return CHARSET_SJIS;
   case KC_UTF8:
      return CHARSET_UTF8;
   default:
      return CHARSET_ASCII;
   }

   return NULL;
}
