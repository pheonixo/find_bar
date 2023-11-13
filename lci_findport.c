#include "lci_findport.h"

static void
visible_set(PhxObject *obj, _Bool visible) {
  if (visible) {
    obj->state &= ~(0x00000001U << (VISUAL + 0));
  } else {
    obj->state |= (0x00000001U << (VISUAL + 0));
  }
}

static __inline__ _Bool
visible_get(PhxObject *obj) {
  return ((obj->state & (0x00000001U << (VISUAL + 0))) == 0);
}

static void
sensitive_set(PhxObject *obj, _Bool sensitive) {
    // must be visiable
  if (visible_get(obj)) {
    if (sensitive)
      obj->state &= ~(0x00000001U << (VISUAL + 1));
    else
      obj->state |= (0x00000001U << (VISUAL + 1));
  }
}

static __inline__ _Bool
sensitive_get(PhxObject *obj) {
  return ((obj->state & (0x00000001U << (VISUAL + 1))) == 0);
}

/* Allows shutting off of button outlines */
static void
frame_draw_set(PhxObject *obj, _Bool draws) {
    // must be visiable
  if (visible_get(obj)) {
    if (draws)
      obj->state &= ~(0x00000001U << (VISUAL + 2));
    else
      obj->state |= (0x00000001U << (VISUAL + 2));
  }
}

static __inline__ _Bool
frame_draw_get(PhxObject *obj) {
  return ((obj->state & (0x00000001U << (VISUAL + 2))) == 0);
}

#pragma mark *** TextMarks ***

#if USE_MARKS

static int
_text_marks_offsets_compare(const void *a, const void *b) {

  int key = *((int*)a);
  PhxMark *mAsk0 = (PhxMark*)b;
  int lineStart  = mAsk0->offset;
  if (lineStart == INT_MAX)  return -1;

  PhxMark *mAsk1 = mAsk0 + 1;
  int lineEnd    = mAsk1->offset;

  if (key < lineStart)  return -1;
  if ((key == lineStart) || (key < lineEnd))  return 0;
  return 1;
}

static int
_text_marks_lines_compare(const void *a, const void *b) {

  int key = *((int*)a);
  PhxMark *mAsk0 = (PhxMark*)b;
  if (mAsk0->offset == INT_MAX) {
    if (key >= mAsk0->line)  return 0;
    return -1;
  }
  int lineStart = mAsk0->line;

  PhxMark *mAsk1 = mAsk0 + 1;
  int lineEnd = (mAsk1->offset == INT_MAX) ? INT_MAX : mAsk1->line;

  if (key < lineStart)  return -1;
  if ((key == lineStart) || (key < lineEnd))  return 0;
  return 1;
}

static void
text_marks_initialize_for(PhxObjectTextview *otxt) {

  if (otxt->newline_list != NULL)
    free(otxt->newline_list);

    // this @ time typing this almost 2800 lines
  otxt->newline_list = malloc(MARK_ALLOC * sizeof(PhxMark));
  memset(otxt->newline_list, 0, (MARK_ALLOC * sizeof(PhxMark)));
  otxt->list_size  = MARK_ALLOC;
  otxt->list_ldelta = 0;
  otxt->list_dirtyo = INT_MAX;

  PhxMark *mPtr = otxt->newline_list;
  mPtr->type = PHXLINE;
  //mPtr->offset = 0;
  //mPtr->line = 0;
  otxt->list_nil = 1;

  if (otxt->string != NULL) {
    char *nPtr = otxt->string;
    do {
      nPtr = strchr(nPtr, '\n');
      if (nPtr == NULL)  break;
      (++mPtr)->type = PHXLINE;
      mPtr->offset = (++nPtr) - otxt->string;
      mPtr->line = otxt->list_nil;
      if ((++otxt->list_nil) == otxt->list_size) {
        size_t newSz = otxt->list_size + MARK_ALLOC;
        PhxMark *newPtr
                    = realloc(otxt->newline_list, (newSz * sizeof(PhxMark)));
        if (newPtr == NULL) {
          puts("realloc failure: text_marks_initialize_for");
          return;
        }
        otxt->newline_list = newPtr;
        otxt->list_size = newSz;
        mPtr = &otxt->newline_list[(otxt->list_nil - 1)];
      }
    } while (1);
  }
  (++mPtr)->type = PHXLINE;
  mPtr->offset = INT_MAX;
  mPtr->line = otxt->list_nil;
  if ((otxt->list_nil + 1) == otxt->list_size) {
    size_t newSz = otxt->list_size + MARK_ALLOC;
    PhxMark *newPtr = realloc(otxt->newline_list, (newSz * sizeof(PhxMark)));
    if (newPtr == NULL) {
      puts("realloc failure: text_marks_initialize_for");
      return;
    }
    otxt->newline_list = newPtr;
    otxt->list_size = newSz;
  }
}

static void
text_marks_DnD_copy_update(PhxObjectTextview *tv) {

  PhxMark *insert_mark, *release_mark, *drop_mark;

  drop_mark    = bsearch(&tv->drop.offset, tv->newline_list,
                                  tv->list_nil,
                                  sizeof(PhxMark),
                                  _text_marks_offsets_compare);
  insert_mark  = bsearch(&tv->insert.offset, tv->newline_list,
                                  tv->list_nil,
                                  sizeof(PhxMark),
                                  _text_marks_offsets_compare);
  release_mark = bsearch(&tv->release.offset, insert_mark,
                                  tv->list_nil - insert_mark->line,
                                  sizeof(PhxMark),
                                  _text_marks_offsets_compare);
      // add 1 for cushion for now
  int lines_affected = release_mark->line - insert_mark->line;
  if (lines_affected == 0) {
    int delta = tv->release.offset - tv->insert.offset;
    while ((++drop_mark)->offset != INT_MAX)
      drop_mark->offset += delta;
    return;
  }

  if ((tv->list_nil + lines_affected + 1) >= tv->list_size) {
    puts("realloc: text_marks_DnD_copy_update"); return;
  }
  memmove(drop_mark + lines_affected, drop_mark,
               (tv->list_nil + 1 - drop_mark->line) * sizeof(PhxMark));
  if (drop_mark < insert_mark) {
    insert_mark += lines_affected;
    release_mark += lines_affected;
  }

  int ndx = drop_mark->line;
  int delta = tv->drop.offset - drop_mark->offset;
  delta += (insert_mark + 1)->offset - tv->insert.offset;
  (drop_mark + 1)->offset = drop_mark->offset + delta;
  (++drop_mark)->line = (++ndx);

  while ((--lines_affected) != 0) {
    insert_mark++;
    delta = (insert_mark + 1)->offset - insert_mark->offset;
    (drop_mark + 1)->offset = drop_mark->offset + delta;
    (++drop_mark)->line = (++ndx);
  }

  delta = tv->release.offset - tv->insert.offset;
  while ((++drop_mark)->offset != INT_MAX) {
    drop_mark->offset += delta;
    drop_mark->line = (++ndx);
  }
  drop_mark->line = (tv->list_nil = (++ndx));
}

/* could possibly invert selections to decrease code size */
static void
text_marks_DnD_move_update(PhxObjectTextview *tv) {

  PhxMark *insert_mark, *release_mark, *drop_mark;

  if (tv->drop.offset < tv->insert.offset) {

    drop_mark    = bsearch(&tv->drop.offset, tv->newline_list,
                                  tv->list_nil,
                                  sizeof(PhxMark),
                                  _text_marks_offsets_compare);
    release_mark = bsearch(&tv->release.offset, drop_mark,
                                  tv->list_nil - drop_mark->line,
                                  sizeof(PhxMark),
                                  _text_marks_offsets_compare);
    insert_mark  = bsearch(&tv->insert.offset, drop_mark,
                                  release_mark->line + 1 - drop_mark->line,
                                  sizeof(PhxMark),
                                  _text_marks_offsets_compare);

    if (insert_mark == drop_mark) {
      if (release_mark != insert_mark) {
        int delta = tv->insert.offset - tv->drop.offset;
        while (release_mark > drop_mark) {
          release_mark->offset -= delta;
          release_mark--;
        }
      }
      return;
    }

      // add 1 for cushion for now
    int lines_affected = release_mark->line - insert_mark->line + 1;
    if ((tv->list_nil + lines_affected + 1) >= tv->list_size) {
      puts("realloc: text_marks_DnD_move_update"); return;
    }
      // copies, at min, the next line after insert_mark i=0+1 to i=0+1+la
    PhxMark *mInsert = &tv->newline_list[(tv->list_nil + 1)];
    memmove(mInsert, insert_mark, (lines_affected * sizeof(PhxMark)));
    lines_affected--;
      // guard against possible overwrite '(++drop_mark)->offset'
    PhxMark *drop_preserve = drop_mark;

      // move full lines between release and drop
    int delta = tv->release.offset - tv->insert.offset;
    while (insert_mark > drop_mark) {
      release_mark->offset = insert_mark->offset + delta;
      --release_mark, --insert_mark;
    }

    if (lines_affected != 0) {
      drop_mark = drop_preserve;
      (drop_mark + 1)->offset = tv->drop.offset
                               + (mInsert + 1)->offset - tv->insert.offset;
      while ((--lines_affected) != 0) {
        drop_mark++, mInsert++;
        (drop_mark + 1)->offset = drop_mark->offset
                                 + (mInsert + 1)->offset - mInsert->offset;
      }
    }

  } else {

    insert_mark  = bsearch(&tv->insert.offset, tv->newline_list,
                                  tv->list_nil,
                                  sizeof(PhxMark),
                                  _text_marks_offsets_compare);
    drop_mark    = bsearch(&tv->drop.offset, insert_mark,
                                  tv->list_nil - insert_mark->line,
                                  sizeof(PhxMark),
                                  _text_marks_offsets_compare);
    if (insert_mark == drop_mark)  return;

    release_mark = bsearch(&tv->release.offset, insert_mark,
                                  drop_mark->line + 1 - insert_mark->line,
                                  sizeof(PhxMark),
                                  _text_marks_offsets_compare);

    if (insert_mark == release_mark) {
      int delta = tv->release.offset - tv->insert.offset;
      while (insert_mark < drop_mark)
        (++insert_mark)->offset -= delta;
      return;
    }
      // add 1 for cushion for now
    int lines_affected = release_mark->line - insert_mark->line + 1;
    if ((tv->list_nil + lines_affected + 1) >= tv->list_size) {
      puts("realloc: text_marks_DnD_move_update"); return;
    }
      // copies, at min, the next line after insert_mark i=0+1 to i=0+1+la
    PhxMark *mInsert = &tv->newline_list[(tv->list_nil + 1)];
    memmove(mInsert, insert_mark, (lines_affected * sizeof(PhxMark)));
    lines_affected--;
    PhxMark *release_preserve = release_mark;

    int delta = tv->release.offset - tv->insert.offset;
    while (release_mark < drop_mark)
      (++insert_mark)->offset = (++release_mark)->offset - delta;

    if (lines_affected != 0) {
        // remaining length of drop line to drop insert
        // remaining length of initial first line grab
      if (release_preserve == drop_mark)
        delta = tv->drop.offset - tv->release.offset
               + (mInsert + 1)->offset - mInsert->offset;
      else
        delta = tv->drop.offset - drop_mark->offset
               + (mInsert + 1)->offset - tv->insert.offset;
      (insert_mark + 1)->offset = insert_mark->offset + delta;
      while ((--lines_affected) != 0) {
        insert_mark++, mInsert++;
        (insert_mark + 1)->offset = insert_mark->offset
                                   + (mInsert + 1)->offset - mInsert->offset;
      }
    }
  }
}

static void
text_marks_update(PhxObjectTextview *tv) {

  if ((tv->type == PHX_LABEL) || (tv->type == PHX_TEXTBUFFER)) return;
  if (tv->list_dirtyo == INT_MAX)  return;

  PhxMark *line_mark = bsearch(&tv->list_dirtyo, tv->newline_list,
                                tv->list_nil, sizeof(PhxMark),
                                _text_marks_offsets_compare);
  char *sPtr = tv->string;
  char *nPtr, *rdPtr = &tv->string[line_mark->offset];

  int ndx = line_mark->line + 1;
  PhxMark *mPtr = line_mark + 1;

  if (tv->gap_start != (tv->str_nil + 1)) {
    char *gsPtr = &tv->string[tv->gap_start];
    char *gePtr = &tv->string[tv->gap_end];
    if (rdPtr < gsPtr)
      do {
        nPtr = strchr(rdPtr, '\n');
        if ((rdPtr <= gsPtr) && (nPtr >= gsPtr))  break;
        if (nPtr == NULL)  goto lastentry;
        mPtr->offset = (rdPtr = (++nPtr)) - sPtr;
        mPtr->line = ndx;
        if ((++ndx) == tv->list_size) {
          size_t newSz = tv->list_size + MARK_ALLOC;
          PhxMark *newPtr
                      = realloc(tv->newline_list, (newSz * sizeof(PhxMark)));
          if (newPtr == NULL) {
            puts("realloc failure: text_marks_update");
            return;
          }
          tv->newline_list = newPtr;
          tv->list_size = newSz;
          mPtr = &tv->newline_list[(ndx - 1)];
        }
        mPtr++;
      } while (1);

    sPtr += (size_t)(gePtr - gsPtr);
    rdPtr = gePtr;
  }
  do {
    nPtr = strchr(rdPtr, '\n');
    if (nPtr == NULL)  break;
    mPtr->offset = (rdPtr = (++nPtr)) - sPtr;
    mPtr->line = ndx;
    if ((++ndx) == tv->list_size) {
      size_t newSz = tv->list_size + MARK_ALLOC;
      PhxMark *newPtr
                  = realloc(tv->newline_list, (newSz * sizeof(PhxMark)));
      if (newPtr == NULL) {
        puts("realloc failure: text_marks_update");
        return;
      }
      tv->newline_list = newPtr;
      tv->list_size = newSz;
      mPtr = &tv->newline_list[(ndx - 1)];
    }
    mPtr++;
  } while (1);

lastentry:
  mPtr->offset = INT_MAX;
  mPtr->line = (tv->list_nil = ndx);

  tv->list_ldelta = 0;
  tv->list_dirtyo = INT_MAX;
}

#endif


#pragma mark *** TextBuffer ***

// given a location
// scroll if needed to view that location
static void
location_auto_scroll(PhxObjectTextview *tv, location *point) {

  int delta;
    // x auto-scroll
  delta = 0;
  int x = point->x;
  int font_em = tv->font_em;
  if (x < (tv->bin.x + font_em)) {
    delta = (tv->bin.x + font_em) - x;
    if (delta > tv->bin.x)  delta = tv->bin.x;
  } else if (x >= (tv->bin.w - font_em)) {
    delta = (tv->bin.w - font_em) - x;
  }
  tv->bin.x -= delta;
  tv->bin.w -= delta;
    // y auto-scroll
  int y = (point->y / font_em) * font_em;
  delta = 0;
  if (y <= tv->bin.y) {
    delta = tv->bin.y - y;
  } else if (y > (tv->bin.h - font_em)) {
    delta = (tv->bin.h - font_em) - y;
  }
  tv->bin.y -= delta;
  tv->bin.h -= delta;
}

// given a offset to text location
// return for temp->offset positions temp->x, temp->y
static void
location_for_offset(PhxObjectTextview *tv, location *temp) {

#if USE_MARKS
  text_marks_update(tv);

  char *gsPtr = &tv->string[tv->gap_start];
  char *gePtr = &tv->string[tv->gap_end];

  PhxMark *line_mark = bsearch(&temp->offset, tv->newline_list,
                                tv->list_nil + 1, sizeof(PhxMark),
                                _text_marks_offsets_compare);
  int x = 0;
  if (line_mark->offset == INT_MAX) {
    line_mark--;
    x = INT_MAX;
  }
  temp->y = line_mark->line * tv->font_em;
  char *rdPtr = &tv->string[line_mark->offset];
  char *textPtr = &tv->string[temp->offset];
  if (textPtr >= gsPtr)  textPtr += gePtr - gsPtr;
  if (rdPtr >= gsPtr)  {
    rdPtr += gePtr - gsPtr;
both_updated:
    while (rdPtr < textPtr)
      x += tv->glyph_widths[(unsigned)(*rdPtr)], rdPtr++;
  } else {
    while (rdPtr < textPtr) {
      x += tv->glyph_widths[(unsigned)(*rdPtr)];
      if ((++rdPtr) == gsPtr) {  rdPtr = gePtr; goto both_updated;  }
    }
  }
  temp->x = x;
#else
  char *rdPtr = tv->string;
  char *gsPtr = &tv->string[tv->gap_start];
  char *gePtr = &tv->string[tv->gap_end];
  char *snPtr = &tv->string[tv->str_nil];
  char *textPtr = &tv->string[temp->offset];
  if (rdPtr >= gsPtr)    rdPtr += gePtr - gsPtr;
  if (textPtr >= gsPtr)  textPtr += gePtr - gsPtr;

  if (rdPtr < textPtr) {
    char *nPtr;
    int font_em = tv->font_em;
    int y = 0;
    do {
      nPtr = strchr(rdPtr, '\n');
      if ((nPtr >= gsPtr) && (rdPtr < gsPtr))
        nPtr = strchr(gePtr, '\n');
      if (nPtr == NULL)    {  nPtr = snPtr;   break;  }
      if (nPtr >= textPtr) {  nPtr = textPtr; break;  }
      if ((rdPtr = nPtr + 1) == gsPtr)  rdPtr = gePtr;
      y += font_em;
    } while (1);

    int x = 0;
      // on editing line
    if (rdPtr < textPtr) {
      if ((rdPtr < gsPtr) && (nPtr > gsPtr)) {
        while (rdPtr < gsPtr)
          x += tv->glyph_widths[(unsigned)(*rdPtr)], rdPtr++;
        rdPtr = gePtr;
      }
      while (rdPtr < textPtr)
        x += tv->glyph_widths[(unsigned)(*rdPtr)], rdPtr++;
    }
    temp->x = x;
    temp->y = y;
    return;
  }
  temp->x = 0;
  temp->y = 0;
#endif
}

// given positions temp->x, temp->y
// return a temp->offset for that position
static void
location_for_point(PhxObjectTextview *tv, location *loc) {

#if USE_MARKS
  text_marks_update(tv);
#endif

  char *nPtr, *rdPtr = tv->string;
  char *gsPtr = &tv->string[tv->gap_start];
  char *gePtr = &tv->string[tv->gap_end];

  int x = loc->x;

#if USE_MARKS
  int y = loc->y / tv->font_em;
  loc->y = 0;
    // used to place with character offset, instead of mouse click
  if (y > 0) {
    PhxMark *line_mark = bsearch(&y, tv->newline_list,
                                 tv->list_nil + 1, sizeof(PhxMark),
                                 _text_marks_lines_compare);
    if (line_mark->offset == INT_MAX) {
      line_mark--;
      x = INT_MAX;
    }
    rdPtr = &tv->string[line_mark->offset];
    loc->y = line_mark->line * tv->font_em;
  }
  if (rdPtr >= gsPtr) rdPtr += gePtr - gsPtr;
  nPtr = strchr(rdPtr, '\n');
  if ((nPtr >= gsPtr) && (rdPtr <= gsPtr))
    nPtr = strchr(gePtr, '\n');
#else
  if (loc->y <= 0) {
    loc->y = 0;
    nPtr = strchr(rdPtr, '\n');
  } else {
    int font_em = tv->font_em;
    int line_no = loc->y / font_em;
    int found_line = 0;
    do {
      nPtr = strchr(rdPtr, '\n');
      if ((nPtr >= gsPtr) && (rdPtr <= gsPtr))
        nPtr = strchr(gePtr, '\n');
      if ((nPtr == NULL) || (found_line == line_no))  break;
      found_line++;
      if ((rdPtr = nPtr + 1) == gsPtr)
        rdPtr = gePtr;
    } while (1);
    if (found_line < line_no)  x = INT_MAX;
    loc->y = (found_line * font_em);
  }
#endif
  if (nPtr == NULL)
    nPtr = &tv->string[tv->str_nil];
  int sum = 0;
  while (rdPtr < nPtr) {
    int gw = tv->glyph_widths[(unsigned)(*rdPtr)];
    if ((sum + gw) > x) {
      if ((x - ((gw + 1) >> 1)) >= sum)  sum += gw, rdPtr++;
      break;
    }
    if ((++rdPtr) == gsPtr)  rdPtr = gePtr;
    if ((sum += gw) == x)  break;
  }
  if (rdPtr > gsPtr)  rdPtr -= gePtr - gsPtr;
  loc->offset = rdPtr - tv->string;
    // used to place with character offset, instead of mouse click
  loc->x = sum;
}

// while editing, gap_buffer moved, set insert info for
// new insert position. This allows delay of having to update
// PhxMark(s) for line_marks
static void
location_update_for_edit(PhxObjectTextview *tv, int sz) {

  if ((tv->type == PHX_LABEL) || (tv->type == PHX_TEXTBUFFER)) return;

#if USE_MARKS
  char *rdPtr = &tv->string[tv->insert.offset];
  if (sz == 0)  goto update_finish;
  if (sz == 1) {
    tv->insert.offset++;
    if (*rdPtr != '\n')
      tv->insert.x += tv->glyph_widths[(unsigned)(*rdPtr)];
    else {
      tv->insert.x = 0;
      tv->insert.y += tv->font_em;
      tv->list_ldelta |= 1;
    }
    goto update_finish;
  }
    // delete 1 character, gap move 'down' 1
  if (sz == -1) {
    tv->insert.offset--;
    if (*(--rdPtr) != '\n')
      tv->insert.x -= tv->glyph_widths[(unsigned)(*rdPtr)];
    else {
      tv->insert.y -= tv->font_em;
      tv->list_ldelta |= 1;
      if (tv->insert.y < 0) {
        tv->insert.x = (tv->insert.y = 0); tv->insert.offset = 0;
        goto update_finish;
      }
      int x = 0;
      while ((--rdPtr) >= tv->string) {
        if (*rdPtr == '\n')  break;
        x += tv->glyph_widths[(unsigned)(*rdPtr)];
      }
      tv->insert.x = x;
    }
    goto update_finish;
  }
  if (sz < 0) {
    printf("    unhandled location_update sz: %d\n", sz);
    goto update_finish;
  }
  //if (sz > 1)
    // sz characters were pasted in
  char *endPtr = &tv->string[(tv->insert.offset + sz)];
  char *nPtr = memchr(rdPtr, '\n', sz);
  if (nPtr != NULL) {
    tv->insert.x = 0;
    tv->insert.y += tv->font_em;
    tv->list_ldelta |= 1;
    do {
      rdPtr = nPtr + 1;
      if (rdPtr >= endPtr)  break;
      nPtr = memchr(rdPtr, '\n', (endPtr - rdPtr));
      if (nPtr == NULL)  break;
      tv->insert.y += tv->font_em;
    } while (1);
  }
  while (rdPtr < endPtr)
    tv->insert.x += tv->glyph_widths[(unsigned)(*rdPtr)], rdPtr++;
  tv->insert.offset += sz;

update_finish:
#else
  location_for_offset(tv, &tv->insert);
#endif
  location_auto_scroll(tv, &tv->insert);
  tv->interim.x = (tv->release.x = tv->insert.x);
  tv->interim.y = (tv->release.y = tv->insert.y);
  tv->interim.offset = (tv->release.offset = tv->insert.offset);
}

static void
text_buffer_reset(PhxObjectTextview *otxt) {

  if ((otxt->str_nil + 1) != otxt->gap_start) {
    memmove(&otxt->string[otxt->gap_start], &otxt->string[otxt->gap_end],
                                  (size_t)(otxt->str_nil + 1 - otxt->gap_end));
    int gapSz = otxt->gap_end - otxt->gap_start;
    otxt->str_nil -= gapSz;
    otxt->gap_start = otxt->str_nil + 1;
    otxt->gap_end = otxt->gap_start + gapSz;
    otxt->gap_delta = 0;
  }

  if (otxt->type != PHX_TEXTBUFFER) {
  #if USE_MARKS
    if (otxt->list_dirtyo != INT_MAX)  text_marks_update(otxt);
  #endif
  }
  otxt->dirtyo = INT_MAX;
}

static void
text_buffer_get(char *dst, PhxObjectTextview *otxt, int start, int end) {

  size_t sz = end - start;
    // translate to storage system
  if (start >= otxt->gap_start) {
    start += otxt->gap_end - otxt->gap_start;
  } else if (end > otxt->gap_start) {
      // split copy, accesses gap points
    end += otxt->gap_end - otxt->gap_start;
    sz = otxt->gap_start - start;
    memmove(dst, &otxt->string[start], sz);
    dst += sz;
    start = otxt->gap_end;
    sz = end - start;
  }
  memmove(dst, &otxt->string[start], sz);
  dst[sz] = 0;
}

static void
text_buffer_for_display(char *dst, PhxObjectTextview *otxt,
                                                    location *tempS,
                                                    location *tempE) {
#if USE_MARKS
  if (otxt->list_ldelta != 0)  text_marks_update(otxt);

  PhxMark *line_mark = otxt->newline_list;
  int y = otxt->bin.y / otxt->font_em;
    // used to place with character offset, instead of mouse click
  if (y != 0) {
    line_mark = bsearch(&y, otxt->newline_list,
                            otxt->list_nil, sizeof(PhxMark),
                            _text_marks_lines_compare);
    if (line_mark == NULL) {
      printf("requested line: %d of %d available\n", y, otxt->list_nil);
      puts("ERROR: location_for_point() line_mark == NULL");
      return;
    }
    if (line_mark->offset == INT_MAX)  line_mark--;
  }
  tempS->x      = 0;
  tempS->y      = line_mark->line * otxt->font_em;
  tempS->offset = line_mark->offset;

  y = (otxt->bin.h + otxt->font_em) / otxt->font_em;
  line_mark = bsearch(&y, line_mark,
                          otxt->list_nil - line_mark->line,
                          sizeof(PhxMark),
                          _text_marks_lines_compare);
  tempE->x      = 0;  // unused
  tempE->y      = line_mark->line * otxt->font_em;
  if ((line_mark + 1)->offset == INT_MAX)
    tempE->offset = otxt->str_nil;
  else {
    int offset = line_mark->offset;
    if (otxt->list_dirtyo != INT_MAX)
      offset += otxt->gap_start - otxt->dirtyo - otxt->gap_delta;
    tempE->offset = offset;
  }
#else
  tempS->x = 0;
  tempS->y = otxt->bin.y;
  tempE->x = otxt->bin.w;
  tempE->y = otxt->bin.h + otxt->font_em;
  location_for_point(otxt, tempS);
  location_for_point(otxt, tempE);
#endif
  text_buffer_get(dst, otxt, tempS->offset, tempE->offset);
}

static int
_text_buffer_fit(PhxObjectTextview *otxt, int sz) {

  int gapSz = otxt->gap_end - otxt->gap_start;
  if (sz >= gapSz) {
      // make sure gap at end
    text_buffer_reset(otxt);
    int addSz = (sz + TGAP_ALLOC) & ~(TGAP_ALLOC - 1);
    size_t newSz = otxt->str_nil + (gapSz += addSz);
    char *newPtr = realloc(otxt->string, newSz);
    if (newPtr == NULL)  return -1;
    otxt->string = newPtr;
    memset(&newPtr[otxt->str_nil], 0, gapSz);
    otxt->gap_end = otxt->gap_start + gapSz;
  }
  return gapSz;
}

static void
text_buffer_replace(PhxObjectTextview *otxt, char *data, int sz) {

  int selSz = otxt->release.offset - otxt->insert.offset;
  if ((sz <= 0) || (selSz == 0))  return;

  int delta = (sz - selSz);
  int gapSz = _text_buffer_fit(otxt, delta);
  if (gapSz < 0)  return;

    // 3 possible scenerios if gap_end < str_nil:
    //   insert before gap_start && release after gap_end
    //   insert && release before gap_start
    //   insert && release after gap_end
    // simplicity sake use:
  text_buffer_reset(otxt);

    // open space for replace (non-editing movement)
  memmove(&otxt->string[(otxt->insert.offset + sz)],
          &otxt->string[otxt->release.offset],
          (otxt->str_nil - otxt->release.offset));
  memmove(&otxt->string[otxt->insert.offset], data, sz);
  otxt->string[(otxt->str_nil += delta)] = 0;
  otxt->gap_start = otxt->str_nil + 1;
  otxt->gap_delta = 0;

#if USE_MARKS
    // location_for_offset() will call update
  if (otxt->list_dirtyo > otxt->insert.offset)
    otxt->list_dirtyo = otxt->insert.offset;
#endif
  otxt->insert.offset += sz;
  location_for_offset(otxt, &otxt->insert);
  location_auto_scroll(otxt, &otxt->insert);
  otxt->interim.x = (otxt->release.x = otxt->insert.x);
  otxt->interim.y = (otxt->release.y = otxt->insert.y);
  otxt->interim.offset = (otxt->release.offset = otxt->insert.offset);
}

/* Insert, in addition to normal insert, can do insert into
 * a selection. Normally that is considered a delete than insert.
 * The delete/insert ability is also needed for 'insert' mode.
 * Instead it currently moves gap_end.
 *        Sadly need to alter such that
 * gap_end doesnt move for mark recording.
 */
static void
text_buffer_insert(PhxObjectTextview *otxt, char *data, int sz) {

    // check if inserting into a selection
  int selSz = otxt->release.offset - otxt->insert.offset;
  if ((sz == 0) && (selSz == 0))  return;

  int gapSz = _text_buffer_fit(otxt, (sz - selSz));
  if (gapSz < 0)  return;

#if USE_MARKS
  if (otxt->type != PHX_TEXTBUFFER) {
      // determine if selection has <newline>, 'force update' of marks if so
    if (selSz != 0) {
      otxt->list_ldelta
         |= (memchr(&otxt->string[otxt->insert.offset], '\n', selSz) != NULL);
    } else if ((otxt->state & (0x00000001 << (SHIFT + 10))) != 0) {
      otxt->list_ldelta |= (otxt->string[otxt->insert.offset] == '\n');
    }
  }
#endif

    // case: buffer reset not used
  if (otxt->gap_end < otxt->str_nil) {
    int delta = otxt->gap_start - otxt->release.offset;
    if (delta >= 0) {
      memmove(&otxt->string[(otxt->gap_end - delta)],
              &otxt->string[otxt->release.offset], delta);
    } else {
      memmove(&otxt->string[otxt->gap_start],
              &otxt->string[otxt->gap_end],
                     otxt->insert.offset - otxt->gap_end);
    }
    goto moved;
  }
    // if editing at end, don't move gap but move/add nil byte and gap_start
  if ( (otxt->release.offset != otxt->str_nil)
      && (otxt->insert.offset != otxt->gap_start) ) {
    memmove(&otxt->string[(otxt->release.offset + gapSz)],
            &otxt->string[otxt->release.offset],
              (otxt->str_nil + 1 - otxt->release.offset));
    otxt->str_nil += gapSz;
moved:
    otxt->gap_delta = selSz;
    otxt->gap_end = otxt->release.offset + gapSz;
  }
  if (sz == 1)
    otxt->string[otxt->insert.offset] = *data;
  else
    memmove(&otxt->string[otxt->insert.offset], data, sz);

  if (otxt->dirtyo > otxt->insert.offset) {
    otxt->dirtyo = otxt->insert.offset;
#if USE_MARKS
    otxt->list_dirtyo = otxt->insert.offset;
#endif
  }

#if USE_MARKS
  otxt->gap_start = otxt->insert.offset + sz;
#else
  otxt->insert.offset += sz;
  otxt->gap_start = otxt->insert.offset;
#endif
  if (otxt->release.offset == otxt->str_nil) {
    otxt->str_nil += (sz - selSz);
    otxt->string[otxt->str_nil] = 0;
    otxt->gap_start = otxt->str_nil + 1;
  } else if ((otxt->state & (0x00000001 << (SHIFT + 10))) != 0) {
// need testing looks wrong... explain
    printf("insert mode\n");
    if (selSz != 0)  sz = 0;
    otxt->gap_delta += sz;
    otxt->gap_end += sz;
  }
  location_update_for_edit(otxt, sz);
}

/* For selection, use release instead of insert. */
/* As with text_buffer_insert(), an action, other then editing,
   will 'reset' buffer. */
static void
text_buffer_delete(PhxObjectTextview *otxt) {

    // get info on selection
  int sz = otxt->release.offset - otxt->insert.offset;
    // cases ruled out by size == 0
  if (sz == 0) {
      // nothing to <backspace>
    if (otxt->insert.offset == 0)  return;
  }
    // debugging verify of selection reversal 'no-no'
  if (sz < 0) {  puts("error, text_buffer_delete()"); return;  }

    // if editing at end, don't move gap but move/add nil byte and gap_start
  if (otxt->release.offset == otxt->str_nil) {
      // on 'delete' moves nil sz amount, 0 if sz == 0
    if ((otxt->state & (0x00000001 << (SHIFT + 11))) != 0) {
        // remove flag
      otxt->state ^= (0x00000001 << (SHIFT + 11));
        // nothing to 'delete' at end of buffer, leave locations as is
      if (sz == 0)  return;
    }
#if USE_MARKS
    location_update_for_edit(otxt, -(!sz));
      // need end adjusted
    if (otxt->dirtyo > otxt->insert.offset)
      otxt->list_dirtyo = (otxt->dirtyo = otxt->insert.offset);
    otxt->str_nil += -(sz + !sz);
    otxt->string[otxt->str_nil] = 0;
    otxt->gap_start = otxt->str_nil + 1;
    return;
#else
    otxt->str_nil += -(sz + !sz);
    otxt->string[otxt->str_nil] = 0;
    otxt->insert.offset -= !sz;
    if (otxt->dirtyo > otxt->insert.offset)
      otxt->dirtyo = otxt->insert.offset;
    otxt->gap_start = otxt->str_nil + 1;
#endif
  } else {
    if (otxt->insert.offset != otxt->gap_start) {
      int gapSz = otxt->gap_end - otxt->gap_start;
        // move release the distance of gapSz, include nil with move
      memmove(&otxt->string[(otxt->release.offset + gapSz)],
              &otxt->string[otxt->release.offset],
              (otxt->gap_start - otxt->release.offset));
      otxt->gap_delta = sz;
      otxt->gap_end = otxt->release.offset + gapSz;
      otxt->str_nil += gapSz;
#if USE_MARKS
      if (otxt->type != PHX_TEXTBUFFER) {
        if (sz != 0) {
          otxt->list_ldelta
           |= (memchr(&otxt->string[otxt->insert.offset], '\n', sz) != NULL);
        }
      }
#endif
    }
    if ((otxt->state & (0x00000001 << (SHIFT + 11))) != 0) {
      otxt->state ^= (0x00000001 << (SHIFT + 11));
      if (!sz)  otxt->gap_end++, otxt->gap_delta++, sz++;
    }
#if USE_MARKS
    otxt->gap_start = otxt->insert.offset - !sz;
    if (otxt->dirtyo > (otxt->insert.offset - !sz))
      otxt->list_dirtyo = (otxt->dirtyo = (otxt->insert.offset - !sz));
#else
    otxt->insert.offset -= !sz;
    if (otxt->dirtyo > otxt->insert.offset)
      otxt->dirtyo = otxt->insert.offset;
    otxt->gap_start = otxt->insert.offset;
#endif
  }
  location_update_for_edit(otxt, -(!sz));
}

static void
text_buffer_select_all(PhxObjectTextview *otxt) {

  text_buffer_reset(otxt);
  otxt->insert.offset = 0;
  otxt->release.offset = otxt->str_nil;
  location_for_offset(otxt, &otxt->insert);
  location_for_offset(otxt, &otxt->release);
  cairo_region_t *crr;
  crr = cairo_region_create_rectangle(
           (cairo_rectangle_int_t *)&otxt->draw_box);
  gdk_window_invalidate_region(otxt->iface->parent_window, crr, FALSE);
  cairo_region_destroy(crr);
}

static void
text_buffer_copy(PhxObjectTextview *otxt) {

  int sz = otxt->release.offset - otxt->insert.offset;
  gtk_clipboard_set_text(clipboard, &otxt->string[otxt->insert.offset], sz);
}

static void
text_buffer_cut(PhxObjectTextview *otxt) {

  int sz = otxt->release.offset - otxt->insert.offset;
  gtk_clipboard_set_text(clipboard, &otxt->string[otxt->insert.offset], sz);
  text_buffer_delete(otxt);
}

static void
text_buffer_paste(PhxObjectTextview *otxt) {

  gchar *clip_string = gtk_clipboard_wait_for_text(clipboard);
  if (clip_string != NULL) {
    text_buffer_insert(otxt, clip_string, (int)strlen(clip_string));
    g_free(clip_string);
  }
}

static void
text_buffer_board_set(PhxObjectTextview *receiver, char *data, size_t sz) {

    // text_buffer_clear()
  receiver->str_nil = 0;
  receiver->insert.offset = 0;
  receiver->gap_start = 1;

  location_for_offset(receiver, &receiver->insert);
  location_auto_scroll(receiver, &receiver->insert);
  receiver->interim.x = (receiver->release.x = receiver->insert.x);
  receiver->interim.y = (receiver->release.y = receiver->insert.y);
  receiver->interim.offset
                      = (receiver->release.offset = receiver->insert.offset);
    // non-gap_move insert, adjust gapSz
  int gapSz = _text_buffer_fit(receiver, sz);
  if (gapSz < 0)  return;
  memmove(receiver->string, data, (size_t)sz);
  receiver->string[sz] = 0;
  receiver->str_nil = sz;
  receiver->gap_start = sz + 1;
  receiver->dirtyo = INT_MAX;

#if USE_MARKS
  text_marks_initialize_for(receiver);
#endif

  cairo_region_t *crr;
  crr = cairo_region_create_rectangle(
           (cairo_rectangle_int_t *)&receiver->draw_box);
  gdk_window_invalidate_region(receiver->iface->parent_window, crr, FALSE);
  cairo_region_destroy(crr);
}

/*
    DnD marks: No edit, no gap set up.
    Move: There is no sz change of marks, just offsets.
   Forward Move:
     Alter from locations insert to drop as delete.
     Then locations drop to list_nil as sz of copied insert.
   Backward Move:
     Alter from locations insert to list_nil, sz of copied, as delete.
     Then locations drop to list_nil, sz of copied, as insert.
    In DnD copy: No edit, no gap set up.
    Copy: There is a sz change of marks. No deletion.
*/
static BState  ui_interface_mouse_get(PhxInterface *);

static void
text_buffer_drag_release(PhxObjectTextview *otxt) {

    // release outside valid drops
  if (otxt->drop.offset == otxt->interim.offset) {
    location_auto_scroll(otxt, &otxt->insert);
    return;
  }

    // 'gap' should be at tail, use as copy buffer
  int sz = otxt->release.offset - otxt->insert.offset;
    // add 1 to sz for nil at end of copy to gap
  if (_text_buffer_fit(otxt, (sz + 1)) < 0)  return;

  memmove(&otxt->string[otxt->gap_start],
            &otxt->string[otxt->insert.offset], (size_t)sz);
    // make copied a c-string
  otxt->string[(otxt->gap_start + sz)] = 0;

  if (ui_interface_mouse_get(otxt->iface) == DND_COPY_CURSOR) {
    otxt->gap_start += sz;
    memmove(&otxt->string[(otxt->drop.offset + sz)],
            &otxt->string[otxt->drop.offset],
            (size_t)(otxt->gap_start - otxt->drop.offset));
    memmove(&otxt->string[otxt->drop.offset],
              &otxt->string[otxt->gap_start], (size_t)sz);
    otxt->str_nil += sz;
#if USE_MARKS
    text_marks_DnD_copy_update(otxt);
#else
    otxt->drop.offset += sz;
#endif
  } else {
    if (otxt->drop.offset < otxt->insert.offset) {
      memmove(&otxt->string[(otxt->drop.offset + sz)],
              &otxt->string[otxt->drop.offset],
              (size_t)(otxt->insert.offset - otxt->drop.offset));
      memmove(&otxt->string[otxt->drop.offset],
                &otxt->string[otxt->gap_start], (size_t)sz);
    } else if (otxt->drop.offset > otxt->release.offset) {
      memmove(&otxt->string[otxt->insert.offset],
              &otxt->string[otxt->release.offset],
              (size_t)(otxt->drop.offset - otxt->release.offset));
      memmove(&otxt->string[(otxt->drop.offset - sz)],
                &otxt->string[otxt->gap_start], (size_t)sz);
    } else
      return;
#if USE_MARKS
    text_marks_DnD_move_update(otxt);
    if (otxt->drop.offset > otxt->release.offset)
      otxt->drop.offset -= sz;
#else
    if (otxt->drop.offset < otxt->insert.offset)
      otxt->drop.offset += sz;
#endif
  }

    // different than 'insert'/'delete', no edit/gap assigned
    // also leaves drop hilighted

#if USE_MARKS
    // without hilight, don't subtract sz, or forward drops
  otxt->insert.offset = otxt->drop.offset;
  location_for_offset(otxt, &otxt->insert);
  otxt->release.offset = otxt->insert.offset + sz;
  location_for_offset(otxt, &otxt->release);
  otxt->interim.x = otxt->release.x;
  otxt->interim.y = otxt->release.y;
  otxt->interim.offset = otxt->release.offset;
#else
    // without hilight, don't subtract sz, forward drops add sz
  otxt->insert.offset = otxt->drop.offset;
  location_for_offset(otxt, &otxt->insert);
  location_auto_scroll(otxt, &otxt->insert);
  otxt->interim.x = (otxt->release.x = otxt->insert.x);
  otxt->interim.y = (otxt->release.y = otxt->insert.y);
  otxt->interim.offset = (otxt->release.offset = otxt->insert.offset);
#endif
}

#pragma mark *** Interface ***

static PhxObject *ui_object_create(PhxInterface *, PhxObjectType,
                                      PhxDrawHandler, int, int, int, int);
static _Bool      uio_configure_event(PhxInterface *, GdkEvent *, void *);
static int        event_meter(PhxInterface *, GdkEvent *, void *);
static _Bool      uio_draw_event(PhxInterface *, cairo_t *, void *);

static PhxInterface *
ui_interface_create(GtkDrawingArea *da, int x, int y, int w, int h) {

  if ((unsigned)OBJS_PWR <= 1) {
    puts("creation error: ui_interface_create... OBJS_PWR < 2");
    return NULL;
  }

  PhxInterface *iface = malloc(sizeof(PhxInterface));

  iface->type = PHX_IFACE;
  iface->state = 1U << 16; // assigning room for 1 OBJS_ALLOC
  iface->draw_box.x = (iface->mete_box.x = x);
  iface->draw_box.y = (iface->mete_box.y = y);
  iface->draw_box.w = (iface->mete_box.w = w);
  iface->draw_box.h = (iface->mete_box.h = h);

  int aSz = OBJS_ALLOC * sizeof(PhxObject*);
  iface->objects = malloc(aSz);
  memset(iface->objects, 0, aSz);
  aSz = w * h * sizeof(char);
  iface->event_map = malloc(aSz);
  memset(iface->event_map, 0, aSz);

    // content object, needed to handle events not in user defined objects
  iface->objects[0] = ui_object_create(iface, PHX_DRAWING, NULL, x, y, w, h);

  iface->has_focus = NULL;
  iface->parent_window = gtk_widget_get_window(GTK_WIDGET(da));
  iface->_configure_event = NULL;
  iface->reserved[3] = (iface->reserved[4] = NULL);
  iface->reserved[1] = (iface->reserved[2] = NULL);
  iface->reserved[0] = NULL;

  gtk_widget_set_can_focus(GTK_WIDGET(da), TRUE);
  gtk_widget_add_events(GTK_WIDGET(da), GDK_POINTER_MOTION_MASK
                                      | GDK_BUTTON1_MOTION_MASK
                                      | GDK_BUTTON_PRESS_MASK
                                      | GDK_BUTTON_RELEASE_MASK
                                      | GDK_KEY_PRESS_MASK
                                      | GDK_KEY_RELEASE_MASK
                                      | GDK_ENTER_NOTIFY_MASK
                                      | GDK_LEAVE_NOTIFY_MASK
                                      | GDK_FOCUS_CHANGE_MASK
                                      | GDK_STRUCTURE_MASK
                                      | GDK_SCROLL_MASK);
  g_signal_connect_swapped(G_OBJECT(da), "draw",
                                      G_CALLBACK(uio_draw_event), iface);
  g_signal_connect_swapped(G_OBJECT(da), "configure-event",
                                      G_CALLBACK(uio_configure_event), iface);
  void (*ecb) = G_CALLBACK(event_meter);
  g_signal_connect_swapped(G_OBJECT(da), "motion-notify-event",  ecb, iface);
  g_signal_connect_swapped(G_OBJECT(da), "button-press-event",   ecb, iface);
  g_signal_connect_swapped(G_OBJECT(da), "button-release-event", ecb, iface);
  g_signal_connect_swapped(G_OBJECT(da), "key-press-event",      ecb, iface);
  g_signal_connect_swapped(G_OBJECT(da), "key-release-event",    ecb, iface);
  g_signal_connect_swapped(G_OBJECT(da), "enter-notify-event",   ecb, iface);
  g_signal_connect_swapped(G_OBJECT(da), "leave-notify-event",   ecb, iface);
  g_signal_connect_swapped(G_OBJECT(da), "focus-in-event",       ecb, iface);
  g_signal_connect_swapped(G_OBJECT(da), "focus-out-event",      ecb, iface);
  g_signal_connect_swapped(G_OBJECT(da), "scroll-event",         ecb, iface);

  return iface;
}

static void
ui_interface_map(PhxInterface *iface, PhxObject *obj) {

  int allot = (iface->state >> 16) << OBJS_PWR;

  int ldx = 1;
  do {
    if (iface->objects[ldx] == NULL) {
      if ((ldx + 1) == allot) {
        iface->state += 1U << 16;
        size_t newSz = (allot + OBJS_ALLOC) * sizeof(PhxObject*);
        PhxObject *newPtr = realloc(iface->objects, newSz);
        if (newPtr == NULL) {
          puts("realloc failure: ui_interface_map");
          return;
        }
        iface->objects = (PhxObject**)newPtr;
        memset(&iface->objects[(ldx + 1)], 0,
                             (OBJS_ALLOC * sizeof(PhxObject*)));
      }
      break;
    }
    if (iface->objects[ldx] == obj)  break;
    ldx++;
  } while (1);

  obj->iface = iface;
  iface->objects[ldx] = obj;
  int sxdx = obj->draw_box.x,
      sydx = obj->draw_box.y;
  int exdx = sxdx + obj->draw_box.w,
      eydx = sydx + obj->draw_box.h;
  if (exdx > iface->mete_box.w)  exdx = iface->mete_box.w;
  if (eydx > iface->mete_box.h)  eydx = iface->mete_box.h;
  eydx *= iface->mete_box.w;
  sydx *= iface->mete_box.w;

  for (int x = sxdx; x < exdx; x++) {
    for (int y = sydx; y < eydx; y += iface->mete_box.w)
      iface->event_map[(x + y)] = (char)ldx;
  }
}

static void
ui_interface_refresh(PhxInterface *iface) {

  int mapSz = iface->mete_box.w * iface->mete_box.h * sizeof(char);
  char *newPtr = malloc(mapSz);
  if (newPtr == NULL) {
    puts("malloc failure: ui_interface_refresh");
    return;
  }
  free(iface->event_map);
  iface->event_map = newPtr;
    // setting map to 'drawing' object == 0
  memset(iface->event_map, 0, mapSz);

  int ldx = 0;
  PhxObject *obj;
  while ((obj = iface->objects[(++ldx)]) != NULL) {
    if (visible_get(obj)) {
      int sxdx = obj->draw_box.x,
          sydx = obj->draw_box.y;
      int exdx = sxdx + obj->draw_box.w,
          eydx = sydx + obj->draw_box.h;

      if (exdx > iface->mete_box.w)  exdx = iface->mete_box.w;
      if (eydx > iface->mete_box.h)  eydx = iface->mete_box.h;
      eydx *= iface->mete_box.w;
      sydx *= iface->mete_box.w;

      for (int x = sxdx; x < exdx; x++) {
        for (int y = sydx; y < eydx; y += iface->mete_box.w)
          iface->event_map[(x + y)] = (char)ldx;
      }
    }
  }
}

static void
ui_interface_set_cursor_named(GdkWindow *window, char *named) {

  GdkCursor *cursor = gdk_window_get_cursor(window);
  if (cursor != NULL)  g_object_unref((void*)cursor);
  cursor = NULL;
  if (named != NULL) {
    GdkDisplay *display = gdk_window_get_display(window);
    cursor = gdk_cursor_new_from_name(display, named);
  }
  gdk_window_set_cursor(window, cursor);
}

static void
ui_interface_mouse_set(PhxInterface *iface, BState value) {
  iface->state = (iface->state & ~15) | value;
}

static BState
ui_interface_mouse_get(PhxInterface *iface) {
  return (iface->state & 15);
}

#pragma mark *** Drawing ***

static void
draw_textview(PhxObject *b, cairo_t *cr) {

  PhxObjectTextview *tv = (PhxObjectTextview*)b;
  char *draw_buffer = tv->draw_buffer;

  cairo_set_source_rgba(cr, 1, 1, 1, 1);
  cairo_rectangle(cr, tv->mete_box.x, tv->mete_box.y,
                      tv->mete_box.w, tv->mete_box.h);
  cairo_fill(cr);

  if ((tv->string == NULL) || (*tv->string == 0))  return;

  double x = tv->draw_box.x,
         y = tv->draw_box.y;
  double font_em = tv->font_em;

  cairo_save(cr);

  cairo_rectangle(cr, x, y, tv->draw_box.w, tv->draw_box.h);
  cairo_clip(cr);

  cairo_select_font_face(cr, tv->font_name, CAIRO_FONT_SLANT_NORMAL,
                                               CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, tv->font_size);

  location tempS, tempE;
  text_buffer_for_display(draw_buffer, tv, &tempS, &tempE);

  x -= tv->bin.x;
  y -= tv->bin.y;
  double glyph_origin = tv->font_origin;
  double endline = tempE.y + font_em + y;
  y += tempS.y;

  _Bool focused = (tv == (PhxObjectTextview*)tv->iface->has_focus);
  double colour = 0.0;
  if ( (!focused) || (!sensitive_get((PhxObject*)tv)) )
    colour = 0.5;
  cairo_set_source_rgba(cr, colour, colour, colour, 1);
  char *tPtr = draw_buffer;
  do {
    char *nPtr = strchr(tPtr, '\n');
    if (nPtr != NULL)  *nPtr = 0;
    cairo_move_to(cr, x, (y + glyph_origin));
      // repace tabs with movement
    do {
      char *tab = strchr(tPtr, '\t');
      if (tab == NULL)  break;
      *tab = 0;
      cairo_show_text(cr, tPtr);
      cairo_rel_move_to(cr, tv->glyph_widths['\t'], 0);
        // re-insert back for case: insert block caret draw size
      *tab = '\t';
      tPtr = tab + 1;
    } while (1);
    cairo_show_text(cr, tPtr);
    if (nPtr == NULL)  break;
    if ((y += font_em) >= endline)  break;
    tPtr = nPtr + 1;
  } while (1);

  double x0, x1, y0, y1;
  x0 = tv->insert.x + tv->draw_box.x - tv->bin.x;
  x1 = tv->release.x + tv->draw_box.x - tv->bin.x;
  y0 = tv->insert.y + tv->draw_box.y - tv->bin.y;
  y1 = tv->release.y + tv->draw_box.y - tv->bin.y;

  if (tv->insert.offset == tv->release.offset) {
    if ( focused && sensitive_get((PhxObject*)tv) ) {
      if ((tv->state & (0x00000001 << (SHIFT + 10))) != 0) {
        int utf = draw_buffer[(tv->insert.offset - tempS.offset)];
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_rectangle(cr, x0, y0, tv->glyph_widths[utf], font_em);
        cairo_fill(cr);
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_move_to(cr, x0, y0 + glyph_origin);
        if (utf != '\t')
          cairo_show_text(cr, (const char*)&utf);
      } else {
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_rectangle(cr, x0, y0, 0.5, font_em);
        cairo_fill(cr);
      }
    }
  } else {
      // if dnd draw caret drop location (only if outside selection)
    if ( ((ui_interface_mouse_get(tv->iface) & MOUSE_DND) == MOUSE_DND)
        && (tv->drop.offset != tv->interim.offset) ) {
      double dx, dy;
      dx = tv->drop.x + tv->draw_box.x - tv->bin.x;
      dy = tv->drop.y + tv->draw_box.y - tv->bin.y;
      cairo_set_source_rgb(cr, 0, 0, 0);
      cairo_rectangle(cr, dx, dy, 0.5, font_em);
      cairo_fill(cr);
    }
      // ? colour change if not focus
    colour = 1.0;
    if ( !focused || !(sensitive_get((PhxObject*)tv)) )
      colour = 0.5;
    cairo_set_source_rgba(cr, 0, 0, colour, .2);
    if (tv->insert.y == tv->release.y) {
      cairo_rectangle(cr, x0, y0, (x1 - x0), font_em);
      cairo_fill(cr);
    } else {
      if (y0 > y1) {
          // needed for mouse movement, swap
        endline = x1, x1 = x0, x0 = endline;
        endline = y1, y1 = y0, y0 = endline;
      }
      if (x0 < tv->bin.x)  x0 = tv->draw_box.x;
      cairo_rectangle(cr, x0, y0, tv->draw_box.w, font_em);
      if ((endline = y0 + font_em) < y1)
        do {
          cairo_rectangle(cr, tv->draw_box.x, endline,
                              tv->draw_box.w, font_em);
        } while ((endline += font_em) < y1);
      if (x1 > tv->draw_box.x) {
        cairo_rectangle(cr, tv->draw_box.x, y1,
                            (x1 - tv->draw_box.x), font_em);
      }
      cairo_fill(cr);
    }
  }
  cairo_restore(cr);
}

static void
draw_label(PhxObject *b, cairo_t *cr) {

  PhxObjectTextview *label = (PhxObjectTextview*)b;

  if ((label->string == NULL) || (*label->string == 0))  return;

  cairo_select_font_face(cr, label->font_name, CAIRO_FONT_SLANT_NORMAL,
                                               CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, label->font_size);
  cairo_text_extents_t search_extents;
  cairo_text_extents(cr, label->string, &search_extents);

  int bx = 0;
  int whtsp = label->draw_box.w - search_extents.x_advance;
  if (whtsp > 0) {
    int position = label->state & HJST_MSK;
    if      (position == HJST_LFT) {  whtsp = 0;  }
    else if (position == HJST_CTR) {  whtsp /= 2; }
    bx -= whtsp;
  }

  double x = label->draw_box.x,
         y = label->draw_box.y;

  cairo_save(cr);
  cairo_rectangle(cr, x, y, label->draw_box.w, label->draw_box.h);
  cairo_clip(cr);

  cairo_set_source_rgba(cr, 0, 0, 0, 1);
  cairo_move_to(cr, x - bx, y + label->font_origin);
  cairo_show_text(cr, label->string);

  cairo_clip_preserve(cr);
  cairo_restore(cr);
}

static void
draw_button(PhxObject *b, cairo_t *cr) {

  PhxObjectButton *button = (PhxObjectButton *)b;
  const double degrees = M_PI / 180.0;
  double radius = button->draw_box.h / 6.5;

  double line_width = 0.5;
  double x = button->draw_box.x + line_width;
  double y = button->draw_box.y + line_width;
  double w = button->draw_box.w - (line_width * 2);
  double h = button->draw_box.h - (line_width * 2);

  cairo_new_sub_path(cr);
  cairo_arc(cr, x + w - radius, y + radius,
                radius, -90 * degrees,   0 * degrees);
  cairo_arc(cr, x + w - radius, y + h - radius,
                radius,   0 * degrees,  90 * degrees);
  cairo_arc(cr, x + radius, y + h - radius,
                radius,  90 * degrees, 180 * degrees);
  cairo_arc(cr, x + radius, y + radius,
                radius, 180 * degrees, 270 * degrees);
  cairo_close_path(cr);

  if ((button->state & 1) == 1)
    cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
  else
    cairo_set_source_rgba(cr, 0.94, 0.94, 0.94, 1);
  if (frame_draw_get((PhxObject*)button)) {
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0, 0, 0, 1);
    cairo_set_line_width(cr, line_width);
    cairo_stroke(cr);
  } else {
    cairo_fill(cr);
  }

    // shade bottom
  cairo_set_source_rgba(cr, 0.7, 0.7, 0.7, 0.25);
  cairo_rectangle(cr, x, y + h - 2 + line_width, w, 2);
  cairo_fill(cr);
}

static void
draw_combo_arrows(PhxObject *b, cairo_t *cr) {

  PhxObjectDrawing *odrw = (PhxObjectDrawing*)b;

  double line_width = 0.5;
  double x = odrw->draw_box.x;
  double y = odrw->draw_box.y - line_width;
  double w = odrw->draw_box.w;
  double h = odrw->draw_box.h + line_width;

  cairo_new_path(cr);  /* current path is not consumed by another */
  cairo_set_source_rgba(cr, 0.25, 0.25, 0.25, 1);
  cairo_move_to(cr, (x + (w/2)),  y);
  cairo_line_to(cr, (x + w),     (y + (h/2) - .5));
  cairo_line_to(cr,  x,          (y + (h/2) - .5));
  cairo_line_to(cr, (x + (w/2)),  y);
  cairo_move_to(cr, (x + (w/2)), (y + h));
  cairo_line_to(cr, (x + w),     (y + (h/2) + .5));
  cairo_line_to(cr,  x,          (y + (h/2) + .5));
  cairo_line_to(cr, (x + (w/2)), (y + h));
  cairo_fill(cr);
}

static void
draw_navigate(PhxObject *b, cairo_t *cr) {

  PhxObjectDrawing *odrw = (PhxObjectDrawing*)b;
  const double degrees = M_PI / 180.0;
  double radius = odrw->draw_box.h / 6.5;

  double line_width = 0.5;
  double x = odrw->draw_box.x + line_width;
  double y = odrw->draw_box.y + line_width;
  double w = odrw->draw_box.w - (line_width * 2);
  double h = odrw->draw_box.h - (line_width * 2);
  double sp = h / 5;

  if (odrw->type == PHX_NAVIGATE_RIGHT) {

    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - radius, y + radius,
                  radius, -90 * degrees,   0 * degrees);
    cairo_line_to(cr, x + w, y + h - radius);
    cairo_arc(cr, x + w - radius, y + h - radius,
                  radius,   0 * degrees,  90 * degrees);
    cairo_line_to(cr, x, y + h);
    cairo_line_to(cr, x, y);
    cairo_line_to(cr, x + w - radius, y);
    cairo_close_path(cr);

    double colour = (sensitive_get((PhxObject*)odrw)) ? 0.0 : 0.5;
    if ((odrw->state & 1) == 1)
      cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    else
      cairo_set_source_rgba(cr, 0.94, 0.94, 0.94, 1);
    if (frame_draw_get((PhxObject*)odrw)) {
      cairo_fill_preserve(cr);
      cairo_set_source_rgba(cr, colour, colour, colour, 1);
      cairo_set_line_width(cr, line_width);
      cairo_stroke(cr);
    } else {
      cairo_fill(cr);
    }

    colour += 0.2;
    cairo_set_source_rgba(cr, colour, colour, colour, 1);
    cairo_move_to(cr, (x + w - sp), (y + (h / 2)));
    cairo_line_to(cr,  x + sp,       y + sp);
    cairo_line_to(cr,  x + sp,      (y + h - sp));
    cairo_line_to(cr, (x + w - sp), (y + (h / 2)));
    cairo_fill(cr);

  } else if (odrw->type == PHX_NAVIGATE_LEFT) {

    w = odrw->draw_box.w - line_width;

    cairo_new_sub_path(cr);
    cairo_arc(cr, x + radius, y + radius,
                  radius, 180 * degrees, 270 * degrees);
    cairo_line_to(cr, x + w, y);
    cairo_line_to(cr, x + w, y + h);
    cairo_line_to(cr, x + radius , y + h);
    cairo_arc(cr, x + radius, y + h - radius,
                  radius,  90 * degrees, 180 * degrees);
    cairo_line_to(cr, x, y + radius);
    cairo_close_path(cr);

    double colour = (sensitive_get((PhxObject*)odrw)) ? 0.0 : 0.5;
    if ((odrw->state & 1) == 1)
      cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    else
      cairo_set_source_rgba(cr, 0.94, 0.94, 0.94, 1);
    if (frame_draw_get((PhxObject*)odrw)) {
      cairo_fill_preserve(cr);
      cairo_set_source_rgba(cr, colour, colour, colour, 1);
      cairo_set_line_width(cr, line_width);
      cairo_stroke(cr);
    } else {
      cairo_fill(cr);
    }

    colour += 0.2;
    cairo_set_source_rgba(cr, colour, colour, colour, 1);
    cairo_move_to(cr,  x + sp,      (y + (h / 2)));
    cairo_line_to(cr, (x + w - sp),  y + sp);
    cairo_line_to(cr, (x + w - sp), (y + h) - sp);
    cairo_line_to(cr,  x + sp,      (y + (h / 2)));
    cairo_fill(cr);
  }

    // shade bottom
  cairo_set_source_rgba(cr, 0.7, 0.7, 0.7, 0.25);
  cairo_rectangle(cr, x, y + h - 2 + line_width, w, 2);
  cairo_fill(cr);
}

static void
_ui_interface_draw(PhxInterface *iface, cairo_t *cr) {

  int ldx = (iface->type == PHX_IFACE);
  cairo_save(cr);
    do {
      PhxObject *obj = iface->objects[ldx];
      if (visible_get(obj)) {
        if (obj->_draw_cb != NULL)  obj->_draw_cb(obj, cr);
        if (obj->child != NULL) {
            // walk through any children
// what if no _draw_cb object
          PhxObject *add = obj->child;
          do {
            if (!(visible_get(add)))  break;
            if (add->_draw_cb != NULL)  add->_draw_cb(add, cr);
            add = add->child;
          } while (add != NULL);
        }
      }
    } while (iface->objects[(++ldx)] != NULL);
  cairo_restore(cr);
}

static gboolean
ui_bank_draw_event(PhxInterface *obank, cairo_t *cr, void *widget) {

  (void)widget;
  cairo_reset_clip(cr);

    // interface background
  cairo_set_source_rgba(cr, 0.94, 0.94, 0.94, 1);
  cairo_rectangle(cr, 0, 0, obank->mete_box.w, obank->mete_box.h);
  cairo_clip_preserve(cr);
  cairo_fill(cr);

  int set = ((PhxBank*)obank)->display_indicies >> 16;
  if (set != -1) {
    PhxObjectLabel *olbl = (PhxObjectLabel*)obank->objects[(unsigned)set];
    cairo_set_source_rgba(cr, 0, 0, 1, .2);
    cairo_rectangle(cr, olbl->mete_box.x + 1, olbl->mete_box.y,
                        olbl->mete_box.w, olbl->mete_box.h);
    cairo_fill(cr);
  }

  _ui_interface_draw(obank, cr);
  return TRUE;
}

static _Bool
uio_draw_event(PhxInterface *iface, cairo_t *cr, void *widget) {

  (void)widget;
  cairo_reset_clip(cr);

    // interface background
  cairo_rectangle(cr, 0, 0, iface->mete_box.w, iface->mete_box.h);
  cairo_clip(cr);

  _ui_interface_draw(iface, cr);
  return TRUE;
}

#pragma mark *** Objects ***

static PhxObject *
ui_object_create(PhxInterface *iface, PhxObjectType type,
                         PhxDrawHandler draw, int x, int y, int w, int h) {

  if ((type >= PHX_OBJECT_LAST) || (type < 0))  return NULL;

  PhxObject *obj;

  size_t sz;
  if ((type == PHX_TEXTVIEW)
          || (type == PHX_ENTRY)
          || (type == PHX_COMBO_LABEL)
          || (type == PHX_TEXTBUFFER)) {
    sz = sizeof(PhxObjectTextview);
  } else if (type == PHX_LABEL) {
    sz = sizeof(PhxObjectLabel);
  } else if ((type == PHX_BUTTON)
          || (type == PHX_BUTTON_LABELED)
          || (type == PHX_BUTTON_COMBO)
          || (type == PHX_NAVIGATE_LEFT)
          || (type == PHX_NAVIGATE_RIGHT)) {
    sz = sizeof(PhxObjectButton);
  } else {
    sz = sizeof(PhxObject);
  }
  obj = malloc(sz);
  memset(obj, 0, sz);

  obj->type = type;
  obj->draw_box.x = (obj->mete_box.x = x);
  obj->draw_box.y = (obj->mete_box.y = y);
  obj->draw_box.w = (obj->mete_box.w = w);
  obj->draw_box.h = (obj->mete_box.h = h);
  obj->_draw_cb = draw;
  obj->iface = iface;

    // want default button action
  if ((type == PHX_BUTTON)
          || (type == PHX_BUTTON_LABELED)
          || (type == PHX_BUTTON_COMBO)
          || (type == PHX_NAVIGATE_LEFT)
          || (type == PHX_NAVIGATE_RIGHT)) {
    ((PhxObjectButton*)obj)->_event_cb = NULL;
    ((PhxObjectButton*)obj)->bank = NULL;
  }

  if ((type == PHX_TEXTVIEW) || (type == PHX_ENTRY)) {
    PhxObjectTextview *otxt = (PhxObjectTextview*)obj;
      // 8.5" page @ 72 p/i with avg 7.5 p/char = 82 chars
      // 11" => 792p or 66 lines
      // based on 82 chars with 1080p @ 12pixels/line = 7380
    otxt->draw_buffer = malloc(BUFF_ALLOC);
    memset(otxt->draw_buffer, 0, BUFF_ALLOC);
  }
  return obj;
}

static void
ui_box_inset(PhxRectangle *box, int l, int t, int r, int b) {
  box->x += l;
  box->y += t;
  box->w -= l + r;
  box->h -= t + b;
}

// returns a glyph table, does not attach to a text-type object
// caller responible for allocated table
// caller decides if attaches. done this way for later possible
// cache of attributes, May store in iface? or something else
// Labels on set up need one shot info.
static char *
ui_textview_font_set(PhxObjectTextview *otxt, int line_height) {

  if ((otxt->type != PHX_TEXTVIEW) && (otxt->type != PHX_ENTRY)
      && (otxt->type != PHX_LABEL) && (otxt->type != PHX_COMBO_LABEL) )
    return NULL;

  int box_width = otxt->iface->mete_box.w;
  int box_height = otxt->iface->mete_box.h;

  cairo_surface_t *surface;
  surface = gdk_window_create_similar_surface(
                                        otxt->iface->parent_window,
                                        CAIRO_CONTENT_COLOR_ALPHA,
                                        box_width, box_height);
  cairo_t *cro = cairo_create(surface);
  cairo_select_font_face(cro, FONT_NAME, CAIRO_FONT_SLANT_NORMAL,
                                         CAIRO_FONT_WEIGHT_NORMAL);

  cairo_font_extents_t font_extents;
  double pixel_line_height = line_height;
  double font_size = line_height;
  cairo_set_font_size(cro, pixel_line_height);
  cairo_font_extents(cro, &font_extents);
  while (pixel_line_height
          < (int)(font_extents.ascent + font_extents.descent)) {
    font_size -= 0.5;
    cairo_set_font_size(cro, font_size);
    cairo_font_extents(cro, &font_extents);
  }
  otxt->font_name   = FONT_NAME;
  otxt->font_size   = font_size;
  otxt->font_origin = font_extents.ascent;
  otxt->font_em     = (int)(font_extents.ascent + font_extents.descent);

  char *glyph_widths = malloc(128 * sizeof(char));
  memset(glyph_widths, 0, 0x20);

  for (int idx = 0x20; idx <= 0x7e; idx++) {
    cairo_text_extents_t search_extents;
    int utf_str = idx;
    cairo_text_extents(cro, (const char*)&utf_str, &search_extents);
    glyph_widths[idx] = (unsigned)(search_extents.x_advance + 0.5);
  }
  glyph_widths[0x7f] = 1;
    // want a size for good representation with block_caret
  glyph_widths[0]    = glyph_widths[0x20];
  glyph_widths['\t'] = glyph_widths[0x20] << 1;

  cairo_destroy(cro);
  cairo_surface_destroy(surface);

  return glyph_widths;
}

static void
ui_textview_buffer_set(PhxObjectTextview *otxt, char *data) {

  if ((otxt->type != PHX_TEXTVIEW) && (otxt->type != PHX_ENTRY)
      && (otxt->type != PHX_LABEL) && (otxt->type != PHX_TEXTBUFFER))
    return;

  if (otxt->string != NULL)  free(otxt->string);

  if (otxt->type == PHX_LABEL) {
    char *str = (data != NULL) ? data : "";
    otxt->string = strdup(str);
    return;
  }

  if (otxt->type != PHX_TEXTBUFFER) {
    otxt->bin.w = otxt->draw_box.w;
    otxt->bin.h = otxt->draw_box.h;
  }

  size_t rdSz = (data != NULL) ? strlen(data) : 0;
  size_t bufSz = (rdSz + TGAP_ALLOC) & ~(TGAP_ALLOC - 1);
  if ((otxt->type == PHX_TEXTVIEW) || (otxt->type == PHX_TEXTBUFFER))
    if ((bufSz - rdSz) < (TGAP_ALLOC >> 1))  bufSz += TGAP_ALLOC;
  otxt->string = malloc(bufSz);
  memset(&otxt->string[rdSz], 'a', (bufSz - rdSz));
  if (rdSz)
    memmove(otxt->string, data, rdSz);
  otxt->string[rdSz] = 0;
  otxt->str_nil = rdSz;
  otxt->gap_start = rdSz + 1;
  otxt->gap_end = bufSz - 1;

  otxt->dirtyo = INT_MAX;
#if USE_MARKS
  if (otxt->type != PHX_TEXTBUFFER)
    text_marks_initialize_for(otxt);
#endif
}

/* uses PhxObjectTextview metrics to calcualate extents x_advance */
/* sPtr must be a c-string, but can return x_advance on <newline> */
static int
ui_text_extent_width(char *glyph_widths, char *sPtr) {

  int x_advance = 0;
  size_t s_len = strlen(sPtr);
  if (s_len != 0) {
    char *nPtr = memchr(sPtr, '\n', s_len);
    if (nPtr == NULL)  nPtr = sPtr + s_len;
    while (nPtr > sPtr)
      x_advance += glyph_widths[(unsigned)(*sPtr)], sPtr++;
  }
  return x_advance;
}

/* returns white space of label in drawing box */
static int
ui_label_set(PhxObjectLabel *olbl, char *str, int position) {

  if (olbl->type != PHX_LABEL)  return -1;

  char *gw = ui_textview_font_set((PhxObjectTextview*)olbl, olbl->draw_box.h);
  ui_textview_buffer_set((PhxObjectTextview*)olbl, str);
  olbl->state &= ~HJST_MSK;
  olbl->state |= position;

  int x_advance = ui_text_extent_width(gw, olbl->string);
  int whtsp = olbl->draw_box.w - x_advance;
  free(gw);
  return whtsp;
}

static int
ui_button_label_create(PhxInterface *iface, PhxObjectButton *obtn,
                                                   char *str, int position) {
  PhxObjectLabel *olbl;
  olbl = (PhxObjectLabel*)ui_object_create(iface, PHX_LABEL, draw_label,
                                  obtn->draw_box.x + 1, obtn->draw_box.y + 1,
                                  obtn->draw_box.w - 2, obtn->draw_box.h - 2);
  obtn->child = (PhxObject*)olbl;
  return ui_label_set(olbl, str, position);
}

#pragma mark *** Popup ***

// menu: all strings are bank, default never changes
//       first object should be a copy of button's child object
// note that sub-menus would be non-menu banks
// behaviour would be not to alter
// combo: first string default with obtn coordinates
// combo: default gets altered by bank objects
// but must maintain mete/draw boxes of 'default'
static void
ui_bank_create(PhxObjectType type, PhxObjectButton *obtn,
                              PhxResultHandler rcb, int number_strings, ...) {

  if ((type != PHX_BANK_MENU) && (type != PHX_BANK_COMBO))  return;

  PhxObjectLabel *olbl;
  char *gw;

  PhxBank *obank = malloc(sizeof(PhxBank));
  memset(obank, 0, sizeof(PhxBank));
  obtn->bank = obank;
  obank->type = type;
  obank->state = 1U << 16; // assigning room for 1 OBJS_ALLOC
  obank->actuator = obtn;
  obank->display_indicies = 0; // first object displayed
  obank->_result_cb = rcb;

  int aSz = OBJS_ALLOC * sizeof(PhxObject*);
  obank->objects = malloc(aSz);
  memset(obank->objects, 0, aSz);
  int allot = (obank->state >> 16) << OBJS_PWR;

  int total_height = 0;
  int max_len = 0;

    // create label object even if 0 strings
    // direct from obtn is used for display (in iface positioning)
    // bank to use 0,0 coorindinates
  olbl = (PhxObjectLabel*)ui_object_create(obtn->iface,
                                PHX_LABEL, draw_label,
                                obtn->draw_box.x + 1, obtn->draw_box.y + 1,
                                obtn->draw_box.w - 2, obtn->draw_box.h - 2);
  gw = ui_textview_font_set((PhxObjectTextview*)olbl, olbl->draw_box.h);
  olbl->state &= ~HJST_MSK;
  olbl->state |= HJST_LFT;

  if (number_strings <= 0) {
    olbl->string = strdup("");
    obank->objects[0] = (PhxObject*)olbl;
  } else {
    int scnt = 0;
    va_list arg;
    va_start(arg, number_strings);
      char *str = va_arg(arg, char*);
      olbl->string = strdup(str);
      do {
        char *nPtr = strchr(str, '\n');
        int x_advance = ui_text_extent_width(gw, str);
        if (x_advance > max_len)  max_len = x_advance;
        total_height += olbl->font_em;
        if (nPtr == NULL)  break;
        olbl->mete_box.y += olbl->font_em;
        olbl->draw_box.y += olbl->font_em;
        str = nPtr + 1;
      } while (1);
      obank->objects[0] = (PhxObject*)olbl;
      while ((++scnt) < number_strings) {
        if ( (obank->objects[scnt] == NULL)
            && ((scnt + 1) == allot) ) {
          obank->state += 1U << 16;
          allot = (obank->state >> 16) << OBJS_PWR;
          size_t newSz = allot * sizeof(PhxObject*);
          PhxObject *newPtr = realloc(obank->objects, newSz);
          if (newPtr == NULL) {
            puts("realloc failure: ui_bank_create");
            return;
          }
          obank->objects = (PhxObject**)newPtr;
          memset(&obank->objects[(scnt + 1)], 0,
                               (OBJS_ALLOC * sizeof(PhxObject*)));
        }
        olbl = (PhxObjectLabel*)ui_object_create((PhxInterface*)obank,
                                    PHX_LABEL, draw_label, 0, 0,
                                    obtn->draw_box.w - 2, obtn->draw_box.h - 2);
        olbl->font_em = ((PhxObjectLabel*)obank->objects[0])->font_em;
        olbl->font_name = ((PhxObjectLabel*)obank->objects[0])->font_name;
        olbl->font_size = ((PhxObjectLabel*)obank->objects[0])->font_size;
        olbl->font_origin = ((PhxObjectLabel*)obank->objects[0])->font_origin;
        olbl->state &= ~HJST_MSK;
        olbl->state |= HJST_LFT;
        char *str = va_arg(arg, char*);
        olbl->string = strdup(str);
        do {
          char *nPtr = strchr(str, '\n');
          int x_advance = ui_text_extent_width(gw, str);
          if (x_advance > max_len)  max_len = x_advance;
          total_height += olbl->font_em;
          if (nPtr == NULL)  break;
          olbl->mete_box.y += olbl->font_em;
          olbl->draw_box.y += olbl->font_em;
          str = nPtr + 1;
        } while (1);
        obank->objects[scnt] = (PhxObject*)olbl;
      }
    va_end(arg);
  }

  olbl = (PhxObjectLabel*)obank->objects[0];
    // without a label set anyway
  olbl->draw_box.x += BUTTON_TEXT_MIN;
  olbl->mete_box.w = (olbl->draw_box.w = (2 * BUTTON_TEXT_MIN) + max_len);
    // set as default,, object in its own right
  obank->display_object = malloc(sizeof(PhxObjectLabel));
  memmove(obank->display_object, olbl, sizeof(PhxObjectLabel));
  obtn->child = obank->display_object;

   // now all in bank must set to bank's mete/draw boxes
  obank->draw_box.x = (obank->mete_box.x = 0);
  obank->draw_box.y = (obank->mete_box.y = 0);
    // no arrows included
  obank->draw_box.w = (obank->mete_box.w = olbl->mete_box.w + 2);
  obank->draw_box.h = (obank->mete_box.h = total_height + 2);
  ui_box_inset(&obank->draw_box, 1, 1, 1, 1);
  obank->parent_window = NULL;
  aSz = obank->mete_box.w * obank->mete_box.h * sizeof(char);
  obank->event_map = malloc(aSz);
  memset(obank->event_map, 0, aSz);

    // now adjust all in bank, while adding to map
  total_height = 0;
  int sxdx = 0;
  int exdx = obank->mete_box.w;
  int ldx = 0;
  PhxObject *obj = obank->objects[ldx];
  do {
      // offset draw by above inset, want mete 0 for events
    obj->mete_box.x = 0;
    obj->draw_box.x = 1 + BUTTON_TEXT_MIN;
      // mete box moved by inset
    obj->mete_box.y = (obj->draw_box.y = total_height + 1);
    obj->mete_box.w = (obj->draw_box.w = obank->draw_box.w);
    int sydx = obj->draw_box.y;
    int eydx = sydx + obj->draw_box.h;
    eydx *= obank->mete_box.w;
    sydx *= obank->mete_box.w;
    for (int x = sxdx; x < exdx; x++) {
      for (int y = sydx; y < eydx; y += obank->mete_box.w)
        obank->event_map[(x + y)] = (char)ldx;
    }
    total_height += obj->draw_box.h;
  } while ((obj = obank->objects[(++ldx)]) != NULL);

/* must add to each object in bank, late add due to 'w' based om max_len */
/* could attach only to [0], on selection of others, attach then from [0] */
/* popup should ignore this */
  olbl = (PhxObjectLabel*)obtn->child;
  int end_x = olbl->draw_box.x + olbl->draw_box.w;
  if (obtn->type == PHX_BUTTON_COMBO) {
      // calc arrows start point, or transtion location
      // add on combo's signature arrows, make tight by reducing end_x
    olbl->child = ui_object_create(obtn->iface, PHX_DRAWING, draw_combo_arrows,
                                  end_x - BUTTON_TEXT_MIN,
                                  olbl->draw_box.y + 1,
                                  (67.824176 / 152.615385) * olbl->draw_box.h,
                                  olbl->draw_box.h - 2);
      // re-calculate obj (button) size based on longest line + signature arrows
    end_x += olbl->child->mete_box.w;
  }

    // base size on "old width's position less new width's position"
  end_x = (obtn->draw_box.x + obtn->draw_box.w) - end_x;

  obtn->draw_box.w -= end_x;
  obtn->mete_box.w -= end_x;

  free(gw);
}

static _Bool
ui_bank_meter(PhxBank *obank, GdkEvent *event, void *widget) {

  if (event->type == GDK_MOTION_NOTIFY) {
    obank->display_indicies &= 0x0FFFFU;
      //allows 65534 entries, -1 reserve as no selection
    int x = (int)(event->motion.x);
    int y = (int)(event->motion.y);
    if ( ((unsigned)x > (unsigned)obank->mete_box.w)
        || ((unsigned)y > (unsigned)obank->mete_box.h)) {
      obank->display_indicies |= 0xFFFF0000U;
    } else {
      int idx = 0;
      do {
        PhxObject *olbl = obank->objects[idx];
        if ((olbl->mete_box.y + olbl->mete_box.h) >= y)  break;
      } while (obank->objects[(++idx)] != NULL);
      if (obank->objects[idx] == NULL)
        obank->display_indicies |= 0xFFFF0000U;
      else
        obank->display_indicies |= (unsigned)idx << 16;
    }
    cairo_region_t *crr;
    crr = cairo_region_create_rectangle(
             (cairo_rectangle_int_t *)&obank->mete_box);
    gdk_window_invalidate_region(obank->parent_window, crr, FALSE);
    cairo_region_destroy(crr);
    return TRUE;
  }
  if (event->type == GDK_BUTTON_PRESS) {
    int x = (int)(event->button.x);
    int y = (int)(event->button.y);
    if ( ((unsigned)x > (unsigned)obank->mete_box.w)
        || ((unsigned)y > (unsigned)obank->mete_box.h)) {
      gtk_widget_destroy(widget);
    }
    return TRUE;
  }
  if (event->type == GDK_BUTTON_RELEASE) {
    int set = obank->display_indicies >> 16;
    _Bool changed = (set != -1);
    if (changed) {
      changed = (set != (int)(obank->display_indicies & 0x0FFFFU));
      if (changed) {
        char *str = ((PhxObjectLabel*)obank->objects[(unsigned)set])->string;
        ((PhxObjectLabel*)obank->display_object)->string = str;
      }
      obank->display_indicies = set;
    }
    set = obank->display_indicies & 0x0FFFFU;
    obank->display_indicies = (set << 16) | set;

      // following needed because of grab
    PhxObjectButton *obtn = obank->actuator;
      // first time release occured on actuation, state should not be 0
      // sets 0 after this test
    if ((obtn->state & 1) == 0) {
      int x = (int)(event->button.x);
      int y = (int)(event->button.y);
      if ( ((unsigned)x <= (unsigned)obank->mete_box.w)
          && ((unsigned)y <= (unsigned)obank->mete_box.h)) {
        if (changed && (obank->_result_cb != NULL))
          obank->_result_cb(obank);
        gtk_widget_destroy(widget);
      }
    }
    obtn->state &= ~7;

      // redraw findPort
    cairo_region_t *crr;
    crr = cairo_region_create_rectangle(
             (cairo_rectangle_int_t *)&obtn->iface->mete_box);
    gdk_window_invalidate_region(obtn->iface->parent_window, crr, FALSE);
    cairo_region_destroy(crr);
    return TRUE;
  }
  return FALSE;
}

static void
ui_bank_combo_run(PhxInterface *fport, GdkEvent *event, PhxObject *obj) {

  (void)event;

  PhxObjectButton *obtn = (PhxObjectButton*)obj;
  PhxBank *obank = obtn->bank;

  GtkWidget *combo_popup;
  combo_popup = gtk_window_new(GTK_WINDOW_POPUP);
  gtk_widget_realize(combo_popup);

  int w = obank->mete_box.w,
      h = obank->mete_box.h;
  gtk_window_set_default_size(GTK_WINDOW(combo_popup), w, h);

  obank->parent_window = gtk_widget_get_window(combo_popup);

  g_signal_connect_swapped(G_OBJECT(combo_popup), "draw",
                                G_CALLBACK(ui_bank_draw_event), obank);
    // NOTE: can't change cursor unless connect to top-most
  g_signal_connect_swapped(G_OBJECT(combo_popup), "button-press-event",
                                G_CALLBACK(ui_bank_meter), obank);
  g_signal_connect_swapped(G_OBJECT(combo_popup), "button-release-event",
                                G_CALLBACK(ui_bank_meter), obank);
    // connections not auto by gtk
  gtk_widget_add_events(combo_popup, GDK_POINTER_MOTION_MASK);
  g_signal_connect_swapped(G_OBJECT(combo_popup), "motion-notify-event",
                                G_CALLBACK(ui_bank_meter), obank);

    // set position based on active text
  int delta_y = obtn->draw_box.y;
  unsigned set = obank->display_indicies & 0x0FFFFU;
  PhxObjectLabel *olbl = (PhxObjectLabel*)obank->objects[set];
  delta_y -= (olbl->draw_box.y - 1);

  double wx, wy;
  GdkWindow *editor_window = gtk_widget_get_window(
                  ((LCIFindPort*)fport)->session->editor_area->editor_box);
  gdk_window_coords_to_parent(editor_window, 0, 0, &wx, &wy);

  double dx, dy;
  GdkWindow *window = gtk_widget_get_window(combo_popup);
  gdk_window_coords_to_parent(
       gtk_widget_get_window(((LCIFindPort*)fport)->session->main_window),
                                     obtn->draw_box.x, delta_y, &dx, &dy);

  gdk_window_move(window, (dx + wx), (dy + wy));
  gtk_widget_show_all(combo_popup);

    // following allows popup to behave like it has all needed events
  GdkDisplay *display = gdk_display_get_default();
  GdkSeat *seat = gdk_display_get_default_seat(display);
  gdk_seat_grab(seat, window, GDK_SEAT_CAPABILITY_ALL, FALSE,
                                                 NULL, NULL, NULL, NULL);
}

#pragma mark *** Events ***

static _Bool
uio_configure_event(PhxInterface *iface, GdkEvent *event, void *widget) {

  (void)widget;

  if (iface->_configure_event != NULL)
    iface->_configure_event(iface, event, widget);
    /* MUST return FALSE, can not update events pass orginal clip.
       Can draw correctly, but useless without handling events.
       Only thing I didn't try is connect_after_swapped, doesn't exist.
       Only a guess, but maybe has something to do with containers?
       A window forces one to connect any drawing to a container.
       User doesn't resize container, maybe during expand, events
       not expanded unless one figures odd connection. */
  return FALSE;
}

static _Bool
mouse_drag_begin(PhxInterface *iface, GdkEvent *event) {

  if ((event->button.state & GDK_CONTROL_MASK) != 0) {
    ui_interface_mouse_set(iface, DND_COPY_CURSOR);
    ui_interface_set_cursor_named(iface->parent_window, "copy");
  } else {
    ui_interface_mouse_set(iface, DND_MOVE_CURSOR);
    ui_interface_set_cursor_named(iface->parent_window, "move");
  }
  return TRUE;
}

static _Bool
mouse_drag_motion(PhxInterface *iface, GdkEvent *event) {

  PhxObject *obj;
  _Bool      in_mete = TRUE;

  int x = (int)(event->motion.x);
  int y = (int)(event->motion.y);
  if ( ((unsigned)x <= (unsigned)iface->mete_box.w)
      && ((unsigned)y <= (unsigned)iface->mete_box.h)) {
    int ldx = iface->event_map[(x + (y * iface->mete_box.w))];
    obj = iface->objects[ldx];
  } else {
// get window at pointer, querry if can accept
// for now use iface focus to handle event
    obj = iface->has_focus;
    in_mete = FALSE;
  }

  double this_event_x = event->motion.x,
         this_event_y = event->motion.y;

  if ( (fabs(this_event_x - press_event_x) > 1.0)
      || (fabs(this_event_y - press_event_y) > 1.0) ) {

    press_event_x = this_event_x, press_event_y = this_event_y;

    if ((obj->type == PHX_TEXTVIEW) || (obj->type == PHX_ENTRY)) {

      PhxObjectTextview *tv = (PhxObjectTextview*)obj;

      location temp;
      temp.x = (int)event->motion.x - tv->draw_box.x;
      temp.y = (int)event->motion.y - tv->draw_box.y;
      if (temp.x < 0)  temp.x = -1;
      if (temp.y < 0)  temp.y = -1;
      temp.x += tv->bin.x;
      temp.y += tv->bin.y;
      location_for_point(tv, &temp);
      location_auto_scroll(tv, &temp);
      if (!in_mete)  goto outside;
      if ( (temp.offset == tv->release.offset)
          || (temp.offset == tv->insert.offset) ) {

        if ( (temp.offset != tv->drop.offset)
            || (ui_interface_mouse_get(iface) == DND_COPY_CURSOR) ) {
          tv->drop.x = temp.x;
          tv->drop.y = temp.y;
          tv->drop.offset = temp.offset;
          goto redraw;
        }
      } else if ( (temp.offset > tv->release.offset)
          || (temp.offset < tv->insert.offset) ) {
        if (temp.offset != tv->drop.offset) {
          tv->drop.x = temp.x;
          tv->drop.y = temp.y;
          tv->drop.offset = temp.offset;
          goto redraw;
        }
      } else {
 outside:
        tv->drop.x = tv->interim.x;
        tv->drop.y = tv->interim.y;
        tv->drop.offset = tv->interim.offset;

        cairo_region_t *crr;
  redraw:
        crr = cairo_region_create_rectangle(
                 (cairo_rectangle_int_t *)&iface->mete_box);
        gdk_window_invalidate_region(event->motion.window, crr, FALSE);
        cairo_region_destroy(crr);
      }
    }
  }
  return TRUE;
}

static _Bool
mouse_drag_finish(PhxInterface *iface, GdkEvent *event) {

  text_buffer_drag_release((PhxObjectTextview*)iface->has_focus);

  ui_interface_mouse_set(iface, MOUSE_CLEAR);
  ui_interface_set_cursor_named(iface->parent_window, "text");

  cairo_region_t *crr;
  crr = cairo_region_create_rectangle(
           (cairo_rectangle_int_t *)&iface->mete_box);
  gdk_window_invalidate_region(event->button.window, crr, FALSE);
  cairo_region_destroy(crr);

  return TRUE;
}

static void
mouse_shift_click(PhxObjectTextview *tv, location *temp) {

  location *active;
  int font_em = tv->font_em;

    // special handle y for mouse_motion, esp. when one line view
  if (ui_interface_mouse_get(tv->iface) == MOUSE_1BUTTON_MOTION) {
    if ((temp->y - 2) <= tv->bin.y) {
      if ( (tv->bin.y != 0)
          && ((tv->bin.y - ((tv->bin.y / font_em) * font_em)) == 0) )
      temp->y -= font_em;
    } else if ((temp->y + 2) >= tv->bin.h) {
      location end;
      end.offset = (int)strlen(tv->string);
      location_for_offset(tv, &end);
        // note 'equal to' means last line not shown
      if (tv->bin.h < (end.y + font_em))
        temp->y += font_em;
    }
  }
  location_for_point(tv, temp);
  location_auto_scroll(tv, temp);

  active = &tv->release;
  if (temp->offset < tv->release.offset) {
    active = &tv->insert;
    if ( (temp->offset > tv->insert.offset)
        && (tv->release.offset == tv->interim.offset) )
      active = &tv->release;
  }
  if (active->offset != temp->offset) {
    tv->interim.x = (active->x = temp->x);
    tv->interim.y = (active->y = temp->y);
    tv->interim.offset = (active->offset = temp->offset);
  }
}

// will enter either mouse_shift_click or mouse_press_event
// use inerim as start point (last active)
static void
mouse_double_click(PhxObjectTextview *tv, int shift_click) {

  location *temp;
  int offset;
  char find_ch;
  char ch0, *cPtr, *iPtr = &tv->string[tv->interim.offset];
  ch0 = *(cPtr = iPtr);

  if (ch0 == 0)  return;

  if (isspace(ch0) != 0) {
      // moves right, at least 1 found
    find_ch = ch0;
    do  ch0 = *(++cPtr);  while (find_ch == ch0);
    offset = (unsigned)(cPtr - tv->string);
    if ( (shift_click && (offset > tv->release.offset))
       || !shift_click ) {
      tv->release.offset = offset;
      location_for_offset(tv, &tv->release);
      temp = &tv->release;
    } else if (shift_click && (offset == tv->release.offset))
      temp = &tv->release;
    if (tv->interim.offset != 0) {
      cPtr = iPtr;
      while ( ((cPtr - 1) >= tv->string)
             && (*(cPtr - 1) == find_ch) )
        --cPtr;
      offset = (unsigned)(cPtr - tv->string);
      if ( (shift_click && (offset < tv->insert.offset))
         || !shift_click ) {
        tv->insert.offset = offset;
        location_for_offset(tv, &tv->insert);
        temp = &tv->insert;
      } else if (shift_click && (offset == tv->insert.offset))
        temp = &tv->insert;
    }
    tv->interim.x = temp->x;
    tv->interim.y = temp->y;
    tv->interim.offset = temp->offset;
    location_auto_scroll(tv, temp);
    return;
  }

  if ((isalnum(ch0) != 0) || (ch0 == '_')) {
    do
      ch0 = *(++cPtr);
    while ((isalnum(ch0) != 0) || (ch0 == '_'));
    offset = (unsigned)(cPtr - tv->string);
    if ( (shift_click && (offset > tv->release.offset))
       || !shift_click ) {
      tv->release.offset = offset;
      location_for_offset(tv, &tv->release);
      temp = &tv->release;
    } else if (shift_click && (offset == tv->release.offset))
      temp = &tv->release;
    if (tv->insert.offset != 0) {
      cPtr = iPtr;
      while ((cPtr - 1) >= tv->string) {
        ch0 = *(cPtr - 1);
        if ((isalnum(ch0) == 0) && (ch0 != '_'))  break;
        --cPtr;
      }
      offset = (unsigned)(cPtr - tv->string);
      if ( (shift_click && (offset < tv->insert.offset))
         || !shift_click ) {
        tv->insert.offset = offset;
        location_for_offset(tv, &tv->insert);
        temp = &tv->insert;
      } else if (shift_click && (offset == tv->insert.offset))
        temp = &tv->insert;
    }
    tv->interim.x = temp->x;
    tv->interim.y = temp->y;
    tv->interim.offset = temp->offset;
    location_auto_scroll(tv, temp);
    return;
  }

// c_parsing, before punct, since punct based

  if (ispunct(ch0) != 0) {
      // moves right, at least 1 found
    do  ch0 = *(++cPtr);  while (ispunct(ch0) != 0);
    offset = (unsigned)(cPtr - tv->string);
    if ( (shift_click && (offset > tv->release.offset))
       || !shift_click ) {
      tv->release.offset = offset;
      location_for_offset(tv, &tv->release);
      temp = &tv->release;
    } else if (shift_click && (offset == tv->release.offset))
      temp = &tv->release;
    if (tv->interim.offset != 0) {
      cPtr = iPtr;
      while ((cPtr - 1) >= tv->string) {
        ch0 = *(cPtr - 1);
        if (ispunct(ch0) == 0)  break;
        --cPtr;
      }
      offset = (unsigned)(cPtr - tv->string);
      if ( (shift_click && (offset < tv->insert.offset))
         || !shift_click ) {
        tv->insert.offset = offset;
        location_for_offset(tv, &tv->insert);
        temp = &tv->insert;
      } else if (shift_click && (offset == tv->insert.offset))
        temp = &tv->insert;
    }
    tv->interim.x = temp->x;
    tv->interim.y = temp->y;
    tv->interim.offset = temp->offset;
    location_auto_scroll(tv, temp);
    return;
  }
}

// will enter either mouse_shift_click or mouse_press_event
// use inerim as start point (last active)
static void
mouse_triple_click(PhxObjectTextview *tv, int shift_click) {

  location *temp;

  char *tempPtr = strchr(&tv->string[tv->interim.offset], '\n');
  int offset = (int)((tempPtr + 1) - tv->string);
  if (tempPtr == NULL) offset = (int)strlen(tv->string);
  if ( (shift_click && (offset > tv->release.offset))
      || !shift_click ) {
    tv->release.offset = offset;
    location_for_offset(tv, &tv->release);
    temp = &tv->release;
  }
  if (tv->interim.offset != 0) {
    tempPtr = memrchr(tv->string, '\n', tv->interim.offset);
    tempPtr = (tempPtr == NULL) ? tv->string : (tempPtr + 1);
    offset = (int)(tempPtr - tv->string);
    if ( (shift_click && (offset < tv->insert.offset))
        || !shift_click ) {
      tv->insert.offset = offset;
      location_for_offset(tv, &tv->insert);
      temp = &tv->insert;
    }
  }
  tv->interim.x = temp->x;
  tv->interim.y = temp->y;
  tv->interim.offset = temp->offset;
}

static _Bool
mouse_press_event_txt(PhxInterface *iface, GdkEvent *event, PhxObject *obj) {

  if ((obj->type != PHX_TEXTVIEW) && (obj->type != PHX_ENTRY))
    return FALSE;

    // first 'click' only sets focus
  if (obj != iface->has_focus) {
    iface->has_focus = obj;
      // at min, draw text cursor
    goto redraw;
  }

  ui_interface_mouse_set(iface, MOUSE_CLEAR);

  PhxObjectTextview *tv = (PhxObjectTextview*)obj;

    // flatten text_buffer, if was editing
  text_buffer_reset(tv);

  int shift_click = ((event->button.state & GDK_SHIFT_MASK) != 0);
  if (event->button.type == GDK_2BUTTON_PRESS) {
    mouse_double_click(tv, shift_click);
    goto redraw;
  }
  if (event->button.type == GDK_3BUTTON_PRESS) {
    mouse_triple_click(tv, shift_click);
    goto redraw;
  }
  if (event->button.type == GDK_BUTTON_PRESS)
    ui_interface_mouse_set(iface, MOUSE_1BUTTON);

  if (shift_click) {
      // Do nothing until release event, or motion
    return TRUE;
  }

  press_event_x = event->button.x;
  press_event_y = event->button.y;

  tv->interim.x = (int)event->button.x - tv->draw_box.x;
  tv->interim.y = (int)event->button.y - tv->draw_box.y;
  if (tv->interim.x < 0)  tv->interim.x = 0;
  if (tv->interim.y < 0)  tv->interim.y = 0;
  tv->interim.x += tv->bin.x;
  tv->interim.y += tv->bin.y;

  location_for_point(tv, &tv->interim);
  location_auto_scroll(tv, &tv->interim);

    // if clicked in selection do nothing until motion or release
  if ( (tv->insert.offset == tv->release.offset)
       || ( ((tv->interim.offset < tv->insert.offset)
          && (tv->interim.offset < tv->release.offset))
        || ((tv->interim.offset > tv->insert.offset)
            && (tv->interim.offset > tv->release.offset)) ) ) {
    tv->insert.x = (tv->release.x = tv->interim.x);
    tv->insert.y = (tv->release.y = tv->interim.y);
    tv->insert.offset = (tv->release.offset = tv->interim.offset);
  }
    // update even if no size change, prevents black pixel(s) line at bottom
  cairo_region_t *crr;
redraw:
  crr = cairo_region_create_rectangle(
           (cairo_rectangle_int_t *)&iface->mete_box);
  gdk_window_invalidate_region(event->button.window, crr, FALSE);
  cairo_region_destroy(crr);
  return TRUE;
}

static _Bool
mouse_release_event_txt(PhxInterface *iface, GdkEvent *event, PhxObject *obj) {

  int x = (int)(event->button.x);
  int y = (int)(event->button.y);

  if (ui_interface_mouse_get(iface) & MOUSE_1BUTTON) {

    PhxObjectTextview *tv = (PhxObjectTextview*)iface->has_focus;

    if ((event->button.state & GDK_SHIFT_MASK) != 0) {

      location temp;
      temp.x = x - tv->draw_box.x;
      temp.y = y - tv->draw_box.y;
      if (temp.x < 0)  temp.x = 0;
      if (temp.y < 0)  temp.y = 0;
      temp.x += tv->bin.x;
      temp.y += tv->bin.y;

      mouse_shift_click(tv, &temp);
      goto redraw;
    }
      // use only if !selection (pressed-released, no motion)
    if ((ui_interface_mouse_get(iface) & MOUSE_MOTION) == 0) {
      tv->insert.x = (tv->release.x = tv->interim.x);
      tv->insert.y = (tv->release.y = tv->interim.y);
      tv->insert.offset = (tv->release.offset = tv->interim.offset);
    } else {
        // motion-released, set cursor based on release point
      GdkWindow *window = iface->parent_window;
      GdkCursor *cursor = gdk_window_get_cursor(window);
      if ( (window == event->button.window)
          && ((obj->type == PHX_TEXTVIEW) || (obj->type == PHX_ENTRY)) ) {
        if (cursor == NULL) {
          GdkDisplay *display = gdk_window_get_display(window);
          cursor = gdk_cursor_new_from_name(display, "text");
          gdk_window_set_cursor(window, cursor);
        }
      } else {
        if (cursor != NULL) {
          gdk_window_set_cursor(window, NULL);
          g_object_unref((void*)cursor);
        }
      }
    }
  }
redraw:
  ui_interface_mouse_set(iface, MOUSE_CLEAR);
  cairo_region_t *crr;
  crr = cairo_region_create_rectangle(
           (cairo_rectangle_int_t *)&iface->mete_box);
  gdk_window_invalidate_region(event->button.window, crr, FALSE);
  cairo_region_destroy(crr);
  return TRUE;
}

static _Bool
mouse_motion_event_txt(PhxInterface *iface, GdkEvent *event, PhxObject *obj) {

  if (ui_interface_mouse_get(iface) == MOUSE_CLEAR)  return FALSE;

  double this_event_x = event->button.x,
         this_event_y = event->button.y;

  if ( (fabs(this_event_x - press_event_x) > 1.0)
      || (fabs(this_event_y - press_event_y) > 1.0) ) {

    press_event_x = this_event_x, press_event_y = this_event_y;

    PhxObjectTextview *tv = (PhxObjectTextview*)iface->has_focus;

// if clicked in a selection, and was first MOUSE_1BUTTON_MOTION
// then start drag
    if (ui_interface_mouse_get(iface) == MOUSE_1BUTTON) {
      if (tv->insert.offset != tv->release.offset) {
        if ( ((tv->interim.offset > tv->insert.offset)
              && (tv->interim.offset < tv->release.offset))
            || ((tv->interim.offset < tv->insert.offset)
                && (tv->interim.offset > tv->release.offset)) ) {
          tv->drop.x = tv->interim.x;
          tv->drop.y = tv->interim.y;
          tv->drop.offset = tv->interim.offset;
          return mouse_drag_begin(iface, event);
        } else {
            // clear selection
          tv->insert.x = (tv->release.x = tv->interim.x);
          tv->insert.y = (tv->release.y = tv->interim.y);
          tv->insert.offset = (tv->release.offset = tv->interim.offset);
        }
      }
    }

    location temp;
    temp.x = (int)this_event_x - tv->draw_box.x;
    temp.y = (int)this_event_y - tv->draw_box.y;
    if (temp.x < 0)  temp.x = 0;
    if (temp.y < 0)  temp.y = 0;
    temp.x += tv->bin.x;
    temp.y += tv->bin.y;

    ui_interface_mouse_set(iface, MOUSE_CLEAR);
      // force no use of mouse_release on shift clicks
    if ((event->button.state & GDK_SHIFT_MASK) == 0)
      ui_interface_mouse_set(iface, MOUSE_1BUTTON_MOTION);

    mouse_shift_click(tv, &temp);

    cairo_region_t *crr;
    crr = cairo_region_create_rectangle(
             (cairo_rectangle_int_t *)&iface->mete_box);
    gdk_window_invalidate_region(event->button.window, crr, FALSE);
    cairo_region_destroy(crr);
  }
  return TRUE;
}

// when vertical scrolling, at least 1 line must display at end
// base on 1/10 of bin height, ceiling of line heights
// bin of 300 => 20l@15p would do (300/10 + 14) / 15 => 2 lines
// bin of 750 => 50l              (75+14)/15 => 5
static _Bool
mouse_scroll_event(PhxInterface *iface, GdkEvent *event) {

  PhxObject *obj = iface->has_focus;
  if ( (obj == NULL)
      || ((obj->type != PHX_TEXTVIEW) && (obj->type != PHX_ENTRY)) ) {
    return TRUE;
  }
  PhxObjectTextview *otxt = (PhxObjectTextview*)obj;

  int y, font_em = otxt->font_em;

  y = (otxt->bin.h - otxt->bin.y) / 10;
  if (y <= font_em)
    y = font_em;
  else
    y = ((y + (font_em - 1)) / font_em) * font_em;

  if (event->scroll.direction == GDK_SCROLL_UP)
    y = ((otxt->bin.y / font_em) * font_em) - y;
  else
    y += ((otxt->bin.h - font_em) / font_em) * font_em;

  location scroll = { otxt->bin.x, y, 0 };
  location_for_point(otxt, &scroll);
  location_auto_scroll(otxt, &scroll);

  cairo_region_t *crr;
  crr = cairo_region_create_rectangle(
           (cairo_rectangle_int_t *)&iface->mete_box);
  gdk_window_invalidate_region(event->scroll.window, crr, FALSE);
  cairo_region_destroy(crr);
  return TRUE;
}

static int
key_movement_page_scroll(PhxObjectTextview *tv, short dirdis) {

  int font_em = tv->font_em;
  int scroll_y = tv->bin.y;
  if (dirdis < 0) {  // GDK_KEY_Page_Up
    if (tv->bin.y != 0) {
        // check for case of less than two line viewing
      if (tv->draw_box.h < (font_em << 1)) {
        tv->bin.y -= font_em;
      } else {
          // move partial exposed below view
        int delta = tv->bin.y - ((tv->bin.y / font_em) * font_em);
        if (delta != 0)  tv->bin.y -= delta - font_em;
          // move full viewed top line to bottom of viewed page
        tv->bin.y -= tv->draw_box.h - font_em;
        tv->bin.h = tv->bin.y + tv->draw_box.h;

        delta = tv->bin.h - ((tv->bin.h / font_em) * font_em);
        if (delta != 0) {
printf("key_movement_page_scroll %d\n", delta);
          tv->bin.y += font_em;
        }
      }
      if (tv->bin.y < 0)  tv->bin.y = 0;
    }
  } else {           // GDK_KEY_Page_Down
    location temp;
    temp.offset = (int)strlen(tv->string);
    location_for_offset(tv, &temp);
    if ((tv->bin.y + tv->draw_box.h - font_em) <= temp.y) {
      if (tv->draw_box.h < (font_em << 1))
            tv->bin.y += font_em;
      else  tv->bin.y = ((tv->bin.h / font_em) * font_em) - font_em;
    }
  }
  tv->bin.h = tv->bin.y + tv->draw_box.h;
  return (tv->bin.y - scroll_y);
}

static int
key_movement_control(PhxObjectTextview *tv, location *active,
                                     location *inactive, short event_flag) {
  int font_em = tv->font_em;
  int offset, shift_click = event_flag & GDK_SHIFT_MASK;
  short dirdis = event_flag >> 4;
  char ch0, *cPtr = &tv->string[active->offset];
  ch0 = *cPtr;
    // horizontal movement to begin/end of previous/next identifiers
  if ((dirdis & 3) == 0) {
    if (dirdis < 0) {  // GDK_KEY_Left
      if ((cPtr - 1) < tv->string)  return TRUE;
      ch0 = *(--cPtr);
      if ((isalnum(ch0) == 0) && (ch0 != '_'))
        do {
          if ((cPtr - 1) < tv->string)  goto mcl;
          ch0 = *(cPtr - 1);
          if ((isalnum(ch0) != 0) || (ch0 == '_'))  break;
          --cPtr;
        } while (1);
      do {
        if ((cPtr - 1) < tv->string)  break;
        ch0 = *(cPtr - 1);
        if ((isalnum(ch0) == 0) && (ch0 != '_'))  break;
        --cPtr;
      } while (1);
mcl:  offset = (int)(cPtr - tv->string);
      if ( (shift_click && (offset < active->offset))
          || !shift_click )
        goto mc0;
    } else {           // GDK_KEY_Right
      if (ch0 == 0)  return TRUE;
      if ((isalnum(ch0) == 0) && (ch0 != '_'))
        do {
          if ((ch0 = *(++cPtr)) == 0)  goto mcr;
        } while ((isalnum(ch0) == 0) && (ch0 != '_'));
      do {
        if ((ch0 = *(++cPtr)) == 0)  break;
      } while ((isalnum(ch0) != 0) || (ch0 == '_'));
mcr:  offset = (int)(cPtr - tv->string);
      if ( (shift_click && (offset > active->offset))
          || !shift_click ) {
  mc0:  active->offset = offset;
        location_for_offset(tv, active);
        location_auto_scroll(tv, active);
      }
    }
    goto kmc;
  }

  if ((dirdis & 3) == 2) {
    char *tempPtr;
    if (dirdis < 0) {  // GDK_KEY_Home
      if (active->offset != 0) {
        tempPtr = memrchr(tv->string, '\n', active->offset);
        tempPtr = (tempPtr == NULL) ? tv->string : (tempPtr + 1);
        if ((ch0 = *tempPtr) != 0)
          while (isblank(ch0))  ch0 = *(++tempPtr);
        active->offset = (int)(tempPtr - tv->string);
          // sum of glyph advances
        location_for_offset(tv, active);
          // auto-scrolls 'tv->bin.x' amount (reduced for begin only)
        tv->bin.x = 0;
        tv->bin.w = tv->draw_box.w;
      }
    } else {           // GDK_KEY_End
      tempPtr = strchr(cPtr, '\n');
      if (tempPtr == NULL) tempPtr = cPtr + (int)strlen(cPtr);
      while ((tempPtr - 1) >= cPtr) {
        ch0 = *(tempPtr - 1);
        if (!isblank(ch0))  break;
        tempPtr--;
      }
      active->offset = (int)(tempPtr - tv->string);
        // sum of glyph advances
      location_for_offset(tv, active);
        // x auto-scroll (reduced for end only)
      int x = active->x + tv->bin.x;
      if (x >= (tv->bin.w - font_em)) {
        int delta = x - (tv->bin.w - font_em);
        tv->bin.x += delta;
        tv->bin.w += delta;
      }
    }
    goto kmc;
  }

  if ((dirdis & 3) == 3) {
    if (dirdis < 0) {  // GDK_KEY_Page_Up
      active->x = (active->y = (active->offset = 0));
      tv->bin.x = (tv->bin.y = 0);
      tv->bin.w = tv->draw_box.w;
      tv->bin.h = tv->draw_box.h;
    } else {           // GDK_KEY_Page_Down
      active->offset = (int)strlen(tv->string);
      location_for_offset(tv, active);
      location_auto_scroll(tv, active);
    }
kmc:
    if (shift_click && (inactive->offset != active->offset))  {
      tv->interim.x = active->x;
      tv->interim.y = active->y;
      tv->interim.offset = active->offset;
    } else {
      tv->interim.x = (inactive->x = active->x);
      tv->interim.y = (inactive->y = active->y);
      tv->interim.offset = (inactive->offset = active->offset);
    }
    return TRUE;
  }
  return FALSE;
}

static void
key_movement_press(PhxObjectTextview *tv, short event_flag) {

  location *active, *inactive;

    // flatten text_buffer, if was editing
  text_buffer_reset(tv);

  int font_em = tv->font_em;

  short dirdis = event_flag >> 4;
    // deselection of selection
  if ( ((event_flag & 0x000F) == 0)
      && (tv->insert.offset != tv->release.offset) ) {
    if (dirdis < 0)
          active = &tv->insert, inactive = &tv->release;
    else  active = &tv->release, inactive = &tv->insert;
    tv->interim.x = (inactive->x = active->x);
    tv->interim.y = (inactive->y = active->y);
    tv->interim.offset = (inactive->offset = active->offset);
    location_auto_scroll(tv, active);
    return;
  }

  active = &tv->release, inactive = &tv->insert;
  if (tv->interim.offset == tv->insert.offset) {
    active = &tv->insert, inactive = &tv->release;
    if ( (tv->interim.x == tv->insert.x)
        && (tv->interim.y == tv->insert.y) ) {
NWSE:   if (dirdis < 0)
            active = &tv->insert, inactive = &tv->release;
      else  active = &tv->release, inactive = &tv->insert;
      tv->interim.x = active->x;
      tv->interim.y = active->y;
      tv->interim.offset = active->offset;
    }
  } else if ( (tv->interim.x == tv->release.x)
             && (tv->interim.y == tv->release.y) ) {
    goto NWSE;
  }

  if (event_flag & GDK_CONTROL_MASK) {
    if (key_movement_control(tv, active, inactive, event_flag) == TRUE)
      return;
  }

  if ((dirdis & 3) == 0) {
    if (dirdis < 0) {  // GDK_KEY_Left
      if (active->offset != 0)              active->offset--;
    } else {           // GDK_KEY_Right
      if (tv->string[active->offset] != 0)  active->offset++;
    }
    location_for_offset(tv, active);
    location_auto_scroll(tv, active);
    goto kmp_h;
  }

  if ((dirdis & 3) == 1) {
    if (dirdis < 0) {  // GDK_KEY_Up
      if (active->y == 0)  goto kmp_v;
       active->y -= font_em;
    } else {           // GDK_KEY_Down
      active->y += font_em;
    }
    goto kmp_v;
  }

  if ((dirdis & 3) == 2) {
    char *tempPtr;
    if (dirdis < 0) {  // GDK_KEY_Home
      if (active->offset != 0) {
        tempPtr = memrchr(tv->string, '\n', active->offset);
        tempPtr = (tempPtr == NULL) ? tv->string : (tempPtr + 1);
        active->offset = (int)(tempPtr - tv->string);
          // auto-scrolls 'tv->bin.x' amount (reduced for begin only)
        active->x = 0;
        tv->bin.x = 0;
        tv->bin.w = tv->draw_box.w;
      }
    } else {           // GDK_KEY_End
      tempPtr = strchr(&tv->string[active->offset], '\n');
      active->offset = (int)(tempPtr - tv->string);
      if (tempPtr == NULL) active->offset = (int)strlen(tv->string);
        // sum of glyph advances
      location_for_offset(tv, active);
        // x auto-scroll (reduced for end only)
      int x = active->x + tv->bin.x;
      if (x >= (tv->bin.w - font_em)) {
        int delta = x - (tv->bin.w - font_em);
        tv->bin.x += delta;
        tv->bin.w += delta;
      }
    }
kmp_h:
    if ( ((event_flag & GDK_SHIFT_MASK) != 0)
        && (inactive->offset != active->offset) )  {
      tv->interim.x = active->x;
      tv->interim.y = active->y;
      tv->interim.offset = active->offset;
    } else {
      tv->interim.x = (inactive->x = active->x);
      tv->interim.y = (inactive->y = active->y);
      tv->interim.offset = (inactive->offset = active->offset);
    }
    return;
  }
  if ((dirdis & 3) == 3) {
      // move bin, adjust y by amount moved
    active->y += key_movement_page_scroll(tv, dirdis);
kmp_v:
    active->x = tv->interim.x;
    location_for_point(tv, active);
    location_auto_scroll(tv, active);
    if ( ((event_flag & GDK_SHIFT_MASK) != 0)
        && (inactive->offset != active->offset) )  {
      tv->interim.y = active->y;
      tv->interim.offset = active->offset;
    } else {
      inactive->x = active->x;
      tv->interim.y = (inactive->y = active->y);
      tv->interim.offset = (inactive->offset = active->offset);
    }
    return;
  }
}

static gboolean
key_press_event(PhxInterface *iface, GdkEvent *event, PhxObject *obj) {

  short state = event->key.state
                 & (GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK);
  int ch = event->key.keyval;

  if ( (obj->type != PHX_TEXTVIEW) && (obj->type != PHX_ENTRY) ) {
    return FALSE;
  }

  PhxObjectTextview *tv = (PhxObjectTextview*)obj;

  if ( (ch >= GDK_KEY_space) && (ch <= GDK_KEY_asciitilde) ) {
    if (state & GDK_CONTROL_MASK) {
      if (ch == 'a') {  text_buffer_select_all(tv);  return TRUE;  }
      if (ch == 'c') {  text_buffer_copy(tv);  return TRUE;  }
      if (ch == 'v') {  text_buffer_paste(tv); goto redraw;  }
      if (ch == 'x') {  text_buffer_cut(tv);   goto redraw;  }
      return FALSE;
    }
    text_buffer_insert(tv, (char*)&ch, 1);
    goto redraw;
  }

  switch (event->key.keyval) {

    case GDK_KEY_BackSpace:                   // 0xff08
      text_buffer_delete(tv);
      goto redraw;

    case GDK_KEY_Tab:                         // 0xff09
      text_buffer_insert(tv, (char*)&ch, 1);
      goto redraw;

    case GDK_KEY_Return:                      // 0xff0d
    case GDK_KEY_KP_Enter:                    // 0xff8d
      ch = '\n';
      text_buffer_insert(tv, (char*)&ch, 1);
      goto redraw;

    case GDK_KEY_Home:                        // 0xff50
    case GDK_KEY_KP_Home:                     // 0xff95
      key_movement_press(tv, (state | (short)0x8020));
      goto redraw;

    case GDK_KEY_Left:                        // 0xff51
    case GDK_KEY_KP_Left:                     // 0xff96
      key_movement_press(tv, (state | (short)0x8000));
      goto redraw;

    case GDK_KEY_Up:                          // 0xff52
    case GDK_KEY_KP_Up:                       // 0xff97
      key_movement_press(tv, (state | (short)0x8010));
      goto redraw;

    case GDK_KEY_Right:                       // 0xff53
    case GDK_KEY_KP_Right:                    // 0xff98
      key_movement_press(tv, (state | (short)0x0000));
      goto redraw;

    case GDK_KEY_Down:                        // 0xff54
    case GDK_KEY_KP_Down:                     // 0xff99
      key_movement_press(tv, (state | (short)0x0010));
      goto redraw;

    case GDK_KEY_Page_Up:                     // 0xff55
    case GDK_KEY_KP_Page_Up:                  // 0xff9a
    //case GDK_KEY_KP_Prior:                  // 0xff9a
      key_movement_press(tv, (state | (short)0x8030));
      goto redraw;

    case GDK_KEY_Page_Down:                   // 0xff56
    case GDK_KEY_KP_Page_Down:                // 0xff9b
    //case GDK_KEY_KP_Next:                   // 0xff9b
      key_movement_press(tv, (state | (short)0x0030));
      goto redraw;

    case GDK_KEY_End:                         // 0xff57
    case GDK_KEY_KP_End:                      // 0xff9c
      key_movement_press(tv, (state | (short)0x0020));
      goto redraw;

    case GDK_KEY_KP_Begin:                    // 0xff9d
      return FALSE;

    case GDK_KEY_Insert:                      // 0xff63
    case GDK_KEY_KP_Insert:                   // 0xff9e
      tv->state ^= (0x00000001 << (SHIFT + 10));
      goto redraw;

    case GDK_KEY_Menu:                        // 0xff67
    case GDK_KEY_Num_Lock:                    // 0xff7f
      return FALSE;

    case GDK_KEY_KP_Multiply:                 // 0xffaa * 2a
    case GDK_KEY_KP_Add:                      // 0xffab + 2b
    case GDK_KEY_KP_Separator:                // 0xffac , 2c
    case GDK_KEY_KP_Subtract:                 // 0xffad - 2d
    case GDK_KEY_KP_Decimal:                  // 0xffae . 2e
    case GDK_KEY_KP_Divide:                   // 0xffaf / 2f
    case GDK_KEY_KP_0:                        // 0xffb0
    case GDK_KEY_KP_1:                        // 0xffb1
    case GDK_KEY_KP_2:                        // 0xffb2
    case GDK_KEY_KP_3:                        // 0xffb3
    case GDK_KEY_KP_4:                        // 0xffb4
    case GDK_KEY_KP_5:                        // 0xffb5
    case GDK_KEY_KP_6:                        // 0xffb6
    case GDK_KEY_KP_7:                        // 0xffb7
    case GDK_KEY_KP_8:                        // 0xffb8
    case GDK_KEY_KP_9:                        // 0xffb9
      ch &= 0x3F;
      text_buffer_insert(tv, (char*)&ch, 1);
      goto redraw;

    case GDK_KEY_Shift_L:                     // 0xffe1
    case GDK_KEY_Shift_R:                     // 0xffe2
    case GDK_KEY_Control_L:                   // 0xffe3
    case GDK_KEY_Control_R:                   // 0xffe4
    case GDK_KEY_Caps_Lock:                   // 0xffe5
    case GDK_KEY_Shift_Lock:                  // 0xffe6
    case GDK_KEY_Alt_L:                       // 0xffe9
    case GDK_KEY_Alt_R:                       // 0xffea
    case GDK_KEY_Super_L:                     // 0xffeb
      return FALSE;

    case GDK_KEY_Delete:                      // 0xffff
    case GDK_KEY_KP_Delete:                   // 0xff9f
      tv->state ^= (0x00000001 << (SHIFT + 11));
      text_buffer_delete(tv);
      goto redraw;

    default:
printf("unhandled key (0x%X)\n", event->key.keyval);
  }
  return FALSE;

  cairo_region_t *crr;
redraw:
  crr = cairo_region_create_rectangle(
           (cairo_rectangle_int_t *)&iface->mete_box);
  gdk_window_invalidate_region(iface->parent_window, crr, FALSE);
  cairo_region_destroy(crr);
  return TRUE;
}

/*
  GDK_MOTION_NOTIFY  = 3,
  GDK_BUTTON_PRESS   = 4,
  GDK_2BUTTON_PRESS  = 5,
  GDK_DOUBLE_BUTTON_PRESS = GDK_2BUTTON_PRESS,
  GDK_3BUTTON_PRESS  = 6,
  GDK_TRIPLE_BUTTON_PRESS = GDK_3BUTTON_PRESS,
  GDK_BUTTON_RELEASE = 7,
  GDK_KEY_PRESS      = 8,
  GDK_KEY_RELEASE    = 9,
  GDK_ENTER_NOTIFY   = 10,
  GDK_LEAVE_NOTIFY   = 11,
  GDK_FOCUS_CHANGE   = 12,
// don't recieve this, must be top-most window
  GDK_CONFIGURE      = 13,
  GDK_SCROLL         = 31,
*/
static int
event_meter(PhxInterface *iface, GdkEvent *event, void *widget) {

  unsigned type = (unsigned)(event->type - GDK_MOTION_NOTIFY);
  if (type > (unsigned)(GDK_FOCUS_CHANGE - GDK_MOTION_NOTIFY)) {

    if (event->type == GDK_SCROLL)
      return mouse_scroll_event(iface, event);
    return FALSE;
  }
  if ( (event->type >= GDK_MOTION_NOTIFY)
       && (event->type <= GDK_BUTTON_RELEASE) ) {

    unsigned button;
    if (gdk_event_get_button(event, &button)) {
      if (button != 1)  return TRUE;
    }

    if ((ui_interface_mouse_get(iface) & MOUSE_DND) == MOUSE_DND) {
      if (event->type == GDK_MOTION_NOTIFY)
        return mouse_drag_motion(iface, event);
      if (event->type == GDK_BUTTON_RELEASE)
        return mouse_drag_finish(iface, event);
    }

    int x = (int)(event->button.x);
    int y = (int)(event->button.y);
    int ldx = 0;
    if ( ((unsigned)x <= (unsigned)iface->mete_box.w)
        && ((unsigned)y <= (unsigned)iface->mete_box.h))
      ldx = iface->event_map[(x + (y * iface->mete_box.w))];
    PhxObject *obj = iface->objects[ldx];

    if ((event->type >= GDK_BUTTON_PRESS)
        && (event->type < GDK_BUTTON_RELEASE)) {

        // update textviews on leaving for another object
      if ((iface->has_focus != NULL) && (obj != iface->has_focus)) {
        PhxObject *fobj = iface->has_focus;
        if ((fobj->type == PHX_TEXTVIEW) || (fobj->type == PHX_ENTRY))
          text_buffer_reset((PhxObjectTextview*)fobj);
      }

      if (obj->type == PHX_LABEL)  return TRUE;
      if (ldx == 0)  return TRUE;
      if (!visible_get(obj))  return TRUE;
      if ((obj->type != PHX_TEXTVIEW) && (obj->type != PHX_ENTRY)) {
          // obj's 'activate on click'
        if (!sensitive_get(obj))  return TRUE;
        obj->state |= 1;
        iface->has_focus = obj;
          // queue_redraw
        cairo_region_t *crr;
        crr = cairo_region_create_rectangle(
                 (cairo_rectangle_int_t *)&obj->draw_box);
        gdk_window_invalidate_region(event->button.window, crr, FALSE);
        cairo_region_destroy(crr);
          // perform action
        if (((PhxObjectButton*)obj)->_event_cb != NULL) {
          ((PhxObjectButton*)obj)->_event_cb(iface, event, obj);
          return TRUE;
        }
        return FALSE;
      }
        // perform action
      return mouse_press_event_txt(iface, event, obj);

    } else if (event->type == GDK_BUTTON_RELEASE) {

      PhxObject *new_obj = (iface->has_focus == NULL) ? obj : iface->has_focus;
      if ((new_obj->type != PHX_TEXTVIEW) && (new_obj->type != PHX_ENTRY)) {
          // obj's 'activate on click release'
        new_obj->state &= ~7;
          // queue_redraw
        cairo_region_t *crr;
        crr = cairo_region_create_rectangle(
                 (cairo_rectangle_int_t *)&new_obj->draw_box);
        gdk_window_invalidate_region(event->button.window, crr, FALSE);
        cairo_region_destroy(crr);
        if (((PhxObjectButton*)obj)->_event_cb != NULL) {
          ((PhxObjectButton*)obj)->_event_cb(iface, event, obj);
          return TRUE;
        }
        return FALSE;
      }
      return mouse_release_event_txt(iface, event, obj);

    } else if (event->type == GDK_MOTION_NOTIFY) {

      PhxObject *new_obj = (iface->has_focus == NULL) ? obj : iface->has_focus;
      if ((new_obj->type == PHX_TEXTVIEW) || (new_obj->type == PHX_ENTRY)) {
        GdkModifierType state;
        if (gdk_event_get_state(event, &state)
                  && (state & GDK_BUTTON1_MASK)) {
          return mouse_motion_event_txt(iface, event, new_obj);
        }
      }
      char *named = NULL;
      if ((obj->type == PHX_TEXTVIEW) || (obj->type == PHX_ENTRY))
        named = "text";
      ui_interface_set_cursor_named(iface->parent_window, named);
    }
    return TRUE;

  } else {

    if (event->type == GDK_FOCUS_CHANGE) {
        // leaving iface, update any textbuffers
        // others that gain focus might require use of buffers
        // in the case of findport,
        // combo popup will focus out on ifaces text types
      if (!event->focus_change.in) {
        int ldx = 0;
        do {
          PhxObject *obj = iface->objects[ldx];
          if ( ((obj->type == PHX_TEXTVIEW) || (obj->type == PHX_ENTRY))
              && (visible_get(obj)) ) {
            text_buffer_reset((PhxObjectTextview*)obj);
            if (obj->child != NULL) {
                // walk through any children
              PhxObject *add = obj->child;
              do {
                if (!(visible_get(add)))  break;
                if ((add->type == PHX_TEXTVIEW) || (add->type == PHX_ENTRY)) {
                  text_buffer_reset((PhxObjectTextview*)add);
                }
                add = add->child;
              } while (add != NULL);
            }
          }
        } while (iface->objects[(++ldx)] != NULL);
      }
      PhxObject *fobj = iface->has_focus;
      if ( (fobj != NULL)
          && ((fobj->type == PHX_TEXTVIEW) || (fobj->type == PHX_ENTRY)) )
        sensitive_set(fobj, event->focus_change.in);
          //queue_redraw
      cairo_region_t *crr;
      crr = cairo_region_create_rectangle(
               (cairo_rectangle_int_t *)&iface->mete_box);
      gdk_window_invalidate_region(event->button.window, crr, FALSE);
      cairo_region_destroy(crr);
      return TRUE;
    }
    if (event->type == GDK_ENTER_NOTIFY) {
      gtk_widget_grab_focus(widget);
      GdkWindow *window = event->crossing.window;
      GdkDisplay *display = gdk_window_get_display(window);
      GdkCursor *cursor = gdk_cursor_new_from_name(display, "text");
      gdk_window_set_cursor(window, cursor);
      return TRUE;
    }
    if (event->type == GDK_LEAVE_NOTIFY) {
      if (event->crossing.window == iface->parent_window)
        return TRUE;

      GdkWindow *window = event->crossing.window;
      GdkCursor *cursor = gdk_window_get_cursor(window);
      if (cursor != NULL) {
        gdk_window_set_cursor(window, NULL);
        g_object_unref((void*)cursor);
      }
      return TRUE;
    }
    if (event->type == GDK_KEY_PRESS) {

      BState cstate = ui_interface_mouse_get(iface);
      if ((cstate & MOUSE_DND) == MOUSE_DND) {
        if ( (event->key.keyval == GDK_KEY_Control_L)
            || (event->key.keyval == GDK_KEY_Control_R) )
          if (cstate != DND_COPY_CURSOR) {
            // need to verify is textview
            ui_interface_mouse_set(iface, DND_COPY_CURSOR);
            ui_interface_set_cursor_named(iface->parent_window, "copy");
            return TRUE;
          }
      }
      if (iface->has_focus == NULL)  return FALSE;
      return key_press_event(iface, event, iface->has_focus);
    }
    if (event->type == GDK_KEY_RELEASE) {

      BState cstate = ui_interface_mouse_get(iface);
      if (cstate == DND_COPY_CURSOR) {
        if ( (event->key.keyval == GDK_KEY_Control_L)
            || (event->key.keyval == GDK_KEY_Control_R) ) {
            // need to verify is textview
          ui_interface_mouse_set(iface, DND_MOVE_CURSOR);
          ui_interface_set_cursor_named(iface->parent_window, "move");
          return TRUE;
        }
      }
      return FALSE;
    }
  }
  return FALSE;
}

#pragma mark *** FindPort ***

static void
text_buffer_apply_search_tag(LCIFindPort *fport, struct _fsearch *searches) {

  LCIEditorArea *ea = fport->session->editor_area;
  LCITextPort *active = ea->editor_view[0];
  GtkTextIter insert, release;
  GtkTextBuffer *tbuf = GTK_TEXT_BUFFER(active->textbuffer);

  gtk_text_buffer_get_selection_bounds(tbuf, &insert, &release);
  if (searches->search_string != NULL) {
    int majdx = searches->qdx >> 5;
    int mnrdx = searches->qdx & ~(-1 << 5);
    int offset = (searches->q_results + majdx)->qoffsets[mnrdx];
    gtk_text_buffer_get_iter_at_offset(tbuf, &insert, offset);
    PhxObjectTextview *key_object
           = (PhxObjectTextview*)fport->objects[textview_find_box];
    offset += key_object->str_nil;
    gtk_text_buffer_get_iter_at_offset(tbuf, &release, offset);
    gtk_text_buffer_select_range(tbuf, (const GtkTextIter*)&insert,
                                       (const GtkTextIter*)&release);
  }
  gtk_text_view_scroll_to_iter(
              GTK_TEXT_VIEW(active->textview), &insert, 0.0, TRUE, 0.5, 0.5);
}

static void
result_cb(PhxBank *obank) {

  PhxObjectButton *obtn = (PhxObjectButton*)obank->actuator;
  LCIEditorArea *ea = ((LCIFindPort*)obtn->iface)->session->editor_area;
  LCIFindPort *findPort = ea->findport;
    // base on if 'Find & Replace' is selected
  _Bool set = (obank->display_indicies & 0x0FFFFU) == 1;

  visible_set(findPort->objects[close0_box], !set);
  visible_set(findPort->objects[replace_all_box], set);
  visible_set(findPort->objects[replace_box], set);
  visible_set(findPort->objects[replace_find_box], set);
  visible_set(findPort->objects[close1_box], set);
  visible_set(findPort->objects[textview_replace_box], set);

    // set viewable size, one or two rows
  int min_width = (int)((double)BOX_HEIGHT / .0406);
  int idx = (BOX_HEIGHT > 20);
  int two_row_height = (BOX_HEIGHT * 2) + window_adjustments[idx][1];
  GtkWidget *find_window = ea->find_window;
  if (set) {
    if (findPort->mete_box.h != two_row_height)
      gtk_widget_set_size_request(find_window, min_width, two_row_height);
  } else {
    if (findPort->mete_box.h == two_row_height)
      gtk_widget_set_size_request(find_window, min_width, BOX_HEIGHT);
  }
}

// with demo passes textview, LCode uses gtk, pass GtkTextBuffer *tbuf instead
static int
findport_display_results(LCIFindPort *fport, struct _fsearch *fdata) {

  int found = 0;
  if (fdata != NULL) {
    int idx = 0;
    do {
      unsigned bits = (fdata->q_results + idx)->qbits;
      bits = bits - ((bits >> 1) & 0x55555555);
      bits = (bits & 0x33333333) + ((bits >> 2) & 0x33333333);
      bits = (bits + (bits >> 4)) & 0x0F0F0F0F;
      bits *= 0x01010101;
      found += bits >> 24;
    } while ((++idx) <= fdata->n_qr);
  }

    // could sprint direct
  char rbuf[32];
  sprintf(rbuf, "%d found matches", found);
  PhxObjectTextview *olbl = (PhxObjectTextview*)fport->objects[found_box];
  ui_textview_buffer_set(olbl, rbuf);

  _Bool set = found > 1;
  if (found == 1) {
    struct _results *results = &fdata->q_results[(fdata->qdx >> 5)];
    unsigned bits = results->qbits;
    int mnrdx = fdata->qdx & ~(-1 << 5);
    set = !(bits & (0x00000001U << mnrdx));
    if (!set) {
      LCIEditorArea *ea = fport->session->editor_area;
      LCITextPort *active = ea->editor_view[0];
      GtkTextIter start;
      GtkTextBuffer *tbuf = GTK_TEXT_BUFFER(active->textbuffer);
      gtk_text_buffer_get_selection_bounds(tbuf, &start, NULL);
      int ins = gtk_text_iter_get_offset((const GtkTextIter*)&start);
      set = (ins == results->qoffsets[mnrdx]);
    }
  }
  sensitive_set(fport->objects[navigate_right_box], set);
  sensitive_set(fport->objects[navigate_left_box], set);

  cairo_region_t *crr;
  crr = cairo_region_create_rectangle(
           (cairo_rectangle_int_t *)&fport->mete_box);
  gdk_window_invalidate_region(fport->parent_window, crr, FALSE);
  cairo_region_destroy(crr);
  return found;
}

static struct _fsearch *
findport_results_for(LCIFindPort *fport, char *filename) {

  struct _fsearch *searches = fport->fsearchs;
  if (searches == NULL) {
    fport->nfsearch = 0;
    fport->fsearchs = (searches = malloc(sizeof(struct _fsearch)));
qfile_create:
    searches->qfile = NULL;
    if (filename != NULL)
      searches->qfile = strdup(filename);
    searches->q_results = malloc(sizeof(struct _results));
    searches->q_results->qbits = 0;
    searches->n_qr = 0;
    searches->qdx = 0;
    searches->changed_id = 0;
    searches->dirtyo = INT_MAX;
    return searches;
  }
  int idx = 0;
  char *fName = filename;
  do {
    char *sName = (&searches[idx])->qfile;
    if ((sName == NULL) && (fName == NULL))  break;
    if ((sName != NULL) && (fName != NULL))
      if (strcmp(sName, fName) == 0)  break;
    if ((++idx) > fport->nfsearch) {
      struct _fsearch *newfs;
      newfs = realloc(fport->fsearchs,
                         ((idx + 1) * sizeof(struct _fsearch)));
      if (newfs == NULL) {
        puts("realloc failed: findport_results_for");
        return NULL;
      }
      fport->nfsearch++;
      fport->fsearchs = newfs;
      searches = &newfs[idx];
      goto qfile_create;
    }
  } while (1);
  return searches;
}

static void
findport_reset_search_data(struct _fsearch *search) {
    // clear tags, then result data
  if (search->search_string != NULL) {
    free(search->search_string);
    search->search_string = NULL;
  }
  if (search->n_qr != 0)
    search->q_results
                = realloc(search->q_results, sizeof(struct _results));
  search->q_results->qbits = 0;
  search->n_qr = 0;
  search->qdx = 0;
}

static int
findport_search_update_from(LCIFindPort *fport, int start) {

  LCIEditorArea *ea = fport->session->editor_area;
  LCITextPort *active = ea->editor_view[0];
  if (active == NULL)  return -1;

  struct _fsearch *searches = findport_results_for(fport, active->filename);
  if (searches == NULL)  return -1;
  if (searches->search_string == NULL)  return -1;

  if (searches->dirtyo < start)  start = searches->dirtyo;
  searches->dirtyo = INT_MAX;

  struct _results *results;
  int mnrdx, majdx, offset;
  majdx = searches->qdx >> 5;
  mnrdx = searches->qdx & ~(1U << 5);
  offset = (searches->q_results + majdx)->qoffsets[mnrdx];
    // set to unknown if destroying
  if (offset > start)
    searches->qdx = 0;

  majdx = searches->n_qr;
  results = searches->q_results + majdx;
    // reguardless if valid, use 0 mnrdx
  offset = results->qoffsets[0];
  while ((offset > start) && (majdx > 0)) {
    results = searches->q_results + (--majdx);
    offset = results->qoffsets[0];
  }
  if (majdx < searches->n_qr) {
    searches->n_qr = majdx;
    struct _results *newPtr;
    newPtr = realloc(searches->q_results,
                      ((searches->n_qr + 1) * sizeof(struct _results)));
    if (newPtr == NULL)  return -1;
    searches->q_results = newPtr;
    results = searches->q_results + searches->n_qr;
  }
  if (searches->n_qr == 0)
    offset = 0;

  mnrdx = 0;
  char *key = searches->search_string;
  size_t key_len = strlen(key);
  GtkTextTag *tag;
  GtkTextIter begin, end;
  GtkTextBuffer *tbuf = GTK_TEXT_BUFFER(active->textbuffer);
  GtkTextTagTable *ttab = gtk_text_buffer_get_tag_table(tbuf);
  if ((tag = gtk_text_tag_table_lookup(ttab, "search_result")) == NULL)
    tag = gtk_text_buffer_create_tag(tbuf, "search_result",
                                   "background", "rgba(255,215,0,0.5)", NULL);
  gtk_text_buffer_get_start_iter(tbuf, &begin);
  GtkTextIter new = begin;
  gtk_text_iter_set_offset(&new, start);
  gtk_text_buffer_get_end_iter(tbuf, &end);
    // clear tags, then result data
  gtk_text_buffer_remove_tag(tbuf, tag, (const GtkTextIter*)&new,
                                        (const GtkTextIter*)&end);

  gtk_text_buffer_get_end_iter(tbuf, &end);
  char *data = gtk_text_buffer_get_text(tbuf, (const GtkTextIter*)&begin,
                                          (const GtkTextIter*)&end, FALSE);

  char *rdPtr = data + offset;
   if ((rdPtr = strstr(rdPtr, key)) == NULL)
    results->qbits = 0;
  else {
    do {
      results->qoffsets[mnrdx] = rdPtr - data;
      gtk_text_buffer_get_iter_at_offset(tbuf, &begin, (rdPtr - data));
      rdPtr += key_len;
      gtk_text_buffer_get_iter_at_offset(tbuf, &end, (rdPtr - data));
      gtk_text_buffer_apply_tag(tbuf, tag, (const GtkTextIter*)&begin,
                                               (const GtkTextIter*)&end);
      if ((rdPtr = strstr(rdPtr, key)) == NULL)  break;
      if ((++mnrdx) == 32) {
        results->qbits = 0xFFFFFFFFU;
        searches->n_qr++;
        struct _results *newPtr;
        newPtr = realloc(searches->q_results,
                          ((searches->n_qr + 1) * sizeof(struct _results)));
        if (newPtr == NULL)  return -1;
        searches->q_results = newPtr;
        results = searches->q_results + searches->n_qr;
        mnrdx = 0;
      }
    } while (1);
    results->qbits = 0xFFFFFFFFU >> (31 - mnrdx);
  }
  int found = findport_display_results(fport, searches);
  g_free(data);
  return found;
}

static unsigned
lsbDeBruijn32(unsigned v) {

  static const unsigned lsbDeBruijn[32] = {
    0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
    31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
  };
  return lsbDeBruijn[((unsigned)((v & -v) * 0x077CB531U)) >> 27];
}

static unsigned
msbDeBruijn32(unsigned v) {

  static const unsigned msbDeBruijn[32] = {
    0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
    8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
  };

  v |= v >> 1; // first round down to one less than a power of 2
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;

  return msbDeBruijn[(unsigned)(v * 0x07C4ACDDU) >> 27];
}

static int
_qresult_maximum(struct _fsearch *searches) {

  int majdx = searches->n_qr;
  struct _results *results = searches->q_results + majdx;
  while (results->qbits == 0) {
      // case: empty set of bits in searches, entered with empty
    if ((--majdx) < 0)  return -1;
    results = searches->q_results + majdx;
  }
  return ((majdx << 5) | msbDeBruijn32(results->qbits));
}

// note: should have different negative error codes
// note: should be range check, alteration may have occured on part of found
static int
findport_update_position(LCIFindPort *fport, int result_direction) {

  LCIEditorArea *ea = fport->session->editor_area;
  LCITextPort *active = ea->editor_view[0];
  if (active == NULL)  return -1;

  struct _fsearch *searches = findport_results_for(fport, active->filename);
  if (searches == NULL)  return -1;
  if (searches->search_string == NULL)  return -1;
  size_t rSz = strlen(searches->search_string);

  GtkTextIter start, end;
  GtkTextBuffer *tbuf = GTK_TEXT_BUFFER(active->textbuffer);
  gtk_text_buffer_get_selection_bounds(tbuf, &start, &end);
  int ins = gtk_text_iter_get_offset((const GtkTextIter*)&start);
  int delta = gtk_text_iter_get_offset((const GtkTextIter*)&end) - ins;

    // reminder: it's a gtk textview ! pheonix
  if (searches->dirtyo != INT_MAX)
    findport_search_update_from(fport, searches->dirtyo);
  _Bool selected = (delta == (int)rSz);
  int offset = _qresult_maximum(searches);
  if (offset < 0)  return -1;
  int majdx = offset >> 5;
  int mnrdx = offset & ~(0xFFFFFFFFU << 5);
  struct _results *results = searches->q_results + majdx;
  offset = results->qoffsets[mnrdx];

  int ret_offset = offset;
  int ret_majdx = majdx,
      ret_mnrdx = mnrdx;

  if (offset <= ins) {
    _Bool selects = (offset == ins) && selected;
    if ( (result_direction == RESULT_LEFT)
         && ( (offset < ins) || ( (offset == ins) && !selects ) ) )
      goto mo_ret;
    if ((offset < ins) || selects) {
      if (result_direction == RESULT_RIGHT) {
          // set exit as min offset result
        ret_majdx = 0;
        results = searches->q_results;
        while (results->qbits == 0)
          results = searches->q_results + (++ret_majdx);
        ret_mnrdx = lsbDeBruijn32(results->qbits);
        ret_offset = results->qoffsets[ret_mnrdx];
        goto mo_ret;
      }
    }
  }
    // note: doesn't care if alters removed offset values
  unsigned bits = results->qbits;
  do {
    if ((bits & (0x00000001U << mnrdx)) != 0) {
      ret_offset = offset;
      ret_majdx = majdx,
      ret_mnrdx = mnrdx;
    }
    if ((--mnrdx) < 0) {
      do {
        if ((--majdx) < 0) {
          if (result_direction == RESULT_LEFT) {
            offset = _qresult_maximum(searches);
            searches->qdx = offset;
            majdx = offset >> 5;
            mnrdx = offset & ~(0xFFFFFFFFU << 5);
            offset = (searches->q_results + majdx)->qoffsets[mnrdx];
            return offset;
          }
          goto mo_ret;
        }
        results = searches->q_results + majdx;
      } while ((bits = results->qbits) == 0);
      mnrdx = msbDeBruijn32(bits);
    }
    offset = results->qoffsets[mnrdx];
  } while (offset > ins);
  if ( (offset == ins) && (result_direction == RESULT_RIGHT)
      && !selected ) {
    ret_offset = offset;
    ret_majdx = majdx,
    ret_mnrdx = mnrdx;
  }
mo_ret:
  if (result_direction == RESULT_LEFT) {
    if (offset == ins) {
      results = searches->q_results + majdx;
      if (((bits = results->qbits) & (0x7FFFFFFF >> (31 - mnrdx))) == 0)
         goto refill;
      do {
        if ((--mnrdx) < 0) {
          do {
      refill:
            if ((--majdx) < 0)  majdx = searches->n_qr;
            results = searches->q_results + majdx;
          } while ((bits = results->qbits) == 0);
          mnrdx = msbDeBruijn32(bits);
          break;
        }
      } while ((bits & (0x00000001U << mnrdx)) == 0);
    }
    ret_offset = results->qoffsets[mnrdx];
    ret_majdx = majdx;
    ret_mnrdx = mnrdx;
  }
  offset = ret_offset;
  majdx = ret_majdx;
  mnrdx = ret_mnrdx;
  searches->qdx = (majdx << 5) + mnrdx;
  return offset;
}

static void
findport_navigate_left(LCIFindPort *fport) {

  LCIEditorArea *ea = fport->session->editor_area;
  LCITextPort *active = ea->editor_view[0];
  if (active == NULL)  return;
  findport_update_position(fport, RESULT_LEFT);
  text_buffer_apply_search_tag(fport,
                               findport_results_for(fport, active->filename));
}

static void
findport_navigate_right(LCIFindPort *fport) {

  LCIEditorArea *ea = fport->session->editor_area;
  LCITextPort *active = ea->editor_view[0];
  if (active == NULL)  return;
  findport_update_position(fport, RESULT_RIGHT);
  text_buffer_apply_search_tag(fport,
                               findport_results_for(fport, active->filename));
}

static void
findport_replace_all(LCIFindPort *fport) {

  LCIEditorArea *ea = fport->session->editor_area;
  LCITextPort *active = ea->editor_view[0];
  if (active == NULL)  return;

  struct _fsearch *searches = findport_results_for(fport, active->filename);
  if (searches == NULL)  return;
  if (searches->search_string == NULL)  return;
  size_t rSz = strlen(searches->search_string);

  PhxObjectTextview *sender;
  sender = (PhxObjectTextview*)fport->objects[textview_replace_box];
    // delta of replacements
  int delta = sender->str_nil - rSz;
  if ((delta == 0) && (strcmp(searches->search_string, sender->string) == 0))
    return;

  GtkTextIter start, end;
  GtkTextBuffer *tbuf = GTK_TEXT_BUFFER(active->textbuffer);
  gtk_text_buffer_get_start_iter(tbuf, &start);
  gtk_text_buffer_get_end_iter(tbuf, &end);
  char *data = gtk_text_buffer_get_text(tbuf, (const GtkTextIter*)&start,
                                          (const GtkTextIter*)&end, FALSE);

    // create an editable buffer, non-drawing
  PhxObjectTextview *buffered;
  buffered = (PhxObjectTextview*)ui_object_create((PhxInterface*)fport,
                                      PHX_TEXTBUFFER, NULL, 0, 0, 0, 0);
  ui_textview_buffer_set(buffered, data);
  g_free(data);

  int mnrdx, majdx = searches->n_qr;
  struct _results *results = searches->q_results + majdx;
  unsigned bits = results->qbits;
  while (bits == 0) {
    if ((--majdx) < 0)  // assumes searches was newly created
      goto clean_up;
    results = searches->q_results + majdx;
    bits = results->qbits;
  }
  mnrdx = msbDeBruijn32(bits);
    // for first one only, verify search results is the correct replacable
  int offset = results->qoffsets[mnrdx];
  buffered->insert.offset = offset;
  buffered->release.offset = offset + rSz;
  if (memcmp(&buffered->string[offset], searches->search_string, rSz) != 0) {
clean_up:
  #if USE_MARKS
    free(buffered->newline_list);
  #endif
    free(buffered->string);
    free(buffered);
    return;
  }
  GtkTextIter s_start, s_end;
  gtk_text_buffer_get_selection_bounds(tbuf, &s_start, &s_end);
  int save_ins_offset = gtk_text_iter_get_offset((const GtkTextIter*)&s_start);
  int save_rel_offset = gtk_text_iter_get_offset((const GtkTextIter*)&s_end);
  do {
    int sndx;
      // need to 'catch' amount locations moved by
    if (offset <= save_ins_offset) {
      if (offset == save_ins_offset) {
        save_ins_offset += sender->str_nil;
        save_rel_offset = save_ins_offset;
      } else {
        save_ins_offset += delta;
        save_rel_offset += delta;
      }
    }
    sndx = mnrdx;
    text_buffer_insert(buffered, sender->string, sender->str_nil);
    if ((--mnrdx) < 0) {
      int sjdx;
refill:
      sjdx = majdx;
      do {
        if (majdx == 0) {
          results = searches->q_results + sjdx;
          mnrdx = sndx;
          goto finally;
        }
        results = searches->q_results + (--majdx);
      } while ((bits = results->qbits) == 0);
      mnrdx = msbDeBruijn32(bits);
    }
    unsigned msk = 0x00000001U << mnrdx;
    while ((bits & msk) == 0) {
      if ( ((bits & (0xFFFFFFFF >> (31 - mnrdx))) == 0)
          || ((msk >>= 1) == 0) ) {
        goto refill;
      }
      --mnrdx;
    }
    offset = results->qoffsets[mnrdx];
    buffered->insert.offset = offset;
    buffered->release.offset = offset + rSz;
  } while (1);
finally:
  findport_reset_search_data(searches);
  text_buffer_reset(buffered);
    // use delete/insert for undo ability
  gtk_text_buffer_begin_user_action(tbuf);
  gtk_text_buffer_delete(tbuf, &start, &end);
  gtk_text_buffer_insert(tbuf, &start, buffered->string, -1);
  gtk_text_buffer_end_user_action(tbuf);
    // user's updated last location
  gtk_text_buffer_get_start_iter(tbuf, &start);
  gtk_text_buffer_get_end_iter(tbuf, &end);
  gtk_text_iter_set_offset(&start, save_ins_offset);
  gtk_text_iter_set_offset(&end, save_rel_offset);
  gtk_text_buffer_select_range(tbuf, (const GtkTextIter*)&start,
                                     (const GtkTextIter*)&end);
  gtk_text_view_scroll_to_iter(
              GTK_TEXT_VIEW(active->textview), &start, 0.0, TRUE, 0.5, 0.5);
  findport_display_results(fport, searches);
    // buffered is history
#if USE_MARKS
  free(buffered->newline_list);
#endif
  free(buffered->string);
  free(buffered);
}

static void
findport_replace(LCIFindPort *fport) {

  LCIEditorArea *ea = fport->session->editor_area;
  LCITextPort *active = ea->editor_view[0];
  if (active == NULL)  return;

  GtkTextIter start, end;
  GtkTextBuffer *tbuf = GTK_TEXT_BUFFER(active->textbuffer);
  gtk_text_buffer_get_selection_bounds(tbuf, &start, &end);
  int ins = gtk_text_iter_get_offset((const GtkTextIter*)&start);
  int tSz = gtk_text_iter_get_offset((const GtkTextIter*)&end) - ins;
  if (tSz != 0) {
    PhxObjectTextview *receiver;
    char *selected = gtk_text_buffer_get_text(tbuf,
                                          (const GtkTextIter*)&start,
                                          (const GtkTextIter*)&end, FALSE);
    receiver = (PhxObjectTextview*)fport->objects[textview_find_box];
       // verify selected text is actual 'find' text
    if ( (memcmp(selected, receiver->string, tSz) == 0)
        && (receiver->string[tSz] == 0) ) {
      PhxObjectTextview *sender;
      sender = (PhxObjectTextview*)fport->objects[textview_replace_box];

      gtk_text_buffer_begin_user_action(tbuf);
      gtk_text_buffer_delete(tbuf, &start, &end);
      gtk_text_buffer_insert(tbuf, &start, sender->string, sender->str_nil);
      gtk_text_buffer_end_user_action(tbuf);

      findport_search_update_from(fport, ins);
      findport_update_position(fport, RESULT_RIGHT);
    }
    g_free(selected);
  }
}

static void
findport_replace_and_find(LCIFindPort *fport) {

  LCIEditorArea *ea = fport->session->editor_area;
  LCITextPort *active = ea->editor_view[0];
  if (active == NULL)  return;

  GtkTextIter start, end;
  GtkTextBuffer *tbuf = GTK_TEXT_BUFFER(active->textbuffer);
  gtk_text_buffer_get_selection_bounds(tbuf, &start, &end);
  int ins = gtk_text_iter_get_offset((const GtkTextIter*)&start);
  int tSz = gtk_text_iter_get_offset((const GtkTextIter*)&end) - ins;

  if (tSz != 0) {
    PhxObjectTextview *receiver;
    char *selected = gtk_text_buffer_get_text(tbuf,
                                          (const GtkTextIter*)&start,
                                          (const GtkTextIter*)&end, FALSE);
    receiver = (PhxObjectTextview*)fport->objects[textview_find_box];
      // verify selected text is actual 'find' text
    if ( (memcmp(selected, receiver->string, tSz) == 0)
        && (receiver->string[tSz] == 0) ) {
      PhxObjectTextview *sender;
      sender = (PhxObjectTextview*)fport->objects[textview_replace_box];

      gtk_text_buffer_begin_user_action(tbuf);
      gtk_text_buffer_delete(tbuf, &start, &end);
      gtk_text_buffer_insert(tbuf, &start, sender->string, sender->str_nil);
      gtk_text_buffer_end_user_action(tbuf);
      if (findport_search_update_from(fport, ins) != 0)
        findport_navigate_right(fport);
    }
    g_free(selected);
  }
}

/* demo/design/test has only 1 file, and no attribute set up as of yet */
/* demo also does not use gtk textbuffer ether */
void
lci_findport_clear_results(LCIFindPort *fport) {

  struct _fsearch *searches = fport->fsearchs;
  if (searches == NULL)  return;

  LCIEditorArea *ea = fport->session->editor_area;
  int sdx = 0;
  do {
    int idx;
    struct _fsearch *csPtr = &searches[sdx];
    char *sName = csPtr->qfile;
    for (idx = 1; idx < ea->n_view; idx++) {
      char *fName = ea->editor_view[idx]->filename;
      if (strcmp(sName, fName) == 0)  break;
    }
    GtkTextIter start, end;
    GtkTextBuffer *tbuf = GTK_TEXT_BUFFER(ea->editor_view[idx]->textbuffer);
    gtk_text_buffer_get_start_iter(tbuf, &start);
    gtk_text_buffer_get_end_iter(tbuf, &end);
    gtk_text_buffer_remove_tag_by_name(tbuf, "search_result",
                                              (const GtkTextIter*)&start,
                                              (const GtkTextIter*)&end);
    free(csPtr->qfile);
    if (csPtr->search_string != NULL)
      free(csPtr->search_string);
    free(csPtr->q_results);
    g_signal_handler_disconnect(G_OBJECT(tbuf), csPtr->changed_id);
  } while ((++sdx) <= fport->nfsearch);
  free(searches);
  fport->nfsearch = 0;
  fport->fsearchs = NULL;
}

static void
_textbuffer_changed(GtkTextBuffer *tbuf, struct _fsearch *searches) {

  GtkTextIter start;
  gtk_text_buffer_get_selection_bounds(tbuf, &start, NULL);
  int ins = gtk_text_iter_get_offset((const GtkTextIter*)&start);
  if (ins < searches->dirtyo)  searches->dirtyo = ins;
}

void
lci_findport_search(LCIFindPort *fport) {

  LCIEditorArea *ea = fport->session->editor_area;
  LCITextPort *active = ea->editor_view[0];
  if (active == NULL)  return;
    // only place to hook in with gtk
  visible_set((PhxObject*)fport, TRUE);

    // get storage for results for this textview file
  struct _fsearch *searches = findport_results_for(fport, active->filename);
  if (searches == NULL)  return;
// here is where to attach file dirtyo signal, has session info
  if (searches->changed_id == 0)
    searches->changed_id = g_signal_connect(G_OBJECT(active->textbuffer),
                                "changed",
                                G_CALLBACK(_textbuffer_changed),
                                searches);

    // clear tags and data
  findport_reset_search_data(searches);

  PhxObjectTextview *key_object
           = (PhxObjectTextview*)fport->objects[textview_find_box];
  if (key_object->str_nil == 0)  return;

  searches->search_string = strdup(key_object->string);
  int found = findport_search_update_from(fport, 0);
  if (found < 0)
    puts("failed error: findport_search_update_from");
  if (found <= 0)  return;

  struct _results *results;
  GtkTextIter start;
  GtkTextBuffer *tbuf = GTK_TEXT_BUFFER(active->textbuffer);
  gtk_text_buffer_get_selection_bounds(tbuf, &start, NULL);
  int ins = gtk_text_iter_get_offset((const GtkTextIter*)&start);
  int majdx = searches->n_qr;
  int mnrdx = 0;
  results = searches->q_results + majdx;
  int offset = results->qoffsets[0];
  while ((offset > ins) && (majdx > 0)) {
    results = searches->q_results + (--majdx);
    offset = results->qoffsets[0];
  }

  if (offset < ins) {
    mnrdx = msbDeBruijn32(results->qbits);
    offset = results->qoffsets[mnrdx];
  }

  int ret_majdx = majdx,
      ret_mnrdx = mnrdx;

  if (offset <= ins)  goto apply_tag;
  do {
    ret_majdx = majdx;
    ret_mnrdx = mnrdx;
    if ((--mnrdx) < 0) {
      if (majdx == 0)  goto apply_tag;
      results = searches->q_results + (--majdx);
      mnrdx = msbDeBruijn32(results->qbits);
    }
    offset = results->qoffsets[mnrdx];
  } while (offset > ins);
  if (offset == ins) {
    ret_majdx = majdx;
    ret_mnrdx = mnrdx;
  }
apply_tag:
  searches->qdx = (ret_majdx << 5) + ret_mnrdx;
  text_buffer_apply_search_tag(fport, searches);
}

void
lci_findport_receiver_text(LCIFindPort *findPort, char ch,
                                                   char *data, size_t sz) {
  PhxObjectTextview *receiver;
  if (ch == 'c') {
    receiver = (PhxObjectTextview*)findPort->has_focus;
    if ((receiver != NULL) && (receiver->type == PHX_ENTRY))
      text_buffer_copy(receiver);
    return;
  }
  if (ch == 'x') {
    receiver = (PhxObjectTextview*)findPort->has_focus;
    if ((receiver != NULL) && (receiver->type == PHX_ENTRY)) {
      text_buffer_cut(receiver);
      cairo_region_t *crr;
      crr = cairo_region_create_rectangle(
               (cairo_rectangle_int_t *)&receiver->draw_box);
      gdk_window_invalidate_region(findPort->parent_window, crr, FALSE);
      cairo_region_destroy(crr);
    }
    return;
  }
  if (ch == 'v') {
    receiver = (PhxObjectTextview*)findPort->has_focus;
    if ((receiver != NULL) && (receiver->type == PHX_ENTRY)) {
      text_buffer_insert(receiver, data, sz);
      cairo_region_t *crr;
      crr = cairo_region_create_rectangle(
               (cairo_rectangle_int_t *)&receiver->draw_box);
      gdk_window_invalidate_region(findPort->parent_window, crr, FALSE);
      cairo_region_destroy(crr);
    }
  } else {
    int idx = (ch == 'e') ? textview_find_box : textview_replace_box;
    receiver = (PhxObjectTextview*)findPort->objects[idx];
    text_buffer_board_set(receiver, data, sz);
  }
}

#pragma mark *** Main ***

static _Bool
configure_event(PhxInterface *fport, GdkEvent *event, void *widget) {

  (void)widget;
  int width = event->configure.width;
    // silliness of gdk start up?
  if (width <= 1)  return FALSE;
  int w_delta = width - fport->mete_box.w;
  if (fport->mete_box.w != width) {
      // base object 'window' size change
    PhxObjectDrawing *odrw
                         = (PhxObjectDrawing*)fport->objects[0];
    ui_box_inset(&odrw->mete_box, 0, 0, -w_delta, 0);
    ui_box_inset(&odrw->draw_box, 0, 0, -w_delta, 0);
      // alter textview variables, move 'Done' x postion
      // since offseting, change right to opposite left
    PhxObjectButton *obtn = (PhxObjectButton*)fport->objects[close0_box];
    ui_box_inset(&obtn->mete_box, w_delta, 0, -w_delta, 0);
    ui_box_inset(&obtn->draw_box, w_delta, 0, -w_delta, 0);
    ui_box_inset(&obtn->child->mete_box, w_delta, 0, -w_delta, 0);
    ui_box_inset(&obtn->child->draw_box, w_delta, 0, -w_delta, 0);
    obtn = (PhxObjectButton*)fport->objects[close1_box];
    ui_box_inset(&obtn->mete_box, w_delta, 0, -w_delta, 0);
    ui_box_inset(&obtn->draw_box, w_delta, 0, -w_delta, 0);
    ui_box_inset(&obtn->child->mete_box, w_delta, 0, -w_delta, 0);
    ui_box_inset(&obtn->child->draw_box, w_delta, 0, -w_delta, 0);

    PhxObjectTextview *otxt
                = (PhxObjectTextview*)fport->objects[textview_find_box];
    ui_box_inset(&otxt->mete_box, 0, 0, -w_delta, 0);
    ui_box_inset(&otxt->draw_box, 0, 0, -w_delta, 0);
    ui_box_inset(&otxt->bin, 0, 0, -w_delta, 0);

    otxt = (PhxObjectTextview*)fport->objects[textview_replace_box];
    ui_box_inset(&otxt->mete_box, 0, 0, -w_delta, 0);
    ui_box_inset(&otxt->draw_box, 0, 0, -w_delta, 0);
    ui_box_inset(&otxt->bin, 0, 0, -w_delta, 0);
  }
  fport->mete_box.w = width;

  int height = event->configure.height;
  int h_delta = height - fport->mete_box.h;
  if (fport->mete_box.h != height) {
    PhxObjectDrawing *odrw
                         = (PhxObjectDrawing*)fport->objects[0];
    ui_box_inset(&odrw->mete_box, 0, 0, 0, h_delta);
    ui_box_inset(&odrw->draw_box, 0, 0, 0, h_delta);
  }
  fport->mete_box.h = height;

  if ( (w_delta != 0) || (h_delta != 0) )
    ui_interface_refresh(fport);

  return FALSE;
}

static void
btn_choose_event(PhxInterface *iface, GdkEvent *event, PhxObject *obj) {
    // reguardless of choice, both choices state 'Find'
  if (event->type != GDK_BUTTON_PRESS)  return;
  LCIFindPort *fport = (LCIFindPort*)iface;
  lci_findport_search(fport);
  ui_bank_combo_run(iface, event, obj);
}

static void
btn_replace_all_event(PhxInterface *iface, GdkEvent *event, PhxObject *obj) {
  if (event->type != GDK_BUTTON_PRESS)  return;
  LCIFindPort *fport = (LCIFindPort*)iface;
  findport_replace_all(fport);
}

static void
btn_replace_event(PhxInterface *iface, GdkEvent *event, PhxObject *obj) {
  if (event->type != GDK_BUTTON_PRESS)  return;
  LCIFindPort *fport = (LCIFindPort*)iface;
  findport_replace(fport);
}

static void
btn_replace_find_event(PhxInterface *iface, GdkEvent *event, PhxObject *obj) {
  if (event->type != GDK_BUTTON_PRESS)  return;
  LCIFindPort *fport = (LCIFindPort*)iface;
  findport_replace_and_find(fport);
}

static void
btn_close_event(PhxInterface *iface, GdkEvent *event, PhxObject *obj) {
  if (event->type != GDK_BUTTON_RELEASE)  return;
  LCIFindPort *fport = (LCIFindPort*)iface;
  lci_findport_clear_results(fport);
  gtk_widget_hide(fport->session->editor_area->find_window);
  visible_set((PhxObject*)iface, FALSE);
}

static void
btn_navigate_right_event(PhxInterface *iface, GdkEvent *event, PhxObject *obj) {
  if (event->type != GDK_BUTTON_PRESS)  return;
  LCIFindPort *fport = (LCIFindPort*)iface;
  findport_navigate_right(fport);
}

static void
btn_navigate_left_event(PhxInterface *iface, GdkEvent *event, PhxObject *obj) {
  if (event->type != GDK_BUTTON_PRESS)  return;
  LCIFindPort *fport = (LCIFindPort*)iface;
  findport_navigate_left(fport);
}

static void
fw_initialize(PhxInterface *fport) {

  PhxObjectButton   *obtn;
  PhxObjectTextview *otxt;
  PhxObjectLabel    *olbl;

  int xpos = 0;
  int box_height = BOX_HEIGHT;

  int mdx = (box_height < 21) ? 0 : 1;
  int bbm = window_adjustments[mdx][2];

                        /* Combo Button [0,0] */
  obtn = (PhxObjectButton*)ui_object_create(fport,
                                     PHX_BUTTON_COMBO, draw_button,
                                     xpos, 0, fport->mete_box.w, box_height);
  ui_box_inset(&obtn->draw_box, bbm, bbm, bbm, bbm);
  ui_bank_create(PHX_BANK_COMBO, obtn, result_cb, 2, "Find", "Find & Replace");
  obtn->_event_cb = btn_choose_event;
  ui_interface_map(fport, (PhxObject*)obtn);

// want decrease in between button drawing, vertical alter mete location
// problem later if create mete_box clip
  int alter_y = bbm + 1;
  int alter_x = bbm;

                        /* Simple Button [1,0] */
    // want this button size same as the combo button's, consider as max,min
  int combo_width = obtn->mete_box.w;
  obtn = (PhxObjectButton*)ui_object_create(fport,
                        PHX_BUTTON_LABELED, draw_button,
                        xpos, (box_height - alter_y), combo_width, box_height);
  ui_box_inset(&obtn->draw_box, bbm, bbm, bbm, bbm);
  int whtsp = ui_button_label_create(fport, obtn, "Replace All", HJST_CTR);
  obtn->_event_cb = btn_replace_all_event;
  ui_interface_map(fport, (PhxObject*)obtn);
  visible_set((PhxObject*)obtn, FALSE);

                        /* Simple Button [1,1] */
  xpos += obtn->mete_box.w - alter_x;
    // using 'combo_width' as starting size, the max
  obtn = (PhxObjectButton*)ui_object_create(fport,
                        PHX_BUTTON_LABELED, draw_button,
                        xpos, (box_height - alter_y), combo_width, box_height);
  ui_box_inset(&obtn->draw_box, bbm, bbm, bbm, bbm);
    // want adjustments to width, used prior width to get difference
  whtsp =
       (ui_button_label_create(fport, obtn, "Replace", HJST_CTR) - whtsp) * 2;
  ui_box_inset(&obtn->child->draw_box, 0, 0, whtsp, 0);
//  ui_box_inset(&obtn->child->bin,      0, 0, whtsp, 0);
  ui_box_inset(&obtn->draw_box,        0, 0, whtsp, 0);
  ui_box_inset(&obtn->mete_box,        0, 0, whtsp, 0);
  obtn->_event_cb = btn_replace_event;
  ui_interface_map(fport, (PhxObject*)obtn);
  visible_set((PhxObject*)obtn, FALSE);

                        /* Simple Button [1,2] */
  xpos += obtn->mete_box.w - alter_x;
    // this button same size as column 0
  obtn = (PhxObjectButton*)ui_object_create(fport,
                        PHX_BUTTON_LABELED, draw_button,
                        xpos, (box_height - alter_y), combo_width, box_height);
  ui_box_inset(&obtn->draw_box, bbm, bbm, bbm, bbm);
  ui_button_label_create(fport, obtn, "Replace & Find", HJST_CTR);
  obtn->_event_cb = btn_replace_find_event;
  ui_interface_map(fport, (PhxObject*)obtn);
  visible_set((PhxObject*)obtn, FALSE);

                        /* Simple Button [1,4] */
    // last in row, right justified
    // xpos: keep for textview start
  xpos += obtn->mete_box.w;
    // this starts at fport width ends at -close width
  obtn = (PhxObjectButton*)ui_object_create(fport,
                     PHX_BUTTON_LABELED, draw_button,
                     (fport->mete_box.w - combo_width), (box_height - alter_y),
                     combo_width, box_height);
  ui_box_inset(&obtn->draw_box, bbm, bbm, bbm, bbm);
  whtsp = ui_button_label_create(fport, obtn, "Done", HJST_CTR);
    // desired size change => draw_box.w - text_width
  whtsp = obtn->child->draw_box.w - (whtsp + 6);
  ui_box_inset(&obtn->child->draw_box, whtsp, 0, -whtsp, 0);
  ui_box_inset(&obtn->draw_box, (2 * whtsp), 0, 0, 0);
  ui_box_inset(&obtn->mete_box, (2 * whtsp), 0, 0, 0);
  obtn->_event_cb = btn_close_event;
  ui_interface_map(fport, (PhxObject*)obtn);
  visible_set((PhxObject*)obtn, FALSE);

                        /* Simple Button [0,4] */
  int done_width = obtn->mete_box.w;
  obtn = (PhxObjectButton*)ui_object_create(fport,
                     PHX_BUTTON_LABELED, draw_button,
                  (fport->mete_box.w - done_width), 0, done_width, box_height);
  ui_box_inset(&obtn->draw_box, bbm, bbm, bbm, bbm);
  ui_button_label_create(fport, obtn, "Done", HJST_CTR);
  obtn->_event_cb = btn_close_event;
  ui_interface_map(fport, (PhxObject*)obtn);

                        /* Textview [1,3] */
  otxt = (PhxObjectTextview*)ui_object_create(fport, PHX_ENTRY, draw_textview,
                         xpos, box_height - 1,
                         obtn->mete_box.x - xpos, (box_height - (2 * bbm)));
  ui_box_inset(&otxt->draw_box, 1, 1, 1, 1);
  otxt->glyph_widths = ui_textview_font_set(otxt, otxt->draw_box.h);
  
  ui_textview_buffer_set(otxt, "replacing_text");
  ui_interface_map(fport, (PhxObject*)otxt);
  visible_set((PhxObject*)otxt, FALSE);

                        /* Textview [0,3] */
  otxt = (PhxObjectTextview*)ui_object_create(fport, PHX_ENTRY, draw_textview,
                         xpos, bbm,
                         obtn->mete_box.x - xpos, (box_height - (2 * bbm)));
  ui_box_inset(&otxt->draw_box, 1, 1, 1, 1);
  otxt->glyph_widths = ui_textview_font_set(otxt, otxt->draw_box.h);
  ui_textview_buffer_set(otxt, "searched_text");
  ui_interface_map(fport, (PhxObject*)otxt);

                        /* Navigation Button [0,2] */
                        /* Navigation Button [0,2.5] */
    // should be 2 square buttons joined, child is drawing instead of label
  xpos = otxt->mete_box.x;
  obtn = (PhxObjectButton*)ui_object_create(fport,
                                             PHX_NAVIGATE_RIGHT, draw_navigate,
                                             (xpos - (box_height - bbm)), 0,
                                             (box_height - bbm), box_height);
  ui_box_inset(&obtn->draw_box, 0, bbm, bbm, bbm);
  obtn->_event_cb = btn_navigate_right_event;
  ui_interface_map(fport, (PhxObject*)obtn);
  sensitive_set((PhxObject*)obtn, FALSE);
  xpos = obtn->mete_box.x;
  obtn = (PhxObjectButton*)ui_object_create(fport,
                                             PHX_NAVIGATE_LEFT, draw_navigate,
                                             (xpos - (box_height - bbm)), 0,
                                             (box_height - bbm), box_height);
  ui_box_inset(&obtn->draw_box, bbm, bbm, 0, bbm);
  sensitive_set((PhxObject*)obtn, FALSE);
  obtn->_event_cb = btn_navigate_left_event;
  ui_interface_map(fport, (PhxObject*)obtn);

                        /* Label [0,1] */
  xpos = obtn->mete_box.x;
  int cbw = fport->objects[choose_box]->mete_box.w;
  olbl = (PhxObjectLabel*)ui_object_create(fport, PHX_LABEL, draw_label,
                                           cbw, 0, (xpos - cbw), box_height);
  ui_box_inset(&olbl->draw_box, bbm, box_height/3.5, bbm, bbm);
  ui_label_set(olbl, "0 found matches", HJST_RGT);
  ui_interface_map(fport, (PhxObject*)olbl);
}

/* Add a second box to 'editor_box' container.
 * Need container to be 'find_box' and editor_box'.
 * Keep 'editor_box' as refering to viewport container for
 * editor, used as that throughout code. Since GTK uses
 * box as sub-class of container, refer to old editor_box as
 * editor_container, which will house boxes as needed, but always
 * be container for 'editor_box'. */

GtkWidget *
lci_findport_create(LCISession *session) {

  if (BOX_HEIGHT < 14) {
    puts("Can not honor height request < 14.");
    return NULL;
  }

  LCIEditorArea *ea = session->editor_area;
  GtkWidget *eb = ea->editor_box;

    // use internal until figure out global set up
  GdkDisplay *display = gdk_display_get_default();
  clipboard
        = gtk_clipboard_get_for_display(display, GDK_SELECTION_CLIPBOARD);

  int window_height = BOX_HEIGHT;
  int min_width = (int)((double)BOX_HEIGHT / .0406);
  int window_width = gtk_widget_get_allocated_width(eb);
  if (window_width < min_width)
    puts("Forced to run as a dialog window due to requested width.");
  int idx = (BOX_HEIGHT < 21) ? 0 : 1;
  int two_row_height = (window_height * 2) + window_adjustments[idx][1];

  GtkWidget *find_window = gtk_drawing_area_new();
    // keep port, even when inactive, hidden
  g_object_ref(G_OBJECT(find_window));
  gtk_widget_set_size_request(find_window, min_width, window_height);
  gtk_widget_set_hexpand(find_window, TRUE);

    // NOTE: all textviews are packed at end
  gtk_box_pack_start(GTK_BOX(eb), find_window, FALSE, FALSE, 0);

    // get attched to gdk
  gtk_widget_realize(find_window);
    // create
  LCIFindPort *findPort
          = (LCIFindPort*)ui_interface_create((GtkDrawingArea*)find_window,
                                        0, 0, window_width, two_row_height);
  ea->findport = findPort;
  visible_set((PhxObject*)findPort, FALSE);
  ea->find_window = find_window;
  findPort->session = session;
  fw_initialize((PhxInterface*)findPort);
  findPort->_configure_event = configure_event;

  return find_window;
}

